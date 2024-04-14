#include "taskReassign.h"
#include <algorithm>
#include <iomanip>

using namespace std;

TaskReassign::TaskReassign(
        ThermalModel *thermalModel,
        const PerformanceCounters *performanceCounters,
        int coreRows,
        int coreColumns,
        float criticalTemperature)
    : thermalModel(thermalModel),
      performanceCounters(performanceCounters),
      coreRows(coreRows),
      coreColumns(coreColumns),
      criticalTemperature(criticalTemperature) {
    pb_epoch_length = (thermalModel->pb_epoch_length) * pow(10,-6) ;
    numberUnits = thermalModel->numberUnits;
    numberThermalNodes = thermalModel->numberThermalNodes;
    eigenVectors = thermalModel->eigenVectors;
    eigenValues = thermalModel->eigenValues;
    eigenVectorsInv = thermalModel->eigenVectorsInv;
    HelpW = thermalModel->HelpW;
    ambientTemperature = thermalModel->ambientTemperature;

    // get core AMD info
    for (int y = 0; y < coreColumns; y++) {
		for (int x = 0; x < coreRows; x++) {
			float amd = getCoreAMD(y, x);
			amds.push_back(amd);
			uniqueAMDs.insert(amd);
		}
	}
    //std::cout << "HelpW[0]  " << HelpW[0][0]<< endl;

}


int TaskReassign::manhattanDistance(int y1, int x1, int y2, int x2) {
	int dy = y1 - y2;
	int dx = x1 - x2;
	return abs(dy) + abs(dx);
}

float TaskReassign::getCoreAMD(int coreY, int coreX) {
	int md_sum = 0;
	for (unsigned int y = 0; y < coreColumns; y++) {
		for (unsigned int x = 0; x < coreRows; x++) {
			md_sum += manhattanDistance(coreY, coreX, y, x);
		}
	}
	return (float)md_sum / coreColumns / coreRows;
}

/** getMappingCandidates
 * Get all near-Pareto-optimal mappings considered in TaskReassign.
 * Return a vector of tuples (max. AMD, power budget, used cores)
 */
vector<tuple<float, float, vector<int>>> TaskReassign::getMappingCandidates(int taskCoreRequirement, const vector<bool> &availableCores, const vector<bool> &activeCores) {
	// get the candidates
	vector<tuple<float, float, vector<int>>> candidates;
	for (const float &amdMax : uniqueAMDs) {
		// get the candidate for the given AMD_max

		vector<int> availableCoresAMD;
		for (unsigned int i = 0; i < coreRows * coreColumns; i++) {
			if (availableCores.at(i) && (amds.at(i) <= amdMax)) {
				availableCoresAMD.push_back (i);
			}
		}

		if ((int)availableCoresAMD.size() >= taskCoreRequirement) {
			vector<int> selectedCores;

			vector<bool> activeCoresCandidate = activeCores;

			float mappingTSP = 0;
			float maxUsedAMD = 0;
			while ((int)selectedCores.size() < taskCoreRequirement) {
				// greedily select one core
#if true
				float bestTSP = 0;
				int bestIndex = 0;
				for (unsigned int i = 0; i < availableCoresAMD.size(); i++) {
					// try each core
					activeCoresCandidate[availableCoresAMD.at(i)] = true;
					float thisTSP = thermalModel->tsp(activeCoresCandidate);
					activeCoresCandidate[availableCoresAMD.at(i)] = false;

					if (thisTSP > bestTSP) {
						bestTSP = thisTSP;
						bestIndex = i;
					}
				}
#else
				vector<double> tsps = thermalModel->tspForManyCandidates(activeCoresCandidate, availableCoresAMD);
				float bestTSP = 0;
				int bestIndex = 0;
				for (unsigned int i = 0; i < tsps.size(); i++) {
					if (tsps.at(i) > bestTSP) {
						bestTSP = tsps.at(i);
						bestIndex = i;
					}
				}
#endif

				activeCoresCandidate[availableCoresAMD.at(bestIndex)] = true;
				selectedCores.push_back(availableCoresAMD.at(bestIndex));

				mappingTSP = bestTSP;
				maxUsedAMD = max(maxUsedAMD, amds.at(availableCoresAMD.at(bestIndex)));

				availableCoresAMD.erase(availableCoresAMD.begin() + bestIndex);
			}

			if (maxUsedAMD == amdMax) {
				// add the mapping to the list of mappings
				tuple<float, float, vector<int>> mapping(amdMax, mappingTSP, selectedCores);
				candidates.push_back(mapping);
			}
		}
	}

	return candidates;
}

