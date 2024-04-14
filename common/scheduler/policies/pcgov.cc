#include "pcgov.h"
#include <iomanip>
#include <limits>
#include <tuple>
#include "powermodel.h"

using namespace std;

PCGov::PCGov(ThermalModel *thermalModel, PerformanceCounters *performanceCounters, int coreRows, int coreColumns, int minFrequency, int maxFrequency, int frequencyStepSize, float delta)
     : thermalModel(thermalModel), performanceCounters(performanceCounters), coreRows(coreRows), coreColumns(coreColumns),
	 minFrequency(minFrequency), maxFrequency(maxFrequency), frequencyStepSize(frequencyStepSize), delta(delta) {
	// get core AMD info
    for (int y = 0; y < coreColumns; y++) {
		for (int x = 0; x < coreRows; x++) {
			float amd = getCoreAMD(y, x);
			amds.push_back(amd);
			uniqueAMDs.insert(amd);
			threadStates.push_back(ThreadState::IDLE);
			powerBudgets.push_back(thermalModel->getInactivePower());
		}
	}
}

int PCGov::manhattanDistance(int y1, int x1, int y2, int x2) {
	int dy = y1 - y2;
	int dx = x1 - x2;
	return abs(dy) + abs(dx);
}

float PCGov::getCoreAMD(int coreY, int coreX) {
	int md_sum = 0;
	for (unsigned int y = 0; y < coreColumns; y++) {
		for (unsigned int x = 0; x < coreRows; x++) {
			md_sum += manhattanDistance(coreY, coreX, y, x);
		}
	}
	return (float)md_sum / coreColumns / coreRows;
}


/** getMappingCandidates
 * Get all near-Pareto-optimal mappings considered in PCGov.
 * Return a vector of tuples (max. AMD, power budget, used cores)
 */
