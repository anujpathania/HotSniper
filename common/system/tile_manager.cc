#include "tile_manager.h"
#include <vector>
#include <iostream>


using namespace std;

TileManager::TileManager(UInt32 numberOfTiles, UInt32 coresPerTile) {
    m_number_of_tiles = numberOfTiles;
    m_cores_per_tile = coresPerTile;

    m_tiles = (Tile **)(malloc(numberOfTiles * sizeof(Tile *)));
    for (unsigned int i = 0; i < numberOfTiles; i++){
        m_tiles[i] = new Tile(i, coresPerTile);
    }
}
TileManager::~TileManager() {
    for(unsigned int i = 0; i <  m_number_of_tiles; i++)
        delete(m_tiles[i]);
}

// Tile* TileManager::getTileFromId(tile_id_t tileId){
//     return m_tiles[tileId];
// }

void TileManager::registerThreadOnTile(thread_id_t threadId, tile_id_t coreId) {
    tile_id_t tile = coreId / m_cores_per_tile;
    cout << "[TILE MANAGER]: Registering thread "<<threadId<< " on tile " <<tile<<endl;
    core_id_t coreOnTile = coreId % m_cores_per_tile;
    m_tiles[tile]->registerThread(threadId, coreOnTile);
}

void TileManager::unregisterThreadOnTile(thread_id_t threadId, tile_id_t coreId) {
    tile_id_t tile = coreId / m_cores_per_tile;
    cout << "[TILE MANAGER]: Unregistering thread "<<threadId<< " on tile " <<tile<<endl;
    core_id_t coreOnTile = coreId % m_cores_per_tile;
    m_tiles[tile]->unregisterThread(threadId, coreOnTile);
}

void TileManager::setThreadSharedTime(thread_id_t threadId, UInt32 sharedTime) {
    for (unsigned int i = 0; i < m_number_of_tiles; i++){
            core_id_t core = m_tiles[i]->findCoreFromThreadId(threadId);
            if (core != -1)
                m_tiles[i]->registerSharedTime(threadId, core, sharedTime);
        }   
    
}

UInt32 TileManager::getMaxSharedTimeOnTile(tile_id_t tileId){
    return m_tiles[tileId]->getMaxSharedTime();
}

UInt32 TileManager::getActiveThreadsOnTile(tile_id_t tileId){
    return m_tiles[tileId]->getActiveThreads();
}

void TileManager::printTileInfo(){
    cout<< "************Tile information*************"<<endl;
    for (unsigned int i = 0; i < m_number_of_tiles; i++){
        cout << "------------------------------------"<<endl;
        cout<<"TILE: "<<m_tiles[i]->getId()<<endl;
        cout << "Active threads :" <<getActiveThreadsOnTile(i)<<endl;
        for (unsigned int j = 0; j < m_cores_per_tile; j++){
            cout<< "Core " << j<< " Thread on Core :"<< m_tiles[i]->getThreadIdFromCore(j)<<endl;
        }
        
    }    
}
