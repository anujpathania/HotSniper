/**
 * This header implements the TSP power DVFS policy
 */

 #ifndef __DVFS_TSP_H
 #define __DVFS_TSP_H
 
 #include <vector>
 #include "dvfspolicy.h"
 #include "thermalComponentModel.h"
 
 class DVFSTSP : public DVFSPolicy {
 public:
     DVFSTSP(ThermalComponentModel* thermalModel, const PerformanceCounters *performanceCounters, int coreRows, int coreColumns, int minFrequency, int maxFrequency, int frequencyStepSize);
     virtual std::vector<int> getFrequencies(const std::vector<int> &oldFrequencies, const std::vector<bool> &activeCores);
 
 private:
     ThermalComponentModel* thermalModel;
     const PerformanceCounters *performanceCounters;
     unsigned int coreRows;
     unsigned int coreColumns;
     int minFrequency;
     int maxFrequency;
     int frequencyStepSize;
 };
 
 #endif
 