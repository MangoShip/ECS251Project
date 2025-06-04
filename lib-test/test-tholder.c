#include "stdatomic.h"

#include "../tholder/tholder.h"
#include "unistd.h"
#include "stdio.h"
#include "stdlib.h"

atomic_int tasks = ATOMIC_VAR_INIT(0);
extern size_t threads_spawned;

// This function just adds one to a global variable
// to keep track of the number of tasks that were completed
void *exec_task(void *)
{
    atomic_fetch_add(&tasks, 1);
    return NULL; 
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: %s [NUM_THREADS] [NUM_LOOPS]\n", argv[0]);
        exit(0);
    }

    size_t num_threads;
    sscanf(argv[1], "%ld", &num_threads);

    size_t num_loops;
    sscanf(argv[2], "%ld", &num_loops);

    tholder_t threads[num_threads];

    for (size_t i = 0; i < num_loops; i++)
    {
        atomic_store(&tasks, 0);

        for (size_t i = 0; i < num_threads; ++i)
            tholder_create(&threads[i], NULL, (void *(*)(void *))exec_task, NULL);

        for (size_t i = 0; i < num_threads; ++i)
            tholder_join(threads[i], NULL);
        
        /*printf("Tasks completed: %d\n", atomic_load(&tasks));*/
    }

    tholder_destroy();
    return threads_spawned;
}
