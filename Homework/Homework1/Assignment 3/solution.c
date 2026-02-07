// Write a function swap(int *a, int *b) that swaps two integer values using pointers.
// In the main() function, call swap() and pass the addresses of two integers.
// Print the values of the integers before and after the swap.

#include <stdio.h>


void swap(int *x, int *y) {
  int temp = *x;
  *x = *y;
  *y = temp;
}

int main() {
  int x = 10;
  int y = 20;

  int *pointer1;
  int *pointer2;

  pointer1 = &x;
  pointer2 = &y;

  printf("Before swap:\n");
  printf("x: %d\ny: %d\n\n", x, y);
  swap(&x, &y);

  printf("After swap:\n");
  printf("x: %d\ny: %d\n", x, y);

  return 0;
}