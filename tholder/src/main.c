#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>

#include "tholder.h"

extern size_t threads_spawned;

atomic_int num_tasks = ATOMIC_VAR_INIT(0);

void *bruh(void * _arg)
{
    size_t output = 1;
    atomic_fetch_add(&num_tasks, 1);
    return (void *)output;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s [NUM THREADS]\n", argv[0]);
        exit(1);
    }

    size_t num_threads;

    sscanf(argv[1], "%lu", &num_threads);

    tholder_t dummy[num_threads];

    // Some parallel work
    for (size_t i = 0; i < num_threads; i++)
    {
        tholder_create(&dummy[i], NULL, bruh, NULL);
    }
    
    for (size_t i = 0; i < num_threads; i++)
    {
        size_t output;
        tholder_join(dummy[i], (void *)&output);
    }
    printf("%d tasks completed\n", atomic_load(&num_tasks));
    printf("%ld threads spawned\n", threads_spawned);

    tholder_destroy();
}
