#include "migNextTile.h"
#include "scheduler_open.h"



core_id_t MigNextTile::getMigrationCandidate(tile_id_t currentTile, const std::vector<bool> &availableCores, const std::vector<UInt32> sharedTimePerTile, 
                                            const std::vector<UInt32> activeThreadsPerTile)
{
	core_id_t currentCore = -1;
	for(tile_id_t tile_id = 0; tile_id < sharedTimePerTile.size(); tile_id++) {
			if (tile_id != currentTile)
				for (core_id_t core_id  = 0; core_id < 4; core_id++){
					if (availableCores.at((tile_id*4  + core_id )))
						return (core_id_t)(tile_id * 4 + core_id );
				}
		}
	return currentCore;
}