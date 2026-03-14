#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include <arm_neon.h>

#define NUM_THREADS 4

typedef struct {
    int width, height, maxval;
    uint8_t *data;
} Image;

static double elapsed_sec(struct timespec a, struct timespec b) {
    return (b.tv_sec - a.tv_sec) + (b.tv_nsec - a.tv_nsec) * 1e-9;
}

static Image *alloc_image(int w, int h) {
    Image *img = malloc(sizeof(Image));
    img->width = w;
    img->height = h;
    img->maxval = 255;
    img->data = malloc((size_t)w * h * 3);
    return img;
}

static void free_image(Image *img) {
    free(img->data);
    free(img);
}

static Image *read_ppm(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); return NULL; }

    char magic[3];
    int w, h, maxval;
    fscanf(f, "%2s %d %d %d", magic, &w, &h, &maxval);
    fgetc(f);

    if (magic[0] != 'P' || magic[1] != '6') {
        fprintf(stderr, "Only P6 PPM supported\n");
        fclose(f); return NULL;
    }

    Image *img = alloc_image(w, h);
    img->maxval = maxval;
    size_t pixels = (size_t)w * h * 3;
    if (fread(img->data, 1, pixels, f) != pixels) {
        fprintf(stderr, "Failed to read pixel data\n");
        free_image(img); fclose(f); return NULL;
    }
    fclose(f);
    return img;
}

static void write_ppm(const char *path, const Image *img) {
    FILE *f = fopen(path, "wb");
    fprintf(f, "P6\n%d %d\n%d\n", img->width, img->height, img->maxval);
    fwrite(img->data, 1, (size_t)img->width * img->height * 3, f);
    fclose(f);
}

static void generate_test_image(const char *path, int w, int h) {
    FILE *f = fopen(path, "wb");
    fprintf(f, "P6\n%d %d\n255\n", w, h);
    uint64_t rng = 0xdeadbeefcafe1234ULL;
    size_t n = (size_t)w * h * 3;
    uint8_t *buf = malloc(n);
    for (size_t i = 0; i < n; i++) {
        rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17;
        buf[i] = rng & 0xFF;
    }
    fwrite(buf, 1, n, f);
    free(buf);
    fclose(f);
}

static void scalar_grayscale(const Image *src, Image *dst) {
    size_t npix = (size_t)src->width * src->height;
    const int wr = (int)(0.299f * 256 + 0.5f);
    const int wg = (int)(0.587f * 256 + 0.5f);
    const int wb = (int)(0.114f * 256 + 0.5f);
    for (size_t i = 0; i < npix; i++) {
        uint8_t r = src->data[i*3];
        uint8_t g = src->data[i*3+1];
        uint8_t b = src->data[i*3+2];
        uint8_t gray = (uint8_t)((wr*r + wg*g + wb*b) >> 8);
        dst->data[i*3]   = gray;
        dst->data[i*3+1] = gray;
        dst->data[i*3+2] = gray;
    }
}

static void simd_grayscale(const Image *src, Image *dst) {
    size_t npix = (size_t)src->width * src->height;

    const uint8x8_t coeff_r = vdup_n_u8((uint8_t)(int)(0.299f*256+0.5f));
    const uint8x8_t coeff_g = vdup_n_u8((uint8_t)(int)(0.587f*256+0.5f));
    const uint8x8_t coeff_b = vdup_n_u8((uint8_t)(int)(0.114f*256+0.5f));

    size_t i = 0;
    for (; i + 8 <= npix; i += 8) {
        uint8x8x3_t px = vld3_u8(src->data + i*3);

        uint16x8_t acc = vmull_u8(px.val[0], coeff_r);
        acc = vmlal_u8(acc, px.val[1], coeff_g);
        acc = vmlal_u8(acc, px.val[2], coeff_b);

        uint8x8_t gray = vshrn_n_u16(acc, 8);

        uint8x8x3_t out;
        out.val[0] = gray;
        out.val[1] = gray;
        out.val[2] = gray;
        vst3_u8(dst->data + i*3, out);
    }

    for (; i < npix; i++) {
        uint8_t r = src->data[i*3];
        uint8_t g = src->data[i*3+1];
        uint8_t b = src->data[i*3+2];
        uint8_t gray = (uint8_t)(0.299f*r + 0.587f*g + 0.114f*b);
        dst->data[i*3]   = gray;
        dst->data[i*3+1] = gray;
        dst->data[i*3+2] = gray;
    }
}

typedef struct {
    const Image *src;
    Image *dst;
    size_t start_pix, end_pix;
    int use_simd;
} WorkArgs;

static void process_chunk_scalar(const Image *src, Image *dst,
                                  size_t start, size_t end) {
    const int wr = (int)(0.299f * 256 + 0.5f);
    const int wg = (int)(0.587f * 256 + 0.5f);
    const int wb = (int)(0.114f * 256 + 0.5f);
    for (size_t i = start; i < end; i++) {
        uint8_t r = src->data[i*3];
        uint8_t g = src->data[i*3+1];
        uint8_t b = src->data[i*3+2];
        uint8_t gray = (uint8_t)((wr*r + wg*g + wb*b) >> 8);
        dst->data[i*3]   = gray;
        dst->data[i*3+1] = gray;
        dst->data[i*3+2] = gray;
    }
}

