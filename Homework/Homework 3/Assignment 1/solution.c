#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define PLAYERS 4
#define ROUNDS  10

int rolls[PLAYERS];
int win_count[PLAYERS] = {0};

pthread_barrier_t barrier;
pthread_mutex_t mutex;

void* player(void* arg)
{
    int id = *(int*)arg;
    unsigned int seed = time(NULL) + id;

    for (int r = 0; r < ROUNDS; r++)
    {
        int roll = rand_r(&seed) % 6 + 1;
        rolls[id] = roll;

        pthread_barrier_wait(&barrier);

        int max_roll = 0;

        for (int i = 0; i < PLAYERS; i++)
            if (rolls[i] > max_roll)
                max_roll = rolls[i];

        if (roll == max_roll)
        {
            pthread_mutex_lock(&mutex);
            win_count[id]++;
            pthread_mutex_unlock(&mutex);
        }

        if (id == 0)
        {
            printf("Round %2d: Winner(s):", r + 1);

            for (int i = 0; i < PLAYERS; i++)
                if (rolls[i] == max_roll)
                    printf(" Player %d", i);

            printf("\n");
        }

    }

    return NULL;
}

int main()
{
    pthread_t threads[PLAYERS];
    int thread_ids[PLAYERS];

    pthread_barrier_init(&barrier, NULL, PLAYERS);
    pthread_mutex_init(&mutex, NULL);

    srand(time(NULL));

    for (int i = 0; i < PLAYERS; i++)
    {
        thread_ids[i] = i;
        if (pthread_create(&threads[i], NULL, player, &thread_ids[i]) != 0)
        {
            perror("pthread_create");
            exit(1);
        }
    }

    for (int i = 0; i < PLAYERS; i++)
        pthread_join(threads[i], NULL);

    int max_wins = 0;
    for (int i = 0; i < PLAYERS; i++)
        if (win_count[i] > max_wins)
            max_wins = win_count[i];

    printf("\nOverall winner(s):");
    for (int i = 0; i < PLAYERS; i++)
        if (win_count[i] == max_wins)
            printf(" Player %d (%d wins)", i, win_count[i]);

    printf("\n");

    pthread_barrier_destroy(&barrier);
    pthread_mutex_destroy(&mutex);

    return 0;
}
