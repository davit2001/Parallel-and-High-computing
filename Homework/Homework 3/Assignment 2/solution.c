#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define NUM_PLAYERS 20

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

my_barrier_t barrier;

void* player(void* arg)
{
    int id = *(int*)arg;
    unsigned int seed = (unsigned int)time(NULL) + (unsigned int)id;

    int sleep_time = rand_r(&seed) % 5 + 1;
    printf("Player %d: Connecting... (will take %d seconds)\n", id, sleep_time);
    sleep(sleep_time);
    printf("Player %d: Connected\n", id);

    my_barrier_wait(&barrier);

    printf("Player %d: Game Started!\n", id);

    return NULL;
}

int main()
{
    pthread_t threads[NUM_PLAYERS];
    int thread_ids[NUM_PLAYERS];

    my_barrier_init(&barrier, NUM_PLAYERS);

    printf("Quake 3 Arena Lobby:\n");
    for (int i = 0; i < NUM_PLAYERS; i++)
    {
        thread_ids[i] = i;
        if (pthread_create(&threads[i], NULL, player, &thread_ids[i]) != 0)
        {
            perror("pthread_create");
            exit(1);
        }
    }

    for (int i = 0; i < NUM_PLAYERS; i++)
        pthread_join(threads[i], NULL);

    my_barrier_destroy(&barrier);

    return 0;
}