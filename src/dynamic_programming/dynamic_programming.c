#include "dynamic_programming.h"
#include <ctype.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int product_size(size_t a, size_t b, size_t *result) {
    if (b && a > SIZE_MAX / b) return SD_ERR_OVERFLOW;
    *result = a * b;
    return SD_OK;
}
static void tick(uint64_t *ops) { if (ops && *ops != UINT64_MAX) ++*ops; }

int knapsack_01_dp(const int *weights, const long long *values, size_t n,
                   size_t capacity, long long *best, size_t **chosen,
                   size_t *chosen_count, uint64_t *ops) {
    if (!best || !chosen || !chosen_count || (n && (!weights || !values))) return SD_ERR_INVALID_ARGUMENT;
    *best = 0; *chosen = NULL; *chosen_count = 0;
    for (size_t i=0;i<n;i++) if (weights[i]<0 || values[i]<0) return SD_ERR_INVALID_ARGUMENT;
    if (capacity==SIZE_MAX || n==SIZE_MAX) return SD_ERR_OVERFLOW;
    size_t cols=capacity+1, cells;
    if (product_size(n+1,cols,&cells)!=SD_OK) return SD_ERR_OVERFLOW;
    long long *dp=checked_calloc(cells,sizeof(*dp));
    if (!dp) return SD_ERR_OUT_OF_MEMORY;
    for (size_t i=1;i<=n;i++) for (size_t c=0;c<=capacity;c++) {
        long long v=dp[(i-1)*cols+c];
        if ((size_t)weights[i-1]<=c) {
            long long old=dp[(i-1)*cols+c-(size_t)weights[i-1]];
            if (values[i-1]>LLONG_MAX-old) { free(dp); return SD_ERR_OVERFLOW; }
            long long candidate=old+values[i-1];
            if (candidate>v) v=candidate;
        }
        dp[i*cols+c]=v; tick(ops);
    }
    *best=dp[n*cols+capacity];
    size_t count=0, c=capacity;
    for (size_t i=n;i>0;i--) if (dp[i*cols+c]!=dp[(i-1)*cols+c]) {
        count++; c-=(size_t)weights[i-1];
    }
    size_t *picked=count?checked_malloc(count*sizeof(*picked)):NULL;
    if (count && !picked) { free(dp); *best=0; return SD_ERR_OUT_OF_MEMORY; }
    c=capacity; size_t at=count;
    for (size_t i=n;i>0;i--) if (dp[i*cols+c]!=dp[(i-1)*cols+c]) {
        picked[--at]=i-1; c-=(size_t)weights[i-1];
    }
    free(dp); *chosen=picked; *chosen_count=count; return SD_OK;
}

