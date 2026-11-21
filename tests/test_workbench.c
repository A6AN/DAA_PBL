#include "test_harness.h"
#include "workbench.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void test_all(void) {
    ASSERT_EQ_INT(36,workbench_count());
    FILE *file=tmpfile();
    ASSERT_TRUE(file!=NULL);
    if(!file)return;
    ASSERT_EQ_INT(SD_OK,workbench_list(file));
    for(size_t i=0;i<workbench_count();i++){
        long start=ftell(file);
        ASSERT_TRUE(start>=0);
        ASSERT_EQ_INT(SD_OK,workbench_run(i,file));
        ASSERT_TRUE(ftell(file)>start);
    }
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT,workbench_run(workbench_count(),file));
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT,workbench_run(0,NULL));
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT,workbench_list(NULL));
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT,workbench_run_all(NULL));
    rewind(file);
    char line[512];int entries=0;
    while(fgets(line,sizeof(line),file))if(strncmp(line,"Operation count:",16)==0)entries++;
    ASSERT_EQ_INT(36,entries);
    fclose(file);
    file=tmpfile();ASSERT_TRUE(file!=NULL);
    if(file){ASSERT_EQ_INT(SD_OK,workbench_run_all(file));fclose(file);}
}
int main(void) {test_all();return harness_summary();}
