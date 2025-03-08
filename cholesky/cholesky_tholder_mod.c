#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "../tholder/tholder.h"
#include <time.h>
#include <sys/stat.h>   // For mkdir, stat
#include <sys/types.h>  // For mode_t

#define SEED 42

// ---------------------------
// Global Variables for Matrices
// ---------------------------
static double **A = NULL;  // Input matrix (N x N)
static double **L = NULL;  // Output lower-triangular matrix (N x N)
static int N = 0;          // Matrix size
static int NUM_THREADS = 0; // Number of threads to use

// ---------------------------
// Structure for Off-Diagonal Work
// ---------------------------
typedef struct {
    int j;       // The current column index
    int start;   // Starting row index (inclusive) for off-diagonal update
    int end;     // Ending row index (exclusive)
} thread_data_t;

// ---------------------------
// Global File Pointers for Logging
// ---------------------------
FILE *result_fp = NULL;   // For results output
FILE *threads_fp = NULL;  // For thread activity logs

// ---------------------------
// Matrix Helper Functions
// ---------------------------
double **allocate_matrix(int N) {
    double **matrix = (double **)malloc(N * sizeof(double *));
    for (int i = 0; i < N; i++) {
        matrix[i] = (double *)malloc(N * sizeof(double));
    }
    return matrix;
}

void free_matrix(double **matrix, int N) {
    for (int i = 0; i < N; i++) {
        free(matrix[i]);
    }
    free(matrix);
}

void generate_positive_definite_matrix(double **A, int N) {
    srand(SEED);
    // Fill with random values in [1,10]
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            A[i][j] = rand() % 10 + 1;
        }
    }
    // Form A = A * A^T
    double **temp = allocate_matrix(N);
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            double sum = 0.0;
            for (int k = 0; k < N; k++) {
                sum += A[i][k] * A[j][k];
            }
            temp[i][j] = sum;
        }
    }
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            A[i][j] = temp[i][j];
        }
    }
    free_matrix(temp, N);
}

void print_matrix(double **matrix, int N, FILE *fp) {
    int limit = (N < 5) ? N : 5;
    for (int i = 0; i < limit; i++) {
        for (int j = 0; j < limit; j++) {
            fprintf(fp, "%8.4f ", matrix[i][j]);
        }
        fprintf(fp, "\n");
    }
}

void print_matrix_stdout(double **matrix, int N) {
    int limit = (N < 5) ? N : 5;
    for (int i = 0; i < limit; i++) {
        for (int j = 0; j < limit; j++) {
            printf("%8.4f ", matrix[i][j]);
        }
        printf("\n");
    }
}

// ---------------------------
// Thread Pool: Off-Diagonal Work per Column
// For each column j, we spawn a new thread pool (multiple threads) to compute
// the off-diagonal entries in that column.
// ---------------------------
void *offdiag_worker(void *arg) {
    thread_data_t *data = (thread_data_t *)arg;
    int j = data->j;
    for (int i = data->start; i < data->end; i++) {
        double sum = 0.0;
        for (int k = 0; k < j; k++) {
            sum += L[i][k] * L[j][k];
        }
        L[i][j] = (A[i][j] - sum) / L[j][j];
    }
    fprintf(threads_fp, "   [Worker] Processed rows [%d, %d) for column %d.\n", data->start, data->end, j);
    return NULL;
}

// ---------------------------
// cholesky_parallel_multiple:
// Implements the parallel-serial-parallel structure with a new thread pool for each column's
// off-diagonal updates. Logs thread activity to threads_fp.
// ---------------------------
void cholesky_parallel_multiple() {
    for (int j = 0; j < N; j++) {
        // SERIAL PORTION: Compute diagonal element L[j][j]
        fprintf(threads_fp, "Main thread: [Serial] Computing diagonal for column %d...\n", j);
        double sum = 0.0;
        for (int k = 0; k < j; k++) {
            sum += L[j][k] * L[j][k];
        }
        L[j][j] = sqrt(A[j][j] - sum);
        fprintf(threads_fp, "Main thread: [Serial] Computed L[%d][%d] = %f\n", j, j, L[j][j]);

        // If no off-diagonals exist, move to next column.
        if (j + 1 >= N)
            continue;

        // PARALLEL PORTION: Spawn a new thread pool for this column.
        fprintf(threads_fp, "Main thread: [Parallel] Spawning thread pool for column %d off-diagonals...\n", j);
        int total_rows = N - (j + 1);
        int num_threads = NUM_THREADS;
        tholder_t *threads = (tholder_t *)malloc(num_threads * sizeof(tholder_t));
        thread_data_t *tdata = (thread_data_t *)malloc(num_threads * sizeof(thread_data_t));

        // Partition rows among threads.
        int base = total_rows / num_threads;
        int rem  = total_rows % num_threads;
        for (int t = 0; t < num_threads; t++) {
            tdata[t].j = j;
            tdata[t].start = (j + 1) + t * base + (t < rem ? t : rem);
            tdata[t].end = tdata[t].start + base + (t < rem ? 1 : 0);
            fprintf(threads_fp, "   Main thread: Thread %d will process rows [%d, %d) for column %d.\n",
                    t, tdata[t].start, tdata[t].end, j);
            tholder_create(&threads[t], NULL, offdiag_worker, &tdata[t]);
        }

        // Wait for all threads in this pool to complete.
        for (int t = 0; t < num_threads; t++) {
            tholder_join(threads[t], NULL);
        }
        fprintf(threads_fp, "Main thread: [Parallel] Completed off-diagonal updates for column %d.\n", j);

        free(threads);
        free(tdata);
    }
}

