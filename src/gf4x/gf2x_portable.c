//
// Code for the following functions:
// secure_cmpeq64_mask()
// gf2x_mul_base_port()
// karatzuba()
// karatzuba_add1()
// karatzuba_add2()
// karatzuba_add3()
// gf2x_mod_mul()
// gf2x_red()
//
// was taken from https://github.com/awslabs/bike-kem
//
#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "mdpc.h"

void karatzuba_add1(uint64_t *alah, uint64_t       *blbh, const uint64_t *a, const uint64_t *b, const size_t    qwords_len) {
    assert(qwords_len % REG_QWORDS == 0);

    uint64_t va0, va1, vb0, vb1;

    for (size_t i = 0; i < qwords_len; i += REG_QWORDS) {
        va0 = a[i];
        va1 = a[i + qwords_len];
        vb0 = b[i];
        vb1 = b[i + qwords_len];

        alah[i] = va0 ^ va1;
        blbh[i] = vb0 ^ vb1;
    }
}

void karatzuba_add2(uint64_t *z, const uint64_t *x, const uint64_t *y, size_t qwords_len) {
    assert(qwords_len % REG_QWORDS == 0);

    uint64_t vx, vy;

    for (size_t i = 0; i < qwords_len; i += REG_QWORDS) {
        vx = x[i];
        vy = y[i];

        z[i] = vx ^ vy;
    }
}

void karatzuba_add3(uint64_t *c, const uint64_t *mid, size_t qwords_len) {
    assert(qwords_len % REG_QWORDS == 0);

    uint64_t vr0, vr1, vr2, vr3, vt;

    uint64_t *c0 = c;
    uint64_t *c1 = &c[qwords_len];
    uint64_t *c2 = &c[2 * qwords_len];
    uint64_t *c3 = &c[3 * qwords_len];

    for (size_t i = 0; i < qwords_len; i += REG_QWORDS) {
        vr0 = c0[i];
        vr1 = c1[i];
        vr2 = c2[i];
        vr3 = c3[i];
        vt  = mid[i];

        c1[i] = vt ^ vr0 ^ vr1;
        c2[i] = vt ^ vr2 ^ vr3;
    }
}

void gf2x_red(GF2X *c, const TwoOfGF2X *a) {
    const uint64_t *a64 = (const uint64_t *) a;
    uint64_t       *c64 = (uint64_t *) c;

    for (size_t i = 0; i < N_QWORDS; i += REG_QWORDS) {
        uint64_t vt0 = a64[i];
        uint64_t vt1 = a64[i + N_QWORDS];
        uint64_t vt2 = a64[i + N_QWORDS - 1];

        vt1 = vt1 << LAST_N_QWORD_TRAIL;
        vt2 = vt2 >> LAST_N_QWORD_LEAD;

        vt0 ^= (vt1 | vt2);

        c64[i] = vt0;
    }

    c64[N_QWORDS - 1] &= LAST_N_QWORD_MASK;
}

void gf2x_add_in_place(GF2X *c, GF2X *a) {
    uint64_t *a64 = (uint64_t *)a;
    uint64_t *c64 = (uint64_t *)c;

    for (int i = 0; i < N_QWORDS; i += 1) {
        c64[i] = a64[i] ^ c64[i];
    }
}

void gf2x_add(GF2X *c, GF2X *a, GF2X *b) {
    memset(c->value, 0, sizeof(c->value));

    uint64_t *a64 = (uint64_t *)a;
    uint64_t *b64 = (uint64_t *)b;
    uint64_t *c64 = (uint64_t *)c;

    for (int i = 0; i < N_QWORDS; i += 1) {
        c64[i] = a64[i] ^ b64[i];
    }
}
