#include <stdio.h>

int main() {
  int value = 100;
  int *pointer1;
  int **pointer2;

  pointer1 = &value;
  pointer2 = &pointer1;

  printf("pointer1: %d\npointer2: %d\n\n", *pointer1, **pointer2);

  return 0;
}