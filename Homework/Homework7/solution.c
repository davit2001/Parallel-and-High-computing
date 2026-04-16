#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

long long fib(int n)
{
    if (n < 2)
        return n;

    if (n <= 10)
        return fib(n - 1) + fib(n - 2);

    long long x, y;

    #pragma omp task shared(x) if(n > 10)
    x = fib(n - 1);
    #pragma omp task shared(y) if(n > 10)
    y = fib(n - 2);

    #pragma omp taskwait

    return x + y;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        perror("Usage: ./fib <n>\n");
        return 1;
    }

    int n = atoi(argv[1]);

    if (n < 0)
    {
        perror("Please enter a non-negative integer.\n");
        return 1;
    }

    long long result;

    #pragma omp parallel
    #pragma omp single
    result = fib(n);

    printf("F(%d) = %lld\n", n, result);
    return 0;
}
