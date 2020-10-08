/**
 * This header implements the MappingPolicy interface.
 * A mapping policy is responsible for task mapping.
 */

#ifndef __MAPPINGPOLICY_H
#define __MAPPINGPOLICY_H

#include "fixed_types.h"
#include <vector>

class MappingPolicy {
public:
    virtual ~MappingPolicy() {}
    virtual std::vector<int> map(String taskName, int taskCoreRequirement, const std::vector<bool> &availableCores, const std::vector<bool> &activeCores) = 0;
};

#endif