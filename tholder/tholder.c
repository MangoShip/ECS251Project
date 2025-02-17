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
    printf("[%ld]: Starting\n", index);
    struct timespec timeout;

    // Loop indefinitely. This loop will only break if we "wait" for too long
    while (true)
    {
        pthread_mutex_lock(&thread_pool[index]->work_lock);
        pthread_mutex_lock(&thread_pool[index]->data_lock);

        // When a task has been received, toggle the switch and execute it, untoggle when done
        ((void *(*)(void *))thread_pool[index]->function)(thread_pool[index]->args);
        printf("[%ld]: Task complete!\n", index);

        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_nsec += 10000;

        // Sleep until (signaled by another thread OR a specified time has passed)
        atomic_store(&thread_pool[index]->has_task, false);
        pthread_mutex_unlock(&thread_pool[index]->data_lock);

        errno = pthread_cond_timedwait(&thread_pool[index]->work_cond_var, &thread_pool[index]->work_lock, &timeout);
        printf("[%ld]: Waking up\n", index);

        // If we timed out, then break and return
        if (errno != 0 && !atomic_load(&thread_pool[index]->has_task))
        {
            atomic_store(&thread_pool[index]->has_thread, false);
            pthread_mutex_unlock(&thread_pool[index]->work_lock);
            break;
        }
        pthread_mutex_unlock(&thread_pool[index]->work_lock);

        // Otherwise, it means there's work to do, so repeat the loop!
    }

    printf("[%ld]: Stopping\n", index);
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
    printf("Sending task to [%ld]\n", index);

    // If there is no thread currently active at the given index, then spawn one
    bool expected = false;
    bool desired = true;
    if (!atomic_load(&thread_pool[index]->has_thread) && atomic_compare_exchange_strong(&thread_pool[index]->has_thread, &expected, &desired))
    {
        // Create a thread and detatch it
        pthread_t new_thread;
        printf("Calling pthread_create()\n");
        pthread_create(&new_thread, NULL, auxiliary_function, (void *)index);
        pthread_detach(new_thread);
    }

    // Else, no need to spawn a new thread, an existing thread will see the task and execute it

    pthread_mutex_lock(&thread_pool[index]->data_lock);

    // Queue the task at the index and wake up the thread
    thread_pool[index]->function = (void *)__start_routine;
    thread_pool[index]->args = __arg;
    pthread_cond_broadcast(&thread_pool[index]->work_cond_var);

    pthread_mutex_unlock(&thread_pool[index]->data_lock);

    return 0;
}

inline void tholder_init(size_t num_threads)
{
    printf("Initializing tholder with %ld threads\n", num_threads);
    pthread_mutex_lock(&thread_pool_mutex);
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
            pthread_mutex_init(&thread_pool[i]->work_lock, NULL);
            pthread_mutex_init(&thread_pool[i]->data_lock, NULL);
        }
    }
    pthread_mutex_unlock(&thread_pool_mutex);
}
