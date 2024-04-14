#include "periodicCore.h"

#include <iomanip>

using namespace std;

PeriodicCore::PeriodicCore(
        const PerformanceCounters *performanceCounters,
        int coreRows,
        int coreColumns,
        float criticalTemperature)
    : performanceCounters(performanceCounters),
      coreRows(coreRows),
      coreColumns(coreColumns),
      criticalTemperature(criticalTemperature) {

}

std::vector<int> PeriodicCore::map(
        String taskName,
        int taskCoreRequirement,
        const std::vector<bool> &availableCoresRO,
        const std::vector<bool> &activeCores) {

    std::vector<bool> availableCores(availableCoresRO);

    std::vector<int> cores;

    //logTemperatures(availableCores);
    for (int i = 0; i < coreRows * coreColumns;i++){
        if(activeCores.at(i)){    
            int selectedCore = getPeriodicCore(activeCores);        
            cores.push_back(selectedCore);
            cout << "[checking]Selected core is " << selectedCore << endl;

            //availableCores.at((i + 1 + (coreRows * coreColumns)) % coreRows * coreColumns) = false;
        }
    }

    // for (; taskCoreRequirement > 0; taskCoreRequirement--) {
    //     //int coldestCore = getColdestCore(availableCores);
    //     int coldestCore = getPeriodicCore(availableCores);
    //     if (coldestCore == -1) {
    //         // not enough free cores
    //         std::vector<int> empty;
    //         return empty;
    //     } else {
    //         cores.push_back(coldestCore);
    //         availableCores.at(coldestCore) = false;
    //     }
    // }

    return cores;
}

std::vector<migration> PeriodicCore::migrate(
        SubsecondTime time,
        const std::vector<int> &taskIds,
        const std::vector<bool> &activeCores) {

    std::vector<migration> migrations;

    std::vector<bool> availableCores(coreRows * coreColumns);
    for (int c = 0; c < coreRows * coreColumns; c++) {
        availableCores.at(c) = taskIds.at(c) == -1;
    }

    for(int c =0; c < coreRows * coreColumns;c++){
        if(activeCores.at(c)){
            int targetCore = getPeriodicCore(activeCores);
            migration m; 
            m.fromCore = c;
            m.toCore = targetCore;
            m.swap = false;
            availableCores.at(targetCore) = false;
            migrations.push_back(m);
        }

    }
    cout << "The size of migrations is " << migrations.size() << endl;
    return migrations;
}

int PeriodicCore::getColdestCore(const std::vector<bool> &availableCores) {
    int coldestCore = -1;
    float coldestTemperature = 0;
    //iterate all cores toCore find coldest
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

int PeriodicCore::getPeriodicCore(const std::vector<bool> &activeCores){
    int periodicCore = 0;
    for (int c = 0; c < coreRows * coreColumns; c++) {  
        if (activeCores.at(c)) {
            if(c == 5) periodicCore = 6;
            else if (c == 6) periodicCore = 10;
            else if (c == 10) periodicCore = 9;
            else if (c == 9) periodicCore = 5;
        }
    }
    return periodicCore;

    
}


void PeriodicCore::logTemperatures(const std::vector<bool> &availableCores) {
    cout << "[Scheduler][periodic-map]: temperatures of available cores:" << endl;
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