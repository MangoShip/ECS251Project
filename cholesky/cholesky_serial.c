#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <sys/stat.h>   // for mkdir, stat
#include <sys/types.h>  // for mode_t

#define SEED 42

// -----------------------------------------------------------------------------
// Allocate a 2D matrix of size N x N
// -----------------------------------------------------------------------------
double **allocate_matrix(int N) {
    double **matrix = (double **)malloc(N * sizeof(double *));
    for (int i = 0; i < N; i++) {
        matrix[i] = (double *)malloc(N * sizeof(double));
    }
    return matrix;
}

// -----------------------------------------------------------------------------
// Free a 2D matrix
// -----------------------------------------------------------------------------
void free_matrix(double **matrix, int N) {
    for (int i = 0; i < N; i++) {
        free(matrix[i]);
    }
    free(matrix);
}

// -----------------------------------------------------------------------------
// Generate a random positive-definite matrix of size N x N.
// We do: A <- A * A^T to ensure it is positive-definite.
// -----------------------------------------------------------------------------
void generate_positive_definite_matrix(double **A, int N) {
    srand(SEED);

    // Fill with random values [1..10]
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            A[i][j] = rand() % 10 + 1;
        }
    }

    // temp = A * A^T
    double **temp = allocate_matrix(N);
    for (int i = 0; i < N; i++){
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

// -----------------------------------------------------------------------------
// Print the top-left corner of a matrix (up to 5x5 for readability)
// -----------------------------------------------------------------------------
void print_matrix(double **matrix, int N) {
    int limit = (N < 5) ? N : 5;
    for (int i = 0; i < limit; i++) {
        for (int j = 0; j < limit; j++) {
            printf("%8.4f ", matrix[i][j]);
        }
        printf("\n");
    }
}

// -----------------------------------------------------------------------------
// Perform the Cholesky Decomposition in a single thread.
// For each column j:
//   1) Compute diagonal element L[j][j] = sqrt(A[j][j] - sum_of_squares)
//   2) Compute L[i][j] for i>j
// -----------------------------------------------------------------------------
void cholesky_serial(double **A, double **L, int N) {
    for (int j = 0; j < N; j++) {
        // 1) Compute diagonal element
        double sum = 0.0;
        for (int k = 0; k < j; k++) {
            sum += L[j][k] * L[j][k];
        }
        L[j][j] = sqrt(A[j][j] - sum);

        // 2) Compute L[i][j] for i>j
        for (int i = j + 1; i < N; i++) {
            sum = 0.0;
            for (int k = 0; k < j; k++) {
                sum += L[i][k] * L[j][k];
            }
            L[i][j] = (A[i][j] - sum) / L[j][j];
        }
    }
}

// -----------------------------------------------------------------------------
// main()
//   Usage: ./cholesky_serial <matrix_size> [<test_number>]
// If test_number is not provided, it defaults to 1.
// This version prints the results to stdout and also writes the initial matrix,
// the L matrix (top-left 5x5) and execution time to a file in the "serial_tests" directory.
// The file name format is: [matrix_size]_[test_number].txt
// -----------------------------------------------------------------------------
int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Usage: %s <matrix_size> [<test_number>]\n", argv[0]);
        return EXIT_FAILURE;
    }

    int N = atoi(argv[1]);
    if (N <= 0) {
        fprintf(stderr, "Matrix size must be a positive integer.\n");
        return EXIT_FAILURE;
    }

    // If test number is provided, use it; otherwise default to 1.
    int test_number = 1;
    if (argc == 3) {
        test_number = atoi(argv[2]);
    }

    // Allocate matrices
    double **A = allocate_matrix(N);
    double **L = allocate_matrix(N);

    // Generate random positive-definite matrix
    generate_positive_definite_matrix(A, N);

    // Initialize L to zero
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            L[i][j] = 0.0;
        }
    }

    // Print initial matrix (first 5x5)
    printf("\nInitial Matrix A (First 5x5):\n");
    print_matrix(A, N);

    // Time the Cholesky decomposition
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    cholesky_serial(A, L, N);

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed_time = (end.tv_sec - start.tv_sec)
                        + (end.tv_nsec - start.tv_nsec) / 1e9;

    // Print the L matrix (first 5x5)
    printf("\nCholesky Decomposition (L Matrix, First 5x5):\n");
    print_matrix(L, N);

    // Print execution time
    printf("\nExecution Time: %.6f seconds\n", elapsed_time);

    // -----------------------------
    // File output section:
    // - Create the directory "serial_tests" if it doesn't exist.
    // - Construct the file name as "[matrix_size]_[test_number].txt".
    // - Write the initial matrix (top-left 5x5), L matrix (top-left 5x5), and execution time to the file.
    // -----------------------------
    struct stat st = {0};
    if (stat("serial_tests", &st) == -1) {
        #ifdef _WIN32
            mkdir("serial_tests");
        #else
            mkdir("serial_tests", 0700);
        #endif
    }
    char subdir[256];
    snprintf(subdir, sizeof(subdir), "serial_tests/%d", N);
    if (stat(subdir, &st) == -1) {
        #ifdef _WIN32
            mkdir(subdir);
        #else 
            mkdir(subdir, 0700);
        #endif
    }

    char filename[256];
    snprintf(filename, sizeof(filename), "%s/%d_%d.txt", subdir, N, test_number);
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        perror("Error opening file for writing");
        return EXIT_FAILURE;
    }
    fprintf(fp, "Matrix Size: %d\nTest Number: %d\n\n", N, test_number);
    fprintf(fp, "Initial Matrix A (top-left 5x5):\n");
    int limit = (N < 5) ? N : 5;
    for (int i = 0; i < limit; i++) {
        for (int j = 0; j < limit; j++) {
            fprintf(fp, "%8.4f ", A[i][j]);
        }
        fprintf(fp, "\n");
    }
    fprintf(fp, "\nCholesky Decomposition (L Matrix, top-left 5x5):\n");
    for (int i = 0; i < limit; i++) {
        for (int j = 0; j < limit; j++) {
            fprintf(fp, "%8.4f ", L[i][j]);
        }
        fprintf(fp, "\n");
    }
    fprintf(fp, "\nExecution Time: %.6f seconds\n", elapsed_time);
    fclose(fp);
    printf("\nResults written to %s\n", filename);

    // Free resources
    free_matrix(A, N);
    free_matrix(L, N);

    return EXIT_SUCCESS;
}