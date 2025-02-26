#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

/*
 * thread_gtod.c: thread.c that uses gettimeofday() clock
*/

volatile double gtod(void)
{
  static struct timeval tv;
  static struct timezone tz;
  gettimeofday(&tv,&tz);
  return tv.tv_sec*1e6 + tv.tv_usec;
}

// Thread argument
struct thread_info {
    int thread_id;
    double launch_time;
    double work_start_time;
    double work_end_time;
    double destroy_time;
};

// Thread work
void *do_something(void *arg)
{
    struct thread_info *tinfo = (struct thread_info *) arg;
    tinfo->work_start_time = gtod();
    sleep(3);
    tinfo->work_end_time = gtod();
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
    double start_time, end_time;

    // Initialize empty thread array
    pthread_t thread_array[num_threads];

    start_time = gtod();
    for (uint64_t i = 0; i < num_threads; i++)
    {
        tinfo[i].thread_id = i;
        tinfo[i].launch_time = gtod();
        pthread_create(&thread_array[i], NULL, do_something, (void *)&tinfo[i]);
    }

    for (uint64_t i = 0; i < num_threads; i++)
    {
        pthread_join(thread_array[i], NULL);
        tinfo[i].destroy_time = gtod();
    }
    end_time = gtod();
    printf("Done!\n");

    for (uint64_t i = 0; i < num_threads; i++)
    {
        double launch_duration = ((double)(tinfo[i].work_start_time - tinfo[i].launch_time));
        double work_duration = ((double)(tinfo[i].work_end_time - tinfo[i].work_start_time));
        double destroy_duration = ((double)(tinfo[i].destroy_time - tinfo[i].work_end_time));
        printf("Thread ID %d - Launch Time: %f μs, Work Time: %f μs, Destroy Time: %f μs\n", tinfo[i].thread_id, launch_duration, work_duration, destroy_duration);
    }

    printf("Execution Time: %f μs\n", ((double)(end_time - start_time)));

    free(tinfo);
}
