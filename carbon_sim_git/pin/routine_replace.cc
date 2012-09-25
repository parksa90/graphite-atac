#include "routine_replace.h"
#include "simulator.h"
#include "thread_manager.h"
#include "thread_scheduler.h"
#include "log.h"
#include "carbon_user.h"
#include "thread_support_private.h"

// -------------------------------------
// Begin Memory redirection stuff
#include "config.h"
#include "simulator.h"
#include "core.h"
#include "tile_manager.h"
#include "redirect_memory.h"
#include "thread_start.h"
#include "network.h"
#include "packet_type.h"
// End Memory redirection stuff
// --------------------------------------

#define CARBON_IARG_END IARG_INVALID

// ---------------------------------------------------------
// Memory Redirection
//
// Routine replacement needs to work a little differently
// We can't use RTN_Replace or RTN_ReplaceSignature because
// the application executes out of a different memory space
// (and hence also uses a different stack) that Pin isn't aware of
// Our solution is to insert a call before the routine executes 
// to a wrapper function that extracts the correct arguments from 
// the user stack (including populating memory pointers with data
// from simulated memory) and then calls the "replacement functions"
// within the pintool. This wrapper also writes back results to user
// memory before returning control to the correct return address
// in user space
//
// --------------------------------------------------------------

bool replaceUserAPIFunction(RTN& rtn, string& name)
{
   AFUNPTR msg_ptr = NULL;

   // TODO: Check that the starting stack is located below the text segment
   // thread management
   if (name == "main") msg_ptr = AFUNPTR (replacementMain);
   else if (name == "CarbonGetThreadToSpawn") msg_ptr = AFUNPTR(replacementGetThreadToSpawn);
   else if (name == "CarbonThreadStart") msg_ptr = AFUNPTR (replacementThreadStartNull);
   else if (name == "CarbonThreadExit") msg_ptr = AFUNPTR (replacementThreadExitNull);
   //else if (name == "CarbonGetCoreId") msg_ptr = AFUNPTR(replacementGetCoreId);
   else if (name == "CarbonGetTileId") msg_ptr = AFUNPTR(replacementGetTileId);
   else if (name == "CarbonDequeueThreadSpawnReq") msg_ptr = AFUNPTR (replacementDequeueThreadSpawnRequest);

   // PIN specific stack management
   else if (name == "CarbonPthreadAttrInitOtherAttr") msg_ptr = AFUNPTR(replacementPthreadAttrInitOtherAttr);

   // Carbon API

   else if (name == "CarbonStartSim") msg_ptr = AFUNPTR(replacementStartSimNull); 
   else if (name == "CarbonStopSim") msg_ptr = AFUNPTR(replacementStopSim);
   else if (name == "CarbonSpawnThread") msg_ptr = AFUNPTR(replacementSpawnThread);
   else if (name == "CarbonSpawnThreadOnTile") msg_ptr = AFUNPTR(replacementSpawnThreadOnTile);
   else if (name == "CarbonSchedSetAffinity") msg_ptr = AFUNPTR(replacementSchedSetAffinity);
   else if (name == "CarbonSchedGetAffinity") msg_ptr = AFUNPTR(replacementSchedGetAffinity);
   else if (name == "CarbonJoinThread") msg_ptr = AFUNPTR(replacementJoinThread);
   
   // CAPI
   else if (name == "CAPI_Initialize") msg_ptr = AFUNPTR(replacement_CAPI_Initialize);
   else if (name == "CAPI_rank") msg_ptr = AFUNPTR(replacement_CAPI_rank);
   else if (name == "CAPI_message_send_w") msg_ptr = AFUNPTR(replacement_CAPI_message_send_w);
   else if (name == "CAPI_message_receive_w") msg_ptr = AFUNPTR(replacement_CAPI_message_receive_w);
   else if (name == "CAPI_message_send_w_ex") msg_ptr = AFUNPTR(replacement_CAPI_message_send_w_ex);
   else if (name == "CAPI_message_receive_w_ex") msg_ptr = AFUNPTR(replacement_CAPI_message_receive_w_ex);
   // CAPI filtered ingestor
   else if (name == "CAPI_set_filter") msg_ptr = AFUNPTR(replacement_CAPI_set_filter);
	else if (name == "CAPI_filter_incoming_messages") msg_ptr = AFUNPTR(replacement_CAPI_filter_incoming_messages);
	else if (name == "CAPI_dequeue_filtered_message") msg_ptr = AFUNPTR(replacement_CAPI_dequeue_filtered_message);
	

   // synchronization
   else if (name == "CarbonMutexInit") msg_ptr = AFUNPTR(replacementMutexInit);
   else if (name == "CarbonMutexLock") msg_ptr = AFUNPTR(replacementMutexLock);
   else if (name == "CarbonMutexUnlock") msg_ptr = AFUNPTR(replacementMutexUnlock);
   else if (name == "CarbonCondInit") msg_ptr = AFUNPTR(replacementCondInit);
   else if (name == "CarbonCondWait") msg_ptr = AFUNPTR(replacementCondWait);
   else if (name == "CarbonCondSignal") msg_ptr = AFUNPTR(replacementCondSignal);
   else if (name == "CarbonCondBroadcast") msg_ptr = AFUNPTR(replacementCondBroadcast);
   else if (name == "CarbonBarrierInit") msg_ptr = AFUNPTR(replacementBarrierInit);
   else if (name == "CarbonBarrierWait") msg_ptr = AFUNPTR(replacementBarrierWait);

   // Enable/Disable/Reset Models
   else if (name == "CarbonEnableModels") msg_ptr = AFUNPTR(replacementEnableModels);
   else if (name == "CarbonDisableModels") msg_ptr = AFUNPTR(replacementDisableModels);

   // Resetting Cache Counters
   else if (name == "CarbonResetCacheCounters") msg_ptr = AFUNPTR(replacementResetCacheCounters);
   else if (name == "CarbonDisableCacheCounters") msg_ptr = AFUNPTR(replacementDisableCacheCounters);

   // pthread wrappers
   else if (name.find("pthread_create") != std::string::npos) msg_ptr = AFUNPTR(replacementPthreadCreate);
   else if (name.find("pthread_join") != std::string::npos) msg_ptr = AFUNPTR(replacementPthreadJoin);
   else if (name.find("pthread_exit") != std::string::npos) msg_ptr = AFUNPTR(replacementPthreadExitNull);

   // For Getting the Simulated Time
   else if (name == "CarbonGetTime") msg_ptr = AFUNPTR(replacementCarbonGetTime);

   // For Dynamic Frequency Scaling
   else if (name == "CarbonGetCoreFrequency") msg_ptr = AFUNPTR(replacementCarbonGetCoreFrequency);
   else if (name == "CarbonSetCoreFrequency") msg_ptr = AFUNPTR(replacementCarbonSetCoreFrequency);

   // Turn off performance modeling at _start()
   if (name == "_start")
   {
      RTN_Open (rtn);

      RTN_InsertCall (rtn, IPOINT_BEFORE,
                      AFUNPTR(Simulator::disablePerformanceModelsInCurrentProcess),
                      IARG_END);

      RTN_Close (rtn);
   }
   
   // Turn off performance modeling after main()
   if (name == "main")
   {
      RTN_Open (rtn);

      // Before main()
      if (! Sim()->getCfg()->getBool("general/trigger_models_within_application",false))
      {
         RTN_InsertCall(rtn, IPOINT_BEFORE,
               AFUNPTR(Simulator::enablePerformanceModelsInCurrentProcess),
               IARG_END);
      }

      RTN_InsertCall(rtn, IPOINT_BEFORE,
            AFUNPTR(CarbonInitModels),
            IARG_END);

      // After main()
      if (! Sim()->getCfg()->getBool("general/trigger_models_within_application",false))
      {
         RTN_InsertCall(rtn, IPOINT_AFTER,
               AFUNPTR(Simulator::disablePerformanceModelsInCurrentProcess),
               IARG_END);
      }
      
      RTN_Close (rtn);
   }

   // do replacement
   if (msg_ptr != NULL)
   {
      RTN_Open (rtn);

      RTN_InsertCall (rtn, IPOINT_BEFORE,
            msg_ptr,
            IARG_CONTEXT,
            IARG_END);

      RTN_Close (rtn);

      return true;
   }
   else
   {
      return false;
   }
}

