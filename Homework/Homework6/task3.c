#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#define N 100000000

double A[N];

int main() {
    printf("=== Task 3: Parallel Filtered Sum (Top-K Style) ===\n");
    printf("N = %d\n\n", N);

    printf("Initializing array...\n");
    #pragma omp parallel for
    for (int i = 0; i < N; i++) {
        A[i] = (double)rand() / RAND_MAX;
    }

    double t_start, t_end;

    double max_val = A[0];

    printf("--- Step 1: Find max(A) ---\n");
    t_start = omp_get_wtime();

    #pragma omp parallel for reduction(max:max_val)
    for (int i = 1; i < N; i++) {
        if (A[i] > max_val) {
            max_val = A[i];
        }
    }

    t_end = omp_get_wtime();
    printf("Time: %.4f seconds\n", t_end - t_start);
    printf("max(A) = %.10f\n", max_val);

    double T = 0.8 * max_val;
    printf("\n--- Step 2: Threshold T = 0.8 * max ---\n");
    printf("T = %.10f\n", T);

    double filtered_sum = 0.0;
    long long count = 0;

    printf("\n--- Step 3: Filtered sum where A[i] > T ---\n");
    t_start = omp_get_wtime();

    #pragma omp parallel for reduction(+:filtered_sum, count)
    for (int i = 0; i < N; i++) {
        if (A[i] > T) {
            filtered_sum += A[i];
            count++;
        }
    }

    t_end = omp_get_wtime();
    printf("Time: %.4f seconds\n", t_end - t_start);
    printf("Elements above T: %lld\n", count);
    printf("Filtered sum: %.6f\n", filtered_sum);
    printf("Average of filtered elements: %.10f\n",
           count > 0 ? filtered_sum / count : 0.0);

    return 0;
}
