#ifndef SIGNALDESK_DYNAMIC_PROGRAMMING_H
#define SIGNALDESK_DYNAMIC_PROGRAMMING_H
#include <stddef.h>
#include <stdint.h>
#include "common.h"
#include "graph_types.h"

/* All returned arrays/strings are allocated and owned by caller (free them).
   On error, output pointers are NULL. ops is optional (NULL disables counting). */
int knapsack_01_dp(const int *weights, const long long *values, size_t n,
                   size_t capacity, long long *best, size_t **chosen,
                   size_t *chosen_count, uint64_t *ops);
/* dims contains n+1 positive matrix dimensions. n=0 is valid (cost 0, ""). */
int matrix_chain_order(const size_t *dims, size_t n, uint64_t *cost,
                       char **parenthesization, uint64_t *ops);
/* Tokenization lowercases ASCII letters and splits on non-ASCII-alphanumeric
   bytes. Each returned token and the array are caller-owned. */
int tokenize_text(const char *text, char ***tokens, size_t *count);
void free_tokens(char **tokens, size_t count);
/* LCS measures textual token overlap, NOT semantic meaning or factual truth.
   Returned indices refer to input token arrays, and belong to the caller.
   Similarity is 2 * LCS / (len_a + len_b), or 1 for two empty sequences. */
int lcs_tokens(const char *const *a, size_t len_a, const char *const *b,
               size_t len_b, size_t *length, size_t **indices_a,
               size_t **indices_b, double *similarity, uint64_t *ops);
/* Sorted keys with nonnegative successful-search frequencies. Cost is
   frequency-weighted comparisons; divide by total frequency for expectation.
   roots is n*n row-major, entries are root indices; SIZE_MAX for empty cells. */
int optimal_bst(const double *frequencies, size_t n, double *expected_cost,
                size_t **roots, uint64_t *ops);
/* Distances use INFINITY for unreachable nodes; predecessor SIZE_MAX means
   absent. A reachable negative cycle sets flag=1 and returns SD_ERR_NEGATIVE_CYCLE.
   Unreachable negative cycles do not trigger the flag. */
int bellman_ford(const GraphList *graph, size_t source, double **distances,
                 size_t **predecessors, int *reachable_negative_cycle, uint64_t *ops);
/* Next-hop is n*n row-major, SIZE_MAX for unreachable pairs. Distances and
   next-hop belong to caller; a negative diagonal detects any negative cycle. */
int floyd_warshall(const GraphMatrix *graph, double **distances,
                   size_t **next_hop, int *negative_cycle, uint64_t *ops);
/* TSP expects a complete matrix of finite distances, with nonnegative edge
   weights (diagonal ignored). Visits each city and returns to city 0.
   n=0 is accepted (cost 0, empty tour); n>15 is invalid. tour length is n+1. */
int tsp_dp(const GraphMatrix *graph, double *cost, size_t **tour,
           size_t *tour_length, uint64_t *ops);
#endif
