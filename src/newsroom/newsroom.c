#include "newsroom.h"
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

static int article_has_tag(const Article *a,const char *tag) {
    for(int i=0;i<a->tag_count;i++) if(strcmp(a->tags[i],tag)==0) return 1;
    return 0;
}
/* Return one representative overlap passage, preserving the token order. */
static int similarity(const Article *a,const Article *b,double *score,char *passage,
                      uint64_t *ops) {
    char **ah=NULL,**bh=NULL,**ab=NULL,**bb=NULL;
    size_t na=0,nb=0,nab=0,nbb=0;
    size_t *ia=NULL,*ib=NULL,length=0;
    double head=0,body=0;int rc=tokenize_text(a->headline,&ah,&na);
    if(rc!=SD_OK) goto done;
    rc=tokenize_text(b->headline,&bh,&nb);if(rc!=SD_OK) goto done;
    rc=lcs_tokens((const char *const *)ah,na,(const char *const *)bh,nb,
                  &length,&ia,&ib,&head,ops);
    if(rc!=SD_OK) goto done;
    size_t used=0;
    for(size_t i=0;i<length;i++) {
        size_t k=strlen(ah[ia[i]]);
        if(used+k+(used?1:0)>=MAX_HEADLINE) break;
        if(used) passage[used++]=' ';
        memcpy(passage+used,ah[ia[i]],k);used+=k;
    }
    passage[used]='\0';free(ia);free(ib);ia=NULL;ib=NULL;
    rc=tokenize_text(a->body,&ab,&nab);if(rc!=SD_OK) goto done;
    rc=tokenize_text(b->body,&bb,&nbb);if(rc!=SD_OK) goto done;
    rc=lcs_tokens((const char *const *)ab,nab,(const char *const *)bb,nbb,
                  &length,&ia,&ib,&body,ops);
    if(rc==SD_OK) {
        if(body>head) {
            size_t used_body=0;
            for(size_t i=0;i<length;i++) {
                size_t k=strlen(ab[ia[i]]);
                if(used_body+k+(used_body?1:0)>=MAX_HEADLINE) break;
                if(used_body) passage[used_body++]=' ';
                memcpy(passage+used_body,ab[ia[i]],k);used_body+=k;
            }
            passage[used_body]='\0';
        }
        *score=0.5*(head+body);
    }
done:
    free(ia);free(ib);free_tokens(ah,na);free_tokens(bh,nb);
    free_tokens(ab,nab);free_tokens(bb,nbb);return rc;
}
int newsroom_overlaps(const ArticleStore *s,OverlapPair **pairs,size_t *count,uint64_t *ops) {
    if(!s||!pairs||!count) return SD_ERR_INVALID_ARGUMENT;
    *pairs=NULL;*count=0;size_t cap=0;
    for(size_t i=0;i<s->count;i++) for(size_t j=i+1;j<s->count;j++) {
        OverlapPair candidate={0};
        int rc=similarity(&s->items[i],&s->items[j],&candidate.score,candidate.passage,ops);
        if(rc!=SD_OK) {free(*pairs);*pairs=NULL;*count=0;return rc;}
        if(candidate.score<SD_DUPLICATE_THRESHOLD) continue;
        if(*count==cap) {
            size_t next=cap?cap*2:8;
            if(next<cap||next>SIZE_MAX/sizeof(candidate)) {free(*pairs);*pairs=NULL;*count=0;return SD_ERR_OVERFLOW;}
            OverlapPair *grown=checked_realloc(*pairs,next*sizeof(candidate));
            if(!grown) {free(*pairs);*pairs=NULL;*count=0;return SD_ERR_OUT_OF_MEMORY;}
            *pairs=grown;cap=next;
        }
        candidate.article_a=s->items[i].id;candidate.article_b=s->items[j].id;
        (*pairs)[(*count)++]=candidate;
    }
    return SD_OK;
}
int newsroom_dashboard(const ArticleStore *a,const SourceTable *s,const EventTable *e,
                       const CheckerTable *c,DashboardStats *stats) {
    if(!a||!s||!e||!c||!stats) return SD_ERR_INVALID_ARGUMENT;
    memset(stats,0,sizeof(*stats));stats->articles=a->count;stats->sources=s->count;
    stats->events=e->count;
    for(size_t i=0;i<a->count;i++) {
        if(a->items[i].verification_status==VERIFY_PENDING) stats->awaiting_verification++;
        if(a->items[i].urgency_score>=75.0) stats->high_urgency++;
    }
    for(size_t i=0;i<c->count;i++) {
        if(c->items[i].available_capacity>c->items[i].assigned_workload)
            stats->available_capacity+=(size_t)(c->items[i].available_capacity-c->items[i].assigned_workload);
    }
    OverlapPair *pairs=NULL;size_t n=0;int rc=newsroom_overlaps(a,&pairs,&n,NULL);
    if(rc==SD_OK) stats->candidate_pairs=n;
    free(pairs);return rc;
}
static int timeline(const ArticleStore *s,int event_id,const char *tag,
                    Article ***items,size_t *count,uint64_t *ops) {
    if(!s||!items||!count) return SD_ERR_INVALID_ARGUMENT;
    *items=NULL;*count=0;
    Article **all=NULL;int rc=article_store_sort(s,ARTICLE_BY_PUBLICATION_TIME,0,&all,ops);
    if(rc!=SD_OK) return rc;
    for(size_t i=0;i<s->count;i++) {
        if(tag?article_has_tag(all[i],tag):all[i]->event_id==event_id) all[(*count)++]=all[i];
    }
    *items=all;return SD_OK;
}
int newsroom_timeline(const ArticleStore *s,int event_id,Article ***items,
                      size_t *count,uint64_t *ops) {
    return timeline(s,event_id,NULL,items,count,ops);
}
int newsroom_tag_timeline(const ArticleStore *s,const char *tag,Article ***items,
                          size_t *count,uint64_t *ops) {
    if(!tag) return SD_ERR_INVALID_ARGUMENT;
    return timeline(s,0,tag,items,count,ops);
}
int newsroom_coverage(const ArticleStore *s,int event_id,long long origin,
                      long long width,size_t n,size_t *counts) {
    if(!s||width<=0||(n&&!counts)) return SD_ERR_INVALID_ARGUMENT;
    for(size_t i=0;i<n;i++) counts[i]=0;
    for(size_t i=0;i<s->count;i++) {
        const Article *a=&s->items[i];
        if(a->event_id!=event_id||a->publication_time<origin) continue;
        /* Avoid subtraction overflow by using unsigned difference. */
        unsigned long long diff=(unsigned long long)a->publication_time-(unsigned long long)origin;
        unsigned long long k=diff/(unsigned long long)width;
        if(k<(unsigned long long)n) counts[(size_t)k]++;
    }
    return SD_OK;
}
int newsroom_surge(const size_t *counts,size_t n,size_t *first,size_t *last,
                   long long *delta,uint64_t *ops) {
    if(!first||!last||!delta||!counts||!n) return SD_ERR_INVALID_ARGUMENT;
    *first=*last=0;*delta=0;
    if(n==1) return SD_OK;
    long long *values=checked_malloc((n-1)*sizeof(*values));
    if(!values) return SD_ERR_OUT_OF_MEMORY;
    for(size_t i=1;i<n;i++) {
        if(counts[i]>LLONG_MAX||counts[i-1]>LLONG_MAX) {free(values);return SD_ERR_OVERFLOW;}
        values[i-1]=(long long)counts[i]-(long long)counts[i-1];
    }
    int rc=max_subarray(values,n-1,first,last,delta,ops);
    free(values);return rc;
}
int newsroom_urgency(double base,long long age_hours,size_t coverage,double *score,uint64_t *ops) {
    if(!score||!isfinite(base)||base<0||age_hours<0||coverage>SIZE_MAX/4) return SD_ERR_INVALID_ARGUMENT;
    double decay=0;
    int rc=fast_power_double(1.0+(double)age_hours/24.0,SD_URGENCY_DECAY_EXPONENT,&decay,ops);
    if(rc!=SD_OK) return rc;
    *score=base*decay+4.0*(double)coverage;
    return isfinite(*score)?SD_OK:SD_ERR_OVERFLOW;
}
void newsroom_allocation_free(AllocationReport *r) {
    if(r) {free(r->items);free(r->workloads);memset(r,0,sizeof(*r));}
}
int newsroom_allocate(const FactCheckTask *tasks,size_t nt,const FactChecker *checkers,
                      size_t nc,AllocationReport *out,uint64_t *ops) {
    if(!out||(nt&&!tasks)||(nc&&!checkers)) return SD_ERR_INVALID_ARGUMENT;
    memset(out,0,sizeof(*out));
    if(nt>SIZE_MAX/sizeof(Assignment)||nc>SIZE_MAX/sizeof(size_t)||
       (nc && nt>SIZE_MAX/nc)) return SD_ERR_OVERFLOW;
    out->items=checked_calloc(nt,sizeof(*out->items));
    out->workloads=checked_calloc(nc,sizeof(*out->workloads));
    if(!out->items||!out->workloads) goto oom;
    out->count=nt;out->checker_count=nc;
    for(size_t i=0;i<nt;i++) {
        out->items[i].article_id=tasks[i].article_id;out->items[i].checker_id=-1;
        if(tasks[i].estimated_work!=1) {newsroom_allocation_free(out);return SD_ERR_INVALID_ARGUMENT;}
    }
    if(nt==0||nc==0) {out->unassigned=nt;return SD_OK;}
    unsigned char *compat=checked_calloc(nt*nc,1);
    if(!compat) goto oom;
    for(size_t i=0;i<nt;i++) for(size_t j=0;j<nc;j++) {
        const FactChecker *c=&checkers[j];
        if(c->available_capacity<c->assigned_workload || c->expertise_count<0 || c->expertise_count>MAX_TAGS) {
            free(compat);newsroom_allocation_free(out);return SD_ERR_INVALID_ARGUMENT;
        }
        for(int k=0;k<c->expertise_count;k++) {
            if(strcmp(tasks[i].required_expertise,c->expertise_tags[k])==0) compat[i*nc+j]=1;
        }
    }
    /* First compute compatibility with the tested matching algorithm. Flow
       subsequently enforces actual capacities (possibly more than one task
       per checker). */
    size_t matching=0,*left=NULL,*right=NULL;
    int rc=max_bipartite_matching(compat,nt,nc,&matching,&left,&right,ops);
    free(left);free(right);
    if(rc!=SD_OK) {free(compat);newsroom_allocation_free(out);return rc;}
    (void)matching;
    if(nt>SIZE_MAX-nc-2) {free(compat);newsroom_allocation_free(out);return SD_ERR_OVERFLOW;}
    size_t vertices=nt+nc+2, source=vertices-2,sink=vertices-1;
    if(vertices>SIZE_MAX/vertices/sizeof(int64_t)) {free(compat);newsroom_allocation_free(out);return SD_ERR_OVERFLOW;}
    int64_t *cap=checked_calloc(vertices*vertices,sizeof(*cap));
    if(!cap) {free(compat);goto oom;}
    for(size_t i=0;i<nt;i++) {
        cap[source*vertices+i]=1;
        for(size_t j=0;j<nc;j++) if(compat[i*nc+j]) cap[i*vertices+nt+j]=1;
    }
    free(compat);
    for(size_t j=0;j<nc;j++) cap[(nt+j)*vertices+sink]=
        checkers[j].available_capacity-checkers[j].assigned_workload;
    int64_t flow_value=0,*flow=NULL;
    rc=ford_fulkerson(cap,vertices,source,sink,&flow_value,&flow,ops);
    free(cap);
    if(rc!=SD_OK) {newsroom_allocation_free(out);return rc;}
    for(size_t i=0;i<nt;i++) for(size_t j=0;j<nc;j++) {
        if(flow[i*vertices+nt+j]>0) {
            out->items[i].checker_id=checkers[j].id;
            out->workloads[j]++;
            out->assigned++;
        }
    }
    free(flow);out->unassigned=nt-out->assigned;return SD_OK;
oom:
    newsroom_allocation_free(out);return SD_ERR_OUT_OF_MEMORY;
}
int newsroom_compare_shock(const FactCheckTask *before,size_t nb,
                           const FactCheckTask *after,size_t na,
                           const FactChecker *checkers,size_t nc,
                           ShockComparison *result,uint64_t *ops) {
    if(!result) return SD_ERR_INVALID_ARGUMENT;
    AllocationReport a={0},b={0};
    int rc=newsroom_allocate(before,nb,checkers,nc,&a,ops);
    if(rc!=SD_OK) return rc;
    rc=newsroom_allocate(after,na,checkers,nc,&b,ops);
    if(rc==SD_OK) {
        result->before_assigned=a.assigned;result->before_unassigned=a.unassigned;
        result->after_assigned=b.assigned;result->after_unassigned=b.unassigned;
    }
    newsroom_allocation_free(&a);newsroom_allocation_free(&b);return rc;
}
int newsroom_source_celebrity(const unsigned char *refs,size_t n,ptrdiff_t *index,uint64_t *ops) {
    return find_celebrity(refs,n,index,ops);
}
int newsroom_network_paths(const GraphList *g,size_t start,double *d,size_t *p,uint64_t *ops) {
    return dijkstra(g,start,d,p,ops);
}
int newsroom_network_mst(const GraphMatrix *g,int kr,SDTreeEdge *edges,size_t *count,
                         double *weight,uint64_t *ops) {
    return kr?kruskal(g,edges,count,weight,ops):prim(g,edges,count,weight,ops);
}
int newsroom_network_all_pairs(const GraphMatrix *g,double **d,size_t **n,int *cycle,uint64_t *ops) {
    return floyd_warshall(g,d,n,cycle,ops);
}
