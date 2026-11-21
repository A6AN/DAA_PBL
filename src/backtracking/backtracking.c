#include "backtracking.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static void count_op(uint64_t *ops) {
    if (ops && *ops < UINT64_MAX) ++*ops;
}
static int validate_items(const int *weights, const long long *values, size_t n) {
    if (n && (!weights || !values)) return SD_ERR_INVALID_ARGUMENT;
    for (size_t i = 0; i < n; ++i)
        if (weights[i] < 0 || values[i] < 0) return SD_ERR_INVALID_ARGUMENT;
    return SD_OK;
}

/* The occupied-column and diagonal masks describe only previous rows. */
typedef struct {
    size_t n;
    uint64_t count;
    size_t *board;
    size_t *first;
    uint64_t *ops;
} QueensContext;
static void queens_search(QueensContext *ctx, size_t row, uint32_t col,
                          uint32_t down, uint32_t up) {
    if (row == ctx->n) {
        ++ctx->count;
        if (ctx->count == 1) memcpy(ctx->first, ctx->board,
                                    ctx->n * sizeof(*ctx->first));
        return;
    }
    for (size_t c = 0; c < ctx->n; ++c) {
        uint32_t cb = UINT32_C(1) << c;
        uint32_t db = UINT32_C(1) << (row + ctx->n - 1 - c);
        uint32_t ub = UINT32_C(1) << (row + c);
        count_op(ctx->ops);
        if ((col & cb) || (down & db) || (up & ub)) continue;
        ctx->board[row] = c;
        queens_search(ctx, row + 1, col | cb, down | db, up | ub);
    }
}
int n_queens(size_t n, uint64_t *solution_count, size_t **first_board,
             uint64_t *ops) {
    if (!solution_count || !first_board || n == 0 || n > 12)
        return SD_ERR_INVALID_ARGUMENT;
    *solution_count = 0;
    *first_board = NULL;
    size_t *board = checked_malloc(n * sizeof(*board));
    size_t *first = checked_malloc(n * sizeof(*first));
    if (!board || !first) { free(board); free(first); return SD_ERR_OUT_OF_MEMORY; }
    QueensContext ctx = {n, 0, board, first, ops};
    queens_search(&ctx, 0, 0, 0, 0);
    free(board);
    *solution_count = ctx.count;
    if (ctx.count) *first_board = first;
    else free(first);
    return SD_OK;
}

void subsets_free(SDSubset *solutions, size_t count) {
    if (!solutions) return;
    for (size_t i = 0; i < count; ++i) free(solutions[i].indices);
    free(solutions);
}
typedef struct {
    const int *values;
    size_t n, target, count, capacity;
    const size_t *suffix;
    size_t *path;
    SDSubset *solutions;
    uint64_t *ops;
} SubsetContext;
static int subsets_record(SubsetContext *ctx, size_t len) {
    if (ctx->count == ctx->capacity) {
        size_t next = ctx->capacity ? ctx->capacity * 2 : 8;
        if (next < ctx->capacity || next > SIZE_MAX / sizeof(SDSubset))
            return SD_ERR_OVERFLOW;
        void *grown = checked_realloc(ctx->solutions, next * sizeof(SDSubset));
        if (!grown) return SD_ERR_OUT_OF_MEMORY;
        ctx->solutions = grown;
        ctx->capacity = next;
    }
    size_t *indices = len ? checked_malloc(len * sizeof(*indices)) : NULL;
    if (len && !indices) return SD_ERR_OUT_OF_MEMORY;
    if (len) memcpy(indices, ctx->path, len * sizeof(*indices));
    ctx->solutions[ctx->count++] = (SDSubset){indices, len};
    return SD_OK;
}
/* A branch is impossible if its sum already exceeds the target or if all
   remaining values still cannot reach it. Both bounds require nonnegativity. */
