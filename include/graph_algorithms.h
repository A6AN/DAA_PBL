#ifndef SIGNALDESK_GRAPH_ALGORITHMS_H
#define SIGNALDESK_GRAPH_ALGORITHMS_H
#include <stddef.h>
#include <stdint.h>
#include "common.h"

/* Matrix entries are row-major integer capacities. On success, *edge_flow
   owns an n*n array, to be freed by caller. For an original arc u->v,
   edge_flow[u*n+v] is its NONNEGATIVE assigned flow. All capacities >= 0.
   n=0 and source==sink are invalid. Parallel arcs are represented by a
   single aggregated matrix entry. ops is optional and may be NULL. */
int ford_fulkerson(const int64_t *capacities, size_t n, size_t source,
                   size_t sink, int64_t *max_flow, int64_t **edge_flow,
                   uint64_t *ops);

/* Compatibility is left_count x right_count row-major, nonzero = allowed.
   *left_match (left_count) and *right_match (right_count) belong to caller
   and must be freed. SIZE_MAX denotes unmatched. Empty sides are allowed. */
int max_bipartite_matching(const unsigned char *compatibility,
                           size_t left_count, size_t right_count,
                           size_t *matching_size, size_t **left_match,
                           size_t **right_match, uint64_t *ops);
#endif
