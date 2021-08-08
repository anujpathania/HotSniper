#include "tile.h"
#include <vector>


using namespace std;

Tile::Tile(tile_id_t tileId, UInt32 coresPerTile) {
    m_id = tileId;
    m_cores.resize(coresPerTile, -1);
    m_time_shared.resize(coresPerTile, 0);
    m_active_threads.resize(coresPerTile, false);
}

UInt32 Tile::getActiveThreads() {
    UInt32 threads = 0;
    for(unsigned int i = 0; i < m_cores.size(); i++ ){
        if (m_cores.at(i) != -1)
            threads++;
    }
    return threads;
}

void Tile::registerThread(thread_id_t threadId , core_id_t coreId) {
    m_cores.at(coreId) = threadId;
    m_active_threads.at(coreId) = true;
}

void Tile::unregisterThread(thread_id_t threadId , core_id_t coreId) {
    m_cores.at(coreId) = -1;
    m_active_threads.at(coreId) = false;
    m_time_shared.at(coreId) = 0;
}

void Tile::registerSharedTime(thread_id_t threadId, core_id_t coreId, UInt32 sharedTime) {
    m_time_shared.at(coreId) = sharedTime;
}

UInt32 Tile::getMaxSharedTime() {
    UInt32 tmax = 0;
    for(unsigned int i = 0; i < m_time_shared.size() ; i++) {
        if (m_time_shared.at(i) > tmax)
            tmax = m_time_shared.at(i);
    }
    return tmax;
}


thread_id_t Tile::getThreadIdFromCore(core_id_t core) {
    return m_cores.at(core);
}

core_id_t Tile::findCoreFromThreadId(thread_id_t threadId) {
    for (size_t i = 0; i < m_cores.size(); i++){
        if (m_cores.at(i) == threadId)
            return i;
    }
    return -1;
    
}