static int append_parentheses(char *dst, size_t limit, size_t *at,
                              const size_t *split, size_t n, size_t i, size_t j) {
    if (i==j) {
        int size=snprintf(dst+*at, limit-*at, "A%zu", i+1);
        if (size<0 || (size_t)size>=limit-*at) return SD_ERR_OVERFLOW;
        *at+=(size_t)size; return SD_OK;
    }
    if (*at+1>=limit) return SD_ERR_OVERFLOW;
    dst[(*at)++]='(';
    size_t k=split[i*n+j];
    int status=append_parentheses(dst,limit,at,split,n,i,k);
    if (status!=SD_OK) return status;
    status=append_parentheses(dst,limit,at,split,n,k+1,j);
    if (status!=SD_OK || *at+1>=limit) return SD_ERR_OVERFLOW;
    dst[(*at)++]=')'; dst[*at]='\0'; return SD_OK;
}
int matrix_chain_order(const size_t *dims, size_t n, uint64_t *cost,
                       char **parenthesization, uint64_t *ops) {
    if (!cost || !parenthesization || (n && !dims)) return SD_ERR_INVALID_ARGUMENT;
    *cost=0; *parenthesization=NULL;
    for (size_t i=0;i<=n && dims;i++) if (dims[i]==0) return SD_ERR_INVALID_ARGUMENT;
    if (n==0) { *parenthesization=checked_calloc(1,1); return *parenthesization?SD_OK:SD_ERR_OUT_OF_MEMORY; }
    size_t cells;
    if (product_size(n,n,&cells)!=SD_OK || cells>SIZE_MAX/sizeof(uint64_t)) return SD_ERR_OVERFLOW;
    uint64_t *dp=checked_calloc(cells,sizeof(*dp));
    size_t *split=checked_calloc(cells,sizeof(*split));
    if (!dp || !split) { free(dp); free(split); return SD_ERR_OUT_OF_MEMORY; }
    for (size_t span=2;span<=n;span++) for (size_t i=0;i+span<=n;i++) {
        size_t j=i+span-1; uint64_t best=UINT64_MAX;
        for (size_t k=i;k<j;k++) {
            tick(ops);
            uint64_t a=(uint64_t)dims[i], b=(uint64_t)dims[k+1], c=(uint64_t)dims[j+1];
            if ((b && a>UINT64_MAX/b) || (c && a*b>UINT64_MAX/c)) continue;
            uint64_t mul=a*b*c, left=dp[i*n+k], right=dp[(k+1)*n+j];
            if (left==UINT64_MAX || right==UINT64_MAX || left>UINT64_MAX-right || left+right>UINT64_MAX-mul) continue;
            uint64_t candidate=left+right+mul;
            if (candidate<best) { best=candidate; split[i*n+j]=k; }
        }
        dp[i*n+j]=best;
    }
    if (dp[n-1]==UINT64_MAX) { free(dp); free(split); return SD_ERR_OVERFLOW; }
    if (n>(SIZE_MAX-32)/32) { free(dp); free(split); return SD_ERR_OVERFLOW; }
    size_t limit=32*n+32, at=0;
    char *repr=checked_calloc(limit,1);
    if (!repr) { free(dp); free(split); return SD_ERR_OUT_OF_MEMORY; }
    int status=append_parentheses(repr,limit,&at,split,n,0,n-1);
    if (status==SD_OK) { *cost=dp[n-1]; *parenthesization=repr; }
    else free(repr);
    free(dp); free(split); return status;
}

