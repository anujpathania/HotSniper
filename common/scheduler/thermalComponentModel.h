#ifndef __TSP_COMPONENT_H
#define __TSP_COMPONENT_H


#include <fstream>
#include <iostream>
#include <vector>
#include "fixed_types.h"
#include "performance_counters.h"

class ThermalComponentModel {
public:
    ThermalComponentModel(unsigned int coreRows, unsigned int coreColumns, unsigned int nodesPerCore, const String thermalModelFilename, const String floorplanFilename, double ambientTemperature, double maxTemperature, double inactivePower, double tdp, const PerformanceCounters *performanceCounters);

    std::vector<double> tsp(const std::vector<bool> &activeCores, const std::vector<double> &powerOfInactiveCores) const;
    double tsp(const std::vector<bool> &activeCores) const;
    std::vector<double> tspForManyCandidates(const std::vector<bool> &activeCores, const std::vector<int> &candidates) const;
    double worstCaseTSP(int amtActiveCores) const;
    std::vector<double> powerBudgetMaxSteadyState(const std::vector<bool> &activeCores) const;
    std::vector<float> getSteadyState(const std::vector<double> &powers) const;

    float getInactivePower() const { return inactivePower; }

private:
    const PerformanceCounters *performanceCounters;
    double ambientTemperature;
    double maxTemperature;
    double inactivePower;
    double tdp;
    template<typename T> T readValue(std::ifstream &file) const;
    std::string readLine(std::ifstream &file) const;
    void readDoubleMatrix(std::ifstream &file, double ***matrix, unsigned int rows, unsigned int columns) const;
    void readDoubleVector(std::ifstream &file, double **vector, unsigned int size) const;
    bool readComponentSizes(const std::string &floorplanFilename, double * &areas, int size) const;
    bool countNumberOfComponentsPerCore(const std::string &floorplanFilename, unsigned int &count);

    unsigned int coreRows;
    unsigned int coreColumns;
    unsigned int nodesPerCore;

    unsigned int numberOfThermalNodes;
    unsigned int numberofAmbientNodes;
    unsigned int numberOfCoreNodes;
    unsigned int numberOfNonCoreNodes;

    double **BInv;
    double *G;
    double *areas;
};

#endif
