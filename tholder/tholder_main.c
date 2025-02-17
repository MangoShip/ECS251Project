#include <stdio.h>
#include <stdlib.h>
#include "tholder.h"

atomic_int bruh_int = ATOMIC_VAR_INIT(0);

int bruh(void)
{
    int hi = atomic_fetch_add(&bruh_int, 1);
    printf("\t\t\t\tYo what up %d\n", hi);
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: %s [NUM THREADS] [POOL SIZE]\n", argv[0]);
        exit(1);
    }

    size_t num_threads, max_threads;

    sscanf(argv[1], "%lu", &num_threads);
    sscanf(argv[2], "%lu", &max_threads);

    tholder_init(max_threads);

    tholder_t dummy[num_threads];

    // Some parallel work
    for (size_t i = 0; i < num_threads; i++)
    {
        tholder_create(&dummy[i], NULL, (void *)bruh, NULL);
    }

    printf("DONE SPAWNING THREADS\n");
    sleep(1);
    int tasks_completed = atomic_load(&bruh_int);
    // printf("%d tasks finished\n", tasks_completed);
    return tasks_completed;
}