void replacementMain (CONTEXT *ctxt)
{
   LOG_PRINT("In replacementMain");
   
   // The main process, which is the first thread (thread 0), waits for all thread spawners to be created.
   if (Sim()->getConfig()->getCurrentProcessNum() == 0)
   {
      LOG_PRINT("ReplaceMain start");
      
      Core *core = Sim()->getTileManager()->getCurrentCore();
      UInt32 num_processes = Sim()->getConfig()->getProcessCount();

      // For each process, send a message to the thread spawner for that process telling it that
      // we're initializing, wait until the thread spawners are all initialized.
      for (UInt32 i = 1; i < num_processes; i++)
      {
         // FIXME: 
         // This whole process should probably happen through the MCP
         core->getNetwork()->netSend (Sim()->getConfig()->getThreadSpawnerCoreId(i), SYSTEM_INITIALIZATION_NOTIFY, NULL, 0);

         // main thread clock is not affected by start-up time of other processes
         core->getNetwork()->netRecv (Sim()->getConfig()->getThreadSpawnerCoreId(i), core->getId(), SYSTEM_INITIALIZATION_ACK);
      }
      
      // Tell the thread spawner for each process that we're done initializing...even though we haven't?
      for (UInt32 i = 1; i < num_processes; i++)
      {
         core->getNetwork()->netSend (Sim()->getConfig()->getThreadSpawnerCoreId(i), SYSTEM_INITIALIZATION_FINI, NULL, 0);
      }

      spawnThreadSpawner(ctxt);

      LOG_PRINT("ReplaceMain end");

      return;
   }
   else
   {
      // This whole process should probably happen through the MCP
      Core *core = Sim()->getTileManager()->getCurrentCore();
      core->getNetwork()->netSend (Sim()->getConfig()->getMainThreadCoreId(), SYSTEM_INITIALIZATION_ACK, NULL, 0);
      core->getNetwork()->netRecv (Sim()->getConfig()->getMainThreadCoreId(), core->getId(), SYSTEM_INITIALIZATION_FINI);

      int res;
      ADDRINT reg_eip = PIN_GetContextReg (ctxt, REG_INST_PTR);

      PIN_LockClient();

      AFUNPTR thread_spawner;
      IMG img = IMG_FindByAddress(reg_eip);
      RTN rtn = RTN_FindByName(img, "CarbonThreadSpawner");
      thread_spawner = RTN_Funptr(rtn);

      PIN_UnlockClient();
      
      PIN_CallApplicationFunction (ctxt,
            PIN_ThreadId(),
            CALLINGSTD_DEFAULT,
            thread_spawner,
            PIN_PARG(int), &res,
            PIN_PARG(void*), NULL,
            PIN_PARG_END());

      Sim()->getThreadManager()->onThreadExit();

      while (!Sim()->finished())
         sched_yield();

      Simulator::release();

      exit (0);
   }
}

