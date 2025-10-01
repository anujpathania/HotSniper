/**
 * This header implements the test static power DVFS policy
 */

#ifndef __DVFS_SAFE_COMPONENT_POWER_H
#define __DVFS_SAFE_COMPONENT_POWER_H

#include <fstream>
#include <iostream>
#include <vector>
#include "dvfspolicy.h"
#include "fixed_types.h"
#include <sstream>

class DVFSSafeComponentPower : public DVFSPolicy {
public:
    DVFSSafeComponentPower(const PerformanceCounters *performanceCounters, int coreRows, int coreColumns, int minFrequency, int maxFrequency, double maxTemp, String floorplanFileName, String safeComponentPowerFile);
    virtual std::vector<int> getFrequencies(const std::vector<int> &oldFrequencies, const std::vector<bool> &activeCores);

private:
    const PerformanceCounters *performanceCounters;
    unsigned int coreRows;
    unsigned int coreColumns;
    int minFrequency;
    int maxFrequency;
    double maxTemp;
    std::vector<std::string> components;

    std::vector<std::string> readComponents(const std::string &floorplanFilename);
};

#endif
