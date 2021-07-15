/**
 * This header implements the Random migration policy
 */

#ifndef __MIG_RANDOM_H
#define __MIG_RANDOM_H

#include "migrationPolicy.h"
#include <random>

class MigRandom : public MigrationPolicy {
public:
    MigRandom(std::mt19937 gen);
    virtual core_id_t getMigrationCandidate(core_id_t currentCore, const std::vector<bool> &availableCores);
private:
    std::mt19937 m_gen;
};

#endif