void replacementGetThreadToSpawn (CONTEXT *ctxt)
{
   Core *core = Sim()->getTileManager()->getCurrentCore();
   assert (core != NULL);
   
   ThreadSpawnRequest *req;
   ThreadSpawnRequest req_buf;
   initialize_replacement_args (ctxt,
         IARG_PTR, &req,
         CARBON_IARG_END);
   
   // Preserve REG_GAX across function call with
   // void return type
   ADDRINT ret_val = PIN_GetContextReg (ctxt, REG_GAX);

   CarbonGetThreadToSpawn (&req_buf);

   core->accessMemory(Core::NONE, Core::WRITE, (IntPtr) req, (char*) &req_buf, sizeof (ThreadSpawnRequest));

   retFromReplacedRtn (ctxt, ret_val);
}

void replacementThreadStartNull (CONTEXT *ctxt)
{
   ADDRINT ret_val = PIN_GetContextReg (ctxt, REG_GAX);
   retFromReplacedRtn (ctxt, ret_val);
}

void replacementThreadExitNull (CONTEXT *ctxt)
{
   ADDRINT ret_val = PIN_GetContextReg (ctxt, REG_GAX);
   retFromReplacedRtn (ctxt, ret_val);
}

void replacementGetTileId (CONTEXT *ctxt)
{
   ADDRINT ret_val = PIN_GetContextReg (ctxt, REG_GAX);
   CarbonGetTileId ();

   retFromReplacedRtn (ctxt, ret_val);
}

//void replacementGetCoreId (CONTEXT *ctxt)
//{
   //ADDRINT ret_val = PIN_GetContextReg (ctxt, REG_GAX);
   //CarbonGetCoreId ();

   //retFromReplacedRtn (ctxt, ret_val);
//}


void replacementDequeueThreadSpawnRequest (CONTEXT *ctxt)
{
   Core *core = Sim()->getTileManager()->getCurrentCore();
   assert (core);

   ThreadSpawnRequest *thread_req;
   ThreadSpawnRequest thread_req_buf;
   
   initialize_replacement_args (ctxt,
         IARG_PTR, &thread_req,
         CARBON_IARG_END);
   
   ADDRINT ret_val = PIN_GetContextReg (ctxt, REG_GAX);
   
   CarbonDequeueThreadSpawnReq (&thread_req_buf);

   core->accessMemory (Core::NONE, Core::WRITE, (IntPtr) thread_req, (char*) &thread_req_buf, sizeof (ThreadSpawnRequest));

   retFromReplacedRtn (ctxt, ret_val);
}

// PIN specific stack management
void replacementPthreadAttrInitOtherAttr(CONTEXT *ctxt)
{
   Core *core = Sim()->getTileManager()->getCurrentCore();
   assert(core != NULL);

   pthread_attr_t *attr;
   pthread_attr_t attr_buf;

   initialize_replacement_args(ctxt,
         IARG_PTR, &attr,
         CARBON_IARG_END);

   core->accessMemory (Core::NONE, Core::READ, (ADDRINT) attr, (char*) &attr_buf, sizeof (pthread_attr_t));

   ADDRINT ret_val = PIN_GetContextReg(ctxt, REG_GAX);

   SimPthreadAttrInitOtherAttr(&attr_buf);

   core->accessMemory (Core::NONE, Core::WRITE, (ADDRINT) attr, (char*) &attr_buf, sizeof (pthread_attr_t));

   retFromReplacedRtn(ctxt, ret_val);
}


void replacementStartSimNull (CONTEXT *ctxt)
{
   ADDRINT ret_val = 0; 
   retFromReplacedRtn (ctxt, ret_val);
}

void replacementStopSim (CONTEXT *ctxt)
{
   ADDRINT ret_val = PIN_GetContextReg (ctxt, REG_GAX);
   retFromReplacedRtn (ctxt, ret_val);
}

void replacementSpawnThread (CONTEXT *ctxt)
{
   thread_func_t func;
   void *arg;

   initialize_replacement_args (ctxt,
         IARG_PTR, &func,
         IARG_PTR, &arg,
         CARBON_IARG_END);

   ADDRINT ret_val = (ADDRINT) CarbonSpawnThread (func, arg);

   retFromReplacedRtn (ctxt, ret_val);

}

void replacementSpawnThreadOnTile (CONTEXT *ctxt)
{
   thread_func_t func;
   tile_id_t tile_id;
   void *arg;

   initialize_replacement_args (ctxt,
         IARG_UINT32, &tile_id,
         IARG_PTR, &func,
         IARG_PTR, &arg,
         CARBON_IARG_END);

   ADDRINT ret_val = (ADDRINT) CarbonSpawnThreadOnTile (tile_id, func, arg);

   retFromReplacedRtn (ctxt, ret_val);
}

