#include "test_harness.h"
#include "greedy.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

static int activity_oracle(const SDInterval *a,size_t n) {
    int best=0;
    for (unsigned mask=0;mask<(1u<<n);++mask) {
        int count=0,valid=1;
        for (size_t i=0;i<n;++i) if (mask&(1u<<i)) {
            ++count;
            for (size_t j=i+1;j<n;++j) if ((mask&(1u<<j)) &&
                a[i].start<a[j].end && a[j].start<a[i].end) valid=0;
        }
        if (valid && count>best) best=count;
    }
    return best;
}
static int overlap_depth(const SDInterval *a,size_t n) {
    int best=0;
    for (size_t i=0;i<n;++i) {
        int depth=0;
        for (size_t j=0;j<n;++j)
            if (a[j].start<=a[i].start && a[i].start<a[j].end) ++depth;
        if (depth>best) best=depth;
    }
    return best;
}
static void tests_scheduling(void) {
    uint64_t ops=0;
    size_t count=99,desks=99,selected[10],assigned[10];
    ASSERT_EQ_INT(SD_OK,activity_selection(NULL,0,NULL,&count,&ops));
    ASSERT_EQ_INT(0,count);
    ASSERT_EQ_INT(SD_OK,interval_partitioning(NULL,0,NULL,&desks,&ops));
    ASSERT_EQ_INT(0,desks);
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT,activity_selection(NULL,1,selected,&count,&ops));
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT,interval_partitioning(NULL,1,assigned,&desks,&ops));
    SDInterval touch[]={{0,2},{2,4},{1,3}};
    ASSERT_EQ_INT(SD_OK,activity_selection(touch,3,selected,&count,&ops));
    ASSERT_EQ_INT(2,count);
    ASSERT_EQ_INT(SD_OK,interval_partitioning(touch,3,assigned,&desks,&ops));
    ASSERT_EQ_INT(2,desks);
    SDInterval bad[]={{4,1}};
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT,activity_selection(bad,1,selected,&count,&ops));
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT,interval_partitioning(bad,1,assigned,&desks,&ops));
    SDInterval zero[]={{1,1}};
    ASSERT_EQ_INT(SD_OK,interval_partitioning(zero,1,assigned,&desks,&ops));
    ASSERT_EQ_INT(0,desks);ASSERT_EQ_INT(SIZE_MAX,assigned[0]);
    lcg_seed(2026);
    for (int trial=0;trial<200;++trial) {
        SDInterval a[8];size_t n=(size_t)lcg_range(1,8);
        for (size_t i=0;i<n;++i) {
            a[i].start=(int)lcg_range(0,12);
            a[i].end=a[i].start+(int)lcg_range(1,5);
        }
        ASSERT_EQ_INT(SD_OK,activity_selection(a,n,selected,&count,&ops));
        ASSERT_EQ_INT(activity_oracle(a,n),count);
        for (size_t i=0;i<count;++i)
            for (size_t j=i+1;j<count;++j)
                ASSERT_TRUE(a[selected[i]].end<=a[selected[j]].start);
        ASSERT_EQ_INT(SD_OK,interval_partitioning(a,n,assigned,&desks,&ops));
        ASSERT_EQ_INT(overlap_depth(a,n),desks);
        for (size_t i=0;i<n;++i) {
            ASSERT_TRUE(assigned[i]<desks);
            for (size_t j=i+1;j<n;++j) if (assigned[i]==assigned[j])
                ASSERT_TRUE(a[i].end<=a[j].start || a[j].end<=a[i].start);
        }
    }
    ASSERT_TRUE(ops>0);
}
/* An optimum of the fractional LP is a subset of full items and at most one
   partly taken item. This oracle enumerates all such extreme points. */
