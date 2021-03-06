/*
 * test cases emulating Noria (OSDI 2018) vote workload
 */

#include <string>
#include <pmemds.h>
#include <climits>

#include "../include/pmemds-client.h"
#include "../include/pmemds-common.h"
#include "../include/hashmap/hashmap-client.h"
#include "../include/priority_queue/priority_queue-client.h"
#include "gtest/gtest.h"
#include "test_client.h"
#include "test_common.hpp"


#define NPARTITIONS 4
#define NARTICLES 10

namespace {

    int partition(pm_rpc_t *op){
        int partition;
        unsigned int op_id = OP_ID(op->meta);
        if(op_id == PUT || op_id == INSERT || op_id == INCREASE_PRIO || op_id == DECREASE_PRIO){
            partition = op->key%NPARTITIONS;
            return partition;
        }
        return INT_MAX; // not being used
    }


    class voteTest : public ::testing::Test {
    protected:
        const uint16_t art_st = 0;
        const uint16_t votes_st = 1;

        /* Code here will be called immediately after the constructor (right
           before each test). */
        void SetUp() override {
            setup_pmem(pmem_path);
            /* server side data-structure creation */
            pmLib = new pmemds::PMLib(pmem_path, NPARTITIONS);

            testClient = new pmemdsclient::ShardedTestClient(pmLib,partition);
            testClient->open("testApp");

            /* create art and vote structures */
            art = new pmemdsclient::HashMapEngine(testClient,art_st,100,0UL,1);
            art->create(PM_CREAT);
            votes = new pmemdsclient::PriorityQueueEngine(testClient,votes_st,100,0UL,1);
            votes->create(PM_CREAT);
            /* put some articles */

            for(int i = 0; i < NARTICLES; i++){
                art->put(i, "Article #" + std::to_string(i));
                votes->insert(i,0);
            }

        }

        /* Code here will be called immediately after each test (right
           before the destructor). */
        void TearDown() override {

            art->remove();
            // our prio-queue is volatile still.
            //votes->remove();
            testClient->close();
            delete(testClient);
            delete(pmLib);
        }

        // Objects declared here can be used by all tests in the test case for Foo.
        pm_rpc_t request,response;
        pmemdsclient::ShardedTestClient *testClient;
        pmemds::PMLib *pmLib;
        pmemdsclient::HashMapEngine *art;
        pmemdsclient::PriorityQueueEngine *votes;


    };



    TEST_F(voteTest, VoteRTest) {
            ASSERT_EQ(testClient->topk(),OK);
    }


    TEST_F(voteTest, VoteIncrTest) {
        ASSERT_EQ(votes->increase_prio(1,1),OK);
        ASSERT_EQ(testClient->topk(),OK);
    }


    /* retrieve top K post from the vote db */
    /*Test_F(voteTest, VoteTopKTest){



    }*/

}
