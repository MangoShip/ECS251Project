#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

#include "tholder.h"

atomic_int bruh_int = ATOMIC_VAR_INIT(0);

void *bruh(void* _args)
{
    atomic_fetch_add(&bruh_int, 1);
    return NULL;
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
    tholder_init(1);

    // Some parallel work
    for (size_t i = 0; i < num_threads; i++)
    {
        tholder_create(&dummy[i], NULL, bruh, (void *)i);
    }

    usleep(2e5);
    
    int tasks_completed = atomic_load(&bruh_int);
    printf("%d tasks finished\n", tasks_completed);
    
    // Free up memory
    tholder_destroy();
    return tasks_completed;
}
