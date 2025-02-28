#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

/*
 * radixsort_seq.c: Radix Sort Implementation (Sequential Version)
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
  if (argc != 2) {
    printf("Usage: ./radixsort_seq N\n");
    exit(1);
  }

  int N;
  sscanf(argv[1], "%d", &N);

  int seed = 0;
  srandom(seed); // Pseudo-random generator

  struct timespec start_time, end_time;

#ifdef TIMER
  struct timespec timer1_start, timer2_start, timer3_start;
  struct timespec timer1_end, timer2_end, timer3_end;

  double time1 = 0;
  double time2 = 0;
  double time3 = 0;
#endif

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
  int *offsets = malloc(N * sizeof(int));
  int *new_indexes = malloc(N * sizeof(int));
  int *new_A = malloc(N * sizeof(int));
  int hist[2];

  clock_gettime(CLOCK_MONOTONIC, &start_time);
  for (int k = 0; k < max_k; k++) {
    hist[0] = 0;
    hist[1] = 0;

#ifdef TIMER
    clock_gettime(CLOCK_MONOTONIC, &timer1_start);
#endif

    for (int i = 0; i < N; i++) {
      // Get k-th bit for each element
      int bit = get_kth_bit(A[i], k);

      // Collect a relative offsets
      offsets[i] = hist[bit];

      // Build a histogram of 0 or 1 bit
      hist[bit]++;
    }

#ifdef TIMER
    clock_gettime(CLOCK_MONOTONIC, &timer1_end);
    time1 += difftimespec_ns(timer1_end, timer1_start); 

    clock_gettime(CLOCK_MONOTONIC, &timer2_start);
#endif

    // Get a new index for each number
    for (int i = 0; i < N; i++) {
      if ((A[i] & ( 1 << k )) == 0) {
        new_indexes[i] = offsets[i];
      }
      else { // k-bit == 1
        new_indexes[i] = hist[0] + offsets[i];
      }
    }

#ifdef TIMER
    clock_gettime(CLOCK_MONOTONIC, &timer2_end);
    time2 += difftimespec_ns(timer2_end, timer2_start); 

    clock_gettime(CLOCK_MONOTONIC, &timer3_start);
#endif

    // Rewrite a list with new index
    for (int i = 0; i < N; i++) {
      new_A[new_indexes[i]] = A[i];
    }
    // Re-route pointer A
    A = new_A;
    new_A = save_A;
    save_A = A;

#ifdef TIMER
    clock_gettime(CLOCK_MONOTONIC, &timer3_end);
    time3 += difftimespec_ns(timer3_end, timer3_start); 
#endif

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

#ifdef TIMER
  printf("Time1: %f s\n", time1 / 1e9);
  printf("Time2: %f s\n", time2 / 1e9);
  printf("Time3: %f s\n", time3 / 1e9);
#endif

  free(A);
  free(offsets);
  free(new_indexes);
  free(new_A);
  return 0;
}