#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <math.h>
#include <string.h>

int global_min_parallel_size = 1000;       // Increased minimum subarray size for parallel threads
int global_thread_stack_size = (1 << 20);    // Default thread stack size (1 MB)
int global_max_depth;  // Computed from desired thread count

// Calculate the difference between two timespecs in nanoseconds
double difftimespec_ns(const struct timespec after, const struct timespec before)
{
    return ((double)after.tv_sec - (double)before.tv_sec) * 1e9 +
           ((double)after.tv_nsec - (double)before.tv_nsec);
}

// Optimized merge that uses a preallocated temporary buffer.
void merge(int *arr, int *temp, int left, int mid, int right)
{
    int i = left, j = mid + 1, k = left;
    while(i <= mid && j <= right)
    {
        if(arr[i] <= arr[j])
            temp[k++] = arr[i++];
        else
            temp[k++] = arr[j++];
    }
    while(i <= mid)
        temp[k++] = arr[i++];
    while(j <= right)
        temp[k++] = arr[j++];
    for(i = left; i <= right; i++)
        arr[i] = temp[i];
}

// Structure to pass arguments to merge_sort threads (parallel version).
typedef struct
{
    int *arr;
    int *temp;
    int left;
    int right;
    int depth;
} merge_args;

// Forward declaration for merge_sort_depth.
void merge_sort_depth(int *arr, int *temp, int left, int right, int depth);

// Unpacks arguments and calls merge_sort_depth
void *merge_sort_thread(void *arg)
{
    merge_args *args = (merge_args *)arg;
    merge_sort_depth(args->arr, args->temp, args->left, args->right, args->depth);
    free(args);
    return NULL;
}

// Recursive parallel merge sort using pthreads and a preallocated buffer.
void merge_sort_depth(int *arr, int *temp, int left, int right, int depth)
{
    if (left < right)
    {
        int mid = left + (right - left) / 2;

        // Spawn threads only if depth is below global_max_depth and subarray is large
        if (depth < global_max_depth && (right - left) >= global_min_parallel_size)
        {
            pthread_t tid1, tid2;
            merge_args *args1 = malloc(sizeof(merge_args));
            merge_args *args2 = malloc(sizeof(merge_args));

            if (!args1 || !args2)
            {
                perror("malloc");
                exit(1);
            }

            args1->arr = arr;
            args1->temp = temp;
            args1->left = left;
            args1->right = mid;
            args1->depth = depth + 1;

            args2->arr = arr;
            args2->temp = temp;
            args2->left = mid + 1;
            args2->right = right;
            args2->depth = depth + 1;

            pthread_attr_t attr;
            pthread_attr_init(&attr);
            pthread_attr_setstacksize(&attr, global_thread_stack_size);

            if (pthread_create(&tid1, &attr, merge_sort_thread, args1) != 0)
            {
                perror("pthread_create");
                exit(1);
            }
            if (pthread_create(&tid2, &attr, merge_sort_thread, args2) != 0)
            {
                perror("pthread_create");
                exit(1);
            }
            pthread_attr_destroy(&attr);

            pthread_join(tid1, NULL);
            pthread_join(tid2, NULL);

            merge(arr, temp, left, mid, right);
        }
        else
        {
            merge_sort_depth(arr, temp, left, mid, depth);
            merge_sort_depth(arr, temp, mid + 1, right, depth);
            merge(arr, temp, left, mid, right);
        }
    }
}

// Wrapper for the pthread parallel merge sort that preallocates the temporary buffer.
void merge_sort_parallel(int *arr, int left, int right)
{
    int n = right - left + 1;
    int *temp = malloc(n * sizeof(int));
    if (!temp)
    {
        perror("malloc");
        exit(1);
    }
    merge_sort_depth(arr, temp, left, right, 0);
    free(temp);
}

/* Remaining functions unchanged */
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
    {
        printf("%d ", arr[i]);
    }
    printf("\n");
}

int main(int argc, char *argv[])
{
    // Existing usage: at least <num_threads> and one size.
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

    global_max_depth = (int)ceil(log2(desired_threads));

    // Parse optional flags: -m and -s before the list of sizes.
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

        // Allocate and initialize the original array with random values
        int *orig = malloc(n * sizeof(int));
        if (!orig)
        {
            fprintf(stderr, "Memory allocation failed\n");
            exit(1);
        }
        for (int i = 0; i < n; i++)
        {
            orig[i] = rand() % n;
        }

        shuffle(orig, n);

        int *arr_parallel = malloc(n * sizeof(int));
        if (!arr_parallel)
        {
            fprintf(stderr, "Memory allocation failed\n");
            exit(1);
        }
        for (int i = 0; i < n; i++)
        {
            arr_parallel[i] = orig[i];
        }
        free(orig);

        struct timespec start_time, end_time;
        double time_parallel;

        // Measure parallel merge sort time
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

        //For passing the printed output to the CSV output line for the Python pipeline
        printf("PERFDATA,%d,parallelMergeSort,%d,%d,%d,%f\n", n, desired_threads, global_min_parallel_size, global_thread_stack_size, time_parallel);

        free(arr_parallel);
    }

    return 0;
}
