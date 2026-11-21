#include "number_theory.h"
#include <limits.h>

/* For 0 <= a,b < mod, this avoids overflow on a+b. */
static uint64_t add_mod(uint64_t a,uint64_t b,uint64_t mod) {
    return a >= mod-b ? a-(mod-b) : a+b;
}
static uint64_t mul_mod(uint64_t a,uint64_t b,uint64_t mod,uint64_t *ops) {
    uint64_t result=0;
    while(b) {
        if(ops) ++*ops;
        if(b&1) result=add_mod(result,a,mod);
        b>>=1;
        if(b) a=add_mod(a,a,mod);
    }
    return result;
}
/* Extended Euclid with coefficients kept modulo mod, avoiding signed
   coefficient overflow even for moduli above INT64_MAX. */
static int inverse_mod(uint64_t a,uint64_t mod,uint64_t *inverse,uint64_t *ops) {
    if(mod==1) { *inverse=0;return SD_OK; }
    uint64_t old_r=mod,r=a%mod;
    uint64_t old_t=0,t=1;
    while(r) {
        uint64_t q=old_r/r;
        uint64_t rem=old_r%r;
        old_r=r;r=rem;
        uint64_t prod=mul_mod(q%mod,t,mod,ops);
        uint64_t next=old_t>=prod ? old_t-prod : mod-(prod-old_t);
        old_t=t;t=next;
        if(ops) ++*ops;
    }
    if(old_r!=1) return SD_ERR_INVALID_ARGUMENT;
    *inverse=old_t;
    return SD_OK;
}
int crt_solve(const uint64_t *remainders,const uint64_t *moduli,size_t n,
              uint64_t *value,uint64_t *product,uint64_t *ops) {
    if(!value || !product || (n && (!remainders || !moduli)))
        return SD_ERR_INVALID_ARGUMENT;
    *value=0; *product=1;
    uint64_t x=0,M=1;
    for(size_t i=0;i<n;++i) {
        uint64_t m=moduli[i];
        if(!m) return SD_ERR_INVALID_ARGUMENT;
        if(M>UINT64_MAX/m) return SD_ERR_OVERFLOW;
        uint64_t inv=0;
        if(inverse_mod(M%m,m,&inv,ops)!=SD_OK) return SD_ERR_INVALID_ARGUMENT;
        uint64_t residue=remainders[i]%m;
        uint64_t xm=x%m;
        uint64_t delta=residue>=xm ? residue-xm : m-(xm-residue);
        uint64_t step=mul_mod(delta,inv,m,ops);
        /* x < M and step < m, and M*m fits uint64_t. */
        x+=M*step;
        M*=m;
    }
    *value=x;*product=M;
    return SD_OK;
}
int reconstruct_ingestion_counter(const uint64_t *partition_remainders,
                                  const uint64_t *partition_moduli,
                                  size_t partitions,uint64_t *counter,
                                  uint64_t *range,uint64_t *ops) {
    return crt_solve(partition_remainders,partition_moduli,partitions,counter,range,ops);
}
