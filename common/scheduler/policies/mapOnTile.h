/**
 * This header implements the "map to specific tile" policy
 */

#ifndef __MAP_ON_TILE_H
#define __MAP_ON_TILE_H

#include "mappingpolicy.h"

class MapOnTile: public MappingPolicy {
public:
    MapOnTile(unsigned int numTasks, unsigned int coreRows, unsigned int coreColumns, std::vector<int> preferredTile);
    virtual std::vector<int> map(UInt32 taskID, int taskCoreRequirement, const std::vector<bool> &availableCores, const std::vector<bool> &activeCores);

private:
    unsigned int m_core_rows;
    unsigned int m_core_columns;
    std::vector<int> m_preferred_tile;
    std::vector<std::vector<int>> m_preferred_cores_order;

};

#endif