#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define NUM_SENSORS  4
#define NUM_READINGS 5

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

typedef struct
{
    int    id;
    double readings[NUM_READINGS];
    double avg;
} sensor_t;

sensor_t     sensors[NUM_SENSORS];
my_barrier_t barrier;

void* sensor_thread(void* arg)
{
    sensor_t*    s    = (sensor_t*)arg;
    unsigned int seed = (unsigned int)time(NULL) + (unsigned int)s->id;

    for (int i = 0; i < NUM_READINGS; i++)
    {
        s->readings[i] = (rand_r(&seed) % 510) / 10.0 - 10.0;
        printf("Sensor %d | Reading %d: %.1f°C\n", s->id, i + 1, s->readings[i]);
        usleep(100000);
    }

    printf("Sensor %d: All readings collected, waiting...\n", s->id);
    my_barrier_wait(&barrier);

    double sum = 0.0;
    for (int i = 0; i < NUM_READINGS; i++)
        sum += s->readings[i];
    s->avg = sum / NUM_READINGS;

    printf("Sensor %d average: %.2f°C\n", s->id, s->avg);

    return NULL;
}

int main()
{
    pthread_t threads[NUM_SENSORS];

    my_barrier_init(&barrier, NUM_SENSORS);

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

    double total = 0.0;
    for (int i = 0; i < NUM_SENSORS; i++)
        total += sensors[i].avg;

    printf("\nOverall average temperature: %.2f°C\n", total / NUM_SENSORS);

    my_barrier_destroy(&barrier);

    return 0;
}