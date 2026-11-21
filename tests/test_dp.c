#include "dynamic_programming.h"
#include "greedy.h"
#include "test_harness.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

static long long subset_best(const int *w,const long long *v,size_t n,size_t cap) {
    long long best=0;
    for (size_t mask=0;mask<((size_t)1<<n);mask++) {
        size_t weight=0; long long val=0;
        for (size_t i=0;i<n;i++) if (mask&((size_t)1<<i)) { weight+=(size_t)w[i];val+=v[i]; }
        if (weight<=cap && val>best) best=val;
    }
    return best;
}
static uint64_t chain_brute(const size_t *d,size_t i,size_t j) {
    if (i==j) return 0;
    uint64_t best=UINT64_MAX;
    for (size_t k=i;k<j;k++) {
        uint64_t candidate=chain_brute(d,i,k)+chain_brute(d,k+1,j)+d[i]*d[k+1]*d[j+1];
        if (candidate<best) best=candidate;
    }
    return best;
}
static size_t lcs_brute(const char *const *a,size_t n,const char *const *b,size_t m) {
    if (!n || !m) return 0;
    if (!strcmp(a[0],b[0])) return 1+lcs_brute(a+1,n-1,b+1,m-1);
    size_t x=lcs_brute(a+1,n-1,b,m),y=lcs_brute(a,n,b+1,m-1);
    return x>y?x:y;
}
static double bst_brute(const double *f,size_t i,size_t j,size_t depth) {
    if (i==j) return 0;
    double best=INFINITY;
    for (size_t root=i;root<j;root++) {
        double v=f[root]*(double)depth+bst_brute(f,i,root,depth+1)+bst_brute(f,root+1,j,depth+1);
        if (v<best) best=v;
    }
    return best;
}
static double tsp_brute_rec(const GraphMatrix *g,size_t at,size_t mask) {
    size_t n=g->count;
    if (mask==((size_t)1<<n)-1) return g->weights[at*n];
    double best=INFINITY;
    for (size_t i=1;i<n;i++) if (!(mask&((size_t)1<<i))) {
        double candidate=g->weights[at*n+i]+tsp_brute_rec(g,i,mask|((size_t)1<<i));
        if (candidate<best) best=candidate;
    }
    return best;
}
static void test_knapsack(void) {
    long long best;size_t *chosen;size_t count;uint64_t ops=0;
    ASSERT_EQ_INT(SD_OK,knapsack_01_dp(NULL,NULL,0,0,&best,&chosen,&count,&ops));
    ASSERT_EQ_INT(0,best);ASSERT_EQ_INT(0,count);free(chosen);
    int w[]={0,2,3};long long v[]={5,6,8};
    ASSERT_EQ_INT(SD_OK,knapsack_01_dp(w,v,3,0,&best,&chosen,&count,&ops));
    ASSERT_EQ_INT(5,best);free(chosen);
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT,knapsack_01_dp(NULL,v,3,2,&best,&chosen,&count,NULL));
    w[0]=-1;ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT,knapsack_01_dp(w,v,3,2,&best,&chosen,&count,NULL));w[0]=0;
    lcg_seed(101);
    for (int trial=0;trial<200;trial++) {
        size_t n=(size_t)lcg_range(0,9), cap=(size_t)lcg_range(0,22);
        int ww[9];long long vv[9];
        for (size_t i=0;i<n;i++) { ww[i]=(int)lcg_range(0,10);vv[i]=(long long)lcg_range(0,30); }
        ASSERT_EQ_INT(SD_OK,knapsack_01_dp(ww,vv,n,cap,&best,&chosen,&count,&ops));
        ASSERT_EQ_INT(subset_best(ww,vv,n,cap),best);
        size_t total_w=0; long long total_v=0;
        for (size_t k=0;k<count;k++) {
            ASSERT_TRUE(chosen[k]<n);
            if (k) ASSERT_TRUE(chosen[k-1]<chosen[k]);
            total_w+=(size_t)ww[chosen[k]];total_v+=vv[chosen[k]];
        }
        ASSERT_TRUE(total_w<=cap);ASSERT_EQ_INT(best,total_v);free(chosen);
    }
}
static void test_chain(void) {
    uint64_t cost,ops=0;char *repr=NULL;
    ASSERT_EQ_INT(SD_OK,matrix_chain_order(NULL,0,&cost,&repr,&ops));
    ASSERT_EQ_INT(0,cost);ASSERT_EQ_STR("",repr);free(repr);
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT,matrix_chain_order(NULL,2,&cost,&repr,&ops));
    size_t d1[]={5,7};
    ASSERT_EQ_INT(SD_OK,matrix_chain_order(d1,1,&cost,&repr,&ops));
    ASSERT_EQ_INT(0,cost);ASSERT_EQ_STR("A1",repr);free(repr);
    lcg_seed(202);
    for (int trial=0;trial<200;trial++) {
        size_t n=(size_t)lcg_range(1,7),d[8];
        for (size_t i=0;i<=n;i++) d[i]=(size_t)lcg_range(1,12);
        ASSERT_EQ_INT(SD_OK,matrix_chain_order(d,n,&cost,&repr,&ops));
        ASSERT_EQ_INT(chain_brute(d,0,n-1),cost);
        ASSERT_TRUE(repr && strlen(repr)>0);free(repr);
    }
}
static void test_lcs(void) {
    uint64_t ops=0;size_t len,*ia,*ib;double sim;char **words;size_t nw;
    ASSERT_EQ_INT(SD_OK,tokenize_text("HELLO, World! 42---hello",&words,&nw));
    ASSERT_EQ_INT(4,nw);ASSERT_EQ_STR("hello",words[0]);ASSERT_EQ_STR("world",words[1]);
    ASSERT_EQ_STR("42",words[2]);ASSERT_EQ_STR("hello",words[3]);free_tokens(words,nw);
    ASSERT_EQ_INT(SD_OK,tokenize_text("...",&words,&nw));ASSERT_EQ_INT(0,nw);free_tokens(words,nw);
    ASSERT_EQ_INT(SD_OK,lcs_tokens(NULL,0,NULL,0,&len,&ia,&ib,&sim,&ops));
    ASSERT_EQ_INT(0,len);ASSERT_EQ_DOUBLE(1,sim,1e-12);free(ia);free(ib);
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT,lcs_tokens(NULL,1,NULL,0,&len,&ia,&ib,&sim,NULL));
    const char *vocab[]={"news","event","source","time","alert","breaking"};
    lcg_seed(303);
    for (int trial=0;trial<200;trial++) {
        size_t n=(size_t)lcg_range(0,8),m=(size_t)lcg_range(0,8);
        const char *a[8],*b[8];
        for (size_t i=0;i<n;i++) a[i]=vocab[lcg_range(0,5)];
        for (size_t j=0;j<m;j++) b[j]=vocab[lcg_range(0,5)];
        ASSERT_EQ_INT(SD_OK,lcs_tokens(a,n,b,m,&len,&ia,&ib,&sim,&ops));
        ASSERT_EQ_INT(lcs_brute(a,n,b,m),len);
        ASSERT_EQ_DOUBLE(n+m?2.0*(double)len/(double)(n+m):1,sim,1e-12);
        for (size_t k=0;k<len;k++) {
            ASSERT_EQ_STR(a[ia[k]],b[ib[k]]);
            if (k) { ASSERT_TRUE(ia[k-1]<ia[k]);ASSERT_TRUE(ib[k-1]<ib[k]); }
        }
        free(ia);free(ib);
    }
    const char *same[]={"a","b"},*other[]={"x","y"};
    ASSERT_EQ_INT(SD_OK,lcs_tokens(same,2,same,2,&len,&ia,&ib,&sim,NULL));
    ASSERT_EQ_INT(2,len);ASSERT_EQ_DOUBLE(1,sim,1e-12);free(ia);free(ib);
    ASSERT_EQ_INT(SD_OK,lcs_tokens(same,2,other,2,&len,&ia,&ib,&sim,NULL));
    ASSERT_EQ_INT(0,len);ASSERT_EQ_DOUBLE(0,sim,1e-12);free(ia);free(ib);
}
static void test_bst(void) {
    double cost;size_t *roots;uint64_t ops=0;
    ASSERT_EQ_INT(SD_OK,optimal_bst(NULL,0,&cost,&roots,&ops));ASSERT_EQ_DOUBLE(0,cost,1e-12);free(roots);
    double neg[]={-1};ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT,optimal_bst(neg,1,&cost,&roots,&ops));
    lcg_seed(404);
    for (int trial=0;trial<200;trial++) {
        size_t n=(size_t)lcg_range(1,6);double f[6],total=0;
        for (size_t i=0;i<n;i++) { f[i]=(double)lcg_range(0,9);total+=f[i]; }
        ASSERT_EQ_INT(SD_OK,optimal_bst(f,n,&cost,&roots,&ops));
        double oracle=bst_brute(f,0,n,1);
        ASSERT_EQ_DOUBLE(total?oracle/total:0,cost,1e-10);
        for (size_t i=0;i<n;i++) for (size_t j=i;j<n;j++) ASSERT_TRUE(roots[i*n+j]>=i && roots[i*n+j]<=j);
        free(roots);
    }
}
static void test_graphs(void) {
    GraphList *list;GraphMatrix *matrix;double *dist,*all;size_t *pred,*next;int cycle;uint64_t ops=0;
    ASSERT_EQ_INT(SD_OK,graph_list_create(4,&list));
    ASSERT_EQ_INT(SD_OK,graph_list_add(list,0,1,2,0));
    ASSERT_EQ_INT(SD_OK,graph_list_add(list,1,2,-4,0));
    ASSERT_EQ_INT(SD_OK,graph_list_add(list,2,1,1,0));
    ASSERT_EQ_INT(SD_ERR_NEGATIVE_CYCLE,bellman_ford(list,0,&dist,&pred,&cycle,&ops));
    ASSERT_EQ_INT(1,cycle);free(dist);free(pred);
    ASSERT_EQ_INT(SD_OK,bellman_ford(list,3,&dist,&pred,&cycle,&ops));
    ASSERT_EQ_INT(0,cycle);ASSERT_TRUE(isinf(dist[0]));free(dist);free(pred);graph_list_free(list);
    ASSERT_EQ_INT(SD_OK,graph_matrix_create(3,&matrix));
    graph_matrix_add(matrix,0,1,2,0);graph_matrix_add(matrix,1,2,-3,0);
    graph_matrix_add(matrix,2,0,-2,0);
    ASSERT_EQ_INT(SD_ERR_NEGATIVE_CYCLE,floyd_warshall(matrix,&all,&next,&cycle,&ops));
    ASSERT_EQ_INT(1,cycle);free(all);free(next);graph_matrix_free(matrix);
    /* Negative edges without a cycle; an unreachable negative cycle stays unreachable. */
    ASSERT_EQ_INT(SD_OK,graph_list_create(5,&list));
    ASSERT_EQ_INT(SD_OK,graph_list_add(list,0,1,3,0));
    ASSERT_EQ_INT(SD_OK,graph_list_add(list,1,2,-5,0));
    ASSERT_EQ_INT(SD_OK,graph_list_add(list,3,4,-2,0));
    ASSERT_EQ_INT(SD_OK,graph_list_add(list,4,3,1,0));
    ASSERT_EQ_INT(SD_OK,bellman_ford(list,0,&dist,&pred,&cycle,&ops));
    ASSERT_EQ_INT(0,cycle);ASSERT_EQ_DOUBLE(-2,dist[2],1e-12);
    ASSERT_EQ_INT(1,pred[2]);ASSERT_TRUE(isinf(dist[3]));free(dist);free(pred);
    ASSERT_EQ_INT(SD_ERR_NEGATIVE_CYCLE,bellman_ford(list,3,&dist,&pred,&cycle,&ops));
    ASSERT_EQ_INT(1,cycle);free(dist);free(pred);graph_list_free(list);
    ASSERT_EQ_INT(SD_OK,graph_matrix_create(0,&matrix));
    ASSERT_EQ_INT(SD_OK,floyd_warshall(matrix,&all,&next,&cycle,&ops));
    ASSERT_TRUE(all==NULL && next==NULL);graph_matrix_free(matrix);
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT,bellman_ford(NULL,0,&dist,&pred,&cycle,&ops));
    lcg_seed(505);
    for (int trial=0;trial<200;trial++) {
        size_t n=(size_t)lcg_range(2,8),s=(size_t)lcg_range(0,n-1);
        ASSERT_EQ_INT(SD_OK,graph_list_create(n,&list));
        ASSERT_EQ_INT(SD_OK,graph_matrix_create(n,&matrix));
        for (size_t i=0;i<n;i++) for (size_t j=0;j<n;j++) if (i!=j && lcg_range(0,3)!=0) {
            double w=(double)lcg_range(0,20);
            ASSERT_EQ_INT(SD_OK,graph_list_add(list,i,j,w,0));
            ASSERT_EQ_INT(SD_OK,graph_matrix_add(matrix,i,j,w,0));
        }
        ASSERT_EQ_INT(SD_OK,bellman_ford(list,s,&dist,&pred,&cycle,&ops));
        ASSERT_EQ_INT(0,cycle);
        double oracle[8];size_t oracle_pred[8];
        ASSERT_EQ_INT(SD_OK,dijkstra(list,s,oracle,oracle_pred,NULL));
        for (size_t j=0;j<n;j++) {
            if (isinf(oracle[j])) ASSERT_TRUE(isinf(dist[j]));
            else ASSERT_EQ_DOUBLE(oracle[j],dist[j],1e-9);
        }
        free(dist);free(pred);
        ASSERT_EQ_INT(SD_OK,floyd_warshall(matrix,&all,&next,&cycle,&ops));
        ASSERT_EQ_INT(0,cycle);
        for (size_t source=0;source<n;source++) {
            ASSERT_EQ_INT(SD_OK,bellman_ford(list,source,&dist,&pred,&cycle,NULL));
            for (size_t j=0;j<n;j++) {
                if (isinf(dist[j])) { ASSERT_TRUE(isinf(all[source*n+j]));ASSERT_EQ_INT(SIZE_MAX,next[source*n+j]); }
                else { ASSERT_EQ_DOUBLE(dist[j],all[source*n+j],1e-9); ASSERT_TRUE(next[source*n+j]<n); }
            }
            free(dist);free(pred);
        }
        free(all);free(next);graph_list_free(list);graph_matrix_free(matrix);
    }
}
static void test_tsp(void) {
    double cost;size_t *tour,len;uint64_t ops=0;
    GraphMatrix *g;
    ASSERT_EQ_INT(SD_OK,graph_matrix_create(0,&g));
    ASSERT_EQ_INT(SD_OK,tsp_dp(g,&cost,&tour,&len,&ops));ASSERT_EQ_INT(0,len);free(tour);graph_matrix_free(g);
    ASSERT_EQ_INT(SD_OK,graph_matrix_create(1,&g));
    ASSERT_EQ_INT(SD_OK,tsp_dp(g,&cost,&tour,&len,&ops));ASSERT_EQ_INT(2,len);
    ASSERT_EQ_INT(0,tour[0]);ASSERT_EQ_INT(0,tour[1]);ASSERT_EQ_DOUBLE(0,cost,1e-12);free(tour);graph_matrix_free(g);
    ASSERT_EQ_INT(SD_OK,graph_matrix_create(16,&g));
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT,tsp_dp(g,&cost,&tour,&len,&ops));graph_matrix_free(g);
    ASSERT_EQ_INT(SD_OK,graph_matrix_create(2,&g));
    graph_matrix_add(g,0,1,4,0);graph_matrix_add(g,1,0,7,0);
    ASSERT_EQ_INT(SD_OK,tsp_dp(g,&cost,&tour,&len,&ops));
    ASSERT_EQ_DOUBLE(11,cost,1e-12);ASSERT_EQ_INT(3,len);
    ASSERT_EQ_INT(1,tour[1]);free(tour);graph_matrix_free(g);
    ASSERT_EQ_INT(SD_OK,graph_matrix_create(15,&g));
    for (size_t i=0;i<15;i++) for (size_t j=0;j<15;j++) if (i!=j) graph_matrix_add(g,i,j,1,0);
    ASSERT_EQ_INT(SD_OK,tsp_dp(g,&cost,&tour,&len,&ops));ASSERT_EQ_DOUBLE(15,cost,1e-9);
    ASSERT_EQ_INT(16,len);free(tour);graph_matrix_free(g);
    lcg_seed(606);
    for (int trial=0;trial<200;trial++) {
        size_t n=(size_t)lcg_range(2,8);
        ASSERT_EQ_INT(SD_OK,graph_matrix_create(n,&g));
        for (size_t i=0;i<n;i++) for (size_t j=0;j<n;j++) if (i!=j)
            graph_matrix_add(g,i,j,(double)lcg_range(1,25),0);
        ASSERT_EQ_INT(SD_OK,tsp_dp(g,&cost,&tour,&len,&ops));
        ASSERT_EQ_DOUBLE(tsp_brute_rec(g,0,1),cost,1e-9);
        ASSERT_EQ_INT(n+1,len);ASSERT_EQ_INT(0,tour[0]);ASSERT_EQ_INT(0,tour[n]);
        int seen[8]={0};double sum=0;
        for (size_t j=1;j<n;j++) { ASSERT_TRUE(tour[j]>0 && tour[j]<n);seen[tour[j]]++; }
        for (size_t j=1;j<n;j++) ASSERT_EQ_INT(1,seen[j]);
        for (size_t j=1;j<=n;j++) sum+=g->weights[tour[j-1]*n+tour[j]];
        ASSERT_EQ_DOUBLE(cost,sum,1e-9);free(tour);graph_matrix_free(g);
    }
}
int main(void) {
    test_knapsack();test_chain();test_lcs();test_bst();test_graphs();test_tsp();
    return harness_summary();
}
