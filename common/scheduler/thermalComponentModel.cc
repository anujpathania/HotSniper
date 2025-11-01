#include "thermalComponentModel.h"
#include <algorithm>
#include <sstream>

ThermalComponentModel::ThermalComponentModel(unsigned int coreRows, unsigned int coreColumns, unsigned int nodesPerCore, const String ThermalComponentModelFilename, const String FloorplanFilename, const String InactivePowerFilename, double ambientTemperature, double maxTemperature, double inactivePower, double tdp, const PerformanceCounters *performanceCounters)
    : performanceCounters(performanceCounters), ambientTemperature(ambientTemperature), maxTemperature(maxTemperature), inactivePower(inactivePower), tdp(tdp) {
    this->coreRows = coreRows;
    this->coreColumns = coreColumns;
    this->nodesPerCore = nodesPerCore;
    std::ifstream f;
    f.open(ThermalComponentModelFilename.c_str());


    unsigned int numberUnits = readValue<unsigned int>(f);

    unsigned int numberNodesAmbient = readValue<unsigned int>(f);
    unsigned int numberThermalNodes = readValue<unsigned int>(f);

    if (numberUnits / nodesPerCore != coreRows * coreColumns) {
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

    numberOfThermalNodes = numberThermalNodes;
    numberofAmbientNodes = numberNodesAmbient;
    numberOfCoreNodes = coreRows * coreColumns * nodesPerCore;
    numberOfNonCoreNodes = 1;

    readDoubleMatrix(f, &BInv, numberThermalNodes, numberThermalNodes);
    readDoubleVector(f, &G, numberofAmbientNodes);
    readComponentSizes(std::string(FloorplanFilename.c_str()), areas, numberUnits);
    readInactivePowers(std::string(InactivePowerFilename.c_str()), inactivePowers, numberOfCoreNodes);

    // remaining file is not read
    f.close();
}

template<typename T>
T ThermalComponentModel::readValue(std::ifstream &file) const {
    T value;
    file.read((char*)&value, sizeof(T));
    if(file.rdstate() != std::stringstream::goodbit){
        std::cout << "Assertion error in thermal model file: file ended too early." << std::endl;
        file.close();
        exit(1);
    }
    return value;
}

std::string ThermalComponentModel::readLine(std::ifstream &file) const {
    std::string value;
    std::getline(file, value);
    if(file.rdstate() != std::stringstream::goodbit){
        std::cout << "Assertion error in thermal model file: file ended too early." << std::endl;
        file.close();
        exit(1);
    }
    return value;
}

void ThermalComponentModel::readDoubleMatrix(std::ifstream &file, double ***matrix, unsigned int rows, unsigned int columns) const {
    (*matrix) = new double*[rows];
    for (unsigned int r = 0; r < rows; r++) {
        (*matrix)[r] = new double[columns];
        for (unsigned int c = 0; c < columns; c++) {
            (*matrix)[r][c] = readValue<double>(file);
        }
    }
}

void ThermalComponentModel::readDoubleVector(std::ifstream &file, double **vector, unsigned int size) const {
    *vector = new double[size];
    for (unsigned int r = 0; r < size; r++) {
        (*vector)[r] = readValue<double>(file);
    }
}

bool ThermalComponentModel::countNumberOfComponentsPerCore(const std::string &floorplanFilename, unsigned int &count) {
    if(floorplanFilename.size() <= 0) {
        std::cout << "Floorplan file name not valid: " << floorplanFilename << std::endl;
        return false;
    }

    std::ifstream inputFile(floorplanFilename.c_str());
	if (!inputFile.is_open() || !inputFile.good()) {
		return false;
	}

    unsigned int i = 0;
	while (inputFile.good()) {
		std::string line;
		getline(inputFile, line);

		// Skip comments
		if (line[0] == '#') {
			continue;
		}

		if (line.find("C_0") == 0) {
            i++;
        }
	}

    count = i;
    inputFile.close();

    return true;
}

bool ThermalComponentModel::readComponentSizes(const std::string &floorplanFilename, double * &areas, int size) const {
    if(floorplanFilename.size() <= 0) {
        return false;
    }

    areas = new double[size];
	for (unsigned int i = 0; i < size; i++) {
		areas[i] = 0;
	}


	std::ifstream inputFile(floorplanFilename.c_str());
	if (!inputFile.is_open() || !inputFile.good()) {
		return false;
	}

	int i = 0;
	while (inputFile.good()) {
		std::string line;
		getline(inputFile, line);

		// Skip comments
		if (line[0] == '#') {
			continue;
		}

		try {
			std::stringstream ss(line);
			ss.exceptions(std::stringstream::goodbit);

			double width, height;
			std::string name;
			ss >> name >> width >> height;
            if (name.empty()) {
                break;
            }
			areas[i] = width * height;
		} catch(...){
			std::cout << "Error: File with the floorplan is invalid." << std::endl;
			inputFile.close();
			return false;
		}

		i++;
	}

    inputFile.close();

	return true;
}

bool ThermalComponentModel::readInactivePowers(const std::string &inactivePowerFilename, double * &powers, unsigned int &node_count) const {
    if(inactivePowerFilename.size() <= 0) {
        return false;
    }

    powers = new double[node_count];
    for (int i = 0; i < node_count; i++) {
        powers[i] = 0.0;
    }

	std::ifstream inputFile(inactivePowerFilename.c_str());
	if (!inputFile.is_open() || !inputFile.good()) {
		return false;
	}

	int i = 0;
	while (inputFile.good()) {
		std::string line;
		getline(inputFile, line);

		// Skip comments
		if (line[0] == '#') {
			continue;
		}

		try {
			std::stringstream ss(line);
			ss.exceptions(std::stringstream::goodbit);

			double power;
			ss >> power;
			powers[i] = power;
		} catch(...){
			std::cout << "Error: File with the floorplan is invalid." << std::endl;
			inputFile.close();
			return false;
		}

		i++;
	}

    inputFile.close();

	return true;
}

double ThermalComponentModel::tsp(const std::vector<bool> &activeCores) const {
    std::vector<double> tsp_values = tsps(activeCores);

    std::vector<double> tspPerCore(4);
    for (int i = 0; i < activeCores.size(); i++) {
        tspPerCore[i] = 0.0;
    }
    int coreSize = tsp_values.size() / activeCores.size();
    for (int i = 0; i < tsp_values.size(); i++) {
        tspPerCore[i / coreSize] += tsp_values.at(i);
    }

    double minTSP = tspPerCore[0];
    for (int i = 0; i < tspPerCore.size(); i++) {
        if (minTSP < tspPerCore[i]) {
            minTSP = tspPerCore[i];
        }
    }

    return minTSP;
}


std::vector<double> ThermalComponentModel::tsps(const std::vector<bool> &activeCores) const {
    if (activeCores.size() != coreRows * coreColumns) {
        std::cout << "\n[Scheduler][TSP][Error]: Invalid system size: " << activeCores.size() << ", expected " << (coreRows * coreColumns) << "cores." << std::endl;
		exit (1);
    }

    double tspValue = 0;
    int n_active_cores = 0;
    int n_active_components = 0;
    std::vector<bool> activeComponents(numberOfCoreNodes);
    for (unsigned int i = 0; i < activeCores.size(); i++) {
        if (activeCores.at(i)) {
            n_active_cores++;
            n_active_components += nodesPerCore;
            for (unsigned int j = 0; j < nodesPerCore; j++) {
                activeComponents[i * nodesPerCore + j] = true;
            }
        } else {
            for (unsigned int j = 0; j < nodesPerCore; j++) {
                activeComponents[i * nodesPerCore + j] = false;
            }
        }
    }
    
    double PWorstStar = __DBL_MAX__;
    int coreIndexPWorstStar = -1;

    // TEMPORARY PBLOCKS code
    double* Pblocks = new double[numberOfThermalNodes];
    for (unsigned int j = 0; j < numberOfThermalNodes; j++) {
        Pblocks[j] = 0;
    }
    Pblocks[0] = performanceCounters->getPowerOfComponent("L3");


    double coreArea = 0.0;
    for (unsigned int i = numberOfNonCoreNodes; i < numberOfNonCoreNodes + nodesPerCore; i++) {
        coreArea += areas[i];
    }

    // Scientific variables from the paper:
    int L = numberOfCoreNodes + numberOfNonCoreNodes;
    int M = numberOfCoreNodes;
    int N = numberOfThermalNodes;

    for (unsigned int i = 0; i < L; i++) {
        double heatContributionInactiveCores = 0;
        double heatContributionActiveCores = 0;

        int activeCompCount = 0;
        for (unsigned int x = 0; x < activeComponents.size(); x++) {
            if (activeComponents.at(x)) {
                activeCompCount++;
            }
        }

        int neg_act_contribs = 0;
        int pos_act_contribs = 0;
        // Compute the heat contributed by the inactive cores and the active cores
        for(unsigned int j = 0; j < M; j++){
            if (activeComponents.at(j)) {
                std::cout << BInv[i][j+numberOfNonCoreNodes] << std::endl;
                heatContributionActiveCores += BInv[i][j+numberOfNonCoreNodes];// * areas[j+numberOfNonCoreNodes];
            } else {
                heatContributionInactiveCores += inactivePowers[j] * (BInv[i][j+numberOfNonCoreNodes]);
            }
        }

        // Compute the heat contributed by the active blocks and ambient temperature
        double heatBlocksAndAmbient = 0;

        for(unsigned int j = 0; j < numberOfThermalNodes; j++){
            int g_offset = numberOfThermalNodes - numberofAmbientNodes;
            if (j < g_offset) {
                heatBlocksAndAmbient += BInv[i][j] * Pblocks[j];
            } else {
                heatBlocksAndAmbient += BInv[i][j] * ( Pblocks[j] + ambientTemperature * G[j - g_offset] );
            }
        }

        double auxP = maxTemperature - heatContributionInactiveCores;
        std::cout << " AUXP1: " << auxP << std::endl;
        auxP = auxP - heatBlocksAndAmbient;
        std::cout << " AUXP2: " << auxP << std::endl;
        auxP = auxP / heatContributionActiveCores;//* areas[i + numberOfNonCoreNodes];
        std::cout << " AUXP3: " << auxP << std::endl;
        if(auxP < PWorstStar){
            PWorstStar = auxP;
            coreIndexPWorstStar = i;
        }
    }

    double pBlockSum = 0.0;
    
    for (unsigned int i = 0; i < (numberOfCoreNodes + numberOfNonCoreNodes); i++) {
        pBlockSum += Pblocks[i];
    }

    double pInactSum = 0.0;
    for (unsigned int i = 0; i < coreRows * coreColumns; i++) {
        if (!activeCores.at(i)) {
            for (int j = 0; j < nodesPerCore; j++) {
                pInactSum += inactivePowers[i+j];
            }
            // pInactSum += powerOfInactiveCores.at(i);
        }
    }
    double areaSum = 0.0;
    for (unsigned int i = 0; i < numberOfCoreNodes; i++) {
        if (activeComponents.at(i)) {
            areaSum += areas[i + numberOfNonCoreNodes];
        }
    }

    double maxTSP = (tdp - pBlockSum - pInactSum) / areaSum;
    if(PWorstStar <= maxTSP)
        tspValue = PWorstStar;
    else
        tspValue = maxTSP;

    std::vector<double> tspValues(numberOfCoreNodes);
    for (unsigned int i = 0; i < numberOfCoreNodes; i++) {
        tspValues.at(i) = tspValue;// (areas[i + numberOfNonCoreNodes] / areas[coreIndexPWorstStar + numberOfNonCoreNodes]);
    }

    return tspValues;
}

std::vector<double> ThermalComponentModel::tspForManyCandidates(const std::vector<bool> &activeCores, const std::vector<int> &candidates) const {
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

double ThermalComponentModel::worstCaseTSP(int amtActiveCores) const {
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
std::vector<double> ThermalComponentModel::powerBudgetMaxSteadyState(const std::vector<bool> &activeCores) const {
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

std::vector<float> ThermalComponentModel::getSteadyState(const std::vector<double> &powers) const {
    std::vector<float> temperatures(coreRows * coreColumns);
    for (unsigned int core = 0; core < (unsigned int)(coreRows * coreColumns); core++) {
        temperatures.at(core) = ambientTemperature;
        for (unsigned int i = 0; i < (unsigned int)(coreRows * coreColumns); i++) {
            temperatures.at(core) += powers.at(i) * BInv[core][i];
        }
    }
    return temperatures;
}
