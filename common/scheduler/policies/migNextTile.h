/**
 * This header implements the Next Tile migration policy
 */

#ifndef __MIG_NEXTTILE_H
#define __MIG_NEXTTILE_H

#include "migrationPolicy.h"

class MigNextTile : public MigrationPolicy {
public:
    virtual core_id_t getMigrationCandidate(core_id_t currentCore, const std::vector<bool> &availableCores);
private:
};

#endif
