#include "filtered_ingestor.h"

#include "network.h"
#include "tile.h"
#include "capi.h"
#include "packet_type.h"
//#include "packet_data.h"

//#include "message.h"


FilteredIngestor::FilteredIngestor(Tile* m_tile_arg) {  
  // accept all messages by default for VCs that you are subscribed to
  filter_mask = 0;
  filter_match_operation = FilterOperations::ALL;
  filter_match_signature = 0;
  
  // initially don't subscribe to any VCs
  for(int i=0; i<MAX_VIRTUAL_CHANNELS; i++) {
    virtual_channel_memberships[i] = false;
  }
  out_to_processor = new queue<Message *>();
	
  m_tile = m_tile_arg;
}

FilteredIngestor::~FilteredIngestor() {
}

int FilteredIngestor::set_filter(unsigned int filter_mask_arg, unsigned int filter_match_operation_arg, unsigned int filter_match_signature_arg) {
  filter_mask = filter_mask_arg;
  filter_match_operation = filter_match_operation_arg;
  filter_match_signature = filter_match_signature_arg;
  cout << "Filter is set! mask: " << filter_mask << " op: " << filter_match_operation << " sig: " << filter_match_signature << endl;
  return 1;
}

void FilteredIngestor::set_network(Network *network_arg) {
  the_network = network_arg;
}

// only for testing purposes; not to be used to read messages that go into the processor.
// you probably want to be using dequeue_filtered if you're reading into the processor
NetPacket FilteredIngestor::read_from_network(PacketType pkt_type) {
  //TODO: FIX this
  //tile_id_t sending_tile = CAPI_ENDPOINT_ANY;
  //PacketType pkt_type = getPktTypeFromUserNetType((carbon_network_t) CARBON_NET_USER_1);
  return the_network->netRecvTypeNonBlock(pkt_type);
}

bool FilteredIngestor::passes_filter(NetPacket packet) {
  // first, try short circuits
  if(filter_match_operation == FilterOperations::ALL) { return true; }
  if(filter_match_operation == FilterOperations::NONE) { return false; }
  
  // otherwise, do the more sophisticated logic
  bool res;
  //unsigned int masked_message_signature = message->get_signature() & filter_mask;
  //HACK (seojin):
  /*
  PacketData *pdata;
  pdata = (PacketData*)packet.data;
  */
  //END HACK
  Message *msg;
  msg = (Message *)packet.data;
  unsigned int masked_message_signature = msg->get_signature() & filter_mask; //TODO: fix detail of way to convey signature.
  
  
  switch(filter_match_operation)
  {
    case FilterOperations::AND:
      res = masked_message_signature & filter_match_signature;
      break;
    case FilterOperations::OR:
      res = masked_message_signature | filter_match_signature;
      break;
    case FilterOperations::XOR:
      res = masked_message_signature ^ filter_match_signature;
      break;
    default:
      res = false;
      cout << "THIS SHOULDN'T HAPPEN. BAD FILTER STATE" << endl << endl;
      exit(-200);
  }
  return res; 
}

/* 
 * Returns true if this filtered_injestor (or, really, this core) is subscribed to
 * the virtual channel for this message (and can, therefore, receive it). Otherwise,
 * returns false.
 * 
 */
bool FilteredIngestor::subscribed_to_channel(Message *curr_message) {
  return virtual_channel_memberships[curr_message->get_vc_id()];
}

// note: naming of this function and above function (subscribed_to_channel) intentionally
// inconsistent because otherwise the function names are off by one character and that it 
// too error-prone.f
void FilteredIngestor::subscribe_to_virtual_channel(int vc_id) {
  virtual_channel_memberships[vc_id] = true;
}

void FilteredIngestor::unsubscribe_from_virtual_channel(int vc_id) {
  virtual_channel_memberships[vc_id] = false;
}


NetPacket FilteredIngestor::receive_filtered(PacketType pkt_type) { 
  NetPacket packet;
  //const int max_trial = 10;
  //for(int i = 0; i < max_trial; ++i){
  while(true){
    packet = read_from_network(pkt_type);
    //Message *msg = (Message *)packet.data;
    if(!packet.found){
      //cout << "[Empty buffer] " << pdata->payload << ", " << pdata->signature << endl;
      break;
    } if(passes_filter(packet) /*&& subscribed_to_channel(msg->get_vc_id())*/) {
      break;
    } else {
      // discard this message and don't pass to processor
      cout << "BOMBING OUT" ;
      cout << " = packet: from {"<< packet.sender.tile_id << ", " << packet.sender.core_type << "}, to {" << packet.receiver.tile_id << ", "<< packet.receiver.core_type << "}, type " << (SInt32)packet.type << ", len "<< packet.length <<endl;
    }
  }
  return packet;
}

NetPacket FilteredIngestor::receive_filtered_from(core_id_t src, PacketType pkt_type) { 
  NetPacket packet;
  //const int max_trial = 10;
  //for(int i = 0; i < max_trial; ++i){
  while(true){
    packet = the_network->netRecvNonBlock(src, pkt_type);
    //PacketData *pdata = (PacketData *)packet.data;
    if(!packet.found){
      //cout << "[Empty buffer] " << pdata->payload << ", " << pdata->signature << endl;
      break;
    } if(passes_filter(packet)) {
      break;
    } else {
      // discard this message and don't pass to processor
      cout << "BOMBING OUT" ;
      cout << " = packet: from {"<< packet.sender.tile_id << ", " << packet.sender.core_type << "}, to {" << packet.receiver.tile_id << ", "<< packet.receiver.core_type << "}, type " << (SInt32)packet.type << ", len "<< packet.length <<endl;
    }
  }
  return packet;
}









