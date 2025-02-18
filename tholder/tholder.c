#include <stdlib.h>
#include <stdio.h>
#include <stdint-gcc.h>
#include <errno.h>

#include <sys/time.h>

#include "tholder.h"

/* LIBRARY GLOBAL VARIABLES */

pthread_mutex_t thread_pool_mutex = PTHREAD_MUTEX_INITIALIZER;
thread_args **thread_pool = NULL;
size_t thread_pool_size = 0;

// Finds a slot with no task to run
size_t get_inactive_index()
{
    size_t index = 0;
    bool expected = false;
    bool desired = true;

    // Attempt to atomically compare and toggle the has_task boolean from false to true
    while (!atomic_compare_exchange_weak(&thread_pool[index]->has_task, &expected, &desired))
    {
        expected = false;
        index = (index + 1) % thread_pool_size;
    }

    return index;
}

void *auxiliary_function(void *args)
{
    // Global array index given to this thread by tholder_create()
    size_t index = (size_t)args;
    struct timespec timeout;

    // Loop as long as there is a task to execute
    while (atomic_load(&thread_pool[index]->has_task))
    {
        // When a task has been received, toggle the switch and execute it, untoggle when done

        // Lock the house, nobody can give us more work while we're working!
        pthread_mutex_lock(&thread_pool[index]->data_lock);

        atomic_store(&thread_pool[index]->has_task, true);
        ((void *(*)(void *))thread_pool[index]->function)(thread_pool[index]->args);

        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_nsec += 10000;

        atomic_store(&thread_pool[index]->has_task, false);

        // We're done working so unlock the house
        pthread_mutex_unlock(&thread_pool[index]->data_lock);

        // Sleep until (signaled by another thread OR a specified time has passed)
        pthread_mutex_lock(&thread_pool[index]->wait_lock);
        int ret = pthread_cond_timedwait(&thread_pool[index]->work_cond_var, &thread_pool[index]->wait_lock, &timeout);
        pthread_mutex_unlock(&thread_pool[index]->wait_lock);
    }

    atomic_store(&thread_pool[index]->has_thread, false);
    atomic_store(&thread_pool[index]->has_task, false);
    return NULL;
}

uint32_t tholder_create(tholder_t *__restrict __newthread,
                        const pthread_attr_t *__restrict __attr,
                        void *(*__start_routine)(void *),
                        void *__restrict __arg)
{
    // If the region is not initialized, init with DEFAULT_MAX_THREADS
    // This first if statement is meant to protect threads from unnecessary lock contention
    if (thread_pool == NULL)
    {
        tholder_init(DEFAULT_MAX_THREADS);
    }

    // Find the next open slot in global array
    size_t index = get_inactive_index();

    // Lock the house just to be safe. Getting past this line means the thread has gone to sleep but is not dead
    pthread_mutex_lock(&thread_pool[index]->data_lock);

    // If there is no thread currently active at the given index, then spawn one
    bool expected = false;
    bool desired = true;
    if (!atomic_load(&thread_pool[index]->has_thread) && atomic_compare_exchange_strong(&thread_pool[index]->has_thread, &expected, &desired))
    {
        // Create a thread and detatch it
        pthread_t new_thread;
        pthread_create(&new_thread, NULL, auxiliary_function, (void *)index);
        pthread_detach(new_thread);
    }

    // Queue the task at the index and wake up the thread
    thread_pool[index]->function = __start_routine;
    thread_pool[index]->args = __arg;

    // Wake up the thread living in this house
    pthread_cond_broadcast(&thread_pool[index]->work_cond_var);

    pthread_mutex_unlock(&thread_pool[index]->data_lock);

    return 0;
}

inline void tholder_init(size_t num_threads)
{
    pthread_mutex_lock(&thread_pool_mutex);
    printf("Initializing tholder with %ld threads\n", num_threads);
    // After acquiring the lock, check if region is still uninit before moving forward
    if (thread_pool == NULL)
    {
        // Initialize global region with default max threads
        thread_pool_size = num_threads;
        thread_pool = (thread_args **)calloc(thread_pool_size, sizeof(thread_args *));
        for (size_t i = 0; i < thread_pool_size; i++)
        {
            // Allocate each thread_args slot
            thread_pool[i] = (thread_args *)malloc(sizeof(thread_args));
            thread_pool[i]->args = NULL;
            thread_pool[i]->function = NULL;
            atomic_init(&thread_pool[i]->has_thread, false);
            atomic_init(&thread_pool[i]->has_task, false);
            pthread_cond_init(&thread_pool[i]->work_cond_var, NULL);
            pthread_mutex_init(&thread_pool[i]->wait_lock, NULL);
            pthread_mutex_init(&thread_pool[i]->data_lock, NULL);
        }
    }
    pthread_mutex_unlock(&thread_pool_mutex);
}

inline void tholder_destroy()
{
    pthread_mutex_lock(&thread_pool_mutex);
    printf("Destroying thread pool\n");
    
    if (thread_pool != NULL) {
        for (size_t i = 0; i < thread_pool_size; i++)
        {
            pthread_cond_destroy(&thread_pool[i]->work_cond_var);
            pthread_mutex_destroy(&thread_pool[i]->wait_lock);
            pthread_mutex_destroy(&thread_pool[i]->data_lock);
            thread_pool[i]->args = NULL;
            thread_pool[i]->function = NULL;

            free(thread_pool[i]);
        }
        free(thread_pool);
        thread_pool = NULL;
    }

    pthread_mutex_unlock(&thread_pool_mutex);
    pthread_mutex_destroy(&thread_pool_mutex);
}
