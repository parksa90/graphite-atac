#pragma once

#include <string>
using std::string;

#include "link_model.h"
#include "fixed_types.h"
#include "electrical_link_power_model.h"

class NetPacket;
class NetworkModel;

class ElectricalLinkModel : public LinkModel
{
public:
   ElectricalLinkModel(NetworkModel* model, string link_type,
                       float link_frequency, double link_length, UInt32 link_width);
   ~ElectricalLinkModel();

   // processPacket() called at every link
   void processPacket(const NetPacket& pkt, UInt64& zero_load_delay);
   
   // Event counters
   UInt64 getTotalTraversals() { return _total_link_traversals; }

private:
   ElectricalLinkPowerModel* _power_model;
   
   // Event counters
   UInt64 _total_link_traversals;
};
