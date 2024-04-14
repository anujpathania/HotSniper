#include "performance_counters.h"

#include <fstream>
#include <sstream>
#include <iostream>

using namespace std;

PerformanceCounters::PerformanceCounters(std::string instPowerFileName, std::string instTemperatureFileName, std::string instCPIStackFileName)
    : instPowerFileName(instPowerFileName), instTemperatureFileName(instTemperatureFileName), instCPIStackFileName(instCPIStackFileName) {

}

/** getPowerOfComponent
    Returns the latest power consumption of a component being tracked using base.cfg. Return -1 if power value not found.
*/
double PerformanceCounters::getPowerOfComponent (string component) const {
	ifstream powerLogFile(instPowerFileName);
	string header;
	string footer;

	if (powerLogFile.good()) {
		getline(powerLogFile, header);
		getline(powerLogFile, footer);
		// cout << "The header is " << header << endl;
		// cout << "The footer is " << footer << endl;

	}

	std::istringstream issHeader(header);
	std::istringstream issFooter(footer);
	std::string token;

	while(getline(issHeader, token, '\t')) {
		std::string value;
		getline(issFooter, value, '\t');
		//cout << "The value is " << value << endl;
		if (token == component) {
			//cout << "The stod (value) is " << stod (value) << endl;
			return stod (value);
		}
	}

	return -1;
}



 /**
  * @brief get the file lines
  * 
  */
int PerformanceCounters::getPowerTraceLines() const{
	ifstream powerTracefile("PeriodicPower.log");
	int numberOfLines = 0;
	string line;
	while(getline(powerTracefile,line)) ++numberOfLines;
	return numberOfLines;

}

double PerformanceCounters::getHistoryPowerOfComponent(int buffsize,string component) const{
	ifstream powerTracefile("PeriodicPower.log");
	string header;
	string line;
	string buff[buffsize];
	int i = 0;
	getline(powerTracefile,header);
	
	while(getline(powerTracefile,line)) {
		buff[i++ % buffsize] = line;
	}
	std::string token;
	std::vector<double> cPower;

	for(auto c : buff) {
		// cout << "The c is " << c << endl;
		std::istringstream issHeader(header);
		std::istringstream issFooter(c);
		while(getline(issHeader, token, '\t')) {
			std::string value;
			getline(issFooter, value, '\t');
			if (token == component && !c.empty()) {
				// cout << "Testing ********" << endl;
				cPower.push_back(stod(value));
			}
		}
	}
	double sum = 0;
	for(auto p : cPower){
		//cout << "The p is " << p << endl;
		sum += p;
	}

	// cout << "The size of P is " << cPower.size() << endl;
	// cout << "the average power is " << sum / cPower.size() << endl;
	if(cPower.size() >= 1)
		return sum / cPower.size();
	else 
		return 0.3;
	
}

/** getPowerOfCore
 * Return the latest total power consumption of the given core. Requires "tp" (total power) to be tracked in base.cfg. Return -1 if power is not tracked.
 */
double PerformanceCounters::getHistoryPowerOfCore(int coreId) const {
	string component = "Core" + std::to_string(coreId) + "-TP";
	return getHistoryPowerOfComponent(3,component);
}


/** getPowerOfCore
 * Return the latest total power consumption of the given core. Requires "tp" (total power) to be tracked in base.cfg. Return -1 if power is not tracked.
 */
double PerformanceCounters::getPowerOfCore(int coreId) const {
	string component = "Core" + std::to_string(coreId) + "-TP";
	return getPowerOfComponent(component);
}


/** getPeakTemperature
    Returns the latest peak temperature of any component
*/
double PerformanceCounters::getPeakTemperature () const {
	ifstream temperatureLogFile(instTemperatureFileName);
	string header;
	string footer;

	if (temperatureLogFile.good()) {
		getline(temperatureLogFile, header);
		getline(temperatureLogFile, footer);
	}

	std::istringstream issFooter(footer);

	double maxTemp = -1;
	std::string value;
	while(getline(issFooter, value, '\t')) {
		double t = stod (value);
		if (t > maxTemp) {
			maxTemp = t;
		}
	}

	return maxTemp;
}


/** getTemperatureOfComponent
    Returns the latest temperature of a component being tracked using base.cfg. Return -1 if power value not found.
*/
double PerformanceCounters::getTemperatureOfComponent (string component) const {
	ifstream temperatureLogFile(instTemperatureFileName);
	string header;
	string footer;

  	if (temperatureLogFile.good()) {
		getline(temperatureLogFile, header);
		getline(temperatureLogFile, footer);
  	}

	std::istringstream issHeader(header);
	std::istringstream issFooter(footer);
	std::string token;

	while(getline(issHeader, token, '\t')) {
		std::string value;
		getline(issFooter, value, '\t');

		if (token == component) {
			return stod (value);
		}
	}

	return -1;
}

/** getTemperatureOfCore
 * Return the latest temperature of the given core. Requires "tp" (total power) to be tracked in base.cfg. Return -1 if power is not tracked.
 */
double PerformanceCounters::getTemperatureOfCore(int coreId) const {
	string component = "Core" + std::to_string(coreId) + "-TP";
	return getTemperatureOfComponent(component);
}

/**
 * Get a performance metric for the given core.
 * Available performance metrics can be checked in InstantaneousPerformanceCounters.log
 */
double PerformanceCounters::getCPIStackPartOfCore(int coreId, std::string metric) const {
	ifstream cpiStackLogFile(instCPIStackFileName);
    string line;
	std::istringstream issLine;

	// first find the line in the logfile that contains the desired metric
	bool metricFound = false;
	while (!metricFound) {
  		if (cpiStackLogFile.good()) {
			getline(cpiStackLogFile, line);
			issLine.str(line);
			issLine.clear();
			std::string m;
			getline(issLine, m, '\t');
			metricFound = (m == metric);
		} else {
			return -1;
		}
	}
	
	// then split the coreId-th value from this line (first value is metric name, but already consumed above)
	std::string value;
	for (int i = 0; i < coreId + 1; i++) {
		getline(issLine, value, '\t');
		if ((i == 0) && (value == "-")) {
			return 0;
		}
	}

	return stod(value);
}

/**
 * Get the utilization of the given core.
 */
double PerformanceCounters::getUtilizationOfCore(int coreId) const {
	return getCPIStackPartOfCore(coreId, "base") / getCPIOfCore(coreId);
}

/**
 * Get the CPI of the given core.
 */
double PerformanceCounters::getCPIOfCore(int coreId) const {
	return getCPIStackPartOfCore(coreId, "total");
}

/**
 * Get the rel. NUCA part of the CPI stack of the given core.
 */
double PerformanceCounters::getRelNUCACPIOfCore(int coreId) const {
	return getCPIStackPartOfCore(coreId, "mem-nuca") / getCPIOfCore(coreId);
}

/**
 * Get the frequency of the given core.
 */
int PerformanceCounters::getFreqOfCore(int coreId) const {
	if (coreId >= (int)frequencies.size()) {
		return -1;
	} else {
		return frequencies.at(coreId);
	}
}

/**
 * Notify new frequencies
 */
void PerformanceCounters::notifyFreqsOfCores(std::vector<int> newFrequencies) {
	frequencies = newFrequencies;
}

/**
 * Get the frequency of the given core.
 */
double PerformanceCounters::getIPSOfCore(int coreId) const {
	return 1e6 * getFreqOfCore(coreId) / getCPIOfCore(coreId);
}
