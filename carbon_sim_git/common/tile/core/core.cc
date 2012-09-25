#include "core.h"
#include "main_core.h"
#include "tile.h"
#include "network.h"
#include "syscall_model.h"
#include "sync_client.h"
#include "network_types.h"
#include "memory_manager.h"
#include "pin_memory_manager.h"
#include "clock_skew_minimization_object.h"
#include "core_model.h"
#include "simulator.h"
#include "log.h"

//#define TEST_ATAC
#ifdef TEST_ATAC
#include "atac_perf_model_tester.h"
#endif

using namespace std;

int Core::s_rambda_buffer_count[MAX_CORES][MAX_CORES] = {{0,}, };
bool Core::s_block_send[MAX_CORES] = {false, };


Core::Core(Tile *tile, core_type_t core_type)
   : m_tile(tile)
   , m_core_id((core_id_t) {tile->getId(), core_type})
   , m_core_state(IDLE)
   , m_pin_memory_manager(NULL)
{
   m_core_model = CoreModel::create(this);
   m_sync_client = new SyncClient(this);
   m_syscall_model = new SyscallMdl(tile->getNetwork());
   m_clock_skew_minimization_client =
      ClockSkewMinimizationClient::create(Sim()->getCfg()->getString("clock_skew_minimization/scheme","none"), this);
 
   m_network = m_tile->getNetwork(); 
   m_shmem_perf_model = m_tile->getShmemPerfModel();
   m_memory_manager = m_tile->getMemoryManager();
    
   if (Config::getSingleton()->isSimulatingSharedMemory())
      m_pin_memory_manager = new PinMemoryManager(this);
      
//ATAC fix start
   message_buffer = new MessageBuffer();
	//TODO: seojin use config file to set min_processing_time.
	UInt64 min_processing_time = 1;
	m_queue_model = QueueModel::create("history_tree", min_processing_time);
	CHECKBUFFER_COST = (UInt64) Sim()->getCfg()->getInt("atac/design/delay_to_access_queue", 0);
   cout << "[ATAC setting] delay_to_access_queue: " << CHECKBUFFER_COST << endl;
   LAMBDA_QUEUE_LENGTH = (UInt64) Sim()->getCfg()->getInt("atac/design/lambda_queue_length", 100);
   cout << "[ATAC setting] lambda_queue_length: " << LAMBDA_QUEUE_LENGTH << endl;
//ATAC fix end
}

Core::~Core()
{
   LOG_PRINT("Deleting core on tile %d", m_core_id.tile_id);

   if (m_pin_memory_manager)
      delete m_pin_memory_manager;

   if (m_clock_skew_minimization_client)
      delete m_clock_skew_minimization_client;
   delete m_syscall_model;
   delete m_sync_client;
   delete m_core_model;
   
   delete message_buffer;//ATAC fix 
}

int Core::filter_incoming_messages(carbon_network_t net_type) { //Just one round.
  
  PacketType pkt_type = getPktTypeFromUserNetType(net_type); //Clean this up.
	
  int total_incoming = 0;
  for(int i=0; i<NUMBER_OF_WAVELENGTHS; i++) {
    core_id_t sender_core = (core_id_t) {i, this->getCoreType()};
	  NetPacket packet = m_tile->m_filtered_ingestor->receive_filtered_from(sender_core, pkt_type); //CHECK whether the dimension of sender ids matches with number of wavelengths.
	  //Check whether it is not found.
	  if(packet.found){
		  //TODO: ATAC PerfModel
        #ifdef TEST_ATAC
            AtacTester::modifyPacketTime(packet);
        #endif
		  UInt64 delay = m_queue_model->computeQueueDelay(packet.time, 10);
			packet.time += delay; //TODO: check correctness.
		  cout<< "** delay for a packet is " << delay << " PACKET.TIME = " << packet.time << " time elapsed = " << packet.time - packet.init_time 
			    << " <sender, receiver>: <" << packet.sender.tile_id << ", " << packet.receiver.tile_id << ">" << endl;
		  //TODO:manipulate packet.time to include the queuing delay.
			m_core_model->increment_packet_processed_count();
			//m_core_model->increment_packet_delay(delay);
         m_core_model->increment_packet_delay(packet.time - packet.init_time);
			
		  
		  if(!message_buffer->enqueue_message( packet )) //TODO: create message buffer in constructor.
		     return -1; //TODO: raise exception here 
		  total_incoming++;
	  }
  }
  return total_incoming;
}

