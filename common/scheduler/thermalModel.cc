#include "thermalModel.h"
#include <algorithm>
#include <sstream>
#include <fstream>
#include <iomanip>
using namespace std;

ThermalModel::ThermalModel(unsigned int coreRows, unsigned int coreColumns, const String thermalModelFilename, double ambientTemperature, double maxTemperature, double inactivePower, double tdp, long pb_epoch_length, long pb_time_overhead)
    : ambientTemperature(ambientTemperature), maxTemperature(maxTemperature), inactivePower(inactivePower), tdp(tdp), pb_epoch_length(pb_epoch_length), pb_time_overhead(pb_time_overhead) {
    this->coreRows = coreRows;
    this->coreColumns = coreColumns;

    std::ifstream f;
    f.open(thermalModelFilename.c_str());

    numberUnits = readValue<unsigned int>(f);
    numberNodesAmbient = readValue<unsigned int>(f);
    numberThermalNodes = readValue<unsigned int>(f);

    if (numberUnits != coreRows * coreColumns) {
        // std::cout << "numberUnits is " << numberUnits << std::endl;
        // std::cout << "coreRows is " << coreRows << std::endl;
        std::cout << "coreColumns is " << coreColumns << std::endl;
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

    Gamb = new double[numberNodesAmbient];
    for (int c = 0; c < numberNodesAmbient; c++) {
            Gamb[c] = readValue<double>(f);
    }
   
    eigenValues = new double[numberThermalNodes];
    for (int c = 0; c < numberThermalNodes; c++) {
        eigenValues[c] = readValue<double>(f);
    }
    
    
    readDoubleMatrix(f, &eigenVectors, numberThermalNodes, numberThermalNodes);
    readDoubleMatrix(f, &eigenVectorsInv, numberThermalNodes, numberThermalNodes);

    matrix_exponentials = new double*[numberThermalNodes];
    Inv_matrix_exponentials = new double*[numberThermalNodes];
    HelpIECT = new double*[numberThermalNodes];
    HelpW = new double*[numberThermalNodes];

    for(int i = 0; i < numberThermalNodes; i++){
        matrix_exponentials[i] = new double[numberThermalNodes];
        Inv_matrix_exponentials[i] = new double[numberThermalNodes];
        HelpIECT[i] = new double[numberThermalNodes];
        HelpW[i] = new double[numberThermalNodes];

    }   
    Tsteady = new double[numberThermalNodes];
    Tinit = new double[numberThermalNodes]; 
    exponentials = new double[numberThermalNodes];

    for(int i = 0; i < numberThermalNodes; i++){
        exponentials[i] = pow((double)M_E, eigenValues[i] * pb_epoch_length * pow(10,-9));
    }
    for(int k = 0; k < numberThermalNodes; k++){
        for(int j = 0; j < numberThermalNodes; j++){
            matrix_exponentials[k][j] = 0;
            Inv_matrix_exponentials[k][j] = 0;
            HelpIECT[k][j] = 0;
            for(int i = 0; i < numberThermalNodes; i++){
                matrix_exponentials[k][j] += eigenVectors[k][i]*eigenVectorsInv[i][j]*exponentials[i];
                HelpIECT[k][j] += eigenVectors[k][i]*eigenVectorsInv[i][j]* (1 - exponentials[i]);
                
            }
            Inv_matrix_exponentials[k][j] = (k==j) ? matrix_exponentials[k][j]-1.0: matrix_exponentials[k][j];
        }
        for(int j = 0; j < numberThermalNodes; j++){
            HelpW[k][j] = 0;
            for(int i = 0; i < numberThermalNodes; i++){
                HelpW[k][j] += HelpIECT[k][i] * BInv[i][j];
            }
        }
    }

    // for(int k = 0; k < numberThermalNodes; k++){
    //     for(int j = 0; j < numberThermalNodes; j++){
    //         HelpW[k][j] = 0;
    //         for(int i = 0; i < numberThermalNodes; i++){
    //             HelpW[k][j] += HelpIECT[k][i] * BInv[i][j];
    //         }

    //     }
    // }

    


    // std::cout << "*******The eigen matrix is  *******" << endl;
    // for(int row = 0; row < numberUnits;row++){
    //     std::cout << '[';
    //     int column = 0;
    //     for(column = 0; column < numberUnits - 1;column++){
    //         std::cout << setiosflags(std::ios::fixed)<< std::setprecision(3);
    //         std::cout << eigenVectors[row][column] << ",";
    //     }
    //     std::cout << eigenVectors[row][column];
    //     std::cout << ']';
    //     if (row != numberUnits - 1) std::cout << ',';
    //     std::cout << std::endl;
        
    // }
    // std::cout << "*******The eigenVectorInv reverse matrix is  *******" << endl;

    // for(int row = 0; row < numberUnits;row++){
    //     std::cout << '[';
    //     int column = 0;
    //     for(column = 0; column < numberUnits - 1;column++){
    //         std::cout << setiosflags(std::ios::fixed)<< std::setprecision(3);
    //         std::cout << eigenVectorsInv[row][column] << ",";
    //     }
    //     std::cout << eigenVectorsInv[row][column];
    //     std::cout << ']';
    //     if (row != numberUnits - 1) std::cout << ',';
    //     std::cout << std::endl;
        
    // }

    // std::cout << "*******The C matrix is  *******" << endl;
    // readDoubleMatrix(f, &C, numberThermalNodes, numberThermalNodes);
    // for(int row = 0; row < numberUnits;row++){
    //     std::cout << '[';
    //     int column = 0;
    //     for(column = 0; column < numberUnits - 1;column++){
    //         std::cout << setiosflags(std::ios::fixed)<< std::setprecision(3);
    //         std::cout << C[row][column] << ",";
    //     }
    //     std::cout << C[row][column];
    //     std::cout << ']';
    //     if (row != numberUnits - 1) std::cout << ',';
    //     std::cout << std::endl;
        
    // }

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


// double ThermalModel::peakTemperature(const std::vector<bool> &activeCores) const{



// }

/** powerBudgetTTSP
 * Return a per-core power budget wrt. the location and transient temperature of the cores that (if matched by the power consumption) heats every core exactly to the critical temperature.
 */
std::vector<double> ThermalModel::powerBudgetTTSP(const std::vector<bool> &activeCores) const {
    
    std::vector<int> activeIndices;
    std::vector<double> inactivePowers(coreRows * coreColumns, 0);
    std::vector<double> powers(coreRows * coreColumns, inactivePower);

    for (int i = 0; i < coreRows * coreColumns; i++) {
        if (activeCores.at(i)) {
            activeIndices.push_back(i);
        } else {
            inactivePowers.at(i) = inactivePower;
        }
    } 

    //1.Reading the transient temperatures 
    std::string instTemperatureFileName="Temperature.init";
	ifstream temperatureLogFile(instTemperatureFileName);
	std::string header;
    double temp;
 
    int component = 0;
    double max_temp = 0;
    //std::cout << "The max_temp *** is " << max_temp << std::endl;
    while(temperatureLogFile >> header >> temp)
    {
        Tinit[component] = temp - 273.15;
        max_temp = (max_temp > Tinit[component]) ? max_temp:Tinit[component];
        //std::cout << "The max_temp *** is " << max_temp << std::endl;
        component++;
    }
    temperatureLogFile.close();
    
    //2. Computing the targeted steady-state termperatures
    std::vector<float> A(activeIndices.size());
    for (unsigned int i = 0; i < activeIndices.size(); i++) {
        A.at(i) = 0;
        for (unsigned int j = 0; j < activeIndices.size(); j++) {
            A.at(i) += matrix_exponentials[activeIndices.at(i)][activeIndices.at(j)]*Tinit[activeIndices.at(j)];
        }
        A.at(i) -= maxTemperature;
    }
    std::vector<float> T_steady = A;
    
        
    std::vector<std::vector<float>> B;
    for (unsigned int i = 0; i < activeIndices.size(); i++) {
        std::vector<float> row;
        for (unsigned int j = 0; j < activeIndices.size(); j++) {
            row.push_back(Inv_matrix_exponentials[activeIndices.at(i)][activeIndices.at(j)]);
        }
        B.push_back(row);
    }

    // now solve B * Tsteady = A
    inplaceGauss(B, T_steady);
    
    for(int k = 0; k < activeIndices.size(); k++)
    {
        cout<<"[ThermalModel][TTSP]: The initial termperature and targeted steady-state termperature  of core "<< activeIndices.at(k) <<" for the next epoch: "<< Tinit[activeIndices.at(k)] <<", "<<T_steady.at(k) <<endl; 
    }

    //3. Computing the corresponding power budget to the computed steady-state termperatures
    std::vector<float> tInactive = getSteadyState(inactivePowers);
    std::vector<float> ambient(activeIndices.size());
    std::vector<float> headroomTrunc(activeIndices.size());

   
    for (unsigned int i = 0; i < activeIndices.size(); i++) {
        int index = activeIndices.at(i);
        headroomTrunc.at(i) = T_steady.at(i) - tInactive.at(index);
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

    for (unsigned int i = 0; i < activeIndices.size(); i++) {
         powers.at(activeIndices.at(i)) = powersTrunc.at(i);
    }

    // for(auto i: powers){
    //     std::cout << "The powers is " << i << std::endl;
    // }

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
