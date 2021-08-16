#include "migFreeTile.h"
#include "scheduler_open.h"




core_id_t MigFreeTile::getMigrationCandidate(core_id_t currentCore, const std::vector<bool> &availableCores, const std::vector<bool> &freeTiles) 
{
    int coresPerTile = Sim()->getConfig()->getCoresPerTile();
    for (unsigned tile = 0; tile < freeTiles.size(); tile++) {
        if(freeTiles.at(tile))  
            for (int core = 0; core < coresPerTile; core++) 
                if (availableCores.at(tile*coresPerTile + core)) 
                    return (core_id_t)(tile*coresPerTile + core);                            
    }
    return currentCore;
}