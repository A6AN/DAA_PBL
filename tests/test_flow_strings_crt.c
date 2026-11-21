#include "test_harness.h"
#include "graph_algorithms.h"
#include "string_matching.h"
#include "number_theory.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>

typedef int (*Searcher)(const char *,size_t,const char *,size_t,size_t *,size_t,size_t *,uint64_t *);
static Searcher searchers[]={naive_search,rabin_karp_search,automaton_search,kmp_search};

static void check_text(const char *text,size_t tlen,const char *pat,size_t plen) {
    size_t ref[128]={0},actual[128]={0},ref_n=0;
    ASSERT_EQ_INT(SD_OK,naive_search(text,tlen,pat,plen,ref,128,&ref_n,NULL));
    for(size_t k=0;k<4;++k) {
        size_t found=999;
        uint64_t ops=0;
        ASSERT_EQ_INT(SD_OK,searchers[k](text,tlen,pat,plen,actual,128,&found,&ops));
        ASSERT_EQ_INT(ref_n,found);
        for(size_t i=0;i<found && i<128;++i) ASSERT_EQ_INT(ref[i],actual[i]);
        size_t tiny[2]={SIZE_MAX,SIZE_MAX},count=0;
        ASSERT_EQ_INT(SD_OK,searchers[k](text,tlen,pat,plen,tiny,2,&count,NULL));
        ASSERT_EQ_INT(ref_n,count);
        for(size_t i=0;i<count && i<2;++i) ASSERT_EQ_INT(ref[i],tiny[i]);
    }
}
static void test_strings(void) {
    check_text("aaaaa",5,"aaa",3);
    check_text("abcabc",6,"abc",3);
    check_text("abc",3,"",0);
    check_text("ab",2,"abcd",4);
    check_text("bbbb",4,"a",1);
    check_text(NULL,0,NULL,0);
    check_text("a\0a",3,"\0a",2);
    lcg_seed(50801);
    for(size_t trial=0;trial<300;++trial) {
        char text[96],pat[24];
        size_t tn=(size_t)lcg_range(0,90),pn=(size_t)lcg_range(0,20);
        for(size_t i=0;i<tn;++i) text[i]=(char)('a'+lcg_range(0,1));
        for(size_t j=0;j<pn;++j) pat[j]=(char)('a'+lcg_range(0,1));
        check_text(text,tn,pat,pn);
    }
    size_t positions[8],found=0;
    uint64_t spurious=42;
    ASSERT_EQ_INT(SD_OK,rabin_karp_search_stats("aaaaa",5,"aaa",3,
                  positions,8,&found,&spurious,NULL));
    ASSERT_EQ_INT(3,found);ASSERT_EQ_INT(0,spurious);
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT,naive_search(NULL,1,"x",1,positions,8,&found,NULL));
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT,kmp_search("abc",3,"a",1,NULL,1,&found,NULL));
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT,automaton_search("abc",3,"a",1,positions,8,NULL,NULL));
}

static size_t oracle_matching_rec(const unsigned char *adj,size_t l,size_t r,
                                   size_t i,unsigned used) {
    if(i==l) return 0;
    size_t best=oracle_matching_rec(adj,l,r,i+1,used);
    for(size_t j=0;j<r;++j)
        if(adj[i*r+j] && !(used&(1u<<j))) {
            size_t a=1+oracle_matching_rec(adj,l,r,i+1,used|(1u<<j));
            if(a>best)best=a;
        }
    return best;
}
static void test_matching(void) {
    size_t size=999,*left=NULL,*right=NULL;
    ASSERT_EQ_INT(SD_OK,max_bipartite_matching(NULL,0,3,&size,&left,&right,NULL));
    ASSERT_EQ_INT(0,size);ASSERT_TRUE(left==NULL);free(right);
    unsigned char none[6]={0};
    ASSERT_EQ_INT(SD_OK,max_bipartite_matching(none,2,3,&size,&left,&right,NULL));
    ASSERT_EQ_INT(0,size);free(left);free(right);
    lcg_seed(663);
    for(size_t trial=0;trial<300;++trial) {
        size_t l=(size_t)lcg_range(0,6),r=(size_t)lcg_range(0,6);
        unsigned char adj[36]={0};
        for(size_t k=0;k<l*r;++k) adj[k]=(unsigned char)lcg_range(0,1);
        uint64_t ops=0;
        ASSERT_EQ_INT(SD_OK,max_bipartite_matching(adj,l,r,&size,&left,&right,&ops));
        ASSERT_EQ_INT(oracle_matching_rec(adj,l,r,0,0),size);
        size_t seen=0;
        for(size_t i=0;i<l;++i) if(left[i]!=SIZE_MAX) {
            ASSERT_TRUE(left[i]<r);
            ASSERT_TRUE(adj[i*r+left[i]]!=0);
            ASSERT_EQ_INT(i,right[left[i]]);
            ++seen;
        }
        ASSERT_EQ_INT(size,seen);
        free(left);free(right);
    }
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT,max_bipartite_matching(NULL,2,2,&size,&left,&right,NULL));
}