static double fractional_oracle(const SDItem *a,size_t n,double capacity) {
    double best=0;
    for (unsigned mask=0;mask<(1u<<n);++mask) {
        double weight=0,value=0;
        for (size_t i=0;i<n;++i) if (mask&(1u<<i)) {
            weight+=a[i].weight;value+=a[i].value;
        }
        if (weight>capacity+1e-10) continue;
        if (value>best) best=value;
        for (size_t i=0;i<n;++i) if (!(mask&(1u<<i))) {
            double fraction=a[i].weight==0 ? 1 : (capacity-weight)/a[i].weight;
            if (fraction>1) fraction=1;
            double candidate=value+fraction*a[i].value;
            if (candidate>best) best=candidate;
        }
    }
    return best;
}
static void tests_fractional(void) {
    double total=0, fractions[8];uint64_t ops=0;
    ASSERT_EQ_INT(SD_OK,fractional_knapsack(NULL,0,0,NULL,&total,&ops));
    ASSERT_EQ_DOUBLE(0,total,0);
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT,fractional_knapsack(NULL,1,1,fractions,&total,&ops));
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT,fractional_knapsack(NULL,0,-1,NULL,&total,&ops));
    SDItem one[]={{0,10},{5,20}};
    ASSERT_EQ_INT(SD_OK,fractional_knapsack(one,2,0,fractions,&total,&ops));
    ASSERT_EQ_DOUBLE(10,total,1e-9);
    ASSERT_EQ_DOUBLE(1,fractions[0],1e-9);
    lcg_seed(2027);
    for (int t=0;t<200;++t) {
        SDItem a[7];size_t n=(size_t)lcg_range(1,7);
        for (size_t i=0;i<n;++i) {
            a[i].weight=(double)lcg_range(0,8);
            a[i].value=(double)lcg_range(0,16);
        }
        double cap=(double)lcg_range(0,24);
        ASSERT_EQ_INT(SD_OK,fractional_knapsack(a,n,cap,fractions,&total,&ops));
        ASSERT_EQ_DOUBLE(fractional_oracle(a,n,cap),total,1e-8);
        double used=0,value=0;
        for (size_t i=0;i<n;++i) {
            ASSERT_TRUE(fractions[i]>=0 && fractions[i]<=1);
            used+=fractions[i]*a[i].weight;
            value+=fractions[i]*a[i].value;
        }
        ASSERT_TRUE(used<=cap+1e-9);
        ASSERT_EQ_DOUBLE(value,total,1e-8);
    }
}
static void roundtrip(const unsigned char *data,size_t n) {
    SDHuffman *enc=NULL;unsigned char *decoded=NULL;size_t outn=0;
    double ratio=-1;uint64_t ops=0;
    ASSERT_EQ_INT(SD_OK,huffman_encode(data,n,&enc,&ratio,&ops));
    ASSERT_TRUE(enc!=NULL);
    if (!enc) return;
    ASSERT_EQ_INT(SD_OK,huffman_decode(enc,&decoded,&outn,&ops));
    ASSERT_EQ_INT(n,outn);
    if (decoded) ASSERT_TRUE(n == 0 || memcmp(data,decoded,n)==0);
    ASSERT_EQ_DOUBLE(n ? (double)enc->byte_count/n : 0,ratio,1e-12);
    free(decoded);huffman_free(enc);
}
static void tests_huffman(void) {
    roundtrip(NULL,0);
    unsigned char one[]={0};roundtrip(one,1);
    unsigned char repeats[257];memset(repeats,42,sizeof(repeats));roundtrip(repeats,sizeof(repeats));
    unsigned char alphabet[256];for (int i=0;i<256;++i) alphabet[i]=(unsigned char)i;
    roundtrip(alphabet,sizeof(alphabet));
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT,huffman_encode(NULL,1,NULL,NULL,NULL));
    lcg_seed(2028);
    for (int t=0;t<200;++t) {
        unsigned char bytes[512];size_t n=(size_t)lcg_range(0,512);
        unsigned symbols=(unsigned)lcg_range(1,256);
        for (size_t i=0;i<n;++i) bytes[i]=(unsigned char)lcg_range(0,symbols-1);
        roundtrip(bytes,n);
    }
}
static void dijkstra_oracle(const GraphList *g,size_t source,double *dist) {
    size_t n=g->count;
    for (size_t i=0;i<n;++i) dist[i]=INFINITY;
    dist[source]=0;
    for (size_t k=0;k+1<n;++k) {
        for (size_t u=0;u<n;++u) if (isfinite(dist[u]))
            for (GraphEdge *e=g->heads[u];e;e=e->next)
                if (dist[u]+e->weight<dist[e->to]) dist[e->to]=dist[u]+e->weight;
    }
}
static void tests_graph(void) {
    GraphList *g=NULL;GraphMatrix *m=NULL;uint64_t ops=0;
    ASSERT_EQ_INT(SD_OK,graph_list_create(0,&g));
    ASSERT_EQ_INT(0,g->count);graph_list_free(g);
    ASSERT_EQ_INT(SD_OK,graph_matrix_create(0,&m));
    SDTreeEdge empty_edges[1];size_t empty_count=99;double empty_weight=9;
    ASSERT_EQ_INT(SD_OK,prim(m,empty_edges,&empty_count,&empty_weight,&ops));
    ASSERT_EQ_INT(0,empty_count);
    ASSERT_EQ_INT(SD_OK,kruskal(m,empty_edges,&empty_count,&empty_weight,&ops));
    graph_matrix_free(m);
    ASSERT_EQ_INT(SD_OK,graph_list_create(1,&g));
    double dist[8];size_t pred[8];
    ASSERT_EQ_INT(SD_OK,dijkstra(g,0,dist,pred,&ops));
    ASSERT_EQ_DOUBLE(0,dist[0],0);
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT,dijkstra(g,1,dist,pred,&ops));
    graph_list_free(g);
    ASSERT_EQ_INT(SD_OK,graph_matrix_create(1,&m));
    SDTreeEdge tree[8];size_t count=99;double weight=0;
    ASSERT_EQ_INT(SD_OK,prim(m,tree,&count,&weight,&ops));
    ASSERT_EQ_INT(0,count);
    ASSERT_EQ_INT(SD_OK,kruskal(m,tree,&count,&weight,&ops));
    graph_matrix_free(m);
    ASSERT_EQ_INT(SD_OK,graph_list_create(2,&g));
    ASSERT_EQ_INT(SD_OK,graph_list_add(g,0,1,-1,0));
    ASSERT_EQ_INT(SD_ERR_NEGATIVE_WEIGHT,dijkstra(g,0,dist,pred,&ops));
    graph_list_free(g);
    ASSERT_EQ_INT(SD_OK,graph_matrix_create(3,&m));
    ASSERT_EQ_INT(SD_ERR_INFEASIBLE,prim(m,tree,&count,&weight,&ops));
    ASSERT_EQ_INT(SD_ERR_INFEASIBLE,kruskal(m,tree,&count,&weight,&ops));
    graph_matrix_free(m);
    ASSERT_EQ_INT(SD_OK,graph_matrix_create(4,&m));
    for (size_t i=0;i<4;++i) for (size_t j=i+1;j<4;++j)
        ASSERT_EQ_INT(SD_OK,graph_matrix_add(m,i,j,3,1));
    double pweight=0,kweight=0;size_t pn=0,kn=0;
    SDTreeEdge pe[4],ke[4];
    ASSERT_EQ_INT(SD_OK,prim(m,pe,&pn,&pweight,&ops));
    ASSERT_EQ_INT(SD_OK,kruskal(m,ke,&kn,&kweight,&ops));
    ASSERT_EQ_DOUBLE(9,pweight,1e-9);
    ASSERT_EQ_DOUBLE(pweight,kweight,1e-9);
    graph_matrix_free(m);
    lcg_seed(2029);
    for (int t=0;t<200;++t) {
        size_t n=(size_t)lcg_range(2,8);
        ASSERT_EQ_INT(SD_OK,graph_list_create(n,&g));
        ASSERT_EQ_INT(SD_OK,graph_matrix_create(n,&m));
        for (size_t u=0;u<n;++u) for (size_t v=u+1;v<n;++v) {
            if (v==u+1 || lcg_range(0,3)!=0) {
                double w=(double)lcg_range(0,12);
                ASSERT_EQ_INT(SD_OK,graph_list_add(g,u,v,w,1));
                ASSERT_EQ_INT(SD_OK,graph_matrix_add(m,u,v,w,1));
            }
        }
        double reference[8];dijkstra_oracle(g,0,reference);
        ASSERT_EQ_INT(SD_OK,dijkstra(g,0,dist,pred,&ops));
        for (size_t i=0;i<n;++i) ASSERT_EQ_DOUBLE(reference[i],dist[i],1e-8);
        SDTreeEdge p[8],k[8];size_t pn=0,kn=0;double pw=0,kw=0;
        ASSERT_EQ_INT(SD_OK,prim(m,p,&pn,&pw,&ops));
        ASSERT_EQ_INT(SD_OK,kruskal(m,k,&kn,&kw,&ops));
        ASSERT_EQ_INT(n-1,pn);ASSERT_EQ_INT(n-1,kn);
        ASSERT_EQ_DOUBLE(pw,kw,1e-9);
        graph_list_free(g);graph_matrix_free(m);
    }
    ASSERT_TRUE(ops>0);
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT,prim(NULL,tree,&count,&weight,NULL));
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT,kruskal(NULL,tree,&count,&weight,NULL));
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT,graph_list_create(2,NULL));
}
int main(void) {
    tests_scheduling();tests_fractional();tests_huffman();tests_graph();
    return harness_summary();
}
