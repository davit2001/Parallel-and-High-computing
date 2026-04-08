#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <string.h>
#include <time.h>
 
#define N 100000000
#define BINS 256
 
int A[N];
 
void print_hist_sample(long long *hist, const char *label) {
    printf("\n[%s] Sample histogram (first 10 bins):\n", label);
    for (int i = 0; i < 10; i++) {
        printf("  hist[%d] = %lld\n", i, hist[i]);
    }
    long long total = 0;
    for (int i = 0; i < BINS; i++) total += hist[i];
    printf("  Total count: %lld (expected: %d)\n", total, N);
}
 
int main() {
    printf("=== Task 1: Parallel Histogram ===\n");
    printf("N = %d, BINS = %d\n\n", N, BINS);
 
    printf("Initializing array...\n");
    srand(42);
    #pragma omp parallel for
    for (int i = 0; i < N; i++) {
        A[i] = rand() % BINS;
    }
 
    double t_start, t_end;
 
    long long hist_naive[BINS];
    memset(hist_naive, 0, sizeof(hist_naive));
 
    printf("--- Version 1: Naive (race condition) ---\n");
    t_start = omp_get_wtime();
    #pragma omp parallel for
    for (int i = 0; i < N; i++) {
        hist_naive[A[i]]++;
    }
    t_end = omp_get_wtime();
    printf("Time: %.4f seconds\n", t_end - t_start);
    print_hist_sample(hist_naive, "Naive");
 
    long long hist_critical[BINS];
    memset(hist_critical, 0, sizeof(hist_critical));
 
    printf("\n--- Version 2: With #pragma omp critical ---\n");
    t_start = omp_get_wtime();
    #pragma omp parallel for
    for (int i = 0; i < N; i++) {
        #pragma omp critical
        {
            hist_critical[A[i]]++;
        }
    }
    t_end = omp_get_wtime();
    printf("Time: %.4f seconds\n", t_end - t_start);
    print_hist_sample(hist_critical, "Critical");
 
    long long hist_reduction[BINS];
    memset(hist_reduction, 0, sizeof(hist_reduction));
 
    printf("\n--- Version 3: Array Reduction (OpenMP 4.5+) ---\n");
    t_start = omp_get_wtime();
    #pragma omp parallel for reduction(+:hist_reduction[:BINS])
    for (int i = 0; i < N; i++) {
        hist_reduction[A[i]]++;
    }
    t_end = omp_get_wtime();
    printf("Time: %.4f seconds\n", t_end - t_start);
    print_hist_sample(hist_reduction, "Reduction");
 
    int mismatch = 0;
    for (int i = 0; i < BINS; i++) {
        if (hist_critical[i] != hist_reduction[i]) {
            mismatch++;
        }
    }
    printf("\nCorrectness check (critical vs reduction): %s\n",
           mismatch == 0 ? "MATCH" : "MISMATCH");
 
    return 0;
}
