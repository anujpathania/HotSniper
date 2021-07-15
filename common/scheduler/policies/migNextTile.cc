#include "migNextTile.h"
#include "scheduler_open.h"



core_id_t MigNextTile::getMigrationCandidate(core_id_t currentCore, const std::vector<bool> &availableCores) 
{
	int cores = Sim()->getConfig()->getApplicationCores();
	for(core_id_t core_id = 0; core_id < (core_id_t)cores; ++core_id) {
			if (core_id >= currentCore && availableCores.at((core_id + 4) % cores))
				return (core_id_t)((core_id + 4) % cores);
		}
	return currentCore;
}