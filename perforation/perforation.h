#ifndef PERFORATION_H
#define PERFORATION_H

#include "sim_api.h"

int get_perforation_rate(int type);

int* perf_vector(int n, int pr);

int get_k(int n, int pr);

int* perf_vector_truncate(int n, int pr);

float get_mf(int n, int pr);

void free_vec(int* vec);

void print_vec(int* vec, int n, int pr);

#endif /* PERFORATION_H */