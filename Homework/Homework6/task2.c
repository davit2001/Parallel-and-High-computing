#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <omp.h>

#define N 50000000

double A[N];

int main() {
    printf("=== Task 2: Global Minimum Distance (Closest Pair) ===\n");
    printf("N = %d\n\n", N);

    printf("Initializing array...\n");
    #pragma omp parallel for
    for (int i = 0; i < N; i++) {
        A[i] = (double)rand() / RAND_MAX;
    }

    double min_diff = DBL_MAX;
    double t_start, t_end;

    printf("--- Parallel reduction(min:min_diff) ---\n");
    t_start = omp_get_wtime();

    #pragma omp parallel for reduction(min:min_diff)
    for (int i = 1; i < N; i++) {
        double diff = fabs(A[i] - A[i - 1]);
        if (diff < min_diff) {
            min_diff = diff;
        }
    }

    t_end = omp_get_wtime();

    printf("Time: %.4f seconds\n", t_end - t_start);
    printf("Minimum absolute difference: %.10f\n", min_diff);

    printf("\n--- Sequential verification (first 1,000,000 elements) ---\n");
    double seq_min = DBL_MAX;
    for (int i = 1; i < 1000000; i++) {
        double diff = fabs(A[i] - A[i - 1]);
        if (diff < seq_min) seq_min = diff;
    }
    printf("Sequential min diff (first 1M): %.10f\n", seq_min);

    return 0;
}
