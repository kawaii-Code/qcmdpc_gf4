#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "mdpc.h"

void fill_random_gf4x(GF4X *a) {
    memset(a, 0, sizeof(*a));
    for (int i = 0; i < (int)(sizeof(a->real.value)); i++) {
        a->real.value[i] = rand() & 0xFF;
        a->imaginary.value[i] = rand() & 0xFF;
    }
    a->real.value[N_BYTES - 1] &= LAST_N_BYTE_MASK;
    a->imaginary.value[N_BYTES - 1] &= LAST_N_BYTE_MASK;
}

void run_gf4x_mod_benchmark(int iterations) {
    fprintf(stderr, "GF4X Operations\n");

    double inv_total = 0;
    double mul_total = 0;

    GF4X a = {0}, b = {0}, c = {0};

    for (int i = 0; i < iterations; i++) {
        fill_random_gf4x(&a);
        fill_random_gf4x(&b);

        Timestamp t0 = get_current_timestamp();
        gf4x_mod_inv(&c, &a);
        inv_total += milliseconds_elapsed_from(t0);

        Timestamp t1 = get_current_timestamp();
        gf4x_mod_mul(&c, &a, &b);
        mul_total += milliseconds_elapsed_from(t1);
    }

    fprintf(stderr, "gf4x_mod_inv: avg %.6f ms (total %.3f ms)\n", inv_total / iterations, inv_total);
    fprintf(stderr, "gf4x_mod_mul: avg %.6f ms (total %.3f ms)\n", mul_total / iterations, mul_total);
}

void run_kem_benchmark(int iterations) {
    fprintf(stderr, "KEM Operations\n");

    double keygen_total = 0;
    double enc_total = 0;
    double dec_total = 0;
    int successes = 0;

    for (int i = 0; i < iterations; i++) {
        Timestamp t0 = get_current_timestamp();
        KeyPair keys = generate_keys();
        keygen_total += milliseconds_elapsed_from(t0);

        Timestamp t1 = get_current_timestamp();
        TwoOfGF4X e = {0};
        fill_with_random_values_of_weight(&e.first, GAMMA, false);
        fill_with_random_values_of_weight(&e.second, T - GAMMA, false);

        GF4X ciphertext = {0};
        encode(&ciphertext, &e, &keys.public_key);
        enc_total += milliseconds_elapsed_from(t1);

        Timestamp t2 = get_current_timestamp();
        TwoOfGF4X decoded = {0};
        bool ok = decode(&decoded, &ciphertext, &keys.private_key);
        dec_total += milliseconds_elapsed_from(t2);

        if (ok) {
            successes++;
        }
    }

    fprintf(stderr, "Keygen: avg %.3f ms (total %.3f ms)\n", keygen_total / iterations, keygen_total);
    fprintf(stderr, "Encaps: avg %.3f ms (total %.3f ms)\n", enc_total / iterations, enc_total);
    fprintf(stderr, "Decaps: avg %.3f ms (total %.3f ms)\n", dec_total / iterations, dec_total);
    fprintf(stderr, "Success rate: %d/%d\n", successes, iterations);
}

int main(int argc, char **argv) {
    int seed = (argc > 1) ? atoi(argv[1]) : (int)time(0);
    srand(seed);

    int iterations = 10;
    if (argc > 2) {
        iterations = atoi(argv[2]);
    }

    fprintf(stderr, "SEED = %d\n", seed);
    fprintf(stderr, "Iterations = %d\n", iterations);
    fprintf(stderr, "N = %d (2N = %d)\n", N, 2 * N);
    fprintf(stderr, "\n");
    run_gf4x_mod_benchmark(iterations);
    fprintf(stderr, "\n");
    run_kem_benchmark(iterations);

    return 0;
}
