/**
 * This header implements a policy that maps new applications to the coldest core
 * and migrates threads from hot cores to the coldest cores.
 */

#ifndef __TASKREASSIGN_H
#define __TASKREASSIGN_H

#include <vector>
#include <set>
#include "mappingpolicy.h"
#include "thermalModel.h"
#include "migrationpolicy.h"
#include "performance_counters.h"

class TaskReassign : public MappingPolicy, public MigrationPolicy {
public:
    TaskReassign(
        ThermalModel *thermalModel,
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
    ThermalModel *thermalModel;
    const PerformanceCounters *performanceCounters;
    unsigned int coreRows;
    unsigned int coreColumns;
    float criticalTemperature;
    unsigned int numberUnits;
    unsigned int numberThermalNodes;
    double ambientTemperature;
    std::vector<float> amds;
    std::set<float> uniqueAMDs;
    //double maxTemperature;
    //double inactivePower;
    //double tdp;

    double pb_epoch_length;
    double** eigenVectors;
    double* eigenValues;
    double** eigenVectorsInv;
    double** HelpW;
    double* nexponentials;
    double** eachItem;
    double** eachComp;
    double* Item;

    int getColdestCore(const std::vector<bool> &availableCores);
    int getPeriodicCore(const std::vector<bool> &activeCores);
    void logTemperatures(const std::vector<bool> &availableCores);
    double* matrixAdd(double* A, double* B);
    double* componentCal(int para,int nActive,double *p);
    double collectComp();
    double concentric(const std::vector<bool> &activeCores);
    double* componentCalActive(int para, int nActive, double* p,const std::vector<bool> &activeCores);
    std::vector<std::tuple<float, float, std::vector<int>>> getMappingCandidates(int taskCoreRequirement, const std::vector<bool> &availableCores, const std::vector<bool> &activeCores);
    int manhattanDistance(int y1, int x1, int y2, int x2);
	float getCoreAMD(int coreY, int coreX);
};

#endif