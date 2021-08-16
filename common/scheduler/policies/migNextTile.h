/**
 * This header implements the Next Tile migration policy
 */

#ifndef __MIG_NEXTTILE_H
#define __MIG_NEXTTILE_H

#include "migrationPolicy.h"

class MigNextTile : public MigrationPolicy {
public:
    core_id_t getMigrationCandidate(tile_id_t currentTile, const std::vector<bool> &availableCores, const std::vector<UInt32> sharedTimePerTile, 
                                            const std::vector<UInt32> activeThreadsPerTile);
    core_id_t getMigrationCandidateNonSecure(tile_id_t currentTile, const std::vector<bool> &availableCores) {return -1;}
    tile_id_t getTileWLessThreads(tile_id_t secureThreadTile, std::vector<UInt32> activeThreadsPerTile) {return -1;}
    core_id_t getFreeCoreOnTile(tile_id_t tileId, const std::vector<bool> &availableCores){ return -1;}

};

#endif
