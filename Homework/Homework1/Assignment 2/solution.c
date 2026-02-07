// Declare an array of integers and initialize it with 5 values.
// Use a pointer to traverse the array and print each element.
// Modify the values of the array using pointer arithmetic.
// Print the modified array using both the pointer and the array name.

#include <stdio.h>

int main() {
  int arr[5] = {10, 20, 30, 40, 50};
  int *pointer;

  pointer = &arr[0];

  for (int i = 0; i < 5; i++) {
    printf("Element value %d\n", *(pointer + i));
  }

  for (int i = 0; i < 5; i++) {
    *(pointer + i) = *(pointer + i) + 10;
  }

  // Print modified values using pointer
  printf("\nUsing pointer:\n");
  for (int i = 0; i < 5; i++) {
    printf("Element %d: %d\n", i, *(pointer + i));
  }

   // Print modified values using array name
  printf("\nUsing array name:\n");
  for (int i = 0; i < 5; i++) {
    printf("Element %d: %d\n", i, arr[i]);
  }

  return 0;
}