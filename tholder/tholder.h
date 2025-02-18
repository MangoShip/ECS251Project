#include <stdbool.h>

#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdatomic.h>

#define DEFAULT_MAX_THREADS 8

// Used as a pointer to the task
typedef unsigned long long tholder_t;

// Holds the status and return values of a task
typedef struct task_output
{
    bool complete;
    void *output;
    pthread_cond_t join;
} task_output;

typedef struct thread_args
{
    pthread_cond_t work_cond_var;
    pthread_mutex_t wait_lock;

    atomic_bool has_thread;
    atomic_bool has_task;

    pthread_mutex_t data_lock;
    void *(*function)(void *);
    void *args;
} thread_args;

u_int32_t tholder_create(tholder_t *__restrict __newthread,
                         const pthread_attr_t *__restrict __attr,
                         void *(*__start_routine)(void *),
                         void *__restrict __arg);

void tholder_init(size_t num_threads);

void tholder_destroy();

void *auxiliary_function(void *args);

size_t get_inactive_index();
