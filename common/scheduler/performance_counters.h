/**
 * performancecounters
 * This header implements the static class PerformanceCounters
 */

#ifndef __PERFORMANCECOUNTERS_H
#define __PERFORMANCECOUNTERS_H

#include <string>
#include <vector>

class PerformanceCounters {
public:
    PerformanceCounters(const char* output_dir, std::string instPowerFileNameParam, std::string instTemperatureFileNameParam, std::string instCPIStackFileNameParam);
    double getPowerOfComponent (std::string component) const;
    double getPowerOfCore(int coreId) const;
    double getPeakTemperature () const;
    double getTemperatureOfComponent (std::string component) const;
    double getTemperatureOfCore (int coreId) const;
    double getCPIStackPartOfCore(int coreId, std::string metric) const;
    double getUtilizationOfCore(int coreId) const;
    double getCPIOfCore(int coreId) const;
    double getRelNUCACPIOfCore(int coreId) const;
    int getFreqOfCore(int coreId) const;
    double getIPSOfCore(int coreId) const;

    void notifyFreqsOfCores(std::vector<int> frequencies);
private:
    std::vector<int> frequencies;

    std::string instPowerFileName;
    std::string instTemperatureFileName;
    std::string instCPIStackFileName;
};

#endif
