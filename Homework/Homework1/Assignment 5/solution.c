// Use malloc() to allocate memory for an integer.
// Assign a value to the allocated memory and print it.
// Use malloc() to allocate memory for an array of 5 integers.
// Populate the array using pointer arithmetic and print the values.
// Free all allocated memory.



#include <stdio.h>
#include <stdlib.h>

int main() {

  int *ptr = (int*) malloc(sizeof(int));

  if (ptr == NULL) {
     printf("Memory allocation failed!\n");
     return 1;
  }

  *ptr = 100;
  printf("Value: %d\n", *ptr);

  int *arr = (int*) malloc(5 * sizeof(int));

  if (arr == NULL) {
    printf("Memory allocation failed!\n");
    return 1;
  }

  for (int i = 0; i < 5; i++) {
    *(arr + i) = i + 1;
    printf("Array value: %d\n", arr[i]);
  }

  free(ptr);
  free(arr);
}