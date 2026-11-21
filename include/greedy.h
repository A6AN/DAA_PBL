#ifndef SIGNALDESK_GREEDY_H
#define SIGNALDESK_GREEDY_H
#include <stddef.h>
#include <stdint.h>
#include "common.h"
#include "graph_types.h"

typedef struct { int start, end; } SDInterval;
typedef struct { double weight, value; } SDItem;
typedef struct { size_t from, to; double weight; } SDTreeEdge;

/* Intervals are half-open [start,end), with start <= end. Output arrays are
   supplied by the caller and have space for n entries. The count is returned
   through selected_count; selected_indices are in selection order. Empty
   intervals are valid but are treated as zero-duration tasks. */
int activity_selection(const SDInterval *intervals, size_t n,
                       size_t *selected_indices, size_t *selected_count,
                       uint64_t *ops);
/* Desk identifiers are [0,desk_count). An empty interval consumes no time;
   its assignment is SIZE_MAX. Caller owns assignment[n]. */
int interval_partitioning(const SDInterval *intervals, size_t n,
                          size_t *assignment, size_t *desk_count, uint64_t *ops);
/* Items need nonnegative finite weights and values; zero-weight positive-value
   items are taken fully. capacity is finite and nonnegative. Caller supplies
   fractions[n], one entry per original item. */
int fractional_knapsack(const SDItem *items, size_t n, double capacity,
                        double *fractions, double *total_value, uint64_t *ops);

typedef struct {
    unsigned char *bytes; /* allocated packed bits; owned by this object */
    size_t byte_count, bit_count, original_size;
    unsigned char code_lengths[256];
    unsigned char code_bits[256][256]; /* each entry is 0 or 1 */
} SDHuffman;
/* On success *out owns bytes and must be released with huffman_free.
   Decode allocates *decoded (even for empty input), caller frees it.
   Compression ratio is payload bytes / original bytes; empty input yields 0.
   Serialized codebook overhead is NOT included in this ratio. */
int huffman_encode(const unsigned char *input, size_t n, SDHuffman **out,
                   double *compression_ratio, uint64_t *ops);
int huffman_decode(const SDHuffman *encoded, unsigned char **decoded,
                   size_t *decoded_size, uint64_t *ops);
void huffman_free(SDHuffman *encoded);

/* List edges must have finite nonnegative weights. Caller supplies distance[n]
   and predecessor[n]; unreachable distances are INFINITY and predecessors
   SIZE_MAX. */
int dijkstra(const GraphList *graph, size_t source, double *distance,
             size_t *predecessor, uint64_t *ops);
/* Undirected weighted matrix (symmetric finite entries; missing edges INFINITY).
   Caller provides edge array with n>0 ? n-1 slots : 0. On disconnected
   graphs returns SD_ERR_INFEASIBLE and sets edge_count to a partial forest
   size; total_weight is that forest's weight. */
int prim(const GraphMatrix *graph, SDTreeEdge *edges, size_t *edge_count,
         double *total_weight, uint64_t *ops);
int kruskal(const GraphMatrix *graph, SDTreeEdge *edges, size_t *edge_count,
            double *total_weight, uint64_t *ops);
#endif
