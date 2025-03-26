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

// Serial merge sort helper (pure recursive implementation without threading)
void merge_sort_serial_helper(int *arr, int *temp, int left, int right)
{
    if (left < right)
    {
        int mid = left + (right - left) / 2;
        merge_sort_serial_helper(arr, temp, left, mid);
        merge_sort_serial_helper(arr, temp, mid + 1, right);
        merge(arr, temp, left, mid, right);
    }
}

// Serial merge sort (pure recursive implementation without threading)
void merge_sort_serial(int *arr, int left, int right)
{
    int n = right - left + 1;
    int *temp = malloc(n * sizeof(int));
    if (!temp)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    merge_sort_serial_helper(arr, temp, left, right);
    free(temp);
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
