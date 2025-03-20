#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>
#include <time.h>
#include <sys/stat.h>   // For mkdir, stat
#include <sys/types.h>  // For mode_t

#define SEED 42

// ----------------------------
// Global File Pointer for Thread Logging
// ----------------------------
static FILE *threads_fp = NULL;

// ----------------------------
// Matrix Helper Functions
// ----------------------------
double **allocate_matrix(int N) {
    double **M = (double **)malloc(N * sizeof(double *));
    for (int i = 0; i < N; i++) {
        M[i] = (double *)malloc(N * sizeof(double));
    }
    return M;
}

void free_matrix(double **M, int N) {
    for (int i = 0; i < N; i++) {
        free(M[i]);
    }
    free(M);
}

void generate_positive_definite_matrix(double **A, int N) {
    srand(SEED);
    // Fill with random values in [1..10]
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
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            A[i][j] = temp[i][j];
        }
    }
    free_matrix(temp, N);
}

void print_matrix(double **M, int N, FILE *fp) {
    int limit = (N < 5) ? N : 5;
    for (int i = 0; i < limit; i++) {
        for (int j = 0; j < limit; j++) {
            fprintf(fp, "%8.4f ", M[i][j]);
        }
        fprintf(fp, "\n");
    }
}

void print_matrix_stdout(double **M, int N) {
    print_matrix(M, N, stdout);
}

// ----------------------------
// OpenMP Cholesky Decomposition (Barrier Version)
// ----------------------------
void cholesky_openmp_barrier(double **A, double **L, int N)
{
    #pragma omp parallel
    {
        for (int j = 0; j < N; j++) {
            // SERIAL portion
            #pragma omp single
            {
                #pragma omp critical
                {
                    fprintf(threads_fp, "[Barrier Mode] [Serial] Diagonal col %d by thread %d...\n",
                            j, omp_get_thread_num());
                }
                double sum = 0.0;
                for (int k = 0; k < j; k++) {
                    sum += L[j][k] * L[j][k];
                }
                L[j][j] = sqrt(A[j][j] - sum);

                #pragma omp critical
                {
                    fprintf(threads_fp, "[Barrier Mode] [Serial] col %d: L[%d][%d] = %f computed by thread %d.\n",
                            j, j, j, L[j][j], omp_get_thread_num());
                }
            }
            // Implicit barrier after single

            // Off-diagonal updates in parallel
            #pragma omp for schedule(static)
            for (int i = j + 1; i < N; i++) {
                double sum = 0.0;
                for (int k = 0; k < j; k++) {
                    sum += L[i][k] * L[j][k];
                }
                L[i][j] = (A[i][j] - sum) / L[j][j];
                #pragma omp critical
                {
                    fprintf(threads_fp, "[Barrier Mode] Thread %d processed L[%d][%d].\n",
                            omp_get_thread_num(), i, j);
                }
            }
            #pragma omp barrier
            #pragma omp single
            {
                fprintf(threads_fp, "[Barrier Mode] Column %d complete.\n", j);
            }
        }
    }
}

// ----------------------------
// OpenMP Cholesky Decomposition (Nowait Version)
// ----------------------------
void cholesky_openmp_nowait(double **A, double **L, int N)
{
    #pragma omp parallel
    {
        for (int j = 0; j < N; j++) {
            // SERIAL portion with nowait
            #pragma omp single nowait
            {
                #pragma omp critical
                {
                    fprintf(threads_fp, "[Nowait Mode] [Serial] Diagonal col %d by thread %d...\n",
                            j, omp_get_thread_num());
                }
                double sum = 0.0;
                for (int k = 0; k < j; k++) {
                    sum += L[j][k] * L[j][k];
                }
                L[j][j] = sqrt(A[j][j] - sum);

                #pragma omp critical
                {
                    fprintf(threads_fp, "[Nowait Mode] [Serial] col %d: L[%d][%d] = %f computed by thread %d.\n",
                            j, j, j, L[j][j], omp_get_thread_num());
                }
            }
            // Off-diagonal updates with nowait
            #pragma omp for schedule(static) nowait
            for (int i = j + 1; i < N; i++) {
                double sum = 0.0;
                for (int k = 0; k < j; k++) {
                    sum += L[i][k] * L[j][k];
                }
                L[i][j] = (A[i][j] - sum) / L[j][j];
                #pragma omp critical
                {
                    fprintf(threads_fp, "[Nowait Mode] Thread %d processed L[%d][%d].\n",
                            omp_get_thread_num(), i, j);
                }
            }
            // Final barrier for this iteration
            #pragma omp barrier
            #pragma omp single
            {
                fprintf(threads_fp, "[Nowait Mode] Column %d complete.\n", j);
            }
        }
    }
}

