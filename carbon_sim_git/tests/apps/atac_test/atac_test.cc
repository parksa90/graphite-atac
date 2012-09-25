#include <string>
#include <stdio.h>
#include <stdlib.h>
#include "carbon_user.h"
#include <assert.h>

//#define VERBOSE

int num_threads;
carbon_barrier_t rank_barrier;

void* thread_func(void* threadid);
VirtualChannel* init_virtual_channel(int rank);
void do_bcast(VirtualChannel*, int rank);
void receive_from_all(VirtualChannel *vc, int rank);
void receive_for_checkbuffer_perf_model_test(VirtualChannel *vc, int rank);
void change_filter_setting(int rank);
void test();

int main(int argc, char **argv)
{
   CarbonStartSim(argc, argv);
   printf("Starting all_to_all!\n");

   if (argc != 2)
   {
      fprintf(stderr, "Usage: ./all_to_all <num_threads>\n");
      CarbonStopSim();
      exit(-1);
   }
   num_threads = atoi(argv[1]);

   // Actually, there are 5 threads including the main thread
   carbon_thread_t threads[num_threads-1];

   CarbonBarrierInit(&rank_barrier, num_threads);

   VirtualChannel::create("testChannel1");
   
   for(int i = 0; i < num_threads-1; i++)
   {
       printf("Spawning thread: %d\n", i);
       threads[i] = CarbonSpawnThread(thread_func, (void *) i);
   }
  
   //test();
   thread_func((void*) (num_threads-1));
	
	
   for(int i = 0; i < num_threads-1; i++)
       CarbonJoinThread(threads[i]);
   
   printf("Ending all_to_all!\n");
   CarbonStopSim();
   return 0;
}

void* thread_func(void* threadid)
{
	long rank = (long)threadid;

  CAPI_Initialize(rank);
  CarbonBarrierWait(&rank_barrier);
  
  VirtualChannel *vc;
  
   
  vc = init_virtual_channel(rank);
  //printf("=========== init VC done (%ld) ==============\n", rank);
  
  do_bcast(vc, rank);
  //printf("=========== bcast done (%ld) ==============\n", rank);
  
  change_filter_setting(rank);
  //printf("=========== change filter done(%ld) ==============\n", rank);
  
  if(rank == 1)
     receive_for_checkbuffer_perf_model_test(vc, rank);
  else
     receive_from_all(vc, rank);
  //printf("=========== receive done(%ld) ==============\n", rank);

  return NULL;
}
/*
void test(){
	//1. Chip test.
	printf("before entering\n");
	Chip::show_registered_virtual_channels();
	int vc_id_1 = Chip::register_virtual_channel("apple");
	int vc_id_2 = Chip::register_virtual_channel("2");
	printf("after entering vc1: %d vc2: %d\n", vc_id_1, vc_id_2);
	Chip::show_registered_virtual_channels();
	if(vc_id_1 != Chip::find_vc_id("apple")){
		printf("chip test1 fail \n");
	}else{
		printf("chip test1 success \n");
	}
	if(vc_id_2 != Chip::find_vc_id("2")){
		printf("chip test2 fail \n");
	}else{
		printf("chip test2 success \n");
	}
	
	//2. direct test
	printf("=== DIRECT TEST of chip ====\n");
	map<string, int> *vcs = new map<string, int>();
	vcs->insert(pair<string, int>("a", 1));
	(*vcs)["a"] = 1;
	vcs->insert(pair<string, int>("b", 2));
	(*vcs)["b"] = 2;
	(*vcs)["c"] = 3;
	map<string, int>::iterator iter;
	for (iter = vcs->begin(); iter != vcs->end(); iter++) {
	   printf("< %s = %d > \n", iter->first.c_str(), iter->second);
	}
}
*/
VirtualChannel* init_virtual_channel(int rank)
{ //Creating local virtual channel object.
	VirtualChannel *vc = NULL;
	if(rank == 0){
      vc = VirtualChannel::createFromRegistered("testChannel1");
	}else{
		printf(" Trying: [reference Creation (%d)] \n", rank);
	   do{
			vc = VirtualChannel::createFromRegistered("testChannel1");
			if(vc == NULL){
			   printf(".. waiting for creation of VC\n");
				sleep(1);
			}
		}while(vc == NULL);
		printf(" [reference Creation (%d)] id: %d \n", rank, vc->get_id());
	}
   //assert(vc->get_owner_tile_id() == rank);
	return vc;
}

void do_bcast(VirtualChannel *vc, int rank)
{
	int payload_byte = rank*10;
	int message_signature_byte = rank;
	int tag_buffer = 0;
	#ifdef VERBOSE
   printf("[do_bcast] rank %d is broadcasting out message; value = %d\n", rank, payload_byte);
   #endif
   //CAPI_message_send_w_ex(sender_rank, receiver_rank, (char *) &pdata, sizeof(PacketData), CARBON_NET_USER_1);
	vc->send_message(payload_byte, message_signature_byte, tag_buffer);
	
   //printf("[do_bcast] rank %d finished broadcasting out message\n", rank);
}

void change_filter_setting(int rank){
   CAPI_endpoint_t tile = (CAPI_endpoint_t) rank;
   unsigned int filter_mask = 7;
   unsigned int filter_match_operation = 0; // TODO: put enum of filter type outside of filtered_ingestor.h
   unsigned int filter_match_signature = rank;
   CAPI_set_filter(tile, filter_mask, filter_match_operation, filter_match_signature);
}

void receive_from_all(VirtualChannel *vc, int rank)
{
   //TODO: set filter.
	int bonus = 0;
	int max_bonus = 0;
   for (int i = 0; i < num_threads + bonus && i < num_threads + max_bonus; i++)
   {
      Message msg;
      #ifdef VERBOSE
      printf("[receive_from_all] rank %d is about to receive message\n", rank);
		if(!(vc->receive_message(msg))){
         printf("[receive_from_all] rank ( %d ) no matching packet  \n", rank);
      } else{
         printf("[receive_from_all] rank ( %d ) received message; value = %d, signature = %d\n", rank, msg.get_payload(), msg.get_signature());
			//printf(("\t\t  " + msg.to_string() + "\n").c_str());
			bonus++;
		}
      #endif
      #ifndef VERBOSE
      vc->receive_message(msg);
      #endif
   }
}

void receive_for_checkbuffer_perf_model_test(VirtualChannel *vc, int rank)
{
   //TODO: set filter.
	//int bonus = 0;
   for (int i = 0; i < num_threads * 10; i++)
   {
      Message msg;
      #ifdef VERBOSE
      printf("[receive_from_all] rank %d is about to receive message\n", rank);
		
      //CAPI_message_receive_w_ex(sender_rank, receiver_rank, (char*) &pdata, sizeof(PacketData), CARBON_NET_USER_1);
		if(!(vc->receive_message(msg))){
         printf("[receive_from_all] rank ( %d ) no matching packet  \n", rank);
      } else{
         printf("[receive_from_all] rank ( %d ) received message; value = %d, signature = %d\n", rank, msg.get_payload(), msg.get_signature());
			//printf(("\t\t  " + msg.to_string() + "\n").c_str());
		}
      #endif
      #ifndef VERBOSE
      vc->receive_message(msg);
      #endif
   }
}