void replacementSchedSetAffinity (CONTEXT *ctxt)
{
   thread_id_t thread_id;
   UInt32 cpusetsize;
   cpu_set_t *set;

   initialize_replacement_args (ctxt,
         IARG_UINT32, &thread_id,
         IARG_UINT32, &cpusetsize,
         IARG_PTR, &set,
         CARBON_IARG_END);

   cpu_set_t* set_cpy = CPU_ALLOC(cpusetsize);

   Core *core = Sim()->getTileManager()->getCurrentCore();
   core->accessMemory (Core::NONE, Core::READ, (ADDRINT) set, (char*) set_cpy, CPU_ALLOC_SIZE(cpusetsize));

   ADDRINT ret_val = (ADDRINT) CarbonSchedSetAffinity (thread_id, cpusetsize, set_cpy);

   retFromReplacedRtn (ctxt, ret_val);
}

void replacementSchedGetAffinity (CONTEXT *ctxt)
{
   thread_id_t thread_id;
   UInt32 cpusetsize;
   cpu_set_t *set;

   initialize_replacement_args (ctxt,
         IARG_UINT32, &thread_id,
         IARG_UINT32, &cpusetsize,
         IARG_PTR, &set,
         CARBON_IARG_END);

   cpu_set_t* set_cpy = CPU_ALLOC(cpusetsize);

   Core *core = Sim()->getTileManager()->getCurrentCore();
   core->accessMemory (Core::NONE, Core::READ, (ADDRINT) set, (char*) set_cpy, CPU_ALLOC_SIZE(cpusetsize));

   ADDRINT ret_val = (ADDRINT) CarbonSchedGetAffinity (thread_id, cpusetsize, set_cpy);

   core->accessMemory (Core::NONE, Core::WRITE, (ADDRINT) set, (char*) set_cpy, CPU_ALLOC_SIZE(cpusetsize));
   retFromReplacedRtn (ctxt, ret_val);
}

void replacementJoinThread (CONTEXT *ctxt)
{
   ADDRINT tid;

   initialize_replacement_args (ctxt,
         IARG_ADDRINT, &tid,
         CARBON_IARG_END);

   ADDRINT ret_val = PIN_GetContextReg (ctxt, REG_GAX);

   CarbonJoinThread ((int) tid);

   retFromReplacedRtn (ctxt, ret_val);
}

void replacement_CAPI_Initialize (CONTEXT *ctxt)
{
   // Only the user-threads (all of which are cores) call
   // the CAPI communication API functions
   __attribute(__unused__) Core *core = Sim()->getTileManager()->getCurrentCore();
   assert (core);
   
   int comm_id;
   initialize_replacement_args (ctxt,
         IARG_UINT32, &comm_id,
         CARBON_IARG_END);

   ADDRINT ret_val = PIN_GetContextReg (ctxt, REG_GAX);

   CAPI_Initialize (comm_id);

   retFromReplacedRtn (ctxt, ret_val);
}

void replacement_CAPI_rank (CONTEXT *ctxt)
{
   // Only the user-threads (all of which are cores) call
   // the CAPI communication API functions
   Core *core = Sim()->getTileManager()->getCurrentCore();
   assert (core);
   
   int *core_id;
   int core_id_buf;
   CAPI_return_t ret_val;

   initialize_replacement_args (ctxt,
         IARG_PTR, &core_id,
         CARBON_IARG_END);

   ret_val = CAPI_rank (&core_id_buf);
   
   core->accessMemory (Core::NONE, Core::WRITE, (ADDRINT) core_id, (char*) &core_id_buf, sizeof (int));

   retFromReplacedRtn (ctxt, ret_val);
}

void replacement_CAPI_message_send_w (CONTEXT *ctxt)
{
   // Only the user-threads (all of which are cores) call
   // the CAPI communication API functions
   Core *core = Sim()->getTileManager()->getCurrentCore();
   assert (core);
   
   CAPI_endpoint_t sender;
   CAPI_endpoint_t receiver;
   char *buffer;
   int size;
   CAPI_return_t ret_val = 0;

   initialize_replacement_args (ctxt,
         IARG_UINT32, &sender,
         IARG_UINT32, &receiver,
         IARG_PTR, &buffer,
         IARG_UINT32, &size,
         CARBON_IARG_END);

   char *buf = new char [size];
   core->accessMemory (Core::NONE, Core::READ, (ADDRINT) buffer, buf, size);
   ret_val = CAPI_message_send_w (sender, receiver, buf, size);

   delete [] buf;
   retFromReplacedRtn (ctxt, ret_val);
}

void replacement_CAPI_message_send_w_ex (CONTEXT *ctxt)
{
   // Only the user-threads (all of which are cores) call
   // the CAPI communication API functions
   Core *core = Sim()->getTileManager()->getCurrentCore();
   assert (core);
   
   CAPI_endpoint_t sender;
   CAPI_endpoint_t receiver;
   char *buffer;
   int size;
   carbon_network_t net_type;
   CAPI_return_t ret_val = 0;

   initialize_replacement_args (ctxt,
         IARG_UINT32, &sender,
         IARG_UINT32, &receiver,
         IARG_PTR, &buffer,
         IARG_UINT32, &size,
         IARG_UINT32, &net_type,
         CARBON_IARG_END);

   char *buf = new char [size];
   core->accessMemory (Core::NONE, Core::READ, (ADDRINT) buffer, buf, size);
   ret_val = CAPI_message_send_w_ex (sender, receiver, buf, size, net_type);

   delete [] buf;
   retFromReplacedRtn (ctxt, ret_val);
}

