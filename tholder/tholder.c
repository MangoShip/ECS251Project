#include <stdlib.h>
#include <stdio.h>
#include <stdint-gcc.h>
 
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include "errno.h"

#include "tholder.h"
#include "pthread.h"

/* library global variables */

size_t threads_spawned = 0;

pthread_mutex_t thread_pool_lock = PTHREAD_MUTEX_INITIALIZER;
thread_data **thread_pool = NULL;
size_t thread_pool_size = 0;

// finds a slot with no task to run
thread_data *get_inactive_index()
{
    size_t index = 0;

    // If the region is not initialized, init with DEFAULT_MAX_THREADS
    // This first if statement is meant to protect threads from unnecessary lock contention
    if (thread_pool == NULL)
    {
        tholder_init(DEFAULT_MAX_THREADS);
    }
    
    // continuously loop through the array
    while (true)
    {
        // if we find an uninitialized slot, use it
        if (thread_pool[index] == NULL)
        {
            /*printf("Found uninitialized slot at index %ld\n", index);*/
            thread_pool[index] = thread_data_init(index);
            break;
        }

        // if we find an open slot, use it
        if (!atomic_load(&thread_pool[index]->has_task))
        {
            //printf("Found non-busy slot at %ld\n", index);
            break;
        }
        // else, keep searching
        index++;
        if (index == thread_pool_size)
        {
            // thread pool is at capacity and there are no free slots
            // resize using realloc

            pthread_mutex_lock(&thread_pool_lock);
            
            thread_pool_size *= 2;
            thread_pool = realloc(thread_pool, thread_pool_size * sizeof(thread_data *));
            if (thread_pool == NULL)
                exit(EXIT_FAILURE);

            /*printf("RESIZED THREAS POOL TO %ld\n", thread_pool_size);*/

            pthread_mutex_unlock(&thread_pool_lock);
        } 
    }
    
    return thread_pool[index];
}

void *auxiliary_function(void *args)
{
    thread_data *td = (thread_data *)args;
    struct timespec timeout;
    int ret = -1;

    // Check the has_task flag to see if there is work to be done
    // Additionally, try grabbing the lock. If it is busy, someone is trying to give us work
    while (true)
    {
        if (ret == ETIMEDOUT)
            printf("[%ld] Waking up via timeout\n", td->index); 
        else if (ret == -1) {
            printf("[%ld] Waking up via startup\n", td->index);
        } else {
            printf("[%ld] Woken up by main thread\n", td->index);
        }

        pthread_mutex_lock(&td->data_lock);

        if (!atomic_load(&td->has_task)) {
            break;
        }

        // Call the function
        td->output->output = td->function(td->args);
        pthread_mutex_unlock(&td->output->join);

        // Set sleep timer
        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_nsec += 1000000;

        atomic_store(&td->has_task, false);

        // We're done working so unlock the house
        pthread_mutex_unlock(&td->data_lock);

        // Sleep until (signaled by another thread OR a specified time has passed)
        pthread_mutex_lock(&td->wait_lock);
        ret = pthread_cond_timedwait(&td->work_cond_var, &td->wait_lock, &timeout);
        pthread_mutex_unlock(&td->wait_lock);

        // If a thread is woken up prematurely, it is assumed that there is work to do
        // Thus, no need to check the return code of `pthread_cond_timedwait()`
        // Just rerun loop and check has_task condition
    }

    atomic_store(&td->has_thread, false);
    pthread_mutex_unlock(&td->data_lock);
    return NULL;
}

int tholder_create(tholder_t *__restrict __newthread,
                        const pthread_attr_t *__restrict __attr,
                        void *(*__start_routine)(void *),
                        void *__restrict __arg)
{

    // Find the next open slot in global array
    thread_data *td = get_inactive_index();

    // Allocate this task's output data
    task_output *output = task_output_init();
    td->output = output;
    *__newthread = (tholder_t)output;
    printf("Thread [%ld] storing output in [%llu]\n", td->index, *__newthread);
    // Lock the join lock immediately, the auxiliary_function will unlock it.
    pthread_mutex_lock(&output->join);
    
    // Lock the house just to be safe. Getting past this line means the thread has gone to sleep but is not dead
    pthread_mutex_lock(&td->data_lock);
    /*printf("Sending task to [%ld]\n", td->index);*/
    
    // If there is no thread currently active at the given index, then spawn one
    bool expected = false;
    bool desired = true;
    if (!atomic_load(&td->has_thread) && atomic_compare_exchange_strong(&td->has_thread, &expected, true))
    {
        // Create a thread and detatch it. This means it will auto-cleanup on exit
        pthread_t new_thread;
        pthread_create(&new_thread, __attr, auxiliary_function, (void *)td);
        pthread_detach(new_thread);
        threads_spawned++;
    }

    // Write the new task function and arguments to the struct 
    td->function = __start_routine;
    td->args = __arg;

    atomic_store(&td->has_task, true);
    
    // Wake up the thread living in this house
    pthread_cond_broadcast(&td->work_cond_var);

    pthread_mutex_unlock(&td->data_lock);

    return 0;
}

thread_data *thread_data_init(size_t index)
{
    thread_data *td = (thread_data *)calloc(1, sizeof(thread_data));

    td->index = index;
    td->args = NULL;
    td->function = NULL;
    atomic_init(&td->has_thread, false);
    atomic_init(&td->has_task, false);
    pthread_cond_init(&td->work_cond_var, NULL);
    pthread_mutex_init(&td->wait_lock, NULL);
    pthread_mutex_init(&td->data_lock, NULL);

    return td;
}

inline void tholder_init(size_t num_threads)
{
    pthread_mutex_lock(&thread_pool_lock);
    /*printf("[tholder.c] Initializing tholder with %ld threads\n", num_threads);*/
    // After acquiring the lock, check if region is still uninit before moving forward
    if (thread_pool == NULL)
    {
        // Initialize global region with default max threads
        thread_pool_size = num_threads;
        thread_pool = (thread_data **)calloc(thread_pool_size, sizeof(thread_data *));
    }
    pthread_mutex_unlock(&thread_pool_lock);
}

inline void tholder_destroy()
{
    pthread_mutex_lock(&thread_pool_lock);
    /*printf("[tholder.c] Destroying thread pool\n");*/

    if (thread_pool != NULL)
    {
        for (size_t i = 0; i < thread_pool_size; i++)
        {
            // Skip if the slot is NULL, it just means it was allocated and never used
            if (thread_pool[i] == NULL)
                continue;

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

    pthread_mutex_unlock(&thread_pool_lock);
    pthread_mutex_destroy(&thread_pool_lock);
}

task_output *task_output_init()
{
    task_output *output = (task_output *)calloc(1, sizeof(task_output));
    output->output = NULL;
    pthread_mutex_init(&output->join, NULL);
    return output;
}

int tholder_join(tholder_t th, void **thread_return)
{
    task_output *output = (task_output *)th; 
    pthread_mutex_lock(&output->join);
    pthread_mutex_unlock(&output->join);

    // If thread_return is not NULL, we must copy the return value over
    if (thread_return != NULL)
        memcpy(thread_return, &output->output, sizeof(void *));

    pthread_mutex_destroy(&output->join);
    free(output);

    return 0;
}