/** map
    This function performs patterning after TaskReassign
*/
std::vector<int> TaskReassign::map(String taskName, int taskCoreRequirement, const vector<bool> &availableCores, const vector<bool> &activeCores) {
	// get the mapping candidates
	vector<tuple<float, float, vector<int>>> mappingCandidates = getMappingCandidates(taskCoreRequirement, availableCores, activeCores);

	// find the best mapping
	int bestMappingNb = 0;
	if (mappingCandidates.size() == 0) {
        vector<int> empty;
		return empty;
	} else if (mappingCandidates.size() == 1) {
		bestMappingNb = 0;
	} else {
		float minAmd = get<0>(mappingCandidates.front());
		float minPowerBudget = get<1>(mappingCandidates.front());
		float deltaAmd = get<0>(mappingCandidates.back()) - minAmd;
		float deltaPowerBudget = get<1>(mappingCandidates.back()) - minPowerBudget;

		float alpha = deltaPowerBudget / deltaAmd;

		float bestRating = numeric_limits<float>::lowest();

		for (unsigned int mappingNb = 0; mappingNb < mappingCandidates.size(); mappingNb++) {
			float amdMax = get<0>(mappingCandidates.at(mappingNb));
			float powerBudget = get<1>(mappingCandidates.at(mappingNb));
			float rating = (powerBudget - minPowerBudget) - alpha * (amdMax - minAmd);
			vector<int> cores = get<2>(mappingCandidates.at(mappingNb));

			cout << "Mapping Candidate: rating; " << setprecision(3) << rating << ", power budget: " << setprecision(3) << powerBudget << " W, max. AMD: " << setprecision(3) << amdMax;
			cout << ", used cores:";
			for (unsigned int i = 0; i < cores.size(); i++) {
				cout << " " << cores.at(i);
			}
			cout << endl;
            
            rating = -1;
			if (rating > bestRating) {
				bestRating = rating;
				bestMappingNb = mappingNb;
			}
		}
	}

	// return the cores
	return get<2>(mappingCandidates.at(bestMappingNb));
}


// std::vector<int> TaskReassign::map(
//         String taskName,
//         int taskCoreRequirement,
//         const std::vector<bool> &availableCoresRO,
//         const std::vector<bool> &activeCores) {

//     std::vector<bool> availableCores(availableCoresRO);

//     std::vector<int> cores;
    

//     //logTemperatures(availableCores);

//     for (; taskCoreRequirement > 0; taskCoreRequirement--) {
//         //int coldestCore = getColdestCore(availableCores);
//         int coldestCore = getPeriodicCore(availableCores);
//         cout << "[checking]Selectedc core is " << coldestCore << endl;

//         if (coldestCore == -1) {
//             // not enough free cores
//             std::vector<int> empty;
//             return empty;
//         } else {
//             cores.push_back(coldestCore);
//             availableCores.at(coldestCore) = false;
//         }
//     }

//     return cores;
// }


