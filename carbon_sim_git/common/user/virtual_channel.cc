
#include "chip.h"

#include "carbon_user.h"
#include "capi.h"

#include "virtual_channel.h"



VirtualChannel::VirtualChannel() {  
  id = UNINITIALIZED_VC_ID;
  active = false;  
}

VirtualChannel::VirtualChannel(int tile_id_arg) {  
  id = UNINITIALIZED_VC_ID;
  active = false;  
  owner_tile_id = tile_id_arg;
}

VirtualChannel::~VirtualChannel() {
}

VirtualChannel *VirtualChannel::create(string name) {
  VirtualChannel *new_vc = new VirtualChannel();
  int vc_id = Chip::register_virtual_channel(name);
  new_vc->id = vc_id;
  new_vc->set_active_status((vc_id == -1) ? false : true);
  return new_vc;
}

VirtualChannel *VirtualChannel::createFromRegistered(string name) {
  int vc_id = Chip::find_vc_id(name);
  if(vc_id == -1){
     return NULL;
  }else{
     VirtualChannel *new_vc = new VirtualChannel();
     new_vc->id = vc_id;
     return new_vc;
  }
}

int VirtualChannel::get_id() {
  return id;
}

int VirtualChannel::get_owner_tile_id(){
   return owner_tile_id;
}

bool VirtualChannel::get_active_status() {
  return active;
}

void VirtualChannel::set_active_status(bool active_arg) {
  active = active_arg;
}


/***********************************************************/

// TODO: something is off here becuase VirtualChannels only are core-local. They don't have global 
// knowledge of who else is a member of the channel. Therefore, it's weird to specify the "rank to add".
// It's also weird to keep track of member_ranks because, frankly, this virtual channel (which only exists on this core)
// only knows about the fact that it's a member. It doesn't know who else is a member. Only the programmer does.
// Virtual channels need to be refactored to address this issue.

//TODO: add subscription part!!
void VirtualChannel::add_member(int rank_to_add) { 
  member_ranks.insert(rank_to_add);
  //associated_core->m_tile->m_filtered_ingestor->subscribe_to_virtual_channel(id); //graphite side //CHECKED
}

// TODO: this is deprecated
set<int> VirtualChannel::list_members() {
  return member_ranks;
}

//TODO: add subscription part!!
void VirtualChannel::remove_member(int rank_to_remove) {
  member_ranks.erase(rank_to_remove);
  //associated_core->m_tile->m_filtered_ingestor->unsubscribe_from_virtual_channel(id); //graphite side //CHECKED
}

// returns the next Message from the MessageBuffer associated with this virtual channel (or NULL)
bool VirtualChannel::receive_message(Message& msg_buffer){
  CAPI_filter_incoming_messages(owner_tile_id);
  //associated_core->filter_incoming_messages(CARBON_NET_USER_1); //graphite side
  //Message* msg_to_copy = associated_core->dequeue_filtered_message(id); //graphite side
  int found = CAPI_dequeue_filtered_message(owner_tile_id, id, (char *) &msg_buffer, sizeof(Message));
  return (found != 0) ? true : false;
}

bool VirtualChannel::send_message(int payload_byte, int message_signature_byte, int tag_buffer) {
  int size_arg = sizeof(int);
  Message *msg = new Message(tag_buffer, size_arg, payload_byte, message_signature_byte, id, owner_tile_id);  
  CAPI_return_t ret = CAPI_message_send_w_ex((CAPI_endpoint_t) owner_tile_id, (CAPI_endpoint_t) CAPI_ENDPOINT_ALL, (char *) msg, sizeof(Message), CARBON_NET_USER_1);
  if(ret == 0) //Successful transfer.
     return true;
  else //Either core is not initialized or size of sent and msg are different.
     return false; //ret == -1 if size mismatch. otherwise ret == CAPI_ReceiverNotInitialized.
}

