#include "backtracking.h"
#include "dynamic_programming.h"
#include "test_harness.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

static long long exhaustive_best(const int *weights, const long long *values,
                                 size_t n, size_t capacity) {
    long long best = 0;
    for (size_t mask = 0; mask < ((size_t)1 << n); ++mask) {
        size_t used = 0; long long value = 0;
        for (size_t i = 0; i < n; ++i) if (mask & ((size_t)1 << i)) {
            used += (size_t)weights[i]; value += values[i];
        }
        if (used <= capacity && value > best) best = value;
    }
    return best;
}
static size_t exhaustive_subsets(const int *values, size_t n, size_t target) {
    size_t count = 0;
    for (size_t mask = 0; mask < ((size_t)1 << n); ++mask) {
        size_t sum = 0;
        for (size_t i = 0; i < n; ++i)
            if (mask & ((size_t)1 << i)) sum += (size_t)values[i];
        if (sum == target) ++count;
    }
    return count;
}
static void verify_picked(const int *weights, const long long *values,
                          size_t n, size_t capacity, long long expected,
                          const size_t *chosen, size_t count) {
    size_t weight = 0; long long value = 0;
    ASSERT_TRUE(count == 0 || chosen != NULL);
    for (size_t i = 0; i < count; ++i) {
        ASSERT_TRUE(chosen[i] < n);
        if (i) ASSERT_TRUE(chosen[i - 1] < chosen[i]);
        if (chosen[i] < n) {
            weight += (size_t)weights[chosen[i]];
            value += values[chosen[i]];
        }
    }
    ASSERT_TRUE(weight <= capacity);
    ASSERT_EQ_INT(expected, value);
}
static void test_queens(void) {
    static const uint64_t known[] = {1,0,0,2,10,4,40,92,352,724,2680,14200};
    uint64_t count = 0, ops = 0; size_t *board = NULL;
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT, n_queens(0, &count, &board, NULL));
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT, n_queens(13, &count, &board, NULL));
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT, n_queens(1, NULL, &board, NULL));
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT, n_queens(1, &count, NULL, NULL));
    for (size_t n = 1; n <= 12; ++n) {
        ASSERT_EQ_INT(SD_OK, n_queens(n, &count, &board, &ops));
        ASSERT_EQ_INT(known[n-1], count);
        ASSERT_EQ_INT(count != 0, board != NULL);
        if (board) {
            for (size_t i = 0; i < n; ++i) {
                ASSERT_TRUE(board[i] < n);
                for (size_t j = i + 1; j < n; ++j) {
                    ASSERT_TRUE(board[i] != board[j]);
                    size_t dr = j - i;
                    size_t dc = board[i] > board[j] ? board[i] - board[j] : board[j] - board[i];
                    ASSERT_TRUE(dr != dc);
                }
            }
        }
        free(board); board = NULL;
    }
    ASSERT_TRUE(ops > 0);
}
static void test_subsets(void) {
    SDSubset *solutions = NULL; size_t count = 0; uint64_t ops = 0;
    ASSERT_EQ_INT(SD_OK, sum_of_subsets(NULL, 0, 0, &solutions, &count, &ops));
    ASSERT_EQ_INT(1, count);
    ASSERT_EQ_INT(0, solutions[0].count);
    subsets_free(solutions, count);
    ASSERT_EQ_INT(SD_OK, sum_of_subsets(NULL, 0, 1, &solutions, &count, NULL));
    ASSERT_EQ_INT(0, count); subsets_free(solutions, count);
    int values[] = {0, 1, 2, 3, 0};
    ASSERT_EQ_INT(SD_OK, sum_of_subsets(values, 5, 0, &solutions, &count, &ops));
    ASSERT_EQ_INT(4, count); subsets_free(solutions, count);
    ASSERT_EQ_INT(SD_OK, sum_of_subsets(values, 5, 50, &solutions, &count, &ops));
    ASSERT_EQ_INT(0, count); subsets_free(solutions, count);
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT, sum_of_subsets(NULL, 1, 0, &solutions, &count, NULL));
    values[0] = -1;
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT, sum_of_subsets(values, 5, 0, &solutions, &count, NULL));
    values[0] = 0;
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT, sum_of_subsets(values, 5, 0, NULL, &count, NULL));
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT, sum_of_subsets(values, 5, 0, &solutions, NULL, NULL));
    lcg_seed(405);
    for (int trial = 0; trial < 300; ++trial) {
        int v[10]; size_t n = (size_t)lcg_range(0, 10);
        size_t target = (size_t)lcg_range(0, 35);
        for (size_t i = 0; i < n; ++i) v[i] = (int)lcg_range(0, 10);
        ASSERT_EQ_INT(SD_OK, sum_of_subsets(v, n, target, &solutions, &count, &ops));
        ASSERT_EQ_INT(exhaustive_subsets(v, n, target), count);
        for (size_t i = 0; i < count; ++i) {
            size_t sum = 0;
            for (size_t j = 0; j < solutions[i].count; ++j) {
                size_t index = solutions[i].indices[j];
                ASSERT_TRUE(index < n);
                if (j) ASSERT_TRUE(solutions[i].indices[j-1] < index);
                if (index < n) sum += (size_t)v[index];
            }
            ASSERT_EQ_INT(target, sum);
        }
        subsets_free(solutions, count);
    }
    ASSERT_TRUE(ops > 0);
}
typedef int (*BBFunction)(const int *, const long long *, size_t, size_t,
                          SDBBResult *, uint64_t *);
