#ifndef PERFORATION_H
#define PERFORATION_H

#include "sim_api.h"

typedef struct entry {
    int key;
    char value[50];
} kv_map;

void init_perforation(); 
int to_application_code(char* name);

void update_perforation_rates();

int set_sim_app(int app_id, int app_code);
int fetch_perforation_rate(int type);

int get_loop_rate(int loop_id);

int* perf_vector(int n, int pr);
int* perf_vector_truncate(int n, int pr);
void free_vec(int* vec);
void print_vec(int* vec, int n, int pr);

int get_k(int n, int pr);
float get_mf(int n, int pr);

#endif /* PERFORATION_H */