#include <stdio.h>
#include <pthread.h>
#include <mach/mach.h>

#define THREADS_COUNT 4
#define ITERATIONS 1000000000

void *thread_function(void *arg) {
    int thread_num = *((int *) arg);

    volatile long sum = 0;
    for (long i = 0; i < ITERATIONS; i++) {
        sum += i;
    }

    printf("Thread %d | Mach thread port: %u | Thread ID: %lu\n",
           thread_num, pthread_mach_thread_np(pthread_self()), (unsigned long) pthread_self());

    return NULL;
}

int main() {
    pthread_t threads[THREADS_COUNT];
    int thread_nums[THREADS_COUNT];

    for (int i = 0; i < THREADS_COUNT; i++) {
        thread_nums[i] = i + 1;
        pthread_create(&threads[i], NULL, thread_function, &thread_nums[i]);
    }

    for (int i = 0; i < THREADS_COUNT; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}