#ifndef LAB3_SYSTEM_H
#define LAB3_SYSTEM_H

#include <stdio.h>

/* augmented matrix [A | b] in one block, rows are pointers so a swap is cheap */
typedef struct {
    int      size;
    double*  data;
    double** rows;
} linear_system;

/* these return the exit codes from common.h */
int  system_create(linear_system* system, int size);
int  system_copy(linear_system* copy, const linear_system* source);
void system_destroy(linear_system* system);

/* format is n and then n rows "a1 ... an b"
   prompt asks for every row when a person types it */
int system_read(FILE* input, linear_system* system, int prompt);

/* random matrix with b_i = sum of row i so the answer is all ones */
int system_generate(linear_system* system, int size);

int system_write_solution(FILE* output, const double* x, int size);

/* max |Ax - b| on the original matrix */
double system_residual(const linear_system* system, const double* x);

#endif
