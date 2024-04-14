/**
 * This header implements a policy that maps new applications to the coldest core
 * and migrates threads from hot cores to the coldest cores.
 */

#ifndef __PERIODICCORE_H
#define __PERIODICCORE_H

#include <vector>
#include "mappingpolicy.h"
#include "migrationpolicy.h"
#include "performance_counters.h"

class PeriodicCore : public MappingPolicy, public MigrationPolicy {
public:
    PeriodicCore(
        const PerformanceCounters *performanceCounters,
        int coreRows,
        int coreColumns,
        float criticalTemperature);
    virtual std::vector<int> map(
        String taskName,
        int taskCoreRequirement,
        const std::vector<bool> &availableCores,
        const std::vector<bool> &activeCores);
    virtual std::vector<migration> migrate(
        SubsecondTime time,
        const std::vector<int> &taskIds,
        const std::vector<bool> &activeCores);

private:
    const PerformanceCounters *performanceCounters;

    unsigned int coreRows;
    unsigned int coreColumns;
    float criticalTemperature;

    int getColdestCore(const std::vector<bool> &availableCores);
    int getPeriodicCore(const std::vector<bool> &activeCores);
    void logTemperatures(const std::vector<bool> &availableCores);
};

#endif