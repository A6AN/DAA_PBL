#ifndef SIGNALDESK_BACKTRACKING_H
#define SIGNALDESK_BACKTRACKING_H

#include <stddef.h>
#include <stdint.h>
#include "common.h"

/* All algorithms return SD_OK (0) or a negative SDStatus code.
   Optional ops counts candidate/branch examinations; NULL is permitted.
   Callers own returned arrays and free them using free(), except subset lists,
   which must be released by subsets_free(). Output pointers are reset on entry. */

/* n in [1,12]. first_board[row] = column of queen; NULL if unsatisfiable.
   The first board and solution_count are returned independently. */
int n_queens(size_t n, uint64_t *solution_count, size_t **first_board,
             uint64_t *ops);

typedef struct {
    size_t *indices; /* Owned index array, ascending by original position. */
    size_t count;
} SDSubset;

/* Enumerate ALL subsets of nonnegative integer values summing to target.
   Empty input has one solution when target is zero: the empty subset.
   Returned array and its index arrays belong to caller. */
int sum_of_subsets(const int *values, size_t n, size_t target,
                   SDSubset **solutions, size_t *solution_count, uint64_t *ops);
void subsets_free(SDSubset *solutions, size_t solution_count);

/* Exact 0/1 knapsack: nonnegative integer weights and values, capacity is
   size_t. best is optimal value, chosen is ascending ORIGINAL item indices.
   A zero-weight item may be selected; zero-value items need not be selected.
   The chosen array belongs to caller, NULL when chosen_count is zero. */
int knapsack_01_backtracking(const int *weights, const long long *values,
                            size_t n, size_t capacity, long long *best,
                            size_t **chosen, size_t *chosen_count, uint64_t *ops);

/* Each B&B strategy uses the same density order, fractional upper bound and
   search node. Only frontier removal differs (FIFO, LIFO or least bound cost).
   nodes_generated includes the root; nodes_expanded counts nodes branched;
   max_frontier is the largest pending-node count. All three return optimal
   values and original indices, with caller ownership of chosen. */
typedef struct {
    long long best_value;
    size_t *chosen;
    size_t chosen_count;
    uint64_t nodes_generated;
    uint64_t nodes_expanded;
    size_t max_frontier;
} SDBBResult;

void bb_result_free(SDBBResult *result);
int knapsack_bb_fifo(const int *weights, const long long *values, size_t n,
                     size_t capacity, SDBBResult *result, uint64_t *ops);
int knapsack_bb_lifo(const int *weights, const long long *values, size_t n,
                     size_t capacity, SDBBResult *result, uint64_t *ops);
int knapsack_bb_least_cost(const int *weights, const long long *values,
                           size_t n, size_t capacity, SDBBResult *result,
                           uint64_t *ops);
#endif
