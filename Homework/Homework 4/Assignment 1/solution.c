#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include <arm_neon.h>

#define DNA_SIZE_MB 256
#define DNA_SIZE ((size_t)(DNA_SIZE_MB) * 1024 * 1024)
#define NUM_THREADS 4

static double elapsed_sec(struct timespec a, struct timespec b) {
    return (b.tv_sec - a.tv_sec) + (b.tv_nsec - a.tv_nsec) * 1e-9;
}

static void generate_dna(char *buf, size_t n) {
    static const char alphabet[4] = {'A','C','G','T'};
    uint64_t rng = 0xdeadbeefcafe1234ULL;
    for (size_t i = 0; i < n; i++) {
        rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17;
        buf[i] = alphabet[rng & 3];
    }
}

static void scalar_count(const char *buf, size_t n,
    long long *cA, long long *cC,
    long long *cG, long long *cT)
{
    long long a=0,c=0,g=0,t=0;
    for (size_t i = 0; i < n; i++) {
        switch (buf[i]) {
            case 'A': a++; break;
            case 'C': c++; break;
            case 'G': g++; break;
            case 'T': t++; break;
        }
    }
    *cA=a; *cC=c; *cG=g; *cT=t;
}

typedef struct {
    const char *buf;
    size_t start, end;
} ChunkArgs;

static long long g_A, g_C, g_G, g_T;
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

static void *mt_worker(void *arg) {
    ChunkArgs *a = (ChunkArgs *)arg;
    long long la=0,lc=0,lg=0,lt=0;
    for (size_t i = a->start; i < a->end; i++) {
        switch (a->buf[i]) {
            case 'A': la++; break;
            case 'C': lc++; break;
            case 'G': lg++; break;
            case 'T': lt++; break;
        }
    }
    pthread_mutex_lock(&g_mutex);
    g_A += la; g_C += lc; g_G += lg; g_T += lt;
    pthread_mutex_unlock(&g_mutex);
    return NULL;
}

static void mt_count(const char *buf, size_t n, int nthreads,
    long long *cA, long long *cC,
    long long *cG, long long *cT)
{
    g_A=g_C=g_G=g_T=0;
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
    *cA=g_A; *cC=g_C; *cG=g_G; *cT=g_T;
}

static void simd_count(const char *buf, size_t n,
    long long *cA, long long *cC,
    long long *cG, long long *cT)
{
    const uint8x16_t vA = vdupq_n_u8('A');
    const uint8x16_t vC = vdupq_n_u8('C');
    const uint8x16_t vG = vdupq_n_u8('G');
    const uint8x16_t vT = vdupq_n_u8('T');

    long long accA=0, accC=0, accG=0, accT=0;

    uint8x16_t sumA8 = vdupq_n_u8(0);
    uint8x16_t sumC8 = vdupq_n_u8(0);
    uint8x16_t sumG8 = vdupq_n_u8(0);
    uint8x16_t sumT8 = vdupq_n_u8(0);

    size_t vec_n = n & ~(size_t)15;
    size_t i = 0;
    int inner = 0;

    for (; i < vec_n; i += 16, inner++) {
        uint8x16_t data = vld1q_u8((const uint8_t *)(buf + i));
        sumA8 = vaddq_u8(sumA8, vandq_u8(vceqq_u8(data, vA), vdupq_n_u8(1)));
        sumC8 = vaddq_u8(sumC8, vandq_u8(vceqq_u8(data, vC), vdupq_n_u8(1)));
        sumG8 = vaddq_u8(sumG8, vandq_u8(vceqq_u8(data, vG), vdupq_n_u8(1)));
        sumT8 = vaddq_u8(sumT8, vandq_u8(vceqq_u8(data, vT), vdupq_n_u8(1)));

        if (inner == 254) {
            accA += vaddlvq_u8(sumA8);
            accC += vaddlvq_u8(sumC8);
            accG += vaddlvq_u8(sumG8);
            accT += vaddlvq_u8(sumT8);
            sumA8 = sumC8 = sumG8 = sumT8 = vdupq_n_u8(0);
            inner = 0;
        }
    }

    accA += vaddlvq_u8(sumA8);
    accC += vaddlvq_u8(sumC8);
    accG += vaddlvq_u8(sumG8);
    accT += vaddlvq_u8(sumT8);

    for (; i < n; i++) {
        switch (buf[i]) {
            case 'A': accA++; break;
            case 'C': accC++; break;
            case 'G': accG++; break;
            case 'T': accT++; break;
        }
    }
    *cA=accA; *cC=accC; *cG=accG; *cT=accT;
}

