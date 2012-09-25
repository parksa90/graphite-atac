#ifndef FILTERED_INGESTOR_H
#define FILTERED_INGESTOR_H

#include "network.h"
#include "tile.h"
#include "capi.h"
#include "packet_type.h"

#include "message.h"
#include "common.h"
#include "chip.h"


/*
 * FILTER OPERATION
 *  PASS message through only if ((message.signature & filter_mask) (filter_operation) filter_match_signature ) is TRUE
 *
 * filter_operations are defined in the enum e in namespace FilterOperations
 */

//class Network; // forward declaration to prevent circular include dependency
//class NetPacket;

namespace FilterOperations { 
  enum e { ALL, NONE, AND, OR, XOR }; 
};

class FilteredIngestor {
  
  unsigned int filter_mask;
  unsigned int filter_match_operation;
  unsigned int filter_match_signature;

  bool virtual_channel_memberships[MAX_VIRTUAL_CHANNELS];
	
  queue<Message*> *out_to_processor;
  Network *the_network;
  
  Tile *m_tile;

  
  NetPacket read_from_network(PacketType pkt_type);
  
public: 
  FilteredIngestor(Tile* m_tile_arg);
  ~FilteredIngestor();
  void set_network(Network *network);
  bool passes_filter(NetPacket packet);
  NetPacket receive_filtered(PacketType pkt_type);
  NetPacket receive_filtered_from(core_id_t src, PacketType pkt_type);
  
  int set_filter(unsigned int filter_mask, unsigned int filter_match_operation, unsigned int filter_match_signature);
  void subscribe_to_virtual_channel(int vc_id);
	void unsubscribe_from_virtual_channel(int vc_id);
	bool subscribed_to_channel(Message *curr_message);
};
  

// TODO: set up operation enums
#endif 