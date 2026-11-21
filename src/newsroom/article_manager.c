#include "article_manager.h"
#include "string_matching.h"
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

void article_store_init(ArticleStore *s) { if (s) memset(s,0,sizeof(*s)); }
void article_store_free(ArticleStore *s) { if (s) { free(s->items); article_store_init(s); } }
int article_store_add(ArticleStore *s, const Article *a) {
    if (!s || !a || a->id < 0 || a->tag_count < 0 || a->tag_count > MAX_TAGS ||
        !isfinite(a->urgency_score)) return SD_ERR_INVALID_ARGUMENT;
    /* ID uniqueness is required for indexed lookup. */
    for (size_t i=0;i<s->count;i++) if (s->items[i].id==a->id) return SD_ERR_INVALID_ARGUMENT;
    if (s->count==s->capacity) {
        size_t cap=s->capacity ? s->capacity*2 : 16;
        if (cap < s->capacity || cap > SIZE_MAX/sizeof(Article)) return SD_ERR_OVERFLOW;
        Article *grown=checked_realloc(s->items,cap*sizeof(*grown));
        if (!grown) return SD_ERR_OUT_OF_MEMORY;
        s->items=grown; s->capacity=cap;
    }
    s->items[s->count++]=*a; return SD_OK;
}
int article_store_sort(const ArticleStore *s,ArticleSortKey key,int quick,
                       Article ***ordered,uint64_t *ops) {
    if (!s || !ordered || (s->count && !s->items)) return SD_ERR_INVALID_ARGUMENT;
    *ordered=NULL;
    if (s->count==0) return SD_OK;
    if (s->count>SIZE_MAX/sizeof(Article *)) return SD_ERR_OVERFLOW;
    Article **ptrs=checked_malloc(s->count*sizeof(*ptrs));
    if (!ptrs) return SD_ERR_OUT_OF_MEMORY;
    for (size_t i=0;i<s->count;i++) ptrs[i]=&s->items[i];
    int rc=quick?quick_sort_articles(ptrs,s->count,key,ops):
                 merge_sort_articles(ptrs,s->count,key,ops);
    if (rc!=SD_OK) {free(ptrs);return rc;}
    *ordered=ptrs;return SD_OK;
}
int article_store_find(const ArticleStore *s,int id,const Article **result,uint64_t *ops) {
    if (!s || !result || s->count>INT_MAX || s->count>SIZE_MAX/sizeof(int)) return SD_ERR_INVALID_ARGUMENT;
    *result=NULL;
    if (!s->count) return SD_ERR_NOT_FOUND;
    Article **order=NULL;
    int rc=article_store_sort(s,ARTICLE_BY_ID,0,&order,ops);
    if (rc!=SD_OK) return rc;
    int *ids=checked_malloc(s->count*sizeof(*ids));
    if (!ids) {free(order);return SD_ERR_OUT_OF_MEMORY;}
    for (size_t i=0;i<s->count;i++) ids[i]=order[i]->id;
    ptrdiff_t at=-1;
    rc=binary_search_iterative(ids,s->count,id,&at,ops);
    if (rc==SD_OK && at>=0) *result=order[(size_t)at];
    free(ids);free(order);
    return rc==SD_OK?(*result?SD_OK:SD_ERR_NOT_FOUND):rc;
}
int article_store_phrase(const ArticleStore *s,const char *phrase,
                         int (*matcher)(const char *,size_t,const char *,size_t,size_t *,size_t,size_t *,uint64_t *),
                         int **ids,size_t *count,uint64_t *ops) {
    if (!s || !phrase || !matcher || !ids || !count) return SD_ERR_INVALID_ARGUMENT;
    *ids=NULL;*count=0;
    if (s->count>SIZE_MAX/sizeof(int)) return SD_ERR_OVERFLOW;
    int *hits=checked_malloc((s->count?s->count:1)*sizeof(*hits));
    if (!hits) return SD_ERR_OUT_OF_MEMORY;
    size_t plen=strlen(phrase);
    for (size_t i=0;i<s->count;i++) {
        const Article *a=&s->items[i];size_t found=0;
        int rc=matcher(a->headline,strlen(a->headline),phrase,plen,NULL,0,&found,ops);
        if (rc==SD_OK && !found) rc=matcher(a->body,strlen(a->body),phrase,plen,NULL,0,&found,ops);
        if (rc!=SD_OK) {free(hits);return rc;}
        if (found) hits[(*count)++]=a->id;
    }
    *ids=hits;return SD_OK;
}

