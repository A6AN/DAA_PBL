#include "newsroom.h"
#include "test_harness.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    ArticleStore a;article_store_init(&a);
    SourceTable s={0};EventTable e={0};CheckerTable c={0};
    CSVReport report;FILE *f=fopen("data/articles.csv","r");
    ASSERT_TRUE(f!=NULL);
    if(f) {ASSERT_EQ_INT(SD_OK,article_csv_read(f,&a,&report));
       ASSERT_EQ_INT(48,report.loaded);ASSERT_EQ_INT(0,report.rejected);fclose(f);}
    f=fopen("data/sources.csv","r");ASSERT_TRUE(f!=NULL);
    if(f) {ASSERT_EQ_INT(SD_OK,sources_csv_read(f,&s,&report));
       ASSERT_EQ_INT(8,report.loaded);fclose(f);}
    f=fopen("data/events.csv","r");ASSERT_TRUE(f!=NULL);
    if(f) {ASSERT_EQ_INT(SD_OK,events_csv_read(f,&e,&report));
       ASSERT_EQ_INT(6,report.loaded);fclose(f);}
    f=fopen("data/fact_checkers.csv","r");ASSERT_TRUE(f!=NULL);
    if(f) {ASSERT_EQ_INT(SD_OK,checkers_csv_read(f,&c,&report));
       ASSERT_EQ_INT(5,report.loaded);fclose(f);}
    int quoted=0,embedded=0;
    for(size_t i=0;i<a.count;i++) {
        if(strchr(a.items[i].headline,',')) quoted++;
        if(strchr(a.items[i].body,'\n')) embedded++;
    }
    ASSERT_TRUE(quoted>0);ASSERT_TRUE(embedded>0);
    DashboardStats stats;
    ASSERT_EQ_INT(SD_OK,newsroom_dashboard(&a,&s,&e,&c,&stats));
    ASSERT_EQ_INT(48,stats.articles);ASSERT_EQ_INT(8,stats.sources);
    ASSERT_EQ_INT(6,stats.events);ASSERT_TRUE(stats.candidate_pairs>0);
    ASSERT_TRUE(stats.awaiting_verification>0);
    ASSERT_TRUE(stats.available_capacity>0);
    const Article *result=NULL;
    ASSERT_EQ_INT(SD_OK,article_store_find(&a,20,&result,NULL));
    ASSERT_TRUE(result && result->id==20);
    sources_free(&s);events_free(&e);checkers_free(&c);article_store_free(&a);
    return harness_summary();
}
