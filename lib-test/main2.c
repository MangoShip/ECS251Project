#include "stdatomic.h"

#include "../tholder/tholder.h"
#include "unistd.h"
#include <stdio.h>

#define NUM_THREADS 10

atomic_int tasks = ATOMIC_VAR_INIT(0);
extern size_t threads_spawned;

int exec_task()
{
    atomic_fetch_add(&tasks, 1);
    return atomic_load(&tasks); 
}

int main()
{
    tholder_t threads[NUM_THREADS];
    for (size_t i = 0; i < NUM_THREADS; ++i)
        tholder_create(&threads[i], NULL, (void *(*)(void *))exec_task, NULL);

    for (size_t i = 0; i < NUM_THREADS; ++i)
    {
        int output = 0;
        tholder_join(threads[i], (void *)&output);
        printf("Output: %d\n", output);
    }

    printf("Total tasks completed: %d\n", atomic_load(&tasks));
    printf("Total threads spawned: %ld\n", threads_spawned);
}