static int subsets_search(SubsetContext *ctx, size_t at, size_t sum, size_t len) {
    if (sum > ctx->target || ctx->suffix[at] < ctx->target - sum) return SD_OK;
    if (at == ctx->n) return sum == ctx->target ? subsets_record(ctx, len) : SD_OK;
    count_op(ctx->ops);
    if ((size_t)ctx->values[at] <= ctx->target - sum) {
        ctx->path[len] = at;
        int status = subsets_search(ctx, at + 1, sum + (size_t)ctx->values[at], len + 1);
        if (status != SD_OK) return status;
    }
    return subsets_search(ctx, at + 1, sum, len);
}
int sum_of_subsets(const int *values, size_t n, size_t target,
                   SDSubset **solutions, size_t *solution_count, uint64_t *ops) {
    if (!solutions || !solution_count || (n && !values)) return SD_ERR_INVALID_ARGUMENT;
    *solutions = NULL;
    *solution_count = 0;
    if (n > SIZE_MAX / sizeof(size_t) - 1) return SD_ERR_OVERFLOW;
    for (size_t i = 0; i < n; ++i) if (values[i] < 0) return SD_ERR_INVALID_ARGUMENT;
    size_t *suffix = checked_calloc(n + 1, sizeof(*suffix));
    size_t *path = n ? checked_malloc(n * sizeof(*path)) : NULL;
    if (!suffix || (n && !path)) { free(suffix); free(path); return SD_ERR_OUT_OF_MEMORY; }
    for (size_t i = n; i-- > 0;) {
        size_t v = (size_t)values[i];
        suffix[i] = suffix[i + 1] > target || v > target - suffix[i + 1]
                        ? target + (target != SIZE_MAX) : suffix[i + 1] + v;
        /* When target is SIZE_MAX, saturate to SIZE_MAX instead of wrapping. */
        if (suffix[i] < suffix[i + 1]) suffix[i] = SIZE_MAX;
    }
    SubsetContext ctx = {values, n, target, 0, 0, suffix, path, NULL, ops};
    int status = subsets_search(&ctx, 0, 0, 0);
    free(suffix); free(path);
    if (status != SD_OK) { subsets_free(ctx.solutions, ctx.count); return status; }
    *solutions = ctx.solutions;
    *solution_count = ctx.count;
    return SD_OK;
}

/* Exact backtracking uses the same original input order as the DP oracle. */
typedef struct {
    const int *weights;
    const long long *values;
    size_t n, capacity, best_count;
    long long best;
    unsigned char *current, *selected;
    uint64_t *ops;
} BacktrackContext;
static int knapsack_search(BacktrackContext *ctx, size_t at, size_t used, long long value) {
    if (at == ctx->n) {
        if (value > ctx->best) {
            ctx->best = value;
            memcpy(ctx->selected, ctx->current, ctx->n);
        }
        return SD_OK;
    }
    count_op(ctx->ops);
    ctx->current[at] = 0;
    int status = knapsack_search(ctx, at + 1, used, value);
    if (status != SD_OK) return status;
    if ((size_t)ctx->weights[at] <= ctx->capacity - used) {
        if (value > LLONG_MAX - ctx->values[at]) return SD_ERR_OVERFLOW;
        ctx->current[at] = 1;
        status = knapsack_search(ctx, at + 1, used + (size_t)ctx->weights[at],
                                 value + ctx->values[at]);
        ctx->current[at] = 0;
    }
    return status;
}
int knapsack_01_backtracking(const int *weights, const long long *values,
                            size_t n, size_t capacity, long long *best,
                            size_t **chosen, size_t *chosen_count, uint64_t *ops) {
    if (!best || !chosen || !chosen_count) return SD_ERR_INVALID_ARGUMENT;
    *best = 0; *chosen = NULL; *chosen_count = 0;
    int status = validate_items(weights, values, n);
    if (status != SD_OK) return status;
    unsigned char *current = checked_calloc(n ? n : 1, 1);
    unsigned char *selected = checked_calloc(n ? n : 1, 1);
    if (!current || !selected) {
        free(current); free(selected); return SD_ERR_OUT_OF_MEMORY;
    }
    BacktrackContext ctx = {weights, values, n, capacity, 0, 0, current, selected, ops};
    status = knapsack_search(&ctx, 0, 0, 0);
    if (status == SD_OK) {
        size_t count = 0;
        for (size_t i = 0; i < n; ++i) count += selected[i] != 0;
        size_t *indices = count ? checked_malloc(count * sizeof(*indices)) : NULL;
        if (count && !indices) status = SD_ERR_OUT_OF_MEMORY;
        else {
            size_t p = 0;
            for (size_t i = 0; i < n; ++i) if (selected[i]) indices[p++] = i;
            *best = ctx.best; *chosen = indices; *chosen_count = count;
        }
    }
    free(current); free(selected);
    return status;
}

/* Items are sorted once, in descending density. A zero-weight positive item
   sorts first. Original indices are retained for result reconstruction. */
