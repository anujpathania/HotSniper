#include "migrateSimple.h"
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <map>
#include <set>

//Initiazation
MigrateSimple::MigrateSimple(unsigned int coreRows, unsigned int coreColumns):coreRows(coreRows), coreColumns(coreColumns){
    // for(unsigned int i = 0; i < coreRows * coreColumns;i++){

    // }
}

std::vector<migration> MigrateSimple::migrate(SubsecondTime time, const std::vector<int> &taskIds, const std::vector<bool> &activeCores){

    std::vector<bool> availableCores(coreRows * coreColumns);
    for (int c = 0; c < coreRows * coreColumns; c++) {
        availableCores.at(c) = taskIds.at(c) == -1;
    }
    std::vector<migration> migrations;
    migration tmp;
    // for(unsigned int i = 0; i < coreRows * coreColumns;i++){
    //     if(activeCores.at(i)){
    //         if(i == coreRows * coreColumns - 1){
    //             tmp.fromCore = i;
    //             tmp.toCore = 0;
    //             tmp.swap = false;
    //             migrations.push_back(tmp);
    //             //activeCores.at(i) = false;
    //             availableCores.at(i) = false;
    //         }

    //         for(unsigned int j =0; j < coreRows * coreColumns;j++){
    //             if(!activeCores.at(j) && j > i){
    //                 tmp.fromCore = i;
    //                 tmp.toCore = j;
    //                 tmp.swap = false;
    //                 migrations.push_back(tmp);
    //                 //activeCores.at(i) = false;
    //                 availableCores.at(i) = false;
    //                 break;

    //             }

    //         }
    //     }

    // }


    if(activeCores.at(0)){
        tmp.fromCore = 0;
        tmp.toCore = 56;
        tmp.swap = false;
        migrations.push_back(tmp);
        //activeCores.at(i) = false;
        availableCores.at(0) = false;

    }
    if(activeCores.at(7)){
        tmp.fromCore = 7;
        tmp.toCore = 0;
        tmp.swap = false;
        migrations.push_back(tmp);
        //activeCores.at(i) = false;
        availableCores.at(7) = false;

    }
    if(activeCores.at(63)){
        tmp.fromCore = 63;
        tmp.toCore = 7;
        tmp.swap = false;
        migrations.push_back(tmp);
        //activeCores.at(i) = false;
        availableCores.at(63) = false;

    }
    if(activeCores.at(56)){
        tmp.fromCore = 56;
        tmp.toCore = 63;
        tmp.swap = false;
        migrations.push_back(tmp);
        //activeCores.at(i) = false;
        availableCores.at(56) = false;

    }
    return migrations;
}

// std::vector<migration> MigrateSimple::migrate(
//         SubsecondTime time,
//         const std::vector<int> &taskIds,
//         const std::vector<bool> &activeCores) {

//     std::vector<migration> migrations;

//     std::vector<bool> availableCores(coreRows * coreColumns);
//     for (int c = 0; c < coreRows * coreColumns; c++) {
//         availableCores.at(c) = taskIds.at(c) == -1;
//     }

//     for (int c = 0; c < coreRows * coreColumns; c++) {
//         if (activeCores.at(c)) {
//             float temperature = performanceCounters->getTemperatureOfCore(c);
//             if (temperature > criticalTemperature) {
//                 cout << "[Scheduler][coldestCore-migrate]: core" << c << " too hot (";
//                 cout << fixed << setprecision(1) << temperature << ") -> migrate";
//                 logTemperatures(availableCores);

//                 int targetCore = getColdestCore(availableCores);

//                 if (targetCore == -1) {
//                     cout << "[Scheduler][coldestCore-migrate]: no target core found, cannot migrate" << endl;
//                 } else {
//                     migration m;
//                     m.fromCore = c;
//                     m.toCore = targetCore;
//                     m.swap = false;
//                     migrations.push_back(m);
//                     availableCores.at(targetCore) = false;
//                 }
//             }
//         }
//     }

//     return migrations;
// }