/* The parser consumes one entire logical record, accounting for quoted newlines.
   No CSV field may exceed 32767 bytes, and at most 16 fields are supported.
   These bounds also prevent malformed input from exhausting memory. */
#define CSV_FIELDS 16
#define CSV_FIELD_CAP 32768
typedef struct { char cell[CSV_FIELDS][CSV_FIELD_CAP];size_t n,line;int malformed; } CSVRow;
static int csv_next(FILE *f,CSVRow *r,size_t *line) {
    memset(r,0,sizeof(*r));r->line=*line;
    int state=0,started=0,ch;size_t pos=0;
    while ((ch=fgetc(f))!=EOF) {
        started=1;
        if (ch=='\n') ++*line;
        if (state==2) {
            if (ch=='"') state=3;
            else if (pos+1<CSV_FIELD_CAP && r->n<CSV_FIELDS) r->cell[r->n][pos++]=(char)ch;
            else r->malformed=1;
            continue;
        }
        if (state==3) {
            if (ch=='"') { if (pos+1<CSV_FIELD_CAP && r->n<CSV_FIELDS) r->cell[r->n][pos++]='"';else r->malformed=1; state=2;continue; }
            if (ch!=',' && ch!='\n' && ch!='\r') r->malformed=1;
        } else if (ch=='"') {
            if (state==0) {state=2;continue;}
            r->malformed=1;
        }
        if (ch==',' && state!=2) {
            if (r->n<CSV_FIELDS) r->cell[r->n][pos]='\0';
            ++r->n;pos=0;state=0;
            if (r->n>=CSV_FIELDS) r->malformed=1;
            continue;
        }
        if (ch=='\n' && state!=2) {++r->n;return 1;}
        if (ch=='\r' && state!=2) continue;
        if (state==3) continue;
        if (pos+1<CSV_FIELD_CAP && r->n<CSV_FIELDS) {r->cell[r->n][pos++]=(char)ch;state=1;}
        else r->malformed=1;
    }
    if (ferror(f)) return -1;
    if (!started) return 0;
    if (state==2) r->malformed=1;
    ++r->n;return 1;
}
static int as_int(const char *s,int *v) {
    char *end;errno=0;long x=strtol(s,&end,10);
    if (errno || end==s || *end || x<INT_MIN || x>INT_MAX) return 0;
    *v=(int)x;return 1;
}
static int as_ll(const char *s,long long *v) {
    char *end;errno=0;long long x=strtoll(s,&end,10);
    if (errno || end==s || *end) return 0;
    *v=x;return 1;
}
static int as_double(const char *s,double *v) {
    char *end;errno=0;double x=strtod(s,&end);
    if (errno || end==s || *end || !isfinite(x)) return 0;
    *v=x;return 1;
}
static int copy_field(char *dst,size_t cap,const char *src) {return !safe_strncpy(dst,src,cap);}
static int add_table(void **ptr,size_t *count,size_t unit,const void *record) {
    if (*count==SIZE_MAX || *count+1>SIZE_MAX/unit) return SD_ERR_OVERFLOW;
    void *grown=checked_realloc(*ptr,(*count+1)*unit);
    if (!grown) return SD_ERR_OUT_OF_MEMORY;
    *ptr=grown;memcpy((char *)grown+*count*unit,record,unit);++*count;return SD_OK;
}
/* Format: id,headline,body,source_id,publication_time,tags,event_id,urgency_score,verification_status.
   Tags are pipe-separated and preserve commas within a quoted CSV field. */
