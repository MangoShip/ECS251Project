#include <stdlib.h>
#include <stdio.h>
#include <stdint-gcc.h>
#include <stdbool.h>

#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#define DEFAULT_MAX_THREADS 8;

typedef struct
{
    bool work2do;
    bool thread_active;
    void *function;
    void *args;
    pthread_mutex_t lock;
} thread_args;

pthread_mutex_t global_region_mutex = PTHREAD_MUTEX_INITIALIZER;
thread_args *global_region = NULL;
size_t global_region_size = 0;

size_t get_inactive_index()
{
    size_t index = 0;
    while (true)
    {
        index = index % global_region_size;
        // For each index, attempt to acquire the lock
        int got_lock = pthread_mutex_trylock(&global_region[index].lock);
        // If locking failed, continue to the next index
        if (got_lock < 0)
        {
            index += 1;
            continue;
        }
        // Otherwise return the index
        // Caller is responsible for unlocking, as this index is now busy with a thread
        return index;
    }
}

void *auxiliary_function(void *args)
{

    // Global array index given to this thread by tholder_create()
    size_t index = (size_t)args;

    // Loop indefinitely. This loop will only break if we "wait" for too long
    while (true)
    {
        // TODO: change this from a spin lock to a condition variable.
        // CondVar will allow the thread to block until
        // (signaled by another thread OR a specified time has passed)
        while (!global_region[index].work2do)
            ;
        ((void (*)(void *))global_region[index].function)(global_region[index].args);
    }

    printf("Auxiliary function done!\n");
    // Release the lock, which free's up that index in the global array
    pthread_mutex_unlock(&global_region[index].lock);
    return NULL;
}

uint32_t tholder_create(pthread_t *__restrict __newthread,
                        const pthread_attr_t *__restrict __attr,
                        void *(*__start_routine)(void *),
                        void *__restrict __arg)
{
    // If the region is not initialized, init with DEFAULT_MAX_THREADS
    // This first if statement is meant to protect threads from unnecessary lock contention
    if (global_region == NULL)
    {
        pthread_mutex_lock(&global_region_mutex);
        // After acquiring the lock, check if region is still uninit before moving forward
        if (global_region == NULL)
        {
            // Initialize global region with default max threads
            global_region_size = DEFAULT_MAX_THREADS;
            global_region = (thread_args *)calloc(global_region_size, sizeof(thread_args));
            for (size_t i = 0; i < global_region_size; i++)
            {
                // Constructor functions are for losers... kidding, this is a TODO
                global_region[i].args = NULL;
                global_region[i].function = NULL;
                global_region[i].thread_active = false;
                global_region[i].work2do = false;
                pthread_mutex_init(&global_region[i].lock, NULL);
            }
        }
        pthread_mutex_unlock(&global_region_mutex);
    }

    // Find the next open slot in global array
    size_t index = get_inactive_index();

    global_region[index].function = __start_routine;
    global_region[index].args = __arg;
    // Only AFTER writing function and args values can we toggle work2do
    // since there may be a thread spin waiting on it
    global_region[index].work2do = true;

    // If there is no thread currently active at the given index, then spawn one
    if (!global_region[index].thread_active)
        pthread_create(__newthread, __attr, auxiliary_function, (void *)index);

    printf("Returning from tholder_create\n");
    return 0;
}

uint32_t tholder_join(pthread_t __th, void **__thead_return)
{
}

void *bruh()
{
    printf("BRUH HAS BEEN EXECUTED\n");
    return NULL;
}

void tholder_init(size_t num_threads)
{
    pthread_mutex_lock(&global_region_mutex);
    // After acquiring the lock, check if region is still uninit before moving forward
    if (global_region == NULL)
    {
        // Initialize global region with default max threads
        global_region_size = num_threads;
        global_region = (thread_args *)calloc(global_region_size, sizeof(thread_args));
        for (size_t i = 0; i < global_region_size; i++)
        {
            // Constructor functions are for losers... kidding, this is a TODO
            global_region[i].args = NULL;
            global_region[i].function = NULL;
            global_region[i].thread_active = false;
            global_region[i].work2do = false;
            pthread_mutex_init(&global_region[i].lock, NULL);
        }
    }
    pthread_mutex_unlock(&global_region_mutex);
}

int main()
{
    pthread_t dummy;
    tholder_create(&dummy, NULL, bruh, NULL);

    sleep(3);
    return 0;
}