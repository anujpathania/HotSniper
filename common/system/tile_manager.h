/**
 * This header implements the Tile  manager class 
 */

#ifndef _TILE_MANAGER_H
#define _TILE_MANAGER_H

#include "tile.h"
#include "fixed_types.h"
#include <vector>

using namespace std; 

class TileManager {
public:
    TileManager(UInt32 numberOfTiles, UInt32 coresPerTile);
    virtual ~TileManager();
    //Tile * getTileFromId(tile_id_t tileId);
    void printTileInfo();
    void registerThreadOnTile(thread_id_t threadId, core_id_t coreId);
    void unregisterThreadOnTile(thread_id_t threadId, core_id_t coreId);
    void setThreadSharedTime(thread_id_t threadId, UInt32 sharedTime);
    UInt32 getMaxSharedTimeOnTile(tile_id_t tileId);
    UInt32 getActiveThreadsOnTile(tile_id_t tileId);
    tile_id_t findTileFromThreadId(thread_id_t tileId);
    
private:
    UInt32 m_number_of_tiles;
    UInt32 m_cores_per_tile;
    Tile ** m_tiles;
};

#endif
