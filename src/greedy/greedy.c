#include "greedy.h"
#include "sorting_searching.h"
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define COUNT(op) do { if (op) ++*(op); } while (0)

typedef struct { size_t id; double key; } HeapEntry;
typedef struct { HeapEntry *a; size_t n, capacity; } Heap;
static int heap_push(Heap *h, HeapEntry x, uint64_t *ops) {
    if (h->n == h->capacity) {
        size_t nc = h->capacity ? h->capacity * 2 : 16;
        if (nc < h->capacity || nc > SIZE_MAX / sizeof(*h->a)) return SD_ERR_OUT_OF_MEMORY;
        void *p = checked_realloc(h->a, nc * sizeof(*h->a));
        if (!p) return SD_ERR_OUT_OF_MEMORY;
        h->a = p; h->capacity = nc;
    }
    size_t i = h->n++;
    while (i) {
        size_t p = (i - 1) / 2;
        COUNT(ops);
        if (h->a[p].key < x.key ||
            (h->a[p].key == x.key && h->a[p].id <= x.id)) break;
        h->a[i] = h->a[p]; i = p;
    }
    h->a[i] = x;
    return SD_OK;
}
static HeapEntry heap_pop(Heap *h, uint64_t *ops) {
    HeapEntry result = h->a[0];
    HeapEntry last = h->a[--h->n];
    if (!h->n) return result;
    size_t i = 0;
    for (;;) {
        size_t left = 2 * i + 1;
        if (left >= h->n) break;
        size_t right = left + 1, best = left;
        COUNT(ops);
        if (right < h->n && (h->a[right].key < h->a[left].key ||
            (h->a[right].key == h->a[left].key && h->a[right].id < h->a[left].id))) best = right;
        COUNT(ops);
        if (last.key < h->a[best].key ||
            (last.key == h->a[best].key && last.id <= h->a[best].id)) break;
        h->a[i] = h->a[best]; i = best;
    }
    h->a[i] = last;
    return result;
}
static void heap_free(Heap *h) { free(h->a); h->a = NULL; h->n = h->capacity = 0; }

static int valid_intervals(const SDInterval *v, size_t n) {
    if (n && !v) return 0;
    for (size_t i = 0; i < n; ++i) if (v[i].end < v[i].start) return 0;
    return 1;
}
int activity_selection(const SDInterval *v, size_t n, size_t *chosen,
                       size_t *count, uint64_t *ops) {
    if (!count || (n && !chosen) || !valid_intervals(v, n) || n > INT_MAX)
        return SD_ERR_INVALID_ARGUMENT;
    *count = 0;
    if (!n) return SD_OK;
    int *finishes = checked_malloc(n * sizeof(*finishes));
    unsigned char *used = checked_calloc(n, 1);
    if (!finishes || !used) { free(finishes); free(used); return SD_ERR_OUT_OF_MEMORY; }
    for (size_t i = 0; i < n; ++i) finishes[i] = v[i].end;
    int status = merge_sort(finishes, n, ops); /* project's own merge sort */
    int last_end = INT_MIN;
    if (status == SD_OK) {
        for (size_t j = 0; j < n; ++j) {
            for (size_t i = 0; i < n; ++i) {
                COUNT(ops);
                if (!used[i] && v[i].end == finishes[j]) {
                    used[i] = 1;
                    if (v[i].start >= last_end) {
                        chosen[(*count)++] = i;
                        last_end = v[i].end;
                    }
                    break;
                }
            }
        }
    }
    free(used); free(finishes); return status;
}

