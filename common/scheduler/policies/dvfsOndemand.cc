#include "dvfsOndemand.h"
#include <iomanip>
#include <iostream>
#include <fstream>


using namespace std;
DVFSOndemand::DVFSOndemand(
	const PerformanceCounters *performanceCounters,
	int coreRows,
	int coreColumns,
	int minFrequency,
	int maxFrequency,
	int frequencyStepSize,
	float upThreshold,
	float downThreshold,
	float dtmCriticalTemperature,
	float dtmRecoveredTemperature)
	: performanceCounters(performanceCounters),
	coreRows(coreRows),
	coreColumns(coreColumns),
	minFrequency(minFrequency),
	maxFrequency(maxFrequency),
	frequencyStepSize(frequencyStepSize),
	upThreshold(upThreshold),
	downThreshold(downThreshold),
	dtmCriticalTemperature(dtmCriticalTemperature),
	dtmRecoveredTemperature(dtmRecoveredTemperature) { 
	}
	
std::vector<int> DVFSOndemand::getFrequencies(
const std::vector<int> &oldFrequencies,
const std::vector<bool> &activeCores) {
	if (throttle()) {
		std::vector<int> minFrequencies(coreRows * coreColumns, minFrequency);
		cout << "[Scheduler][ondemand-DTM]: in throttle mode -> return min.frequencies" << endl;
		return minFrequencies;
		}
	else {
		std::vector<int> frequencies(coreRows * coreColumns);
		for (unsigned int coreCounter = 0; coreCounter < coreRows * coreColumns; coreCounter++) {
			if (activeCores.at(coreCounter)) {
				float power = performanceCounters->getPowerOfCore(coreCounter);
				float temperature = performanceCounters->getTemperatureOfCore(coreCounter);
				int frequency = oldFrequencies.at(coreCounter);
				float utilization = performanceCounters->getUtilizationOfCore(coreCounter);

				cout << "[Scheduler][ondemand]: Core " << setw(2) << coreCounter << ":";
				cout << " P=" << fixed << setprecision(3) << power << " W";
				cout << " f=" << frequency << " MHz";
				cout << " T=" << fixed << setprecision(1) << temperature << " C";
				// avoid the little circle symbol, it is not ASCII
				cout << " utilization=" << fixed << setprecision(3) << utilization ;

                float RelNUCACPI  = performanceCounters->getRelNUCACPIOfCore(coreCounter);
				float IPS         = performanceCounters->getIPSOfCore(coreCounter);
				float cpi_total   = performanceCounters->getCPIOfCore(coreCounter);
				float peak_temperature = performanceCounters->getPeakTemperature();
				
				cout << " IPS=" << fixed << setprecision(3) << IPS ;
				cout << " RelNUCACPI=" << fixed << setprecision(3) << RelNUCACPI ;
				cout << " hotspot_peak_temperature=" << fixed << setprecision(3) << peak_temperature ;
				cout << " cpi-total=" << fixed << setprecision(3) <<cpi_total << endl;


				// use same period for upscaling and downscaling as described
				// in "The ondemand governor."
				
				if (utilization > upThreshold) {
					cout << "[Scheduler][ondemand]: utilization > upThreshold";
					if (frequency == maxFrequency) {
						cout << " but already at max frequency" << endl;
					} else {
						cout << " -> go to max frequency" << endl;
						frequency = maxFrequency;
						}
				} else if (utilization < downThreshold) {
					cout << "[Scheduler][ondemand]: utilization < downThreshold";
						if (frequency == minFrequency) {
							cout << " but already at min frequency" << endl;
						} else {
							cout << " -> lower frequency" << endl;
							frequency = frequency * 80 / 100;
							frequency = (frequency / frequencyStepSize) * frequencyStepSize; // round
							if (frequency < minFrequency) {
								frequency = minFrequency;
							}
						}
				}
				frequencies.at(coreCounter) = frequency;
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

return frequencies;
}
}


bool DVFSOndemand::throttle() {
	if (performanceCounters->getPeakTemperature() > dtmCriticalTemperature) {
		if (!in_throttle_mode) {
		cout << "[Scheduler][ondemand-DTM]: detected thermal violation" << endl;
		}
	in_throttle_mode = true;
	} else if (performanceCounters->getPeakTemperature() < dtmRecoveredTemperature) {
		if (in_throttle_mode) {
			cout << "[Scheduler][ondemand-DTM]: thermal violation ended" << endl;
		}
	in_throttle_mode = false;
	}
return in_throttle_mode;
}