double* TaskReassign::componentCal(int para, int nActive, double* p){
    nexponentials = new double[numberThermalNodes];
    eachItem = new double*[numberThermalNodes];
    eachComp = new double*[numberThermalNodes];
    Item = new double[numberThermalNodes];

    for(int i = 0; i < numberThermalNodes; i++){
        eachItem[i] = new double[numberThermalNodes];
        eachComp[i] = new double[numberThermalNodes];
    }

    for(int i = 0; i < numberThermalNodes; i++){
        nexponentials[i] = pow((double)M_E, eigenValues[i] * pb_epoch_length  * para) 
            /(1 - pow((double)M_E, eigenValues[i] * pb_epoch_length  * nActive));
    }

    for(int k = 0; k < numberThermalNodes; k++){
        for(int j = 0; j < numberThermalNodes; j++){
            eachItem[k][j] = 0;
            for(int i = 0; i < numberThermalNodes; i++){
                eachItem[k][j] += eigenVectors[k][i]*eigenVectorsInv[i][j]*nexponentials[i];       
            }
        }
        for(int j = 0; j < numberThermalNodes; j++){
            eachComp[k][j] = 0;
            for(int i = 0; i < numberThermalNodes; i++){
                eachComp[k][j] += eachItem[k][i] * HelpW[i][j];
            }
        }
        Item[k] = 0;
        for(int i = 0;i < 16;i++){
            Item[k] +=  eachComp[k][i] * p[i];
        }
    }
    return Item;
}

double* TaskReassign::componentCalActive(int para, int nActive, double* p,const std::vector<bool> &activeCores){
    nexponentials = new double[numberThermalNodes];
    eachItem = new double*[numberThermalNodes];
    eachComp = new double*[numberThermalNodes];
    
    std::vector<int> activeIndices;
    for (int i = 0; i < coreRows * coreColumns; i++) {
        if (activeCores.at(i)) {
            activeIndices.push_back(i);
        }
    } 
    //cout << "The activeIndices is " << activeIndices.size() << endl;
    Item = new double[activeIndices.size()];


    for(int i = 0; i < numberThermalNodes; i++){
        eachItem[i] = new double[numberThermalNodes];
        eachComp[i] = new double[numberThermalNodes];
    }

    for(int i = 0; i < numberThermalNodes; i++){
        nexponentials[i] = pow((double)M_E, eigenValues[i] * pb_epoch_length  * para) 
            /(1 - pow((double)M_E, eigenValues[i] * pb_epoch_length  * nActive));
    }

    for(int k = 0; k < numberThermalNodes; k++){
        for(int j = 0; j < numberThermalNodes; j++){
            eachItem[k][j] = 0;
            for(int i = 0; i < numberThermalNodes; i++){
                eachItem[k][j] += eigenVectors[k][i]*eigenVectorsInv[i][j]*nexponentials[i];       
            }
        }
        for(int j = 0; j < numberThermalNodes; j++){
            eachComp[k][j] = 0;
            for(int i = 0; i < numberThermalNodes; i++){
                eachComp[k][j] += eachItem[k][i] * HelpW[i][j];
            }
        }
    }
        
    for(int i = 0;i < activeIndices.size();i++){
        Item[i] = 0;
        for(int j = 0;j < activeIndices.size();j++){
            Item[i] +=  eachComp[activeIndices.at(i)][activeIndices.at(j)] * p[j];
        }
    }
    return Item;
}


