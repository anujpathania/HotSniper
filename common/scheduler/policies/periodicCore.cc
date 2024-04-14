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
    std::cout << "The taskCoreRequirement is " << taskCoreRequirement << std::endl;
    for (; taskCoreRequirement > 0; taskCoreRequirement--) {
        for (int i = 0; i < coreRows * coreColumns;i++){
            if(availableCores.at(i)){    
                int selectedCore = getPeriodicCore(activeCores);        
                // int selectedCore = i;
                cores.push_back(selectedCore);
                availableCores.at(i) = false;
                cout << "[checking]Selected core is " << selectedCore << endl;
                //availableCores.at((i + 1 + (coreRows * coreColumns)) % coreRows * coreColumns) = false;
            }
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

    // int temp[4] = {5,6,9,10};
    // //for(auto i : activeCores) std::cout << "&&&The active core is " << i << std::endl;
    // for(int i = 1;i < 4;i++){
    //     if (activeCores.at(temp[i])) {
    //         migration m;
    //         //std::cout << "*****This is a test **********" << std::endl;
    //         m.fromCore = temp[i];
    //         m.toCore = temp[0];
    //         if(availableCores.at(temp[0])) m.swap = false;
    //         else m.swap = true;
    //         migrations.push_back(m);
    //         if(m.swap){
    //             availableCores.at(temp[0]) = false;
    //             availableCores.at(temp[i]) = false;
    //         }else{
    //             availableCores.at(temp[i]) = true;
    //             availableCores.at(temp[0]) = false;
    //         }
    //     } 
    //     else{
    //         if(availableCores.at(temp[0]) || !activeCores.at(temp[0]) ) continue;
    //         else{
    //             migration m;
    //             m.fromCore = temp[0];
    //             m.toCore = temp[i];
    //             // std::cout << "availableCores.at(temp[0]) " << availableCores.at(temp[0]) << std::endl;
    //             // std::cout << "active.at(temp[0]) " << activeCores.at(temp[0]) << std::endl;
    //             // std::cout << "m.fromCore* is " << m.fromCore << std::endl;
    //             // std::cout << "m.toCore* is " << m.toCore << std::endl;
    //             m.swap = (taskIds.at(m.toCore) != -1);
                
    //             migrations.push_back(m);
    //             availableCores.at(temp[i]) = false;
    //             availableCores.at(temp[0]) = true;
    //             }                
    //         }
    // }
    int countCore = 0;

    for(int c =0; c < coreRows * coreColumns;c++){
        if(activeCores.at(c)) countCore++;
    }
    migration m; 
    migration m1;
    int targetCore;
    if(countCore == 1){
        if(activeCores.at(5)){
            targetCore = 6;
            m.fromCore = 5;
            m.toCore = targetCore;
            m.swap = false;
            availableCores.at(m.toCore) = false;
            availableCores.at(m.fromCore) = true;
            migrations.push_back(m);
        }else if(activeCores.at(6)){
            targetCore = 5;
            m.fromCore = 6;
            m.toCore = targetCore;
            m.swap = false;
            availableCores.at(m.toCore) = false;
            availableCores.at(m.fromCore) = true;
            migrations.push_back(m);
        }else if(activeCores.at(10)){
            targetCore = 9;
            m.fromCore = 10;
            m.toCore = targetCore;
            m.swap = false;
            availableCores.at(m.toCore) = false;
            availableCores.at(m.fromCore) = true;
            migrations.push_back(m);
        }else if(activeCores.at(9)){
            targetCore = 10;
            m.fromCore = 9;
            m.toCore = targetCore;
            m.swap = false;
            availableCores.at(m.toCore) = false;
            availableCores.at(m.fromCore) = true;
            migrations.push_back(m);
        }
        cout << "The size of migrations is " << migrations.size() << endl;
        return migrations;
            
    }else if(countCore == 2){
        if(activeCores.at(6) && activeCores.at(10)){
            targetCore = 9;
            m.fromCore = 10;
            m.toCore = targetCore;
            m.swap = false;
            availableCores.at(m.toCore) = false;
            availableCores.at(m.fromCore) = true;
            migrations.push_back(m);
        } else if(activeCores.at(6) && activeCores.at(9)){
            targetCore = 5;
            m.fromCore = 9;
            m.toCore = targetCore;
            m.swap = false;
            availableCores.at(m.toCore) = false;
            availableCores.at(m.fromCore) = true;
            migrations.push_back(m);
            targetCore = 10;
            m1.fromCore = 6;
            m1.toCore = targetCore;
            m1.swap = false;
            availableCores.at(m1.toCore) = false;
            availableCores.at(m1.fromCore) = true;
            migrations.push_back(m1);
        } else if(activeCores.at(5) && activeCores.at(10)){
            targetCore = 9;
            m.fromCore = 10;
            m.toCore = targetCore;
            m.swap = false;
            availableCores.at(m.toCore) = false;
            availableCores.at(m.fromCore) = true;
            migrations.push_back(m);
            targetCore = 6;
            m1.fromCore = 5;
            m1.toCore = targetCore;
            m1.swap = false;
            availableCores.at(m1.toCore) = false;
            availableCores.at(m1.fromCore) = true;
            migrations.push_back(m1);

    }
    
    cout << "The size of migrations is " << migrations.size() << endl;
    return migrations;
}
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

// int PeriodicCore::getPeriodicCore(const std::vector<bool> &activeCores){
//     int periodicCore = 0;
//     for (int c = 0; c < coreRows * coreColumns; c++) {  
//         if (activeCores.at(c)) {
//             if(c == 5) return 6;
//             else if (c == 6) return 5;
//             else if (c == 10) return 9;
//             else if (c == 9) return 10;
//         }
//     }
//     return 0;

    
// }

int PeriodicCore::getPeriodicCore(const std::vector<bool> &activeCores){
    int periodicCore = 0;
    for (int c = 0; c < coreRows * coreColumns; c++) {  
        if (activeCores.at(c)) {
            if(c == 5) return 6;
            else if (c == 6) return 10;
            else if (c == 10) return 9;
            else if (c == 9) return 5;
        }
    }
    return 0;

    
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