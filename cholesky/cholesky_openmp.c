#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>

#define SEED 42

// Allocate a 2D Matrix
double **allocate_matrix(int N) {
    double **M = (double **)malloc(N * sizeof(double *));
    for (int i = 0; i < N; i++) {
        M[i] = (double *)malloc(N * sizeof(double));
    }
    return M;
}

// Free a 2D Matrix
void free_matrix(double **M, int N) {
    for (int i = 0; i < N; i++) {
        free(M[i]);
    }
    free(M);
}

// Generate a random positive-definite Matrix
void generate_positive_definite_matrix(double **A, int N) {
    srand(SEED);
    // Fill with random values [1..10]
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            A[i][j] = rand() % 10 + 1;
        }
    }
    // A = A * A^T
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
    // Copy temp back into A
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            A[i][j] = temp[i][j];
        }
    }
    free_matrix(temp, N);
}

// Print top-left corner of the matrix
void print_matrix(double **M, int N) {
    int limit = (N < 5) ? N : 5;
    for (int i = 0; i < limit; i++) {
        for (int j = 0; j < limit; j++) {
            printf("%8.4f ", M[i][j]);
        }
        printf("\n");
    }
}

// OpenMP-based Cholesky
void cholesky_openmp(double **A, double **L, int N) {
    // A single parallel region encloses the entire decomposition
    // The threads are created (or reused) once here and persist for the entire loop region.
    #pragma omp parallel
    {
        // Each thread has its own private i, j, etc.
        for (int j = 0; j < N; j++) {
            // One thread computes diagonal element
            #pragma omp single
            {
                double sum = 0.0;
                for (int k = 0; k < j; k++) {
                    sum += L[j][k] * L[j][k];
                }
                L[j][j] = sqrt(A[j][j] - sum);
            }

            #pragma omp barrier

            // Off-diagonal updates in parallel

            #pragma omp for
            for (int i = j + 1; i < N; i++) {
                double sum = 0.0;
                for (int k = 0; k < j; k++) {
                    sum += L[i][k] * L[j][k];
                }
                L[i][j] = (A[i][j] - sum) / L[j][j];
            }

            #pragma omp barrier
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <matrix_size> <num_threads>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int N = atoi(argv[1]);
    int num_threads = atoi(argv[2]);
    if (N <= 0 || num_threads <= 0) {
        fprintf(stderr, "Matrix size and number of threads must be positive.\n");
        return EXIT_FAILURE;
    }

    // Set the number of threads for OpenMP
    omp_set_num_threads(num_threads);

    // Allocate and Initialize Matrices
    double **A = allocate_matrix(N);
    double **L = allocate_matrix(N);
    generate_positive_definite_matrix(A, N);

    // Initialize L to zero
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            L[i][j] = 0.0;
        }
    }

    printf("\nInitial Matrix A (First 5x5):\n");
    print_matrix(A, N);

    double start = omp_get_wtime();
    cholesky_openmp(A, L, N);
    double end = omp_get_wtime();

    printf("\nCholesky Decomposition (L Matrix, First 5x5):\n");
    print_matrix(L, N);

    printf("\nExecution Time: %.6f seconds\n", end - start);

    free_matrix(A, N);
    free_matrix(L, N);
    return EXIT_SUCCESS;
}