// ----------------------------
// main()
// Usage: ./cholesky_openmp_mod <matrix_size> <num_threads> <mode>
//  - matrix_size: dimension of the matrix (N x N)
//  - num_threads: number of OpenMP threads
//  - mode: 0 = nowait version, 1 = barrier version
// Also prints OMP_WAIT_POLICY if set.
// Writes two files in "openmp_parallel_tests/<matrix_size>/":
//   1) [matrix_size]_1.txt => results
//   2) [matrix_size]_1_threads.txt => thread logs
// (Here test_number is hard-coded to 1, but you could parse a 4th argument if you prefer.)
// ----------------------------
int main(int argc, char *argv[])
{
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <matrix_size> <num_threads> <mode> <test_number>\n", argv[0]);
        fprintf(stderr, "  mode: 0 = nowait version, 1 = barrier version\n");
	fprintf(stderr, " <test_number>: e.g., 1, 2, 3, etc.\n");
        return 1;
    }
    int N = atoi(argv[1]);
    int num_threads = atoi(argv[2]);
    int mode = atoi(argv[3]);
    int test_number = atoi(argv[4]);
    if (N <= 0 || num_threads <= 0 || (mode != 0 && mode != 1) || test_number <= 0) {
        fprintf(stderr, "Invalid input.\n");
        return 1;
    }

    omp_set_num_threads(num_threads);

    // Print OMP_WAIT_POLICY if set
    char *wait_policy = getenv("OMP_WAIT_POLICY");
    if (wait_policy) {
        printf("OMP_WAIT_POLICY is set to: %s\n", wait_policy);
    } else {
        printf("OMP_WAIT_POLICY is not set; default behavior applies.\n");
    }

    // Create matrices
    double **A = allocate_matrix(N);
    double **L = allocate_matrix(N);
    generate_positive_definite_matrix(A, N);

    // Initialize L
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            L[i][j] = 0.0;
        }
    }

    printf("Initial Matrix A (top-left 5x5):\n");
    print_matrix_stdout(A, N);

    // Prepare directory structure
    struct stat st = {0};
    if (stat("openmp_parallel_tests", &st) == -1) {
        #ifdef _WIN32
            mkdir("openmp_parallel_tests");
        #else
            mkdir("openmp_parallel_tests", 0700);
        #endif
    }
    char subdir[256];
    snprintf(subdir, sizeof(subdir), "openmp_parallel_tests/%d", N);
    if (stat(subdir, &st) == -1) {
        #ifdef _WIN32
            mkdir(subdir);
        #else
            mkdir(subdir, 0700);
        #endif
    }

    char result_filename[256];
    char threads_filename[256];
    snprintf(result_filename, sizeof(result_filename), "%s/%d_%d.txt", subdir, N, test_number);
    snprintf(threads_filename, sizeof(threads_filename), "%s/%d_%d_threads.txt", subdir, N, test_number);

    // Open thread log file
    threads_fp = fopen(threads_filename, "w");
    if (!threads_fp) {
        perror("Error opening thread log file");
        return 1;
    }

    double start = omp_get_wtime();

    if (mode == 1) {
        printf("Running OpenMP barrier version...\n");
        cholesky_openmp_barrier(A, L, N);
    } else {
        printf("Running OpenMP nowait version...\n");
        cholesky_openmp_nowait(A, L, N);
    }

    double end = omp_get_wtime();
    double elapsed = end - start;

    printf("\nCholesky Decomposition (L Matrix, top-left 5x5):\n");
    print_matrix_stdout(L, N);
    printf("\nExecution Time: %.6f seconds\n", elapsed);

    // Write results to file
    FILE *result_fp = fopen(result_filename, "w");
    if (!result_fp) {
        perror("Error opening result file");
        return 1;
    }
    fprintf(result_fp, "Matrix Size: %d\nTest Number: %d\n\n", N, test_number);
    fprintf(result_fp, "Initial Matrix A (top-left 5x5):\n");
    print_matrix(A, N, result_fp);
    fprintf(result_fp, "\nCholesky Decomposition (L Matrix, top-left 5x5):\n");
    print_matrix(L, N, result_fp);
    fprintf(result_fp, "\nExecution Time: %.6f seconds\n", elapsed);
    fclose(result_fp);
    fclose(threads_fp);

    printf("\nResults written to %s\n", result_filename);
    printf("Thread activity log written to %s\n", threads_filename);

    free_matrix(A, N);
    free_matrix(L, N);
    return 0;
}