vector<tuple<float, float, vector<int>>> PCGov::getMappingCandidates(int taskCoreRequirement, const vector<bool> &availableCores, const vector<bool> &activeCores) {
	// get the candidates
	vector<tuple<float, float, vector<int>>> candidates;

	std::cout <<"===========getMappingCandidates==========" <<std::endl;
	for (const float &amdMax : uniqueAMDs) {
		// get the candidate for the given AMD_max

		vector<int> availableCoresAMD;
		for (unsigned int i = 0; i < coreRows * coreColumns; i++) {
			if (availableCores.at(i) && (amds.at(i) <= amdMax)) {
				availableCoresAMD.push_back (i);
				//std::cout << "availableCoresAMD = " << i <<std::endl;
			}
		}

		if ((int)availableCoresAMD.size() >= taskCoreRequirement) {
			vector<int> selectedCores;

			vector<bool> activeCoresCandidate = activeCores;

			float mappingTSP = 0;
			float maxUsedAMD = 0;
			while ((int)selectedCores.size() < taskCoreRequirement) {
				// greedily select one core

			//std::cout <<" PCGov::getMappingCandidates  calling tspformany candidate   ====="<<std::endl;

				vector<double> tsps = thermalModel->tspForManyCandidates(activeCoresCandidate, availableCoresAMD);
 
				float bestTSP = 0;
				int bestIndex = 0;
				for (unsigned int i = 0; i < tsps.size(); i++) {
					//std::cout <<  "tsps.at(i) = " <<tsps.at(i) <<std::endl;
					if (tsps.at(i) > bestTSP) {
						bestTSP = tsps.at(i);
						bestIndex = i;

						//std::cout << "i = " << i << " bestTSP = " <<bestTSP <<std::endl;
					   //std::cout <<  "bestIndex  = " <<bestIndex  <<std::endl;

					}
				}

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
    This function performs patterning
*/
std::vector<int> PCGov::map(String taskName, int taskCoreRequirement, const vector<bool> &availableCores, const vector<bool> &activeCores) {
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
   
        /*
		std::cout << " map ==============="<<std::endl;
		std::cout <<"minAmd = " << minAmd <<std::endl;
		std::cout << "minPowerBudget =" << minPowerBudget <<std::endl;
		std::cout << "deltaAmd =" <<deltaAmd <<std::endl;
		std::cout << "deltaPowerBudget = "<< deltaPowerBudget<<std::endl;
		std::cout << "alpha = " << alpha << std::endl;
		*/

		for (unsigned int mappingNb = 0; mappingNb < mappingCandidates.size(); mappingNb++) {
			float amdMax = get<0>(mappingCandidates.at(mappingNb));
			float powerBudget = get<1>(mappingCandidates.at(mappingNb));
			float rating = (powerBudget - minPowerBudget) - alpha * (amdMax - minAmd);
			vector<int> cores = get<2>(mappingCandidates.at(mappingNb));
           
		    /*
			std::cout << "amdMax = " << amdMax<<std::endl;
			std::cout << "powerBudget = " <<powerBudget <<std::endl;
			std::cout << "rating " << rating <<std::endl;

			cout << "Mapping Candidate: rating; " << setprecision(3) << rating << ", power budget: " << setprecision(3) << powerBudget << " W, max. AMD: " << setprecision(3) << amdMax;
			cout << ", used cores:";
			*/
		
			for (unsigned int i = 0; i < cores.size(); i++) {
				cout << " " << cores.at(i);
			}
			cout << endl;
            
			
			if (rating > bestRating) {
				bestRating = rating;
				bestMappingNb = mappingNb;
			}
		}
	}

	// return the cores
	return get<2>(mappingCandidates.at(bestMappingNb));
}


void PCGov::updatePowerBudgets(const std::vector<int> &oldFrequencies) {
	bool recalculateComputeBound = false;
	for (unsigned int coreCounter = 0; coreCounter < coreRows * coreColumns; coreCounter++) {
		double utilization = performanceCounters->getUtilizationOfCore(coreCounter);
		double power = utilization > 0 ? performanceCounters->getPowerOfCore(coreCounter) : 0;
		int frequency = oldFrequencies.at(coreCounter);
		bool atMaximumFrequency = (frequency == maxFrequency);
		switch (threadStates.at(coreCounter)) {
			case ThreadState::IDLE:
				if (utilization > 0) {
					std::cout <<"IDLE utilization"<< utilization <<std::endl;
					cout << "[Scheduler][PCGov] Core " << coreCounter << " switches to state COMPUTE" << endl;
					threadStates.at(coreCounter) = ThreadState::COMPUTE;
					powerBudgets.at(coreCounter) = -1;
					recalculateComputeBound = true;
				}
				break;
			case ThreadState::COMPUTE:
				if (utilization == 0) {

					std::cout <<"COMPUTE utilization"<< utilization <<std::endl;

					cout << "[Scheduler][PCGov] Core " << coreCounter << " switches to state IDLE due to low utilization" << endl;
					threadStates.at(coreCounter) = ThreadState::IDLE;
					powerBudgets.at(coreCounter) = thermalModel->getInactivePower();
					recalculateComputeBound = true;
                    
					std::cout << "coreCounter = " << coreCounter<<std::endl;
				    std::cout << "powerBudgets.at(coreCounter) "<< powerBudgets.at(coreCounter) << "thermalModel->getInactivePower()= "<< thermalModel->getInactivePower()  <<std::endl;
					std::cout <<" COMPUTE utilization"<< utilization <<std::endl;

				} else if (atMaximumFrequency && (power < powerBudgets.at(coreCounter) - delta)) {
					std::cout <<"COMPUTE utilization"<< utilization <<std::endl;

					cout << "[Scheduler][PCGov] Core " << coreCounter << " switches to state MEMORY due to low power consumption" << endl;
					threadStates.at(coreCounter) = ThreadState::MEMORY;
					powerBudgets.at(coreCounter) = power + delta;
					recalculateComputeBound = true;
 					
					std::cout << "coreCounter = " << coreCounter<<std::endl;
				    std::cout << "powerBudgets.at(coreCounter) "<< powerBudgets.at(coreCounter)  <<  "power = " <<power <<" power + delta = "  << power + delta <<std::endl;
					
					std::cout <<"COMPUTE utilization"<< utilization <<std::endl;
				}
				break;
			case ThreadState::MEMORY:
				if (utilization == 0) {
					cout << "[Scheduler][PCGov] Core " << coreCounter << " switches to state IDLE due to low utilization" << endl;
					threadStates.at(coreCounter) = ThreadState::IDLE;
					powerBudgets.at(coreCounter) = thermalModel->getInactivePower();
					recalculateComputeBound = true;
				} else if (!atMaximumFrequency || (power > powerBudgets.at(coreCounter))) {
					cout << "[Scheduler][PCGov] Core " << coreCounter << " switches to state COMPUTE due to high power consumption" << endl;
					threadStates.at(coreCounter) = ThreadState::COMPUTE;
					powerBudgets.at(coreCounter) = -1;
					recalculateComputeBound = true;
				} else if (power < powerBudgets.at(coreCounter) - delta) {
					powerBudgets.at(coreCounter) = power + delta;
					recalculateComputeBound = true;
				}
				break; 
			default:
				cout << "PCGov::updatePowerBudgets: unknown thread state encountered" << endl;
				exit(1);
		}
	}

	if (recalculateComputeBound) {
		std::cout << "recalculateComputeBound ===" <<std::endl;

		std::vector<bool> unrestrictedCores(coreRows * coreColumns);
		for (unsigned int coreCounter = 0; coreCounter < coreRows * coreColumns; coreCounter++) {
			unrestrictedCores.at(coreCounter) = (threadStates.at(coreCounter) == ThreadState::COMPUTE);
		}
		float uniformPerCorePowerBudget = thermalModel->tsp(unrestrictedCores);
		for (unsigned int coreCounter = 0; coreCounter < coreRows * coreColumns; coreCounter++) {
			if (unrestrictedCores.at(coreCounter)) {
				powerBudgets.at(coreCounter) = uniformPerCorePowerBudget;
				std::cout <<  " coreCounter = " <<powerBudgets.at(coreCounter);

			}
		}
	}
}

std::vector<int> PCGov::getFrequencies(const std::vector<int> &oldFrequencies, const std::vector<bool> &activeCores) {
	updatePowerBudgets(oldFrequencies);

	std::vector<int> frequencies(coreRows * coreColumns);

	for (unsigned int coreCounter = 0; coreCounter < coreRows * coreColumns; coreCounter++) {
		if (activeCores.at(coreCounter)) {
			float powerBudget = powerBudgets.at(coreCounter);
			float power = performanceCounters->getPowerOfCore(coreCounter);
			float temperature = performanceCounters->getTemperatureOfCore(coreCounter);
			int frequency = oldFrequencies.at(coreCounter);
			float utilization = performanceCounters->getUtilizationOfCore(coreCounter);

			cout << "[Scheduler][PCGov]: Core " << setw(2) << coreCounter << " ";
			switch (threadStates.at(coreCounter)) {
				case ThreadState::IDLE:
					cout << "[IDLE]   ";
					break;
				case ThreadState::COMPUTE:
					cout << "[COMPUTE]";
					break;
				case ThreadState::MEMORY:
					cout << "[MEMORY] ";
					break;
				default:
					cout << "[???????]";
					break;
			}
			cout << ": P=" << fixed << setprecision(4) << power << " W";
			cout << " (budget: " << fixed << setprecision(4) << powerBudget << " W)";
			cout << " f=" << frequency << " MHz";
			cout << " T=" << fixed << setprecision(1) << temperature << " °C";
			cout << " utilization=" << fixed << setprecision(4) << utilization << endl;

            // added by anu

			float RelNUCACPI  = performanceCounters->getRelNUCACPIOfCore(coreCounter);
			float IPS         = performanceCounters->getIPSOfCore(coreCounter);
			float cpi_total   = performanceCounters->getCPIOfCore(coreCounter);
			float peak_temperature = performanceCounters->getPeakTemperature();

			cout << " IPS=" << fixed << setprecision(3) << IPS ;
			cout << " RelNUCACPI=" << fixed << setprecision(3) << RelNUCACPI ;
			cout << " hotspot_peak_temperature=" << fixed << setprecision(3) << peak_temperature ;
			cout << " cpi-total=" << fixed << setprecision(3) <<cpi_total << endl;



			int expectedGoodFrequency = PowerModel::getExpectedGoodFrequency(frequency, power, powerBudget, minFrequency, maxFrequency, frequencyStepSize);
			frequencies.at(coreCounter) = expectedGoodFrequency;
		} else {
			frequencies.at(coreCounter) = minFrequency;
		}
	}

// added by anu
	std::ofstream outputFileC("instacpi_total.txt");
	std::ofstream outputFileCP("periodiccpi_total.txt",fstream::app);

    for (unsigned int coreCounter = 0; coreCounter < coreRows * coreColumns; coreCounter++) {

       outputFileC << std::fixed << std::setprecision(4) << performanceCounters->getCPIOfCore(coreCounter) << "\t";
       outputFileCP << std::fixed << std::setprecision(4) << performanceCounters->getCPIOfCore(coreCounter) << "\t";
	}

    outputFileC << "\n" ;
	outputFileCP << "\n";
	outputFileC.close();
	outputFileCP.close();

// added by anu
	std::ofstream outputFileI("instaIPS.txt");
	std::ofstream outputFileIP("periodicIPS.txt",fstream::app);

    for (unsigned int coreCounter = 0; coreCounter < coreRows * coreColumns; coreCounter++) {

       outputFileI << std::fixed << std::setprecision(3) << performanceCounters->getIPSOfCore(coreCounter) << "\t";
       outputFileIP << std::fixed << std::setprecision(3) << performanceCounters->getIPSOfCore(coreCounter) << "\t";
	}

    outputFileI << "\n" ;
	outputFileIP << "\n";
	outputFileI.close();
	outputFileIP.close();

// added by anu
	std::ofstream outputFileR("instaRelNUCACPI.txt");
	std::ofstream outputFileRP("periodicRelNUCACPI.txt",fstream::app);

    for (unsigned int coreCounter = 0; coreCounter < coreRows * coreColumns; coreCounter++) {

       outputFileR << std::fixed << std::setprecision(3) << performanceCounters->getRelNUCACPIOfCore(coreCounter) << "\t";
       outputFileRP << std::fixed << std::setprecision(3) << performanceCounters->getRelNUCACPIOfCore(coreCounter) << "\t";
	}

    outputFileR << "\n" ;
	outputFileRP << "\n";
	outputFileR.close();
	outputFileRP.close();

// added by anu
	std::ofstream outputFile("instaUtlization.txt");
	std::ofstream outputFileP("periodicUtlization.txt",fstream::app);

    for (unsigned int coreCounter = 0; coreCounter < coreRows * coreColumns; coreCounter++) {

       outputFile << std::fixed << std::setprecision(3) << performanceCounters->getUtilizationOfCore(coreCounter) << "\t";
       outputFileP << std::fixed << std::setprecision(3) << performanceCounters->getUtilizationOfCore(coreCounter) << "\t";
	}

    outputFile << "\n" ;
	outputFileP << "\n";
	outputFile.close();
	outputFileP.close();

// added by anu
	std::ofstream outputFileT("insta_anu_Temperature.txt");
	std::ofstream outputFileTP("periodic_anu_Temperature.txt",fstream::app);

    for (unsigned int coreCounter = 0; coreCounter < coreRows * coreColumns; coreCounter++) {

       outputFileT << std::fixed << std::setprecision(3) << performanceCounters->getTemperatureOfCore(coreCounter) << "\t";
       outputFileTP << std::fixed << std::setprecision(3) << performanceCounters->getTemperatureOfCore(coreCounter) << "\t";
	}

    outputFileT << "\n" ;
	outputFileTP << "\n";
	outputFileT.close();
	outputFileTP.close();

// added by anu
	std::ofstream outputFilePT("insta_peak_Temperature.txt");
	std::ofstream outputFilePTP("periodic_peak_Temperature.txt",fstream::app);

    for (unsigned int coreCounter = 0; coreCounter < coreRows * coreColumns; coreCounter++) {

       outputFilePT << std::fixed << std::setprecision(3) << performanceCounters->getPeakTemperature() << "\t";
       outputFilePTP << std::fixed << std::setprecision(3) << performanceCounters->getPeakTemperature()<< "\t";
	}

    outputFilePT << "\n" ;
	outputFilePTP << "\n";
	outputFilePT.close();
	outputFilePTP.close();

// added by anu
	std::ofstream outputFilep("insta_power_used.txt");
	std::ofstream outputFilePw("periodic_power_used.txt",fstream::app);

    for (unsigned int coreCounter = 0; coreCounter < coreRows * coreColumns; coreCounter++) {

       outputFilep << std::fixed << std::setprecision(4) << performanceCounters->getPowerOfCore(coreCounter) << "\t";
       outputFilePw << std::fixed << std::setprecision(4) << performanceCounters->getPowerOfCore(coreCounter) << "\t";
	}

    outputFilep << "\n" ;
	outputFilePw << "\n";
	outputFilep.close();
	outputFilePw.close();

// added by anu
	std::ofstream outputFilePA("insta_powerAllocatedTSP.txt");
	std::ofstream outputFilePt("periodic_powerAllocatedTSP.txt",fstream::app);

    for (unsigned int coreCounter = 0; coreCounter < coreRows * coreColumns; coreCounter++) {

       outputFilePA << std::fixed << std::setprecision(4) <<powerBudgets.at(coreCounter)<< "\t";
       outputFilePt<< std::fixed << std::setprecision(4) << powerBudgets.at(coreCounter)<< "\t";
	}

    outputFilePA << "\n" ;
	outputFilePt << "\n";
	outputFilePA.close();
	outputFilePt.close();

	return frequencies;
}