double TaskReassign::collectComp(){
    double maxPeak = -99999999;
    //double pTemp[16] = {6.194, 4.836, 4.325, 1.859, 2.407, 2.315, 4.513, 5.865, 3.058, 4.035, 3.144, 4.943, 3.531, 0.27,0.27,0.27};
    vector<double> pTemp;
    //cout << "The number of numberUnits is " << numberUnits << endl;
    for(int i = 0;i < numberUnits;i++){
        //float corePower = performanceCounters->getPowerOfCore(i);
        float corePower = performanceCounters->getHistoryPowerOfCore(i);
        //cout << "corePower is " << corePower << endl;
        pTemp.push_back(corePower);
    }
    double** P = new double*[numberUnits];  
    for(int i = 0;i < numberUnits;i++){
        P[i] = new double[numberUnits];
        for(int j = 0; j < numberUnits;j++) P[i][j] = pTemp[j];
        double dataTemp = pTemp[0];
        for(int j = 1; j < numberUnits;j++) pTemp[j-1] = pTemp[j];
        pTemp[numberUnits-1] = dataTemp;
    }
    //for(int j = 0; j < numberUnits; j ++) cout << P[1][j] << endl;
    double* tmp = new double[numberUnits];
    double *tmpC;
    //for(int i = 0; i < numberUnits;i++) tmp[i] = 0;
    int para;
    for(int i = 0;i < numberUnits;i++){
        int count = 0;
        para = i;
        for(int kk = 0; kk < numberUnits;kk++) tmp[kk] = 0;
        for (int j = 0;j < i;j++){  
            tmpC = componentCal(para,numberUnits,P[count]);
            for(int k = 0;k < numberUnits;k++) tmp[k] += tmpC[k];
            count++;
            para--;
        }
        tmpC = componentCal(0,numberUnits,P[count]);
        //for(int h = 0;h < numberUnits;h++) cout << "P[count] is " << P[count][h] << endl;
        for(int k = 0; k < numberUnits;k++) tmp[k] += tmpC[k];
        //cout << "tmp[0] is " << tmp[0] << endl;
        para = numberUnits - 1;
        for(int j = i+1; j < numberUnits;j++){
            count++;
            tmpC = componentCal(para,numberUnits,P[count]);
            for(int k = 0; k < numberUnits;k++) tmp[k] += tmpC[k];
            para--;
        }
        for(int k = 0; k < numberUnits;k++){
            if(maxPeak < tmp[k]) maxPeak = tmp[k];
        } 
    }
    return maxPeak + ambientTemperature;       

}

