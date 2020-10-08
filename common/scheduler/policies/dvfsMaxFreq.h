/**
 * This header implements the max. freq DVFS policy
 */

#ifndef __DVFS_MAXFREQ_H
#define __DVFS_MAXFREQ_H

#include <vector>
#include "dvfspolicy.h"

class DVFSMaxFreq : public DVFSPolicy {
public:
    DVFSMaxFreq(const PerformanceCounters *performanceCounters, int coreRows, int coreColumns, int maxFrequency);
    virtual std::vector<int> getFrequencies(const std::vector<int> &oldFrequencies, const std::vector<bool> &activeCores);

private:
    const PerformanceCounters *performanceCounters;
    unsigned int coreRows;
    unsigned int coreColumns;
    int maxFrequency;
};

#endif