/**
 * This header implements the DVFS policy wrt. the computed per-core power budgets using TTSP
 */

#ifndef __DVFS_TTSP_H
#define __DVFS_TTSP_H

#include <vector>
#include "dvfspolicy.h"

class DVFSTTSP : public DVFSPolicy {
public:
    DVFSTTSP(const PerformanceCounters *performanceCounters, int coreRows, int coreColumns, int minFrequency, int maxFrequency, int frequencyStepSize);
    virtual std::vector<int> getFrequencies(const std::vector<int> &oldFrequencies, const std::vector<bool> &activeCores);
    virtual std::vector<double> getPowerBudget() {return budgets;}
    virtual void setPowerBudget(const std::vector<double> &budgets) {this->budgets = budgets;}

private:
    const PerformanceCounters *performanceCounters;
    std::vector<double> budgets;
    unsigned int coreRows;
    unsigned int coreColumns;
    int minFrequency;
    int maxFrequency;
    int frequencyStepSize;
};

#endif