double TaskReassign::concentric(const std::vector<bool> &activeCores){
    double maxPeak = -99999999;
    //double pTemp[16] = {6.194, 4.836, 4.325, 1.859, 2.407, 2.315, 4.513, 5.865, 3.058, 4.035, 3.144, 4.943, 3.531, 0.27,0.27,0.27};
    std::vector<double> pTemp;
    std::vector<double> fastTrack;
    std::vector<double> slowTrack;
    std::vector<int> activeIndices;
    std::set<double> nActiveAMD;
    std::vector<tuple<float ,float ,float>> singlePower;
    for(int i = 0;i < coreRows * coreColumns;i++){
        if(activeCores.at(i)){
            activeIndices.push_back(i);
        }
    }

    for (int i = 0; i < coreRows * coreColumns; i++) {
        if (activeCores.at(i)) {
            float amd = getCoreAMD(i / coreColumns, i % coreRows);
            float corePower = performanceCounters->getHistoryPowerOfCore(i);
            tuple<float,float,float> tempData(amd,corePower,i);
            singlePower.push_back(tempData);
        }
    } 
    for(int i = 0;i < singlePower.size();i++) nActiveAMD.insert(get<0>(singlePower[i]));
    std::vector<double> nActiveAMD1(nActiveAMD.begin(),nActiveAMD.end());
    std::sort(nActiveAMD1.begin(),nActiveAMD1.end());


    // cout << "the size of singlePower is " << singlePower.size() << endl;
    // cout << "***  the size of nActiveAMD is ****" << nActiveAMD.size() << endl;
    std::vector<std::vector<std::pair<float, float>> > groupTrack(nActiveAMD1.size());

    for(int i = 0;i < nActiveAMD1.size();i++){
        for(int j = 0;j < singlePower.size();j++){
            if(nActiveAMD1[i] == get<0>(singlePower[j])){
                groupTrack[i].push_back(make_pair(get<2>(singlePower[j]),get<1>(singlePower[j])));
            }
        }
    }
    // for(auto i : groupTrack[1]){
    //     cout << "test ***" << i.first << "    " << i.second << endl;
    // }
    double** P1 = new double*[groupTrack[0].size()]; 
    
    for(int i = 0;i < groupTrack[0].size();i++){
        P1[i] = new double[activeCores.size()];

        vector<pair<double,double>> tempCollect;
        for(int jj = 0;jj < groupTrack.size();jj++){
            for(auto eachTrack: groupTrack[jj]){
                tempCollect.push_back(eachTrack);
            }
        }
        sort(tempCollect.begin(),tempCollect.end());

        for(int jj = 0;jj < tempCollect.size();jj++){
            P1[i][jj] = tempCollect[jj].second;
            cout << "The data is " << i  << " ....  "<< P1[i][jj] << endl;
        }

        
        for(int j = 0;j < groupTrack.size();j++){
            double temp = groupTrack[j][0].second;
            for(int k = 1;k < groupTrack[j].size();k++){
                groupTrack[j][k-1].second = groupTrack[j][k].second;
            }
            groupTrack[j][groupTrack[j].size() - 1].second = temp;
        }
    }







    


    

   
    // for(int i = 0;i < activeIndices.size();i++){
    //     float amd = getCoreAMD(activeIndices.at(i) / coreColumns, activeIndices.at(i) % coreRows);
    //     float corePower = performanceCounters->getHistoryPowerOfCore(activeIndices.at(i));
    //     //activeAMD.insert(amd);     
    // }
    // for(auto i : activeAMD){
    //     sortAMD.push_back(i);
    // }
    // sort(sortAMD.begin(),sortAMD.end());
  
    // for(int i = 0;i < sortAMD.size();i++){
    //     for(int k = 0; k < activeIndices.size();k++){
    //         if(i == 0 && sortAMD[i] == getCoreAMD(activeIndices.at(k) / coreColumns, activeIndices.at(k) % coreRows)){
    //             float corePower = performanceCounters->getHistoryPowerOfCore(activeIndices.at(i));
    //             fastTrack.push_back(corePower);
    //         }
    //         else if(i == 1 && sortAMD[i] == getCoreAMD(activeIndices.at(k) / coreColumns, activeIndices.at(k) % coreRows)){
    //             float corePower = performanceCounters->getHistoryPowerOfCore(activeIndices.at(i));
    //             slowTrack.push_back(corePower);
    //         }
    //     }
    // }



    for(int i = 0;i < activeIndices.size();i++){
        //float corePower = performanceCounters->getPowerOfCore(i);
        float corePower = performanceCounters->getHistoryPowerOfCore(activeIndices.at(i));
        //cout << "corePower is " << corePower << endl;
        pTemp.push_back(corePower);
    }

    double** P = new double*[activeIndices.size()];  
    for(int i = 0;i < activeIndices.size();i++){
        P[i] = new double[activeIndices.size()];
        for(int j = 0; j < activeIndices.size();j++) P[i][j] = pTemp[j];
        double dataTemp = pTemp[0];
        for(int j = 1; j < activeIndices.size();j++) pTemp[j-1] = pTemp[j];
        pTemp[activeIndices.size()-1] = dataTemp;
    }
    double* tmp = new double[activeIndices.size()];
    double *tmpC;
    //for(int i = 0; i < numberUnits;i++) tmp[i] = 0;
    int para;
    // for(int i = 0;i < activeIndices.size();i++){
    //     int count = 0;
    //     para = i;
    //     for(int kk = 0; kk < activeIndices.size();kk++) tmp[kk] = 0;
    //     for (int j = 0;j < i;j++){  
    //         tmpC = componentCalActive(para,activeIndices.size(),P[count],activeCores);
    //         for(int k = 0;k < activeIndices.size();k++) tmp[k] += tmpC[k];
    //         count++;
    //         para--;
    //     }
    //     tmpC = componentCalActive(0,activeIndices.size(),P[count],activeCores);
    //     //for(int h = 0;h < numberUnits;h++) cout << "P[count] is " << P[count][h] << endl;
    //     for(int k = 0; k < activeIndices.size();k++) tmp[k] += tmpC[k];
    //     //cout << "tmp[0] is " << tmp[0] << endl;
    //     para = activeIndices.size() - 1;
    //     for(int j = i+1; j < activeIndices.size();j++){
    //         count++;
    //         tmpC = componentCalActive(para,activeIndices.size(),P[count],activeCores);
    //         for(int k = 0; k < activeIndices.size();k++) tmp[k] += tmpC[k];
    //         para--;
    //     }
    //     for(int k = 0; k < activeIndices.size();k++){
    //         if(maxPeak < tmp[k]) maxPeak = tmp[k];
    //     } 
    // }

    // Confirm the rotation line
    int concenticPeriod = groupTrack[0].size();

    for(int i = 0;i < concenticPeriod;i++){
        int count = 0;
        para = i;
        // calculate the temperatuer of active cores
        for(int kk = 0; kk < activeIndices.size();kk++) tmp[kk] = 0;
        for (int j = 0;j < i;j++){  
            tmpC = componentCalActive(para,activeIndices.size(),P1[count],activeCores);
            for(int k = 0;k < activeIndices.size();k++) tmp[k] += tmpC[k];
            count++;
            para--;
        }
        tmpC = componentCalActive(0,activeIndices.size(),P1[count],activeCores);
        //for(int h = 0;h < numberUnits;h++) cout << "P[count] is " << P[count][h] << endl;
        for(int k = 0; k < activeIndices.size();k++) tmp[k] += tmpC[k];
        //cout << "tmp[0] is " << tmp[0] << endl;
        para = concenticPeriod - 1;
        for(int j = i+1; j < concenticPeriod;j++){
            count++;
            tmpC = componentCalActive(para,activeIndices.size(),P1[count],activeCores);
            for(int k = 0; k < activeIndices.size();k++) tmp[k] += tmpC[k];
            para--;
        }
        for(int k = 0; k < activeIndices.size();k++){
            if(maxPeak < tmp[k]) maxPeak = tmp[k];
        } 
    }
    
    return maxPeak + ambientTemperature;       

}


