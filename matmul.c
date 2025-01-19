#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define N 10

void initialize_matrix(int matrix[N][N]) {
    for(int i = 0; i < N; i++) {
        for(int j = 0; j < N; j++) {
            matrix[i][j] = rand() % 10;  // Random values between 0-9
        }
    }
}

void print_matrix(int matrix[N][N]) {
    for(int i = 0; i < N; i++) {
        for(int j = 0; j < N; j++) {
            printf("%d ", matrix[i][j]);
        }
        printf("\n");
    }
}

void matrix_multiply(int A[N][N], int B[N][N], int C[N][N]) {
    for(int i = 0; i < N; i++) {
        for(int j = 0; j < N; j++) {
            C[i][j] = 0;
            for(int k = 0; k < N; k++) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }
}

int main() {
    int A[N][N], B[N][N], C[N][N];
    
    // Initialize random seed
    srand(time(NULL));
    
    // Initialize matrices with random values
    initialize_matrix(A);
    initialize_matrix(B);
    
    // Print input matrices
    printf("Matrix A:\n");
    print_matrix(A);
    printf("\nMatrix B:\n");
    print_matrix(B);
    
    // Perform matrix multiplication
    matrix_multiply(A, B, C);
    
    // Print result
    printf("\nResultant Matrix C = A × B:\n");
    print_matrix(C);
    
    return 0;
}