void replacement_CAPI_message_receive_w (CONTEXT *ctxt)
{
   // Only the user-threads (all of which are cores) call
   // the CAPI communication API functions
   Core *core = Sim()->getTileManager()->getCurrentCore();
   assert (core);
   
   CAPI_endpoint_t sender;
   CAPI_endpoint_t receiver;
   char *buffer;
   int size;
   CAPI_return_t ret_val = 0;

   initialize_replacement_args (ctxt,
         IARG_UINT32, &sender,
         IARG_UINT32, &receiver,
         IARG_PTR, &buffer,
         IARG_UINT32, &size,
         CARBON_IARG_END);

   char *buf = new char [size];
   ret_val = CAPI_message_receive_w (sender, receiver, buf, size);
   core->accessMemory (Core::NONE, Core::WRITE, (ADDRINT) buffer, buf, size);

   delete [] buf;
   retFromReplacedRtn (ctxt, ret_val);
}

void replacement_CAPI_message_receive_w_ex (CONTEXT *ctxt)
{
   // Only the user-threads (all of which are cores) call
   // the CAPI communication API functions
   Core *core = Sim()->getTileManager()->getCurrentCore();
   assert (core);
   
   CAPI_endpoint_t sender;
   CAPI_endpoint_t receiver;
   char *buffer;
   int size;
   carbon_network_t net_type;
   CAPI_return_t ret_val = 0;

   initialize_replacement_args (ctxt,
         IARG_UINT32, &sender,
         IARG_UINT32, &receiver,
         IARG_PTR, &buffer,
         IARG_UINT32, &size,
         IARG_UINT32, &net_type,
         CARBON_IARG_END);

   char *buf = new char [size];
   ret_val = CAPI_message_receive_w_ex (sender, receiver, buf, size, net_type);
   core->accessMemory (Core::NONE, Core::WRITE, (ADDRINT) buffer, buf, size);

   delete [] buf;
   retFromReplacedRtn (ctxt, ret_val);
}

void replacement_CAPI_set_filter (CONTEXT *ctxt)
{
   // Only the user-threads (all of which are cores) call
   // the CAPI communication API functions
	/*
   Core *core = Sim()->getTileManager()->getCurrentCore();
   assert (core);
   */
   CAPI_endpoint_t tile;
   unsigned int filter_mask;
   unsigned int filter_match_operation;
   unsigned int filter_match_signature;
   CAPI_return_t ret_val = 0;

   initialize_replacement_args (ctxt,
         IARG_UINT32, &tile,
         IARG_UINT32, &filter_mask,
         IARG_UINT32, &filter_match_operation,
         IARG_UINT32, &filter_match_signature,
         CARBON_IARG_END);

   ret_val = CAPI_set_filter (tile, filter_mask, filter_match_operation, filter_match_signature);

   retFromReplacedRtn (ctxt, ret_val);
}

void replacement_CAPI_filter_incoming_messages (CONTEXT *ctxt)
{
   // Only the user-threads (all of which are cores) call
   // the CAPI communication API functions
   Core *core = Sim()->getTileManager()->getCurrentCore();
   assert (core);
   
   CAPI_endpoint_t tile;
   CAPI_return_t ret_val = 0;

   initialize_replacement_args (ctxt,
         IARG_UINT32, &tile,
         CARBON_IARG_END);

   ret_val = CAPI_filter_incoming_messages(tile);

   retFromReplacedRtn (ctxt, ret_val);
}

void replacement_CAPI_dequeue_filtered_message (CONTEXT *ctxt)
{
   // Only the user-threads (all of which are cores) call
   // the CAPI communication API functions
   Core *core = Sim()->getTileManager()->getCurrentCore();
   assert (core);
	
	CAPI_endpoint_t tile_id_arg;
   int id;
	char *msg_to_receive;
	int msg_size;
	
	CAPI_return_t ret_val = 0;

   initialize_replacement_args (ctxt,
         IARG_UINT32, &tile_id_arg,
         IARG_UINT32, &id,
         IARG_PTR, &msg_to_receive,
         IARG_UINT32, &msg_size,
         CARBON_IARG_END);

   char *buf = new char [msg_size];
   ret_val = CAPI_dequeue_filtered_message (tile_id_arg, id, buf, msg_size);
   core->accessMemory (Core::NONE, Core::WRITE, (ADDRINT) msg_to_receive, buf, msg_size);

   delete [] buf;
   retFromReplacedRtn (ctxt, ret_val);
}


void replacementMutexInit (CONTEXT *ctxt)
{
   // Only the user-threads (all of which are cores) call
   // the Carbon synch API functions
   Core *core = Sim()->getTileManager()->getCurrentCore();
   assert (core);
   
   carbon_mutex_t *mux;
   initialize_replacement_args (ctxt,
         IARG_PTR, &mux,
         CARBON_IARG_END);
   
   carbon_mutex_t mux_buf;
   ADDRINT ret_val = PIN_GetContextReg (ctxt, REG_GAX);
   
   core->accessMemory (Core::NONE, Core::READ, (ADDRINT) mux, (char*) &mux_buf, sizeof (mux_buf));
   CarbonMutexInit (&mux_buf);
   core->accessMemory (Core::NONE, Core::WRITE, (ADDRINT) mux, (char*) &mux_buf, sizeof (mux_buf));

   retFromReplacedRtn (ctxt, ret_val);
}

void replacementMutexLock (CONTEXT *ctxt)
{
   // Only the user-threads (all of which are cores) call
   // the Carbon synch API functions
   Core *core = Sim()->getTileManager()->getCurrentCore();
   assert (core);
   
   carbon_mutex_t *mux;
   initialize_replacement_args (ctxt,
         IARG_PTR, &mux,
         CARBON_IARG_END);
   
   carbon_mutex_t mux_buf;
   ADDRINT ret_val = PIN_GetContextReg (ctxt, REG_GAX);
   
   core->accessMemory (Core::NONE, Core::READ, (ADDRINT) mux, (char*) &mux_buf, sizeof (mux_buf));
   CarbonMutexLock (&mux_buf);

   retFromReplacedRtn (ctxt, ret_val);
}

