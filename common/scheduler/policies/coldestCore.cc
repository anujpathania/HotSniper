#include "coldestCore.h"

#include <iomanip>

using namespace std;

ColdestCore::ColdestCore(
        const PerformanceCounters *performanceCounters,
        int coreRows,
        int coreColumns,
        float criticalTemperature)
    : performanceCounters(performanceCounters),
      coreRows(coreRows),
      coreColumns(coreColumns),
      criticalTemperature(criticalTemperature) {

}

std::vector<int> ColdestCore::map(
        String taskName,
        int taskCoreRequirement,
        const std::vector<bool> &availableCoresRO,
        const std::vector<bool> &activeCores) {

    std::vector<bool> availableCores(availableCoresRO);

    std::vector<int> cores;

    //logTemperatures(availableCores);

    for (; taskCoreRequirement > 0; taskCoreRequirement--) {
        //int coldestCore = getColdestCore(availableCores);
        int coldestCore = getPeriodicCore(availableCores);
        cout << "[checking]Selectedc core is " << coldestCore << endl;

        if (coldestCore == -1) {
            // not enough free cores
            std::vector<int> empty;
            return empty;
        } else {
            cores.push_back(coldestCore);
            availableCores.at(coldestCore) = false;
        }
    }

    return cores;
}

std::vector<migration> ColdestCore::migrate(
        SubsecondTime time,
        const std::vector<int> &taskIds,
        const std::vector<bool> &activeCores) {

    std::vector<migration> migrations;

    std::vector<bool> availableCores(coreRows * coreColumns);
    for (int c = 0; c < coreRows * coreColumns; c++) {
        availableCores.at(c) = taskIds.at(c) == -1;
    }

    for (int c = 0; c < coreRows * coreColumns; c++) {
        // if (activeCores.at(c)) {
        //     float temperature = performanceCounters->getTemperatureOfCore(c);
        //     if (temperature > criticalTemperature) {
        //         cout << "[Scheduler][coldestCore-migrate]: core" << c << " too hot (";
        //         cout << fixed << setprecision(1) << temperature << ") -> migrate";
        //         logTemperatures(availableCores);

        //         int targetCore = getColdestCore(availableCores);

        //         if (targetCore == -1) {
        //             cout << "[Scheduler][coldestCore-migrate]: no target core found, cannot migrate" << endl;
        //         } else {
        //             migration m;
        //             m.fromCore = c;
        //             m.toCore = targetCore;
        //             m.swap = false;
        //             migrations.push_back(m);
        //             availableCores.at(targetCore) = false;
        //         }
        //     }
        // }
        if (activeCores.at(c)) {
            float temperature = performanceCounters->getTemperatureOfCore(c);
            if (temperature > criticalTemperature  || 1) {
                cout << "[Scheduler][coldestCore-migrate]: core" << c << " too hot (";
                cout << fixed << setprecision(1) << temperature << ") -> migrate";
                //logTemperatures(availableCores);

                //int targetCore = getColdestCore(availableCores);
                int targetCore = getPeriodicCore(activeCores);

                if (targetCore == -1) {
                    cout << "[Scheduler][coldestCore-migrate]: no target core found, cannot migrate" << endl;
                } else {
                    migration m;
                    m.fromCore = c;
                    m.toCore = targetCore;
                    m.swap = false;
                    migrations.push_back(m);
                    availableCores.at(targetCore) = false;
                }
            }
        }
    }
    return migrations;
}

int ColdestCore::getColdestCore(const std::vector<bool> &availableCores) {
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

int ColdestCore::getPeriodicCore(const std::vector<bool> &activeCores){
    int periodicCore = 0;
    for (int c = 0; c < coreRows * coreColumns; c++) {  
        if (activeCores.at(c)) {
            if(c == 0) periodicCore = 56;
            else if (c == 56) periodicCore = 63;
            else if (c == 63) periodicCore = 7;
            else if (c == 7) periodicCore = 0;
        }
    }
    // for (int c = 0; c < coreRows * coreColumns; c++) {  
    //     if (activeCores.at(c)) {
    //         if(c == 27) periodicCore = 28;
    //         else if (c == 28) periodicCore = 36;
    //         else if (c == 36) periodicCore = 35;
    //         else if (c == 35) periodicCore = 27;
    //     }
    // }
    return periodicCore;

    
}

// int ColdestCore::getNextCore(const std::vector<bool> &activeCores){


// }

void ColdestCore::logTemperatures(const std::vector<bool> &availableCores) {
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