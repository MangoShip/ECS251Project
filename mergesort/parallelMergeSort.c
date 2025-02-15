#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <math.h>

#define MIN_SIZE_FOR_PARALLEL 10  // Minimum subarray size to use parallel threads
#define THREAD_STACK_SIZE (1 << 20) // 1 MB stack for each thread

// Global maximum depth for parallel recursion, computed from desired thread count.
int global_max_depth;

// Calculate the difference between two timespecs in nanoseconds
double difftimespec_ns(const struct timespec after, const struct timespec before)
{
    return ((double)after.tv_sec - (double)before.tv_sec) * 1e9 +
           ((double)after.tv_nsec - (double)before.tv_nsec);
}

// Merge two sorted subarrays [left, mid] and [mid+1, right]
void merge(int *arr, int left, int mid, int right)
{
    int size_left = mid - left + 1;
    int size_right = right - mid;

    int *left_arr = malloc(size_left * sizeof(int));
    int *right_arr = malloc(size_right * sizeof(int));

    for (int i = 0; i < size_left; i++)
    {
        left_arr[i] = arr[left + i];
    }
    for (int i = 0; i < size_right; i++)
    {
        right_arr[i] = arr[mid + 1 + i];
    }

    int i = 0, j = 0, k = left;
    while (i < size_left && j < size_right)
    {
        if (left_arr[i] <= right_arr[j])
        {
            arr[k++] = left_arr[i++];
        }
        else
        {
            arr[k++] = right_arr[j++];
        }
    }
    while (i < size_left)
    {
        arr[k++] = left_arr[i++];
    }
    while (j < size_right)
    {
        arr[k++] = right_arr[j++];
    }

    free(left_arr);
    free(right_arr);
}

// Structure to pass arguments to merge_sort threads (parallel version)
typedef struct
{
    int *arr;
    int left;
    int right;
    int depth;
} merge_args;

void merge_sort_depth(int *arr, int left, int right, int depth);
void *merge_sort_thread(void *arg);

// Unpacks arguments and calls merge_sort_depth
void *merge_sort_thread(void *arg)
{
    merge_args *args = (merge_args *)arg;
    merge_sort_depth(args->arr, args->left, args->right, args->depth);
    free(args);
    return NULL;
}

// Recursive parallel merge sort with a depth parameter
void merge_sort_depth(int *arr, int left, int right, int depth)
{
    if (left < right)
    {
        int mid = left + (right - left) / 2;

        // Spawn threads only if depth is below global_max_depth and subarray is large
        if (depth < global_max_depth && (right - left) >= MIN_SIZE_FOR_PARALLEL)
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
            args1->left = left;
            args1->right = mid;
            args1->depth = depth + 1;

            args2->arr = arr;
            args2->left = mid + 1;
            args2->right = right;
            args2->depth = depth + 1;

            pthread_attr_t attr;
            pthread_attr_init(&attr);
            pthread_attr_setstacksize(&attr, THREAD_STACK_SIZE);

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

            merge(arr, left, mid, right);
        }
        else
        {
            // Serial recursion when subarray is small or maximum depth reached
            merge_sort_depth(arr, left, mid, depth);
            merge_sort_depth(arr, mid + 1, right, depth);
            merge(arr, left, mid, right);
        }
    }
}

void merge_sort_parallel(int *arr, int left, int right)
{
    merge_sort_depth(arr, left, right, 0);
}

// Fisher-Yates shuffle makes sure that ordering of elements doesn't affect performance
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
    // Expect at least two arguments: number of threads and at least one size
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <num_threads> <size1> [size2 ...]\n", argv[0]);
        exit(1);
    }

    int desired_threads = atoi(argv[1]);
    if (desired_threads <= 0)
    {
        fprintf(stderr, "Number of threads must be positive.\n");
        exit(1);
    }
    // Compute global_max_depth = ceil(log2(desired_threads))
    global_max_depth = (int)ceil(log2(desired_threads));

    srand(0);

    int num_sizes = argc - 2;

    for (int t = 0; t < num_sizes; t++)
    {
        int n = atoi(argv[t + 2]);
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
        // Shuffle the original array
        shuffle(orig, n);

        // Create copies for parallel and serial sorting
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

        // For small arrays, print the sorted output
        if (n <= 100)
        {
            printf("Parallel Sorted: ");
            print_array(arr_parallel, n);
        }
        free(arr_parallel);
    }
    return 0;
}
