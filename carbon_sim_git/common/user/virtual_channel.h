#ifndef VIRTUALCHANNEL_H
#define VIRTUALCHANNEL_H

#include "common.h"
#include "message.h"
#include "capi.h"

#define UNINITIALIZED_VC_ID -1

class Chip;
class Core;

class VirtualChannel {
  int id;  // TODO: enforce that this id is globally unique
   int owner_tile_id; //used when send message.
  bool active;
  //Core* associated_core;
  
  set<int> member_ranks; // TODO: kill this?
    
  VirtualChannel(); // set current core as associated core. getCurrentTileID();
  VirtualChannel(int tile_id_arg);
public: 
  ~VirtualChannel();
  static VirtualChannel *create(string name); /* factory method */
  static VirtualChannel *createFromRegistered(string name);
  int get_id();
  int get_owner_tile_id();
  bool get_active_status();
  void set_active_status(bool active_arg);
  //Core *get_associated_core();
  
  void add_member(int rank_to_add);
  set<int> list_members();
  void remove_member(int rank_to_remove);

  bool receive_message(Message& msg_buffer);
  bool send_message(int payload_byte, int message_signature_byte, int tag_buffer);
};

#endif