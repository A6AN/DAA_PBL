#ifndef SIGNALDESK_DIVIDE_CONQUER_H
#define SIGNALDESK_DIVIDE_CONQUER_H
#include <stddef.h>
#include <stdint.h>
#include "common.h"

/* Non-empty array. Results are inclusive indices. On ties choose the
   smallest start index, then the smallest end index. Detect sum overflow. */
int max_subarray(const long long *values, size_t n, size_t *start,
                 size_t *end, long long *sum, uint64_t *ops);
/* Nonnegative exponent; 0^0 = 1. */
int fast_power(long long base, uint64_t exponent, long long *out, uint64_t *ops);
/* modulus > 0; returns residue in [0, modulus). Safe signed-base reduction. */
int fast_power_mod(long long base, uint64_t exponent, long long modulus,
                   long long *out, uint64_t *ops);
/* Integer exponent including negative; 0 to negative power is invalid. */
int fast_power_double(double base, long long exponent, double *out, uint64_t *ops);
/* Matrix is contiguous row-major n*n bytes, entry (i,j) = matrix[i*n+j].
   A celebrity follows nobody (diagonal ignored), and everyone else follows
   them. Returns -1 if absent. n=1 returns 0. */
int find_celebrity(const unsigned char *matrix, size_t n, ptrdiff_t *index,
                   uint64_t *ops);
/* Square row-major matrices. Caller owns a, b, out (n*n doubles each).
   Inputs and output must not overlap; zero size allows NULL arrays.
   Returns overflow if n*n or byte counts cannot be represented. */
int matrix_multiply_classical(const double *a, const double *b, double *out,
                              size_t n, uint64_t *ops);
int matrix_multiply_strassen(const double *a, const double *b, double *out,
                             size_t n, uint64_t *ops);
#define STRASSEN_CUTOFF 32u
#endif
