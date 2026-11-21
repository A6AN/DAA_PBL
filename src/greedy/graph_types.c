#include "graph_types.h"
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

int graph_list_create(size_t count, GraphList **out) {
    if (!out || count > SIZE_MAX / sizeof(GraphEdge *)) return SD_ERR_INVALID_ARGUMENT;
    *out = NULL;
    GraphList *g = checked_calloc(1, sizeof(*g));
    if (!g) return SD_ERR_OUT_OF_MEMORY;
    g->heads = checked_calloc(count, sizeof(*g->heads));
    if (!g->heads) { free(g); return SD_ERR_OUT_OF_MEMORY; }
    g->count = count; *out = g; return SD_OK;
}
int graph_matrix_create(size_t count, GraphMatrix **out) {
    if (!out || (count && count > SIZE_MAX / count / sizeof(double)))
        return SD_ERR_INVALID_ARGUMENT;
    *out = NULL;
    GraphMatrix *g = checked_calloc(1, sizeof(*g));
    if (!g) return SD_ERR_OUT_OF_MEMORY;
    g->weights = checked_malloc((count ? count * count : 1) * sizeof(double));
    if (!g->weights) { free(g); return SD_ERR_OUT_OF_MEMORY; }
    for (size_t i = 0; i < count * count; ++i) g->weights[i] = INFINITY;
    for (size_t i = 0; i < count; ++i) g->weights[i * count + i] = 0;
    g->count = count; *out = g; return SD_OK;
}
int graph_list_add(GraphList *g, size_t u, size_t v, double w, int undirected) {
    if (!g || u >= g->count || v >= g->count || !isfinite(w)) return SD_ERR_INVALID_ARGUMENT;
    GraphEdge *a = checked_malloc(sizeof(*a));
    if (!a) return SD_ERR_OUT_OF_MEMORY;
    GraphEdge *b = NULL;
    if (undirected && u != v) {
        b = checked_malloc(sizeof(*b));
        if (!b) { free(a); return SD_ERR_OUT_OF_MEMORY; }
    }
    a->to = v; a->weight = w; a->next = g->heads[u]; g->heads[u] = a;
    if (b) { b->to = u; b->weight = w; b->next = g->heads[v]; g->heads[v] = b; }
    return SD_OK;
}
int graph_matrix_add(GraphMatrix *g, size_t u, size_t v, double w, int undirected) {
    if (!g || u >= g->count || v >= g->count || !isfinite(w)) return SD_ERR_INVALID_ARGUMENT;
    g->weights[u * g->count + v] = w;
    if (undirected) g->weights[v * g->count + u] = w;
    return SD_OK;
}
void graph_list_free(GraphList *g) {
    if (!g) return;
    for (size_t u = 0; u < g->count; ++u) {
        GraphEdge *e = g->heads[u];
        while (e) { GraphEdge *next = e->next; free(e); e = next; }
    }
    free(g->heads); free(g);
}
void graph_matrix_free(GraphMatrix *g) {
    if (!g) return;
    free(g->weights); free(g);
}
