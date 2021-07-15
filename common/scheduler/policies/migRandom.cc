#include "migRandom.h"
#include "scheduler_open.h"
#include <random>
#include <iostream>
using namespace std;

MigRandom::MigRandom(std::mt19937 gen){
    m_gen = gen;
}


core_id_t MigRandom::getMigrationCandidate(core_id_t currentCore, const std::vector<bool> &availableCores) 
{
	int cores = Sim()->getConfig()->getApplicationCores();
    bool found = false;
    core_id_t candidate = currentCore;
    std::uniform_int_distribution<> distrib(0, cores-1);

	while (!found){
        candidate = distrib(m_gen);
        cout <<"Candidate = "<<candidate<<endl;
		if (availableCores.at(candidate))
				found = true;
    }
	return candidate;
}