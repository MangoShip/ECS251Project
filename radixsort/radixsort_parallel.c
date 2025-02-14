#include <math.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

/*
 * radixsort_parallel.c: Radix Sort Implementation (Parallel Version)
*/

//-----------------------------------------------------
// MACROS
#define MIN(a,b) ((a) < (b) ? (a) : (b))
//-----------------------------------------------------
// Helper Functions
// Calculate the difference between two timepsecs in nanoseconds
double difftimespec_ns(const struct timespec after, const struct timespec before)
{
  return ((double)after.tv_sec - (double)before.tv_sec) * (double)1000000000
        + ((double)after.tv_nsec - (double)before.tv_nsec);
}
// Print list
void print_list(int *A, int N) {
  for (int i = 0; i < N; i++) {
    printf("%d ", A[i]);
  }
  printf("\n");
}
// Shuffle list
// Source: https://stackoverflow.com/questions/44187061/how-to-shuffle-an-array-in-c
void shuffle_list(int *A, int N) {
  for (int i = 0; i < N - 1; i++) {
    size_t j = i + rand() / (RAND_MAX / (N - i) + 1);
    int t = A[j];
    A[j] = A[i];
    A[i] = t;
  }
}
int get_max_bit(int num) {
  int bit_pos = 0;
  while (num != 0) {
    bit_pos++;
    num = num >> 1;
  }
  return bit_pos;
}
int get_kth_bit(int num, int k) {
  return ((num & ( 1 << k )) >> k);
}
//-----------------------------------------------------
// PThread argument
struct hist_arg {
  int start_index;
  int end_index;
  int k;

  int *A;
  int *local_hist;
};
struct new_index_arg {
  int thread_id;
  int start_index;
  int end_index;
  int k;
  int total_0bits;

