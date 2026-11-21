#ifndef SIGNALDESK_GRAPH_TYPES_H
#define SIGNALDESK_GRAPH_TYPES_H
#include <stddef.h>
#include "common.h"

typedef struct GraphEdge {
    size_t to;
    double weight;
    struct GraphEdge *next;
} GraphEdge;

typedef struct {
    size_t count;
    GraphEdge **heads;
} GraphList;

typedef struct {
    size_t count;
    double *weights; /* row-major; INFINITY means no edge */
} GraphMatrix;

/* On success *out owns the graph and all its edges; free using the matching
   graph_*_free function. On failure *out is NULL. Zero vertices is valid. */
int graph_list_create(size_t count, GraphList **out);
int graph_matrix_create(size_t count, GraphMatrix **out);
/* Adds an edge u->v; when undirected is nonzero adds v->u as well.
   Matrix edge insertion overwrites existing weights; list insertion adds edges.
   Weights must be finite. */
int graph_list_add(GraphList *graph, size_t u, size_t v, double weight, int undirected);
int graph_matrix_add(GraphMatrix *graph, size_t u, size_t v, double weight, int undirected);
void graph_list_free(GraphList *graph);
void graph_matrix_free(GraphMatrix *graph);
#endif
