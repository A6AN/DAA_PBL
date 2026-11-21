#include "scenarios.h"
#include "newsroom.h"
#include "string_matching.h"
#include "sorting_searching.h"
#include "common.h"
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

static FILE *open_data(const char *name) {
    const char *prefixes[]={"data/","signaldesk/data/","../data/","../signaldesk/data/"};
    char path[256];
    for(size_t i=0;i<sizeof(prefixes)/sizeof(prefixes[0]);i++) {
        int n=snprintf(path,sizeof(path),"%s%s",prefixes[i],name);
        if(n>0 && (size_t)n<sizeof(path)) {
            FILE *f=fopen(path,"rb");
            if(f)return f;
        }
    }
    return NULL;
}
static int load_samples(ArticleStore *a,SourceTable *s,EventTable *e,CheckerTable *c) {
    const char *files[]={"articles.csv","sources.csv","events.csv","fact_checkers.csv"};
    int status=SD_OK;
    for(size_t i=0;i<4;i++) {
        FILE *f=open_data(files[i]);
        if(!f)return SD_ERR_NOT_FOUND;
        CSVReport report={0};
        switch(i) {
        case 0:status=article_csv_read(f,a,&report);break;
        case 1:status=sources_csv_read(f,s,&report);break;
        case 2:status=events_csv_read(f,e,&report);break;
        default:status=checkers_csv_read(f,c,&report);break;
        }
        fclose(f);
        if(status!=SD_OK)return status;
        if(report.rejected!=0)return SD_ERR_INVALID_ARGUMENT;
    }
    return SD_OK;
}
static void release_samples(ArticleStore *a,SourceTable *s,EventTable *e,CheckerTable *c) {
    article_store_free(a);sources_free(s);events_free(e);checkers_free(c);
}
static int scenario_normal(FILE *out) {
    ArticleStore a;article_store_init(&a);
    SourceTable s={0};EventTable e={0};CheckerTable c={0};
    int rc=load_samples(&a,&s,&e,&c);
    if(rc!=SD_OK)goto done;
    DashboardStats dash={0};
    rc=newsroom_dashboard(&a,&s,&e,&c,&dash);
    if(rc!=SD_OK)goto done;
    fprintf(out,"Scenario 1: Normal newsroom operation\nDashboard: articles=%zu sources=%zu events=%zu overlap candidates=%zu pending=%zu high urgency=%zu available capacity=%zu\n",dash.articles,dash.sources,dash.events,dash.candidate_pairs,dash.awaiting_verification,dash.high_urgency,dash.available_capacity);
    Article **sorted=NULL;
    rc=article_store_sort(&a,ARTICLE_BY_PUBLICATION_TIME,0,&sorted,NULL);
    if(rc!=SD_OK)goto done;
    if(a.count)fprintf(out,"Timeline earliest article ID=%d\n",sorted[0]->id);
    free(sorted);
    int *ids=NULL;size_t matches=0;
    rc=article_store_phrase(&a,"monitoring",kmp_search,&ids,&matches,NULL);
    if(rc!=SD_OK)goto done;
    fprintf(out,"Phrase search 'monitoring': %zu articles\n",matches);
    free(ids);
    Article **timeline=NULL;size_t timeline_count=0;
    rc=newsroom_timeline(&a,2,&timeline,&timeline_count,NULL);
    if(rc!=SD_OK)goto done;
    fprintf(out,"Event 2 timeline: %zu stories\n",timeline_count);
    free(timeline);
    OverlapPair *pairs=NULL;size_t overlaps=0;
    rc=newsroom_overlaps(&a,&pairs,&overlaps,NULL);
    if(rc!=SD_OK)goto done;
    if(overlaps)fprintf(out,"First textual-overlap pair: %d / %d; score %.3f; passage '%s'\n",pairs[0].article_a,pairs[0].article_b,pairs[0].score,pairs[0].passage);
    free(pairs);
    FactCheckTask tasks[5]={0};
    const char *tags[]={"transit","river","energy","technology","health"};
    for(size_t i=0;i<5;i++) {
        tasks[i].id=(int)(i+1);tasks[i].article_id=(int)(i+1);
        tasks[i].estimated_work=1;
        safe_strncpy(tasks[i].required_expertise,tags[i],sizeof(tasks[i].required_expertise));
    }
    AllocationReport report={0};
    rc=newsroom_allocate(tasks,5,c.items,c.count,&report,NULL);
    if(rc==SD_OK)fprintf(out,"Verification allocation: assigned=%zu unassigned=%zu\n",report.assigned,report.unassigned);
    newsroom_allocation_free(&report);
done:
    release_samples(&a,&s,&e,&c);
    return rc;
}
static int scenario_shock(FILE *out) {
    ArticleStore a;article_store_init(&a);
    SourceTable s={0};EventTable e={0};CheckerTable c={0};
    int rc=load_samples(&a,&s,&e,&c);
    if(rc!=SD_OK)goto done;
    const size_t before_count=5, after_count=30;
    FactCheckTask tasks[30]={0};
    const char *tags[]={"transit","river","energy","technology","health"};
    for(size_t i=0;i<after_count;i++) {
        tasks[i].id=(int)(i+1);tasks[i].article_id=(int)(i+1);tasks[i].estimated_work=1;
        safe_strncpy(tasks[i].required_expertise,tags[i%5],sizeof(tasks[i].required_expertise));
    }
    AllocationReport before={0},after={0};
    rc=newsroom_allocate(tasks,before_count,c.items,c.count,&before,NULL);
    if(rc!=SD_OK)goto done;
    fprintf(out,"Scenario 2: Breaking-news capacity shock\nBefore: assigned=%zu unassigned=%zu\n",before.assigned,before.unassigned);
    lcg_seed(UINT64_C(20261121));
    for(size_t i=0;i<25;i++) {
        Article article={0};
        article.id=1000+(int)i;article.source_id=1+(int)(lcg_range(0,7));
        article.event_id=2;article.publication_time=1730000000LL+(long long)i*60;
        article.verification_status=VERIFY_PENDING;article.urgency_score=82.0;
        safe_strncpy(article.headline,"Additional river coverage arrives",sizeof(article.headline));
        safe_strncpy(article.body,"Reporter update on the same developing event.",sizeof(article.body));
        article.tag_count=1;safe_strncpy(article.tags[0],"river",sizeof(article.tags[0]));
        rc=article_store_add(&a,&article);
        if(rc!=SD_OK)break;
    }
    if(rc==SD_OK)rc=newsroom_allocate(tasks,after_count,c.items,c.count,&after,NULL);
    if(rc==SD_OK) {
        fprintf(out,"After: assigned=%zu unassigned=%zu; injected=25; articles=%zu\n",after.assigned,after.unassigned,a.count);
        for(size_t j=0;j<c.count;j++)fprintf(out,"Checker %d workload=%zu/%d\n",c.items[j].id,after.workloads[j],c.items[j].available_capacity);
        if(after.unassigned==0 || after.unassigned<=before.unassigned)rc=SD_ERR_INFEASIBLE;
    }
    newsroom_allocation_free(&before);newsroom_allocation_free(&after);
done:
    release_samples(&a,&s,&e,&c);return rc;
}
static int scenario_stress(FILE *out) {
    enum {N=SIZE_MEDIUM*2};
    int *a=checked_malloc(N*sizeof(*a)),*b=checked_malloc(N*sizeof(*b));
    if(!a||!b){free(a);free(b);return SD_ERR_OUT_OF_MEMORY;}
    lcg_seed(UINT64_C(20261121));
    for(size_t i=0;i<N;i++)a[i]=(int)lcg_range(0,100000);
    memcpy(b,a,N*sizeof(*a));
    uint64_t op_merge=0,op_quick=0;
    uint64_t t0=sd_time_ns();int rc=merge_sort(a,N,&op_merge);
    uint64_t t1=sd_time_ns();
    if(rc!=SD_OK)goto done;
    uint64_t t2=sd_time_ns();rc=quick_sort(b,N,&op_quick);
    uint64_t t3=sd_time_ns();
    if(rc!=SD_OK)goto done;
    if(memcmp(a,b,N*sizeof(*a))!=0){rc=SD_ERR_INFEASIBLE;goto done;}
    ptrdiff_t found=-1;uint64_t search_ops=0;
    rc=binary_search_iterative(a,N,a[N/2],&found,&search_ops);
    if(rc!=SD_OK || found<0){rc=SD_ERR_INFEASIBLE;goto done;}
    fprintf(out,"Scenario 3: Large-data stress test\nDataset size: %u integers (fixed seed)\n",(unsigned)N);
    fprintf(out,"Merge Sort: operations=%" PRIu64 ", elapsed_ns=%" PRIu64 ", estimated auxiliary memory=%zu bytes\n",op_merge,t1>=t0?t1-t0:0,(size_t)N*sizeof(int));
    fprintf(out,"Quick Sort: operations=%" PRIu64 ", elapsed_ns=%" PRIu64 ", auxiliary memory=recursion stack\n",op_quick,t3>=t2?t3-t2:0);
    fprintf(out,"Sorted output equality: yes; Binary Search index=%td operations=%" PRIu64 "\n",found,search_ops);
    {
        const char *text="river river rose river river";const char *pat="river";
        typedef int (*Matcher)(const char *,size_t,const char *,size_t,size_t *,size_t,size_t *,uint64_t *);
        Matcher matchers[]={naive_search,rabin_karp_search,automaton_search,kmp_search};
        const char *names[]={"Naive","Rabin-Karp","Automaton","KMP"};
        size_t expected=0;
        for(size_t i=0;i<4;i++){
            size_t pos[16]={0},count=0;uint64_t ops=0;
            uint64_t start=sd_time_ns();
            rc=matchers[i](text,strlen(text),pat,strlen(pat),pos,16,&count,&ops);
            uint64_t end=sd_time_ns();
            if(rc!=SD_OK)goto done;
            if(i==0)expected=count;
            if(count!=expected){rc=SD_ERR_INFEASIBLE;goto done;}
            fprintf(out,"%s: matches=%zu operations=%" PRIu64 " elapsed_ns=%" PRIu64 "\n",names[i],count,ops,end>=start?end-start:0);
        }
    }
done:
    free(a);free(b);return rc;
}
int newsroom_run_scenario(unsigned number,FILE *out) {
    if(!out)return SD_ERR_INVALID_ARGUMENT;
    int rc;
    switch(number) {
    case 1:rc=scenario_normal(out);break;
    case 2:rc=scenario_shock(out);break;
    case 3:rc=scenario_stress(out);break;
    default:return SD_ERR_INVALID_ARGUMENT;
    }
    return ferror(out)?SD_ERR_INVALID_ARGUMENT:rc;
}
