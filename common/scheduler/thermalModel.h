#ifndef __TSP_H
#define __TSP_H


#include <fstream>
#include <iostream>
#include <vector>
#include "fixed_types.h"

class ThermalModel {
public:
    ThermalModel(unsigned int coreRows, unsigned int coreColumns, const String thermalModelFilename, double ambientTemperature, double maxTemperature, double inactivePower, double tdp, long pb_epoch_length, long pb_time_overhead);

    double tsp(const std::vector<bool> &activeCores, const std::vector<double> &powerOfInactiveCores) const;
    double tsp(const std::vector<bool> &activeCores) const;
    std::vector<double> tspForManyCandidates(const std::vector<bool> &activeCores, const std::vector<int> &candidates) const;
    double worstCaseTSP(int amtActiveCores) const;
    std::vector<double> powerBudgetMaxSteadyState(const std::vector<bool> &activeCores) const;
    std::vector<double> powerBudgetTTSP(const std::vector<bool> &activeCores) const;
    std::vector<float> getSteadyState(const std::vector<double> &powers) const;
    double peakTemperature(const std::vector<bool> &activeCores) const;

    float getInactivePower() const { return inactivePower; }

public:
    double ambientTemperature;
    double maxTemperature;
    double inactivePower;
    double tdp;
    template<typename T> T readValue(std::ifstream &file) const;
    std::string readLine(std::ifstream &file) const;
    void readDoubleMatrix(std::ifstream &file, double ***matrix, unsigned int rows, unsigned int columns) const;

    unsigned int coreRows;
    unsigned int coreColumns;

    unsigned int numberUnits;
    unsigned int numberNodesAmbient;
    unsigned int numberThermalNodes;

    long pb_epoch_length;
    long pb_time_overhead;
    double *Gamb;
    double *eigenValues;
    double *Tsteady;
    double *Tinit;

    double **eigenVectors;
    double **eigenVectorsInv;
    double **C;
    double *exponentials;
    double **matrix_exponentials;
    double **Inv_matrix_exponentials;
    double **HelpW;
    double **HelpIECT;
    double **BInv;
};

#endif
