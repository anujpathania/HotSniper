#include "scheduler_open.h"
#include "migSecure.h"
#include <vector>
#include <iostream>

using namespace std;




MigSecure::MigSecure(UInt32 maxTimeShared, UInt32 coresPerTile)
{
    m_max_time_shared = maxTimeShared;
    m_cores_per_tile = coresPerTile;

}

core_id_t MigSecure::getMigrationCandidate(tile_id_t currentTile, const std::vector<bool> &availableCores, 
                                           const std::vector<UInt32> sharedTimePerTile, const std::vector<UInt32> activeThreadsPerTile) 
{
    //First try to find the tile with the less time shared
    tile_id_t destTile = getTileFreeORWLessTimeShared(currentTile, sharedTimePerTile, activeThreadsPerTile);
    //If we have found a tile different than our own
    if (destTile >= 0)
        // Try to get a core on that tile
        return  getFreeCoreOnTile(destTile, availableCores);
    //If we didn't find a valid tile, return invalid
    return -1;


}

core_id_t MigSecure::getMigrationCandidateNonSecure(tile_id_t currentTile, const std::vector<bool> &availableCores) {
    core_id_t candidate = -1;
    for (tile_id_t i = 0; i < (tile_id_t) (availableCores.size() / m_cores_per_tile); i++)
        if (i != currentTile) {
            candidate = getFreeCoreOnTile(i, availableCores);           
            if (candidate != -1)
                return candidate;
        }
    return candidate;
}

core_id_t MigSecure::getFreeCoreOnTile(tile_id_t tileId, const std::vector<bool> &availableCores){
	for(core_id_t core_id = 0; core_id < (core_id_t) m_cores_per_tile; ++core_id) 
        if (availableCores.at(tileId * m_cores_per_tile + core_id)) 
            return (core_id_t)(tileId * m_cores_per_tile + core_id);   
		
    return -1;
} 


tile_id_t MigSecure::getTileFreeORWLessTimeShared(tile_id_t callingTile, std::vector<UInt32> sharedTimePerTile, std::vector<UInt32> activeThreadsPerTile) {
    UInt32 min = sharedTimePerTile.at(callingTile);
    tile_id_t minIdx = -1;
    cout <<"Calling tile" <<callingTile<<" shared time: "<<sharedTimePerTile.at(callingTile)<<endl;
    for (unsigned int i = 0; i < sharedTimePerTile.size(); i++) {
        cout << "Tile: "<<i<< " Shared time: "<< sharedTimePerTile.at(i)<<endl;
        if (sharedTimePerTile.at(i) < min) {
            if (activeThreadsPerTile.at(i) == 0)
                return i;
            minIdx = i;
        }
    }
    // If the best candidate has more shared time than the max allowed, return Invalid tile
    if ((minIdx >= 0) && (sharedTimePerTile.at(minIdx) >= m_max_time_shared))
        minIdx = -1;
   
    return minIdx;
}

tile_id_t MigSecure::getTileWLessThreads(tile_id_t secureThreadTile, std::vector<UInt32> activeThreadsPerTile) {
    UInt32 min = activeThreadsPerTile.at(0);
    tile_id_t minIdx = 0;
    for (unsigned int i = 0; i < activeThreadsPerTile.size(); i++)
        if (activeThreadsPerTile.at(i) < min)
            minIdx = i;
    
    if ((activeThreadsPerTile.at(secureThreadTile) == minIdx) || 
        (activeThreadsPerTile.at(secureThreadTile) == activeThreadsPerTile.at(minIdx) + 1 )) 
            minIdx = secureThreadTile;

    return minIdx;
}

