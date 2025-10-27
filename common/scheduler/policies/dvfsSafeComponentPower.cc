#include "dvfsSafeComponentPower.h"
#include "powermodel.h"
#include <iomanip>
#include <iostream>

using namespace std;

DVFSSafeComponentPower::DVFSSafeComponentPower(const PerformanceCounters *performanceCounters, int coreRows, int coreColumns, int minFrequency, int maxFrequency, double maxTemp, String floorplanFileName, String safeComponentPowerFile)
	: performanceCounters(performanceCounters), coreRows(coreRows), coreColumns(coreColumns), minFrequency(minFrequency), maxFrequency(maxFrequency), maxTemp(maxTemp) {
	
	this->components = readComponents(std::string(floorplanFileName.c_str()));
}

std::vector<int> DVFSSafeComponentPower::getFrequencies(const std::vector<int> &oldFrequencies, const std::vector<bool> &activeCores) {
	for (int i = 0; i < this->components.size(); i++) {
		cout << "Component temperature: " << this->components.at(i) << " = " << performanceCounters->getTemperatureOfComponent(this->components.at(i)) << endl;
		cout << "Component power      : " << this->components.at(i) << " = " << performanceCounters->getPowerOfComponent(this->components.at(i)) << endl;
	}
	
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

std::vector<std::string> DVFSSafeComponentPower::readComponents(const std::string &floorplanFilename) {
	if(floorplanFilename.size() <= 0) {
        return std::vector<std::string>();
    }

	std::ifstream inputFile(floorplanFilename.c_str());
	if (!inputFile.is_open() || !inputFile.good()) {
		return std::vector<std::string>();
    }


	std::vector<std::string> components;

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
			components.push_back(name);
		} catch(...){
			std::cout << "Error: File with the floorplan is invalid." << std::endl;
			inputFile.close();
			return std::vector<std::string>();
		}
	}

    inputFile.close();

	return components;
}
