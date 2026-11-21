#include "divide_conquer.h"
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct { size_t start, end; long long sum; } Subarray;
static int add_ll(long long a, long long b, long long *out) {
    if ((b > 0 && a > LLONG_MAX - b) || (b < 0 && a < LLONG_MIN - b))
        return SD_ERR_OVERFLOW;
    *out = a + b;
    return SD_OK;
}
static int better(Subarray a, Subarray b) {
    return a.sum > b.sum || (a.sum == b.sum &&
           (a.start < b.start || (a.start == b.start && a.end < b.end)));
}
static int subarray_rec(const long long *a, size_t lo, size_t hi,
                        Subarray *result, uint64_t *ops) {
    if (hi - lo == 1) { *result = (Subarray){lo, lo, a[lo]}; return SD_OK; }
    size_t mid = lo + (hi - lo) / 2;
    Subarray left, right;
    int rc = subarray_rec(a, lo, mid, &left, ops);
    if (rc != SD_OK) return rc;
    rc = subarray_rec(a, mid, hi, &right, ops);
    if (rc != SD_OK) return rc;
    Subarray best_left = {mid - 1, mid - 1, a[mid - 1]};
    long long running = 0;
    for (size_t i = mid; i-- > lo;) {
        rc = add_ll(running, a[i], &running);
        if (rc != SD_OK) return rc;
        if (ops) ++*ops;
        Subarray candidate = {i, mid - 1, running};
        if (better(candidate, best_left)) best_left = candidate;
    }
    Subarray best_right = {mid, mid, a[mid]};
    running = 0;
    for (size_t j = mid; j < hi; ++j) {
        rc = add_ll(running, a[j], &running);
        if (rc != SD_OK) return rc;
        if (ops) ++*ops;
        Subarray candidate = {mid, j, running};
        if (better(candidate, best_right)) best_right = candidate;
    }
    Subarray cross = {best_left.start, best_right.end, 0};
    rc = add_ll(best_left.sum, best_right.sum, &cross.sum);
    if (rc != SD_OK) return rc;
    *result = better(left, right) ? left : right;
    if (better(cross, *result)) *result = cross;
    return SD_OK;
}
int max_subarray(const long long *a, size_t n, size_t *start,
                 size_t *end, long long *sum, uint64_t *ops) {
    if (!a || !n || !start || !end || !sum || n > PTRDIFF_MAX)
        return SD_ERR_INVALID_ARGUMENT;
    Subarray answer;
    int rc = subarray_rec(a, 0, n, &answer, ops);
    if (rc == SD_OK) { *start = answer.start; *end = answer.end; *sum = answer.sum; }
    return rc;
}
static int multiply_ll(long long a, long long b, long long *out) {
    if (a > 0) {
        if ((b > 0 && a > LLONG_MAX / b) || (b < 0 && b < LLONG_MIN / a))
            return SD_ERR_OVERFLOW;
    } else if (a < 0) {
        if ((b > 0 && a < LLONG_MIN / b) ||
            (b < 0 && a < LLONG_MAX / b)) return SD_ERR_OVERFLOW;
    }
    *out = a * b;
    return SD_OK;
}
int fast_power(long long base, uint64_t exponent, long long *out, uint64_t *ops) {
    if (!out) return SD_ERR_INVALID_ARGUMENT;
    long long result = 1;
    while (exponent) {
        int rc;
        if (exponent & 1u) {
            rc = multiply_ll(result, base, &result);
            if (rc != SD_OK) return rc;
            if (ops) ++*ops;
        }
        exponent >>= 1;
        if (exponent) {
            rc = multiply_ll(base, base, &base);
            if (rc != SD_OK) return rc;
            if (ops) ++*ops;
        }
    }
    *out = result;
    return SD_OK;
}
/* Add modulo m without ever forming a+b when it would overflow. */
static uint64_t add_mod(uint64_t a, uint64_t b, uint64_t m) {
    return a >= m - b ? a - (m - b) : a + b;
}
static uint64_t multiply_mod(uint64_t a, uint64_t b, uint64_t m) {
    uint64_t result = 0;
    while (b) {
        if (b & 1u) result = add_mod(result, a, m);
        b >>= 1;
        if (b) a = add_mod(a, a, m);
    }
    return result;
}
int fast_power_mod(long long base, uint64_t exponent, long long modulus,
                   long long *out, uint64_t *ops) {
    if (!out || modulus <= 0) return SD_ERR_INVALID_ARGUMENT;
    uint64_t m = (uint64_t)modulus;
    uint64_t x = (uint64_t)((base % modulus + (base % modulus < 0 ? modulus : 0)));
    uint64_t result = 1u % m;
    while (exponent) {
        if (exponent & 1u) {
            result = multiply_mod(result, x, m);
            if (ops) ++*ops;
        }
        exponent >>= 1;
        if (exponent) {
            x = multiply_mod(x, x, m);
            if (ops) ++*ops;
        }
    }
    *out = (long long)result;
    return SD_OK;
}
int fast_power_double(double base, long long exponent, double *out, uint64_t *ops) {
    if (!out || !isfinite(base) || (base == 0.0 && exponent < 0))
        return SD_ERR_INVALID_ARGUMENT;
    int negative = exponent < 0;
    uint64_t e = negative ? (uint64_t)(-(exponent + 1)) + 1u : (uint64_t)exponent;
    double result = 1.0;
    if (negative) base = 1.0 / base;
    if (!isfinite(base)) return SD_ERR_OVERFLOW;
    while (e) {
        if (e & 1u) {
            result *= base;
            if (!isfinite(result)) return SD_ERR_OVERFLOW;
            if (ops) ++*ops;
        }
        e >>= 1;
        if (e) {
            base *= base;
            if (!isfinite(base)) return SD_ERR_OVERFLOW;
            if (ops) ++*ops;
        }
    }
    *out = result;
    return SD_OK;
}
int find_celebrity(const unsigned char *matrix, size_t n, ptrdiff_t *index,
                   uint64_t *ops) {
    if (!index || (!matrix && n) || n > PTRDIFF_MAX ||
        (n && n > SIZE_MAX / n)) return SD_ERR_INVALID_ARGUMENT;
    if (n == 0) { *index = -1; return SD_OK; }
    size_t candidate = 0;
    for (size_t i = 1; i < n; ++i) {
        if (ops) ++*ops;
        if (matrix[candidate * n + i]) candidate = i;
    }
    for (size_t i = 0; i < n; ++i) {
        if (i == candidate) continue;
        if (ops) *ops += 2;
        if (matrix[candidate * n + i] || !matrix[i * n + candidate]) {
            *index = -1; return SD_OK;
        }
    }
    *index = (ptrdiff_t)candidate;
    return SD_OK;
}
static int valid_matrix(const double *a, const double *b, double *out, size_t n) {
    if (n == 0) return 1;
    if (!a || !b || !out || n > SIZE_MAX / n ||
        n * n > SIZE_MAX / sizeof(double)) return 0;
    uintptr_t aa = (uintptr_t)a, bb = (uintptr_t)b, cc = (uintptr_t)out;
    size_t bytes = n * n * sizeof(double);
    if (aa > UINTPTR_MAX - bytes || bb > UINTPTR_MAX - bytes ||
        cc > UINTPTR_MAX - bytes) return 0;
    return !(aa < cc + bytes && cc < aa + bytes) &&
           !(bb < cc + bytes && cc < bb + bytes);
}
static void classical(const double *a, const double *b, double *c,
                      size_t n, uint64_t *ops) {
    for (size_t i = 0; i < n; ++i)
        for (size_t j = 0; j < n; ++j) {
            double sum = 0.0;
            for (size_t k = 0; k < n; ++k) {
                sum += a[i*n+k] * b[k*n+j];
                if (ops) ++*ops;
            }
            c[i*n+j] = sum;
        }
}
int matrix_multiply_classical(const double *a, const double *b, double *out,
                              size_t n, uint64_t *ops) {
    if (!valid_matrix(a, b, out, n)) return SD_ERR_INVALID_ARGUMENT;
    if (n) classical(a, b, out, n, ops);
    return SD_OK;
}
static void combine(const double *a, const double *b, double *out,
                    size_t n, int sign) {
    for (size_t i = 0; i < n*n; ++i) out[i] = a[i] + sign * b[i];
}
static void extract(const double *a, double *out, size_t n,
                    size_t row, size_t col) {
    size_t h = n / 2;
    for (size_t i = 0; i < h; ++i)
        for (size_t j = 0; j < h; ++j)
            out[i*h+j] = a[(row*h+i)*n + col*h+j];
}
static int strassen_rec(const double *a, const double *b, double *c,
                        size_t n, uint64_t *ops) {
    if (n <= STRASSEN_CUTOFF) { classical(a, b, c, n, ops); return SD_OK; }
    size_t h = n / 2, cells = h*h;
    /* 8 quadrants, 7 products and 2 scratch buffers. */
    if (cells > SIZE_MAX / (17 * sizeof(double))) return SD_ERR_OVERFLOW;
    double *space = checked_calloc(17*cells, sizeof(double));
    if (!space) return SD_ERR_OUT_OF_MEMORY;
    double *q[4], *r[4], *p[7], *s, *t;
    for (size_t i = 0; i < 4; ++i) {
        q[i] = space + i*cells;
        r[i] = space + (i+4)*cells;
    }
    for (size_t i = 0; i < 7; ++i) p[i] = space + (i+8)*cells;
    s = space + 15*cells;
    t = space + 16*cells;
    for (size_t i = 0; i < 4; ++i) {
        extract(a, q[i], n, i/2, i%2);
        extract(b, r[i], n, i/2, i%2);
    }
    int rc;
#define RUN(X,Y,K) do { rc = strassen_rec((X),(Y),p[(K)],h,ops); if (rc != SD_OK) goto cleanup; } while (0)
    combine(q[0],q[3],s,h,1); combine(r[0],r[3],t,h,1); RUN(s,t,0);
    combine(q[2],q[3],s,h,1); RUN(s,r[0],1);
    combine(r[1],r[3],t,h,-1); RUN(q[0],t,2);
    combine(r[2],r[0],t,h,-1); RUN(q[3],t,3);
    combine(q[0],q[1],s,h,1); RUN(s,r[3],4);
    combine(q[2],q[0],s,h,-1); combine(r[0],r[1],t,h,1); RUN(s,t,5);
    combine(q[1],q[3],s,h,-1); combine(r[2],r[3],t,h,1); RUN(s,t,6);
#undef RUN
    for (size_t i = 0; i < h; ++i)
        for (size_t j = 0; j < h; ++j) {
            size_t k = i*h+j;
            c[i*n+j] = p[0][k]+p[3][k]-p[4][k]+p[6][k];
            c[i*n+j+h] = p[2][k]+p[4][k];
            c[(i+h)*n+j] = p[1][k]+p[3][k];
            c[(i+h)*n+j+h] = p[0][k]-p[1][k]+p[2][k]+p[5][k];
        }
cleanup:
    free(space);
    return rc;
}
int matrix_multiply_strassen(const double *a, const double *b, double *out,
                             size_t n, uint64_t *ops) {
    if (!valid_matrix(a,b,out,n)) return SD_ERR_INVALID_ARGUMENT;
    if (!n) return SD_OK;
    size_t padded = 1;
    while (padded < n) {
        if (padded > SIZE_MAX/2) return SD_ERR_OVERFLOW;
        padded *= 2;
    }
    if (padded > SIZE_MAX/padded ||
        padded*padded > SIZE_MAX/(3*sizeof(double))) return SD_ERR_OVERFLOW;
    double *space = checked_calloc(3*padded*padded, sizeof(double));
    if (!space) return SD_ERR_OUT_OF_MEMORY;
    size_t cells = padded*padded;
    double *aa = space, *bb = space+cells, *cc = space+2*cells;
    for (size_t i = 0; i < n; ++i) {
        memcpy(aa+i*padded,a+i*n,n*sizeof(double));
        memcpy(bb+i*padded,b+i*n,n*sizeof(double));
    }
    int rc = strassen_rec(aa,bb,cc,padded,ops);
    if (rc == SD_OK)
        for (size_t i = 0; i < n; ++i)
            memcpy(out+i*n,cc+i*padded,n*sizeof(double));
    free(space);
    return rc;
}
