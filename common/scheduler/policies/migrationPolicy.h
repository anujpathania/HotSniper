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
    MigrationPolicy(){ m_curr_shared_slots = 0; m_migra_function = 0; }
    virtual ~MigrationPolicy() {}
    virtual core_id_t getMigrationCandidate(core_id_t currentCore, const std::vector<bool> &availableCores, const std::vector<bool> &freeTiles) = 0;
    void setCurrentSharedSlots(int sharedSlots){ m_curr_shared_slots = sharedSlots; }
    void setCurrentPerformance(double performance){ m_curr_performance = performance; }
    int getCurrentSharedSlots() const { return m_curr_shared_slots; }
    double getCurrentPerformance() const { return m_curr_performance; }
   
   
    double getMigrationFunction() { 
        cout << "Performance " <<m_curr_performance<< " Slots: "<<m_curr_shared_slots<<endl;
        //double m_migra_function =   m_alpha*(1 - (m_curr_performance/m_max_performance)) + ((double)m_curr_shared_slots/m_max_slots);
        m_migra_function =   m_alpha*(m_max_performance - m_curr_performance ) + ((double)m_curr_shared_slots/m_max_slots);
        cout << "Migrationfunction = "<< m_migra_function<<endl;
        return m_migra_function; 
        }

    bool evaluateMigrationFunction() {
        return  ((m_migra_function >= 1) && (m_curr_shared_slots > 0));
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
    double m_migra_function;
};

#endif