void replacementMutexUnlock (CONTEXT *ctxt)
{
   // Only the user-threads (all of which are cores) call
   // the Carbon synch API functions
   Core *core = Sim()->getTileManager()->getCurrentCore();
   assert (core);
   
   carbon_mutex_t *mux;
   initialize_replacement_args (ctxt,
         IARG_PTR, &mux,
         CARBON_IARG_END);
   
   carbon_mutex_t mux_buf;
   ADDRINT ret_val = PIN_GetContextReg (ctxt, REG_GAX);
   
   core->accessMemory (Core::NONE, Core::READ, (ADDRINT) mux, (char*) &mux_buf, sizeof (mux_buf));
   CarbonMutexUnlock (&mux_buf);

   retFromReplacedRtn (ctxt, ret_val);
}

void replacementCondInit (CONTEXT *ctxt)
{
   // Only the user-threads (all of which are cores) call
   // the Carbon synch API functions
   Core *core = Sim()->getTileManager()->getCurrentCore();
   assert (core);
   
   carbon_cond_t *cond;
   initialize_replacement_args (ctxt,
         IARG_PTR, &cond,
         CARBON_IARG_END);
   
   carbon_cond_t cond_buf;
   ADDRINT ret_val = PIN_GetContextReg (ctxt, REG_GAX);
   
   core->accessMemory (Core::NONE, Core::READ, (ADDRINT) cond, (char*) &cond_buf, sizeof (cond_buf));
   CarbonCondInit (&cond_buf);
   core->accessMemory (Core::NONE, Core::WRITE, (ADDRINT) cond, (char*) &cond_buf, sizeof (cond_buf));

   retFromReplacedRtn (ctxt, ret_val);
}

void replacementCondWait (CONTEXT *ctxt)
{
   // Only the user-threads (all of which are cores) call
   // the Carbon synch API functions
   Core *core = Sim()->getTileManager()->getCurrentCore();
   assert (core);
   
   carbon_cond_t *cond;
   carbon_mutex_t *mux;
   initialize_replacement_args (ctxt,
         IARG_PTR, &cond,
         IARG_PTR, &mux,
         CARBON_IARG_END);
   
   carbon_cond_t cond_buf;
   carbon_mutex_t mux_buf;
   ADDRINT ret_val = PIN_GetContextReg (ctxt, REG_GAX);
   
   core->accessMemory (Core::NONE, Core::READ, (ADDRINT) cond, (char*) &cond_buf, sizeof (cond_buf));
   core->accessMemory (Core::NONE, Core::READ, (ADDRINT) mux, (char*) &mux_buf, sizeof (mux_buf));
   CarbonCondWait (&cond_buf, &mux_buf);

   retFromReplacedRtn (ctxt, ret_val);
}

void replacementCondSignal (CONTEXT *ctxt)
{
   // Only the user-threads (all of which are cores) call
   // the Carbon synch API functions
   Core *core = Sim()->getTileManager()->getCurrentCore();
   assert (core);
   
   carbon_cond_t *cond;
   initialize_replacement_args (ctxt,
         IARG_PTR, &cond,
         CARBON_IARG_END);
   
   carbon_cond_t cond_buf;
   ADDRINT ret_val = PIN_GetContextReg (ctxt, REG_GAX);
   
   core->accessMemory (Core::NONE, Core::READ, (ADDRINT) cond, (char*) &cond_buf, sizeof (cond_buf));
   CarbonCondSignal (&cond_buf);

   retFromReplacedRtn (ctxt, ret_val);
}

void replacementCondBroadcast (CONTEXT *ctxt)
{
   carbon_cond_t *cond;
   initialize_replacement_args (ctxt,
         IARG_PTR, &cond,
         CARBON_IARG_END);
   
   carbon_cond_t cond_buf;
   ADDRINT ret_val = PIN_GetContextReg (ctxt, REG_GAX);
   
   Core *core = Sim()->getTileManager()->getCurrentCore();
   assert (core);
   core->accessMemory (Core::NONE, Core::READ, (ADDRINT) cond, (char*) &cond_buf, sizeof (cond_buf));
   CarbonCondBroadcast (&cond_buf);

   retFromReplacedRtn (ctxt, ret_val);
}

void replacementBarrierInit (CONTEXT *ctxt)
{
   carbon_barrier_t *barrier;
   UInt32 count;
   initialize_replacement_args (ctxt,
         IARG_PTR, &barrier,
         IARG_UINT32, &count,
         CARBON_IARG_END);
   
   carbon_barrier_t barrier_buf;
   ADDRINT ret_val = PIN_GetContextReg (ctxt, REG_GAX);
   
   Core *core = Sim()->getTileManager()->getCurrentCore();
   assert (core);
   core->accessMemory (Core::NONE, Core::READ, (ADDRINT) barrier, (char*) &barrier_buf, sizeof (barrier_buf));
   CarbonBarrierInit (&barrier_buf, count);
   core->accessMemory (Core::NONE, Core::WRITE, (ADDRINT) barrier, (char*) &barrier_buf, sizeof (barrier_buf));

   retFromReplacedRtn (ctxt, ret_val);
}

