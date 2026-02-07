#include <stdio.h>

int main() {
  int a = 100;
  int *pointer;
  pointer = &a;

  printf("Address using &a: %p\n", &a);
  printf("Address using pointer: %p\n", pointer);

  *pointer = 120;
  printf("Updated value: %d\n", *pointer);

  return 0;
}