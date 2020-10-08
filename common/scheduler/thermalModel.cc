#include "thermalModel.h"
#include <algorithm>
#include <sstream>

ThermalModel::ThermalModel(unsigned int coreRows, unsigned int coreColumns, const String thermalModelFilename, double ambientTemperature, double maxTemperature, double inactivePower, double tdp)
    : ambientTemperature(ambientTemperature), maxTemperature(maxTemperature), inactivePower(inactivePower), tdp(tdp) {
    this->coreRows = coreRows;
    this->coreColumns = coreColumns;

    std::ifstream f;
    f.open(thermalModelFilename.c_str());

    unsigned int numberUnits = readValue<unsigned int>(f);
    unsigned int numberNodesAmbient = readValue<unsigned int>(f);
    unsigned int numberThermalNodes = readValue<unsigned int>(f);

    if (numberUnits != coreRows * coreColumns) {
        std::cout << "Assertion error in thermal model file: numberUnits != coreRows * coreColumns" << std::endl;
		exit (1);
    }
    if (numberThermalNodes != 4 * numberUnits + 12) {
        std::cout << "Assertion error in thermal model file: numberThermalNodes != 4 * numberUnits + 12" << std::endl;
		exit (1);
    }
    if (numberNodesAmbient != numberThermalNodes - 3 * numberUnits) {
        std::cout << "Assertion error in thermal model file: numberNodesAmbient != numberThermalNodes - 3 * numberUnits" << std::endl;
		exit (1);
    }

    for (unsigned int u = 0; u < numberUnits; u++) {
        std::string unitName = readLine(f);
        //width = readDouble(f);
        //height = readDouble(f);
    }

    readDoubleMatrix(f, &BInv, numberThermalNodes, numberThermalNodes);

    // remaining file is not read
    f.close();
}

template<typename T>
T ThermalModel::readValue(std::ifstream &file) const {
    T value;
    file.read((char*)&value, sizeof(T));
    if(file.rdstate() != std::stringstream::goodbit){
        std::cout << "Assertion error in thermal model file: file ended too early." << std::endl;
        file.close();
        exit(1);
    }
    return value;
}

std::string ThermalModel::readLine(std::ifstream &file) const {
    std::string value;
    std::getline(file, value);
    if(file.rdstate() != std::stringstream::goodbit){
        std::cout << "Assertion error in thermal model file: file ended too early." << std::endl;
        file.close();
        exit(1);
    }
    return value;
}

void ThermalModel::readDoubleMatrix(std::ifstream &file, double ***matrix, unsigned int rows, unsigned int columns) const {
    (*matrix) = new double*[rows];
    for (unsigned int r = 0; r < rows; r++) {
        (*matrix)[r] = new double[columns];
        for (unsigned int c = 0; c < columns; c++) {
            (*matrix)[r][c] = readValue<double>(file);
        }
    }
}

double ThermalModel::tsp(const std::vector<bool> &activeCores) const {
    std::vector<double> powerOfInactiveCores(activeCores.size(), inactivePower);

    return tsp(activeCores, powerOfInactiveCores);
}

double ThermalModel::tsp(const std::vector<bool> &activeCores, const std::vector<double> &powerOfInactiveCores) const {
    if (activeCores.size() != coreRows * coreColumns) {
        std::cout << "\n[Scheduler][TSP][Error]: Invalid system size: " << activeCores.size() << ", expected " << (coreRows * coreColumns) << "cores." << std::endl;
		exit (1);
    }

    int amtActiveCores = 0;
    double idlePower = 0;
    for (unsigned int i = 0; i < activeCores.size(); i++) {
        if (activeCores.at(i)) {
            amtActiveCores++;
        } else {
            idlePower += powerOfInactiveCores.at(i);
        }
    }

    double minTSP = (tdp - idlePower) / amtActiveCores; // TDP constraint

    if (amtActiveCores > 0) {
        for (unsigned int core = 0; core < activeCores.size(); core++) {
            double activeSum = 0;
            double inactiveSum = 0;
            for (unsigned int i = 0; i < activeCores.size(); i++) {
                if (activeCores.at(i)) {
                    activeSum += BInv[core][i];
                } else {
                    inactiveSum += powerOfInactiveCores.at(i) * BInv[core][i];
                }
            }
            double coreSafePower = (maxTemperature - ambientTemperature - inactiveSum) / activeSum;
            minTSP = std::min(minTSP, coreSafePower);
        }
    }

    return minTSP;
}

