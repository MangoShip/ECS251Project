#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

/*
 * thread_clock_gettime.c: thread.c that uses clock_gettime() clock
*/

// Credit: https://stackoverflow.com/questions/64893834/measuring-elapsed-time-using-clock-gettimeclock-monotonic
// Calculate the difference between two timepsecs in nanoseconds
double difftimespec_ns(const struct timespec after, const struct timespec before)
{
    return ((double)after.tv_sec - (double)before.tv_sec) * (double)1000000000
         + ((double)after.tv_nsec - (double)before.tv_nsec);
}

// Calculate the difference between two timepsecs in microseconds
double difftimespec_us(const struct timespec after, const struct timespec before)
{
    return ((double)after.tv_sec - (double)before.tv_sec) * (double)1000000
         + ((double)after.tv_nsec - (double)before.tv_nsec) / 1000;
}

// Thread argument
struct thread_info {
    int thread_id;
    struct timespec launch_time, work_start_time, work_end_time, destroy_time;
};

// Thread work
void *do_something(void *arg)
{
    struct thread_info *tinfo = (struct thread_info *) arg;
    clock_gettime(CLOCK_MONOTONIC, &(tinfo->work_start_time));
    sleep(3);
    clock_gettime(CLOCK_MONOTONIC, &(tinfo->work_end_time));
    return NULL;
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

    struct thread_info *tinfo = (struct thread_info *)malloc(sizeof(struct thread_info) * num_threads);
    struct timespec start_time, end_time;

    // Initialize empty thread array
    pthread_t thread_array[num_threads];

    clock_gettime(CLOCK_MONOTONIC, &start_time);
    for (uint64_t i = 0; i < num_threads; i++)
    {
        tinfo[i].thread_id = i;
        clock_gettime(CLOCK_MONOTONIC, &tinfo[i].launch_time);
        pthread_create(&thread_array[i], NULL, do_something, (void *)&tinfo[i]);
    }

    for (uint64_t i = 0; i < num_threads; i++)
    {
        pthread_join(thread_array[i], NULL);
        clock_gettime(CLOCK_MONOTONIC, &tinfo[i].destroy_time);
    }
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    printf("Done!\n");

    for (uint64_t i = 0; i < num_threads; i++)
    {
        double launch_duration = difftimespec_us(tinfo[i].work_start_time, tinfo[i].launch_time);
        double work_duration = difftimespec_us(tinfo[i].work_end_time, tinfo[i].work_start_time);
        double destroy_duration = difftimespec_us(tinfo[i].destroy_time, tinfo[i].work_end_time);
        printf("Thread ID %d - Launch Time: %f μs, Work Time: %f μs, Destroy Time: %f μs\n", tinfo[i].thread_id, launch_duration, work_duration, destroy_duration);
    }

    printf("Execution Time: %f μs\n", difftimespec_us(end_time, start_time));
    
    free(tinfo);
}