void replacementBarrierWait (CONTEXT *ctxt)
{
   carbon_barrier_t *barrier;
   initialize_replacement_args (ctxt,
         IARG_ADDRINT, &barrier,
         CARBON_IARG_END);
   
   carbon_barrier_t barrier_buf;
   ADDRINT ret_val = PIN_GetContextReg (ctxt, REG_GAX);
   
   Core *core = Sim()->getTileManager()->getCurrentCore();
   assert (core);
   core->accessMemory (Core::NONE, Core::READ, (ADDRINT) barrier, (char*) &barrier_buf, sizeof (barrier_buf));
   CarbonBarrierWait (&barrier_buf);

   retFromReplacedRtn (ctxt, ret_val);
}

//assumption: the pthread_create from the ThreadSpawner is the first pthread_create() we encounter, and we need to let it fall through
bool pthread_create_first_time = true; 

void replacementPthreadCreate (CONTEXT *ctxt)
{
   tile_id_t current_tile_id =  
                        Sim()->getTileManager()->getCurrentTileID();
   tile_id_t current_thread_spawner_id =  
                        Sim()->getConfig()->getCurrentThreadSpawnerTileNum();
   
   if ( pthread_create_first_time || current_tile_id == current_thread_spawner_id )
   {
      //let the pthread_create call fall through 
      pthread_create_first_time = false;
   }
   else
   {
      pthread_t *thread_id;
      pthread_attr_t *attributes;
      thread_func_t func;
      void *func_arg;

      initialize_replacement_args (ctxt,
            IARG_PTR, &thread_id,
            IARG_PTR, &attributes,
            IARG_PTR, &func,
            IARG_PTR, &func_arg,
            CARBON_IARG_END);

      // Throw warning message if ((attr)) field is non-NULL
      // TODO: Add Graphite support for using a non-NULL attribute field
      LOG_ASSERT_WARNING(attributes == NULL, "pthread_create() is using a non-NULL ((attr)) parameter. "
                                              "Unsupported currently.");
      
      carbon_thread_t new_thread_id = CarbonSpawnThread(func, func_arg);
      
      Core *core = Sim()->getTileManager()->getCurrentCore();
      assert (core);
      
      core->accessMemory(Core::NONE, Core::WRITE, (IntPtr) thread_id, (char*) &new_thread_id, sizeof (pthread_t));
      
      //pthread_create() expects a return value of 0 on success
      ADDRINT ret_val = 0;
      retFromReplacedRtn (ctxt, ret_val);
   }
}

void replacementPthreadJoin (CONTEXT *ctxt)
{
   pthread_t thread_id;
   void** return_value;

   initialize_replacement_args (ctxt,
         IARG_PTR, &thread_id,
         IARG_PTR, &return_value,
         CARBON_IARG_END);

   // Throw warning message if ((thread_return)) field is non-NULL
   // TODO: The return_value needs to be set, but CarbonJoinThread() provides no return value.
   LOG_ASSERT_WARNING (return_value == NULL, "pthread_join() is expecting a return value through the "
                                             "((thread_return)) parameter. Unsuported currently.");
   
   CarbonJoinThread ((carbon_thread_t) thread_id);

   //pthread_join() expects a return value of 0 on success
   ADDRINT ret_val = 0;
   retFromReplacedRtn (ctxt, ret_val);
}

void replacementPthreadExitNull (CONTEXT *ctxt)
{
   ADDRINT ret_val = PIN_GetContextReg (ctxt, REG_GAX);
   retFromReplacedRtn (ctxt, ret_val);
}

void replacementEnableModels(CONTEXT* ctxt)
{
   CarbonEnableModels();

   ADDRINT ret_val = PIN_GetContextReg(ctxt, REG_GAX);
   retFromReplacedRtn(ctxt, ret_val);
}

void replacementDisableModels(CONTEXT* ctxt)
{
   CarbonDisableModels();

   ADDRINT ret_val = PIN_GetContextReg(ctxt, REG_GAX);
   retFromReplacedRtn(ctxt, ret_val);
}

void replacementResetCacheCounters (CONTEXT *ctxt)
{
   CarbonResetCacheCounters();
   
   ADDRINT ret_val = PIN_GetContextReg (ctxt, REG_GAX);
   retFromReplacedRtn (ctxt, ret_val);
}

void replacementDisableCacheCounters (CONTEXT *ctxt)
{
   CarbonDisableCacheCounters();
   
   ADDRINT ret_val = PIN_GetContextReg (ctxt, REG_GAX);
   retFromReplacedRtn (ctxt, ret_val);
}

void replacementCarbonGetTime(CONTEXT *ctxt)
{
   UInt64 ret_val = CarbonGetTime();

   retFromReplacedRtn(ctxt, ret_val);
}

void replacementCarbonGetCoreFrequency(CONTEXT *ctxt)
{
   float* core_frequency;

   initialize_replacement_args(ctxt,
         IARG_PTR, &core_frequency,
         CARBON_IARG_END);

   volatile float core_frequency_buf;
   CarbonGetCoreFrequency(&core_frequency_buf);

   Core* core = Sim()->getTileManager()->getCurrentCore();
   core->accessMemory(Core::NONE, Core::WRITE, (IntPtr) core_frequency, (char*) &core_frequency_buf, sizeof(core_frequency_buf));

   ADDRINT ret_val = PIN_GetContextReg(ctxt, REG_GAX);
   retFromReplacedRtn(ctxt, ret_val);
}

