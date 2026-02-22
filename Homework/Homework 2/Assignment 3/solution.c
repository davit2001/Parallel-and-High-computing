#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>


#define ARRAY_SIZE 50000000
#define THREADS_COUNT 4

int array[ARRAY_SIZE];

typedef struct {
  int start;
  int end;
} ThreadArgs;

void *thread_function(void *arg) {
  ThreadArgs *args = (ThreadArgs* ) arg;
  int *max = malloc(sizeof(int));
  *max = array[args->start];

  for (int i = args->start + 1; i < args->end; i++) {
    if (array[i] > *max) {
      *max = array[i];
    }
  }

  return (void *) max;
}

double get_time_in_seconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

int main() {
  srand(50);

  for (int i = 0; i < ARRAY_SIZE; i++) {
    array[i] = rand() % 100;
  }

  double start = get_time_in_seconds();
  int max = array[0];

  for (int i = 0; i < ARRAY_SIZE; i++) {
    if (array[i] > max) {
      max = array[i];
    }
  }

  double seq_time = get_time_in_seconds() - start;


  pthread_t threads[THREADS_COUNT];
  ThreadArgs args[THREADS_COUNT];
  int chunk = ARRAY_SIZE / THREADS_COUNT;

  start = get_time_in_seconds();

  for (int i = 0; i < THREADS_COUNT; i++) {
    args[i].start = i * chunk;
    args[i].end = (i == THREADS_COUNT - 1) ? ARRAY_SIZE : (i + 1) * chunk;
    pthread_create(&threads[i], NULL, thread_function, &args[i]);
  }

  int parallelMax = array[0];

  for (int i = 0; i < THREADS_COUNT; i++) {
    int *partialMax;
    pthread_join(threads[i], (void **)&partialMax);

    if (*partialMax > parallelMax) {
      parallelMax = *partialMax;
    }
    free(partialMax);
  }

  double par_time = get_time_in_seconds() - start;

  printf("Sequential max : %d\n", max);
  printf("Parallel max   : %d\n", parallelMax);
  printf("Sequential Time: %.4f seconds\n", seq_time);
  printf("Parallel Time  : %.4f seconds\n", par_time);
  printf("Speedup        : %.2fx\n", seq_time / par_time);
}
