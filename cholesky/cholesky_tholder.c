#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "../tholder/tholder.h"
#include <time.h>

// Define seed for reproducibility
#define SEED 42 

// ---------------------------
// Global variables for matrices
// ---------------------------
static double **A = NULL;  // Input matrix (N x N)
static double **L = NULL;  // Output lower-triangular matrix (N x N)
static int N = 0;          // Matrix size
static int NUM_THREADS = 0;

// ---------------------------
// Structure for passing work to each thread (for off-diagonal update)
// ---------------------------
typedef struct {
    int j;       // The current column index
    int start;   // Starting row index (inclusive)
    int end;     // Ending row index (exclusive)
} thread_data_t;

// ---------------------------
// Helper functions: allocate_matrix, free_matrix, generate_positive_definite_matrix, print_matrix
// ---------------------------
static double **allocate_matrix(int size) {
    double **matrix = (double **)malloc(size * sizeof(double *));
    for (int i = 0; i < size; i++) {
        matrix[i] = (double *)malloc(size * sizeof(double));
    }
    return matrix;
}

static void free_matrix(double **matrix, int size) {
    for (int i = 0; i < size; i++) {
        free(matrix[i]);
    }
    free(matrix);
}

static void generate_positive_definite_matrix(double **matrix, int size) {
    srand(SEED);
    // Fill with random values in [1,10]
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            matrix[i][j] = rand() % 10 + 1;
        }
    }
    // Form a positive-definite matrix via A = A * A^T
    double **temp = allocate_matrix(size);
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            double sum = 0.0;
            for (int k = 0; k < size; k++) {
                sum += matrix[i][k] * matrix[j][k];
            }
            temp[i][j] = sum;
        }
    }
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            matrix[i][j] = temp[i][j];
        }
    }
    free_matrix(temp, size);
}

static void print_matrix(double **matrix, int size) {
    int limit = (size < 5) ? size : 5;
    for (int i = 0; i < limit; i++) {
        for (int j = 0; j < limit; j++) {
            printf("%8.4f ", matrix[i][j]);
        }
        printf("\n");
    }
}

// ---------------------------
// Worker function for off-diagonal update of one column.
// This function will be invoked by each thread in the thread pool created
// for a given column's parallel section.
// ---------------------------
void *offdiag_worker(void *arg) {
    thread_data_t *data = (thread_data_t *) arg;
    int j = data->j;
    for (int i = data->start; i < data->end; i++) {
        double sum = 0.0;
        for (int k = 0; k < j; k++) {
            sum += L[i][k] * L[j][k];
        }
        L[i][j] = (A[i][j] - sum) / L[j][j];
    }
    printf("   [Worker] Processed rows [%d, %d) for column %d.\n", data->start, data->end, j);
    return NULL;
}

// ---------------------------
// cholesky_parallel_multiple:
// This function implements the Cholesky Decomposition using a parallel-serial-parallel structure,
// but it forces multiple thread pools to be spawned: one new thread pool is created for each column's
// off-diagonal (parallel) update, then destroyed, before moving on to the next column.
// ---------------------------
void cholesky_parallel_multiple() {
    // Loop over each column j
    for (int j = 0; j < N; j++) {
        // SERIAL PORTION: Compute diagonal element L[j][j]
        printf("Main thread: [Serial] Computing diagonal for column %d...\n", j);
        double sum = 0.0;
        for (int k = 0; k < j; k++) {
            sum += L[j][k] * L[j][k];
        }
        L[j][j] = sqrt(A[j][j] - sum);
        printf("Main thread: [Serial] Computed L[%d][%d] = %f\n", j, j, L[j][j]);

        // If there are no rows below the diagonal, continue.
        if (j + 1 >= N) {
            continue;
        }

        // PARALLEL PORTION: Spawn a new thread pool for the off-diagonal updates in column j.
        printf("Main thread: [Parallel] Spawning new thread pool for column %d off-diagonals...\n", j);

        int total_rows = N - (j + 1);  // rows to update: j+1, ..., N-1
        int num_threads = NUM_THREADS; // Use NUM_THREADS for this column
        tholder_t *threads = (tholder_t *)malloc(num_threads * sizeof(tholder_t));
        thread_data_t *tdata = (thread_data_t *)malloc(num_threads * sizeof(thread_data_t));

        // Partition the rows among the threads.
        int base = total_rows / num_threads;
        int rem  = total_rows % num_threads;
        for (int t = 0; t < num_threads; t++) {
            tdata[t].j = j;
            tdata[t].start = (j + 1) + t * base + (t < rem ? t : rem);
            tdata[t].end   = tdata[t].start + base + (t < rem ? 1 : 0);
            printf("   Main thread: Thread %d will process rows [%d, %d) for column %d.\n",
                   t, tdata[t].start, tdata[t].end, j);
            tholder_create(&threads[t], NULL, offdiag_worker, &tdata[t]);
        }

        // Wait for all threads in this pool to complete.
        for (int t = 0; t < num_threads; t++) {
            tholder_join(threads[t], NULL);
        }
        printf("Main thread: [Parallel] Completed off-diagonal updates for column %d.\n", j);

        // Free the resources for this thread pool.
        free(threads);
        free(tdata);
    }
}

// ---------------------------
// main: Setup matrices, run the parallel Cholesky decomposition (with multiple thread pools),
// then print results and execution time.
// ---------------------------
int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <matrix_size> <num_threads>\n", argv[0]);
        return EXIT_FAILURE;
    }
    N = atoi(argv[1]);
    NUM_THREADS = atoi(argv[2]);
    if (N <= 0 || NUM_THREADS <= 0) {
        fprintf(stderr, "Error: matrix_size and num_threads must be positive.\n");
        return EXIT_FAILURE;
    }

    // Allocate matrices A and L.
    A = allocate_matrix(N);
    L = allocate_matrix(N);

    // Generate a random positive-definite matrix.
    generate_positive_definite_matrix(A, N);

    // Initialize L to zeros.
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            L[i][j] = 0.0;
        }
    }

    printf("\nInitial Matrix A (top-left 5x5):\n");
    print_matrix(A, N);

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    // Perform the Cholesky Decomposition, spawning a new thread pool for each column.
    cholesky_parallel_multiple();

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed_time = (end.tv_sec - start.tv_sec) + 
                          (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("\nCholesky Decomposition (L Matrix, top-left 5x5):\n");
    print_matrix(L, N);
    printf("\nExecution Time: %.6f seconds\n", elapsed_time);

    free_matrix(A, N);
    free_matrix(L, N);

    return EXIT_SUCCESS;
}
