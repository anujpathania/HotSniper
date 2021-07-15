

	
#ifndef __MIGRATIONPOLICY_H
#define __MIGRATIONPOLICY_H

#include "fixed_types.h"
#include <vector>

class MigrationPolicy {
public:
    virtual ~MigrationPolicy() {}
    virtual core_id_t getMigrationCandidate(core_id_t currentCore, const std::vector<bool> &availableCores) = 0;
};

#endif