typedef struct { size_t index; int weight; long long value; } BBItem;
typedef struct {
    size_t parent, level, used;
    long long value;
    long double bound;
    unsigned char taken;
} BBNode;
typedef enum { FRONT_FIFO, FRONT_LIFO, FRONT_LEAST_COST } FrontierKind;
typedef struct {
    size_t *indices;
    size_t size, capacity, head;
    FrontierKind kind;
    const BBNode *nodes;
} BBFrontier;
static int bb_item_before(const BBItem *a, const BBItem *b) {
    if (a->weight == 0 || b->weight == 0) {
        if (a->weight == 0 && b->weight != 0) return 1;
        if (a->weight != 0 && b->weight == 0) return 0;
    }
    long double left = (long double)a->value * (long double)b->weight;
    long double right = (long double)b->value * (long double)a->weight;
    if (left != right) return left > right;
    return a->index < b->index;
}
static void bb_sort_items(BBItem *items, size_t n) {
    for (size_t i = 1; i < n; ++i) {
        BBItem x = items[i]; size_t j = i;
        while (j && bb_item_before(&x, &items[j - 1])) {
            items[j] = items[j - 1]; --j;
        }
        items[j] = x;
    }
}
/* One fractional bound for all frontier policies. Using round-up makes the
   bound conservative for integral objective values. The tiny precision guard
   prevents dropping a candidate due only to floating-point roundoff. */
