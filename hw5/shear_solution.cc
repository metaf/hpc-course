/**
 * University of Pittsburgh
 * Department of Computer Science
 * CS1645: Introduction to HPC Systems
 * Instructor Bryan Mills, PhD (bmills@cs.pitt.edu)
 * STUDENTS: Implement OpenMP parallel shear sort.
 */

#include <omp.h>
#include <math.h>
#include "timer.h"
#include "io.h"

#define MAX_VALUE 10000

// Function to sort row (uses bubble sort)
void sort_row(int **A, int M, int i, int order){
  for(int step=0; step<M; step++) {
    for(int j=1; j<M; j++){
      if(A[i][j-1]*order > A[i][j]*order) {
	int tmp = A[i][j-1];
	A[i][j-1] = A[i][j];
	A[i][j] = tmp;
      }
    }
  }
}

// Function to sort column (uses bubble sort)
void sort_col(int **A, int M, int j){
  for(int step=0; step<M; step++) {
    for(int i=1; i<M; i++){
      if(A[i-1][j] > A[i][j]) {
	int tmp = A[i-1][j];
	A[i-1][j] = A[i][j];
	A[i][j] = tmp;
      }
    }
  }
}

void shear_sort(int **A, int M){
  int N = M*M, total;
  total = ceil(log2(N));

  for(int stage=0; stage<total; stage++){

    // sorting rows
    #pragma omp parallel for
    for(int i=0; i<M; i++){
      sort_row(A,M,i,int(pow(-1,i)));
    }

    // sorting columns
    #pragma omp parallel for
    for(int j=0; j<M; j++){
      sort_col(A,M,j);
    }
  }
}

// Allocate square matrix.
int **allocMatrix(int size) {
  int **matrix;
  matrix = (int **)malloc(size * sizeof(int *));
  for (int row = 0; row < size; row++) {
    matrix[row] = (int *)malloc(size * sizeof(int));
  }
  for (int i = 0; i < size; i++) {
    for (int j = 0; j < size; j++) {
      matrix[i][j] = 0;
    }
  }
  return matrix;
}

// Main method      
int main(int argc, char* argv[]) {
  int N, M;
  int **A;
  double elapsedTime;

  // checking parameters
  if (argc != 2 && argc != 3) {
    printf("Parameters: <N> [<file>]\n");
    return 1;
  }
  N = atoi(argv[1]);
  M = (int) sqrt(N); 
  if(N != M*M){
    printf("N has to be a perfect square!\n");
    exit(1);
  }

  // allocating matrix A
  A = allocMatrix(M);

  // reading files (optional)
  if(argc == 3){
    readMatrixFile(A,M,argv[2]);
  } else {
    srand (time(NULL));
    // Otherwise, generate random matrix.
    for (int i=0; i<M; i++) {
      for (int j=0; j<M; j++) {
	A[i][j] = rand() % MAX_VALUE;
      }
    }
  }
  
  // starting timer
  timerStart();

  // calling shear sort function
  shear_sort(A,M);
  // stopping timer
  elapsedTime = timerStop();

  // print if reasonably small
  if (M <= 10) {
    printMatrix(A,M);
  }

  printf("Took %ld ms\n", timerStop());

  // releasing memory
  for (int i=0; i<M; i++) {
    delete [] A[i];
  }
  delete [] A;

  return 0;
}