  int *A;
  int *hist;
  int *new_indexes;
};
//-----------------------------------------------------
// PThread Tasks
// Collect number of 0 and 1 bits in local section
void *build_local_hist(void *arg) {
  struct hist_arg *thr_arg = (struct hist_arg *) arg;

  thr_arg->local_hist[0] = 0;
  thr_arg->local_hist[1] = 0;

  for (int i = thr_arg->start_index; i < thr_arg->end_index; i++) {
    int bit = get_kth_bit(thr_arg->A[i], thr_arg->k); 

    // Build a histogram of 0 or 1 bit
    thr_arg->local_hist[bit]++;
  }
  return 0;
}
// Compute new index for each element
void *compute_new_indexes(void *arg) {
  struct new_index_arg *thr_arg = (struct new_index_arg *) arg;
  int offset_0bit = 0;
  int offset_1bit = 0;

  for (int i = 0; i < thr_arg->thread_id; i++) {
    offset_0bit += thr_arg->hist[i * 2];
    offset_1bit += thr_arg->hist[(i * 2) + 1];
  }

  //printf("Thread #%d: offset_0bit: %d, offset_1bit: %d\n", thr_arg->thread_id, offset_0bit, offset_1bit);

  for (int i = thr_arg->start_index; i < thr_arg->end_index; i++) {
    if (get_kth_bit(thr_arg->A[i], thr_arg->k)) { // bit = 1
      thr_arg->new_indexes[i] = thr_arg->total_0bits + offset_1bit++;
    }
    else {
      thr_arg->new_indexes[i] = offset_0bit++;
    }
  }
}
//-----------------------------------------------------
// Main Function
int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("Usage: ./thread N NUM_THREADS\n");
    exit(1);
  }

  int N;
  sscanf(argv[1], "%d", &N);

  int num_threads;
  sscanf(argv[2], "%d", &num_threads);

  int seed = 0;
  srandom(seed); // Pseudo-random generator

  // Initialize empty thread array
  pthread_t thread_array[num_threads];

  // Allocate memory for thread argument (building histogram)
  struct hist_arg *hist_args = (struct hist_arg *)malloc(sizeof(struct hist_arg) * num_threads);
  struct new_index_arg *new_index_args = (struct new_index_arg *)malloc(sizeof(struct new_index_arg) * num_threads);

  struct timespec start_time, end_time;

  struct timespec timer1_start, timer2_start, timer3_start, timer4_start;
  struct timespec timer1_end, timer2_end, timer3_end, timer4_end;

  double time1 = 0;
  double time2 = 0;
  double time3 = 0;
  double time4 = 0;

  // List that will be used for sorting
  int *A = malloc(N * sizeof(int));

  // Initialize A
  for (int i = 0; i < N; i++) {
    A[i] = i;
  }
  shuffle_list(A, N);
  //print_list(A, N);

  // Get the highest bit position based on the maximum number
  int max_k = get_max_bit(N - 1);
  printf("Number of iterations: %d\n", max_k);
  
  // Main loop for radix sort
  int *new_indexes = malloc(N * sizeof(int));
  int *new_A = malloc(N * sizeof(int));
  int hist[2 * num_threads];
  int local_N = floor((N + (num_threads - 1)) / num_threads);
  clock_gettime(CLOCK_MONOTONIC, &start_time);
  for (int k = 0; k < max_k; k++) {

    clock_gettime(CLOCK_MONOTONIC, &timer1_start);
    for (int thr_id = 0; thr_id < num_threads; thr_id++) {
      int start_index = local_N * thr_id;
      struct hist_arg arg = {start_index, MIN(start_index + local_N, N), k, A, &hist[2 * thr_id]};
      hist_args[thr_id] = arg;
      pthread_create(&thread_array[thr_id], NULL, build_local_hist, (void *)&hist_args[thr_id]);
    }

    for (int thr_id = 0; thr_id < num_threads; thr_id++) {
      pthread_join(thread_array[thr_id], NULL);
    }
    clock_gettime(CLOCK_MONOTONIC, &timer1_end);
    time1 += difftimespec_ns(timer1_end, timer1_start); 

    clock_gettime(CLOCK_MONOTONIC, &timer2_start);

    // Get total number of 0 bits
    int total_0bits = 0;
    for (int thr_id = 0; thr_id < num_threads; thr_id++) {
      total_0bits += hist[2 * thr_id];
    }
    clock_gettime(CLOCK_MONOTONIC, &timer2_end);
    time2 += difftimespec_ns(timer2_end, timer2_start); 

    clock_gettime(CLOCK_MONOTONIC, &timer3_start);

    for (int thr_id = 0; thr_id < num_threads; thr_id++) {
      int start_index = local_N * thr_id;
      struct new_index_arg arg = {thr_id, start_index, MIN(start_index + local_N, N), k, total_0bits, A, hist, new_indexes};
      new_index_args[thr_id] = arg;
      pthread_create(&thread_array[thr_id], NULL, compute_new_indexes, (void *)&new_index_args[thr_id]);
    }

    for (int thr_id = 0; thr_id < num_threads; thr_id++) {
      pthread_join(thread_array[thr_id], NULL);
    }
    clock_gettime(CLOCK_MONOTONIC, &timer3_end);
    time3 += difftimespec_ns(timer3_end, timer3_start); 

    clock_gettime(CLOCK_MONOTONIC, &timer4_start);

    // Rewrite a list with new index
    for (int i = 0; i < N; i++) {
      new_A[new_indexes[i]] = A[i];
    }
    for (int i = 0; i < N; i++) {
      A[i] = new_A[i];
    }
    clock_gettime(CLOCK_MONOTONIC, &timer4_end);
    time4 += difftimespec_ns(timer4_end, timer4_start); 

    /*printf("k: %d\n", k);
    printf("Offsets: ");
    print_list(offsets, N);
    printf("New Indexes: ");
    print_list(new_indexes, N);
    printf("New A: ");
    print_list(A, N);
    printf("\n");*/
  }
  clock_gettime(CLOCK_MONOTONIC, &end_time);

  // Check if array has been sorted correctly
  for (int i = 0; i < N; i++) {
    if (i != A[i]) {
      printf("Incorrectly sorted! A[%d] = %d\n", i, A[i]);
      exit(1);
    }
  }

  printf("PASSED\n");
  printf("Execution Time: %f s\n", difftimespec_ns(end_time, start_time) / 1e9);

  printf("Time1: %f s\n", time1 / 1e9);
  printf("Time2: %f s\n", time2 / 1e9);
  printf("Time3: %f s\n", time3 / 1e9);
  printf("Time4: %f s\n", time4 / 1e9);

  free(hist_args);
  free(new_index_args);
  free(A);
  free(new_indexes);
  free(new_A);
  return 0;
}