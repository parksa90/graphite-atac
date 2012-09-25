#include "l2_cache_cntlr.h"
#include "memory_manager.h"
#include "dram_directory_cntlr.h"
#include "log.h"

namespace ShL1ShL2
{

L2CacheCntlr::L2CacheCntlr(MemoryManager* memory_manager,
                           UInt32 cache_line_size,
                           UInt32 l2_cache_size,
                           UInt32 l2_cache_associativity,
                           string l2_cache_replacement_policy,
                           UInt32 l2_cache_access_delay,
                           bool l2_cache_track_miss_types,
                           float frequency)
   : _memory_manager(memory_manager)
{
   _l2_cache_replacement_policy_obj = 
      CacheReplacementPolicy::create(l2_cache_replacement_policy, l2_cache_size, l2_cache_associativity, cache_line_size);
   _l2_cache_hash_fn_obj = new CacheHashFn(l2_cache_size, l2_cache_associativity, cache_line_size);
   
   _l2_cache = new Cache("L2",
         SH_L1_SH_L2,
         Cache::UNIFIED_CACHE,
         L2,
         Cache::WRITE_BACK, 
         l2_cache_size, 
         l2_cache_associativity, 
         cache_line_size, 
         _l2_cache_replacement_policy_obj,
         _l2_cache_hash_fn_obj,
         l2_cache_access_delay,
         frequency,
         l2_cache_track_miss_types);
}

L2CacheCntlr::~L2CacheCntlr()
{
   delete _l2_cache;
   delete _l2_cache_replacement_policy_obj;
   delete _l2_cache_hash_fn_obj;
}

void
L2CacheCntlr::setCacheLineState(IntPtr address, CacheState::Type cstate)
{
   ShL2CacheLineInfo l2_cache_line_info;
   _l2_cache->getCacheLineInfo(address, &l2_cache_line_info);
   l2_cache_line_info.setCState(cstate);
   _l2_cache->setCacheLineInfo(address, &l2_cache_line_info);
}

void
L2CacheCntlr::readData(IntPtr address, UInt32 offset, UInt32 size, Byte* data_buf)
{
   _l2_cache->accessCacheLine(address + offset, Cache::LOAD, data_buf, size);
}

void
L2CacheCntlr::writeData(IntPtr address, UInt32 offset, UInt32 size, Byte* data_buf)
{
   _l2_cache->accessCacheLine(address + offset, Cache::STORE, data_buf, size);
}

void
L2CacheCntlr::insertCacheLine(IntPtr address, CacheState::Type cstate, Byte* fill_buf)
{
   // Construct meta-data info about l2 cache line
   ShL2CacheLineInfo l2_cache_line_info;
   l2_cache_line_info.setTag(_l2_cache->getTag(address));
   l2_cache_line_info.setCState(cstate);

   // Evicted line information
   bool eviction;
   IntPtr evicted_address;
   ShL2CacheLineInfo evicted_cache_line_info;
   Byte writeback_buf[getCacheLineSize()];

   _l2_cache->insertCacheLine(address, &l2_cache_line_info, fill_buf,
                              &eviction, &evicted_address, &evicted_cache_line_info, writeback_buf);

   if (eviction)
   {
      LOG_PRINT("Eviction: address(%#lx)", evicted_address);

      if (evicted_cache_line_info.getCState() == CacheState::MODIFIED)
         _dram_directory_cntlr->handleEvictedCacheLineFromL2Cache(evicted_address, writeback_buf);
      else if (evicted_cache_line_info.getCState() == CacheState::SHARED)
         _dram_directory_cntlr->handleEvictedCacheLineFromL2Cache(evicted_address, NULL);
      else
         LOG_PRINT_ERROR("Unrecognized State(%u)", evicted_cache_line_info.getCState());
   }
}

tile_id_t
L2CacheCntlr::getTileId()
{
   return _memory_manager->getTile()->getId();
}

UInt32
L2CacheCntlr::getCacheLineSize()
{ 
   return _memory_manager->getCacheLineSize();
}

ShmemPerfModel*
L2CacheCntlr::getShmemPerfModel()
{
   return _memory_manager->getShmemPerfModel();
}

}
