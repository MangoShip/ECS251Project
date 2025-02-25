#include <stdbool.h>
#include <stdatomic.h>

#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#define DEFAULT_MAX_THREADS 8

// Used as a pointer to the task
typedef unsigned long long tholder_t;

// Holds the status and return values of a task
typedef struct task_output
{
    bool complete;
    void *output;
    pthread_mutex_t join;
} task_output;

typedef struct thread_data
{
    // Index of thread_data struct in thread_pool, used for debugging
    size_t index;

    // Condition variable used in `pthread_cond_timedwait();
    pthread_cond_t work_cond_var;
    // Mutex lock used in `pthread_cond_timedwait();
    pthread_mutex_t wait_lock;

    atomic_bool has_thread;
    atomic_bool has_task;

    // Lock that should be used when accessing function & args
    pthread_mutex_t data_lock;
    void *(*function)(void *);
    void *args;
    task_output *output;
} thread_data;

int tholder_create(tholder_t *__restrict __newthread,
                         const pthread_attr_t *__restrict __attr,
                         void *(*__start_routine)(void *),
                         void *__restrict __arg);

int tholder_join(tholder_t th, void **thread_return);

void tholder_init(size_t num_threads);

thread_data *thread_data_init(size_t index);

void tholder_destroy();

void *auxiliary_function(void *args);

thread_data *get_inactive_index();
