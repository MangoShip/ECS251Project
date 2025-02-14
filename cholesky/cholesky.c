#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <time.h>

#define SEED 42

double **A, **L;
int N;
int NUM_THREADS;
pthread_barrier_t barrier;

typedef struct {
    int start;
    int end;
    int column;
} ThreadData;

double **allocate_matrix(int size) {
    double **matrix = (double **)malloc(size * sizeof(double *));
    for (int i = 0; i < size; ++i) {
        matrix[i] = (double *)malloc(size * sizeof(double));
    }
    return matrix;
}

void free_matrix(double **matrix, int size) {
    for (int i = 0; i < size; ++i) {
        free(matrix[i]);
    }
    free(matrix);
}

void generate_positive_definite_matrix(double **matrix, int size) {
    srand(SEED);
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            matrix[i][j] = rand() % 10 + 1;
        }
    }

    double **temp = allocate_matrix(size);
    for (int i = 0; i < size; ++i){
        for (int j = 0; j < size; ++j) {
            temp[i][j] = 0.0;
            for (int k = 0; k < size; ++k) {
                temp[i][j] += matrix[i][k] * matrix[j][k];
            }
        }
    }

    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            matrix[i][j] = temp[i][j];
        }
    }

    free_matrix(temp, size);
}

void print_matrix(double **matrix, int size) {
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            printf("%8.4f ", matrix[i][j]);
        }
        printf("\n");
    }
}

void *cholesky_worker(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    int start = data->start;
    int end = data->end;
    int j = data->column;

    for (int i = start; i < end; ++i) {
        if (i < j) {
            double sum = 0.0;
            for (int k = 0; k < j; ++k) {
                sum += L[i][k] * L[j][k];
            }
            L[i][j] = (A[i][j] - sum) / L[j][j];
        }
    }

    pthread_barrier_wait(&barrier);
    return NULL;
}

void cholesky_parallel() {
    for (int j = 0; j < N; ++j) {
        double sum = 0.0;
        for (int k = 0; k < j; ++k) {
            sum += L[j][k] * L[j][k];
        }
        L[j][j] = sqrt(A[j][j] - sum);

        pthread_t threads[NUM_THREADS];
        ThreadData thread_data[NUM_THREADS];

        int rows_per_thread = (N - j - 1) / NUM_THREADS;
        if (rows_per_thread == 0) {
            rows_per_thread = 1;
        }

        for (int t = 0; t < NUM_THREADS; ++t) {
            thread_data[t].start = j + 1 + t * rows_per_thread;
            thread_data[t].end = j + 1 + (t + 1) * rows_per_thread;
            if (thread_data[t].end > N) {
                thread_data[t].end = N;
            }
            thread_data[t].column = j;

            pthread_create(&threads[t], NULL, cholesky_worker, (void *)&thread_data[t]);
        }

        for (int t = 0; t < NUM_THREADS; ++t) {
            pthread_join(threads[t], NULL);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <matrix_size> <num_threads>\n", argv[0]);
        return EXIT_FAILURE;
    }

    N = atoi(argv[1]);
    NUM_THREADS = atoi(argv[2]);

    if (N <= 0 || NUM_THREADS <= 0) {
        fprintf(stderr, "Matrix size and number of threads must be positive integeres.\n");
        return EXIT_FAILURE;
    }

    A = allocate_matrix(N);
    L = allocate_matrix(N);

    generate_positive_definite_matrix(A, N);

    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            L[i][j] = 0.0;
        }
    }

    pthread_barrier_init(&barrier, NULL, NUM_THREADS);

    printf("\nInput Matrix A (First 5x5):\n");
    print_matrix(A, (N < 5) ? N : 5);

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    cholesky_parallel();

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("\nCholesky Decomposition (L Matrix, First 5x5):\n");
    print_matrix(L, (N < 5) ? N : 5);

    printf("\nExecution Time: %.6f seconds\n", elapsed_time);

    pthread_barrier_destroy(&barrier);
    free_matrix(A, N);
    free_matrix(L, N);

    return EXIT_SUCCESS;
}