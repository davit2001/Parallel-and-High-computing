#include <stdio.h>

int main()
{
    int size = 5;
    char *names[] = {"Davit", "Tigran", "Ruzanna", "Taron", "Rob"};

    for (int i = 0; i < size; i++)
        printf("names[%d] = %s\n", i, *(names + i));

    names[1] = "Arthur";

    printf("\n");

    for (int i = 0; i < size; i++)
        printf("names[%d] = %s\n", i, *(names + i));

    return 0;
}