static int parse_article(const CSVRow *r,Article *a) {
    if (r->n!=9 || r->malformed) return 0;
    memset(a,0,sizeof(*a));
    if (!as_int(r->cell[0],&a->id) || a->id<0 ||
        !copy_field(a->headline,sizeof(a->headline),r->cell[1]) ||
        !copy_field(a->body,sizeof(a->body),r->cell[2]) ||
        !as_int(r->cell[3],&a->source_id) || !as_ll(r->cell[4],&a->publication_time) ||
        !as_int(r->cell[6],&a->event_id) || !as_double(r->cell[7],&a->urgency_score) ||
        !as_int(r->cell[8],&a->verification_status) || a->verification_status<0 ||
        a->verification_status>3) return 0;
    const char *tags=r->cell[5];
    while (*tags) {
        if (a->tag_count==MAX_TAGS) return 0;
        const char *end=strchr(tags,'|');size_t len=end?(size_t)(end-tags):strlen(tags);
        if (!len || len>=MAX_TAG_LENGTH) return 0;
        memcpy(a->tags[a->tag_count],tags,len);a->tags[a->tag_count++][len]='\0';
        if (!end) break;
        tags=end+1;
    }
    return 1;
}
static int parse_source(const CSVRow *r,Source *s) {
    if (r->n!=3 || r->malformed) return 0;
    memset(s,0,sizeof(*s));return as_int(r->cell[0],&s->id)&&
      copy_field(s->name,sizeof(s->name),r->cell[1])&&
      copy_field(s->description,sizeof(s->description),r->cell[2]);
}
static int parse_event(const CSVRow *r,Event *e) {
    int band; if (r->n!=5 || r->malformed) return 0;
    memset(e,0,sizeof(*e));
    if (!as_int(r->cell[0],&e->id)||!copy_field(e->name,sizeof(e->name),r->cell[1])||
        !copy_field(e->description,sizeof(e->description),r->cell[2])||
        !as_ll(r->cell[3],&e->first_seen_time)||!as_int(r->cell[4],&band)||band<0||band>3) return 0;
    e->urgency_band=(UrgencyBand)band;return 1;
}
static int parse_checker(const CSVRow *r,FactChecker *c) {
    if (r->n!=5 || r->malformed) return 0;
    memset(c,0,sizeof(*c));
    if (!as_int(r->cell[0],&c->id)||!copy_field(c->name,sizeof(c->name),r->cell[1])||
        !as_int(r->cell[3],&c->available_capacity)||c->available_capacity<0||
        !as_int(r->cell[4],&c->assigned_workload)||c->assigned_workload<0||
        c->assigned_workload>c->available_capacity) return 0;
    const char *tag=r->cell[2];
    while (*tag) {
        if (c->expertise_count==MAX_TAGS) return 0;
        const char *end=strchr(tag,'|');size_t len=end?(size_t)(end-tag):strlen(tag);
        if (!len || len>=MAX_TAG_LENGTH) return 0;
        memcpy(c->expertise_tags[c->expertise_count],tag,len);
        c->expertise_tags[c->expertise_count++][len]='\0';
        if (!end) break;
        tag=end+1;
    }
    return 1;
}
/* A table kind chooses the only parser and insertion strategy. */
static int csv_read(FILE *f,void *table,CSVReport *report,int kind) {
    if (!f || !table || !report) return SD_ERR_INVALID_ARGUMENT;
    memset(report,0,sizeof(*report));size_t line=1;int first=1;
    CSVRow *r=checked_malloc(sizeof(*r));if (!r) return SD_ERR_OUT_OF_MEMORY;
    int result=SD_OK,step;
    while ((step=csv_next(f,r,&line))==1) {
        if (first) {first=0;continue;} /* consume column names */
        int valid=0,rc=SD_OK;
        if (kind==0) {
            Article a;valid=parse_article(r,&a);
            if (valid) rc=article_store_add(table,&a);
            if (rc==SD_ERR_INVALID_ARGUMENT) valid=0;
        } else if (kind==1) {
            Source s;valid=parse_source(r,&s);
            if (valid) {SourceTable *t=table;rc=add_table((void **)&t->items,&t->count,sizeof(s),&s);}
        } else if (kind==2) {
            Event e;valid=parse_event(r,&e);
            if (valid) {EventTable *t=table;rc=add_table((void **)&t->items,&t->count,sizeof(e),&e);}
        } else {
            FactChecker c;valid=parse_checker(r,&c);
            if (valid) {CheckerTable *t=table;rc=add_table((void **)&t->items,&t->count,sizeof(c),&c);}
        }
        if (!valid) {report->rejected++;if (!report->first_error_line) report->first_error_line=r->line;}
        else if (rc!=SD_OK) {result=rc;break;}
        else report->loaded++;
    }
    if (step<0) result=SD_ERR_INVALID_ARGUMENT;
    free(r);return result;
}
int article_csv_read(FILE *f,ArticleStore *s,CSVReport *r){return csv_read(f,s,r,0);}
int sources_csv_read(FILE *f,SourceTable *s,CSVReport *r){return csv_read(f,s,r,1);}
int events_csv_read(FILE *f,EventTable *s,CSVReport *r){return csv_read(f,s,r,2);}
int checkers_csv_read(FILE *f,CheckerTable *s,CSVReport *r){return csv_read(f,s,r,3);}
void sources_free(SourceTable *t){if(t){free(t->items);memset(t,0,sizeof(*t));}}
void events_free(EventTable *t){if(t){free(t->items);memset(t,0,sizeof(*t));}}
void checkers_free(CheckerTable *t){if(t){free(t->items);memset(t,0,sizeof(*t));}}
static int field(FILE *f,const char *str) {
    if (fputc('"',f)==EOF) return SD_ERR_INVALID_ARGUMENT;
    for (;*str;str++) {
        if (*str=='"'&&fputc('"',f)==EOF) return SD_ERR_INVALID_ARGUMENT;
        if (fputc((unsigned char)*str,f)==EOF) return SD_ERR_INVALID_ARGUMENT;
    }
    return fputc('"',f)==EOF?SD_ERR_INVALID_ARGUMENT:SD_OK;
}
static int comma(FILE *f){return fputc(',',f)==EOF?SD_ERR_INVALID_ARGUMENT:SD_OK;}
static int tag_line(FILE *f,char tags[MAX_TAGS][MAX_TAG_LENGTH],int n) {
    char joined[MAX_TAGS*(MAX_TAG_LENGTH+1)]={0};size_t len=0;
    for (int i=0;i<n;i++) {
        size_t k=strlen(tags[i]);if (len+k+2>sizeof(joined)) return SD_ERR_INVALID_ARGUMENT;
        if (i) joined[len++]='|';
        memcpy(joined+len,tags[i],k);len+=k;
    }
    return field(f,joined);
}
int article_csv_write(FILE *f,const ArticleStore *s) {
    if(!f||!s) return SD_ERR_INVALID_ARGUMENT;
    if(fputs("id,headline,body,source_id,publication_time,tags,event_id,urgency_score,verification_status\n",f)==EOF) return SD_ERR_INVALID_ARGUMENT;
    for(size_t i=0;i<s->count;i++) {
        const Article *a=&s->items[i];
        if(fprintf(f,"%d,",a->id)<0||field(f,a->headline)||comma(f)||field(f,a->body)||
           fprintf(f,",%d,%lld,",a->source_id,a->publication_time)<0||
           tag_line(f,(char (*)[MAX_TAG_LENGTH])a->tags,a->tag_count)||
           fprintf(f,",%d,%.17g,%d\n",a->event_id,a->urgency_score,a->verification_status)<0)
            return SD_ERR_INVALID_ARGUMENT;
    }return SD_OK;
}
int sources_csv_write(FILE *f,const SourceTable *t) {
    if(!f||!t) return SD_ERR_INVALID_ARGUMENT;
    if(fputs("id,name,description\n",f)==EOF) return SD_ERR_INVALID_ARGUMENT;
    for(size_t i=0;i<t->count;i++) {
        Source *s=&t->items[i];if(fprintf(f,"%d,",s->id)<0||field(f,s->name)||comma(f)||field(f,s->description)||fputc('\n',f)==EOF) return SD_ERR_INVALID_ARGUMENT;
    }return SD_OK;
}
int events_csv_write(FILE *f,const EventTable *t) {
    if(!f||!t) return SD_ERR_INVALID_ARGUMENT;
    if(fputs("id,name,description,first_seen_time,urgency_band\n",f)==EOF) return SD_ERR_INVALID_ARGUMENT;
    for(size_t i=0;i<t->count;i++) {
        Event *e=&t->items[i];if(fprintf(f,"%d,",e->id)<0||field(f,e->name)||comma(f)||field(f,e->description)||fprintf(f,",%lld,%d\n",e->first_seen_time,(int)e->urgency_band)<0) return SD_ERR_INVALID_ARGUMENT;
    }return SD_OK;
}
int checkers_csv_write(FILE *f,const CheckerTable *t) {
    if(!f||!t) return SD_ERR_INVALID_ARGUMENT;
    if(fputs("id,name,expertise_tags,available_capacity,assigned_workload\n",f)==EOF) return SD_ERR_INVALID_ARGUMENT;
    for(size_t i=0;i<t->count;i++) {
        FactChecker *c=&t->items[i];if(fprintf(f,"%d,",c->id)<0||field(f,c->name)||comma(f)||tag_line(f,c->expertise_tags,c->expertise_count)||fprintf(f,",%d,%d\n",c->available_capacity,c->assigned_workload)<0) return SD_ERR_INVALID_ARGUMENT;
    }return SD_OK;
}