static void process_chunk_simd(const Image *src, Image *dst,
                                size_t start, size_t end) {
    const uint8x8_t coeff_r = vdup_n_u8((uint8_t)(uint16_t)(0.299f*256+0.5f));
    const uint8x8_t coeff_g = vdup_n_u8((uint8_t)(uint16_t)(0.587f*256+0.5f));
    const uint8x8_t coeff_b = vdup_n_u8((uint8_t)(uint16_t)(0.114f*256+0.5f));

    size_t i = start;
    for (; i + 8 <= end; i += 8) {
        uint8x8x3_t px = vld3_u8(src->data + i*3);
        uint16x8_t acc = vmull_u8(px.val[0], coeff_r);
        acc = vmlal_u8(acc, px.val[1], coeff_g);
        acc = vmlal_u8(acc, px.val[2], coeff_b);
        uint8x8_t gray = vshrn_n_u16(acc, 8);
        uint8x8x3_t out;
        out.val[0] = gray;
        out.val[1] = gray;
        out.val[2] = gray;
        vst3_u8(dst->data + i*3, out);
    }
    for (; i < end; i++) {
        uint8_t r = src->data[i*3];
        uint8_t g = src->data[i*3+1];
        uint8_t b = src->data[i*3+2];
        uint8_t gray = (uint8_t)(0.299f*r + 0.587f*g + 0.114f*b);
        dst->data[i*3]   = gray;
        dst->data[i*3+1] = gray;
        dst->data[i*3+2] = gray;
    }
}

static void *mt_worker(void *arg) {
    WorkArgs *a = (WorkArgs *)arg;
    if (a->use_simd)
        process_chunk_simd(a->src, a->dst, a->start_pix, a->end_pix);
    else
        process_chunk_scalar(a->src, a->dst, a->start_pix, a->end_pix);
    return NULL;
}

static void mt_grayscale(const Image *src, Image *dst, int nthreads, int use_simd) {
    pthread_t threads[nthreads];
    WorkArgs args[nthreads];
    size_t npix = (size_t)src->width * src->height;
    size_t chunk = npix / nthreads;
    for (int t = 0; t < nthreads; t++) {
        args[t].src = src;
        args[t].dst = dst;
        args[t].start_pix = (size_t)t * chunk;
        args[t].end_pix = (t == nthreads-1) ? npix : args[t].start_pix + chunk;
        args[t].use_simd = use_simd;
        pthread_create(&threads[t], NULL, mt_worker, &args[t]);
    }
    for (int t = 0; t < nthreads; t++) pthread_join(threads[t], NULL);
}

static int verify(const Image *a, const Image *b) {
    size_t n = (size_t)a->width * a->height * 3;
    return memcmp(a->data, b->data, n) == 0;
}

int main(int argc, char **argv) {
    const char *input = (argc > 1) ? argv[1] : "input.ppm";
    const char *output = (argc > 2) ? argv[2] : "gray_output.ppm";

    FILE *test = fopen(input, "rb");
    if (!test) {
        printf("No input image found, generating 3840x2160 test image...\n");
        generate_test_image(input, 3840, 2160);
    } else {
        fclose(test);
    }

    Image *src = read_ppm(input);
    if (!src) return 1;

    printf("Image size:   %d x %d\n", src->width, src->height);
    printf("Threads used: %d\n\n", NUM_THREADS);

    Image *dst_scalar   = alloc_image(src->width, src->height);
    Image *dst_simd     = alloc_image(src->width, src->height);
    Image *dst_mt       = alloc_image(src->width, src->height);
    Image *dst_mt_simd  = alloc_image(src->width, src->height);

    struct timespec t0, t1;

    clock_gettime(CLOCK_MONOTONIC, &t0);
    scalar_grayscale(src, dst_scalar);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double t_scalar = elapsed_sec(t0, t1);

    clock_gettime(CLOCK_MONOTONIC, &t0);
    simd_grayscale(src, dst_simd);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double t_simd = elapsed_sec(t0, t1);

    clock_gettime(CLOCK_MONOTONIC, &t0);
    mt_grayscale(src, dst_mt, NUM_THREADS, 0);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double t_mt = elapsed_sec(t0, t1);

    clock_gettime(CLOCK_MONOTONIC, &t0);
    mt_grayscale(src, dst_mt_simd, NUM_THREADS, 1);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double t_mt_simd = elapsed_sec(t0, t1);

    int ok = verify(dst_scalar, dst_simd) &&
             verify(dst_scalar, dst_mt) &&
             verify(dst_scalar, dst_mt_simd);

    printf("%-30s %.3f sec\n", "Scalar time:", t_scalar);
    printf("%-30s %.3f sec\n", "SIMD time:", t_simd);
    printf("%-30s %.3f sec\n", "Multithreading time:", t_mt);
    printf("%-30s %.3f sec\n", "Multithreading + SIMD time:", t_mt_simd);
    printf("\nVerification: %s\n", ok ? "PASSED" : "FAILED");

    write_ppm(output, dst_scalar);
    printf("Output image: %s\n", output);

    free_image(src);
    free_image(dst_scalar);
    free_image(dst_simd);
    free_image(dst_mt);
    free_image(dst_mt_simd);
    return 0;
}