// ---------------------------
// main:
// Usage: ./cholesky_parallel_mod <matrix_size> <num_threads> [<test_number>]
// If test_number is not provided, it defaults to 1.
// This program writes two files:
//   1. Results: [matrix_size]_[test_number].txt in directory "tholder_tests/<matrix_size>/".
//   2. Thread logs: [matrix_size]_[test_number]_threads.txt in the same directory.
// ---------------------------
int main(int argc, char *argv[]) {
    if (argc < 3 || argc > 4) {
        fprintf(stderr, "Usage: %s <matrix_size> <num_threads> [<test_number>]\n", argv[0]);
        return EXIT_FAILURE;
    }

    N = atoi(argv[1]);
    NUM_THREADS = atoi(argv[2]);
    if (N <= 0 || NUM_THREADS <= 0) {
        fprintf(stderr, "Error: matrix_size and num_threads must be positive.\n");
        return EXIT_FAILURE;
    }

    int test_number = 1;
    if (argc == 4) {
        test_number = atoi(argv[3]);
    }

    // Allocate matrices A and L.
    A = allocate_matrix(N);
    L = allocate_matrix(N);

    // Generate random positive-definite matrix.
    generate_positive_definite_matrix(A, N);

    // Initialize L to zeros.
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            L[i][j] = 0.0;
        }
    }

    printf("\nInitial Matrix A (top-left 5x5):\n");
    print_matrix(A, N, stdout);

    // -----------------------------
    // Prepare directory structure:
    // Create "tholder_tests" if it doesn't exist,
    // and a subdirectory named with the matrix size.
    // -----------------------------
    struct stat st = {0};
    if (stat("tholder_tests", &st) == -1) {
        #ifdef _WIN32
            mkdir("tholder_tests");
        #else
            mkdir("tholder_tests", 0700);
        #endif
    }
    char subdir[256];
    snprintf(subdir, sizeof(subdir), "tholder_tests/%d", N);
    if (stat(subdir, &st) == -1) {
        #ifdef _WIN32
            mkdir(subdir);
        #else
            mkdir(subdir, 0700);
        #endif
    }

    // Construct filenames for results and thread logs.
    char result_filename[256];
    char threads_filename[256];
    snprintf(result_filename, sizeof(result_filename), "%s/%d_%d.txt", subdir, N, test_number);
    snprintf(threads_filename, sizeof(threads_filename), "%s/%d_%d_threads.txt", subdir, N, test_number);

    // Open the thread log file.
    threads_fp = fopen(threads_filename, "w");
    if (threads_fp == NULL) {
        perror("Error opening thread log file");
        return EXIT_FAILURE;
    }

    // Time the Cholesky decomposition.
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    cholesky_parallel_multiple();

    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double elapsed_time = (end_time.tv_sec - start_time.tv_sec) +
                          (end_time.tv_nsec - start_time.tv_nsec) / 1e9;

    printf("\nCholesky Decomposition (L Matrix, top-left 5x5):\n");
    print_matrix(L, N, stdout);
    printf("\nExecution Time: %.6f seconds\n", elapsed_time);

    // Open results file and write results.
    result_fp = fopen(result_filename, "w");
    if (result_fp == NULL) {
        perror("Error opening result file for writing");
        return EXIT_FAILURE;
    }
    fprintf(result_fp, "Matrix Size: %d\nTest Number: %d\n\n", N, test_number);
    fprintf(result_fp, "Initial Matrix A (top-left 5x5):\n");
    print_matrix(A, N, result_fp);
    fprintf(result_fp, "\nCholesky Decomposition (L Matrix, top-left 5x5):\n");
    print_matrix(L, N, result_fp);
    fprintf(result_fp, "\nExecution Time: %.6f seconds\n", elapsed_time);
    fclose(result_fp);
    fclose(threads_fp);
    printf("\nResults written to %s\n", result_filename);
    printf("Thread activity log written to %s\n", threads_filename);

    // Free resources.
    free_matrix(A, N);
    free_matrix(L, N);

    return EXIT_SUCCESS;
}
