#include "message.h"


Message::Message(int tag_arg, int size_arg, int payload_arg, int signature_arg, int vc_id_arg, int wavelength_id_arg) {  
  tag = tag_arg;
  size = size_arg;
  payload = payload_arg;
  signature = signature_arg;
  vc_id = vc_id_arg;
  wavelength_id = wavelength_id_arg;
}

Message::Message(Message* msg_to_copy) {  
  tag = msg_to_copy->tag;
  size = msg_to_copy->size;
  payload = msg_to_copy->payload;
  signature = msg_to_copy->signature;
  vc_id = msg_to_copy->vc_id;
  wavelength_id = msg_to_copy->wavelength_id;
}

Message::Message(int vc_id_arg, int wavelength_id_arg) {  
  tag = 0;
  size = 0;
  payload = -999;
  signature = 0;
  vc_id = vc_id_arg;
  wavelength_id = wavelength_id_arg;
}

Message::Message(){
}

Message::~Message() {
}

void Message::copy(Message* msg_to_copy) {  
  tag = msg_to_copy->tag;
  size = msg_to_copy->size;
  payload = msg_to_copy->payload;
  signature = msg_to_copy->signature;
  vc_id = msg_to_copy->vc_id;
  wavelength_id = msg_to_copy->wavelength_id;
}

int Message::get_signature() {
  return signature;
}

int Message::get_vc_id() {
  return vc_id;
}

int Message::get_wavelength_id() {
  return wavelength_id;
}

int Message::get_payload(){
	return payload;
}

std::string Message::to_string() {
  stringstream res;
  res << "[";
  res << "tag:" << tag << " | ";
  res << "size:" << size << " | ";
  res << "payload:" << payload << " | ";
  res << "signature:" << signature << " | ";
  res << "vc_id:" << vc_id << " | ";
  res << "wavelength_id:" << wavelength_id;
  res << "]";
  return res.str();
}