std::vector<double> ThermalModel::tspForManyCandidates(const std::vector<bool> &activeCores, const std::vector<int> &candidates) const {
    if (activeCores.size() != coreRows * coreColumns) {
        std::cout << "\n[Scheduler][TSP][Error]: Invalid system size: " << activeCores.size() << ", expected " << (coreRows * coreColumns) << "cores." << std::endl;
		exit (1);
    }

    int amtActiveCores = count(activeCores.begin(), activeCores.end(), true) + 1; // start at one

    double idlePower = (coreRows * coreColumns - amtActiveCores) * inactivePower;
    double tdpConstraint = (tdp - idlePower) / amtActiveCores;
    std::vector<double> tsps(candidates.size(), tdpConstraint);

    if (amtActiveCores > 0) {
        for (unsigned int core = 0; core < activeCores.size(); core++) {
            double activeSum = 0;
            double inactiveSum = 0;
            for (unsigned int i = 0; i < activeCores.size(); i++) {
                if (activeCores.at(i)) {
                    activeSum += BInv[core][i];
                } else {
                    inactiveSum += BInv[core][i];
                }
            }

            for (unsigned int candidateIdx = 0; candidateIdx < candidates.size(); candidateIdx++) {
                int candidate = candidates.at(candidateIdx);
                double candActiveSum = activeSum + BInv[core][candidate];
                double candInactiveSum = inactiveSum - BInv[core][candidate];
                double coreSafePower = (maxTemperature - ambientTemperature - inactivePower * candInactiveSum) / candActiveSum;
                tsps.at(candidateIdx) = std::min(tsps.at(candidateIdx), coreSafePower);
            }
        }
    }

    return tsps;
}

double ThermalModel::worstCaseTSP(int amtActiveCores) const {
    double amtIdleCores = coreRows * coreColumns - amtActiveCores;
    double minTSP = (tdp - amtIdleCores * inactivePower) / amtActiveCores; // TDP constraint

    if (amtActiveCores > 0) {
        for (unsigned int core = 0; core < (unsigned int)(coreRows * coreColumns); core++) {
            std::vector<double> BInvRow(coreRows * coreColumns);
            for (unsigned int i = 0; i < (unsigned int)(coreRows * coreColumns); i++) {
                BInvRow.at(i) = BInv[core][i];
            }
            std::sort(BInvRow.begin(), BInvRow.end(), std::greater<double>()); // sort descending

            double activeSum = 0;
            double inactiveSum = 0;
            for (unsigned int i = 0; i < (unsigned int)(coreRows * coreColumns); i++) {
                if (i < (unsigned int)amtActiveCores) {
                    activeSum += BInvRow.at(i);
                } else {
                    inactiveSum += BInvRow.at(i) * inactivePower;
                }
            }

            double coreSafePower = (maxTemperature - ambientTemperature - inactiveSum) / activeSum;
            minTSP = std::min(minTSP, coreSafePower);
        }
    }

    return minTSP;
}

void inplaceGauss(std::vector<std::vector<float>> &A, std::vector<float> &b) {
    unsigned int n = b.size();
    for (unsigned int row = 0; row < n; row++) {
        // divide
        float pivot = A.at(row).at(row);
        b.at(row) /= pivot;
        for (unsigned int col = 0; col < n; col++) {
            A.at(row).at(col) /= pivot;
        }

        // add
        for (unsigned int row2 = 0; row2 < n; row2++) {
            if (row != row2) {
                float factor = A.at(row2).at(row);
                b.at(row2) -= factor * b.at(row);
                for (unsigned int col = 0; col < n; col++) {
                    A.at(row2).at(col) -= factor * A.at(row).at(col);
                }
            }
        }
    }
}

/** powerBudgetMaxSteadyState
 * Return a per-core power budget that (if matched by the power consumption) heats every core exactly to the critical temperature.
 */
std::vector<double> ThermalModel::powerBudgetMaxSteadyState(const std::vector<bool> &activeCores) const {
    std::vector<int> activeIndices;
    std::vector<double> inactivePowers(coreRows * coreColumns, 0);
    for (unsigned int i = 0; i < coreRows * coreColumns; i++) {
        if (activeCores.at(i)) {
            activeIndices.push_back(i);
        } else {
            inactivePowers.at(i) = inactivePower;
        }
    }
    std::vector<float> tInactive = getSteadyState(inactivePowers);
    std::vector<float> headroomTrunc(activeIndices.size());
    for (unsigned int i = 0; i < activeIndices.size(); i++) {
        int index = activeIndices.at(i);
        headroomTrunc.at(i) = maxTemperature - tInactive.at(index);
    }

    std::vector<float> powersTrunc = headroomTrunc;
    std::vector<std::vector<float>> BInvTrunc;
    for (unsigned int i = 0; i < activeIndices.size(); i++) {
        std::vector<float> row;
        for (unsigned int j = 0; j < activeIndices.size(); j++) {
            row.push_back(BInv[activeIndices.at(i)][activeIndices.at(j)]);
        }
        BInvTrunc.push_back(row);
    }
    // now solve BInvTrunc * powersTrunc = headroomTrunc
    inplaceGauss(BInvTrunc, powersTrunc);

    std::vector<double> powers(coreRows * coreColumns, inactivePower);
    for (unsigned int i = 0; i < activeIndices.size(); i++) {
        powers.at(activeIndices.at(i)) = powersTrunc.at(i);
    }

    return powers;
}

std::vector<float> ThermalModel::getSteadyState(const std::vector<double> &powers) const {
    std::vector<float> temperatures(coreRows * coreColumns);
    for (unsigned int core = 0; core < (unsigned int)(coreRows * coreColumns); core++) {
        temperatures.at(core) = ambientTemperature;
        for (unsigned int i = 0; i < (unsigned int)(coreRows * coreColumns); i++) {
            temperatures.at(core) += powers.at(i) * BInv[core][i];
        }
    }
    return temperatures;
}
