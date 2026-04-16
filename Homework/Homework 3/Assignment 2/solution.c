#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define NUM_PLAYERS 20

pthread_barrier_t barrier;

void* player(void* arg)
{
    int id = *(int*)arg;
    unsigned int seed = time(NULL) + id;

    int sleep_time = rand_r(&seed) % 5 + 1;
    printf("Player %d: Connecting... (will take %d seconds)\n", id, sleep_time);
    sleep(sleep_time);
    printf("Player %d: Connected\n", id);

    pthread_barrier_wait(&barrier);

    if (id == 0) printf("\nGame Started!\n");

    return NULL;
}

int main()
{
    pthread_t threads[NUM_PLAYERS];
    int thread_ids[NUM_PLAYERS];

    pthread_barrier_init(&barrier, NULL, NUM_PLAYERS);

    srand(time(NULL));

    printf("Quake 3 Arena Lobby:");
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

    pthread_barrier_destroy(&barrier);

    return 0;
}