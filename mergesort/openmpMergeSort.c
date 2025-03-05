#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>
#include <math.h>
#include <string.h>

int global_min_parallel_size = 1000;  // Default minimum subarray size for OpenMP tasks
int global_thread_stack_size = 0;       // Not used in OpenMP, but recorded for CSV output

// Global maximum recursion depth for parallelism, computed from the desired thread count.
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
    if (!left_arr || !right_arr)
    {
        perror("malloc");
        exit(1);
    }
    for (int i = 0; i < size_left; i++)
        left_arr[i] = arr[left + i];
    for (int i = 0; i < size_right; i++)
        right_arr[i] = arr[mid + 1 + i];

    int i = 0, j = 0, k = left;
    while (i < size_left && j < size_right)
    {
        if (left_arr[i] <= right_arr[j])
            arr[k++] = left_arr[i++];
        else
            arr[k++] = right_arr[j++];
    }
    while (i < size_left)
        arr[k++] = left_arr[i++];
    while (j < size_right)
        arr[k++] = right_arr[j++];
    free(left_arr);
    free(right_arr);
}

// Recursive parallel merge sort with a depth parameter using OpenMP tasks
void merge_sort_omp_depth(int *arr, int left, int right, int depth)
{
    if (left < right)
    {
        int mid = left + (right - left) / 2;
        // If we are below the maximum parallel depth and the subarray is large enough, spawn tasks.
        if (depth < global_max_depth && (right - left) >= global_min_parallel_size)
        {
#pragma omp task shared(arr) firstprivate(left, mid, depth)
            {
                merge_sort_omp_depth(arr, left, mid, depth + 1);
            }
#pragma omp task shared(arr) firstprivate(mid, right, depth)
            {
                merge_sort_omp_depth(arr, mid + 1, right, depth + 1);
            }
#pragma omp taskwait
            merge(arr, left, mid, right);
        }
        else
        {
            // Otherwise, perform serial recursion.
            merge_sort_omp_depth(arr, left, mid, depth);
            merge_sort_omp_depth(arr, mid + 1, right, depth);
            merge(arr, left, mid, right);
        }
    }
}

// Wrapper for OpenMP parallel merge sort that starts a parallel region
void merge_sort_parallel_omp(int *arr, int left, int right)
{
#pragma omp parallel
    {
#pragma omp single
        {
            merge_sort_omp_depth(arr, left, right, 0);
        }
    }
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

    omp_set_num_threads(desired_threads);

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

        clock_gettime(CLOCK_MONOTONIC, &start_time);
        merge_sort_parallel_omp(arr_parallel, 0, n - 1);
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
        printf("PERFDATA,%d,openmpMergeSort,%d,%d,%d,%f\n", n, desired_threads, global_min_parallel_size, global_thread_stack_size, time_parallel);

        free(arr_parallel);
    }
    return 0;
}
