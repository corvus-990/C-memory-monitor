/*
 * Test program for C-memory-monitor.
 * Makes a few allocations and deliberately leaks one 20-byte block.
 *
 * Build:  gcc -o leaky examples/leaky.c
 * Run:    LD_PRELOAD=./mem_leak.so ./leaky
*/

#include <stdlib.h>

int main(void)
{
    char *a = malloc(16);
    char *b = malloc(8);
    char *c = malloc(32);

    free(a);
    b = realloc(b, 64);
    free(c);

    char *d = malloc(64);
    free(b);
    free(d);

    char *leak = calloc(5, 4);   /* 20 bytes, never freed */
    (void)leak;

    return 0;
}
