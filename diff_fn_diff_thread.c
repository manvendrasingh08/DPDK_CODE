#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <rte_eal.h>
#include <rte_lcore.h>
#include <rte_launch.h>



static void woo(void* arg){
    unsigned int lcore_id = rte_lcore_id();
    printf("%s() running on lcore %u\n", __func__, lcore_id);

}

static void foo(void* arg){
    unsigned int lcore_id = rte_lcore_id();
    printf("%s() running on lcore %u\n",__func__, lcore_id);
    woo(NULL);
}

int main(int argc, char** argv){

    int ret = rte_eal_init(argc, argv);
    if(ret< 0){
        rte_exit(EXIT_FAILURE, "EAL initlaization failed");
    }

  rte_eal_remote_launch(foo, NULL, 1);
  rte_eal_remote_launch(woo, NULL, 2);

  rte_eal_wait_lcore(1);
  rte_eal_wait_lcore(2);
  
  return 0;
}