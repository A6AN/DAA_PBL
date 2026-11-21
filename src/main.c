#include "article_manager.h"
#include "newsroom.h"
#include "scenarios.h"
#include "workbench.h"
#include "string_matching.h"
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INPUT_CAP 128u

/* Numeric-only input. A long line is discarded before asking again. */
static int read_choice(const char *prompt, long minimum, long maximum, long *value) {
    char line[INPUT_CAP];
    for(;;) {
        fputs(prompt,stdout);fflush(stdout);
        if(!fgets(line,sizeof(line),stdin))return 0;
        if(!strchr(line,'\n') && !feof(stdin)) {
            int ch;
            while((ch=getchar())!='\n' && ch!=EOF){}
            puts("Input too long. Try again.");
            continue;
        }
        errno=0;char *end=NULL;
        long chosen=strtol(line,&end,10);
        if(end==line || errno==ERANGE || chosen<minimum || chosen>maximum){
            puts("Invalid selection. Enter a number from the menu.");continue;
        }
        while(*end==' ' || *end=='\t' || *end=='\r' || *end=='\n')end++;
        if(*end!='\0'){puts("Enter a whole number only.");continue;}
        *value=chosen;return 1;
    }
}
static FILE *open_articles(void) {
    const char *paths[]={"data/articles.csv","signaldesk/data/articles.csv","../data/articles.csv"};
    for(size_t i=0;i<sizeof(paths)/sizeof(paths[0]);i++){
        FILE *file=fopen(paths[i],"rb");if(file)return file;
    }
    return NULL;
}
static int load_articles(ArticleStore *store) {
    FILE *file=open_articles();
    if(!file)return SD_ERR_NOT_FOUND;
    CSVReport report={0};int rc=article_csv_read(file,store,&report);
    fclose(file);
    if(rc==SD_OK && report.rejected>0)fprintf(stderr,"Skipped %zu invalid CSV rows.\n",report.rejected);
    return rc;
}
static int article_menu(void) {
    ArticleStore store;article_store_init(&store);
    int rc=load_articles(&store);
    if(rc!=SD_OK){article_store_free(&store);return rc;}
    long mode=0;
    if(!read_choice("Article Management: 1 List chronological, 2 Sort by urgency, 3 Search phrase, 0 Back: ",0,3,&mode)){
        article_store_free(&store);return SD_OK;
    }
    if(mode==1 || mode==2){
        Article **order=NULL;
        rc=article_store_sort(&store,mode==1?ARTICLE_BY_PUBLICATION_TIME:ARTICLE_BY_URGENCY,0,&order,NULL);
        if(rc==SD_OK){
            for(size_t i=0;i<store.count;i++)fprintf(stdout,"%d | %s | urgency %.1f\n",order[i]->id,order[i]->headline,order[i]->urgency_score);
        }
        free(order);
    }else if(mode==3){
        const char *phrase="river";int *ids=NULL;size_t count=0;
        rc=article_store_phrase(&store,phrase,kmp_search,&ids,&count,NULL);
        if(rc==SD_OK){printf("Phrase '%s' appears in %zu articles:",phrase,count);
            for(size_t i=0;i<count;i++)printf(" %d",ids[i]);
            puts("");}
        free(ids);
    }
    article_store_free(&store);return rc;
}
static int overlap_menu(void) {
    ArticleStore store;article_store_init(&store);
    int rc=load_articles(&store);
    if(rc!=SD_OK){article_store_free(&store);return rc;}
    OverlapPair *pairs=NULL;size_t count=0;
    rc=newsroom_overlaps(&store,&pairs,&count,NULL);
    if(rc==SD_OK){
        printf("Textual-overlap candidates: %zu (threshold %.2f)\n",count,SD_DUPLICATE_THRESHOLD);
        for(size_t i=0;i<count;i++)printf("%d / %d | score %.3f | passage: %s\n",pairs[i].article_a,pairs[i].article_b,pairs[i].score,pairs[i].passage);
    }
    free(pairs);article_store_free(&store);return rc;
}
static int events_menu(void) {
    ArticleStore store;article_store_init(&store);
    int rc=load_articles(&store);
    if(rc!=SD_OK){article_store_free(&store);return rc;}
    long choice=0;
    if(!read_choice("Enter event ID (1..6), 0 Back: ",0,6,&choice)){
        article_store_free(&store);return SD_OK;
    }
    if(choice==0){article_store_free(&store);return SD_OK;}
    Article **timeline=NULL;size_t n=0;
    rc=newsroom_timeline(&store,(int)choice,&timeline,&n,NULL);
    if(rc==SD_OK){
        printf("Event %ld timeline (%zu articles):\n",choice,n);
        for(size_t i=0;i<n;i++)printf("%lld | %d | %s\n",timeline[i]->publication_time,timeline[i]->id,timeline[i]->headline);
        size_t windows[8]={0};size_t first=0,last=0;long long change=0;
        rc=newsroom_coverage(&store,(int)choice,1730000000LL,3600,8,windows);
        if(rc==SD_OK)rc=newsroom_surge(windows,8,&first,&last,&change,NULL);
        if(rc==SD_OK)printf("Greatest coverage increase: windows %zu..%zu; delta %lld\n",first,last,change);
    }
    free(timeline);article_store_free(&store);return rc;
}
static int workbench_menu(void) {
    int rc=workbench_list(stdout);
    if(rc!=SD_OK)return rc;
    long choice=0;
    if(!read_choice("Algorithm (1..36), 0 Back: ",0,(long)workbench_count(),&choice))return SD_OK;
    if(choice==0)return SD_OK;
    return workbench_run((size_t)(choice-1),stdout);
}
static int scenario_menu(void) {
    long number=0;
    if(!read_choice("Scenario 1 Normal, 2 Shock, 3 Stress, 0 Back: ",0,3,&number))return SD_OK;
    if(number==0)return SD_OK;
    return newsroom_run_scenario((unsigned)number,stdout);
}
static int export_menu(void) {
    ArticleStore store;article_store_init(&store);
    int rc=load_articles(&store);
    if(rc!=SD_OK){article_store_free(&store);return rc;}
    const char *path="signaldesk_articles_export.csv";
    FILE *file=fopen(path,"wb");
    if(!file){article_store_free(&store);return SD_ERR_INVALID_ARGUMENT;}
    rc=article_csv_write(file,&store);
    if(fclose(file)!=0)rc=SD_ERR_INVALID_ARGUMENT;
    if(rc==SD_OK)printf("Exported %zu articles to %s\n",store.count,path);
    article_store_free(&store);return rc;
}
static int interactive(void) {
    for(;;){
        puts("\nSignalDesk — Newsroom Crisis and Narrative Intelligence System");
        puts("1. Newsroom Dashboard");
        puts("2. Article Management");
        puts("3. Duplicate and Narrative Detection");
        puts("4. Breaking Event Tracker");
        puts("5. Editorial Resource Allocation");
        puts("6. Graph and Network Analysis");
        puts("7. DAA Algorithm Workbench");
        puts("8. Benchmark and Comparison Lab");
        puts("9. Run Demonstration Scenarios");
        puts("10. Export Reports");
        puts("0. Exit");
        long selected=0;
        if(!read_choice("Select: ",0,10,&selected))return 0;
        if(selected==0)return 0;
        int rc=SD_OK;
        switch(selected){
        case 1:rc=newsroom_run_scenario(1,stdout);break;
        case 2:rc=article_menu();break;
        case 3:rc=overlap_menu();break;
        case 4:rc=events_menu();break;
        case 5:rc=newsroom_run_scenario(2,stdout);break;
        case 6:
            rc=workbench_run(13,stdout);
            if(rc==SD_OK)rc=workbench_run(14,stdout);
            if(rc==SD_OK)rc=workbench_run(15,stdout);
            if(rc==SD_OK)rc=workbench_run(21,stdout);
            break;
        case 7:rc=workbench_menu();break;
        case 8:rc=newsroom_run_scenario(3,stdout);break;
        case 9:rc=scenario_menu();break;
        case 10:rc=export_menu();break;
        default:rc=SD_ERR_INVALID_ARGUMENT;break;
        }
        if(rc!=SD_OK)fprintf(stderr,"Operation failed (status %d).\n",rc);
    }
}
int main(int argc,char **argv) {
    if(argc==1)return interactive();
    if(argc==2 && strcmp(argv[1],"--workbench-all")==0){
        int rc=workbench_run_all(stdout);
        if(rc!=SD_OK)fprintf(stderr,"Workbench error %d\n",rc);
        return rc==SD_OK?0:1;
    }
    if(argc==3 && strcmp(argv[1],"--scenario")==0 && strlen(argv[2])==1 &&
       argv[2][0]>='1' && argv[2][0]<='3'){
        int rc=newsroom_run_scenario((unsigned)(argv[2][0]-'0'),stdout);
        if(rc!=SD_OK)fprintf(stderr,"Scenario error %d\n",rc);
        return rc==SD_OK?0:1;
    }
    fputs("Usage: signaldesk [--scenario 1|2|3 | --workbench-all]\n",stderr);
    return 2;
}
