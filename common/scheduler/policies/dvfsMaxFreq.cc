#include "dvfsMaxFreq.h"
#include <iomanip>
#include <iostream>

using namespace std;

DVFSMaxFreq::DVFSMaxFreq(const PerformanceCounters *performanceCounters, int coreRows, int coreColumns, int maxFrequency)
	: performanceCounters(performanceCounters), coreRows(coreRows), coreColumns(coreColumns), maxFrequency(maxFrequency) {
	
}

std::vector<int> DVFSMaxFreq::getFrequencies(const std::vector<int> &oldFrequencies, const std::vector<bool> &activeCores) {
	std::vector<int> frequencies(coreRows * coreColumns);

	for (unsigned int coreCounter = 0; coreCounter < coreRows * coreColumns; coreCounter++) {
		if (activeCores.at(coreCounter)) {
			float power = performanceCounters->getPowerOfCore(coreCounter);
			float temperature = performanceCounters->getTemperatureOfCore(coreCounter);
			int frequency = oldFrequencies.at(coreCounter);
			float utilization = performanceCounters->getUtilizationOfCore(coreCounter);

			cout << "[Scheduler][DVFS_MAX_FREQ]: Core " << setw(2) << coreCounter << ":";
			cout << " P=" << fixed << setprecision(3) << power << " W";
			cout << " f=" << frequency << " MHz";
			cout << " T=" << fixed << setprecision(1) << temperature << " °C";
			cout << " utilization=" << fixed << setprecision(3) << utilization << endl;
		}
		frequencies.at(coreCounter) = maxFrequency;
	}

	return frequencies;
}