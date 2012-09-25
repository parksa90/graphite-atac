#pragma once

#include "fixed_types.h"
#include "contrib/orion/orion.h"

class RouterPowerModel
{
public:
   class BufferAccess
   {
   public:
      enum Type
      {
         READ = 0,
         WRITE,
         NUM_ACCESS_TYPES
      };
   };

   RouterPowerModel(float frequency, UInt32 num_input_ports, UInt32 num_output_ports, UInt32 num_flits_per_port_buffer, UInt32 flit_width);
   ~RouterPowerModel();

   // Update Dynamic Energy
   void updateDynamicEnergy(UInt32 num_flits, UInt32 num_packets = 1);
   void updateDynamicEnergyBuffer(BufferAccess::Type buffer_access_type, UInt32 num_bit_flips, UInt32 num_flits);
   void updateDynamicEnergyCrossbar(UInt32 num_bit_flips, UInt32 num_flits);
   void updateDynamicEnergySwitchAllocator(UInt32 num_requests, UInt32 num_packets);
   void updateDynamicEnergyClock(UInt32 num_events);

   // Get Dynamic Energy
   volatile double getDynamicEnergy()
   {  
      return (_total_dynamic_energy_buffer + _total_dynamic_energy_crossbar + 
              _total_dynamic_energy_switch_allocator + _total_dynamic_energy_clock);
   }
   volatile double getDynamicEnergyBuffer()           { return _total_dynamic_energy_buffer;             }
   volatile double getDynamicEnergyCrossbar()         { return _total_dynamic_energy_crossbar;           }
   volatile double getDynamicEnergySwitchAllocator()  { return _total_dynamic_energy_switch_allocator;   }
   volatile double getDynamicEnergyClock()            { return _total_dynamic_energy_clock;              }
   
   // Static Power
   volatile double getStaticPowerBuffer()             { return _orion_router->get_static_power_buf();    }
   volatile double getStaticPowerBufferCrossbar()     { return _orion_router->get_static_power_xbar();   }
   volatile double getStaticPowerSwitchAllocator()    { return _orion_router->get_static_power_sa();     }
   volatile double getStaticPowerClock()              { return _orion_router->get_static_power_clock();  }
   volatile double getStaticPower()
   {
      return (_orion_router->get_static_power_buf() + _orion_router->get_static_power_xbar() +
              _orion_router->get_static_power_sa() + _orion_router->get_static_power_clock());
   }

private:
   volatile float _frequency;
   UInt32 _num_input_ports;
   UInt32 _num_output_ports;
   UInt32 _num_flits_per_port_buffer;
   UInt32 _flit_width;

   OrionRouter* _orion_router;

   volatile double _total_dynamic_energy_buffer;
   volatile double _total_dynamic_energy_crossbar;
   volatile double _total_dynamic_energy_switch_allocator;
   volatile double _total_dynamic_energy_clock;

   void initializeCounters();
};