void free_tokens(char **tokens,size_t count) {
    if (!tokens) return;
    for (size_t i=0;i<count;i++) free(tokens[i]);
    free(tokens);
}
int tokenize_text(const char *text,char ***tokens,size_t *count) {
    if (!text || !tokens || !count) return SD_ERR_INVALID_ARGUMENT;
    *tokens=NULL; *count=0; size_t used=0,cap=0; char **arr=NULL;
    const unsigned char *p=(const unsigned char *)text;
    while (*p) {
        while (*p && !((*p>='a'&&*p<='z')||(*p>='A'&&*p<='Z')||(*p>='0'&&*p<='9'))) p++;
        if (!*p) break;
        const unsigned char *start=p;
        while ((*p>='a'&&*p<='z')||(*p>='A'&&*p<='Z')||(*p>='0'&&*p<='9')) p++;
        size_t len=(size_t)(p-start);
        if (len==SIZE_MAX) { free_tokens(arr,used); return SD_ERR_OVERFLOW; }
        char *word=checked_malloc(len+1);
        if (!word) { free_tokens(arr,used); return SD_ERR_OUT_OF_MEMORY; }
        for (size_t i=0;i<len;i++) word[i]=(char)tolower(start[i]);
        word[len]='\0';
        if (used==cap) {
            size_t nc=cap?cap*2:8;
            if (nc<cap || nc>SIZE_MAX/sizeof(*arr)) { free(word); free_tokens(arr,used); return SD_ERR_OVERFLOW; }
            char **tmp=checked_realloc(arr,nc*sizeof(*arr));
            if (!tmp) { free(word); free_tokens(arr,used); return SD_ERR_OUT_OF_MEMORY; }
            arr=tmp; cap=nc;
        }
        arr[used++]=word;
    }
    *tokens=arr; *count=used; return SD_OK;
}
int lcs_tokens(const char *const *a,size_t na,const char *const *b,size_t nb,
               size_t *length,size_t **indices_a,size_t **indices_b,
               double *similarity,uint64_t *ops) {
    if (!length || !indices_a || !indices_b || !similarity || (na&&!a) || (nb&&!b)) return SD_ERR_INVALID_ARGUMENT;
    *length=0; *indices_a=NULL; *indices_b=NULL; *similarity=0;
    for (size_t i=0;i<na;i++) if (!a[i]) return SD_ERR_INVALID_ARGUMENT;
    for (size_t i=0;i<nb;i++) if (!b[i]) return SD_ERR_INVALID_ARGUMENT;
    if (na>SIZE_MAX-nb || na==SIZE_MAX || nb==SIZE_MAX) return SD_ERR_OVERFLOW;
    if (na==0 || nb==0) { *similarity=na+nb==0?1.0:0.0; return SD_OK; }
    size_t cells;
    if (product_size(na+1,nb+1,&cells)!=SD_OK || cells>SIZE_MAX/sizeof(size_t)) return SD_ERR_OVERFLOW;
    size_t *dp=checked_calloc(cells,sizeof(*dp));
    if (!dp) return SD_ERR_OUT_OF_MEMORY;
    size_t cols=nb+1;
    for (size_t i=1;i<=na;i++) for (size_t j=1;j<=nb;j++) {
        tick(ops);
        dp[i*cols+j]=strcmp(a[i-1],b[j-1])==0 ? dp[(i-1)*cols+j-1]+1 :
            (dp[(i-1)*cols+j]>=dp[i*cols+j-1]?dp[(i-1)*cols+j]:dp[i*cols+j-1]);
    }
    size_t len=dp[na*cols+nb];
    size_t *ia=len?checked_malloc(len*sizeof(*ia)):NULL;
    size_t *ib=len?checked_malloc(len*sizeof(*ib)):NULL;
    if (len && (!ia || !ib)) { free(ia); free(ib); free(dp); return SD_ERR_OUT_OF_MEMORY; }
    size_t i=na,j=nb,k=len;
    while (i && j) {
        if (strcmp(a[i-1],b[j-1])==0) { --k; ia[k]=i-1; ib[k]=j-1; i--; j--; }
        else if (dp[(i-1)*cols+j]>=dp[i*cols+j-1]) --i;
        else --j;
    }
    free(dp); *length=len; *indices_a=ia; *indices_b=ib;
    *similarity=2.0*(double)len/((double)na+(double)nb); return SD_OK;
}

int optimal_bst(const double *frequencies,size_t n,double *expected_cost,
                size_t **roots,uint64_t *ops) {
    if (!expected_cost || !roots || (n&&!frequencies)) return SD_ERR_INVALID_ARGUMENT;
    *expected_cost=0; *roots=NULL;
    if (!n) return SD_OK;
    size_t cells;
    if (product_size(n,n,&cells)!=SD_OK || cells>SIZE_MAX/sizeof(double) || cells>SIZE_MAX/sizeof(size_t)) return SD_ERR_OVERFLOW;
    double *dp=checked_calloc(cells,sizeof(*dp));
    size_t *rt=checked_malloc(cells*sizeof(*rt));
    double *prefix=checked_calloc(n+1,sizeof(*prefix));
    if (!dp || !rt || !prefix) { free(dp); free(rt); free(prefix); return SD_ERR_OUT_OF_MEMORY; }
    for (size_t i=0;i<cells;i++) rt[i]=SIZE_MAX;
    for (size_t i=0;i<n;i++) {
        if (!isfinite(frequencies[i]) || frequencies[i]<0 || !isfinite(prefix[i]+frequencies[i])) {
            free(dp); free(rt); free(prefix); return SD_ERR_INVALID_ARGUMENT;
        }
        prefix[i+1]=prefix[i]+frequencies[i];
    }
    for (size_t span=1;span<=n;span++) for (size_t i=0;i+span<=n;i++) {
        size_t j=i+span-1; double total=prefix[j+1]-prefix[i], best=INFINITY;
        for (size_t k=i;k<=j;k++) {
            tick(ops);
            double candidate=total+(k>i?dp[i*n+k-1]:0)+(k<j?dp[(k+1)*n+j]:0);
            if (candidate<best) { best=candidate; rt[i*n+j]=k; }
        }
        dp[i*n+j]=best;
    }
    *expected_cost=prefix[n]>0?dp[n-1]/prefix[n]:0;
    free(dp); free(prefix); *roots=rt; return SD_OK;
}

