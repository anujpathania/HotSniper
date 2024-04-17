#include "perforation.h"

#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>


#define BUFF_LEN   512
#define LOOP_COUNT  32

#define USER_GET_PERFORATION_RATE       0x125
#define USER_SET_PERFORATION_APP        0x126

kv_map app_code_map[] = {
    {0, "BLACKSCHOLES"},
    {1, "BODYTRACK"},
    {2, "CANNEAL"},
    {3, "STREAMCLUSTER"},
    {4, "SWAPTIONS"},
    {5, "X264"}
};

int pr[LOOP_COUNT];

int app_id = -1;
int app_code = -1;

int to_application_code(char* name) {
    int code = -1;

    // 5 is a magic number ...
    for(int i = 0; i <= 5; i++) {
        if(!strncmp(name, app_code_map[i].value, strlen(app_code_map[i].value))){
            code = app_code_map[i].key;
        }
    }
    
    return code; 
}

void init_perforation() 
{
    for(int i = 0; i < LOOP_COUNT; i++){
        pr[i] = 0;
    }

    char id_str[BUFF_LEN], app_name[BUFF_LEN];
    memset(id_str, 0, BUFF_LEN * sizeof(char));
    memset(app_name, 0, BUFF_LEN * sizeof(char));

    strncpy(id_str, getenv("SNIPER_ID"), sizeof(id_str) - 1);
    id_str[sizeof(id_str) - 1] = '\0';
    app_id = strtol(id_str, NULL, 10);

    strncpy(app_name, getenv("SNIPER_APP_NAME"), sizeof(app_name) - 1);
    app_name[BUFF_LEN - 1] = '\0';
    app_code = to_application_code(app_name);

     int ret = set_sim_app(app_id, app_code);
    printf("recieved app id: %d from (%d = %s). (notify = %d)\n", app_id, app_code, app_name, ret);    
}

int set_sim_app(int app_id, int app_code) {
    unsigned int kv = 0;
    kv |= app_id;
    kv |= (app_code << 16);

    return SimUser(USER_SET_PERFORATION_APP, kv);
}

int get_loop_rate(int loop_id)
{
    return pr[loop_id];
}

void update_perforation_rates() {
    for(int i = 0; i < LOOP_COUNT; i++) {
        pr[i] = fetch_perforation_rate(i);
    }
}

int fetch_perforation_rate(int loop_id)
{
    assert(app_id >= 0); // sanity check.

    // add app id to the upper bits and the loop_id to the lower bits.
    unsigned int id = 0;
    id |= loop_id;
    id |= (app_id << 16);

    // send it on to sniper to retrieve the correct loop value. 
    return SimUser(USER_GET_PERFORATION_RATE, id);
}

int *perf_vector(int n, int pr)
{
    int *pr_vector = (int *)calloc(n, sizeof(int));

    int k = get_k(n, pr);
    int i;
    
    if(pr == 0) {
        for(i = 0; i < n; i++)
            pr_vector[i] = 1;    
    } 
    else if (pr >= 50) {
        for (i = 0; i < n; i += k)
            pr_vector[i] = 1;
    }
    else {
        for (i = 0; i < n; i++)
            if(i % k) pr_vector[i] = 1;
    }

    return pr_vector;
}

int *perf_vector_truncate(int n, int pr)
{
    int *pr_vector = (int *)calloc(n, sizeof(int));

    int i;
    if (pr == 0) {
        for(i = 0; i < n; i++)
            pr_vector[i] = 1;
    }
    else {
        int m = (int)((float) n * ((float) (100 - pr)/100.0));
        for (i = 0; i < m; i++)
            pr_vector[i] = 1;
    }

    return pr_vector;
}

void free_vec(int *vec)
{
    free(vec);
}

int get_k(int n, int pr)
{
    int k = 0;
    float m;

    if (pr == 0) return k;

    if (pr >= 50)
    {
        m = (float) ((float)n * (pr / 100.f));
        k = (int) ceil(n / (n - m));
    }
    else
    {
        m = (float) n * ((float)pr / 100.f);
        k = (int)ceil(n / m);
    }

    return k;
}

float get_mf(int n, int pr)
{
    float m = ((float)n * ((float) pr / 100.f));
    
    return ((float) n / ((float)n - m));
}

void print_vec(int *vec, int n, int pr)
{
    // printf("[");

    int j = 0;
    for (int i = 0; i < n; i++)
    {
        // printf(", %d", vec[i]);

        if (!vec[i])
            continue;
        j++;
    }
    
    // printf("]\n");
    printf("len: %d, executed: %d, pr: %d, pr_actual: %f, k: %d, mf: %f\n", 
                                n, j, pr, 
                                (((float)n - (float)j) / (float)n), 
                                get_k(n, pr), get_mf(n, pr));
}
