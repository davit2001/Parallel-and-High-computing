#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define NUM_SENSORS  4
#define NUM_READINGS 5

typedef struct
{
    int id;
    double readings[NUM_READINGS];
    double avg;
} sensor_t;

sensor_t sensors[NUM_SENSORS];
pthread_barrier_t barrier;

void* sensor_thread(void* arg)
{
    sensor_t* s = (sensor_t*)arg;
    unsigned int seed = time(NULL) + s->id;

    for (int i = 0; i < NUM_READINGS; i++)
    {
        s->readings[i] = (rand_r(&seed) % 510) / 10.0 - 10.0;
        printf("S%dD%d: %.1f°C\n", s->id, i, s->readings[i]);
        usleep(1000000);
    }

    pthread_barrier_wait(&barrier);

    double sum = 0.0;
    for (int i = 0; i < NUM_READINGS; i++)
        sum += s->readings[i];

    s->avg = sum / NUM_READINGS;
    printf("Sensor %d average: %.2f C\n", s->id, s->avg);

    return NULL;
}

int main()
{
    pthread_t threads[NUM_SENSORS];
    pthread_barrier_init(&barrier, NULL, NUM_SENSORS);
    srand(time(NULL));

    for (int i = 0; i < NUM_SENSORS; i++)
    {
        sensors[i].id = i;
        if (pthread_create(&threads[i], NULL, sensor_thread, &sensors[i]) != 0)
        {
            perror("pthread_create");
            exit(1);
        }
    }

    for (int i = 0; i < NUM_SENSORS; i++)
        pthread_join(threads[i], NULL);

    double total_sum = 0.0;
    for (int i = 0; i < NUM_SENSORS; i++)
        total_sum += sensors[i].avg;

    double overall_avg = total_sum / NUM_SENSORS;

    printf("\nOverall average temperature: %.2f°C\n", overall_avg);

    pthread_barrier_destroy(&barrier);

    return 0;
}