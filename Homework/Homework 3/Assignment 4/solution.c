#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define NUM_THREADS 4
#define NUM_STAGES  3


typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
    int             count;
    int             total;
    int             phase;
} my_barrier_t;

void my_barrier_init(my_barrier_t *b, int total)
{
    pthread_mutex_init(&b->mutex, NULL);
    pthread_cond_init(&b->cond, NULL);
    b->count = 0;
    b->total = total;
    b->phase = 0;
}

void my_barrier_wait(my_barrier_t *b)
{
    pthread_mutex_lock(&b->mutex);
    int my_phase = b->phase;
    b->count++;
    if (b->count == b->total) {
        b->count = 0;
        b->phase ^= 1;
        pthread_cond_broadcast(&b->cond);
    } else {
        while (b->phase == my_phase)
            pthread_cond_wait(&b->cond, &b->mutex);
    }
    pthread_mutex_unlock(&b->mutex);
}

void my_barrier_destroy(my_barrier_t *b)
{
    pthread_mutex_destroy(&b->mutex);
    pthread_cond_destroy(&b->cond);
}

my_barrier_t stage_barrier;

void* worker(void* arg)
{
    int id = *(int*)arg;
    unsigned int seed = (unsigned int)time(NULL) + (unsigned int)id;

    for (int stage = 1; stage <= NUM_STAGES; stage++)
    {
        int work_time = rand_r(&seed) % 3 + 1;
        printf("Thread %d: stage %d started  (takes %d sec)\n", id, stage, work_time);
        sleep(work_time);
        printf("Thread %d: stage %d done, waiting...\n", id, stage);

        my_barrier_wait(&stage_barrier);

        if (id == 0)
            printf("\n--- All threads completed stage %d ---\n\n", stage);

        my_barrier_wait(&stage_barrier);
    }

    return NULL;
}

int main()
{
    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];

    my_barrier_init(&stage_barrier, NUM_THREADS);

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

    my_barrier_destroy(&stage_barrier);

    printf("All stages completed.\n");
    return 0;
}