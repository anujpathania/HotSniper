/**
 * This header implements the next free tile migration policy
 */

#ifndef __MIG_FREE_TILE_H
#define __MIG_FREE_TILE__H

#include "migrationPolicy.h"

class MigFreeTile : public MigrationPolicy {
public:
    virtual core_id_t getMigrationCandidate(core_id_t currentCore, const std::vector<bool> &availableCores, const std::vector<bool> &freeTiles);
private:
   
};

#endif
