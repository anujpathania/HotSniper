#include "thermalComponentModel.h"
#include <algorithm>
#include <sstream>

ThermalComponentModel::ThermalComponentModel(unsigned int coreRows, unsigned int coreColumns, unsigned int nodesPerCore, const String ThermalComponentModelFilename, const String FloorplanFilename, double ambientTemperature, double maxTemperature, double inactivePower, double tdp, const PerformanceCounters *performanceCounters)
    : performanceCounters(performanceCounters), ambientTemperature(ambientTemperature), maxTemperature(maxTemperature), inactivePower(inactivePower), tdp(tdp) {
    std::cout << "TCM INVOKED WHOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO!" << std::endl;
    std::cout << coreRows << ", " << coreColumns << ", " << ThermalComponentModelFilename << "," << FloorplanFilename << ", " << ambientTemperature << ", " << maxTemperature << ", " << inactivePower << ", " << tdp << std::endl;
    this->coreRows = coreRows;
    this->coreColumns = coreColumns;
    this->nodesPerCore = nodesPerCore;
    std::ifstream f;
    f.open(ThermalComponentModelFilename.c_str());


    unsigned int numberUnits = readValue<unsigned int>(f);

    unsigned int numberNodesAmbient = readValue<unsigned int>(f);
    unsigned int numberThermalNodes = readValue<unsigned int>(f);

    std::cout << "Number of units , corerows, corcols" << numberUnits << " " << nodesPerCore << " " << numberUnits / nodesPerCore << " " << coreRows << " " << coreColumns << std::endl;
    if (numberUnits / nodesPerCore != coreRows * coreColumns) {
        std::cout << "Fiale: " << std::endl;
        std::cout << "Assertion error in thermal model file: numberUnits != coreRows * coreColumns" << std::endl;
		exit (1);
    }
    std::cout << "Passed 1" << std::endl;
    if (numberThermalNodes != 4 * numberUnits + 12) {
        std::cout << "Assertion error in thermal model file: numberThermalNodes != 4 * numberUnits + 12" << std::endl;
		exit (1);
    }
    std::cout << "Passed 2" << std::endl;
    if (numberNodesAmbient != numberThermalNodes - 3 * numberUnits) {
        std::cout << "Assertion error in thermal model file: numberNodesAmbient != numberThermalNodes - 3 * numberUnits" << std::endl;
		exit (1);
    }
    std::cout << "Passed 3" << std::endl;
    for (unsigned int u = 0; u < numberUnits; u++) {
        std::string unitName = readLine(f);
        //width = readDouble(f);
        //height = readDouble(f);
    }
    numberOfThermalNodes = numberThermalNodes;
    numberofAmbientNodes = numberNodesAmbient;
    numberOfCoreNodes = coreRows * coreColumns * nodesPerCore;
    numberOfNonCoreNodes = 1;
    std::cout << "Passed 4" << std::endl;
    readDoubleMatrix(f, &BInv, numberThermalNodes, numberThermalNodes);
    std::cout << "Passed 5" << std::endl;
    readDoubleVector(f, &G, numberofAmbientNodes);
    // for (int i = 0; i < numberofAmbientNodes; i++) {
    //     std::cout << "G[" << i << "]: " << G[i] << std::endl; 
    // }
    std::cout << "Passed 6 " << numberUnits << std::endl;
    readComponentSizes(std::string(FloorplanFilename.c_str()), areas, numberUnits);

    std::cout << "Finished reading all files" << std::endl;
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
    std::cout << "AREAS SIZE: " << size << std::endl;
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
            std::cout << name << "," << width << "," << height <<  " < " << size << std::endl;
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

double ThermalComponentModel::tsp(const std::vector<bool> &activeCores) const {
    std::vector<double> powerOfInactiveCores(activeCores.size(), inactivePower);

    double res = 0.0;
    std::vector<double> tsps = tsp(activeCores, powerOfInactiveCores);
    // std::cout << " TSPS COUNT" << tsps.size() << std::endl;
    std::vector<double> tspPerCore(4);
    for (int i = 0; i < activeCores.size(); i++) {
        std::cout << "ACTIVE: " << i << ": " << activeCores.at(i) << std::endl;
        tspPerCore[i] = 0.0;
    }
    int coreSize = tsps.size() / activeCores.size();
    for (int i = 0; i < tsps.size(); i++) {
        // tsps[i] = tsps[i] / 2;
        // std::cout << "TSP Partial (" << i << "): " << tsps.at(i) << std::endl;
        res += tsps.at(i);
        // Sum
        tspPerCore[i / coreSize] += tsps.at(i);
        // Max
        // if (tspPerCore[i / coreSize] < tsps.at(i)) {
        //     tspPerCore[i / coreSize] = tsps.at(i);
        // }
        // Average
        // tspPerCore[i / coreSize] += tsps.at(i) / coreSize;
    }

    for (int i = 0; i < tspPerCore.size(); i++) {
        std::cout << "TPS (" << i << ") = " << tspPerCore[i] << std::endl;
    }

    double minTSP = tspPerCore[0];
    for (int i = 0; i < tspPerCore.size(); i++) {
        if (minTSP < tspPerCore[i]) {
            minTSP = tspPerCore[i];
        }
    }

    std::cout << "!!!!!!!!!!!!!!!! TSP VALUE: " << minTSP << std::endl;

    return minTSP;
}


std::vector<double> ThermalComponentModel::tsp(const std::vector<bool> &activeCores, const std::vector<double> &powerOfInactiveCores) const {
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
            std::cout << "Max temp: " << maxTemperature << ", amb T: " << ambientTemperature << ", inact: " << inactiveSum << ", act: " << activeSum << std::endl;
            std::cout <<" MINT TSP: " << minTSP << " CORE SAFE POWER" << coreSafePower << std::endl;
            minTSP = std::min(minTSP, coreSafePower);
            if (coreSafePower < minTSP) {
                std::cout << "old auxp = " << coreSafePower << std::endl;
            }
            std::cout <<" AFTER MIN" << minTSP << std::endl;
        }
    }
    
    std::vector<double> oldRes(1);
    oldRes.at(0) = minTSP;

    std::cout << "OLD RES: " << oldRes.at(0) << std::endl;

    // return oldRes;


    double tspValue = 0;
    int n_active_cores = 0;
    int n_active_components = 0;
    std::vector<bool> activeComponents(numberOfCoreNodes);
    // std::cout << "Active cores: ";
    for (unsigned int i = 0; i < activeCores.size(); i++) {
        // std::cout << activeCores.at(i);
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
    // std::cout << std::endl;
    double PWorstStar = __DBL_MAX__;
    std::cout << "WORST INIT: " << PWorstStar << std::endl;
    int coreIndexPWorstStar = -1;

    // TEMPORARY PBLOCKS code
    double* Pblocks = new double[numberOfThermalNodes];
    for (unsigned int j = 0; j < numberOfThermalNodes; j++) {
        Pblocks[j] = 0;
    }
    Pblocks[0] = performanceCounters->getPowerOfComponent("L3");
    std::cout << "L3 power: " << performanceCounters->getPowerOfComponent("L3") << std::endl;
    // REMOVE WHEN TESTING WITH non-core blocks and figure out how to get this.

    // for (unsigned int i = 0; i < 4; i++) {
    //     std::cout <<" P inact: " << powerOfInactiveCores.at(i) << std::endl;
    // }
    double coreArea = 0.0;
    for (unsigned int i = numberOfNonCoreNodes; i < numberOfNonCoreNodes + nodesPerCore; i++) {
        coreArea += areas[i];
    }
    std::cout <<" CORE AREA:" << coreArea << std::endl;
    std::cout << "Node counts: " <<  numberOfCoreNodes << " , " << numberOfNonCoreNodes << " , " << numberOfThermalNodes << ", n_amb: " << numberofAmbientNodes << std::endl;
    // std::cout << "ac size: " << activeComponents.size() << " < " << numberOfThermalNodes << " NON CORE: " << numberOfNonCoreNodes << std::endl;

    std::vector<double> powerOfInactiveComponents(numberOfCoreNodes);
    for (unsigned int i = 0; i < numberOfCoreNodes; i++) {
        powerOfInactiveComponents[i] = powerOfInactiveCores.at(i / nodesPerCore) * (areas[numberOfNonCoreNodes + i] / coreArea);
        // std::cout << "COMPO INACT: " << powerOfInactiveComponents[i] << "index: " << i / nodesPerCore << std::endl;
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
            // std::cout << "Getting active" << activeComponents.size() << std::endl;
            if (activeComponents.at(j)) {
                // std::cout << "Active comopnent contrib: " << BInv[i][j] * areas[j] << std::endl;
                // std::cout << "Active" << i << " < " << j << std::endl;
                if (BInv[i][j+numberOfNonCoreNodes] < 0) {
                    neg_act_contribs++;
                } else {
                    pos_act_contribs++;
                }
                heatContributionActiveCores += BInv[i][j+numberOfNonCoreNodes];// * areas[j+numberOfNonCoreNodes];
            } else {
                // std::cout << "InActive" << i << " < " << j << "NON_CORE: " << numberOfNonCoreNodes << std::endl;
                // std::cout << "INACT INDEX: " << powerOfInactiveComponents.at(j - numberOfNonCoreNodes) << std::endl;
                double b = BInv[i][j+numberOfNonCoreNodes];
                // std::cout << "b" << b << std::endl;
                double inact_comp = powerOfInactiveComponents.at(j);
                // std::cout << "c inact: " << inact_comp << std::endl;
                heatContributionInactiveCores += powerOfInactiveComponents.at(j) * (BInv[i][j+numberOfNonCoreNodes]);
            }
        }

        // std::cout << " Heat inact attrib: " << heatContributionActiveCores << std::endl;

        // Compute the heat contributed by the active blocks and ambient temperature
        double heatBlocksAndAmbient = 0;

        for(unsigned int j = 0; j < numberOfThermalNodes; j++){
            int g_offset = numberOfThermalNodes - numberofAmbientNodes;
            if (j < g_offset) {
                heatBlocksAndAmbient += BInv[i][j] * Pblocks[j];
            } else {
                heatBlocksAndAmbient += BInv[i][j] * ( Pblocks[j] + ambientTemperature * G[j - g_offset] );
            }
            // std::cout << "AMBIENT: " << heatBlocksAndAmbient << "G" << G[j] << std::endl;
        }
        // std::cout <<" TDM :" << tdp << std::endl;

        double auxP = maxTemperature - heatContributionInactiveCores;
        // std::cout << "AUX1: " << auxP << ", heat contrib inact: " << heatContributionInactiveCores << std::endl;
        auxP = auxP - heatBlocksAndAmbient;
        // std::cout << "AUX2: " << auxP << " heat contrib blocks and amb: " << heatBlocksAndAmbient << std::endl;
        auxP = auxP / heatContributionActiveCores;//* areas[i + numberOfNonCoreNodes];
        // std::cout << heatContributionActiveCores << std::endl;
        // std::cout << heatContributionActiveCores * 1000.0 << std::endl;
        // std::cout << "n pos: " << pos_act_contribs << ", n_neg: " << neg_act_contribs << std::endl; 
        // std::cout << "AUX3: " << auxP << " ROUND: "  << i << ", heatcontrib active: " << heatContributionActiveCores << " active comp count: " << activeCompCount << std::endl;
        if(auxP < PWorstStar){
            // std::cout << "Better: " << PWorstStar * coreArea << " > " << auxP * coreArea << std::endl;
            PWorstStar = auxP;
            coreIndexPWorstStar = i;
        }
        std::cout << "P WORST: " << PWorstStar << std::endl;
    }

    double pBlockSum = 0.0;
    
    for (unsigned int i = 0; i < (numberOfCoreNodes + numberOfNonCoreNodes); i++) {
        pBlockSum += Pblocks[i];
        if (Pblocks[i] > 0) {
            std::cout << "NONNULL BLOCK: " << i << "," << Pblocks[i] << std::endl;
        }
    }

    double pInactSum = 0.0;
    for (unsigned int i = 0; i < coreRows * coreColumns; i++) {
        if (!activeCores.at(i)) {
            pInactSum += powerOfInactiveCores.at(i);
        }
    }
    double areaSum = 0.0;
    std::cout << "n core nodes: " << numberOfCoreNodes << std::endl;
    for (unsigned int i = 0; i < numberOfCoreNodes; i++) {
        if (activeComponents.at(i)) {
            areaSum += areas[i + numberOfNonCoreNodes];
        }
    }

    std::cout << "Max tgemp: " << maxTemperature << "< tdm: " << tdp << std::endl;
    double maxTSP = (tdp - pBlockSum - pInactSum) / areaSum;
    // single core area: 0.000008.949
    std::cout << "MAXTSP:" << maxTSP * coreArea << " p worst" << PWorstStar << std::endl;
    if(PWorstStar <= maxTSP)
        tspValue = PWorstStar;
    else
        tspValue = maxTSP;

    std::vector<double> tspValues(numberOfCoreNodes);
    for (unsigned int i = 0; i < numberOfCoreNodes; i++) {
        // std::cout << "TSP: " << tspValue * areas[i + numberOfNonCoreNodes] << " area: " << areas[i + numberOfNonCoreNodes] << std::endl;
        tspValues.at(i) = tspValue;// * (areas[i + numberOfNonCoreNodes] / areas[coreIndexPWorstStar + numberOfNonCoreNodes]);
    }

    // return minTSP;
    // std::cout <<" NON CORE COUNT: " << numberOfNonCoreNodes << std::endl;
    std::cout << "OLD RES: " << oldRes.at(0) << std::endl;
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

    std::cout << "MIN TSP:" << minTSP << std::endl;
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
