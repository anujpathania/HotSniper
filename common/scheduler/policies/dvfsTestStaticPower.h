/**
 * This header implements the test static power DVFS policy
 */

#ifndef __DVFS_TEST_STATIC_POWER_H
#define __DVFS_TEST_STATIC_POWER_H

#include <vector>
#include "dvfspolicy.h"

class DVFSTestStaticPower : public DVFSPolicy {
public:
    DVFSTestStaticPower(const PerformanceCounters *performanceCounters, int coreRows, int coreColumns, int minFrequency, int maxFrequency);
    virtual std::vector<int> getFrequencies(const std::vector<int> &oldFrequencies, const std::vector<bool> &activeCores);

private:
    const PerformanceCounters *performanceCounters;
    unsigned int coreRows;
    unsigned int coreColumns;
    int minFrequency;
    int maxFrequency;
};

#endif