int Core::dequeue_filtered_message(int virtual_channel_id, char* msg_to_receive, int msg_size) {
   NetPacket packet =  message_buffer->dequeue_filtered_message(virtual_channel_id);
	Message *msg = (Message *)packet.data;
   
   getPerformanceModel()->queueDynamicInstruction(new CheckBufferInstruction((UInt64) CHECKBUFFER_COST));
   
	if(msg == NULL) //TODO: fix this to up-to-date. 
		return 0;
      
	//TODO:check packet.time to determine.
	memcpy(msg_to_receive, msg, msg_size);
	return msg_size;
}

int Core::coreSendW(int sender, int receiver, char* buffer, int size, carbon_network_t net_type)
{
   PacketType pkt_type = getPktTypeFromUserNetType(net_type);

   core_id_t receiver_core = (core_id_t) {receiver, this->getCoreType()};
   
   SInt32 sent;
   if (receiver == CAPI_ENDPOINT_ALL){
      cout<< " capi endpoint all inside" << endl;
      //ATAC fix start
      if (s_block_send[sender]){
         cout<< " Cannot broadcast due to contention" << endl;
         return -1; //Cannot broadcast due to contention.
      }
      for (int i = 0; i < MAX_CORES; i++){
         Core::s_rambda_buffer_count[i][sender]++;
         if (Core::s_rambda_buffer_count[i][sender] >= LAMBDA_QUEUE_LENGTH - 4){
            //cout<< "[ATAC] STOP SEND ALERT for " << i <<endl;
            s_block_send[i] = true; //STOP SEND ALERT
         }
      }
      //ATAC fix end
      sent = m_tile->getNetwork()->netBroadcast(pkt_type, buffer, size);
   } else{
      cout<< " p 2 p inside" << endl;
      sent = m_tile->getNetwork()->netSend(receiver_core, pkt_type, buffer, size);
   }
   
   
   //CHECK buffer is over the limit.
   LOG_ASSERT_ERROR(sent == size, "Bytes Sent(%i), Message Size(%i)", sent, size);

   return sent == size ? 0 : -1;
}

int Core::coreRecvW(int sender, int receiver, char* buffer, int size, carbon_network_t net_type)
{
   PacketType pkt_type = getPktTypeFromUserNetType(net_type);
   //ATAC fix start
   Core::s_rambda_buffer_count[receiver][sender]--;
   if (Core::s_rambda_buffer_count[receiver][sender] == LAMBDA_QUEUE_LENGTH - 4 - 1){
      s_block_send[sender] = false; //STOP SEND ALERT REMOVE
      cout<< "[ATAC] STOP SEND ALERT REMOVED for " << sender <<endl;
   }
   //ATAC fix end

   core_id_t sender_core = (core_id_t) {sender, getCoreType()};

   NetPacket packet;
   if (sender == CAPI_ENDPOINT_ANY)
      //packet = m_tile->getNetwork()->netRecvType(pkt_type, m_core_id);
      packet = m_tile->m_filtered_ingestor->receive_filtered(pkt_type);
   else
      packet = m_tile->getNetwork()->netRecv(sender_core, m_core_id, pkt_type);

   LOG_PRINT("Got packet: from {%i, %i}, to {%i, %i}, type %i, len %i", packet.sender.tile_id, packet.sender.core_type, packet.receiver.tile_id, packet.receiver.core_type, (SInt32)packet.type, packet.length);

   LOG_ASSERT_ERROR((unsigned)size == packet.length, "Tile: User thread requested packet of size: %d, got a packet from %d of size: %d", size, sender, packet.length);

   memcpy(buffer, packet.data, size);

   // De-allocate dynamic memory
   // Is this the best place to de-allocate packet.data ??
   delete [](Byte*)packet.data;

   return (unsigned)size == packet.length ? 0 : -1;
}

PacketType Core::getPktTypeFromUserNetType(carbon_network_t net_type)
{
   switch(net_type)
   {
      case CARBON_NET_USER_1:
         return USER_1;

      case CARBON_NET_USER_2:
         return USER_2;

      default:
         LOG_PRINT_ERROR("Unrecognized User Network(%u)", net_type);
         return (PacketType) -1;
   }
}

Core::State 
Core::getState()
{
   ScopedLock scoped_lock(m_core_state_lock);
   return m_core_state;
}

void
Core::setState(State core_state)
{
   ScopedLock scoped_lock(m_core_state_lock);
   m_core_state = core_state;
}