double* TaskReassign::matrixAdd(double* A, double* B){
    double*C = new double[numberUnits];
    for (int i = 0;i < numberUnits;i++){
        C[i] = A[i] + B[i];
    }
    return C;
}

std::vector<migration> TaskReassign::migrate(
        SubsecondTime time,
        const std::vector<int> &taskIds,
        const std::vector<bool> &activeCores) {


    // double powerCore5 = performanceCounters->getPowerOfCore(5);
    // cout << "The power of core 5 is " << powerCore5 << endl;
    // double predictTemp = collectComp();
    // cout << "The peak temperature is " << predictTemp << endl;
    // double predictAnother = concentric(activeCores);
    // cout << "The predictAnother temperature is " << predictAnother << endl;


    std::vector<migration> migrations;

    std::vector<bool> availableCores(coreRows * coreColumns);
    for (int c = 0; c < coreRows * coreColumns; c++) {
        availableCores.at(c) = taskIds.at(c) == -1;
    }

    for (int c = 0; c < coreRows * coreColumns; c++) {
        if (activeCores.at(c)) {
            float temperature = performanceCounters->getTemperatureOfCore(c);
            if (temperature > criticalTemperature  || 1) {
                migration m;
                cout << "[Scheduler][AnuCore-migrate]: core" << c << " too hot (";
                cout << fixed << setprecision(1) << temperature << ") -> migrate";
                 //logTemperatures(availableCores);
                 //int targetCore = getColdestCore(availableCores);
                 int targetCore = getPeriodicCore(activeCores);

                 if (targetCore == -1) {
                     cout << "[Scheduler][coldestCore-migrate]: no target core found, cannot migrate" << endl;
                 }

                 cout << "--------------------outside anu" << availableCores.at(targetCore) <<targetCore <<endl;

                if(availableCores.at(targetCore)){ 
                m.swap = false; 
                cout << "inside anu" << availableCores.at(targetCore) << targetCore  <<endl;
                }
                else
                { m.swap = true;
               cout << "else anu" << availableCores.at(targetCore) << targetCore  <<endl;
                }

              if(m.swap){
                 m.fromCore = c;
                 m.toCore = targetCore;
                  migrations.push_back(m);
                 availableCores.at(c) = false;
                 availableCores.at(targetCore) = false;
                 cout << "inside mswap anu" << availableCores.at(targetCore) << targetCore  <<endl; 
               } else{ 
                   cout << "else mswap anu" << availableCores.at(targetCore) << targetCore  <<endl;
                    m.fromCore = c;
                    m.toCore = targetCore;

                   migrations.push_back(m);
                    
                    availableCores.at(c) = true;
                    availableCores.at(targetCore) = false;
               }
            }
        }/*
        else if(!availableCores.at(c)) {
            cout << "$$$$$   major else if " <<endl;
            int targetCore = getPeriodicCore(activeCores);
            if(availableCores.at(targetCore)) continue;
            migration m;
            m.fromCore = c;
            m.toCore = targetCore;
            m.swap = false;
            migrations.push_back(m);
            availableCores.at(targetCore) = true;
            availableCores.at(c) = true;
        }*/
    }

    
    // int init[8] = {1,2,7,11,14,13,8,4};
    // for(int i = 0;i < 8;i+=2){
    //     if(activeCores.at(init[i%8]) && availableCores.at(init[(i+1)%8])){
    //         migration m;
    //         m.fromCore = init[i%8];
    //         m.toCore = init[(i+1)%8];
    //         m.swap = false;
    //         migrations.push_back(m);
    //     }else if(availableCores.at(init[i%8]) && availableCores.at(init[(i+1)%8])){
    //         continue;
    //     }else{
    //         if(activeCores.at(init[(i+1)%8]) && availableCores.at(init[(i+2)%8])){
    //             migration m;
    //             m.fromCore = init[(i+1)%8];
    //             m.toCore = init[(i+2)%8];
    //             m.swap = false;
    //             migrations.push_back(m);
    //         }
    //     }
        
    // }


    //int temp[4] = {5,6,9,10};
    //for(int i = 1;i < 4;i++){
    //    if (activeCores.at(temp[i])) {
    //        migration m;
    //        m.fromCore = temp[i];
    //        m.toCore = temp[0];
    //        if(availableCores.at(temp[0])) m.swap = false;
    //        else m.swap = true;
    //       migrations.push_back(m);
    //        if(m.swap){
    //            availableCores.at(temp[0]) = false;
    //            availableCores.at(temp[i]) = false;
    //        }else{
    //            availableCores.at(temp[i]) = true;
    //            availableCores.at(temp[0]) = false;
    //        }
    //    } 
    //    else{
    //        if(availableCores.at(temp[0])) continue;
    //        else{
    //            migration m;
    //            m.fromCore = temp[0];
    //            m.toCore = temp[i];
    //            m.swap = false;
    //            migrations.push_back(m);
    //           availableCores.at(temp[i]) = false;
    //            availableCores.at(temp[0]) = true;
    //            }                
    //        }
    //}
    
    
    return migrations;
}



