#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <omp.h>

/*
 * radixsort_openmp.c: Radix Sort Implementation (OpenMP Version)
*/

//-----------------------------------------------------
// Calculate the difference between two timepsecs in nanoseconds
double difftimespec_ns(const struct timespec after, const struct timespec before)
{
    return ((double)after.tv_sec - (double)before.tv_sec) * (double)1000000000
         + ((double)after.tv_nsec - (double)before.tv_nsec);
}
// Helper Functions
void print_list(int *A, int N) {
  for (int i = 0; i < N; i++) {
    printf("%d ", A[i]);
  }
  printf("\n");
}
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
// Main Function
int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("Usage: ./radixsort_openmp N NUM_THREADS\n");
    exit(1);
  }

  int N;
  sscanf(argv[1], "%d", &N);

  int num_threads;
  sscanf(argv[2], "%d", &num_threads);

  omp_set_num_threads(num_threads);

  int seed = 0;
  srandom(seed); // Pseudo-random generator

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

  // Get the highest bit position based on the maximum number
  int max_k = get_max_bit(N - 1);
  printf("Number of iterations: %d\n", max_k);
  
  // Main loop for radix sort
  int *save_A = A;
  int *new_indexes = malloc(N * sizeof(int));
  int *new_A = malloc(N * sizeof(int));
  int hist[2 * num_threads];
  int offsets[2 * num_threads];
  int local_N = floor((N + (num_threads - 1)) / num_threads);

  clock_gettime(CLOCK_MONOTONIC, &start_time);
  for (int k = 0; k < max_k; k++) {
    int total_0bits = 0;

    // Initialize hist and offsets to 0
    for (int i = 0; i < (2 * num_threads); i++){
      hist[i] = 0;
      offsets[i] = 0;
    }

    clock_gettime(CLOCK_MONOTONIC, &timer1_start);
#pragma omp parallel for schedule(static,local_N) reduction(+:total_0bits)
    for (int i = 0; i < N; i++) {
      int tid = omp_get_thread_num();

      // Get k-th bit for each element
      int bit = get_kth_bit(A[i], k);

      // Build a histogram of 0 or 1 bit
      hist[(tid * 2) + bit]++;

      if (bit == 0) total_0bits++;
    }
    clock_gettime(CLOCK_MONOTONIC, &timer1_end);
    time1 += difftimespec_ns(timer1_end, timer1_start); 

    clock_gettime(CLOCK_MONOTONIC, &timer2_start);

#pragma omp parallel for schedule(static, 1)
    for (int chunk_id = 0; chunk_id < num_threads; chunk_id++) {
      for (int i = 0; i < omp_get_thread_num(); i++) {
        offsets[(chunk_id * 2)] += hist[(i * 2)];
        offsets[(chunk_id * 2) + 1] += hist[(i * 2) + 1];
      }
    }
    clock_gettime(CLOCK_MONOTONIC, &timer2_end);
    time2 += difftimespec_ns(timer2_end, timer2_start); 


    int offset_0bit = 0;
    int offset_1bit = 0;
    clock_gettime(CLOCK_MONOTONIC, &timer3_start);
    // Get a new index for each number
#pragma omp parallel for schedule(static,local_N) firstprivate(offset_0bit, offset_1bit)
    for (int i = 0; i < N; i++) {
      if ((A[i] & ( 1 << k )) == 0) {
        new_indexes[i] = offsets[(omp_get_thread_num() * 2)] + offset_0bit++;
      }
      else { // k-bit == 1
        new_indexes[i] = total_0bits + offsets[(omp_get_thread_num() * 2) + 1] + offset_1bit++;
      }
    }
    clock_gettime(CLOCK_MONOTONIC, &timer3_end);
    time3 += difftimespec_ns(timer3_end, timer3_start); 

    clock_gettime(CLOCK_MONOTONIC, &timer4_start);
    // Rewrite a list with new index
#pragma omp parallel for
    for (int i = 0; i < N; i++) {
      new_A[new_indexes[i]] = A[i];
    }

    // Re-route pointer A
    A = new_A;
    new_A = save_A;
    save_A = A;

    clock_gettime(CLOCK_MONOTONIC, &timer4_end);
    time4 += difftimespec_ns(timer4_end, timer4_start); 

    /*printf("k: %d\n", k);
    printf("hist: ");
    print_list(hist, 2 * num_threads);
    printf("offsets: ");
    print_list(offsets, 2 * num_threads);
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

  free(A);
  free(new_indexes);
  free(new_A);
  return 0;
}