typedef struct {
    const char *buf;
    size_t start, end;
    long long lA, lC, lG, lT;
} SimdChunkArgs;

static void *simd_mt_worker(void *arg) {
    SimdChunkArgs *a = (SimdChunkArgs *)arg;
    simd_count(a->buf + a->start, a->end - a->start,
               &a->lA, &a->lC, &a->lG, &a->lT);
    return NULL;
}

static void simd_mt_count(const char *buf, size_t n, int nthreads,
    long long *cA, long long *cC,
    long long *cG, long long *cT)
{
    pthread_t threads[nthreads];
    SimdChunkArgs args[nthreads];
    size_t chunk = n / nthreads;
    for (int t = 0; t < nthreads; t++) {
        args[t].buf = buf;
        args[t].start = (size_t)t * chunk;
        args[t].end = (t == nthreads-1) ? n : args[t].start + chunk;
        pthread_create(&threads[t], NULL, simd_mt_worker, &args[t]);
    }
    long long A=0,C=0,G=0,T=0;
    for (int t = 0; t < nthreads; t++) {
        pthread_join(threads[t], NULL);
        A += args[t].lA; C += args[t].lC;
        G += args[t].lG; T += args[t].lT;
    }
    *cA=A; *cC=C; *cG=G; *cT=T;
}

int main(void) {
    printf("Allocating %d MB DNA buffer...\n", DNA_SIZE_MB);
    char *dna = (char *)malloc(DNA_SIZE);
    if (!dna) { perror("malloc"); return 1; }

    printf("Generating random DNA sequence...\n\n");
    generate_dna(dna, DNA_SIZE);

    printf("DNA size: %d MB\n", DNA_SIZE_MB);
    printf("Threads used: %d\n\n", NUM_THREADS);

    struct timespec t0, t1;
    long long A, C, G, T;

    clock_gettime(CLOCK_MONOTONIC, &t0);
    scalar_count(dna, DNA_SIZE, &A, &C, &G, &T);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double t_scalar = elapsed_sec(t0, t1);
    long long refA=A, refC=C, refG=G, refT=T;

    printf("Counts (A C G T):\n");
    printf(" %lld %lld %lld %lld\n\n", A, C, G, T);

    clock_gettime(CLOCK_MONOTONIC, &t0);
    mt_count(dna, DNA_SIZE, NUM_THREADS, &A, &C, &G, &T);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double t_mt = elapsed_sec(t0, t1);
    printf("MT verify: %s\n",
           (A==refA&&C==refC&&G==refG&&T==refT) ? "OK" : "MISMATCH");

    clock_gettime(CLOCK_MONOTONIC, &t0);
    simd_count(dna, DNA_SIZE, &A, &C, &G, &T);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double t_simd = elapsed_sec(t0, t1);
    printf("SIMD verify: %s\n",
           (A==refA&&C==refC&&G==refG&&T==refT) ? "OK" : "MISMATCH");

    clock_gettime(CLOCK_MONOTONIC, &t0);
    simd_mt_count(dna, DNA_SIZE, NUM_THREADS, &A, &C, &G, &T);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double t_simd_mt = elapsed_sec(t0, t1);
    printf("SIMD+MT verify: %s\n\n",
           (A==refA&&C==refC&&G==refG&&T==refT) ? "OK" : "MISMATCH");

    printf("%-35s %.3f sec\n", "Scalar time:", t_scalar);
    printf("%-35s %.3f sec (%.1fx)\n", "Multithreading time:", t_mt,
           t_scalar / t_mt);
    printf("%-35s %.3f sec (%.1fx)\n", "SIMD time:", t_simd,
           t_scalar / t_simd);
    printf("%-35s %.3f sec (%.1fx)\n", "SIMD + Multithreading time:",
           t_simd_mt, t_scalar / t_simd_mt);

    free(dna);
    return 0;
}