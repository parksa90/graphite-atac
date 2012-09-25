#include "tile.h"
#include "tile_manager.h"
#include "perf_counter_manager.h"
#include "log.h"
#include "packet_type.h"
#include "simulator.h"
#include "network.h"

PerfCounterManager::PerfCounterManager(ThreadManager *thread_manager):
   m_thread_manager(thread_manager),
   m_counter(0),
   m_has_been_reset(false),
   m_has_been_disabled(false)
{
}

PerfCounterManager::~PerfCounterManager()
{}

void
PerfCounterManager::resetCacheCounters(SInt32 sender)
{
   LOG_ASSERT_WARNING(!m_has_been_reset, 
         "ResetCacheCounters called again, Sender(%i)", sender);
   UInt32 num_app_tiles = Sim()->getConfig()->getApplicationTiles();

   m_counter ++;
   if (m_counter == num_app_tiles)
   {
      m_has_been_reset = true;
      m_counter = 0;

      assert (m_thread_manager->areAllCoresRunning());
      
      Network* net = Sim()->getTileManager()->getCurrentTile()->getNetwork();

      // Send a message to all real tiles to reset cache counters
      for (UInt32 i = 0; i < num_app_tiles; i++)
      {
         UInt32 buff = 0;
         net->netSend(Tile::getMainCoreId(i), RESET_CACHE_COUNTERS, (const void*) &buff, sizeof(buff));
      }

      // Send a message to all worker threads to continue execution
      for (UInt32 i = 0; i < num_app_tiles; i++)
      {
         UInt32 buff = 0;
         net->netSend(Tile::getMainCoreId(i), MCP_RESPONSE_TYPE, (const void*) &buff, sizeof(buff));
      }

   }
}

void
PerfCounterManager::disableCacheCounters(SInt32 sender)
{
   LOG_ASSERT_WARNING(!m_has_been_disabled, 
         "DisableCacheCounters called again, Sender(%i)", sender);
   UInt32 num_app_tiles = Sim()->getConfig()->getApplicationTiles();

   m_counter ++;
   if (m_counter == num_app_tiles)
   {
      m_has_been_disabled = true;
      m_counter = 0;

      assert (m_thread_manager->areAllCoresRunning());
      
      Network* net = Sim()->getTileManager()->getCurrentTile()->getNetwork();

      // Send a message to all real tiles to reset cache counters
      for (UInt32 i = 0; i < num_app_tiles; i++)
      {
         UInt32 buff = 0;
         net->netSend(Tile::getMainCoreId(i), DISABLE_CACHE_COUNTERS, (const void*) &buff, sizeof(buff));
      }

      // Send a message to all worker threads to continue execution
      for (UInt32 i = 0; i < num_app_tiles; i++)
      {
         UInt32 buff = 0;
         net->netSend(Tile::getMainCoreId(i), MCP_RESPONSE_TYPE, (const void*) &buff, sizeof(buff));
      }

   }
}
