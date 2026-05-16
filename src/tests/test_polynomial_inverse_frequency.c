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

    int samples = 100;
    int ok = 0;
    for (int i = 0; i < samples; i++) {
        GF4X v = {0};
        GF4X vinv = {0};
        GF4X temp = {0};

        fill_with_random_values_of_weight(&v, GAMMA, true);

        gf4x_mod_inv(&vinv, &v);

        gf4x_mod_mul(&temp, &vinv, &v);

        int total = 0;
        for (int i = 0; i < N_BYTES; i++) {
            total += temp.real.value[i] + 256 * temp.imaginary.value[i];
        }
        if (total != 1 || temp.real.value[0] != 1) {
            // Not an inverse
        } else {
            ok += 1;
        }
    }

    printf("Inverse polynomial was found in %d/%d samples (%.2f%%)\n", ok, samples, (float)ok / (float)samples);
}