/* Independent Edmonds-Karp oracle for small capacities, using BFS. */
static int64_t flow_oracle(const int64_t *cap,size_t n,size_t s,size_t t) {
    int64_t res[64]; memcpy(res,cap,n*n*sizeof(*cap));
    int64_t sum=0;
    for(;;) {
        size_t parent[8],queue[8],front=0,back=0;
        for(size_t i=0;i<n;++i)parent[i]=SIZE_MAX;
        parent[s]=s;queue[back++]=s;
        while(front<back && parent[t]==SIZE_MAX) {
            size_t u=queue[front++];
            for(size_t v=0;v<n;++v) if(res[u*n+v]>0 && parent[v]==SIZE_MAX) {
                parent[v]=u;queue[back++]=v;
            }
        }
        if(parent[t]==SIZE_MAX)break;
        int64_t add=INT64_MAX;
        for(size_t v=t;v!=s;v=parent[v]) if(res[parent[v]*n+v]<add)add=res[parent[v]*n+v];
        for(size_t v=t;v!=s;v=parent[v]) {
            size_t u=parent[v];res[u*n+v]-=add;res[v*n+u]+=add;
        }
        sum+=add;
    }
    return sum;
}
static void verify_flow(const int64_t *capacity,size_t n,size_t s,size_t t) {
    int64_t max=0,*flow=NULL;
    uint64_t ops=0;
    ASSERT_EQ_INT(SD_OK,ford_fulkerson(capacity,n,s,t,&max,&flow,&ops));
    ASSERT_EQ_INT(flow_oracle(capacity,n,s,t),max);
    if(flow) {
        for(size_t u=0;u<n;++u) {
            int64_t outgoing=0,incoming=0;
            for(size_t v=0;v<n;++v) {
                ASSERT_TRUE(flow[u*n+v]>=0 && flow[u*n+v]<=capacity[u*n+v]);
                outgoing+=flow[u*n+v];incoming+=flow[v*n+u];
            }
            if(u==s) ASSERT_EQ_INT(max,outgoing-incoming);
            else if(u==t) ASSERT_EQ_INT(max,incoming-outgoing);
            else ASSERT_EQ_INT(incoming,outgoing);
        }
    }
    free(flow);
}
static void test_flow(void) {
    const int64_t textbook[36]={
        0,16,13,0,0,0,
        0,0,10,12,0,0,
        0,4,0,0,14,0,
        0,0,9,0,0,20,
        0,0,0,7,0,4,
        0,0,0,0,0,0
    };
    verify_flow(textbook,6,0,5);
    const int64_t single[]={0,7,0,0};verify_flow(single,2,0,1);
    const int64_t zero[]={0,0,0,0};verify_flow(zero,2,0,1);
    const int64_t disconnected[]={0,4,0,0,0,0,0,0,0};verify_flow(disconnected,3,0,2);
    const int64_t opposite[]={0,5,1,3,0,5,4,1,0};verify_flow(opposite,3,0,2);
    int64_t bad[]={0,-1,0,0},max=0,*edges=NULL;
    ASSERT_EQ_INT(SD_ERR_NEGATIVE_WEIGHT,ford_fulkerson(bad,2,0,1,&max,&edges,NULL));
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT,ford_fulkerson(single,2,0,0,&max,&edges,NULL));
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT,ford_fulkerson(NULL,2,0,1,&max,&edges,NULL));
    lcg_seed(991);
    for(size_t trial=0;trial<200;++trial) {
        int64_t cap[36]={0};size_t n=(size_t)lcg_range(2,6);
        for(size_t i=0;i<n;++i)for(size_t j=0;j<n;++j)
            if(i!=j)cap[i*n+j]=(int64_t)lcg_range(0,7);
        verify_flow(cap,n,0,n-1);
    }
}
static void test_crt(void) {
    uint64_t value=0,range=0;
    ASSERT_EQ_INT(SD_OK,crt_solve(NULL,NULL,0,&value,&range,NULL));
    ASSERT_EQ_INT(0,value);ASSERT_EQ_INT(1,range);
    uint64_t mod[]={3,5,7},rem[]={2,3,2};
    ASSERT_EQ_INT(SD_OK,crt_solve(rem,mod,3,&value,&range,NULL));
    ASSERT_EQ_INT(23,value);ASSERT_EQ_INT(105,range);
    ASSERT_EQ_INT(SD_OK,reconstruct_ingestion_counter(rem,mod,3,&value,&range,NULL));
    ASSERT_EQ_INT(23,value);
    uint64_t badmod[]={4,6},badrem[]={1,3};
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT,crt_solve(badrem,badmod,2,&value,&range,NULL));
    uint64_t huge[]={UINT64_MAX,2},residues[]={1,1};
    ASSERT_EQ_INT(SD_ERR_OVERFLOW,crt_solve(residues,huge,2,&value,&range,NULL));
    uint64_t zero[]={0};ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT,crt_solve(zero,zero,1,&value,&range,NULL));
    lcg_seed(4711);
    for(size_t trial=0;trial<300;++trial) {
        uint64_t primes[]={2,3,5,7,11,13};
        size_t n=(size_t)lcg_range(1,4);
        uint64_t m[4],r[4],prod=1;
        for(size_t i=0;i<n;++i) { m[i]=primes[i+(trial%3)];prod*=m[i];r[i]=lcg_range(0,m[i]-1); }
        uint64_t brute=0;
        for(;brute<prod;++brute) {
            size_t j=0;
            for(;j<n && brute%m[j]==r[j];++j){}
            if(j==n) break;
        }
        uint64_t ops=0;
        ASSERT_EQ_INT(SD_OK,crt_solve(r,m,n,&value,&range,&ops));
        ASSERT_EQ_INT(prod,range);ASSERT_EQ_INT(brute,value);
    }
    /* Large modulus tests exercise full-width modular inverse arithmetic. */
    uint64_t wide_m[]={UINT64_C(18446744073709551557)};
    uint64_t wide_r[]={UINT64_C(18446744073709551556)};
    ASSERT_EQ_INT(SD_OK,crt_solve(wide_r,wide_m,1,&value,&range,NULL));
    ASSERT_TRUE(value==wide_r[0] && range==wide_m[0]);
}
int main(void) {
    test_strings();test_matching();test_flow();test_crt();
    return harness_summary();
}
