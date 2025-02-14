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
    void *function;
    void *args;
    pthread_mutex_t lock;
} thread_args;

pthread_mutex_t global_region_mutex = PTHREAD_MUTEX_INITIALIZER;
thread_args *global_region = NULL;
size_t global_region_size = 0;

thread_args *create_thread_args(void *(*function)(void *), void *args)
{
    thread_args *t = (thread_args *)malloc(sizeof(thread_args));
    t->function = function;
    t->args = args;
    pthread_mutex_init(&t->lock, NULL);
    return t;
}

size_t get_inactive_index()
{
    size_t index = 0;
    while (true)
    {
        int got_lock = pthread_mutex_trylock(&global_region[index].lock);
        if (got_lock < 0)
            continue;

        
    }
}

void *auxiliary_function(void *region)
{
    // If the region is not initialized, init with DEFAULT_MAX_THREADS
    // This first if statement is meant to protect threads from unnecessary lock contention
    if (global_region == NULL)
    {
        pthread_mutex_lock(&global_region_mutex);
        // After acquiring the lock, check if region is still uninit before moving forward
        if (global_region == NULL)
        {
            global_region_size = DEFAULT_MAX_THREADS;
            global_region = (thread_args *)calloc(global_region_size, sizeof(thread_args));
        }
        pthread_mutex_lock(&global_region_mutex);
    }

    thread_args *t = (thread_args *)region;

    // Loop indefinitely. This loop will only break if we "wait" for too long
    // ((void (*)(void *))t->function)(t->args);

    printf("Auxiliary function done!\n");
    return NULL;
}

uint32_t tholder_create(pthread_t *__restrict __newthread,
                        const pthread_attr_t *__restrict __attr,
                        void *(*__start_routine)(void *),
                        void *__restrict __arg)
{
    thread_args *t = create_thread_args(__start_routine, __arg);

    uint32_t ret = pthread_create(__newthread, __attr, auxiliary_function, (void *)t);

    printf("Returning from tholder_create\n");
    return ret;
}

uint32_t tholder_join(pthread_t __th, void **__thead_return)
{
}

void *bruh()
{
    printf("BRUH HAS BEEN EXECUTED\n");
    return NULL;
}

void tholder_init(int num_threads) {
    global_region = calloc(num_threads, sizeof(thread_args));
}

int main()
{
    pthread_t dummy;
    tholder_create(&dummy, NULL, bruh, NULL);

    sleep(3);
    return 0;
}