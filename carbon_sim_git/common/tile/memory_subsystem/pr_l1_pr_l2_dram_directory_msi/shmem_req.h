#pragma once

#include "shmem_msg.h"
#include "fixed_types.h"

namespace PrL1PrL2DramDirectoryMSI
{
   class ShmemReq
   {
      private:
         ShmemMsg* m_shmem_msg;
         UInt64 m_time;

      public:
         ShmemReq(ShmemMsg* shmem_msg, UInt64 time);
         ~ShmemReq();

         ShmemMsg* getShmemMsg() { return m_shmem_msg; }
         UInt64 getTime() { return m_time; }
         
         void setTime(UInt64 time) { m_time = time; }
         void updateTime(UInt64 time)
         {
            if (time > m_time)
               m_time = time;
         }
   };

}
