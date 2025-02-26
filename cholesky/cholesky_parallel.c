#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <time.h>
#include <stdbool.h>

// Enable this to see verbose debug output.
// Otherwise, comment it out to reduce prints.
// #define DEBUG 1

#define SEED 42

// ------------------------------------------------------------
// Global Variables
// ------------------------------------------------------------
static double **A = NULL;      // Input matrix (N x N)
static double **L = NULL;      // Output lower-triangular matrix (N x N)
static int N = 0;              // Matrix size
static int NUM_THREADS = 0;    // Number of worker threads

// These manage the parallel loop
static volatile int current_column = -1;  // The column being processed
static volatile bool done = false;        // Signals worker threads to exit

// Barrier that includes ALL workers + the main thread
static pthread_barrier_t barrier;

// ------------------------------------------------------------
// allocate_matrix: allocates a 2D array of size N x N
// ------------------------------------------------------------
static double **allocate_matrix(int size) {
    double **matrix = (double **)malloc(size * sizeof(double *));
    for (int i = 0; i < size; i++) {
        matrix[i] = (double *)malloc(size * sizeof(double));
    }
    return matrix;
}

// ------------------------------------------------------------
// free_matrix: frees a 2D array
// ------------------------------------------------------------
static void free_matrix(double **matrix, int size) {
    for (int i = 0; i < size; i++) {
        free(matrix[i]);
    }
    free(matrix);
}

// ------------------------------------------------------------
// generate_positive_definite_matrix:
//   - fill matrix with random values in [1..10]
//   - do A <- A * A^T to make it positive-definite
// ------------------------------------------------------------
static void generate_positive_definite_matrix(double **matrix, int size) {
    srand(SEED);
    // Fill with random values
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            matrix[i][j] = rand() % 10 + 1; // [1..10]
        }
    }
    // temp = A * A^T
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
    // Copy temp back into matrix
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            matrix[i][j] = temp[i][j];
        }
    }
    free_matrix(temp, size);
}

// ------------------------------------------------------------
// print_matrix: prints the top-left 5x5 portion of matrix
// ------------------------------------------------------------
static void print_matrix(double **matrix, int size) {
    int limit = (size < 5) ? size : 5;
    for (int i = 0; i < limit; i++) {
        for (int j = 0; j < limit; j++) {
            printf("%8.4f ", matrix[i][j]);
        }
        printf("\n");
    }
}

// ------------------------------------------------------------
// Worker Thread Function
//   - Wait for main thread to set 'current_column'
//   - If 'done' is true, exit
//   - Otherwise compute rows for that column
//   - Repeat
// ------------------------------------------------------------
static void *worker(void *arg) {
    long tid = *(long *)arg;

    while (true) {
        // Barrier #1: Wait for main to set current_column
        #ifdef DEBUG
        fprintf(stderr, "[DEBUG] Thread %ld: waiting at barrier #1.\n", tid);
        #endif
        pthread_barrier_wait(&barrier);

        // Check if done is set
        if (done) {
            #ifdef DEBUG
            fprintf(stderr, "[DEBUG] Thread %ld: 'done' is true, exiting loop.\n", tid);
            #endif
            break;
        }

        int j = current_column;
        #ifdef DEBUG
        fprintf(stderr, "[DEBUG] Thread %ld: got column j=%d.\n", tid, j);
        #endif

        // Partition rows [j+1, N) among NUM_THREADS
        int total_rows = N - (j + 1);
        int base = (total_rows <= 0) ? 0 : total_rows / NUM_THREADS;
        int rem  = (total_rows <= 0) ? 0 : total_rows % NUM_THREADS;

        // Each thread gets 'base' rows, plus 1 extra row if tid < rem
        // Also add offset from remainder distribution
        int start = (j + 1) + tid * base + (tid < rem ? tid : rem);
        int count = base + (tid < rem ? 1 : 0);
        int end = start + count;

        #ifdef DEBUG
        fprintf(stderr, "[DEBUG] Thread %ld: start=%d, end=%d, total_rows=%d.\n",
                tid, start, end, total_rows);
        #endif

        // Compute L[i][j] for i in [start, end)
        for (int i = start; i < end && i < N; i++) {
            double sum = 0.0;
            for (int k = 0; k < j; k++) {
                sum += L[i][k] * L[j][k];
            }
            L[i][j] = (A[i][j] - sum) / L[j][j];
        }

        // Barrier #2: Wait until all threads finish updating column j
        #ifdef DEBUG
        fprintf(stderr, "[DEBUG] Thread %ld: waiting at barrier #2 for column j=%d.\n", tid, j);
        #endif
        pthread_barrier_wait(&barrier);

        #ifdef DEBUG
        fprintf(stderr, "[DEBUG] Thread %ld: passed barrier #2 for column j=%d.\n", tid, j);
        #endif
    }

    // Final barrier wait before fully exiting
    #ifdef DEBUG
    fprintf(stderr, "[DEBUG] Thread %ld: final barrier wait before exit.\n", tid);
    #endif
    pthread_barrier_wait(&barrier);

    #ifdef DEBUG
    fprintf(stderr, "[DEBUG] Thread %ld: fully exiting now.\n", tid);
    #endif
    return NULL;
}