void replacementCarbonSetCoreFrequency(CONTEXT *ctxt)
{
   float* core_frequency;

   initialize_replacement_args(ctxt,
         IARG_PTR, &core_frequency,
         CARBON_IARG_END);

   volatile float core_frequency_buf;
   Core* core = Sim()->getTileManager()->getCurrentCore();
   core->accessMemory(Core::NONE, Core::READ, (IntPtr) core_frequency, (char*) &core_frequency_buf, sizeof(core_frequency_buf));

   CarbonSetCoreFrequency(&core_frequency_buf);

   ADDRINT ret_val = PIN_GetContextReg(ctxt, REG_GAX);
   retFromReplacedRtn(ctxt, ret_val);
}

void initialize_replacement_args (CONTEXT *ctxt, ...)
{
#ifdef TARGET_IA32
   va_list vl;
   va_start (vl, ctxt);
   int type;
   ADDRINT addr;
   ADDRINT ptr;
   ADDRINT buffer;
   unsigned int count = 0;
   Core *core = Sim()->getTileManager()->getCurrentCore();
   assert (core);

   do
   {
      type = va_arg (vl, int);
      addr = PIN_GetContextReg (ctxt, REG_STACK_PTR) + ((count + 1) * sizeof (ADDRINT));
     
      core->accessMemory (Core::NONE, Core::READ, addr, (char*) &buffer, sizeof (ADDRINT));
      switch (type)
      {
         case IARG_ADDRINT:
            ptr = va_arg (vl, ADDRINT);
            * ((ADDRINT*) ptr) = buffer;
            count++;
            break;

         case IARG_PTR:
            ptr = va_arg (vl, ADDRINT);
            * ((ADDRINT*) ptr) = buffer;
            count++;
            break;

         case IARG_UINT32:
            ptr = va_arg (vl, ADDRINT);
            * ((UInt32*) ptr) = (UInt32) buffer;
            count++;
            break;

         case CARBON_IARG_END:
            break;

         default:
            assert (false);
            break;
      }
   } while (type != CARBON_IARG_END);
#endif

#ifdef TARGET_X86_64
   va_list vl;
   va_start (vl, ctxt);
   int type;
   ADDRINT ptr;
   ADDRINT val;
   unsigned int count = 0;
   REG reg_sequence[] = {REG_GDI, REG_GSI, REG_GDX, REG_GCX, REG_R8, REG_R9};

   do
   {
      LOG_ASSERT_ERROR (count < 6, "Don't support more than 6 function call arguments for replaced functions");

      type = va_arg (vl, int);
      val = PIN_GetContextReg (ctxt, reg_sequence[count]);
      LOG_PRINT("function args(%i) -> 0x%x", count, (IntPtr) val);
     
      switch (type)
      {
         case IARG_ADDRINT:
            ptr = va_arg (vl, ADDRINT);
            * ((ADDRINT*) ptr) = val;
            count++;
            break;

         case IARG_PTR:
            ptr = va_arg (vl, ADDRINT);
            * ((ADDRINT*) ptr) = val;
            count++;
            break;

         case IARG_UINT32:
            ptr = va_arg (vl, ADDRINT);
            * ((UInt32*) ptr) = (UInt32) val;
            count++;
            break;

         case CARBON_IARG_END:
            break;

         default:
            assert (false);
            break;
      }
   } while (type != CARBON_IARG_END);
#endif
}

void retFromReplacedRtn (CONTEXT *ctxt, ADDRINT ret_val)
{
   ADDRINT esp = PIN_GetContextReg (ctxt, REG_STACK_PTR);
   ADDRINT next_ip = emuRet (&esp, 0, sizeof (ADDRINT), false);

   PIN_SetContextReg (ctxt, REG_GAX, ret_val);
   PIN_SetContextReg (ctxt, REG_STACK_PTR, esp);
   PIN_SetContextReg (ctxt, REG_INST_PTR, next_ip);

   PIN_ExecuteAt (ctxt);
}

void setupCarbonSpawnThreadSpawnerStack (CONTEXT *ctx)
{
   // FIXME: 
   // This will clearly need to change somewhat in the multi-process case
   // We can go back to our original scheme of having the "main" thread 
   // on processes other than 0 execute the thread spawner, in which case
   // this will probably just work as is

   ADDRINT esp = PIN_GetContextReg (ctx, REG_STACK_PTR);
   ADDRINT ret_ip = * (ADDRINT*) esp;

   Core *core = Sim()->getTileManager()->getCurrentCore();
   assert (core);

   core->accessMemory(Core::NONE, Core::WRITE, (IntPtr) esp, (char*) &ret_ip, sizeof (ADDRINT));
}

void setupCarbonThreadSpawnerStack (CONTEXT *ctx)
{
   if (Sim()->getConfig()->getCurrentProcessNum() == 0)
      return;

   ADDRINT esp = PIN_GetContextReg (ctx, REG_STACK_PTR);
   ADDRINT ret_ip = * (ADDRINT*) esp;
   ADDRINT p = * (ADDRINT*) (esp + sizeof (ADDRINT));

   Core *core = Sim()->getTileManager()->getCurrentCore();
   assert (core);

   core->accessMemory (Core::NONE, Core::WRITE, (IntPtr) esp, (char*) &ret_ip, sizeof (ADDRINT));
   core->accessMemory (Core::NONE, Core::WRITE, (IntPtr) (esp + sizeof (ADDRINT)), (char*) &p, sizeof (ADDRINT));
}


