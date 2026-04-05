#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#define N 10000
#define NUM_THREADS 4

typedef enum { HIGH, NORMAL } Priority;
typedef struct {
    int order_id;
    float distance_km;
    Priority priority;
} Order;

int main() {
    Order orders[N];
    int thread_high_count[NUM_THREADS] = {0};
    int threshold = 0;

    #pragma omp parallel num_threads(NUM_THREADS)
    {
        #pragma omp single
        {
            threshold = 20;
            printf("[Thread %d] Distance threshold set to %d km\n", omp_get_thread_num(), threshold);
            srand(42);
            for (int i = 0; i < N; i++) {
                orders[i].order_id = i + 1;
                orders[i].distance_km = (float)(rand() % 100);
                orders[i].priority = NORMAL;
            }
            printf("[Thread %d] Orders initialized.\n", omp_get_thread_num());
        }

        #pragma omp for schedule(static)
        for (int i = 0; i < N; i++) {
            if (orders[i].distance_km < threshold)
                orders[i].priority = HIGH;
            else
                orders[i].priority = NORMAL;
        }

        #pragma omp barrier
        #pragma omp single
        {
            printf("[Thread %d] Priority assignment finished for all orders.\n", omp_get_thread_num());
        }
        int tid = omp_get_thread_num();

        #pragma omp for schedule(static)
        for (int i = 0; i < N; i++) {
            if (orders[i].priority == HIGH)
                thread_high_count[tid]++;
        }

       #pragma omp barrier
        #pragma omp single
        {
            int total = 0;
            printf("\n===== HIGH Priority Order Counts =====\n");
            for (int t = 0; t < NUM_THREADS; t++) {
                printf("  Thread %d : %d HIGH orders\n", t, thread_high_count[t]);
                total += thread_high_count[t];
            }
            printf("--------------------------------------\n");
            printf("  TOTAL    : %d HIGH priority orders\n", total);
            printf("======================================\n");
        }
    }
    return 0;
}
