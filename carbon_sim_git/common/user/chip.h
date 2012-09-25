#ifndef CHIP_H
#define CHIP_H

#include "common.h"

#define MAX_VIRTUAL_CHANNELS 1024
#define NUMBER_OF_WAVELENGTHS 100

class VirtualChannel;
class Chip {
  static int next_vc_id;  // this is a magic hack; this should really be a property of FilteredInjestor and updated when a new virtual channel is created. Discover of a new virtual channel's creation possibly happens via global broadcast message on one of the system-level (reserved) channnels
  
  
public: 
  static map<char, int> *active_virtual_channels;
  Chip();
  ~Chip();
  static int get_next_vc_id();
  static int find_vc_id(string name);
  static int register_virtual_channel(string name);  
  static void show_registered_virtual_channels();
};

#endif