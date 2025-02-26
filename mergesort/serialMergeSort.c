#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

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

// Serial merge sort (pure recursive implementation without threading)
void merge_sort_serial(int *arr, int left, int right)
{
    if (left < right)
    {
        int mid = left + (right - left) / 2;

        merge_sort_serial(arr, left, mid);
        merge_sort_serial(arr, mid + 1, right);

        merge(arr, left, mid, right);
    }
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
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <size1> [size2 ...]\n", argv[0]);
        exit(1);
    }

    int num_sizes = argc - 1;

    srand(0);

    for (int t = 0; t < num_sizes; t++)
    {
        int n = atoi(argv[t + 1]);
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

        // Create a copy for serial sorting
        int *arr_serial = malloc(n * sizeof(int));
        if (!arr_serial)
        {
            fprintf(stderr, "Memory allocation failed\n");
            exit(1);
        }

        for (int i = 0; i < n; i++)
        {
            arr_serial[i] = orig[i];
        }

        free(orig);

        struct timespec start_time, end_time;
        double time_serial;

        // Measure serial merge sort time using clock_gettime
        clock_gettime(CLOCK_MONOTONIC, &start_time);
        merge_sort_serial(arr_serial, 0, n - 1);
        clock_gettime(CLOCK_MONOTONIC, &end_time);

        time_serial = difftimespec_ns(end_time, start_time);

        printf("Test case %d, size %d\n", t + 1, n);
        printf("Serial merge sort time:   %f nanoseconds\n", time_serial);

        if (n <= 100)
        {
            printf("Serial Sorted:   ");
            print_array(arr_serial, n);
        }

        //For passing the printed output to the CSV output line for the Python pipeline
        printf("PERFDATA,%d,serialMergeSort,1,0,0,%f\n", n, time_serial);

        free(arr_serial);
    }
    return 0;
}
