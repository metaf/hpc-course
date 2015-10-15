/**
 * University of Pittsburgh
 * Department of Computer Science
 * CS1645: Introduction to HPC Systems
 * Instructor Bryan Mills, PhD
 * Student: 
 * Implement Pthreads version of trapezoidal approximation.
 */

#include <stdio.h>
#include "timer.h"
#include <pthread.h>

#define NUM_THREADS 20

// Global variables to make coverting to pthreads easier :)
double a;
double b;
int n;
double approx;
pthread_mutex_t mutex;

// Actual areas under the f(x) = x^2 curves, for you to check your
// values against.
double static NEG_1_TO_POS_1 = 0.66666666666667;
double static ZERO_TO_POS_10 = 333.333;

// f function is defined a x^2
double f(double a) {
  return a * a;
}

void* trap_loop(void *rank) {
  int *rank_int_ptr = (int*) rank;
  int my_rank = *rank_int_ptr;
  double x_i;
  double h = (b-a) / n;
  int step = n / NUM_THREADS;
  int start = step * my_rank;
  int end = step * (my_rank + 1) -1;

  double local_approx = 0;
  for(int i = start; i < end; i++) {
    x_i = a + i*h;
    local_approx = local_approx + f(x_i);
  }
  pthread_mutex_lock(&mutex);
  approx = approx + local_approx;
  pthread_mutex_unlock(&mutex);
  return NULL;
}

void trap() {
  double x_i;
  double h = (b-a) / n;
  approx = ( f(a) - f(b) ) / 2.0;

  pthread_t ids[NUM_THREADS];
  int ranks[NUM_THREADS];
  for (int i=0; i < NUM_THREADS; i++) {
    ranks[i] = i;
    pthread_create(&ids[i], NULL, trap_loop, &ranks[i]);
  }
  for (int i=0; i < NUM_THREADS; i++) {
    pthread_join(ids[i], NULL);
  }
  approx = h*approx;
  return;
}

int main() {
  pthread_mutex_init(&mutex, NULL);
  // Example 1 [-1,1]
  a = -1.0;
  b = 1.0;
  n = 100000000;
  timerStart();
  trap();
  printf("Took %ld ms\n", timerStop());
  printf("a:%f\t b:%f\t n:%d\t actual:%f\t approximation:%f\n", a, b, n, NEG_1_TO_POS_1, approx);

  // Example 2 [0,10]
  a = 0.0;
  b = 10.0;
  n = 100000000;
  timerStart();
  trap();
  printf("Took %ld ms\n", timerStop());
  printf("a:%f\t b:%f\t n:%d\t actual:%f\t approximation:%f\n", a, b, n, ZERO_TO_POS_10, approx);

  return 0;
}
