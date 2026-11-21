#include "workbench.h"
#include "sorting_searching.h"
#include "divide_conquer.h"
#include "greedy.h"
#include "dynamic_programming.h"
#include "backtracking.h"
#include "graph_algorithms.h"
#include "string_matching.h"
#include "number_theory.h"
#include <inttypes.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* The sole metadata table: every entry is executable using the real library. */
typedef struct {
    const char *group, *name, *explanation, *inputs, *time, *space, *edges;
} WorkbenchEntry;
static const WorkbenchEntry entries[] = {
    {"Sorting and searching","Merge Sort","Stable divide-and-merge sorting.","integer array; Article pointers","O(n log n)","O(n)","empty, duplicate and ordered data"},
    {"Sorting and searching","Quick Sort","Partition around median-of-three pivots.","integer array; Article pointers","O(n log n) average, O(n^2) worst","O(log n) average stack","empty, duplicates and reverse order"},
    {"Sorting and searching","Iterative Binary Search","First match in sorted array.","sorted integers and target","O(log n)","O(1)","empty and duplicate keys"},
    {"Sorting and searching","Recursive Binary Search","Recursive first-match lookup.","sorted integers and target","O(log n)","O(log n)","empty and missing target"},
    {"Divide and conquer","Maximum Subarray","Maximum sum over a contiguous interval.","signed integers","O(n log n)","O(log n)","negative-only and ties"},
    {"Divide and conquer","Fast Exponentiation","Binary exponentiation (also modular and double).","base, exponent, optional modulus","O(log exponent)","O(1)","zero exponent and overflow"},
    {"Divide and conquer","Celebrity Identification","Eliminate and verify candidate source.","source reference matrix","O(n)","O(1)","none or single source"},
    {"Divide and conquer","Classical Matrix Multiplication","Three-loop matrix product.","two square matrices","O(n^3)","O(1) extra","zero-size and one-size"},
    {"Divide and conquer","Strassen Matrix Multiplication","Seven recursive products with padding.","two square matrices","O(n^log2(7))","O(n^2) workspace per recursion level","odd sizes and cutoff"},
    {"Greedy","Activity Selection","Choose maximum nonoverlapping tasks.","start/end intervals","O(n log n)","O(n)","touching and empty intervals"},
    {"Greedy","Interval Partitioning","Assign minimum parallel desks.","start/end intervals","O(n log n)","O(n)","touching boundaries"},
    {"Greedy","Fractional Knapsack","Take best value-density fractions.","weights, values, capacity","O(n log n)","O(n)","zero capacity or weight"},
    {"Greedy","Huffman Coding","Prefix-code byte stream, then decode.","byte buffer","O(n + k log k)","O(k + encoded bits)","empty and one symbol"},
    {"Greedy","Dijkstra","Single-source nonnegative shortest paths.","weighted adjacency list","O((V+E) log V)","O(V+E)","unreachable and negative edges"},
    {"Greedy","Prim","Minimum spanning tree from vertices.","undirected weighted matrix","O(V^2)","O(V)","disconnected network"},
    {"Greedy","Kruskal","Minimum spanning tree with disjoint sets.","undirected weighted matrix","O(E log E)","O(V+E)","equal weights and disconnected"},
    {"Dynamic programming","0/1 Knapsack DP","Optimize whole-item selections.","integer weights, values, capacity","O(n*capacity)","O(n*capacity)","zero capacity and overweight"},
    {"Dynamic programming","Matrix Chain Order","Choose least-cost parenthesization.","matrix dimensions","O(n^3)","O(n^2)","zero or one matrix"},
    {"Dynamic programming","Longest Common Subsequence","Measure token overlap (not meaning).","two token sequences","O(a*b)","O(a*b)","empty and disjoint tokens"},
    {"Dynamic programming","Optimal Binary Search Tree","Choose roots minimizing weighted cost.","sorted key frequencies","O(n^3)","O(n^2)","zero frequencies and empty"},
    {"Dynamic programming","Bellman-Ford","Negative-edge paths and reachable-cycle check.","weighted adjacency list","O(V*E)","O(V)","negative cycle and unreachable"},
    {"Dynamic programming","Floyd-Warshall","All-pairs paths with next hops.","weighted adjacency matrix","O(V^3)","O(V^2)","unreachable and negative cycles"},
    {"Dynamic programming","Travelling Salesperson DP","Bitmask optimum closed tour.","complete graph, up to 15 cities","O(n^2*2^n)","O(n*2^n)","one city and cap of 15"},
    {"Backtracking and branch and bound","N-Queens","Enumerate nonattacking queen placements.","board size 1..12","O(n!) worst","O(n)","unsatisfiable n=2,3"},
    {"Backtracking and branch and bound","Sum of Subsets","Enumerate combinations totaling target.","nonnegative values and target","O(2^n) worst","O(n + output)","zero target and unreachable"},
    {"Backtracking and branch and bound","0/1 Knapsack Backtracking","Exact include/exclude exploration.","integer items and capacity","O(2^n) worst","O(n)","empty and overweight"},
    {"Backtracking and branch and bound","FIFO Branch and Bound","Queue frontier using shared upper bound.","integer items and capacity","O(2^n) worst","O(2^n) frontier","empty and zero capacity"},
    {"Backtracking and branch and bound","LIFO Branch and Bound","Stack frontier using shared upper bound.","integer items and capacity","O(2^n) worst","O(2^n) frontier","equal-density items"},
    {"Backtracking and branch and bound","Least-Cost Branch and Bound","Priority frontier using shared upper bound.","integer items and capacity","O(2^n log 2^n) worst","O(2^n) frontier","zero-value items"},
    {"Flow and matching","Ford-Fulkerson","Residual-network augmenting flow.","capacity matrix, source, sink","O(E*|maxflow|) integral","O(V^2)","zero capacities and disconnected"},
    {"Flow and matching","Maximum Bipartite Matching","Augment across compatible pairs.","left-right compatibility matrix","O(V*E)","O(V+E)","no compatible pairs"},
    {"Number theory","Chinese Remainder Theorem","Reconstruct bounded modular counter.","pairwise-coprime moduli, residues","O(k log M)","O(1) extra","noncoprime and overflow"},
    {"String matching","Naive Matching","Compare each text window.","text bytes and pattern bytes","O(n*m)","O(1)","overlap and empty pattern"},
    {"String matching","Rabin-Karp","Rolling hashes verified bytewise.","text bytes and pattern bytes","O(n+m) expected; O(n*m) worst","O(1)","collisions and overlaps"},
    {"String matching","Finite-Automaton Matching","Follow precomputed byte transitions.","text bytes and pattern bytes","O(n + 256*m)","O(256*m)","empty and repeated patterns"},
    {"String matching","KMP","Reuse prefix matches on mismatch.","text bytes and pattern bytes","O(n+m)","O(m)","repeated and overlapping patterns"}
};
#define WB_COUNT (sizeof(entries)/sizeof(entries[0]))
size_t workbench_count(void) { return WB_COUNT; }
int workbench_list(FILE *out) {
    if (!out) return SD_ERR_INVALID_ARGUMENT;
    const char *group=NULL;
    for(size_t i=0;i<WB_COUNT;i++) {
        if(!group || strcmp(group,entries[i].group)) {group=entries[i].group;fprintf(out,"\n%s\n",group);}
        fprintf(out,"%2zu. %s\n",i+1,entries[i].name);
    }
    return ferror(out)?SD_ERR_INVALID_ARGUMENT:SD_OK;
}

