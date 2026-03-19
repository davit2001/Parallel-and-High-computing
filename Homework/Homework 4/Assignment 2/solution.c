#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <emmintrin.h>

#define BUF_SIZE_MB 256
#define BUF_SIZE ((size_t)(BUF_SIZE_MB) * 1024 * 1024)
#define NUM_THREADS 4

static double elapsed(struct timespec a, struct timespec b) {
    return (b.tv_sec - a.tv_sec) + (b.tv_nsec - a.tv_nsec) * 1e-9;
}

static void fill_buffer(char *buf, size_t n) {
    static const char cs[] =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789 !.,;:?-_()[]{}";
    int len = sizeof(cs) - 1;
    srandom(42);
    for (size_t i = 0; i < n; i++)
        buf[i] = cs[random() % len];
}

typedef struct { char *buf; size_t start, end; } ChunkArgs;

static void *mt_worker(void *arg) {
    ChunkArgs *a = (ChunkArgs *)arg;
    for (size_t i = a->start; i < a->end; i++)
        if (a->buf[i] >= 'a' && a->buf[i] <= 'z')
            a->buf[i] -= 32;
    return NULL;
}

static void mt_toupper(char *buf, size_t n, int nthreads) {
    pthread_t threads[nthreads];
    ChunkArgs args[nthreads];
    size_t chunk = n / nthreads;
    for (int t = 0; t < nthreads; t++) {
        args[t].buf   = buf;
        args[t].start = (size_t)t * chunk;
        args[t].end   = (t == nthreads-1) ? n : args[t].start + chunk;
        pthread_create(&threads[t], NULL, mt_worker, &args[t]);
    }
    for (int t = 0; t < nthreads; t++)
        pthread_join(threads[t], NULL);
}

static void simd_toupper(char *buf, size_t n) {
    const __m128i low_a  = _mm_set1_epi8('a' - 1);
    const __m128i thresh = _mm_set1_epi8(127);
    const __m128i offset = _mm_set1_epi8((char)(128 - 'z' - 1));
    const __m128i bit5   = _mm_set1_epi8(0x20);
    size_t i = 0, vec_n = n & ~(size_t)15;
    for (; i < vec_n; i += 16) {
        __m128i data = _mm_loadu_si128((const __m128i *)(buf + i));
        __m128i mask = _mm_and_si128(
            _mm_cmpgt_epi8(data, low_a),
            _mm_cmpgt_epi8(thresh, _mm_add_epi8(data, offset)));
        data = _mm_sub_epi8(data, _mm_and_si128(mask, bit5));
        _mm_storeu_si128((__m128i *)(buf + i), data);
    }
    for (; i < n; i++)
        if (buf[i] >= 'a' && buf[i] <= 'z')
            buf[i] -= 32;
}

static void *simd_mt_worker(void *arg) {
    ChunkArgs *a = (ChunkArgs *)arg;
    simd_toupper(a->buf + a->start, a->end - a->start);
    return NULL;
}

static void simd_mt_toupper(char *buf, size_t n, int nthreads) {
    pthread_t threads[nthreads];
    ChunkArgs args[nthreads];
    size_t chunk = n / nthreads;
    for (int t = 0; t < nthreads; t++) {
        args[t].buf   = buf;
        args[t].start = (size_t)t * chunk;
        args[t].end   = (t == nthreads-1) ? n : args[t].start + chunk;
        pthread_create(&threads[t], NULL, simd_mt_worker, &args[t]);
    }
    for (int t = 0; t < nthreads; t++)
        pthread_join(threads[t], NULL);
}

int main(void) {
    char *src         = malloc(BUF_SIZE);
    char *buf_mt      = malloc(BUF_SIZE);
    char *buf_simd    = malloc(BUF_SIZE);
    char *buf_simd_mt = malloc(BUF_SIZE);
    if (!src || !buf_mt || !buf_simd || !buf_simd_mt) { perror("malloc"); return 1; }

    fill_buffer(src, BUF_SIZE);
    memcpy(buf_mt,      src, BUF_SIZE);
    memcpy(buf_simd,    src, BUF_SIZE);
    memcpy(buf_simd_mt, src, BUF_SIZE);

    printf("Buffer size:  %d MB\n", BUF_SIZE_MB);
    printf("Threads used: %d\n\n", NUM_THREADS);

    struct timespec t0, t1;

    clock_gettime(CLOCK_MONOTONIC, &t0);
    mt_toupper(buf_mt, BUF_SIZE, NUM_THREADS);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    printf("Multithreading time: %.3f sec\n", elapsed(t0, t1));

    clock_gettime(CLOCK_MONOTONIC, &t0);
    simd_toupper(buf_simd, BUF_SIZE);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    printf("SIMD time: %.3f sec\n", elapsed(t0, t1));

    clock_gettime(CLOCK_MONOTONIC, &t0);
    simd_mt_toupper(buf_simd_mt, BUF_SIZE, NUM_THREADS);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    printf("SIMD + MT time: %.3f sec\n", elapsed(t0, t1));

    free(src); free(buf_mt); free(buf_simd); free(buf_simd_mt);
    return 0;
}