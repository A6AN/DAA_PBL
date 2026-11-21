#include "greedy.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define COUNT(o) do { if (o) ++*(o); } while (0)
typedef struct { size_t frequency; int left, right, symbol; } HNode;
typedef struct { int id; size_t freq; } HEntry;
typedef struct { HEntry a[512]; size_t n; } HHeap;
static int hless(HEntry a,HEntry b) {
    return a.freq<b.freq || (a.freq==b.freq && a.id<b.id);
}
static void push(HHeap *h,HEntry x,uint64_t *ops) {
    size_t i=h->n++;
    while (i) {
        size_t p=(i-1)/2;COUNT(ops);
        if (hless(h->a[p],x)) break;
        h->a[i]=h->a[p];i=p;
    }
    h->a[i]=x;
}
static HEntry pop(HHeap *h,uint64_t *ops) {
    HEntry result=h->a[0],last=h->a[--h->n];
    if (!h->n) return result;
    size_t i=0;
    for (;;) {
        size_t l=2*i+1,r=l+1,b=l;
        if (l>=h->n) break;
        COUNT(ops);
        if (r<h->n && hless(h->a[r],h->a[l])) b=r;
        COUNT(ops);
        if (hless(last,h->a[b])) break;
        h->a[i]=h->a[b];i=b;
    }
    h->a[i]=last;return result;
}
static void fill_codes(const HNode *nodes,int id,SDHuffman *out,
                       unsigned char path[256],size_t depth) {
    if (nodes[id].symbol>=0) {
        int sym=nodes[id].symbol;
        if (!depth) {path[0]=0;depth=1;}
        out->code_lengths[sym]=(unsigned char)depth;
        memcpy(out->code_bits[sym],path,depth);
        return;
    }
    path[depth]=0;fill_codes(nodes,nodes[id].left,out,path,depth+1);
    path[depth]=1;fill_codes(nodes,nodes[id].right,out,path,depth+1);
}
void huffman_free(SDHuffman *encoded) {
    if (!encoded) return;
    free(encoded->bytes);free(encoded);
}
int huffman_encode(const unsigned char *input,size_t n,SDHuffman **out,
                   double *ratio,uint64_t *ops) {
    if (!out || !ratio || (n && !input)) return SD_ERR_INVALID_ARGUMENT;
    *out=NULL;*ratio=0;
    SDHuffman *enc=checked_calloc(1,sizeof(*enc));
    if (!enc) return SD_ERR_OUT_OF_MEMORY;
    enc->original_size=n;
    size_t freq[256]={0};
    for (size_t i=0;i<n;++i) {++freq[input[i]];COUNT(ops);}
    HNode nodes[511];HHeap heap={0};int used=0;
    for (int s=0;s<256;++s) if (freq[s]) {
        nodes[used]=(HNode){freq[s],-1,-1,s};
        push(&heap,(HEntry){used,freq[s]},ops);++used;
    }
    while (heap.n>1) {
        HEntry a=pop(&heap,ops),b=pop(&heap,ops);
        if (a.freq>SIZE_MAX-b.freq) {huffman_free(enc);return SD_ERR_OVERFLOW;}
        nodes[used]=(HNode){a.freq+b.freq,a.id,b.id,-1};
        push(&heap,(HEntry){used,a.freq+b.freq},ops);++used;
    }
    if (heap.n) {
        unsigned char path[256]={0};
        fill_codes(nodes,heap.a[0].id,enc,path,0);
    }
    size_t bits=0;
    for (int s=0;s<256;++s) if (freq[s]) {
        size_t len=enc->code_lengths[s];
        if (freq[s]>SIZE_MAX/len || bits>SIZE_MAX-freq[s]*len) {
            huffman_free(enc);return SD_ERR_OVERFLOW;
        }
        bits+=freq[s]*len;
    }
    enc->bit_count=bits;
    enc->byte_count=bits/8+(bits%8!=0);
    enc->bytes=checked_calloc(enc->byte_count,1);
    if (!enc->bytes) {huffman_free(enc);return SD_ERR_OUT_OF_MEMORY;}
    size_t pos=0;
    for (size_t i=0;i<n;++i) {
        unsigned char sym=input[i];
        for (size_t j=0;j<enc->code_lengths[sym];++j) {
            if (enc->code_bits[sym][j]) enc->bytes[pos/8]|=(unsigned char)(1u<<(7-(pos%8)));
            ++pos;COUNT(ops);
        }
    }
    *ratio=n ? (double)enc->byte_count/(double)n : 0;
    *out=enc;return SD_OK;
}

typedef struct {int child[2];int symbol;} DecodeNode;
int huffman_decode(const SDHuffman *enc,unsigned char **decoded,
                   size_t *decoded_size,uint64_t *ops) {
    if (!enc || !decoded || !decoded_size || enc->byte_count!=enc->bit_count/8+(enc->bit_count%8!=0) ||
        (enc->byte_count && !enc->bytes)) return SD_ERR_INVALID_ARGUMENT;
    *decoded=NULL;*decoded_size=0;
    size_t nodes_cap=1;
    for (int s=0;s<256;++s) {
        if (nodes_cap>SIZE_MAX-enc->code_lengths[s]) return SD_ERR_OVERFLOW;
        nodes_cap+=enc->code_lengths[s];
    }
    if (nodes_cap>SIZE_MAX/sizeof(DecodeNode)) return SD_ERR_OVERFLOW;
    DecodeNode *nodes=checked_malloc(nodes_cap*sizeof(*nodes));
    if (!nodes) return SD_ERR_OUT_OF_MEMORY;
    nodes[0]=(DecodeNode){{-1,-1},-1};size_t used=1;
    int status=SD_OK;
    for (int s=0;s<256;++s) {
        size_t len=enc->code_lengths[s];
        if (!len) continue;
        size_t node=0;
        for (size_t j=0;j<len;++j) {
            unsigned char bit=enc->code_bits[s][j];
            if (bit>1 || nodes[node].symbol>=0) {status=SD_ERR_INVALID_ARGUMENT;goto done;}
            int child=nodes[node].child[bit];
            if (child<0) {
                if (used>=nodes_cap) {status=SD_ERR_INVALID_ARGUMENT;goto done;}
                child=(int)used++;
                nodes[child]=(DecodeNode){{-1,-1},-1};
                nodes[node].child[bit]=child;
            }
            node=(size_t)child;
        }
        if (nodes[node].symbol>=0 || nodes[node].child[0]>=0 || nodes[node].child[1]>=0) {
            status=SD_ERR_INVALID_ARGUMENT;goto done;
        }
        nodes[node].symbol=s;
    }
    unsigned char *output=checked_malloc(enc->original_size);
    if (!output) {status=SD_ERR_OUT_OF_MEMORY;goto done;}
    size_t produced=0,node=0;
    for (size_t b=0;b<enc->bit_count;++b) {
        unsigned bit=(enc->bytes[b/8]>>(7-(b%8)))&1u;
        int next=nodes[node].child[bit];COUNT(ops);
        if (next<0) {status=SD_ERR_INVALID_ARGUMENT;break;}
        node=(size_t)next;
        if (nodes[node].symbol>=0) {
            if (produced>=enc->original_size) {status=SD_ERR_INVALID_ARGUMENT;break;}
            output[produced++]=(unsigned char)nodes[node].symbol;
            node=0;
        }
    }
    if (status==SD_OK && (node || produced!=enc->original_size)) status=SD_ERR_INVALID_ARGUMENT;
    if (status==SD_OK) {*decoded=output;*decoded_size=produced;}
    else free(output);
done:
    free(nodes);return status;
}
