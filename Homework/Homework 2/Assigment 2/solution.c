#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define Array_SIZE 50000000
#define NUM_THREADS 4

int array[Array_SIZE];

typedef struct {
  int start;
  int end;
} ThreadArgs;

void *thread_function(void *arg) {
  ThreadArgs *args = (ThreadArgs *) arg;
  long long *sum = malloc(sizeof(long long));
  *sum = 0;

  for (int i = args->start; i < args->end; i++) {
    *sum = array[i];
  }
  return (void *) sum;
}

double get_time_in_seconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

int main() {
  srand(50);

  for (int i = 0; i < Array_SIZE; i++) {
    array[i] = rand() % 100;
  }

  // Sequential Sum
  double start = get_time_in_seconds();
  long long sequential_sum = 0;
  for (int i = 0; i < Array_SIZE; i++) {
      sequential_sum += array[i];
  }
  double seq_time = get_time_in_seconds() - start;

  pthread_t threads[NUM_THREADS];
  ThreadArgs args[NUM_THREADS];
  int chunk = Array_SIZE / NUM_THREADS;

  start = get_time_in_seconds();

  for (int i = 0; i < NUM_THREADS; i++) {
    args[i].start = i * chunk;
    args[i].end = (i == NUM_THREADS - 1) ? Array_SIZE : (i + 1) * chunk;
    pthread_create(&threads[i], NULL, thread_function, &args[i]);
  }


  long long parallel_sum = 0;
  for (int i = 0; i < NUM_THREADS; i++) {
    long long *partial;
    pthread_join(threads[i], (void **)&partial);
    parallel_sum += *partial;
    free(partial);
  }

  double par_time = get_time_in_seconds() - start;

  printf("Sequential Sum : %lld\n", sequential_sum);
  printf("Parallel Sum   : %lld\n", parallel_sum);
  printf("Both match     : %s\n\n", sequential_sum == parallel_sum ? "YES" : "NO");
  printf("Sequential Time: %.4f seconds\n", seq_time);
  printf("Parallel Time  : %.4f seconds\n", par_time);
  printf("Speedup        : %.2fx\n", seq_time / par_time);

  return 0;
}