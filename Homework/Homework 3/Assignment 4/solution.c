#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define NUM_THREADS 4
#define NUM_STAGES  3

pthread_barrier_t stage_barrier;

void* worker(void* arg)
{
    int id = *(int*)arg;
    unsigned int seed = time(NULL) + id;

    for (int stage = 1; stage <= NUM_STAGES; stage++)
    {
        int work_time = rand_r(&seed) % 3 + 1;
        printf(
            "Thread %d starting stage %d (will take %d sec)\n",
            id, stage, work_time
        );
        sleep(work_time);
        pthread_barrier_wait(&stage_barrier);
        printf("Thread %d completed stage %d\n", id, stage);
    }

    return NULL;
}

int main()
{
    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];
    pthread_barrier_init(&stage_barrier, NULL, NUM_THREADS);
    srand(time(NULL));

    for (int i = 0; i < NUM_THREADS; i++)
    {
        thread_ids[i] = i;
        if (pthread_create(&threads[i], NULL, worker, &thread_ids[i]) != 0)
        {
            perror("pthread_create");
            exit(1);
        }
    }

    for (int i = 0; i < NUM_THREADS; i++)
        pthread_join(threads[i], NULL);

    pthread_barrier_destroy(&stage_barrier);

    printf("All stages completed.\n");
    return 0;
}