int bellman_ford(const GraphList *graph,size_t source,double **distances,
                 size_t **predecessors,int *reachable_negative_cycle,uint64_t *ops) {
    if (!graph || !distances || !predecessors || !reachable_negative_cycle || source>=graph->count || !graph->heads) return SD_ERR_INVALID_ARGUMENT;
    *distances=NULL; *predecessors=NULL; *reachable_negative_cycle=0;
    size_t n=graph->count;
    double *dist=checked_malloc(n*sizeof(*dist));
    size_t *pred=checked_malloc(n*sizeof(*pred));
    if (!dist || !pred) { free(dist);free(pred);return SD_ERR_OUT_OF_MEMORY; }
    for (size_t i=0;i<n;i++) { dist[i]=INFINITY; pred[i]=SIZE_MAX; }
    dist[source]=0;
    for (size_t pass=1;pass<n;pass++) {
        int changed=0;
        for (size_t u=0;u<n;u++) for (const GraphEdge *e=graph->heads[u];e;e=e->next) {
            if (e->to>=n || !isfinite(e->weight)) { free(dist);free(pred);return SD_ERR_INVALID_ARGUMENT; }
            tick(ops);
            if (isfinite(dist[u]) && dist[u]+e->weight<dist[e->to]) {
                if (!isfinite(dist[u]+e->weight)) { free(dist);free(pred);return SD_ERR_OVERFLOW; }
                dist[e->to]=dist[u]+e->weight; pred[e->to]=u; changed=1;
            }
        }
        if (!changed) break;
    }
    for (size_t u=0;u<n;u++) for (const GraphEdge *e=graph->heads[u];e;e=e->next) {
        if (e->to>=n || !isfinite(e->weight)) { free(dist);free(pred);return SD_ERR_INVALID_ARGUMENT; }
        tick(ops);
        if (isfinite(dist[u]) && dist[u]+e->weight<dist[e->to]) {
            *reachable_negative_cycle=1; *distances=dist; *predecessors=pred;
            return SD_ERR_NEGATIVE_CYCLE;
        }
    }
    *distances=dist; *predecessors=pred; return SD_OK;
}

int floyd_warshall(const GraphMatrix *graph,double **distances,size_t **next_hop,
                   int *negative_cycle,uint64_t *ops) {
    if (!graph || !distances || !next_hop || !negative_cycle || (graph->count&&!graph->weights)) return SD_ERR_INVALID_ARGUMENT;
    *distances=NULL; *next_hop=NULL; *negative_cycle=0;
    size_t n=graph->count,cells;
    if (product_size(n,n,&cells)!=SD_OK || cells>SIZE_MAX/sizeof(double) || cells>SIZE_MAX/sizeof(size_t)) return SD_ERR_OVERFLOW;
    if (!n) return SD_OK;
    double *dist=checked_malloc(cells*sizeof(*dist));
    size_t *next=checked_malloc(cells*sizeof(*next));
    if (!dist || !next) { free(dist);free(next);return SD_ERR_OUT_OF_MEMORY; }
    for (size_t i=0;i<n;i++) for (size_t j=0;j<n;j++) {
        double w=graph->weights[i*n+j];
        if (isnan(w) || w==-INFINITY) { free(dist);free(next);return SD_ERR_INVALID_ARGUMENT; }
        if (i==j && w>0) w=0;
        dist[i*n+j]=w;
        next[i*n+j]=isfinite(w)?j:SIZE_MAX;
    }
    for (size_t k=0;k<n;k++) for (size_t i=0;i<n;i++) for (size_t j=0;j<n;j++) {
        tick(ops);
        if (!isfinite(dist[i*n+k]) || !isfinite(dist[k*n+j])) continue;
        double alternative=dist[i*n+k]+dist[k*n+j];
        if (!isfinite(alternative)) { free(dist);free(next);return SD_ERR_OVERFLOW; }
        if (alternative<dist[i*n+j]) {
            dist[i*n+j]=alternative; next[i*n+j]=next[i*n+k];
        }
    }
    for (size_t i=0;i<n;i++) if (dist[i*n+i]<0) *negative_cycle=1;
    *distances=dist; *next_hop=next;
    return *negative_cycle?SD_ERR_NEGATIVE_CYCLE:SD_OK;
}

