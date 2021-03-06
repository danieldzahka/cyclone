/*
 * Copyright (c) 2015, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include <pthread.h>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/bind.hpp>
#include "../../../core/clock.hpp"
#include "../../../core/logging.hpp"
#include "../../../core/libcyclone.hpp"
#include <rte_launch.h>

unsigned long request_id = 0UL; // wrap around at MAX
std::vector<unsigned long> *replayq;
std::map<unsigned long, unsigned long> *clnt_map;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

unsigned long tx_block_cnt;
unsigned long tx_failed_cnt;
unsigned long total_latency;
unsigned long tx_begin_time;

int driver(void *arg);

typedef struct replay_st{
	unsigned long client_id;
	unsigned long latency;
}relay_t;

typedef struct driver_args_st {
  int leader;
  int me; 
  int mc;
  int replicas;
  int clients;
  int partitions;
  int buf_cap;
  void **handles;
  void operator() ()
  {
    (void)driver((void *)this);
  }
} driver_args_t;

typedef struct cb_st{
	unsigned long request_id;
}cb_t;


void async_callback(void *args, int code, unsigned long msg_latency){
	//cb_t *cb = (cb_t *)args;
	if(code == REP_SUCCESS){
		tx_block_cnt++;
	}else{
		tx_failed_cnt++;
	}
	//free(cb);
	total_latency += msg_latency; //timouts get added in to message latency
	if(tx_block_cnt >= 5000) {
		unsigned long total_elapsed_time = (rtc_clock::current_time() - tx_begin_time);
		std::cout << "LOAD = "
				<< ((double)1000000*tx_block_cnt)/total_elapsed_time
				<< " tx/sec "
				<< "LATENCY = "
				<< ((double)total_latency)/tx_block_cnt
				<< " us "
				<< std::endl;
		tx_block_cnt   = 0;
		tx_failed_cnt  = 0;
		total_latency  = 0;
		tx_begin_time = rtc_clock::current_time();
	}
}


int driver(void *arg)
{
  driver_args_t *dargs = (driver_args_t *)arg;
  int me = dargs->me; 
  int mc = dargs->mc;
  int replicas = dargs->replicas;
  int clients = dargs->clients;
  int partitions = dargs->partitions;
  void **handles = dargs->handles;
  char *buffer = new char[DISP_MAX_MSGSIZE];
  struct proposal *prop = (struct proposal *)buffer;
  srand(time(NULL));
  struct proposal *resp;
  int rpc_flags;
	int ret;

  int my_core;
	
  total_latency = 0;
  tx_block_cnt  = 0;
  tx_begin_time = rtc_clock::current_time();
  unsigned long payload = 0;
  //const char *payload_env = getenv("PAYLOAD");
  //if(payload_env != NULL) {
    //callback->payload = atol(payload_env);
  //}
  BOOST_LOG_TRIVIAL(info) << "PAYLOAD = " << payload;


	srand(rtc_clock::current_time());
  int partition;
	int newcb  = 1;
  for( ; ; ) {
    rpc_flags = 0;
    my_core = dargs->me % executor_threads;
		//cb_t *cb_args = (cb_t *)malloc(sizeof(cb_t));;
		//cb_args->request_id =request_id++;
		do{
			ret = make_rpc_async(handles[0],
				buffer,
				payload,
				async_callback,
				(void *)NULL,
				1UL << my_core,
				rpc_flags);
			if(ret == EMAX_INFLIGHT) {
				//BOOST_LOG_TRIVIAL(fatal) << "buffer full";
				continue;
			}
		}while(ret);
  }
  return 0;
}

int main(int argc, const char *argv[]) {
  if(argc != 11) {
    printf("Usage: %s client_id_start client_id_stop mc replicas" 
				"clients partitions cluster_config quorum_config_prefix" 
				"server_ports inflight_cap\n", argv[0]);
    exit(-1);
  }
  
  int client_id_start = atoi(argv[1]);
  int client_id_stop  = atoi(argv[2]);
  driver_args_t *dargs;
  void **prev_handles;

  replayq = new std::vector<unsigned long>();
  clnt_map = new std::map<unsigned long, unsigned long>();

  cyclone_network_init(argv[7], 1, atoi(argv[3]), 2 + client_id_stop - client_id_start);
  driver_args_t ** dargs_array = 
    (driver_args_t **)malloc((client_id_stop - client_id_start)*sizeof(driver_args_t *));
  for(int me = client_id_start; me < client_id_stop; me++) {
    dargs = (driver_args_t *) malloc(sizeof(driver_args_t));
    dargs_array[me - client_id_start] = dargs;
    if(me == client_id_start) {
      dargs->leader = 1;
    }
    else {
      dargs->leader = 0;
    }
    dargs->me = me;
    dargs->mc = atoi(argv[3]);
    dargs->replicas = atoi(argv[4]);
    dargs->clients  = atoi(argv[5]);
    dargs->partitions = atoi(argv[6]);
    dargs->buf_cap = atoi(argv[10]);
    dargs->handles = new void *[dargs->partitions];
    char fname_server[50];
    char fname_client[50];
    for(int i=0;i<dargs->partitions;i++) {
      sprintf(fname_server, "%s", argv[7]);
      sprintf(fname_client, "%s%d.ini", argv[8], i);
      dargs->handles[i] = cyclone_client_init(dargs->me,
					      dargs->mc,
					      1 + me - client_id_start,
					      fname_server,
					      atoi(argv[9]),
					      fname_client,
						  CLIENT_ASYNC,
                          dargs->buf_cap);
    }
  }
  for(int me = client_id_start; me < client_id_stop; me++) {
		cyclone_launch_clients(dargs_array[me-client_id_start]->handles[0],driver, dargs_array[me-client_id_start], 1+me-client_id_start);
  }
	rte_eal_mp_wait_lcore();
	// free vector and map
}
