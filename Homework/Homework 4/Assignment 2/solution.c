#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include <arm_neon.h>

#define BUF_SIZE_MB 256
#define BUF_SIZE ((size_t)(BUF_SIZE_MB) * 1024 * 1024)
#define NUM_THREADS 4

static double elapsed_sec(struct timespec a, struct timespec b) {
    return (b.tv_sec - a.tv_sec) + (b.tv_nsec - a.tv_nsec) * 1e-9;
}

static void fill_buffer(char *buf, size_t n) {
    static const char charset[] =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789 !.,;:?-_()[]{}";
    uint64_t rng = 0xdeadbeefcafe1234ULL;
    for (size_t i = 0; i < n; i++) {
        rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17;
        buf[i] = charset[rng % (sizeof(charset) - 1)];
    }
}

static void scalar_toupper(char *buf, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (buf[i] >= 'a' && buf[i] <= 'z')
            buf[i] -= 32;
    }
}

typedef struct {
    char *buf;
    size_t start, end;
} ChunkArgs;

static void *mt_worker(void *arg) {
    ChunkArgs *a = (ChunkArgs *)arg;
    for (size_t i = a->start; i < a->end; i++) {
        if (a->buf[i] >= 'a' && a->buf[i] <= 'z')
            a->buf[i] -= 32;
    }
    return NULL;
}

static void mt_toupper(char *buf, size_t n, int nthreads) {
    pthread_t threads[nthreads];
    ChunkArgs args[nthreads];
    size_t chunk = n / nthreads;
    for (int t = 0; t < nthreads; t++) {
        args[t].buf = buf;
        args[t].start = (size_t)t * chunk;
        args[t].end = (t == nthreads-1) ? n : args[t].start + chunk;
        pthread_create(&threads[t], NULL, mt_worker, &args[t]);
    }
    for (int t = 0; t < nthreads; t++) pthread_join(threads[t], NULL);
}

static void simd_toupper(char *buf, size_t n) {
    const uint8x16_t lower_a = vdupq_n_u8('a');
    const uint8x16_t lower_z = vdupq_n_u8('z');
    const uint8x16_t diff = vdupq_n_u8(32);

    size_t vec_n = n & ~(size_t)15;
    size_t i = 0;

    for (; i < vec_n; i += 16) {
        uint8x16_t data = vld1q_u8((uint8_t *)(buf + i));
        uint8x16_t is_lower = vandq_u8(
            vcgeq_u8(data, lower_a),
            vcleq_u8(data, lower_z)
        );
        uint8x16_t mask = vandq_u8(is_lower, diff);
        data = vsubq_u8(data, mask);
        vst1q_u8((uint8_t *)(buf + i), data);
    }

    for (; i < n; i++) {
        if (buf[i] >= 'a' && buf[i] <= 'z')
            buf[i] -= 32;
    }
}

typedef struct {
    char *buf;
    size_t start, end;
} SimdChunkArgs;

static void *simd_mt_worker(void *arg) {
    SimdChunkArgs *a = (SimdChunkArgs *)arg;
    simd_toupper(a->buf + a->start, a->end - a->start);
    return NULL;
}

static void simd_mt_toupper(char *buf, size_t n, int nthreads) {
    pthread_t threads[nthreads];
    SimdChunkArgs args[nthreads];
    size_t chunk = n / nthreads;
    for (int t = 0; t < nthreads; t++) {
        args[t].buf = buf;
        args[t].start = (size_t)t * chunk;
        args[t].end = (t == nthreads-1) ? n : args[t].start + chunk;
        pthread_create(&threads[t], NULL, simd_mt_worker, &args[t]);
    }
    for (int t = 0; t < nthreads; t++) pthread_join(threads[t], NULL);
}

static int verify(const char *a, const char *b, size_t n) {
    for (size_t i = 0; i < n; i++)
        if (a[i] != b[i]) return 0;
    return 1;
}

int main(void) {
    printf("Allocating buffers (%d MB each)...\n", BUF_SIZE_MB);

    char *src = malloc(BUF_SIZE);
    char *buf_mt = malloc(BUF_SIZE);
    char *buf_simd = malloc(BUF_SIZE);
    char *buf_simd_mt = malloc(BUF_SIZE);
    char *ref = malloc(BUF_SIZE);

    if (!src || !buf_mt || !buf_simd || !buf_simd_mt || !ref) {
        perror("malloc"); return 1;
    }

    printf("Filling buffer with random characters...\n\n");
    fill_buffer(src, BUF_SIZE);

    memcpy(ref, src, BUF_SIZE);
    scalar_toupper(ref, BUF_SIZE);

    memcpy(buf_mt, src, BUF_SIZE);
    memcpy(buf_simd, src, BUF_SIZE);
    memcpy(buf_simd_mt, src, BUF_SIZE);

    printf("Buffer size:  %d MB\n", BUF_SIZE_MB);
    printf("Threads used: %d\n\n", NUM_THREADS);

    struct timespec t0, t1;

    clock_gettime(CLOCK_MONOTONIC, &t0);
    mt_toupper(buf_mt, BUF_SIZE, NUM_THREADS);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double t_mt = elapsed_sec(t0, t1);
    printf("MT verify: %s\n", verify(buf_mt, ref, BUF_SIZE) ? "OK" : "MISMATCH");

    clock_gettime(CLOCK_MONOTONIC, &t0);
    simd_toupper(buf_simd, BUF_SIZE);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double t_simd = elapsed_sec(t0, t1);
    printf("SIMD verify: %s\n", verify(buf_simd, ref, BUF_SIZE) ? "OK" : "MISMATCH");

    clock_gettime(CLOCK_MONOTONIC, &t0);
    simd_mt_toupper(buf_simd_mt, BUF_SIZE, NUM_THREADS);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double t_simd_mt = elapsed_sec(t0, t1);
    printf("SIMD+MT verify: %s\n\n", verify(buf_simd_mt, ref, BUF_SIZE) ? "OK" : "MISMATCH");

    printf("%-30s %.3f sec\n", "Multithreading time:", t_mt);
    printf("%-30s %.3f sec\n", "SIMD time:", t_simd);
    printf("%-30s %.3f sec\n", "SIMD + Multithreading:", t_simd_mt);

    free(src); free(buf_mt); free(buf_simd); free(buf_simd_mt); free(ref);
    return 0;
}