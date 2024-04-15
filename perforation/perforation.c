#include "perforation.h"

#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define USER_GET_PERFORATION_RATE 0x125

int pr = 0;

void init_perforation() {
    // tonight
}

int get_perforation_rate(int type)
{
    return SimUser(USER_GET_PERFORATION_RATE, type);
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

