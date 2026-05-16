#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "mdpc.h"

int division_table[4][4] = {
    {-1, 0, 0, 0 },
    {-1, 1, 3, 2 },
    {-1, 2, 1, 3 },
    {-1, 3, 2, 1 },
};

int multiplication_table[4][4] = {
    {0, 0, 0, 0},
    {0, 1, 2, 3},
    {0, 2, 3, 1},
    {0, 3, 1, 2},
};

int subtraction_table[4][4] = {
    {0, 1, 2, 3},
    {1, 0, 3, 2},
    {2, 3, 0, 1},
    {3, 2, 1, 0},
};

int addition_table[4][4] = {
    {0, 1, 2, 3},
    {1, 0, 3, 2},
    {2, 3, 0, 1},
    {3, 2, 1, 0},
};

int inv_table[4] = {
    -1, 1, 3, 2,
};

KeyPair generate_keys(void) {
try_again:;
    KeyPair keys = {0};

    initialize_sparse_vector_with_random_values(&keys.private_key.v_sparse, true);
    initialize_sparse_vector_with_random_values(&keys.private_key.w_sparse, true);
    for (int i = 0; i < GAMMA; i++) {
        gf4x_set_value(&keys.private_key.v, keys.private_key.v_sparse.indices[i], keys.private_key.v_sparse.values[i]);
        gf4x_set_value(&keys.private_key.w, keys.private_key.w_sparse.indices[i], keys.private_key.w_sparse.values[i]);
    }

    Timestamp start = get_current_timestamp();

    GF4X vinv = {0};
    gf4x_mod_inv(&vinv, &keys.private_key.v);

    GF4X temp = {0};
    gf4x_mod_mul(&temp, &keys.private_key.v, &vinv);
    int total = 0;
    for (int i = 0; i < N_BYTES; i++) {
        total += temp.real.value[i] + 256 * temp.imaginary.value[i];
    }

    //fprintf(stderr, "gf4x_mod_inv() took %dms\n", milliseconds_elapsed_from(start));

    if (total != 1 || temp.real.value[0] != 1) {
        goto try_again;
    }

    gf4x_mod_mul(&keys.public_key, &vinv, &keys.private_key.w);

    return keys;
}

void encode(GF4X *ciphertext, TwoOfGF4X *plaintext, GF4X *public_key) {
    gf4x_mod_mul(ciphertext, &plaintext->second, public_key);
    gf4x_add(ciphertext, ciphertext, &plaintext->first);
}

int counter_argmax(int counter[4]) {
    int result = 1;
    for (int i = 2; i < 4; i++) {
        if (counter[i] > counter[result]) {
            result = i;
        }
    }
    return result;
}

bool decode(TwoOfGF4X *plaintext, GF4X *ciphertext, PrivateKey *private_key) {
    (void)plaintext;
    GF4XSparse v = private_key->v_sparse;
    GF4XSparse w = private_key->w_sparse;

    TwoOfGF4X et = {0};
    GF4X s_original = {0};
    gf4x_mod_mul(&s_original, &private_key->v, ciphertext);
    GF4X s = s_original;

    int counter[N][4] = {0};
    for (int j = 0; j < GAMMA; j++) {
        int inv = division_table[1][v.values[j]];
        int shift = v.indices[j];
        for (int i = shift; i < N; i++) {
            int diff = multiplication_table[gf4x_value_at(&s, i)][inv];
            counter[i - shift][diff] += 1;
        }
        for (int i = 0; i < shift; i++) {
            int diff = multiplication_table[gf4x_value_at(&s, i)][inv];
            counter[i + N - shift][diff] += 1;
        }
    }
    for (int i = 0; i < N; i++) {
        int mxi = counter_argmax(counter[i]);
        if (counter[i][mxi] - counter[i][0] >= THRESHOLD1) {
            gf4x_set_value(&et.first, i, mxi);
        }
    }

    for (int i = 0; i < N; i++) {
        counter[i][0] = 0;
        counter[i][1] = 0;
        counter[i][2] = 0;
        counter[i][3] = 0;
    }

    for (int j = 0; j < GAMMA; j++) {
        int inv = division_table[1][w.values[j]];
        int shift = w.indices[j];
        for (int i = shift; i < N; i++) {
            int diff = multiplication_table[gf4x_value_at(&s, i)][inv];
            counter[i - shift][diff] += 1;
        }
        for (int i = 0; i < shift; i++) {
            int diff = multiplication_table[gf4x_value_at(&s, i)][inv];
            counter[i + N - shift][diff] += 1;
        }
    }
    for (int i = 0; i < N; i++) {
        int mxi = counter_argmax(counter[i]);
        if (counter[i][mxi] - counter[i][0] >= THRESHOLD1) {
            gf4x_set_value(&et.first, i, mxi);
        }
    }

    GF4X t0 = {0};
    GF4X t1 = {0};
    gf4x_mod_mul(&t0, &et.first, &private_key->v);
    gf4x_mod_mul(&t1, &et.second, &private_key->w);
    gf4x_add_in_place(&s, &t0);
    gf4x_add_in_place(&s, &t1);

    int total = 0;
    for (int i = 0; i < N_BYTES; i++) {
        total += s.real.value[i] + 256 * s.imaginary.value[i];
    }

    if (total == 0) {
        return true;
    }

    return false;
}
