#ifndef MESSAGE_H
#define MESSAGE_H

#include "common.h"


class Message {

  int tag;  /* todo: remove this, most likely */
  int size;
  int payload; /* todo: fix this; make 64 bit */
  int signature;
  int vc_id;
  int wavelength_id;

public: 
  Message(int, int, int, int, int, int);
	Message(Message* msg_to_copy);
	Message(int, int);
	Message();
  ~Message();
	void copy(Message* msg_to_copy);
  std::string to_string();
  int get_signature();
  int get_vc_id();
  int get_wavelength_id();  
  int get_payload();
};

#endif