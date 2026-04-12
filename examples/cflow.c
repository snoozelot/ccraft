// cflow.c — example for cflow
//
// A small program with a clear call hierarchy and one recursive function.
//
// Forward tree from main:
//   cflow examples/cflow.c
//
// Forward tree from process:
//   cflow -r process examples/cflow.c
//
// Reverse tree (who calls validate):
//   cflow -R validate examples/cflow.c
//
// Limited depth:
//   cflow -d 2 examples/cflow.c

#include <stdio.h>
#include <stdlib.h>

int validate(int x);
int transform(int x);
int process(int x);
int factorial(int n);

int
validate(int x)
{
    return x >= 0 && x < 100;
}

int
transform(int x)
{
    return x * 2 + 1;
}

int
process(int x)
{
    if (!validate(x)) {
        return -1;
    }
    return transform(x);
}

int
factorial(int n)
{
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

int
main(void)
{
    int result;

    result = process(42);
    printf("process(42) = %d\n", result);

    result = factorial(5);
    printf("factorial(5) = %d\n", result);

    return 0;
}
