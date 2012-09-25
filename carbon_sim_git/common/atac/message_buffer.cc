#include "message_buffer.h"
#include "message.h"

MessageBuffer::MessageBuffer() {  
  for(int i=0; i<MAX_VIRTUAL_CHANNELS; i++) {
    message_queues[i] = new queue<NetPacket>();
  }
}

MessageBuffer::~MessageBuffer() {
}

bool MessageBuffer::enqueue_message(NetPacket packet_to_enqueue) {
  Message *message_to_enqueue = (Message*) packet_to_enqueue.data;
  int vc_id = message_to_enqueue->get_vc_id();
  assert(vc_id >= 0);
  assert(vc_id < MAX_VIRTUAL_CHANNELS);
  message_queues[vc_id]->push(packet_to_enqueue);
  return true;
}

NetPacket MessageBuffer::dequeue_filtered_message(int virtual_channel_id) {
  if(message_queues[virtual_channel_id]->size() > 0) {
    NetPacket message_to_return = message_queues[virtual_channel_id]->front();
    message_queues[virtual_channel_id]->pop();
    return message_to_return;
  } else {
    return NetPacket();
  }
}