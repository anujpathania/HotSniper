#include <stdlib.h>
#include <stdio.h>

#include "perforation.h"

#define SIZE 10

int main() {
    int* tests[SIZE]; 
    int* trunc[SIZE];

    printf("pr rates: [");
    for (int i = 0; i < SIZE; i++) {
        printf("%f, ", ((float)i / (float)SIZE) * 100);
        tests[i] = perf_vector(1080, ((float)i / (float)SIZE) * 100);
        trunc[i] = perf_vector_truncate(1080, ((float)i / (float)SIZE) * 100);
    }
    printf("]\n\n");

    printf("[Selection]:\n");
    for (int i = 0; i < SIZE; i++) {
        print_vec(tests[i], 1080, ((float)i / (float)SIZE) * 100);
    }

    printf("\n[Truncation]:\n");
    for (int i = 0; i < SIZE; i++) {
        print_vec(trunc[i], 1080, ((float)i / (float)SIZE) * 100);
    }

    for (int i = 0; i < SIZE; i++) {
        free_vec(tests[i]);
        free_vec(trunc[i]);
    }

    return 0;
}
