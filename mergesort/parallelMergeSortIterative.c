#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <math.h>
#include <string.h>

int global_min_parallel_size = 1000;
int global_thread_stack_size = (1 << 20); // Default thread stack size (1 MB)
int global_max_depth;
int global_desired_threads;

// Calculate the difference between two timespecs in nanoseconds.
double difftimespec_ns(const struct timespec after, const struct timespec before)
{
    return ((double)after.tv_sec - (double)before.tv_sec) * 1e9 +
           ((double)after.tv_nsec - (double)before.tv_nsec);
}

// Optimized merge using a preallocated temporary buffer.
// It merges the subarrays [left, mid] and [mid+1, right] into temp,
// then copies the result back to arr.
void merge(int *arr, int *temp, int left, int mid, int right)
{
    int i = left, j = mid + 1, k = left;
    while (i <= mid && j <= right)
    {
        if (arr[i] <= arr[j])
            temp[k++] = arr[i++];
        else
            temp[k++] = arr[j++];
    }
    while (i <= mid)
        temp[k++] = arr[i++];
    while (j <= right)
        temp[k++] = arr[j++];
    for (i = left; i <= right; i++)
        arr[i] = temp[i];
}

// Structure used when spawning one thread per merge (the “individual thread” path).
typedef struct
{
    int *arr;
    int *temp;
    int left;
    int mid;
    int right;
} merge_params;

void *merge_thread(void *arg)
{
    merge_params *mp = (merge_params *)arg;
    merge(mp->arr, mp->temp, mp->left, mp->mid, mp->right);
    free(mp);
    return NULL;
}

// Structure used to pass a batch of merge tasks to a single thread.
// This allows one thread to process several merge operations in one pass.
typedef struct
{
    int *arr;
    int *temp;
    int curr_size;
    int left;       // starting index of the overall merge pass
    int right;      // ending index of the overall merge pass
    int start_task; // index of first merge task (0-indexed) assigned to this thread
    int end_task;   // index (non-inclusive) of the merge tasks for this thread
} merge_pass_args;

void *merge_pass_thread(void *arg)
{
    merge_pass_args *margs = (merge_pass_args *)arg;
    int merge_task = 0;

    // Iterate over all merge tasks in this pass.
    for (int i = margs->left; i <= margs->right - margs->curr_size; i += 2 * margs->curr_size)
    {
        // Only perform the merge if the current task index is within our assigned batch.
        if (merge_task >= margs->start_task && merge_task < margs->end_task)
        {
            int mid = i + margs->curr_size - 1;
            int r = (i + 2 * margs->curr_size - 1 <= margs->right) ? i + 2 * margs->curr_size - 1 : margs->right;
            merge(margs->arr, margs->temp, i, mid, r);
        }
        merge_task++;
    }

    free(margs);
    return NULL;
}

// Iterative (bottom‑up) merge sort using pthreads.
// A temporary buffer is allocated once in the wrapper and reused for all merges.
// For each merge pass:
// The code counts the number of merge tasks.
// If there are more merge tasks than global_desired_threads, it partitions the tasks among
// a fixed set of threads (reducing thread creation overhead).
// Otherwise, it spawns individual threads for each merge as before.
void merge_sort_parallel(int *arr, int left, int right)
{
    int n = right - left + 1;
    int *temp = malloc(n * sizeof(int));
    if (!temp)
    {
        perror("malloc");
        exit(1);
    }

    for (int curr_size = 1; curr_size < n; curr_size *= 2)
    {
        // First count the number of merge tasks in this pass.
        int num_merges = 0;
        for (int i = left; i <= right - curr_size; i += 2 * curr_size)
            num_merges++;

        // If many merge tasks exist, partition them among a fixed set of threads.
        if (num_merges > global_desired_threads)
        {
            int num_threads = global_desired_threads;
            pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
            int tasks_per_thread = (num_merges + num_threads - 1) / num_threads;
            int current_task = 0;
            for (int t = 0; t < num_threads; t++)
            {
                merge_pass_args *margs = malloc(sizeof(merge_pass_args));
                if (!margs)
                {
                    perror("malloc");
                    exit(1);
                }
                margs->arr = arr;
                margs->temp = temp;
                margs->curr_size = curr_size;
                margs->left = left;
                margs->right = right;
                margs->start_task = current_task;
                margs->end_task = (current_task + tasks_per_thread < num_merges) ? current_task + tasks_per_thread : num_merges;
                current_task = margs->end_task;
                if (pthread_create(&threads[t], NULL, merge_pass_thread, margs) != 0)
                {
                    perror("pthread_create");
                    exit(1);
                }
            }
            for (int t = 0; t < num_threads; t++)
            {
                pthread_join(threads[t], NULL);
            }
            free(threads);
        }
        else
        {
            // If there are few merge tasks, spawn one thread per merge as before.
            int merge_count = 0;
            pthread_t *threads = malloc(num_merges * sizeof(pthread_t));
            for (int i = left; i <= right - curr_size; i += 2 * curr_size)
            {
                int mid = i + curr_size - 1;
                int r = (i + 2 * curr_size - 1 <= right) ? i + 2 * curr_size - 1 : right;
                // For small merge sizes, do it serially.
                if ((r - i + 1) < global_min_parallel_size)
                {
                    merge(arr, temp, i, mid, r);
                }
                else
                {
                    merge_params *mp = malloc(sizeof(merge_params));
                    if (!mp)
                    {
                        perror("malloc");
                        exit(1);
                    }
                    mp->arr = arr;
                    mp->temp = temp;
                    mp->left = i;
                    mp->mid = mid;
                    mp->right = r;
                    if (pthread_create(&threads[merge_count], NULL, merge_thread, mp) != 0)
                    {
                        perror("pthread_create");
                        exit(1);
                    }
                    merge_count++;
                }
            }
            for (int t = 0; t < merge_count; t++)
            {
                pthread_join(threads[t], NULL);
            }
            free(threads);
        }
    }
    free(temp);
}

