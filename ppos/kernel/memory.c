// Alocador básico de memória heap.

// somente para a implementação trivial
#include <stdlib.h>

#include "lib/pplibc.h"

// implementação trivial, a ser substituída
void *mem_alloc(int size)
{
    return (malloc(size));
}

// implementação trivial, a ser substituída
int mem_free(void *addr)
{
    free(addr);
    return (NOERROR);
}

void mem_init()
{
}

void mem_term()
{
}

