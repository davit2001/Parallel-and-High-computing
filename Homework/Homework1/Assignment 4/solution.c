// Declare an integer variable and a pointer to that variable.
// Declare a pointer to the pointer and initialize it.
// Print the value of the integer using both the pointer and the double-pointer.


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