typedef struct { size_t id; int start, end; } OrderedInterval;
static void merge_intervals(OrderedInterval *v, OrderedInterval *tmp, size_t l, size_t r, uint64_t *ops) {
    if (r - l < 2) return;
    size_t m = l + (r - l) / 2;
    merge_intervals(v, tmp, l, m, ops); merge_intervals(v, tmp, m, r, ops);
    size_t i = l, j = m;
    for (size_t k = l; k < r; ++k) {
        COUNT(ops);
        if (j == r || (i < m && (v[i].start < v[j].start ||
            (v[i].start == v[j].start && v[i].end <= v[j].end)))) tmp[k] = v[i++];
        else tmp[k] = v[j++];
    }
    memcpy(v + l, tmp + l, (r-l) * sizeof(*v));
}
int interval_partitioning(const SDInterval *v, size_t n, size_t *assigned,
                          size_t *desk_count, uint64_t *ops) {
    if (!desk_count || (n && !assigned) || !valid_intervals(v, n) ||
        n > SIZE_MAX / sizeof(OrderedInterval)) return SD_ERR_INVALID_ARGUMENT;
    *desk_count = 0;
    if (!n) return SD_OK;
    OrderedInterval *sorted = checked_malloc(n * sizeof(*sorted));
    OrderedInterval *tmp = checked_malloc(n * sizeof(*tmp));
    if (!sorted || !tmp) { free(sorted); free(tmp); return SD_ERR_OUT_OF_MEMORY; }
    for (size_t i = 0; i < n; ++i) {
        sorted[i] = (OrderedInterval){i, v[i].start, v[i].end};
        assigned[i] = SIZE_MAX;
    }
    merge_intervals(sorted, tmp, 0, n, ops);
    Heap busy = {0}, available = {0};
    int status = SD_OK;
    for (size_t i = 0; i < n; ++i) {
        OrderedInterval it = sorted[i];
        while (busy.n && busy.a[0].key <= (double)it.start) {
            HeapEntry freed = heap_pop(&busy, ops);
            status = heap_push(&available, (HeapEntry){freed.id, (double)freed.id}, ops);
            if (status) goto done;
        }
        if (it.start == it.end) continue;
        size_t desk = available.n ? heap_pop(&available, ops).id : (*desk_count)++;
        assigned[it.id] = desk;
        status = heap_push(&busy, (HeapEntry){desk, (double)it.end}, ops);
        if (status) goto done;
    }
done:
    heap_free(&busy); heap_free(&available); free(tmp); free(sorted);
    return status;
}

static int item_before(const SDItem *a, size_t ia, const SDItem *b, size_t ib) {
    if (a->weight == 0 || b->weight == 0) {
        if (a->weight == 0 && b->weight == 0) return ia < ib;
        return a->weight == 0;
    }
    long double va = (long double)a->value / (long double)a->weight;
    long double vb = (long double)b->value / (long double)b->weight;
    return va > vb || (va == vb && ia < ib);
}
static void merge_items(const SDItem *items, size_t *ids, size_t *temp,
                        size_t l, size_t r, uint64_t *ops) {
    if (r-l < 2) return;
    size_t m = l + (r-l)/2, i=l, j=m;
    merge_items(items,ids,temp,l,m,ops); merge_items(items,ids,temp,m,r,ops);
    for (size_t k=l;k<r;++k) {
        COUNT(ops);
        if (j==r || (i<m && item_before(&items[ids[i]],ids[i],&items[ids[j]],ids[j]))) temp[k]=ids[i++];
        else temp[k]=ids[j++];
    }
    memcpy(ids+l,temp+l,(r-l)*sizeof(*ids));
}
int fractional_knapsack(const SDItem *items, size_t n, double capacity,
                        double *fractions, double *total, uint64_t *ops) {
    if (!total || (n && (!items || !fractions)) || !isfinite(capacity) || capacity<0 ||
        n > SIZE_MAX / sizeof(size_t)) return SD_ERR_INVALID_ARGUMENT;
    for (size_t i=0;i<n;++i)
        if (!isfinite(items[i].weight) || !isfinite(items[i].value) ||
            items[i].weight<0 || items[i].value<0) return SD_ERR_INVALID_ARGUMENT;
    *total=0;
    for (size_t i=0;i<n;++i) fractions[i]=0;
    if (!n) return SD_OK;
    size_t *ids=checked_malloc(n*sizeof(*ids)), *tmp=checked_malloc(n*sizeof(*tmp));
    if (!ids || !tmp) { free(ids);free(tmp);return SD_ERR_OUT_OF_MEMORY; }
    for (size_t i=0;i<n;++i) ids[i]=i;
    merge_items(items,ids,tmp,0,n,ops);
    long double sum=0, left=capacity;
    for (size_t k=0;k<n;++k) {
        size_t i=ids[k]; COUNT(ops);
        if (!items[i].value) continue;
        if (!items[i].weight) fractions[i]=1;
        else if (left>0) {
            long double frac = left >= items[i].weight ? 1 : left / items[i].weight;
            fractions[i]=(double)frac;
            left -= frac*items[i].weight;
        }
        sum+=(long double)fractions[i]*items[i].value;
    }
    free(ids);free(tmp);
    if (sum>DBL_MAX) return SD_ERR_OVERFLOW;
    *total=(double)sum; return SD_OK;
}