static long double bb_upper_bound(const BBNode *node, const BBItem *items,
                                  size_t n, size_t capacity, uint64_t *ops) {
    long double bound = (long double)node->value;
    size_t remaining = capacity - node->used;
    for (size_t i = node->level; i < n; ++i) {
        count_op(ops);
        if (items[i].weight == 0) {
            bound += (long double)items[i].value;
        } else if ((size_t)items[i].weight <= remaining) {
            remaining -= (size_t)items[i].weight;
            bound += (long double)items[i].value;
        } else {
            bound += ((long double)remaining * (long double)items[i].value)
                     / (long double)items[i].weight;
            break;
        }
    }
    return ceill(bound + 1e-9L);
}
static int bb_frontier_push(BBFrontier *f, size_t index) {
    if (f->size == f->capacity) {
        size_t next = f->capacity ? f->capacity * 2 : 16;
        if (next < f->capacity || next > SIZE_MAX / sizeof(*f->indices))
            return SD_ERR_OVERFLOW;
        void *grown = checked_realloc(f->indices, next * sizeof(*f->indices));
        if (!grown) return SD_ERR_OUT_OF_MEMORY;
        f->indices = grown; f->capacity = next;
    }
    size_t at = f->size++;
    if (f->kind == FRONT_LEAST_COST) {
        while (at) {
            size_t parent = (at - 1) / 2;
            if (f->nodes[f->indices[parent]].bound >= f->nodes[index].bound) break;
            f->indices[at] = f->indices[parent]; at = parent;
        }
    }
    f->indices[at] = index;
    return SD_OK;
}
static size_t bb_frontier_pop(BBFrontier *f) {
    if (f->kind == FRONT_FIFO) return f->indices[f->head++];
    if (f->kind == FRONT_LIFO) return f->indices[--f->size];
    size_t result = f->indices[0], last = f->indices[--f->size];
    if (f->size) {
        size_t at = 0;
        while (2 * at + 1 < f->size) {
            size_t child = 2 * at + 1;
            if (child + 1 < f->size &&
                f->nodes[f->indices[child + 1]].bound > f->nodes[f->indices[child]].bound)
                ++child;
            if (f->nodes[last].bound >= f->nodes[f->indices[child]].bound) break;
            f->indices[at] = f->indices[child]; at = child;
        }
        f->indices[at] = last;
    }
    return result;
}
static size_t bb_frontier_pending(const BBFrontier *f) {
    return f->kind == FRONT_FIFO ? f->size - f->head : f->size;
}
static int bb_append_node(BBNode **nodes, size_t *count, size_t *capacity,
                          BBNode node, size_t *index) {
    if (*count == *capacity) {
        size_t next = *capacity ? *capacity * 2 : 16;
        if (next < *capacity || next > SIZE_MAX / sizeof(**nodes))
            return SD_ERR_OVERFLOW;
        BBNode *grown = checked_realloc(*nodes, next * sizeof(*grown));
        if (!grown) return SD_ERR_OUT_OF_MEMORY;
        *nodes = grown; *capacity = next;
    }
    *index = (*count)++;
    (*nodes)[*index] = node;
    return SD_OK;
}
void bb_result_free(SDBBResult *result) {
    if (!result) return;
    free(result->chosen);
    memset(result, 0, sizeof(*result));
}
static int bb_run(const int *weights, const long long *values, size_t n,
                  size_t capacity, SDBBResult *out, uint64_t *ops,
                  FrontierKind kind) {
    if (!out) return SD_ERR_INVALID_ARGUMENT;
    memset(out, 0, sizeof(*out));
    int status = validate_items(weights, values, n);
    if (status != SD_OK) return status;
    if (n > SIZE_MAX / sizeof(BBItem)) return SD_ERR_OVERFLOW;
    BBItem *items = n ? checked_malloc(n * sizeof(*items)) : NULL;
    if (n && !items) return SD_ERR_OUT_OF_MEMORY;
    for (size_t i = 0; i < n; ++i) items[i] = (BBItem){i, weights[i], values[i]};
    bb_sort_items(items, n);
    BBNode *nodes = NULL;
    size_t node_count = 0, node_capacity = 0, best_node = 0;
    BBFrontier frontier = {NULL, 0, 0, 0, kind, NULL};
    BBNode root = {SIZE_MAX, 0, 0, 0, 0, 0};
    root.bound = bb_upper_bound(&root, items, n, capacity, ops);
    size_t root_index;
    status = bb_append_node(&nodes, &node_count, &node_capacity, root, &root_index);
    if (status != SD_OK) goto cleanup;
    frontier.nodes = nodes;
    status = bb_frontier_push(&frontier, root_index);
    if (status != SD_OK) goto cleanup;
    out->max_frontier = 1;
    while (bb_frontier_pending(&frontier)) {
        size_t current_index = bb_frontier_pop(&frontier);
        /* Copy before adding nodes: checked_realloc can move the node array. */
        BBNode current = nodes[current_index];
        if (current.level == n || current.bound < (long double)out->best_value) continue;
        ++out->nodes_expanded;
        for (int take = 1; take >= 0; --take) {
            count_op(ops);
            const BBItem *item = &items[current.level];
            if (take && (size_t)item->weight > capacity - current.used) continue;
            if (take && current.value > LLONG_MAX - item->value) {
                status = SD_ERR_OVERFLOW; goto cleanup;
            }
            BBNode child = {current_index, current.level + 1,
                            current.used + (take ? (size_t)item->weight : 0),
                            current.value + (take ? item->value : 0), 0,
                            (unsigned char)take};
            child.bound = bb_upper_bound(&child, items, n, capacity, ops);
            size_t child_index;
            status = bb_append_node(&nodes, &node_count, &node_capacity,
                                    child, &child_index);
            if (status != SD_OK) goto cleanup;
            frontier.nodes = nodes;
            if (child.value > out->best_value) {
                out->best_value = child.value;
                best_node = child_index;
            }
            if (child.level < n && child.bound >= (long double)out->best_value) {
                status = bb_frontier_push(&frontier, child_index);
                if (status != SD_OK) goto cleanup;
                size_t pending = bb_frontier_pending(&frontier);
                if (pending > out->max_frontier) out->max_frontier = pending;
            }
        }
    }
    if (node_count > UINT64_MAX) { status = SD_ERR_OVERFLOW; goto cleanup; }
    out->nodes_generated = (uint64_t)node_count;
    if (out->best_value) {
        size_t count = 0;
        for (size_t at = best_node; at != 0; at = nodes[at].parent)
            count += nodes[at].taken != 0;
        size_t *picked = count ? checked_malloc(count * sizeof(*picked)) : NULL;
        if (count && !picked) { status = SD_ERR_OUT_OF_MEMORY; goto cleanup; }
        size_t p = 0;
        for (size_t at = best_node; at != 0; at = nodes[at].parent)
            if (nodes[at].taken) picked[p++] = items[nodes[at].level - 1].index;
        /* Return indices in ascending original order, not density order. */
        for (size_t i = 1; i < count; ++i) {
            size_t x = picked[i], j = i;
            while (j && picked[j - 1] > x) { picked[j] = picked[j - 1]; --j; }
            picked[j] = x;
        }
        out->chosen = picked;
        out->chosen_count = count;
    }
cleanup:
    free(frontier.indices); free(nodes); free(items);
    if (status != SD_OK) bb_result_free(out);
    return status;
}
int knapsack_bb_fifo(const int *weights, const long long *values, size_t n,
                     size_t capacity, SDBBResult *result, uint64_t *ops) {
    return bb_run(weights, values, n, capacity, result, ops, FRONT_FIFO);
}
int knapsack_bb_lifo(const int *weights, const long long *values, size_t n,
                     size_t capacity, SDBBResult *result, uint64_t *ops) {
    return bb_run(weights, values, n, capacity, result, ops, FRONT_LIFO);
}
int knapsack_bb_least_cost(const int *weights, const long long *values,
                           size_t n, size_t capacity, SDBBResult *result,
                           uint64_t *ops) {
    return bb_run(weights, values, n, capacity, result, ops, FRONT_LEAST_COST);
}
