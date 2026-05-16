#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "mdpc.h"

int main() {
    int seed = time(0);
    printf("SEED: %d\n", seed);
    srand(seed);

    int samples = 1000;
    int stats[N][4] = {0};
    for (int i = 0; i < samples; i++) {
        GF4X v = {0};
        fill_with_random_values_of_weight(&v, GAMMA, true);
        int wt = 0;
        for (int j = 0; j < N; j++) {
            stats[j][gf4x_value_at(&v, j)] += 1;
            wt += gf4x_value_at(&v, j) != 0;
        }
        if (wt != GAMMA) {
            printf("ERROR WEIGHT: %d\n", wt);
        }
    }

    for (int i = 0; i < N; i++) {
        int sum = 0;
        for (int j = 0; j < 4; j++) {
            printf("%d ", stats[i][j]);
            if (j > 0) {
                sum += stats[i][j];
            }
        }
        printf("|\t%d\n", sum);
    }
}
