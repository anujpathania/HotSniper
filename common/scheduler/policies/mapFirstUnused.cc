#include "mapFirstUnused.h"
#include <algorithm>
#include <iostream>
#include <map>
#include <set>

MapFirstUnused::MapFirstUnused(unsigned int numTasks, unsigned int coreRows, unsigned int coreColumns)
	: coreRows(coreRows), coreColumns(coreColumns) {
	for (unsigned int i = 0; i < coreRows * coreColumns; i++) 
			this->preferredCoresOrder.push_back(i);
}

std::vector<int> MapFirstUnused::map(UInt32 taskID, int taskCoreRequirement, const std::vector<bool> &availableCores, const std::vector<bool> &activeCores) {
	std::vector<int> cores;

	// try to fill with preferred cores
	for (const int &c : preferredCoresOrder) {
		if (availableCores.at(c)) {
			cores.push_back(c);
			if ((int)cores.size() == taskCoreRequirement) {
				return cores;
			}
		}
	}

	std::vector<int> empty;
	return empty;
}