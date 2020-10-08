#include "mapFirstUnused.h"
#include <algorithm>
#include <iostream>
#include <map>
#include <set>

MapFirstUnused::MapFirstUnused(unsigned int coreRows, unsigned int coreColumns, std::vector<int> preferredCoresOrder)
	: coreRows(coreRows), coreColumns(coreColumns), preferredCoresOrder(preferredCoresOrder) {
	for (unsigned int i = 0; i < coreRows * coreColumns; i++) {
		if (std::find(this->preferredCoresOrder.begin(), this->preferredCoresOrder.end(), i) == this->preferredCoresOrder.end()) {
			this->preferredCoresOrder.push_back(i);
		}
	}
}

std::vector<int> MapFirstUnused::map(String taskName, int taskCoreRequirement, const std::vector<bool> &availableCores, const std::vector<bool> &activeCores) {
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