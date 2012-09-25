#include "simulator.h"
#include "tile_manager.h"
#include "config.h"
#include "tile.h"
#include "carbon_user.h"
#include "log.h"

CAPI_return_t CAPI_rank(int *tile_id)
{
   *tile_id = CarbonGetTileId();
   return 0;
}

CAPI_return_t CAPI_Initialize(int rank)
{
   Sim()->getTileManager()->initializeCommId(rank);
   return 0;
}

CAPI_return_t CAPI_message_send_w(CAPI_endpoint_t sender, 
      CAPI_endpoint_t receiver, 
      char *buffer, 
      int size)
{
   Core *core = Sim()->getTileManager()->getCurrentCore();

   LOG_PRINT("SimSendW - sender: %d, recv: %d, size: %d", sender, receiver, size);

   tile_id_t sending_tile = Config::getSingleton()->getTileFromCommId(sender);
   
   tile_id_t receiving_tile = CAPI_ENDPOINT_ALL;
   if (receiver != CAPI_ENDPOINT_ALL)
      receiving_tile = Config::getSingleton()->getTileFromCommId(receiver);

   if(sending_tile == INVALID_TILE_ID)
       return CAPI_SenderNotInitialized;
   if(receiving_tile == INVALID_TILE_ID)
       return CAPI_ReceiverNotInitialized;

   return core ? core->coreSendW(sending_tile, receiving_tile, buffer, size, (carbon_network_t) CARBON_NET_USER_1) : CAPI_SenderNotInitialized;
}

CAPI_return_t CAPI_message_send_w_ex(CAPI_endpoint_t sender, 
      CAPI_endpoint_t receiver, 
      char *buffer, 
      int size,
      carbon_network_t net_type)
{
   Core *core = Sim()->getTileManager()->getCurrentCore();

   LOG_PRINT("SimSendW - sender: %d, recv: %d, size: %d", sender, receiver, size);

   tile_id_t sending_tile = Config::getSingleton()->getTileFromCommId(sender);
   
   tile_id_t receiving_tile = CAPI_ENDPOINT_ALL;
   if (receiver != CAPI_ENDPOINT_ALL)
      receiving_tile = Config::getSingleton()->getTileFromCommId(receiver);

   if(sending_tile == INVALID_TILE_ID)
       return CAPI_SenderNotInitialized;
   if(receiving_tile == INVALID_TILE_ID)
       return CAPI_ReceiverNotInitialized;

   return core ? core->coreSendW(sending_tile, receiving_tile, buffer, size, net_type) : CAPI_SenderNotInitialized;
}

CAPI_return_t CAPI_message_receive_w(CAPI_endpoint_t sender, 
      CAPI_endpoint_t receiver,
      char *buffer, 
      int size)
{
   Core *core = Sim()->getTileManager()->getCurrentCore();

   LOG_PRINT("SimRecvW - sender: %d, recv: %d, size: %d", sender, receiver, size);

   tile_id_t sending_tile = CAPI_ENDPOINT_ANY;
   if (sender != CAPI_ENDPOINT_ANY)
      sending_tile = Config::getSingleton()->getTileFromCommId(sender);
   
   tile_id_t receiving_tile = Config::getSingleton()->getTileFromCommId(receiver);

   if(sending_tile == INVALID_TILE_ID)
       return CAPI_SenderNotInitialized;
   if(receiving_tile == INVALID_TILE_ID)
       return CAPI_ReceiverNotInitialized;

   return core ? core->coreRecvW(sending_tile, receiving_tile, buffer, size, (carbon_network_t) CARBON_NET_USER_1) : CAPI_ReceiverNotInitialized;
}

CAPI_return_t CAPI_message_receive_w_ex(CAPI_endpoint_t sender, 
      CAPI_endpoint_t receiver,
      char *buffer, 
      int size,
      carbon_network_t net_type)
{
   Core *core = Sim()->getTileManager()->getCurrentCore();

   LOG_PRINT("SimRecvW - sender: %d, recv: %d, size: %d", sender, receiver, size);

   tile_id_t sending_tile = CAPI_ENDPOINT_ANY;
   if (sender != CAPI_ENDPOINT_ANY)
      sending_tile = Config::getSingleton()->getTileFromCommId(sender);
   
   tile_id_t receiving_tile = Config::getSingleton()->getTileFromCommId(receiver);

   if(sending_tile == INVALID_TILE_ID)
       return CAPI_SenderNotInitialized;
   if(receiving_tile == INVALID_TILE_ID)
       return CAPI_ReceiverNotInitialized;

   return core ? core->coreRecvW(sending_tile, receiving_tile, buffer, size, net_type) : CAPI_ReceiverNotInitialized;
}

CAPI_return_t CAPI_set_filter(CAPI_endpoint_t tile_id_arg, unsigned int filter_mask, unsigned int filter_match_operation, unsigned int filter_match_signature){
   tile_id_arg = Sim()->getTileManager()->getCurrentTileID(); //HACK
   tile_id_t tile_id = Config::getSingleton()->getTileFromCommId(tile_id_arg);
   Tile *tile = Sim()->getTileManager()->getTileFromID(tile_id);
   //Core *core = Sim()->getTileManager()->getCurrentCore();
   //return core ? core->m_tile->m_filtered_ingestor->set_filter(filter_mask, filter_match_operation, filter_match_signature) : CAPI_ReceiverNotInitialized;
   return tile->m_filtered_ingestor->set_filter(filter_mask, filter_match_operation, filter_match_signature);
}

void CAPI_subscribe_to_virtual_channel(CAPI_endpoint_t tile_id_arg, int vc_id){
   tile_id_arg = Sim()->getTileManager()->getCurrentTileID();//HACK
   tile_id_t tile_id = Config::getSingleton()->getTileFromCommId(tile_id_arg);
   Tile *tile = Sim()->getTileManager()->getTileFromID(tile_id);
//   return
	 tile->m_filtered_ingestor->subscribe_to_virtual_channel(vc_id);
}

void CAPI_unsubscribe_from_virtual_channel(CAPI_endpoint_t tile_id_arg, int vc_id){
   tile_id_arg = Sim()->getTileManager()->getCurrentTileID(); //HACK
   tile_id_t tile_id = Config::getSingleton()->getTileFromCommId(tile_id_arg);
   Tile *tile = Sim()->getTileManager()->getTileFromID(tile_id);
   //return 
	 tile->m_filtered_ingestor->unsubscribe_from_virtual_channel(vc_id);
}

CAPI_return_t CAPI_filter_incoming_messages(CAPI_endpoint_t tile_id_arg){
   tile_id_arg = Sim()->getTileManager()->getCurrentTileID(); //HACK
   tile_id_t tile_id = Config::getSingleton()->getTileFromCommId(tile_id_arg);
   Tile *tile = Sim()->getTileManager()->getTileFromID(tile_id);
   tile->getCore()->filter_incoming_messages(CARBON_NET_USER_1);
	return 1;
}

CAPI_return_t CAPI_dequeue_filtered_message(CAPI_endpoint_t tile_id_arg, int id, char *msg_to_receive, int msg_size){
   tile_id_arg = Sim()->getTileManager()->getCurrentTileID(); //HACK
   tile_id_t tile_id = Config::getSingleton()->getTileFromCommId(tile_id_arg);
   Tile *tile = Sim()->getTileManager()->getTileFromID(tile_id);
   return tile->getCore()->dequeue_filtered_message(id, msg_to_receive, msg_size);
}