int TaskReassign::getColdestCore(const std::vector<bool> &availableCores) {
    int coldestCore = -1;
    float coldestTemperature = 0;
    //iterate all cores to find coldest
    for (int c = 0; c < coreRows * coreColumns; c++) {
        if (availableCores.at(c)) {
            float temperature = performanceCounters->getTemperatureOfCore(c);
            if ((coldestCore == -1) || (temperature < coldestTemperature)) {
                coldestCore = c;
                coldestTemperature = temperature;
            }
        }
    }
    
    return coldestCore;
    
}

int TaskReassign::getPeriodicCore(const std::vector<bool> &activeCores){
    int periodicCore = 0;
    for (int c = 0; c < coreRows * coreColumns; c++) {  
        if (activeCores.at(c)) {
            if(c == 27) periodicCore = 28;
            else if (c == 28) periodicCore = 35;
            else if (c == 35) periodicCore = 36;
            else if (c == 36) periodicCore = 27;
        }
    }                
    return periodicCore;

    
}




void TaskReassign::logTemperatures(const std::vector<bool> &availableCores) {
    cout << "[Scheduler][coldestCore-map]: temperatures of available cores:" << endl;
    for (int y = 0; y < coreRows; y++) {
        for (int x = 0; x < coreColumns; x++) {
            if (x > 0) {
                cout << " ";
            }
            int coreId = y * coreColumns + x;

            if (!availableCores.at(coreId)) {
                cout << "  - ";
            } else {
                float temperature = performanceCounters->getTemperatureOfCore(coreId);
                cout << fixed << setprecision(1) << temperature;
            }
        }
        cout << endl;
    }
}