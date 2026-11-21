#include "graph_algorithms.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>

static int path_dfs(size_t u, size_t sink, size_t n, const int64_t *residual,
                    unsigned char *seen, size_t *parent, uint64_t *ops) {
    if (u == sink) return 1;
    seen[u] = 1;
    for (size_t v = 0; v < n; ++v) {
        if (ops) ++*ops;
        if (!seen[v] && residual[u*n+v] > 0) {
            parent[v] = u;
            if (path_dfs(v, sink, n, residual, seen, parent, ops)) return 1;
        }
    }
    return 0;
}

int ford_fulkerson(const int64_t *capacities, size_t n, size_t source,
                   size_t sink, int64_t *max_flow, int64_t **edge_flow,
                   uint64_t *ops) {
    if (!max_flow || !edge_flow || !capacities || !n || source >= n || sink >= n ||
        source == sink || n > SIZE_MAX/n || n*n > SIZE_MAX/sizeof(int64_t))
        return SD_ERR_INVALID_ARGUMENT;
    *max_flow = 0; *edge_flow = NULL;
    size_t cells = n*n;
    for (size_t i = 0; i < cells; ++i)
        if (capacities[i] < 0) return SD_ERR_NEGATIVE_WEIGHT;
    int64_t *r = checked_malloc(cells*sizeof(*r));
    int64_t *flow = checked_calloc(cells,sizeof(*flow));
    unsigned char *seen = checked_malloc(n);
    size_t *parent = checked_malloc(n*sizeof(*parent));
    if (!r || !flow || !seen || !parent) {
        free(r); free(flow); free(seen); free(parent);
        return SD_ERR_OUT_OF_MEMORY;
    }
    memcpy(r, capacities, cells*sizeof(*r));
    int result = SD_OK;
    /* Residual reverse capacities may sum opposite-direction capacities. */
    while (1) {
        memset(seen, 0, n);
        for (size_t i = 0; i < n; ++i) parent[i] = SIZE_MAX;
        if (!path_dfs(source, sink, n, r, seen, parent, ops)) break;
        int64_t add = INT64_MAX;
        for (size_t v = sink; v != source; v = parent[v]) {
            size_t u = parent[v];
            if (r[u*n+v] < add) add = r[u*n+v];
        }
        if (*max_flow > INT64_MAX-add) { result = SD_ERR_OVERFLOW; break; }
        /* A residual reverse update can overflow if opposite arcs are large.
           In that case the maximum flow cannot safely be represented here. */
        for (size_t v = sink; v != source; v = parent[v]) {
            size_t u = parent[v];
            if (r[v*n+u] > INT64_MAX-add) { result = SD_ERR_OVERFLOW; break; }
        }
        if (result != SD_OK) break;
        for (size_t v = sink; v != source; v = parent[v]) {
            size_t u = parent[v];
            r[u*n+v] -= add;
            r[v*n+u] += add;
        }
        *max_flow += add;
    }
    if (result == SD_OK) {
        /* Convert signed net flow to a feasible nonnegative flow on arcs.
           Cancel opposing flow rather than reporting negative arc values. */
        for (size_t u = 0; u < n; ++u) for (size_t v = 0; v < n; ++v) {
            if (u == v) continue;
            /* Net flow equals original capacity minus residual capacity,
               computed without overflowing a signed subtraction. */
            if (capacities[u*n+v] > r[u*n+v])
                flow[u*n+v] = capacities[u*n+v] - r[u*n+v];
        }
        *edge_flow = flow;
    } else { free(flow); *max_flow = 0; }
    free(r); free(seen); free(parent);
    return result;
}

static int augment(size_t u, size_t nr, const unsigned char *compat,
                   unsigned char *seen, size_t *right, uint64_t *ops) {
    for (size_t v = 0; v < nr; ++v) {
        if (ops) ++*ops;
        if (compat[u*nr+v] && !seen[v]) {
            seen[v] = 1;
            if (right[v] == SIZE_MAX || augment(right[v], nr, compat, seen, right, ops)) {
                right[v] = u;
                return 1;
            }
        }
    }
    return 0;
}

int max_bipartite_matching(const unsigned char *compatibility,
                           size_t left_count, size_t right_count,
                           size_t *matching_size, size_t **left_match,
                           size_t **right_match, uint64_t *ops) {
    if (!matching_size || !left_match || !right_match ||
        (left_count && right_count && !compatibility) ||
        (right_count && left_count > SIZE_MAX/right_count) ||
        left_count > SIZE_MAX/sizeof(size_t) ||
        right_count > SIZE_MAX/sizeof(size_t)) return SD_ERR_INVALID_ARGUMENT;
    *matching_size = 0; *left_match = NULL; *right_match = NULL;
    size_t *left = left_count ? checked_malloc(left_count*sizeof(*left)) : NULL;
    size_t *right = right_count ? checked_malloc(right_count*sizeof(*right)) : NULL;
    unsigned char *seen = right_count ? checked_malloc(right_count) : NULL;
    if ((left_count && !left) || (right_count && (!right || !seen))) {
        free(left); free(right); free(seen); return SD_ERR_OUT_OF_MEMORY;
    }
    for (size_t i=0;i<left_count;++i) left[i]=SIZE_MAX;
    for (size_t j=0;j<right_count;++j) right[j]=SIZE_MAX;
    for (size_t i=0;i<left_count && right_count;++i) {
        memset(seen,0,right_count);
        if (augment(i,right_count,compatibility,seen,right,ops)) ++*matching_size;
    }
    for (size_t j=0;j<right_count;++j)
        if (right[j]!=SIZE_MAX) left[right[j]]=j;
    *left_match=left; *right_match=right;
    free(seen);
    return SD_OK;
}
