#ifndef __MIGRATE_SIMPLE_H
#define __MIGRATE_SIMPLE_H

#include <vector>
#include "migrationpolicy.h"

class MigrateSimple : public MigrationPolicy{
public:
    MigrateSimple(unsigned int coreRows, unsigned int coreColumns);
    std::vector<migration> migrate(SubsecondTime time, const std::vector<int> &taskIds, const std::vector<bool> &activeCores);

private:
    unsigned int coreRows;
    unsigned int coreColumns;


};

#endif