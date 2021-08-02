#ifndef __MIGRATIONPOLICY_H
#define __MIGRATIONPOLICY_H

#include "fixed_types.h"
#include <vector>
#include <iostream>
#include <fstream>
#include <cmath>

using namespace std;

class MigrationPolicy {
public:
    MigrationPolicy(){ m_curr_shared_slots = 0; }
    virtual ~MigrationPolicy() {}
    virtual core_id_t getMigrationCandidate(core_id_t currentCore, const std::vector<bool> &availableCores, const std::vector<bool> &freeTiles) = 0;
    void setCurrentSharedSlots(int sharedSlots){ m_curr_shared_slots = sharedSlots; }
    void setCurrentPerformance(double performance){ m_curr_performance = performance; }
    int getCurrentSharedSlots() const { return m_curr_shared_slots; }
    double getCurrentPerformance() const { return m_curr_performance; }

    bool evaluateMigrationFunction() {
        cout << "Performance " <<m_curr_performance<< " Slots: "<<m_curr_shared_slots<<endl;
        double migrationFunction =   m_alpha*(1 - (pow((m_curr_performance/m_max_performance),2.0))) + ((double)m_curr_shared_slots/m_max_slots);
        cout << "Migrationfunction = "<< migrationFunction<<endl;
    	ofstream myfile;
	    myfile.open ("Migration.log", ios::app);
        myfile <<m_curr_shared_slots<<"\t"<<m_curr_performance<<"\t"<<migrationFunction<<"\n";
        myfile.close();
        return  (migrationFunction >= 1);
    }
    void setMigrationFunction(float alpha, float beta, UInt32 max_slots, float max_performance){
        m_alpha = alpha;
        m_beta = beta;
        m_max_slots = max_slots;
        m_max_performance = max_performance;
    }

private:
    float m_alpha, m_beta,  m_max_performance;
	UInt32 m_max_slots;
    int m_curr_shared_slots;
    double m_curr_performance;
    

};

#endif