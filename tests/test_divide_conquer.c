#include "divide_conquer.h"
#include "test_harness.h"
#include <limits.h>
#include <math.h>
#include <stdlib.h>

static void max_oracle(const long long *a, size_t n, size_t *start,
                       size_t *end, long long *sum) {
    *sum = LLONG_MIN;
    for (size_t i = 0; i < n; ++i) {
        long long value = 0;
        for (size_t j = i; j < n; ++j) {
            value += a[j];
            if (value > *sum || (value == *sum &&
                (i < *start || (i == *start && j < *end)))) {
                *start=i; *end=j; *sum=value;
            }
        }
    }
}
static void check_subarray(const long long *a, size_t n) {
    size_t es=0,ee=0,s=0,e=0;
    long long expected=0,sum=0;
    max_oracle(a,n,&es,&ee,&expected);
    ASSERT_EQ_INT(SD_OK,max_subarray(a,n,&s,&e,&sum,NULL));
    ASSERT_EQ_INT(es,s);
    ASSERT_EQ_INT(ee,e);
    ASSERT_EQ_INT(expected,sum);
}
static long long pow_oracle(long long a, unsigned e) {
    long long out=1;
    for (unsigned i=0;i<e;++i) out*=a;
    return out;
}
static long long mod_oracle(long long base, unsigned e, long long m) {
    long long result=1%m, x=base%m;
    if (x<0) x+=m;
    for (unsigned i=0;i<e;++i) result=(result*x)%m;
    return result;
}
static ptrdiff_t celebrity_oracle(const unsigned char *a,size_t n) {
    for (size_t c=0;c<n;++c) {
        int celebrity=1;
        for (size_t i=0;i<n;++i) if (i!=c &&
            (a[c*n+i] || !a[i*n+c])) celebrity=0;
        if (celebrity) return (ptrdiff_t)c;
    }
    return -1;
}
static void check_celebrity(const unsigned char *a,size_t n) {
    ptrdiff_t result=-2;
    ASSERT_EQ_INT(SD_OK,find_celebrity(a,n,&result,NULL));
    ASSERT_EQ_INT(celebrity_oracle(a,n),result);
}
static void check_matrices(size_t n) {
    double a[64]={0},b[64]={0},reference[64]={0},actual[64]={0};
    for (size_t i=0;i<n*n;++i) {
        a[i]=(double)((int)lcg_range(0,10)-5)/8.0;
        b[i]=(double)((int)lcg_range(0,10)-5)/8.0;
    }
    ASSERT_EQ_INT(SD_OK,matrix_multiply_classical(a,b,reference,n,NULL));
    ASSERT_EQ_INT(SD_OK,matrix_multiply_strassen(a,b,actual,n,NULL));
    for (size_t i=0;i<n*n;++i) ASSERT_EQ_DOUBLE(reference[i],actual[i],1e-9);
}
int main(void) {
    lcg_seed(10923);
    long long neg[]={-8,-3,-7,-1,-5}, sample[]={-2,1,-3,4,-1,2,1,-5,4},
        one[]={-9}, same[]={0,0,0,0};
    check_subarray(neg,5);
    check_subarray(sample,9);
    check_subarray(one,1);
    check_subarray(same,4);
    for (int trial=0;trial<200;++trial) {
        long long a[30];
        size_t n=(size_t)lcg_range(1,30);
        for (size_t i=0;i<n;++i) a[i]=(long long)lcg_range(0,20)-10;
        check_subarray(a,n);
    }
    long long value=0;
    double floating=0;
    ASSERT_EQ_INT(SD_OK,fast_power(0,0,&value,NULL));
    ASSERT_EQ_INT(1,value);
    ASSERT_EQ_INT(SD_ERR_OVERFLOW,fast_power(LLONG_MAX,2,&value,NULL));
    ASSERT_EQ_INT(SD_OK,fast_power(LLONG_MIN,1,&value,NULL));
    ASSERT_EQ_INT(LLONG_MIN,value);
    for (int trial=0;trial<200;++trial) {
        long long base=(long long)lcg_range(0,10)-5;
        unsigned exp=(unsigned)lcg_range(0,8);
        long long m=(long long)lcg_range(1,1000);
        ASSERT_EQ_INT(SD_OK,fast_power(base,exp,&value,NULL));
        ASSERT_EQ_INT(pow_oracle(base,exp),value);
        ASSERT_EQ_INT(SD_OK,fast_power_mod(base,exp,m,&value,NULL));
        ASSERT_EQ_INT(mod_oracle(base,exp,m),value);
        ASSERT_EQ_INT(SD_OK,fast_power_double((double)base,exp,&floating,NULL));
        ASSERT_EQ_DOUBLE((double)pow_oracle(base,exp),floating,1e-9);
        if (base) {
            ASSERT_EQ_INT(SD_OK,fast_power_double((double)base,-(long long)exp,&floating,NULL));
            ASSERT_EQ_DOUBLE(1.0/(double)pow_oracle(base,exp),floating,1e-9);
        }
    }
    ASSERT_EQ_INT(SD_OK,fast_power_mod(LLONG_MIN,17,LLONG_MAX,&value,NULL));
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT,fast_power_mod(3,2,0,&value,NULL));
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT,fast_power(2,3,NULL,NULL));
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT,fast_power_double(0,-1,&floating,NULL));
    unsigned char known[]={0,1,0, 0,0,0, 0,1,0};
    unsigned char absent[]={0,1,1,0};
    unsigned char only[]={1};
    check_celebrity(NULL,0);
    check_celebrity(only,1);
    check_celebrity(known,3);
    check_celebrity(absent,2);
    for (int trial=0;trial<200;++trial) {
        unsigned char a[64]={0};
        size_t n=(size_t)lcg_range(1,8);
        for (size_t i=0;i<n*n;++i) a[i]=(unsigned char)lcg_range(0,1);
        if (trial%2==0) {
            size_t c=(size_t)lcg_range(0,n-1);
            for (size_t i=0;i<n;++i) if (i!=c) {
                a[c*n+i]=0; a[i*n+c]=1;
            }
        }
        check_celebrity(a,n);
    }
    size_t sizes[]={1,2,3,5,7,8};
    for (size_t k=0;k<sizeof(sizes)/sizeof(sizes[0]);++k)
        check_matrices(sizes[k]);
    for (int trial=0;trial<200;++trial)
        check_matrices((size_t)lcg_range(1,8));
    /* Force the recursive Strassen branch, not only its classical cutoff. */
    {
        enum { N=33 };
        double a[N*N]={0},b[N*N]={0},c[N*N]={0},d[N*N]={0};
        for (size_t i=0;i<N*N;++i) {
            a[i]=(double)((int)lcg_range(0,4)-2)/8.0;
            b[i]=(double)((int)lcg_range(0,4)-2)/8.0;
        }
        ASSERT_EQ_INT(SD_OK,matrix_multiply_classical(a,b,c,N,NULL));
        ASSERT_EQ_INT(SD_OK,matrix_multiply_strassen(a,b,d,N,NULL));
        for (size_t i=0;i<N*N;++i) ASSERT_EQ_DOUBLE(c[i],d[i],1e-9);
    }
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT,max_subarray(NULL,1,NULL,NULL,NULL,NULL));
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT,max_subarray(one,0,NULL,NULL,NULL,NULL));
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT,find_celebrity(NULL,2,NULL,NULL));
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT,matrix_multiply_classical(NULL,NULL,NULL,2,NULL));
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT,matrix_multiply_strassen(NULL,NULL,NULL,2,NULL));
    ASSERT_EQ_INT(SD_OK,matrix_multiply_strassen(NULL,NULL,NULL,0,NULL));
    return harness_summary();
}