int dijkstra(const GraphList *g, size_t source, double *dist, size_t *pred, uint64_t *ops) {
    if (!g || source>=g->count || !dist || !pred || (g->count && !g->heads))
        return SD_ERR_INVALID_ARGUMENT;
    for (size_t i=0;i<g->count;++i)
        for (const GraphEdge *e=g->heads[i];e;e=e->next) {
            if (e->to>=g->count || !isfinite(e->weight)) return SD_ERR_INVALID_ARGUMENT;
            if (e->weight<0) return SD_ERR_NEGATIVE_WEIGHT;
        }
    Heap heap={0};
    for (size_t i=0;i<g->count;++i) { dist[i]=INFINITY;pred[i]=SIZE_MAX; }
    dist[source]=0;
    int status=heap_push(&heap,(HeapEntry){source,0},ops);
    while (!status && heap.n) {
        HeapEntry cur=heap_pop(&heap,ops);
        if (cur.key!=dist[cur.id]) continue;
        for (const GraphEdge *e=g->heads[cur.id];e;e=e->next) {
            COUNT(ops);
            double alt=cur.key+e->weight;
            if (!isfinite(alt)) { status=SD_ERR_OVERFLOW;break; }
            if (alt<dist[e->to]) {
                dist[e->to]=alt;pred[e->to]=cur.id;
                status=heap_push(&heap,(HeapEntry){e->to,alt},ops);
                if (status) break;
            }
        }
    }
    heap_free(&heap);return status;
}

static int valid_undirected(const GraphMatrix *g) {
    if (!g || (g->count && !g->weights)) return 0;
    for (size_t i=0;i<g->count;++i)
        for (size_t j=i+1;j<g->count;++j) {
            double a=g->weights[i*g->count+j], b=g->weights[j*g->count+i];
            if (isnan(a) || isnan(b) || a!=b) return 0;
        }
    return 1;
}
int prim(const GraphMatrix *g, SDTreeEdge *edges, size_t *edge_count,
         double *weight, uint64_t *ops) {
    if (!valid_undirected(g) || !edge_count || !weight || (g->count>1 && !edges) ||
        g->count>SIZE_MAX/sizeof(double)) return SD_ERR_INVALID_ARGUMENT;
    *edge_count=0;*weight=0;
    size_t n=g->count;
    if (!n) return SD_OK;
    double *best=checked_malloc(n*sizeof(*best));
    size_t *parent=checked_malloc(n*sizeof(*parent));
    unsigned char *seen=checked_calloc(n,1);
    if (!best || !parent || !seen) {free(best);free(parent);free(seen);return SD_ERR_OUT_OF_MEMORY;}
    for (size_t i=0;i<n;++i) {best[i]=INFINITY;parent[i]=SIZE_MAX;}
    best[0]=0;
    int status=SD_OK;
    for (size_t step=0;step<n;++step) {
        size_t u=SIZE_MAX;
        for (size_t i=0;i<n;++i) { COUNT(ops);if (!seen[i] && (u==SIZE_MAX || best[i]<best[u])) u=i; }
        if (u==SIZE_MAX || !isfinite(best[u])) {status=SD_ERR_INFEASIBLE;break;}
        seen[u]=1;
        if (parent[u]!=SIZE_MAX) {
            edges[*edge_count]=(SDTreeEdge){parent[u],u,best[u]}; ++*edge_count;
            *weight+=best[u];
            if (!isfinite(*weight)) {status=SD_ERR_OVERFLOW;break;}
        }
        for (size_t v=0;v<n;++v) {
            double w=g->weights[u*n+v]; COUNT(ops);
            if (!seen[v] && w<best[v]) {best[v]=w;parent[v]=u;}
        }
    }
    free(best);free(parent);free(seen);return status;
}

