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
            int selectedCore = (i + 1 + (coreRows * coreColumns)) % (coreRows * coreColumns);        
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
    

    // for (int c = 1; c < coreRows * coreColumns; c++) {
    //     float temperature = performanceCounters->getTemperatureOfCore(c);
    //     if (activeCores.at(c)) {
    //         //float temperature = performanceCounters->getTemperatureOfCore(c);
    //         //if (temperature > criticalTemperature  || 1) {
    //         cout << "[Scheduler][periodic-migrate]: core" << c << " too hot (";
    //         cout << fixed << setprecision(1) << temperature << ") -> migrate" << endl;
    //         migration m;
    //         m.fromCore = c;
    //         m.toCore = 0;
    //         if(availableCores.at(0)) m.swap = false;
    //         else m.swap = true;
    //         migrations.push_back(m);
    //         if(m.swap){
    //             availableCores.at(0) = false;
    //             availableCores.at(c) = false;
    //         }else{
    //             availableCores.at(c) = true;
    //             availableCores.at(0) = false;
    //         }
    //     } else{
    //         if(availableCores.at(0)) continue;
    //         else{
    //             cout << "[Scheduler][periodic-migrate1]: core" << c << " too hot (";
    //             cout << fixed << setprecision(1) << temperature << ") -> migrate" << endl;
    //             migration m;
    //             m.fromCore = 0;
    //             m.toCore = c;
    //             m.swap = false;
    //             migrations.push_back(m);
    //             availableCores.at(c) = false;
    //             availableCores.at(0) = true;
    //             }                
    //         }
    //     }
    // for (int c = 0; c < coreRows * coreColumns; c++){

    // }

    // if(activeCores.at(1) && availableCores.at(2)){
    //     //cout << "Core 1 test <<<< " << endl;
    //         migration m;
    //         m.fromCore = 1;
    //         m.toCore = 2;
    //         m.swap= false;
    //         availableCores.at(1) = true;
    //         availableCores.at(2) = false;
    //         migrations.push_back(m);
    // } else{
    //         if(activeCores.at(2)){
    //             migration m;
    //             m.fromCore = 2;
    //             m.toCore = 7;
    //             m.swap= false;
    //             availableCores.at(2) = true;
    //             availableCores.at(7) = false;
    //             migrations.push_back(m);
    //         }
    // }
    // if(activeCores.at(7) && availableCores.at(11)){
    //         migration m;
    //         m.fromCore = 7;
    //         m.toCore = 11;
    //         m.swap= false;
    //         availableCores.at(7) = true;
    //         availableCores.at(11) = false;
    //         migrations.push_back(m);
    // } else {
    //     if(activeCores.at(11)){
    //         migration m;
    //         m.fromCore = 11;
    //         m.toCore = 14;
    //         m.swap= false;
    //         availableCores.at(11) = true;
    //         availableCores.at(14) = false;
    //         migrations.push_back(m);
    //     }
    // }
    // if(activeCores.at(14) && availableCores.at(13)){
    //         migration m;
    //         m.fromCore = 14;
    //         m.toCore = 13;
    //         availableCores.at(14) = true;
    //         availableCores.at(13) = false;
    //         migrations.push_back(m);
    // }else{
    //         if(activeCores.at(13)){
    //             migration m;
    //             m.fromCore = 13;
    //             m.toCore = 8;
    //             availableCores.at(11) = true;
    //             availableCores.at(14) = false;
    //             migrations.push_back(m);

    //         }
            
    //         }
    // if(activeCores.at(8) && availableCores.at(4)){
    //         migration m;
    //         m.fromCore = 8;
    //         m.toCore = 4;
    //         availableCores.at(8) = true;
    //         availableCores.at(4) = false;
    //         migrations.push_back(m);
    // }else{
    //         if(activeCores.at(4)){
    //             migration m;
    //             m.fromCore = 4;
    //             m.toCore = 1;
    //             availableCores.at(3) = true;
    //             availableCores.at(1) = false;
    //             migrations.push_back(m);
    //         }
            
    // }
    int init[8] = {1,2,7,11,14,13,8,4};
    for(int i = 0;i < 8;i+=2){
        if(activeCores.at(init[i%8]) && availableCores.at(init[(i+1)%8])){
            migration m;
            m.fromCore = init[i%8];
            m.toCore = init[(i+1)%8];
            m.swap = false;
            migrations.push_back(m);
        }else if(availableCores.at(init[i%8]) && availableCores.at(init[(i+1)%8])){
            continue;
        }else{
            if(activeCores.at(init[(i+1)%8]) && availableCores.at(init[(i+2)%8])){
                migration m;
                m.fromCore = init[(i+1)%8];
                m.toCore = init[(i+2)%8];
                m.swap = false;
                migrations.push_back(m);
            }
        }
        
    }


    int temp[4] = {5,6,9,10};
    for(int i = 1;i < 4;i++){
        if (activeCores.at(temp[i])) {
            migration m;
            m.fromCore = temp[i];
            m.toCore = temp[0];
            if(availableCores.at(temp[0])) m.swap = false;
            else m.swap = true;
            migrations.push_back(m);
            if(m.swap){
                availableCores.at(temp[0]) = false;
                availableCores.at(temp[i]) = false;
            }else{
                availableCores.at(temp[i]) = true;
                availableCores.at(temp[0]) = false;
            }
        } 
        else{
            if(availableCores.at(temp[0])) continue;
            else{
                migration m;
                m.fromCore = temp[0];
                m.toCore = temp[i];
                m.swap = false;
                migrations.push_back(m);
                availableCores.at(temp[i]) = false;
                availableCores.at(temp[0]) = true;
                }                
            }
    }
    // if(activeCores.at(5) && activeCores.at(6) && activeCores.at(9) && activeCores.at(10)){
    //     //cout << "This is a small test" << endl;
    //     migration m1;
    //     m1.fromCore = 5;
    //     m1.toCore = 6;
    //     m1.swap= true;
    //     migrations.push_back(m1);
    //     migration m2;
    //     m2.fromCore = 5;
    //     m2.toCore = 9;
    //     m2.swap= true;
    //     migrations.push_back(m2);
    //     migration m3;
    //     m3.fromCore = 5;
    //     m3.toCore = 10;
    //     m3.swap= true;
    //     migrations.push_back(m3);
    // } else{
        
    // }


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