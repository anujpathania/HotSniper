/**
 * This header implements the fixed power DVFS policy
 */

#ifndef __DVFS_FIXED_POWER_H
#define __DVFS_FIXED_POWER_H

#include <vector>
#include "dvfspolicy.h"

class DVFSFixedPower : public DVFSPolicy {
public:
    DVFSFixedPower(const PerformanceCounters *performanceCounters, int coreRows, int coreColumns, int minFrequency, int maxFrequency, int frequencyStepSize, float perCorePowerBudget);
    virtual std::vector<int> getFrequencies(const std::vector<int> &oldFrequencies, const std::vector<bool> &activeCores);
    virtual std::vector<double> getPowerBudget(){return std::vector<double>();}
    virtual void setPowerBudget(const std::vector<double> &budgets){}
private:
    const PerformanceCounters *performanceCounters;
    unsigned int coreRows;
    unsigned int coreColumns;
    int minFrequency;
    int maxFrequency;
    int frequencyStepSize;

    float perCorePowerBudget;
};

#endif