static const BBFunction solvers[] = {knapsack_bb_fifo, knapsack_bb_lifo,
                                     knapsack_bb_least_cost};
static void crosscheck_one(const int *weights, const long long *values,
                           size_t n, size_t capacity, uint64_t *ops) {
    long long expected = exhaustive_best(weights, values, n, capacity);
    long long best = -1, dp_best = -1;
    size_t *chosen = NULL, *dp_chosen = NULL, chosen_count = 0, dp_count = 0;
    ASSERT_EQ_INT(SD_OK, knapsack_01_backtracking(weights, values, n, capacity,
                   &best, &chosen, &chosen_count, ops));
    ASSERT_EQ_INT(expected, best);
    verify_picked(weights, values, n, capacity, best, chosen, chosen_count);
    free(chosen);
    ASSERT_EQ_INT(SD_OK, knapsack_01_dp(weights, values, n, capacity,
                   &dp_best, &dp_chosen, &dp_count, ops));
    ASSERT_EQ_INT(expected, dp_best);
    verify_picked(weights, values, n, capacity, dp_best, dp_chosen, dp_count);
    free(dp_chosen);
    for (size_t solver = 0; solver < 3; ++solver) {
        SDBBResult result = {0};
        ASSERT_EQ_INT(SD_OK, solvers[solver](weights, values, n, capacity, &result, ops));
        ASSERT_EQ_INT(expected, result.best_value);
        verify_picked(weights, values, n, capacity, result.best_value,
                      result.chosen, result.chosen_count);
        ASSERT_TRUE(result.nodes_generated >= 1);
        ASSERT_TRUE(result.nodes_expanded <= result.nodes_generated);
        ASSERT_TRUE(result.max_frontier >= 1);
        ASSERT_TRUE(result.max_frontier <= result.nodes_generated);
        bb_result_free(&result);
        ASSERT_EQ_INT(0, result.chosen_count);
        ASSERT_TRUE(result.chosen == NULL);
    }
}
static void test_knapsack(void) {
    uint64_t ops = 0;
    crosscheck_one(NULL, NULL, 0, 0, &ops);
    crosscheck_one(NULL, NULL, 0, 50, &ops);
    int w1[] = {11}; long long v1[] = {100};
    crosscheck_one(w1, v1, 1, 10, &ops);
    crosscheck_one(w1, v1, 1, 11, &ops);
    int w2[] = {0, 1, 8, 2}; long long v2[] = {7, 10, 200, 10};
    crosscheck_one(w2, v2, 4, 0, &ops);
    crosscheck_one(w2, v2, 4, 9, &ops);
    int w3[] = {1, 1, 1, 1, 1}; long long v3[] = {9, 9, 9, 9, 9};
    crosscheck_one(w3, v3, 5, 3, &ops);
    int w4[] = {0, 0, 0}; long long v4[] = {5, 0, 7};
    crosscheck_one(w4, v4, 3, 0, &ops);
    long long best; size_t *chosen; size_t count; SDBBResult out = {0};
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT, knapsack_01_backtracking(NULL, v1, 1, 1,
                  &best, &chosen, &count, NULL));
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT, knapsack_01_backtracking(w1, NULL, 1, 1,
                  &best, &chosen, &count, NULL));
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT, knapsack_01_backtracking(w1, v1, 1, 1,
                  NULL, &chosen, &count, NULL));
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT, knapsack_bb_fifo(NULL, v1, 1, 1, &out, NULL));
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT, knapsack_bb_lifo(w1, NULL, 1, 1, &out, NULL));
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT, knapsack_bb_least_cost(w1, v1, 1, 1, NULL, NULL));
    w1[0] = -1;
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT, knapsack_bb_fifo(w1, v1, 1, 1, &out, NULL));
    w1[0] = 1; v1[0] = -1;
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT, knapsack_bb_fifo(w1, v1, 1, 1, &out, NULL));
    v1[0] = 100;
    lcg_seed(404);
    for (int trial = 0; trial < 300; ++trial) {
        int weights[11]; long long values[11];
        size_t n = (size_t)lcg_range(0, 11);
        size_t capacity = (size_t)lcg_range(0, 28);
        for (size_t i = 0; i < n; ++i) {
            weights[i] = (int)lcg_range(0, 14);
            values[i] = (long long)lcg_range(0, 30);
        }
        crosscheck_one(weights, values, n, capacity, &ops);
    }
    ASSERT_TRUE(ops > 0);
}
int main(void) {
    test_queens();
    test_subsets();
    test_knapsack();
    return harness_summary();
}
