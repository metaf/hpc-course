/**
 * University of Pittsburgh
 * Department of Computer Science
 * CS1645: Introduction to HPC Systems
 * Instructor Bryan Mills, PhD
 * Student: 
 * Implement Pthreads version of Strassen algorithm for matrix multiplication.
 */

#include "timer.h"
#include "io.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define NUM_THREADS 10

// Make these globals so threads can operate on them. You will need to
// add additional matrixes for all the M and C values in the Strassen
// algorithms.
int **A;
int **B;
int **C;
// Reference matrix, call simpleMM to populate.
int **R;

// Parts, I copied them but you could reference them.
int **A11;
int **A12;
int **A21;
int **A22;
int **B11;
int **B12;
int **B21;
int **B22;

// M matrixes
int **M1; // (A_1,1 + A_2,2)*(B_1,1 + B_2,1)
int **M2; // (A_2,1 + A_2,2)*B_1,1
int **M3; // A_1,1 * (B_1,2 - B_2,2)
int **M4; // A_2,2 * (B_2,1 - B_1,1)
int **M5; // (A_1,1 + A_1,2) * B_2,2
int **M6; // (A_2,1 - A_1,1) * (B_1,1 + B_1,2)
int **M7; // (A_1,2 - A_2,2) * (B_2,1 + B_2,2)

// C matrixes
int **C11;
int **C12;
int **C21;
int **C22;

int N;

// Stupid simple Matrix Multiplication, meant as example.
void simpleMM(int **a, int **b, int **r, int N) {
  for (int i=0; i<N; i++) {
    for (int j=0; j<N; j++) {
      for (int k=0; k<N; k++) {
	r[i][j] += a[i][k] * b[k][j];
      }
    }
  }
}

void simpleAdd(int **a, int **b, int **r, int N) {
  for (int i=0; i<N; i++) {
    for (int j=0; j<N; j++) {
      r[i][j] = a[i][j] + b[i][j];
    }
  }
}

void simpleSub(int **a, int **b, int **r, int N) {
  for (int i=0; i<N; i++) {
    for (int j=0; j<N; j++) {
      r[i][j] = a[i][j] - b[i][j];
    }
  }
}