/* Fixed tiny inputs keep every exercise suitable for an interactive demo. */
int workbench_run(size_t index, FILE *out) {
    if (!out || index>=WB_COUNT) return SD_ERR_INVALID_ARGUMENT;
    const WorkbenchEntry *e=&entries[index];
    fprintf(out,"\n[%zu] %s / %s\nExplanation: %s\nInputs: %s\nTime: %s; Space: %s\nEdge cases: %s\n",index+1,e->group,e->name,e->explanation,e->inputs,e->time,e->space,e->edges);
    uint64_t ops=0;
    int rc=SD_OK;
    switch(index) {
    case 0: case 1: {
        int a[]={7,2,4,2,9};
        rc=index==0?merge_sort(a,5,&ops):quick_sort(a,5,&ops);
        if(rc==SD_OK) fprintf(out,"Sorted: %d %d %d %d %d\n",a[0],a[1],a[2],a[3],a[4]);
        break;
    }
    case 2: case 3: {
        int a[]={1,2,2,4,8};ptrdiff_t first=-1;
        rc=index==2?binary_search_iterative(a,5,2,&first,&ops):binary_search_recursive(a,5,2,&first,&ops);
        if(rc==SD_OK) fprintf(out,"First index of 2: %td\n",first);
        break;
    }
    case 4: {long long a[]={-2,3,4,-1,2};size_t s=0,t=0;long long sum=0;
        rc=max_subarray(a,5,&s,&t,&sum,&ops);if(rc==SD_OK)fprintf(out,"Range [%zu,%zu], sum %lld\n",s,t,sum);
        break;}
    case 5: {long long x=0,r=0;double d=0;rc=fast_power(3,5,&x,&ops);
        if(rc==SD_OK)rc=fast_power_mod(3,5,7,&r,&ops);
        if(rc==SD_OK)rc=fast_power_double(2.0,-3,&d,&ops);
        if(rc==SD_OK)fprintf(out,"3^5=%lld, modulo 7=%lld, 2^-3=%.3f\n",x,r,d);
        break;}
    case 6: {unsigned char refs[]={0,1,0,0,0,0,0,1,0};ptrdiff_t found=-1;
        rc=find_celebrity(refs,3,&found,&ops);if(rc==SD_OK)fprintf(out,"Referenced-by-all source index: %td\n",found);
        break;}
    case 7: case 8: {double a[]={1,2,3,4},b[]={2,0,1,2},c[4]={0};
        rc=index==7?matrix_multiply_classical(a,b,c,2,&ops):matrix_multiply_strassen(a,b,c,2,&ops);
        if(rc==SD_OK)fprintf(out,"Matrix product: %.0f %.0f / %.0f %.0f\n",c[0],c[1],c[2],c[3]);
        break;}
    case 9: case 10: {SDInterval a[]={{1,3},{2,4},{3,5}};size_t ids[3]={0},n=0;
        rc=index==9?activity_selection(a,3,ids,&n,&ops):interval_partitioning(a,3,ids,&n,&ops);
        if(rc==SD_OK)fprintf(out,"%s: %zu\n",index==9?"Selected tasks":"Desks",n);
        break;}
    case 11: {SDItem items[]={{2,6},{3,6}};double f[2],value=0;
        rc=fractional_knapsack(items,2,3,f,&value,&ops);if(rc==SD_OK)fprintf(out,"Value %.2f, fractions %.2f %.2f\n",value,f[0],f[1]);
        break;}
    case 12: {const unsigned char source[]="ABABABA";SDHuffman *encoded=NULL;unsigned char *decoded=NULL;size_t count=0;double ratio=0;
        rc=huffman_encode(source,sizeof(source)-1,&encoded,&ratio,&ops);
        if(rc==SD_OK)rc=huffman_decode(encoded,&decoded,&count,&ops);
        if(rc==SD_OK && (count!=sizeof(source)-1 || memcmp(decoded,source,count)))rc=SD_ERR_INFEASIBLE;
        if(rc==SD_OK)fprintf(out,"Huffman round-trip %zu bytes, payload ratio %.3f\n",count,ratio);
        free(decoded);huffman_free(encoded);
        break;}
    case 13: case 20: {
        GraphList *g=NULL;double *d=NULL;size_t *p=NULL;int cycle=0;
        rc=graph_list_create(3,&g);
        if(rc==SD_OK)rc=graph_list_add(g,0,1,2,0);
        if(rc==SD_OK)rc=graph_list_add(g,1,2,3,0);
        if(rc==SD_OK && index==13){double dist[3];size_t pred[3];rc=dijkstra(g,0,dist,pred,&ops);if(rc==SD_OK)fprintf(out,"0->2 distance: %.1f\n",dist[2]);}
        if(rc==SD_OK && index==20){rc=bellman_ford(g,0,&d,&p,&cycle,&ops);if(rc==SD_OK)fprintf(out,"0->2 distance: %.1f, cycle=%d\n",d[2],cycle);}
        free(d);
        free(p);graph_list_free(g);
        break;
    }
    case 14: case 15: case 21: case 22: {
        GraphMatrix *g=NULL;rc=graph_matrix_create(3,&g);
        if(rc==SD_OK)rc=graph_matrix_add(g,0,1,2,1);
        if(rc==SD_OK)rc=graph_matrix_add(g,1,2,3,1);
        if(rc==SD_OK)rc=graph_matrix_add(g,0,2,8,1);
        if(rc==SD_OK && (index==14 || index==15)) {
            SDTreeEdge edges[2];size_t edge_count=0;double weight=0;
            rc=index==14?prim(g,edges,&edge_count,&weight,&ops):kruskal(g,edges,&edge_count,&weight,&ops);
            if(rc==SD_OK)fprintf(out,"Spanning weight: %.1f, edges=%zu\n",weight,edge_count);
        }
        if(rc==SD_OK && index==21){double *d=NULL;size_t *hop=NULL;int cycle=0;
            rc=floyd_warshall(g,&d,&hop,&cycle,&ops);if(rc==SD_OK)fprintf(out,"0->2 shortest: %.1f\n",d[2]);
        free(d);
        free(hop);}
        if(rc==SD_OK && index==22){double cost=0;size_t *tour=NULL,n=0;
            rc=tsp_dp(g,&cost,&tour,&n,&ops);if(rc==SD_OK)fprintf(out,"TSP cost: %.1f, tour length=%zu\n",cost,n);
        free(tour);}
        graph_matrix_free(g);
        break;
    }
    case 16: case 25: case 26: case 27: case 28: {
        int w[]={2,3,4};long long v[]={4,5,7};long long best=0;size_t *picked=NULL,n=0;
        if(index==16)rc=knapsack_01_dp(w,v,3,5,&best,&picked,&n,&ops);
        else if(index==25)rc=knapsack_01_backtracking(w,v,3,5,&best,&picked,&n,&ops);
        else {SDBBResult result={0};
            rc=index==26?knapsack_bb_fifo(w,v,3,5,&result,&ops):index==27?knapsack_bb_lifo(w,v,3,5,&result,&ops):knapsack_bb_least_cost(w,v,3,5,&result,&ops);
            if(rc==SD_OK)fprintf(out,"Best value: %lld; generated=%" PRIu64 "; expanded=%" PRIu64 "; frontier=%zu\n",result.best_value,result.nodes_generated,result.nodes_expanded,result.max_frontier);
            bb_result_free(&result);
        }
        if(rc==SD_OK && index<=25)fprintf(out,"Best value: %lld; selected=%zu\n",best,n);
        free(picked);
        break;
    }
    case 17: {size_t dims[]={10,20,5,15};uint64_t cost=0;char *order=NULL;
        rc=matrix_chain_order(dims,3,&cost,&order,&ops);
        if(rc==SD_OK)fprintf(out,"Scalar multiplications: %" PRIu64 "; order: %s\n",cost,order);
        free(order);
        break;}
    case 18: {const char *a[]={"river","level","rose"},*b[]={"river","levels","rose"};
        size_t len=0,*ia=NULL,*ib=NULL;double sim=0;
        rc=lcs_tokens(a,3,b,3,&len,&ia,&ib,&sim,&ops);
        if(rc==SD_OK)fprintf(out,"Common tokens: %zu; overlap %.3f\n",len,sim);
        free(ia);
        free(ib);
        break;}
    case 19: {double freq[]={0.3,0.5,0.2},cost=0;size_t *roots=NULL;
        rc=optimal_bst(freq,3,&cost,&roots,&ops);if(rc==SD_OK)fprintf(out,"Weighted search cost: %.3f\n",cost);
        free(roots);
        break;}
    case 23: {uint64_t solutions=0;size_t *board=NULL;
        rc=n_queens(4,&solutions,&board,&ops);
        if(rc==SD_OK)fprintf(out,"4-Queens solutions: %" PRIu64 "\n",solutions);
        free(board);
        break;}
    case 24: {int a[]={1,2,3};SDSubset *solutions=NULL;size_t n=0;
        rc=sum_of_subsets(a,3,3,&solutions,&n,&ops);
        if(rc==SD_OK)fprintf(out,"Subsets summing to 3: %zu\n",n);
        subsets_free(solutions,n);
        break;}
    case 29: {int64_t cap[]={0,3,1,0,0,2,0,0,0};int64_t flow=0,*edges=NULL;
        rc=ford_fulkerson(cap,3,0,2,&flow,&edges,&ops);
        if(rc==SD_OK)fprintf(out,"Maximum flow: %" PRId64 "\n",flow);
        free(edges);
        break;}
    case 30: {unsigned char adj[]={1,0,1,1};size_t size=0,*l=NULL,*r=NULL;
        rc=max_bipartite_matching(adj,2,2,&size,&l,&r,&ops);
        if(rc==SD_OK)fprintf(out,"Assigned pairs: %zu\n",size);
        free(l);
        free(r);
        break;}
    case 31: {uint64_t mod[]={3,5,7},res[]={2,3,2},value=0,range=0;
        rc=reconstruct_ingestion_counter(res,mod,3,&value,&range,&ops);
        if(rc==SD_OK)fprintf(out,"Reconstructed counter: %" PRIu64 " of %" PRIu64 "\n",value,range);
        break;}
    case 32: case 33: case 34: case 35: {
        const char *text="aaaaa",*pattern="aaa";size_t pos[8],found=0;
        typedef int (*Matcher)(const char *,size_t,const char *,size_t,size_t *,size_t,size_t *,uint64_t *);
        Matcher matchers[]={naive_search,rabin_karp_search,automaton_search,kmp_search};
        rc=matchers[index-32](text,5,pattern,3,pos,8,&found,&ops);
        if(rc==SD_OK)fprintf(out,"Overlapping matches: %zu (first=%zu)\n",found,found?pos[0]:0);
        break;
    }
    default: rc=SD_ERR_INVALID_ARGUMENT;break;
    }
    fprintf(out,"Operation count: %" PRIu64 "; status: %d\n",ops,rc);
    return ferror(out)?SD_ERR_INVALID_ARGUMENT:rc;
}
int workbench_run_all(FILE *out) {
    if(!out)return SD_ERR_INVALID_ARGUMENT;
    for(size_t i=0;i<WB_COUNT;i++){
        int rc=workbench_run(i,out);
        if(rc!=SD_OK)return rc;
    }
    return SD_OK;
}