int tsp_dp(const GraphMatrix *graph,double *cost,size_t **tour,size_t *tour_length,uint64_t *ops) {
    if (!graph || !cost || !tour || !tour_length || (graph->count && !graph->weights)) return SD_ERR_INVALID_ARGUMENT;
    *cost=0; *tour=NULL; *tour_length=0;
    size_t n=graph->count;
    if (n>15) return SD_ERR_INVALID_ARGUMENT;
    if (!n) return SD_OK;
    for (size_t i=0;i<n;i++) for (size_t j=0;j<n;j++) {
        double w=graph->weights[i*n+j];
        if (i!=j && (!isfinite(w) || w<0)) return SD_ERR_INVALID_ARGUMENT;
    }
    size_t states=(size_t)1<<n, cells=states*n;
    double *dp=checked_malloc(cells*sizeof(*dp));
    size_t *parent=checked_malloc(cells*sizeof(*parent));
    if (!dp || !parent) { free(dp);free(parent);return SD_ERR_OUT_OF_MEMORY; }
    for (size_t i=0;i<cells;i++) { dp[i]=INFINITY; parent[i]=SIZE_MAX; }
    dp[n]=0; /* state mask 1, ending at city 0 */
    for (size_t mask=1;mask<states;mask+=2) for (size_t last=0;last<n;last++) {
        if (!(mask & ((size_t)1<<last)) || !isfinite(dp[mask*n+last])) continue;
        for (size_t next=1;next<n;next++) {
            if (mask & ((size_t)1<<next)) continue;
            tick(ops);
            size_t next_mask=mask|((size_t)1<<next);
            double candidate=dp[mask*n+last]+graph->weights[last*n+next];
            if (!isfinite(candidate)) { free(dp);free(parent);return SD_ERR_OVERFLOW; }
            if (candidate<dp[next_mask*n+next]) {
                dp[next_mask*n+next]=candidate; parent[next_mask*n+next]=last;
            }
        }
    }
    size_t full=states-1,best_last=SIZE_MAX;
    double best=INFINITY;
    if (n==1) { best=0; best_last=0; }
    else for (size_t last=1;last<n;last++) {
        tick(ops);
        double candidate=dp[full*n+last]+graph->weights[last*n];
        if (candidate<best) { best=candidate;best_last=last; }
    }
    if (!isfinite(best) || best_last==SIZE_MAX) { free(dp);free(parent);return SD_ERR_INFEASIBLE; }
    size_t *path=checked_malloc((n+1)*sizeof(*path));
    if (!path) { free(dp);free(parent);return SD_ERR_OUT_OF_MEMORY; }
    path[0]=0;path[n]=0;
    size_t mask=full,last=best_last;
    for (size_t pos=n-1;pos>0;pos--) {
        path[pos]=last;
        size_t prev=parent[mask*n+last];
        mask^=(size_t)1<<last; last=prev;
    }
    free(dp);free(parent); *cost=best;*tour=path;*tour_length=n+1;return SD_OK;
}
