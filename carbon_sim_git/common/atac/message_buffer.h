#ifndef MESSAGEBUFFER_H
#define MESSAGEBUFFER_H

#include "common.h"
#include "chip.h"
//#include "message.h"
#include "network.h"

class MessageBuffer {
  queue<NetPacket> *message_queues[MAX_VIRTUAL_CHANNELS];

public: 
  MessageBuffer();
  ~MessageBuffer();
  bool enqueue_message(NetPacket packet_to_enqueue);
  NetPacket dequeue_filtered_message(int virtual_channel_id);
};

#endif