// ------------------------------------------------------------
// cholesky_parallel:
//   For each column j:
//     - compute L[j][j] in main thread
//     - barrier #1: workers update L[i][j]
//     - barrier #2: wait for them to finish
//   Then after the loop, set done=true, final barrier to let workers exit
// ------------------------------------------------------------
static void cholesky_parallel() {
    for (int j = 0; j < N; j++) {
        if (j % 10 == 0) {
            fprintf(stderr, "[DEBUG] Main thread: starting column j=%d\n", j);
        }

        // 1) Compute diagonal L[j][j]
        double sum = 0.0;
        for (int k = 0; k < j; k++) {
            sum += L[j][k] * L[j][k];
        }
        L[j][j] = sqrt(A[j][j] - sum);

        // 2) Set current_column and let workers do row-updates
        current_column = j;

        #ifdef DEBUG
        fprintf(stderr, "[DEBUG] Main thread: barrier #1 for column j=%d\n", j);
        #endif
        pthread_barrier_wait(&barrier);

        #ifdef DEBUG
        fprintf(stderr, "[DEBUG] Main thread: barrier #2 for column j=%d\n", j);
        #endif
        pthread_barrier_wait(&barrier);
    }

    // All columns done, signal 'done' and do a final barrier to wake workers
    done = true;
    #ifdef DEBUG
    fprintf(stderr, "[DEBUG] Main thread: done=true, final barrier to let workers exit.\n");
    #endif
    pthread_barrier_wait(&barrier);

    // Wait once more so all threads pass their final barrier
    pthread_barrier_wait(&barrier);

    #ifdef DEBUG
    fprintf(stderr, "[DEBUG] Main thread: final barrier done, returning from cholesky_parallel.\n");
    #endif
}

// ------------------------------------------------------------
// main
// ------------------------------------------------------------
int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <matrix_size> <num_threads>\n", argv[0]);
        return EXIT_FAILURE;
    }

    N = atoi(argv[1]);
    NUM_THREADS = atoi(argv[2]);

    if (N <= 0 || NUM_THREADS <= 0) {
        fprintf(stderr, "Error: N and NUM_THREADS must be positive.\n");
        return EXIT_FAILURE;
    }

    // Allocate matrices
    A = allocate_matrix(N);
    L = allocate_matrix(N);

    // Generate random positive-definite matrix
    generate_positive_definite_matrix(A, N);

    // Initialize L to zero
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            L[i][j] = 0.0;
        }
    }

    // Print a small portion of A
    printf("\nInitial Matrix A (First 5x5):\n");
    print_matrix(A, N);

    // Initialize barrier for (NUM_THREADS + 1) participants
    pthread_barrier_init(&barrier, NULL, NUM_THREADS + 1);

    // Create the worker threads
    pthread_t *threads = (pthread_t *)malloc(NUM_THREADS * sizeof(pthread_t));
    long *thread_ids = (long *)malloc(NUM_THREADS * sizeof(long));

    for (long t = 0; t < NUM_THREADS; t++) {
        thread_ids[t] = t;
        pthread_create(&threads[t], NULL, worker, &thread_ids[t]);
    }

    // Time the parallel Cholesky
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    cholesky_parallel();

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed_time = (end.tv_sec - start.tv_sec) + 
                          (end.tv_nsec - start.tv_nsec) / 1e9;

    // Join threads
    for (int t = 0; t < NUM_THREADS; t++) {
        pthread_join(threads[t], NULL);
    }

    free(threads);
    free(thread_ids);
    pthread_barrier_destroy(&barrier);

    // Print a small portion of L
    printf("\nCholesky Decomposition (L Matrix, First 5x5):\n");
    print_matrix(L, N);

    printf("\nExecution Time: %.6f seconds\n", elapsed_time);

    // Clean up
    free_matrix(A, N);
    free_matrix(L, N);

    return EXIT_SUCCESS;
}
