/**
 * This header implements the Next Tile migration policy
 */

#ifndef __MIG_SECURE_H
#define __MIG_SECURE_H

#include "migrationPolicy.h"


class MigSecure : public MigrationPolicy {
public:
    MigSecure(UInt32 maxTimeShared, UInt32 coresPerTile);
    core_id_t getMigrationCandidate(tile_id_t currentTile, const std::vector<bool> &availableCores, const std::vector<UInt32> sharedTimePerTile, 
    const std::vector<UInt32> activeThreadsPerTile); 
    core_id_t getMigrationCandidateNonSecure(tile_id_t currentTile, const std::vector<bool> &availableCores);
    tile_id_t getTileWLessThreads(tile_id_t secureThreadTile, std::vector<UInt32> activeThreadsPerTile);
    core_id_t getFreeCoreOnTile(tile_id_t tileId, const std::vector<bool> &availableCores);
    

private:
    UInt32 m_cores_per_tile, m_max_time_shared;
    
    tile_id_t getTileFreeORWLessTimeShared(tile_id_t callingTile, std::vector<UInt32> sharedTimePerTile, std::vector<UInt32> activeThreadsPerTile);
};

#endif
