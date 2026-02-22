#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define RANGE 20000000
#define THREADS_COUNT 4

typedef struct {
    int start;
    int end;
} ThreadArgs;

int is_prime(int n) {
    if (n < 2) return 0;
    if (n == 2) return 1;
    if (n % 2 == 0) return 0;
    
    for (int i = 3; i * i <= n; i += 2) {
        if (n % i == 0) return 0;
    }
    
    return 1;
}

void *thread_function(void *arg) {
    ThreadArgs *args = (ThreadArgs *) arg;
    int *count = malloc(sizeof(int));
    *count = 0;

    for (int i = args->start; i < args->end; i++) {
        if (is_prime(i)) (*count)++;
    }

    return (void *) count;
}

double get_time_in_seconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

int main() {
    double start = get_time_in_seconds();
    int sequential_count = 0;

    for (int i = 1; i <= RANGE; i++) {
        if (is_prime(i)) sequential_count++;
    }

    double sequnetial_time = get_time_in_seconds() - start;

    pthread_t threads[THREADS_COUNT];
    ThreadArgs args[THREADS_COUNT];
    int chunk = RANGE / THREADS_COUNT;

    start = get_time_in_seconds();

    for (int i = 0; i < THREADS_COUNT; i++) {
        args[i].start = i * chunk + 1;
        args[i].end = (i == THREADS_COUNT - 1) ? RANGE + 1 : (i + 1) * chunk + 1;
        pthread_create(&threads[i], NULL, thread_function, &args[i]);
    }

    int parallel_count = 0;
    for (int i = 0; i < THREADS_COUNT; i++) {
        int *partial;
        pthread_join(threads[i], (void **)&partial);
        parallel_count += *partial;
        free(partial);
    }

    double parallel_time = get_time_in_seconds() - start;

    printf("Sequential Count : %d\n", sequential_count);
    printf("Parallel Count   : %d\n", parallel_count);
    printf("Both match       : %s\n\n", sequential_count == parallel_count ? "YES" : "NO");
    printf("Sequential Time  : %.4f seconds\n", sequnetial_time);
    printf("Parallel Time    : %.4f seconds\n", parallel_time);
    printf("Speedup          : %.2fx\n", sequnetial_time / parallel_time);

    return 0;
}