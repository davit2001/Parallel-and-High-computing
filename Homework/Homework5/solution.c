#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#define N 20000
#define FAST_THRESHOLD 100
#define SLOW_THRESHOLD 300

typedef enum { FAST, MEDIUM, SLOW } Category;
typedef struct {
    int request_id;
    int user_id;
    int response_time_ms;
    Category category;
} LogEntry;

int main() {
    LogEntry logs[N];
    int fast_count = 0;
    int medium_count = 0;
    int slow_count = 0;

    #pragma omp parallel num_threads(4)
    {
        #pragma omp single
        {
            printf("[Thread %d] Initializing %d log entries...\n", omp_get_thread_num(), N);
            srand(42);
            for (int i = 0; i < N; i++) {
                logs[i].request_id = i + 1;
                logs[i].user_id = (rand() % 1000) + 1;
                logs[i].response_time_ms = rand() % 600;
                logs[i].category = FAST;
            }
            printf("[Thread %d] Initialization complete.\n", omp_get_thread_num());
        }
        #pragma omp barrier
        #pragma omp for schedule(static)

        for (int i = 0; i < N; i++) {
            int t = logs[i].response_time_ms;
            if (t < FAST_THRESHOLD)
                logs[i].category = FAST;
            else if (t <= SLOW_THRESHOLD)
                logs[i].category = MEDIUM;
            else
                logs[i].category = SLOW;
        }

        #pragma omp barrier
        #pragma omp single
        {
            for (int i = 0; i < N; i++) {
                if (logs[i].category == FAST) fast_count++;
                else if (logs[i].category == MEDIUM) medium_count++;
                else slow_count++;
            }

            printf("\n===== Log Classification Summary =====\n");
            printf("FAST   (< %d ms)    : %d\n", FAST_THRESHOLD, fast_count);
            printf("MEDIUM (%d-%d ms) : %d\n", FAST_THRESHOLD, SLOW_THRESHOLD, medium_count);
            printf("SLOW   (> %d ms)   : %d\n", SLOW_THRESHOLD, slow_count);
            printf("TOTAL               : %d\n", fast_count + medium_count + slow_count);
            printf("======================================\n");
        }
    }
    return 0;
}
