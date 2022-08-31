/**
 * This is an implemention of the PCGov policy published in
 *   Martin Rapp, Mark Sagi, Anuj Pathania, Andreas Herkersdorf, Jörg Henkel.
 *   Power- and Cache-Aware Task Mapping with Dynamic Power Budgeting for Many-Cores.
 *   in IEEE Transactions on Computers (TC) (Vol. 69, Issue 1), Jan 2020.
 *   https://doi.org/10.1109/TC.2019.2935446
 *
 * Notice that this policy is intended to work well with S-NUCA many-core architectures.
 * See for instance kingscross-nuca.cfg.
 */

#ifndef __PCGOV_H
#define __PCGOV_H

#include <set>
#include <vector>
#include "thermalModel.h"
#include "mappingpolicy.h"
#include "dvfspolicy.h"

class PCGov : public MappingPolicy, public DVFSPolicy {
public:
    PCGov(ThermalModel *thermalModel, PerformanceCounters *performanceCounters, int coreRows, int coreColumns, int minFrequency, int maxFrequency, int frequencyStepSize, float delta);
    virtual std::vector<int> map(String taskName, int taskCoreRequirement, const std::vector<bool> &availableCores, const std::vector<bool> &activeCores);
    virtual std::vector<int> getFrequencies(const std::vector<int> &oldFrequencies, const std::vector<bool> &activeCores);

private:
    enum ThreadState { IDLE, COMPUTE, MEMORY };

    unsigned int coreRows;
    unsigned int coreColumns;
    ThermalModel *thermalModel;
    PerformanceCounters *performanceCounters;
    int minFrequency;
    int maxFrequency;
    int frequencyStepSize;
    float delta;
    std::vector<float> amds;
    std::set<float> uniqueAMDs;
    std::vector<ThreadState> threadStates;
    std::vector<double> powerBudgets;

    void updatePowerBudgets(const std::vector<int> &oldFrequencies);

    std::vector<std::tuple<float, float, std::vector<int>>> getMappingCandidates(int taskCoreRequirement, const std::vector<bool> &availableCores, const std::vector<bool> &activeCores);
    int manhattanDistance(int y1, int x1, int y2, int x2);
	float getCoreAMD(int coreY, int coreX);
};

#endif
