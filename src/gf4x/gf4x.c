#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "mdpc.h"

void gf4x_add_in_place(GF4X *c, GF4X *a) {
    gf2x_add_in_place(&c->real, &a->real);
    gf2x_add_in_place(&c->imaginary, &a->imaginary);
}

void gf4x_add(GF4X *c, GF4X *a, GF4X *b) {
    gf2x_add(&c->real, &a->real, &b->real);
    gf2x_add(&c->imaginary, &a->imaginary, &b->imaginary);
}

void gf4x_mod_mul(GF4X *c, GF4X *a, GF4X *b) {
    GF2X z0 = {0};
    GF2X z1 = {0};
    GF2X z2 = {0};
    GF2X t1 = {0};
    GF2X t2 = {0};

    gf2x_mod_mul(&z0, &a->real, &b->real);
    gf2x_mod_mul(&z2, &a->imaginary, &b->imaginary);
    gf2x_add(&t1, &a->real, &a->imaginary);
    gf2x_add(&t2, &b->real, &b->imaginary);
    gf2x_mod_mul(&z1, &t1, &t2);
    gf2x_add_in_place(&z1, &z0);
    gf2x_add_in_place(&z1, &z2);

    gf2x_add(&c->real, &z0, &z2);
    gf2x_add(&c->imaginary, &z1, &z2);
}

// a^2^2^n
void gf4x_k_sqr(GF4X *c, GF4X *a, int kpow2inv, bool special) {
    memset(c, 0, sizeof(*c));

    int index = 0;
    for (int i = 0; i < N_BYTES; i++) {
        for (int j = 0; j < 8; j++) {
            int pos = (kpow2inv * index) % N;

            int byte_index = pos >> 3;
            int bit_index = pos & 7;
            uint8_t real = ((a->real.value[byte_index] >> bit_index) & 1);
            uint8_t imaginary = ((a->imaginary.value[byte_index] >> bit_index) & 1);

            if (special && imaginary != 0) {
                real ^= 1;
            }
            c->real.value[i] |= real << j;
            c->imaginary.value[i] |= imaginary << j;

            index += 1;
        }
    }
    c->real.value[N_BYTES - 1] &= LAST_N_BYTE_MASK;
    c->imaginary.value[N_BYTES - 1] &= LAST_N_BYTE_MASK;
}

void gf4x_mod_inv(GF4X *c, GF4X *a) {
    const size_t exp0_l[MAX_I] = {EXP0_L_VALS};
    const size_t exp1_l[MAX_I] = {EXP1_L_VALS};
    const int rems[MAX_I] = {EXP1_REMS};

    GF4X f = {0};
    GF4X g = {0};
    GF4X r = {0};

    f = *a;
    r = *a;

    for (int i = 1; i < MAX_I; i++) {
        gf4x_k_sqr(&g, &f, exp0_l[i - 1], i == 1);

        gf4x_mod_mul(&f, &g, &f);

        if (exp1_l[i] != 0) {
            gf4x_k_sqr(&g, &f, exp1_l[i], rems[i] == 2);
            gf4x_mod_mul(&r, &g, &r);
        }
    }

    gf4x_mod_mul(&r, &r, &r);

    *c = r;
}

int gf4x_value_at(GF4X *a, int index) {
    assert(0 <= index && index < N);
    int byte = index / 8;
    int bit = index % 8;
    int r = (a->real.value[byte] >> bit) & 1;
    int i = (a->imaginary.value[byte] >> bit) & 1;
    return 2 * i + r;
}

void gf4x_set_value(GF4X *a, int index, int value) {
    assert(0 <= index && index < N);
    assert(0 <= value && value <= 3);

    int byte = index / 8;
    int bit = index % 8;
    int r = value % 2;
    int i = value / 2;

    a->real.value[byte] &= ~(1 << bit);
    a->real.value[byte] |= (r << bit);

    a->imaginary.value[byte] &= ~(1 << bit);
    a->imaginary.value[byte] |= (i << bit);
}

void gf4x_print(GF4X *a) {
    printf("POLY = ");
    int pow = N_BYTES * 8 - 1;
    bool first = true;
    for (int i = N_BYTES - 1; i >= 0; i--) {
        for (int b = 7; b >= 0; b--) {
            uint8_t real = (a->real.value[i] >> b) & 1;
            uint8_t imaginary = (a->imaginary.value[i] >> b) & 1;
            if (real + imaginary > 0) {
                if (!first) {
                    printf(" + ");
                }
                first = false;
                if (real && imaginary) {
                    printf("(z+1)");
                } else if (imaginary) {
                    printf("z");
                }

                if (imaginary) {
                    printf("*");
                }

                if (pow == 0) {
                    printf("1");
                } else {
                    if (pow == 1) {
                        printf("x");
                    } else {
                        printf("x^%d", pow);
                    }
                }
            }
            pow -= 1;
        }
    }
    printf("\n");
}
