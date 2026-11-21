#include "string_matching.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>

static int valid(const char *text,size_t tlen,const char *pat,size_t plen,
                 size_t *out,size_t cap,size_t *found) {
    return found && (text || !tlen) && (pat || !plen) &&
           (out || !cap) && (plen != SIZE_MAX);
}
static void hit(size_t position,size_t *out,size_t cap,size_t *found) {
    if (*found < cap) out[*found] = position;
    ++*found;
}
static int empty_or_long(size_t tlen,size_t plen,size_t *out,size_t cap,
                         size_t *found,int *done) {
    *found=0; *done=1;
    if (!plen) {
        if (tlen==SIZE_MAX) return SD_ERR_OVERFLOW;
        for(size_t i=0;i<=tlen;++i) hit(i,out,cap,found);
    } else if (plen<=tlen) *done=0;
    return SD_OK;
}
int naive_search(const char *text,size_t tlen,const char *pat,size_t plen,
                 size_t *out,size_t cap,size_t *found,uint64_t *ops) {
    if (!valid(text,tlen,pat,plen,out,cap,found)) return SD_ERR_INVALID_ARGUMENT;
    int done; int rc=empty_or_long(tlen,plen,out,cap,found,&done);
    if (rc || done) return rc;
    for(size_t i=0;i<=tlen-plen;++i) {
        size_t j=0;
        while(j<plen) {
            if (ops) ++*ops;
            if ((unsigned char)text[i+j]!=(unsigned char)pat[j]) break;
            ++j;
        }
        if(j==plen) hit(i,out,cap,found);
    }
    return SD_OK;
}

/* Modular addition and multiplication keep intermediates in uint64_t. */
#define RK_MOD UINT64_C(1000000007)
#define RK_BASE UINT64_C(257)
int rabin_karp_search_stats(const char *text,size_t tlen,const char *pat,
                            size_t plen,size_t *out,size_t cap,size_t *found,
                            uint64_t *spurious,uint64_t *ops) {
    if (!spurious || !valid(text,tlen,pat,plen,out,cap,found))
        return SD_ERR_INVALID_ARGUMENT;
    *spurious=0;
    int done; int rc=empty_or_long(tlen,plen,out,cap,found,&done);
    if (rc || done) return rc;
    uint64_t hp=0,ht=0,power=1;
    for(size_t i=0;i<plen;++i) {
        hp=(hp*RK_BASE+(unsigned char)pat[i])%RK_MOD;
        ht=(ht*RK_BASE+(unsigned char)text[i])%RK_MOD;
        if(i+1<plen) power=power*RK_BASE%RK_MOD;
        if(ops) ++*ops;
    }
    for(size_t i=0;i<=tlen-plen;++i) {
        if(ops) ++*ops;
        if(hp==ht) {
            size_t j=0;
            while(j<plen) {
                if(ops) ++*ops;
                if((unsigned char)text[i+j]!=(unsigned char)pat[j]) break;
                ++j;
            }
            if(j==plen) hit(i,out,cap,found);
            else ++*spurious;
        }
        if(i<tlen-plen) {
            uint64_t remove=((unsigned char)text[i]*power)%RK_MOD;
            ht=(ht+RK_MOD-remove)%RK_MOD;
            ht=(ht*RK_BASE+(unsigned char)text[i+plen])%RK_MOD;
        }
    }
    return SD_OK;
}
int rabin_karp_search(const char *text,size_t tlen,const char *pat,size_t plen,
                      size_t *out,size_t cap,size_t *found,uint64_t *ops) {
    uint64_t spurious=0;
    return rabin_karp_search_stats(text,tlen,pat,plen,out,cap,found,&spurious,ops);
}
static int prefix_table(const char *pat,size_t plen,size_t **prefix,uint64_t *ops) {
    if(plen > SIZE_MAX/sizeof(size_t)) return SD_ERR_OVERFLOW;
    size_t *pi=checked_calloc(plen,sizeof(*pi));
    if(!pi) return SD_ERR_OUT_OF_MEMORY;
    for(size_t i=1,j=0;i<plen;++i) {
        while(j && pat[i]!=pat[j]) {
            if(ops) ++*ops;
            j=pi[j-1];
        }
        if(ops) ++*ops;
        if(pat[i]==pat[j]) ++j;
        pi[i]=j;
    }
    *prefix=pi;
    return SD_OK;
}
int kmp_search(const char *text,size_t tlen,const char *pat,size_t plen,
               size_t *out,size_t cap,size_t *found,uint64_t *ops) {
    if(!valid(text,tlen,pat,plen,out,cap,found)) return SD_ERR_INVALID_ARGUMENT;
    int done; int rc=empty_or_long(tlen,plen,out,cap,found,&done);
    if(rc || done) return rc;
    size_t *pi=NULL;
    rc=prefix_table(pat,plen,&pi,ops);
    if(rc) return rc;
    size_t j=0;
    for(size_t i=0;i<tlen;++i) {
        while(j && text[i]!=pat[j]) {
            if(ops) ++*ops;
            j=pi[j-1];
        }
        if(ops) ++*ops;
        if(text[i]==pat[j]) ++j;
        if(j==plen) { hit(i+1-plen,out,cap,found); j=pi[j-1]; }
    }
    free(pi);
    return SD_OK;
}
int automaton_search(const char *text,size_t tlen,const char *pat,size_t plen,
                     size_t *out,size_t cap,size_t *found,uint64_t *ops) {
    if(!valid(text,tlen,pat,plen,out,cap,found)) return SD_ERR_INVALID_ARGUMENT;
    int done; int rc=empty_or_long(tlen,plen,out,cap,found,&done);
    if(rc || done) return rc;
    if(plen > SIZE_MAX/256/sizeof(size_t)-1) return SD_ERR_OVERFLOW;
    size_t *pi=NULL;
    rc=prefix_table(pat,plen,&pi,ops);
    if(rc) return rc;
    size_t *table=checked_calloc((plen+1)*256,sizeof(*table));
    if(!table) { free(pi); return SD_ERR_OUT_OF_MEMORY; }
    table[(unsigned char)pat[0]]=1;
    for(size_t state=1;state<=plen;++state) {
        size_t fallback=pi[state-1];
        memcpy(table+state*256,table+fallback*256,256*sizeof(size_t));
        if(state<plen) table[state*256+(unsigned char)pat[state]]=state+1;
        if(ops) *ops+=256;
    }
    size_t state=0;
    for(size_t i=0;i<tlen;++i) {
        state=table[state*256+(unsigned char)text[i]];
        if(ops) ++*ops;
        if(state==plen) hit(i+1-plen,out,cap,found);
    }
    free(table);free(pi);
    return SD_OK;
}
