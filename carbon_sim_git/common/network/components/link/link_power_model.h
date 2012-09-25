#pragma once

#include "fixed_types.h"

class LinkPowerModel
{
public:
   LinkPowerModel(float frequency, double link_length, UInt32 link_width)
      : _frequency(frequency)
      , _link_length(link_length)
      , _link_width(link_width)
      , _total_static_power(0.0)
      , _total_dynamic_energy(0.0)
   {}
   virtual ~LinkPowerModel() {}

   volatile double getStaticPower()    { return _total_static_power;    }
   volatile double getDynamicEnergy()  { return _total_dynamic_energy;  }
   
protected:
   // Input parameters
   volatile float _frequency;
   volatile double _link_length;
   UInt32 _link_width;
   
   // Output parameters 
   volatile double _total_static_power;
   volatile double _total_dynamic_energy;
};
