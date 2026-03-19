#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include <emmintrin.h>

#define THREADS 4

#define kR 9798
#define kG 19235
#define kB 3735

typedef struct { int width, height; uint8_t *data; } Image;

static double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static Image read_ppm(const char *path) {
    Image img = {0};
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); exit(1); }
    char magic[3]; int maxval;
    if (fscanf(f, "%2s %d %d %d", magic, &img.width, &img.height, &maxval) != 4 ||
        magic[0] != 'P' || magic[1] != '6') {
        fprintf(stderr, "Only P6 PPM supported\n"); exit(1);
    }
    fgetc(f);
    size_t n = (size_t)img.width * img.height * 3;
    img.data = malloc(n);
    if (!img.data) { perror("malloc"); exit(1); }
    if (fread(img.data, 1, n, f) != n) { fprintf(stderr, "fread\n"); exit(1); }
    fclose(f);
    return img;
}

static void write_ppm(const char *path, const Image *img) {
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); exit(1); }
    fprintf(f, "P6\n%d %d\n255\n", img->width, img->height);
    fwrite(img->data, 1, (size_t)img->width * img->height * 3, f);
    fclose(f);
}

static void grayscale_scalar(const uint8_t *src, uint8_t *dst, size_t npixels) {
    for (size_t i = 0; i < npixels; i++) {
        uint8_t gray = (uint8_t)((kR*src[i*3] + kG*src[i*3+1] + kB*src[i*3+2]) >> 15);
        dst[i*3] = dst[i*3+1] = dst[i*3+2] = gray;
    }
}

static void grayscale_simd(const uint8_t *src, uint8_t *dst, size_t npixels) {
    const __m128i coeff = _mm_set_epi16(0,kB,kG,kR, 0,kB,kG,kR);
    const __m128i zero  = _mm_setzero_si128();
    size_t i = 0;

    for (; i + 4 <= npixels; i += 4) {
        const uint8_t *s = src + i*3;

        __m128i raw = _mm_loadu_si128((const __m128i *)s);
        __m128i v01 = _mm_set_epi16(0,s[5],s[4],s[3], 0,s[2],s[1],s[0]);
        __m128i v23 = _mm_set_epi16(0,s[11],s[10],s[9], 0,s[8],s[7],s[6]);

        int32_t t01[4], t23[4];
        _mm_storeu_si128((__m128i *)t01, _mm_madd_epi16(v01, coeff));
        _mm_storeu_si128((__m128i *)t23, _mm_madd_epi16(v23, coeff));

        uint8_t g[4];
        g[0] = (uint8_t)((t01[0]+t01[1]) >> 15);
        g[1] = (uint8_t)((t01[2]+t01[3]) >> 15);
        g[2] = (uint8_t)((t23[0]+t23[1]) >> 15);
        g[3] = (uint8_t)((t23[2]+t23[3]) >> 15);

        for (int p = 0; p < 4; p++)
            dst[(i+p)*3] = dst[(i+p)*3+1] = dst[(i+p)*3+2] = g[p];
    }
    for (; i < npixels; i++) {
        uint8_t gray = (uint8_t)((kR*src[i*3] + kG*src[i*3+1] + kB*src[i*3+2]) >> 15);
        dst[i*3] = dst[i*3+1] = dst[i*3+2] = gray;
    }
}

typedef struct { const uint8_t *src; uint8_t *dst; size_t start, end; } ChunkArgs;

static void *mt_worker(void *arg) {
    ChunkArgs *a = (ChunkArgs *)arg;
    grayscale_scalar(a->src + a->start*3, a->dst + a->start*3, a->end - a->start);
    return NULL;
}

static void grayscale_mt(const uint8_t *src, uint8_t *dst, size_t npixels, int nthreads) {
    pthread_t threads[nthreads];
    ChunkArgs args[nthreads];
    size_t chunk = npixels / nthreads;
    for (int t = 0; t < nthreads; t++) {
        args[t].src   = src;
        args[t].dst   = dst;
        args[t].start = (size_t)t * chunk;
        args[t].end   = (t == nthreads-1) ? npixels : args[t].start + chunk;
        pthread_create(&threads[t], NULL, mt_worker, &args[t]);
    }
    for (int t = 0; t < nthreads; t++) pthread_join(threads[t], NULL);
}

static void *simd_mt_worker(void *arg) {
    ChunkArgs *a = (ChunkArgs *)arg;
    grayscale_simd(a->src + a->start*3, a->dst + a->start*3, a->end - a->start);
    return NULL;
}

static void grayscale_simd_mt(const uint8_t *src, uint8_t *dst, size_t npixels, int nthreads) {
    pthread_t threads[nthreads];
    ChunkArgs args[nthreads];
    size_t chunk = npixels / nthreads;
    for (int t = 0; t < nthreads; t++) {
        args[t].src   = src;
        args[t].dst   = dst;
        args[t].start = (size_t)t * chunk;
        args[t].end   = (t == nthreads-1) ? npixels : args[t].start + chunk;
        pthread_create(&threads[t], NULL, simd_mt_worker, &args[t]);
    }
    for (int t = 0; t < nthreads; t++) pthread_join(threads[t], NULL);
}

static int verify(const uint8_t *ref, const uint8_t *out, size_t nbytes) {
    for (size_t i = 0; i < nbytes; i++)
        if (ref[i] != out[i]) return 0;
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc < 2) { fprintf(stderr, "Usage: %s input.ppm\n", argv[0]); return 1; }

    Image src = read_ppm(argv[1]);
    size_t npixels = (size_t)src.width * src.height;
    size_t nbytes  = npixels * 3;

    uint8_t *out_scalar  = malloc(nbytes);
    uint8_t *out_simd    = malloc(nbytes);
    uint8_t *out_mt      = malloc(nbytes);
    uint8_t *out_simd_mt = malloc(nbytes);
    if (!out_scalar || !out_simd || !out_mt || !out_simd_mt) { perror("malloc"); return 1; }

    printf("Image size:   %d x %d\n", src.width, src.height);
    printf("Threads used: %d\n\n", THREADS);

    double t0, t1;

    t0 = now_sec(); grayscale_scalar(src.data, out_scalar, npixels);
    t1 = now_sec();
    printf("Scalar time: %.3f sec\n", t1-t0);

    t0 = now_sec(); grayscale_simd(src.data, out_simd, npixels);
    t1 = now_sec();
    printf("SIMD time: %.3f sec\n", t1-t0);

    t0 = now_sec(); grayscale_mt(src.data, out_mt, npixels, THREADS);
    t1 = now_sec();
    printf("MT time: %.3f sec\n", t1-t0);

    t0 = now_sec(); grayscale_simd_mt(src.data, out_simd_mt, npixels, THREADS);
    t1 = now_sec();
    printf("SIMD + MT time:      %.3f sec\n", t1-t0);

    int ok = verify(out_scalar, out_simd,    nbytes) &&
             verify(out_scalar, out_mt,      nbytes) &&
             verify(out_scalar, out_simd_mt, nbytes);
    printf("\nVerification: %s\n", ok ? "PASSED" : "FAILED");

    Image out_img = { src.width, src.height, out_scalar };
    write_ppm("gray_output.ppm", &out_img);
    printf("Output image: gray_output.ppm\n");

    free(src.data);
    free(out_scalar); free(out_simd); free(out_mt); free(out_simd_mt);
    return 0;
}