#include "test_harness.h"
#include "scenarios.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *capture(unsigned number,int *status) {
    FILE *file=tmpfile();
    if(!file){*status=SD_ERR_OUT_OF_MEMORY;return NULL;}
    *status=newsroom_run_scenario(number,file);
    if(fflush(file)!=0 || fseek(file,0,SEEK_END)!=0){fclose(file);return NULL;}
    long size=ftell(file);
    if(size<0 || fseek(file,0,SEEK_SET)!=0){fclose(file);return NULL;}
    char *text=malloc((size_t)size+1);
    if(!text){fclose(file);return NULL;}
    if(fread(text,1,(size_t)size,file)!=(size_t)size){free(text);fclose(file);return NULL;}
    text[size]='\0';fclose(file);return text;
}
static void assert_has(const char *text,const char *needle) {
    ASSERT_TRUE(text!=NULL && strstr(text,needle)!=NULL);
}
static void test_scenarios(void) {
    for(unsigned i=1;i<=3;i++){
        int status=SD_ERR_INVALID_ARGUMENT;
        char *text=capture(i,&status);
        ASSERT_EQ_INT(SD_OK,status);ASSERT_TRUE(text!=NULL);
        if(text){
            char label[32];(void)snprintf(label,sizeof(label),"Scenario %u:",i);
            assert_has(text,label);
            if(i==1){assert_has(text,"Dashboard: articles=48");assert_has(text,"textual-overlap pair");assert_has(text,"Verification allocation: assigned=");}
            if(i==2){assert_has(text,"Before: assigned=");assert_has(text,"After: assigned=");assert_has(text,"unassigned=17");assert_has(text,"articles=73");}
            if(i==3){assert_has(text,"Dataset size: 2000");assert_has(text,"Sorted output equality: yes");assert_has(text,"Rabin-Karp:");assert_has(text,"KMP:");}
        }
        if(i<3){
            int status2=SD_ERR_INVALID_ARGUMENT;char *again=capture(i,&status2);
            ASSERT_EQ_INT(SD_OK,status2);
            ASSERT_TRUE(text!=NULL && again!=NULL && strcmp(text,again)==0);
            free(again);
        }
        free(text);
    }
    FILE *file=tmpfile();ASSERT_TRUE(file!=NULL);
    if(file){
        ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT,newsroom_run_scenario(0,file));
        ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT,newsroom_run_scenario(4,file));
        fclose(file);
    }
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT,newsroom_run_scenario(1,NULL));
}
int main(void) {test_scenarios();return harness_summary();}