// Fisher-Yates shuffle to randomize array order
void shuffle(int *arr, int n)
{
    for (int i = n - 1; i > 0; i--)
    {
        int j = rand() % (i + 1);
        int temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
}

void print_array(int *arr, int n)
{
    for (int i = 0; i < n; i++)
        printf("%d ", arr[i]);
    printf("\n");
}

int main(int argc, char *argv[])
{
    // Usage: <num_threads> [ -m <min_parallel_size> ] [ -s <thread_stack_size> ] <size1> [size2 ...]
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <num_threads> [ -m <min_parallel_size> ] [ -s <thread_stack_size> ] <size1> [size2 ...]\n", argv[0]);
        exit(1);
    }

    int desired_threads = atoi(argv[1]);
    if (desired_threads <= 0)
    {
        fprintf(stderr, "Number of threads must be positive.\n");
        exit(1);
    }

    global_desired_threads = desired_threads;
    global_max_depth = (int)ceil(log2(desired_threads));

    // Parse optional flags: -m and -s.
    int arg_index = 2;
    while (arg_index < argc && argv[arg_index][0] == '-')
    {
        if (strcmp(argv[arg_index], "-m") == 0 && arg_index + 1 < argc)
        {
            global_min_parallel_size = atoi(argv[arg_index + 1]);
            arg_index += 2;
        }
        else if (strcmp(argv[arg_index], "-s") == 0 && arg_index + 1 < argc)
        {
            global_thread_stack_size = atoi(argv[arg_index + 1]);
            arg_index += 2;
        }
        else
        {
            arg_index++;
        }
    }
    int num_sizes = argc - arg_index;

    srand(0);

    for (int t = 0; t < num_sizes; t++)
    {
        int n = atoi(argv[arg_index + t]);
        if (n <= 0)
        {
            fprintf(stderr, "Size must be positive.\n");
            continue;
        }

        int *orig = malloc(n * sizeof(int));
        if (!orig)
        {
            fprintf(stderr, "Memory allocation failed\n");
            exit(1);
        }

        for (int i = 0; i < n; i++)
            orig[i] = rand() % n;

        shuffle(orig, n);

        int *arr_parallel = malloc(n * sizeof(int));
        if (!arr_parallel)
        {
            fprintf(stderr, "Memory allocation failed\n");
            exit(1);
        }
        for (int i = 0; i < n; i++)
            arr_parallel[i] = orig[i];

        free(orig);

        struct timespec start_time, end_time;
        double time_parallel;

        clock_gettime(CLOCK_MONOTONIC, &start_time);
        merge_sort_parallel(arr_parallel, 0, n - 1);
        clock_gettime(CLOCK_MONOTONIC, &end_time);
        time_parallel = difftimespec_ns(end_time, start_time);

        printf("Test case %d, size %d\n", t + 1, n);
        printf("Parallel merge sort time: %f nanoseconds\n", time_parallel);

        if (n <= 100)
        {
            printf("Parallel Sorted: ");
            print_array(arr_parallel, n);
        }
        printf("PERFDATA,%d,parallelMergeSort,%d,%d,%d,%f\n", n, desired_threads, global_min_parallel_size, global_thread_stack_size, time_parallel);
        free(arr_parallel);
    }
    return 0;
}
