#include "newsroom.h"
#include "string_matching.h"
#include "test_harness.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void test_events_and_overlap(void) {
    ArticleStore s;article_store_init(&s);
    Article a={0};a.id=1;a.event_id=5;a.publication_time=100;
    strcpy(a.headline,"River levels rise in northern district");
    strcpy(a.body,"River sensor reports higher water levels this evening");
    strcpy(a.tags[0],"river");a.tag_count=1;
    ASSERT_EQ_INT(SD_OK,article_store_add(&s,&a));
    a.id=2;a.publication_time=110;
    ASSERT_EQ_INT(SD_OK,article_store_add(&s,&a));
    a.id=3;a.event_id=6;a.publication_time=120;
    strcpy(a.headline,"Unrelated technical conference");
    strcpy(a.body,"Technology program opens in another city");
    ASSERT_EQ_INT(SD_OK,article_store_add(&s,&a));
    OverlapPair *pairs=NULL;size_t n=0;
    ASSERT_EQ_INT(SD_OK,newsroom_overlaps(&s,&pairs,&n,NULL));
    ASSERT_EQ_INT(1,n);if(n) {ASSERT_EQ_INT(1,pairs[0].article_a);
      ASSERT_EQ_INT(2,pairs[0].article_b);ASSERT_TRUE(pairs[0].score>=SD_DUPLICATE_THRESHOLD);}
    free(pairs);
    Article **timeline=NULL;
    ASSERT_EQ_INT(SD_OK,newsroom_timeline(&s,5,&timeline,&n,NULL));
    ASSERT_EQ_INT(2,n);if(n>=2) ASSERT_TRUE(timeline[0]->publication_time<=timeline[1]->publication_time);
    free(timeline);
    ASSERT_EQ_INT(SD_OK,newsroom_tag_timeline(&s,"river",&timeline,&n,NULL));
    ASSERT_EQ_INT(3,n);free(timeline);
    size_t counts[4];ASSERT_EQ_INT(SD_OK,newsroom_coverage(&s,5,100,10,4,counts));
    ASSERT_EQ_INT(1,counts[0]);ASSERT_EQ_INT(1,counts[1]);ASSERT_EQ_INT(0,counts[2]);
    size_t first=0,last=0;long long delta=0;
    ASSERT_EQ_INT(SD_OK,newsroom_surge(counts,4,&first,&last,&delta,NULL));
    ASSERT_TRUE(first<=last && last<=2);
    double score=0;ASSERT_EQ_INT(SD_OK,newsroom_urgency(80,24,2,&score,NULL));
    ASSERT_EQ_DOUBLE(28,score,1e-10);
    SourceTable sources={0};EventTable events={0};CheckerTable checkers={0};
    DashboardStats stats;
    ASSERT_EQ_INT(SD_OK,newsroom_dashboard(&s,&sources,&events,&checkers,&stats));
    ASSERT_EQ_INT(3,stats.articles);ASSERT_EQ_INT(1,stats.candidate_pairs);
    article_store_free(&s);
}
static void test_allocation(void) {
    FactChecker c[2]={0};c[0].id=10;c[0].available_capacity=2;
    strcpy(c[0].expertise_tags[0],"health");c[0].expertise_count=1;
    c[1].id=11;c[1].available_capacity=1;
    strcpy(c[1].expertise_tags[0],"river");c[1].expertise_count=1;
    FactCheckTask tasks[5]={0};
    for(int i=0;i<5;i++) {tasks[i].article_id=i+1;tasks[i].estimated_work=1;
        strcpy(tasks[i].required_expertise,i<3?"health":i==3?"river":"space");}
    AllocationReport r={0};
    ASSERT_EQ_INT(SD_OK,newsroom_allocate(tasks,5,c,2,&r,NULL));
    ASSERT_EQ_INT(3,r.assigned);ASSERT_EQ_INT(2,r.unassigned);
    ASSERT_EQ_INT(2,r.workloads[0]);ASSERT_EQ_INT(1,r.workloads[1]);
    newsroom_allocation_free(&r);
    ShockComparison cmp;
    ASSERT_EQ_INT(SD_OK,newsroom_compare_shock(tasks,2,tasks,5,c,2,&cmp,NULL));
    ASSERT_EQ_INT(2,cmp.before_assigned);ASSERT_EQ_INT(3,cmp.after_assigned);
    ASSERT_EQ_INT(SD_OK,newsroom_allocate(tasks,5,NULL,0,&r,NULL));
    ASSERT_EQ_INT(5,r.unassigned);newsroom_allocation_free(&r);
}
static void test_graphs(void) {
    unsigned char refs[]={0,1,0, 0,0,0, 0,1,0};ptrdiff_t celebrity=-1;
    ASSERT_EQ_INT(SD_OK,newsroom_source_celebrity(refs,3,&celebrity,NULL));
    ASSERT_EQ_INT(1,celebrity);
    GraphList *l=NULL;GraphMatrix *m=NULL;
    ASSERT_EQ_INT(SD_OK,graph_list_create(3,&l));
    ASSERT_EQ_INT(SD_OK,graph_matrix_create(3,&m));
    ASSERT_EQ_INT(SD_OK,graph_list_add(l,0,1,2,1));
    ASSERT_EQ_INT(SD_OK,graph_list_add(l,1,2,3,1));
    ASSERT_EQ_INT(SD_OK,graph_matrix_add(m,0,1,2,1));
    ASSERT_EQ_INT(SD_OK,graph_matrix_add(m,1,2,3,1));
    double d[3];size_t predecessor[3];
    ASSERT_EQ_INT(SD_OK,newsroom_network_paths(l,0,d,predecessor,NULL));
    ASSERT_EQ_DOUBLE(5,d[2],1e-9);
    SDTreeEdge edges[2];size_t count=0;double total=0;
    ASSERT_EQ_INT(SD_OK,newsroom_network_mst(m,0,edges,&count,&total,NULL));
    ASSERT_EQ_DOUBLE(5,total,1e-9);
    ASSERT_EQ_INT(SD_OK,newsroom_network_mst(m,1,edges,&count,&total,NULL));
    ASSERT_EQ_DOUBLE(5,total,1e-9);
    double *all=NULL;size_t *next=NULL;int cycle=0;
    ASSERT_EQ_INT(SD_OK,newsroom_network_all_pairs(m,&all,&next,&cycle,NULL));
    ASSERT_EQ_DOUBLE(5,all[2],1e-9);free(all);free(next);
    graph_list_free(l);graph_matrix_free(m);
}
int main(void) {test_events_and_overlap();test_allocation();test_graphs();return harness_summary();}
