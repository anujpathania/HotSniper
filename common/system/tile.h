/**
 * This header implements the Tile class 
 */

#ifndef _TILE_H
#define _TILE_H

#include "fixed_types.h"
#include <vector>

using namespace std; 

class Tile {
public:
    Tile(tile_id_t tileId, UInt32 coresPerTile);
    virtual ~Tile() {}

    UInt32 getActiveThreads();
    UInt32 getMaxSharedTime();
    UInt32 getId() { return m_id; }
    
    void registerThread(thread_id_t threadId , core_id_t coreId);
    void registerSharedTime(thread_id_t threadId, core_id_t coreId, UInt32 sharedTime);
    void unregisterThread(thread_id_t threadId , core_id_t coreId);
    core_id_t findCoreFromThreadId(thread_id_t threadId);
    thread_id_t getThreadIdFromCore(core_id_t core);
    
private:
   tile_id_t m_id;
   UInt32 m_max_time_shared;

   std::vector<bool> m_active_threads;
   std::vector<UInt32> m_time_shared;

   std::vector<core_id_t> m_cores;
   
};

#endif
