#ifndef LAB3_GAUSS_H
#define LAB3_GAUSS_H

#include "system.h"

/* solves in place with the given number of threads, the matrix is destroyed */
int gauss_solve(linear_system* system, int threads, double* x);

#endif
