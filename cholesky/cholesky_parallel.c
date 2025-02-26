#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <time.h>

static double **A = NULL;
static double **L = NULL;
static int N = 0;
static int NUM_THREADS = 0;

// Barrier for synchronizing threads
pthread_barrier_t barrier;

// ------------------------------------------------------------
// Worker function: Each thread runs the same outer loop (over columns j).
//   - "serial" portion for diagonal update (only thread 0 does it)
//   - barrier
//   - "parallel" portion for off-diagonal updates
//   - barrier
// for each column j in [0..N-1].
// ------------------------------------------------------------
void *worker(void *arg)
{
    long tid = (long)arg;

    // Loop over columns
    for (int j = 0; j < N; j++)
    {
        // ---------------------------
        // SERIAL portion:
        // Only thread 0 computes the diagonal element L[j][j].
        // ---------------------------
        if (tid == 0)
        {
            printf("[Serial] Thread %ld computing diagonal for column j=%d...\n", tid, j);
            double sum = 0.0;
            for (int k = 0; k < j; k++) {
                sum += L[j][k] * L[j][k];
            }
            L[j][j] = sqrt(A[j][j] - sum);
            printf("[Serial] Thread %ld finished diagonal for column j=%d: L[%d][%d] = %f\n",
                   tid, j, j, j, L[j][j]);
        }

        // Barrier #1: Ensure the diagonal is computed before off-diagonal updates.
        pthread_barrier_wait(&barrier);

        // ---------------------------
        // PARALLEL portion:
        // All threads update a chunk of rows [j+1..N-1].
        // ---------------------------
        int total = N - (j + 1);
        if (total < 0) total = 0;

        // Basic chunking
        int base = total / NUM_THREADS;
        int rem  = total % NUM_THREADS;

        int start = (j + 1) + tid * base + (tid < rem ? tid : rem);
        int count = base + (tid < rem ? 1 : 0);
        int end   = start + count;

        if (count > 0) {
            printf("[Parallel] Thread %ld updating rows [%d..%d) for column j=%d\n",
                   tid, start, end, j);
        }

        // Off-diagonal updates
        for (int i = start; i < end; i++) {
            double sum = 0.0;
            for (int k = 0; k < j; k++) {
                sum += L[i][k] * L[j][k];
            }
            L[i][j] = (A[i][j] - sum) / L[j][j];
        }

        // Barrier #2: Ensure all threads finish off-diagonal updates before next column.
        pthread_barrier_wait(&barrier);

        if (tid == 0) {
            printf("[Main/Serial] Column j=%d completed. Moving to next column.\n", j);
        }
    }

    return NULL;
}

// ------------------------------------------------------------
// Cholesky function that spawns threads (a single parallel region).
// Each thread runs 'worker', which loops over columns j. 
// ------------------------------------------------------------
void cholesky_pthreads(double **A_in, double **L_in, int N_in, int num_threads)
{
    // Copy inputs to globals so the worker can see them
    A = A_in;
    L = L_in;
    N = N_in;
    NUM_THREADS = num_threads;

    // Initialize barrier for exactly NUM_THREADS
    pthread_barrier_init(&barrier, NULL, NUM_THREADS);

    // Create thread pool
    pthread_t *threads = (pthread_t *)malloc(NUM_THREADS * sizeof(pthread_t));
    for (long t = 0; t < NUM_THREADS; t++) {
        pthread_create(&threads[t], NULL, worker, (void *)t);
    }

    // Join threads
    for (int t = 0; t < NUM_THREADS; t++) {
        pthread_join(threads[t], NULL);
    }

    pthread_barrier_destroy(&barrier);
    free(threads);
}

// ------------------------------------------------------------
// Helpers for matrix allocation, generation, printing
// ------------------------------------------------------------
double **allocate_matrix(int n)
{
    double **m = (double **)malloc(n * sizeof(double *));
    for (int i = 0; i < n; i++) {
        m[i] = (double *)malloc(n * sizeof(double));
    }
    return m;
}

void free_matrix(double **m, int n)
{
    for (int i = 0; i < n; i++) {
        free(m[i]);
    }
    free(m);
}

void generate_positive_definite_matrix(double **matrix, int n)
{
    srand(42);
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            matrix[i][j] = (rand() % 10) + 1; // 1..10
        }
    }
    // A = A * A^T
    double **temp = allocate_matrix(n);
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            double sum = 0.0;
            for (int k = 0; k < n; k++) {
                sum += matrix[i][k] * matrix[j][k];
            }
            temp[i][j] = sum;
        }
    }
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            matrix[i][j] = temp[i][j];
        }
    }
    free_matrix(temp, n);
}

void print_matrix(double **m, int n)
{
    int limit = (n < 5) ? n : 5;
    for (int i = 0; i < limit; i++) {
        for (int j = 0; j < limit; j++) {
            printf("%8.4f ", m[i][j]);
        }
        printf("\n");
    }
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <matrix_size> <num_threads>\n", argv[0]);
        return 1;
    }
    int n = atoi(argv[1]);
    int num_threads = atoi(argv[2]);
    if (n <= 0 || num_threads <= 0) {
        fprintf(stderr, "Invalid input.\n");
        return 1;
    }

    double **A = allocate_matrix(n);
    double **L = allocate_matrix(n);
    generate_positive_definite_matrix(A, n);

    // Initialize L to zero
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            L[i][j] = 0.0;
        }
    }

    printf("Initial Matrix A (top-left 5x5):\n");
    print_matrix(A, n);

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    // Run the Cholesky with pthreads
    cholesky_pthreads(A, L, n, num_threads);

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("\nCholesky Decomposition L (top-left 5x5):\n");
    print_matrix(L, n);
    printf("\nElapsed time: %.6f s\n", elapsed);

    free_matrix(A, n);
    free_matrix(L, n);
    return 0;
}
