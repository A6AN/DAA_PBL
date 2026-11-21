#include "article_manager.h"
#include "string_matching.h"
#include "test_harness.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void test_growth(void) {
    ArticleStore store;article_store_init(&store);
    Article *a=calloc(1,sizeof(*a));ASSERT_TRUE(a!=NULL);
    if(!a)return;
    for(int i=4999;i>=0;i--) {
        a->id=i;a->publication_time=i;a->urgency_score=(double)(i%100);
        ASSERT_EQ_INT(SD_OK,article_store_add(&store,a));
    }
    ASSERT_EQ_INT(5000,store.count);
    const Article *hit=NULL;
    for(int i=0;i<5000;i+=17) {
        ASSERT_EQ_INT(SD_OK,article_store_find(&store,i,&hit,NULL));
        ASSERT_TRUE(hit!=NULL && hit->id==i);
    }
    Article **ptr=NULL;
    ASSERT_EQ_INT(SD_OK,article_store_sort(&store,ARTICLE_BY_ID,0,&ptr,NULL));
    ASSERT_EQ_INT(0,ptr[0]->id);ASSERT_EQ_INT(4999,ptr[4999]->id);
    free(ptr);ptr=NULL;
    ASSERT_EQ_INT(SD_OK,article_store_sort(&store,ARTICLE_BY_URGENCY,1,&ptr,NULL));
    ASSERT_TRUE(ptr[0]->urgency_score<=ptr[4999]->urgency_score);free(ptr);
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT,article_store_add(&store,a));
    article_store_free(&store);free(a);
}
static void test_csv(void) {
    ArticleStore a,b;article_store_init(&a);article_store_init(&b);
    FILE *f=tmpfile();ASSERT_TRUE(f!=NULL);if(!f)return;
    Article item={0};item.id=25;item.source_id=3;item.event_id=2;
    item.publication_time=123456;item.urgency_score=38.125;
    strcpy(item.headline,"Metro, \"new\" update");
    strcpy(item.body,"First line\nsecond, line with \"quotes\"");
    strcpy(item.tags[0],"transit");item.tag_count=1;
    ASSERT_EQ_INT(SD_OK,article_store_add(&a,&item));
    ASSERT_EQ_INT(SD_OK,article_csv_write(f,&a));rewind(f);
    CSVReport report;
    ASSERT_EQ_INT(SD_OK,article_csv_read(f,&b,&report));
    ASSERT_EQ_INT(1,report.loaded);ASSERT_EQ_INT(0,report.rejected);
    ASSERT_EQ_STR(item.headline,b.items[0].headline);
    ASSERT_EQ_STR(item.body,b.items[0].body);
    ASSERT_EQ_STR("transit",b.items[0].tags[0]);fclose(f);
    article_store_free(&a);article_store_free(&b);

    f=tmpfile();ASSERT_TRUE(f!=NULL);if(!f)return;
    fputs("id,headline,body,source_id,publication_time,tags,event_id,urgency_score,verification_status\n",f);
    fputs("1,good,body,2,100,river,1,30,0\n",f);
    fputs("2,broken,body,no,110,river,1,30,0\n",f);
    fputs("3,good again,body,2,120,river,1,40,0\n",f);
    rewind(f);article_store_init(&a);
    ASSERT_EQ_INT(SD_OK,article_csv_read(f,&a,&report));
    ASSERT_EQ_INT(2,report.loaded);ASSERT_EQ_INT(1,report.rejected);
    ASSERT_EQ_INT(3,report.first_error_line);article_store_free(&a);fclose(f);
}
static void test_tables(void) {
    SourceTable s={0},copy_s={0};EventTable e={0},copy_e={0};
    CheckerTable c={0},copy_c={0};CSVReport r;
    s.items=calloc(1,sizeof(*s.items));s.count=1;
    e.items=calloc(1,sizeof(*e.items));e.count=1;
    c.items=calloc(1,sizeof(*c.items));c.count=1;
    ASSERT_TRUE(s.items && e.items && c.items);
    if(!s.items||!e.items||!c.items) goto done;
    s.items[0].id=1;strcpy(s.items[0].name,"Source, Alpha");
    strcpy(s.items[0].description,"A \"quoted\" note\nline two");
    e.items[0].id=3;e.items[0].urgency_band=URGENCY_HIGH;
    strcpy(e.items[0].name,"Storm, local");
    c.items[0].id=4;c.items[0].available_capacity=2;
    strcpy(c.items[0].name,"Editor, One");strcpy(c.items[0].expertise_tags[0],"weather");c.items[0].expertise_count=1;
    FILE *f=tmpfile();ASSERT_TRUE(f!=NULL);
    if(f) {ASSERT_EQ_INT(SD_OK,sources_csv_write(f,&s));rewind(f);
      ASSERT_EQ_INT(SD_OK,sources_csv_read(f,&copy_s,&r));
      ASSERT_EQ_INT(1,r.loaded);ASSERT_EQ_STR(s.items[0].description,copy_s.items[0].description);fclose(f);}
    f=tmpfile();ASSERT_TRUE(f!=NULL);
    if(f) {ASSERT_EQ_INT(SD_OK,events_csv_write(f,&e));rewind(f);
      ASSERT_EQ_INT(SD_OK,events_csv_read(f,&copy_e,&r));
      ASSERT_EQ_INT(1,r.loaded);ASSERT_EQ_STR(e.items[0].name,copy_e.items[0].name);fclose(f);}
    f=tmpfile();ASSERT_TRUE(f!=NULL);
    if(f) {ASSERT_EQ_INT(SD_OK,checkers_csv_write(f,&c));rewind(f);
      ASSERT_EQ_INT(SD_OK,checkers_csv_read(f,&copy_c,&r));
      ASSERT_EQ_INT(1,r.loaded);ASSERT_EQ_STR(c.items[0].name,copy_c.items[0].name);fclose(f);}
done:
    sources_free(&s);sources_free(&copy_s);events_free(&e);events_free(&copy_e);
    checkers_free(&c);checkers_free(&copy_c);
}
static void test_phrase(void) {
    ArticleStore s;article_store_init(&s);
    Article a={0};a.id=1;strcpy(a.headline,"transport report");
    ASSERT_EQ_INT(SD_OK,article_store_add(&s,&a));a.id=2;strcpy(a.body,"weather transport");
    ASSERT_EQ_INT(SD_OK,article_store_add(&s,&a));
    int *ids=NULL;size_t n=0;
    ASSERT_EQ_INT(SD_OK,article_store_phrase(&s,"transport",kmp_search,&ids,&n,NULL));
    ASSERT_EQ_INT(2,n);free(ids);article_store_free(&s);
}
int main(void) {test_growth();test_csv();test_tables();test_phrase();return harness_summary();}
