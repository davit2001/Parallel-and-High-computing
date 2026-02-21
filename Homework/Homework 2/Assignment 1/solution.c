#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void *thread_function(void *arg) {
  char* message = (char*) arg;
  printf("%s (Thread ID: %lu)\n", message, (unsigned long)pthread_self());
  return NULL;
}

int main()
{
   pthread_t thread_1;
   pthread_t thread_2;
   pthread_t thread_3;

   char* message1 = "Thread 1 is running";
   char* message2 = "Thread 2 is running";
   char* message3 = "Thread 3 is running";

   if (pthread_create(&thread_1, NULL, thread_function, (void*)message1) != 0) {
      perror("Failed to create thread 1");
      return 1;
   }

   if (pthread_create(&thread_2, NULL, thread_function, (void*)message2) != 0) {
       perror("Failed to create thread 2");
       return 1;
   }

   if (pthread_create(&thread_3, NULL, thread_function, (void*)message3) != 0) {
      perror("Failed to create thread 3");
      return 1;
  }

   if (pthread_join(thread_1, NULL) != 0) {
      perror("pthread_join");
      return 1;
   }

   if (pthread_join(thread_2, NULL) != 0) {
     perror("pthread_join");
     return 1;
   }

   if (pthread_join(thread_3, NULL) != 0) {
    perror("pthread_join");
    return 1;
  }

    return 0;
}