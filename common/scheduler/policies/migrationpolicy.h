/**
 * This header implements the MigrationPolicy interface.
 * A migration policy is responsible for task migration.
 */

#ifndef __MIGRATIONPOLICY_H
#define __MIGRATIONPOLICY_H

#include "fixed_types.h"
#include <vector>
#include "subsecond_time.h"

struct migration {
    unsigned int fromCore;
    unsigned int toCore;
    bool swap;
};

class MigrationPolicy {
public:
    virtual ~MigrationPolicy() {}
    virtual std::vector<migration> migrate(SubsecondTime time, const std::vector<int> &taskIds, const std::vector<bool> &activeCores) = 0;
};

#endif
