/**
 * This header implements the "map to first unused core" policy
 */

#ifndef __MAP_FIRST_UNUSED_H
#define __MAP_FIRST_UNUSED_H

#include "mappingpolicy.h"

class MapFirstUnused : public MappingPolicy {
public:
    MapFirstUnused(unsigned int numTasks, unsigned int coreRows, unsigned int coreColumns);
    virtual std::vector<int> map(UInt32 taskID, int taskCoreRequirement, const std::vector<bool> &availableCores, const std::vector<bool> &activeCores);

private:
    unsigned int coreRows;
    unsigned int coreColumns;
    std::vector<int> preferredCoresOrder;
};

#endif