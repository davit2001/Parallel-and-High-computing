#include <stdio.h>
#include <string.h>

int string_length(char *str)
{
    int length = 0;
    char *ptr = str;

    while(*ptr++ != '\0') length++;

    return length;
}

int main()
{
    char *string = "This is a sample text string";
    printf("String: \"%s\"\n\n", string);

    char *ptr = string;

    while(*ptr != '\0') printf("%c ", *ptr++);

    printf("\n\n");
    printf("Length of \"%s\": %d\n", string, string_length(string));

    return 0;
}