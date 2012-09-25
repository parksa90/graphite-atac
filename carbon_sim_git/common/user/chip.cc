#include "chip.h"
#define CHAR_HACK

Chip::Chip() {  
}

Chip::~Chip() {
}

int Chip::get_next_vc_id() {
  return next_vc_id;
}


int Chip::register_virtual_channel(string name_arg) {
#ifdef CHAR_HACK
  char name = name_arg.c_str()[0];
#endif
  // TODO: when we port this to graphite, this needs to be locked appropriately to prevent race conditions.
  if(next_vc_id < MAX_VIRTUAL_CHANNELS) {
    int new_vc_id = next_vc_id++;
    //Chip::active_virtual_channels->insert(pair<string, int>(name, new_vc_id));
	 (*Chip::active_virtual_channels)[name] = new_vc_id;
    //return true;
	 //printf("\t name: %s \t vc_id: %d, found from map: %d, %d\n", name.c_str(), new_vc_id, (*Chip::active_virtual_channels)[name], Chip::active_virtual_channels->find(name)->second);
	 return new_vc_id;
  } else {
    // too many VC already created
    //return false; //FIXME; raise exception instead
	 return -1;
  }
}

int Chip::find_vc_id(string name_arg){
#ifdef CHAR_HACK
	char name = name_arg.c_str()[0];
#endif
	if (Chip::active_virtual_channels->find(name) == Chip::active_virtual_channels->end() || (*Chip::active_virtual_channels)[name] == 0){
		return -1;
	}
	return (*Chip::active_virtual_channels)[name];
}

void Chip::show_registered_virtual_channels(){
#ifdef CHAR_HACK
	map<char, int>::iterator iter;
#endif
#ifndef CHAR_HACK
	map<string, int>::iterator iter;
#endif
	for (iter = active_virtual_channels->begin(); iter != active_virtual_channels->end(); iter++) {
	#ifdef CHAR_HACK
		printf("< %c = %d > \n", iter->first, iter->second);
#endif
#ifndef CHAR_HACK
		printf("< %s = %d > \n", iter->first.c_str(), iter->second);
#endif
	   
	}
}

int Chip::next_vc_id = 10;

map<char, int> *Chip::active_virtual_channels = new map<char, int>();