typedef struct {size_t u,v;double w;} Candidate;
static void merge_candidates(Candidate *a,Candidate *tmp,size_t l,size_t r,uint64_t *ops) {
    if (r-l<2) return;
    size_t m=l+(r-l)/2,i=l,j=m;
    merge_candidates(a,tmp,l,m,ops);merge_candidates(a,tmp,m,r,ops);
    for (size_t k=l;k<r;++k) {
        COUNT(ops);
        if (j==r || (i<m && (a[i].w<a[j].w || (a[i].w==a[j].w &&
            (a[i].u<a[j].u || (a[i].u==a[j].u && a[i].v<=a[j].v)))))) tmp[k]=a[i++];
        else tmp[k]=a[j++];
    }
    memcpy(a+l,tmp+l,(r-l)*sizeof(*a));
}
static size_t uf_root(size_t *parent,size_t i,uint64_t *ops) {
    while (parent[i]!=i) { COUNT(ops);parent[i]=parent[parent[i]];i=parent[i]; }
    return i;
}
int kruskal(const GraphMatrix *g, SDTreeEdge *edges, size_t *edge_count,
            double *weight, uint64_t *ops) {
    if (!valid_undirected(g) || !edge_count || !weight || (g->count>1 && !edges))
        return SD_ERR_INVALID_ARGUMENT;
    *edge_count=0;*weight=0;
    size_t n=g->count;
    if (n<2) return SD_OK;
    if (n>SIZE_MAX/(n-1) || n*(n-1)/2>SIZE_MAX/sizeof(Candidate) ||
        n>SIZE_MAX/sizeof(size_t)) return SD_ERR_INVALID_ARGUMENT;
    size_t capacity=n*(n-1)/2, count=0;
    Candidate *a=checked_malloc(capacity*sizeof(*a)),*tmp=checked_malloc(capacity*sizeof(*tmp));
    size_t *parent=checked_malloc(n*sizeof(*parent));
    unsigned char *rank=checked_calloc(n,1);
    if (!a || !tmp || !parent || !rank) {
        free(a);free(tmp);free(parent);free(rank);return SD_ERR_OUT_OF_MEMORY;
    }
    for (size_t u=0;u<n;++u) {
        parent[u]=u;
        for (size_t v=u+1;v<n;++v) {
            double w=g->weights[u*n+v];
            if (isfinite(w)) a[count++]=(Candidate){u,v,w};
        }
    }
    merge_candidates(a,tmp,0,count,ops);
    int status=SD_OK;
    for (size_t k=0;k<count && *edge_count<n-1;++k) {
        size_t u=uf_root(parent,a[k].u,ops),v=uf_root(parent,a[k].v,ops);
        if (u==v) continue;
        if (rank[u]<rank[v]) {size_t t=u;u=v;v=t;}
        parent[v]=u;
        if (rank[u]==rank[v]) ++rank[u];
        edges[*edge_count]=(SDTreeEdge){a[k].u,a[k].v,a[k].w};++*edge_count;
        *weight+=a[k].w; COUNT(ops);
        if (!isfinite(*weight)) {status=SD_ERR_OVERFLOW;break;}
    }
    if (status==SD_OK && *edge_count!=n-1) status=SD_ERR_INFEASIBLE;
    free(a);free(tmp);free(parent);free(rank);return status;
}
