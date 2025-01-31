#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

/*
 * thread.c: Original thread.c
*/
 
void *do_nothing()
{
    // Does nothing!
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: ./thread [NUM THREADS]\n");
        exit(1);
    }

    uint64_t num_threads;

    sscanf(argv[1], "%lu", &num_threads);
    printf("Starting %lu threads\n", num_threads);

    // Initialize empty thread array
    pthread_t thread_array[num_threads];

    clock_t start_time = clock();
    for (uint64_t i = 0; i < num_threads; i++)
    {
        pthread_create(&thread_array[i], NULL, do_nothing, NULL);
    }
    clock_t create_time = clock();

    printf("Destroying threads\n");

    for (uint64_t i = 0; i < num_threads; i++)
    {
        pthread_join(thread_array[i], NULL);
    }
    clock_t destroy_time = clock();
    printf("Done!\n");

    double create_elapsed = ((double)(create_time - start_time)) / CLOCKS_PER_SEC * 1e6;
    double destroy_elapsed = ((double)(destroy_time - create_time)) / CLOCKS_PER_SEC * 1e6;

    printf("Creation took %f μs (~ %f μs/thread)\n", create_elapsed, create_elapsed / (double)num_threads);
    printf("Destruction took %f μs (~ %f μs/thread)\n", destroy_elapsed, destroy_elapsed / (double)num_threads);
}
