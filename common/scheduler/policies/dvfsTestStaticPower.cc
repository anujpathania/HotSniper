#include "dvfsTestStaticPower.h"
#include "powermodel.h"
#include <iomanip>
#include <iostream>

using namespace std;

DVFSTestStaticPower::DVFSTestStaticPower(const PerformanceCounters *performanceCounters, int coreRows, int coreColumns, int minFrequency, int maxFrequency)
	: performanceCounters(performanceCounters), coreRows(coreRows), coreColumns(coreColumns), minFrequency(minFrequency), maxFrequency(maxFrequency) {
	
}

std::vector<int> DVFSTestStaticPower::getFrequencies(const std::vector<int> &oldFrequencies, const std::vector<bool> &activeCores) {
	std::vector<int> frequencies(coreRows * coreColumns);

	for (unsigned int coreCounter = 0; coreCounter < coreRows * coreColumns; coreCounter++) {
		float power = performanceCounters->getPowerOfCore(coreCounter);
		int frequency = oldFrequencies.at(coreCounter);

		cout << "[Scheduler][DVFSTestStaticPower]: Core " << setw(2) << coreCounter << ":";
		cout << " P=" << fixed << setprecision(3) << power << " W";
		cout << " f=" << frequency << " MHz" << endl;

		frequencies.at(coreCounter) = (coreCounter % 2 == 0) ? minFrequency : maxFrequency;
	}

	return frequencies;
}