void makeParts(int N) {
  int half = N/2;
  for (int row = 0; row < half; row++) {
    for (int col = 0; col < half; col++) {
      A11[row][col] = A[row][col];
      A12[row][col] = A[row][half+col];
      A21[row][col] = A[half+row][col];
      A22[row][col] = A[half+row][half+col];

      B11[row][col] = B[row][col];
      B12[row][col] = B[row][half+col];
      B21[row][col] = B[half+row][col];
      B22[row][col] = B[half+row][half+col];
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

// (A_1,1 + A_2,2)*(B_1,1 + B_2,1)
void* calcM1(void* nothing) {
  int **temp1;
  int **temp2;
  temp1 = allocMatrix(N/2);
  temp2 = allocMatrix(N/2);
  simpleAdd(A11, A22, temp1, N/2);
  simpleAdd(A11, A22, temp2, N/2);
  simpleMM(temp1, temp2, M1, N/2);
  free(temp1);
  free(temp2);
  return 0;
}

// (A_2,1 + A_2,2)*B_1,1
void* calcM2(void* nothing) {
  int **temp1;
  temp1 = allocMatrix(N/2);
  simpleAdd(A21, A22, temp1, N/2);
  simpleMM(temp1, B11, M2, N/2);
  free(temp1);
  return 0;
}

// A_1,1 * (B_1,2 - B_2,2)
void* calcM3(void* nothing) {
  int **temp1;
  temp1 = allocMatrix(N/2);
  simpleSub(B12, B22, temp1, N/2);
  simpleMM(A11, temp1, M3, N/2);
  free(temp1);
  return 0;
}

// A_2,2 * (B_2,1 - B_1,1)
void* calcM4(void* nothing) {
  int **temp1;
  temp1 = allocMatrix(N/2);
  simpleSub(B21, B11, temp1, N/2);
  simpleMM(A22, temp1, M4, N/2);
  free(temp1);
  return 0;
}

// (A_1,1 + A_1,2) * B_2,2
void* calcM5(void *nothing) {
  int **temp1;
  temp1 = allocMatrix(N/2);
  simpleAdd(A11, A12, temp1, N/2);
  simpleMM(temp1, B22, M5, N/2);
  free(temp1);
  return 0;
}

// (A_2,1 - A_1,1) * (B_1,1 + B_1,2)
void* calcM6(void* nothing) {
  int **temp1;
  int **temp2;
  temp1 = allocMatrix(N/2);
  temp2 = allocMatrix(N/2);
  simpleSub(A21, A11, temp1, N/2);
  simpleAdd(B11, B12, temp2, N/2);
  simpleMM(temp1, temp2, M6, N/2);
  free(temp1);
  free(temp2);
  return 0;
}

// (A_1,2 - A_2,2) * (B_2,1 + B_2,2)
void* calcM7(void *nothing) {
  int **temp1;
  int **temp2;
  temp1 = allocMatrix(N/2);
  temp2 = allocMatrix(N/2);
  simpleSub(A12, A22, temp1, N/2);
  simpleAdd(B21, B22, temp2, N/2);
  simpleMM(temp1, temp2, M7, N/2);
  free(temp1);
  free(temp2);
  return 0;
}

// (A_1,1 * B_1,1) + (A_1,2 * B_2,1)
void* calcC11(void* nothing) {
  int **temp1;
  int **temp2;
  temp1 = allocMatrix(N/2);
  temp2 = allocMatrix(N/2);
  simpleMM(A11, B11, temp1, N/2);
  simpleMM(A12, B21, temp2, N/2);
  simpleAdd(temp1, temp2, C11, N/2);
  free(temp1);
  free(temp2);
  return 0;
}

// (A_1,1 * B_1,2) + (A_1,2 * B_2,2)
void* calcC12(void* nothing) {
  int **temp1;
  int **temp2;
  temp1 = allocMatrix(N/2);
  temp2 = allocMatrix(N/2);
  simpleMM(A11, B12, temp1, N/2);
  simpleMM(A12, B22, temp2, N/2);
  simpleAdd(temp1, temp2, C12, N/2);
  free(temp1);
  free(temp2);
  return 0;
}

// (A_2,1 * B_1,1) + (A_2,2 * B_2,1)
void* calcC21(void* nothing) {
  int **temp1;
  int **temp2;
  temp1 = allocMatrix(N/2);
  temp2 = allocMatrix(N/2);
  simpleMM(A21, B11, temp1, N/2);
  simpleMM(A22, B21, temp2, N/2);
  simpleAdd(temp1, temp2, C21, N/2);
  free(temp1);
  free(temp2);
  return 0;
}

// (A_2,1 * B_1,2) + (A_2,2 * B_2,2)
void* calcC22(void * nothing) {
  int **temp1;
  int **temp2;
  temp1 = allocMatrix(N/2);
  temp2 = allocMatrix(N/2);
  simpleMM(A21, B12, temp1, N/2);
  simpleMM(A22, B22, temp2, N/2);
  simpleAdd(temp1, temp2, C22, N/2);
  free(temp1);
  free(temp2);
  return 0;
}

void copyC(int N) {
  int half = N/2;
  for (int row = 0; row < half; row++) {
    for (int col = 0; col < half; col++) {
      C[row][col] = C11[row][col];
      C[row][half+col] = C12[row][col];
      C[half+row][col] = C21[row][col];
      C[half+row][half+col] = C22[row][col];
    }
  }
}

// WRITE YOUR CODE HERE, you will need to also add functions for each
// of the sub-matrixes you will need to calculate but you can create your
// threads in this fucntion.
void strassenMM(int N) {
  pthread_t ids[NUM_THREADS];
  int i_s[NUM_THREADS];
  makeParts(N);
  pthread_create(&ids[0], NULL, calcM1, NULL); // calcM1(NULL)
  pthread_create(&ids[1], NULL, calcM2, NULL); // calcM2(NULL)
  pthread_create(&ids[2], NULL, calcM3, NULL); // calcM3(NULL)
  pthread_create(&ids[3], NULL, calcM4, NULL); // calcM4(NULL)
  pthread_create(&ids[4], NULL, calcM5, NULL); // calcM5(NULL)
  pthread_create(&ids[5], NULL, calcM6, NULL); // calcM6(NULL)
  pthread_create(&ids[6], NULL, calcM7, NULL); // calcM7(NULL)
  for (int i = 0; i < 7; i++)
    pthread_join(ids[i], NULL);
  pthread_create(&ids[0], NULL, calcC11, NULL); // calcC11(NULL)
  pthread_create(&ids[1], NULL, calcC12, NULL); // calcC12(NULL)
  pthread_create(&ids[2], NULL, calcC21, NULL); // calcC21(NULL)
  pthread_create(&ids[3], NULL, calcC22, NULL); // calcC22(NULL)
  for (int i = 0; i < 4; i++)
    pthread_join(ids[i], NULL);
  copyC(N);
}

// Allocate memory for all the matrixes, you will need to add code
// here to initialize any matrixes that you need.
void initMatrixes(int N) {
  A = allocMatrix(N); B = allocMatrix(N); C = allocMatrix(N); R = allocMatrix(N);
  int half = N/2;
  A11 = allocMatrix(half); A12 = allocMatrix(half); A21 = allocMatrix(half); A22 = allocMatrix(half);
  B11 = allocMatrix(half); B12 = allocMatrix(half); B21 = allocMatrix(half); B22 = allocMatrix(half);
  M1 = allocMatrix(half); M2 = allocMatrix(half); M3 = allocMatrix(half); M4 = allocMatrix(half);
  M5 = allocMatrix(half); M6 = allocMatrix(half); M7 = allocMatrix(half);
  C11 = allocMatrix(half); C12 = allocMatrix(half); C21 = allocMatrix(half); C22 = allocMatrix(half);
}

// Free up matrixes.
void cleanup() {
  free(A);
  free(B);
  free(C);
  free(R);
}

// Main method
int main(int argc, char* argv[]) {
  double elapsedTime;

  // checking parameters
  if (argc != 2 && argc != 4) {
    printf("Parameters: <N> [<fileA> <fileB>]\n");
    return 1;
  }
  N = atoi(argv[1]);
  initMatrixes(N);

  // reading files (optional)
  if(argc == 4){
    readMatrixFile(A,N,argv[2]);
    readMatrixFile(B,N,argv[3]);
  } else {
    // Otherwise, generate two random matrix.
    for (int i=0; i<N; i++) {
      for (int j=0; j<N; j++) {
	A[i][j] = rand() % 5;
	B[i][j] = rand() % 5;
      }
    }
  }

  // Do simple multiplication and time it.
  timerStart();
  simpleMM(A, B, R, N);
  printf("Simple MM took %ld ms\n", timerStop());

  // Do strassen multiplication and time it.
  timerStart();
  strassenMM(N);
  printf("Strassen MM took %ld ms\n", timerStop());

  if (compareMatrix(C, R, N) != 0) {
    if (N < 20) {
      printf("\n\n------- MATRIX C\n");
      printMatrix(C,N);
      printf("\n------- MATRIX R\n");
      printMatrix(R,N);
    }
    printf("Matrix C doesn't match Matrix R, if N < 20 they will be printed above.\n");
  }

  // stopping timer
  
  cleanup();
  return 0;
}
