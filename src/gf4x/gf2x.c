#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "mdpc.h"

uint64_t secure_cmpeq64_mask(const uint64_t v1, const uint64_t v2) {
    return -(1 - ((uint64_t) ((v1 - v2) | (v2 - v1)) >> 63));
}

void gf2x_mul_base_port(uint64_t *c, const uint64_t *a, const uint64_t *b) {
    uint64_t       h = 0, l = 0, g1, g2, u[8];
    const uint64_t w  = 64;
    const uint64_t s  = 3;
    const uint64_t a0 = a[0];
    const uint64_t b0 = b[0];

    const uint64_t b0m = b0 & MASK(61);

    u[0] = 0;
    u[1] = b0m;
    u[2] = u[1] << 1;
    u[3] = u[2] ^ b0m;
    u[4] = u[2] << 1;
    u[5] = u[4] ^ b0m;
    u[6] = u[3] << 1;
    u[7] = u[6] ^ b0m;

    for (size_t i = 0; i < 8; ++i) {
        l ^= u[i] & secure_cmpeq64_mask(LSB3(a0), i);
        l ^= (u[i] << 3) & secure_cmpeq64_mask(LSB3(a0 >> 3), i);
        h ^= (u[i] >> 61) & secure_cmpeq64_mask(LSB3(a0 >> 3), i);
    }

    for (size_t i = (2 * s); i < w; i += (2 * s)) {
        const size_t i2 = (i + s);

        g1 = 0;
        g2 = 0;
        for (size_t j = 0; j < 8; ++j) {
            g1 ^= u[j] & secure_cmpeq64_mask(LSB3(a0 >> i), j);
            g2 ^= u[j] & secure_cmpeq64_mask(LSB3(a0 >> i2), j);
        }

        l ^= (g1 << i) ^ (g2 << i2);
        h ^= (g1 >> (w - i)) ^ (g2 >> (w - i2));
    }

    for (size_t i = 61; i < 64; i++) {
        uint64_t mask = (-((b0 >> i) & 1));
        l ^= ((a0 << i) & mask);
        h ^= ((a0 >> (w - i)) & mask);
    }

    c[0] = l;
    c[1] = h;
}

void karatzuba(uint64_t       *c,
               const uint64_t *a,
               const uint64_t *b,
               const size_t    qwords_len,
               const size_t    qwords_len_pad,
               uint64_t       *sec_buf) {
    if (qwords_len <= 1) {
        gf2x_mul_base_port(c, a, b);
        return;
    }

    const size_t half_qw_len = qwords_len_pad >> 1;

    const uint64_t *a_lo = a;
    const uint64_t *b_lo = b;
    const uint64_t *a_hi = &a[half_qw_len];
    const uint64_t *b_hi = &b[half_qw_len];

    uint64_t *c0 = c;
    uint64_t *c1 = &c[half_qw_len];
    uint64_t *c2 = &c[half_qw_len * 2];

    uint64_t *alah = sec_buf;
    uint64_t *blbh = &sec_buf[half_qw_len];
    uint64_t *tmp  = &sec_buf[half_qw_len * 2];

    sec_buf = &sec_buf[half_qw_len * 3];

    karatzuba(c0, a_lo, b_lo, half_qw_len, half_qw_len, sec_buf);

    if (qwords_len > half_qw_len) {
        karatzuba(c2, a_hi, b_hi, qwords_len - half_qw_len, half_qw_len, sec_buf);

        karatzuba_add1(alah, blbh, a, b, half_qw_len);

        karatzuba_add2(tmp, c1, c2, half_qw_len);

        karatzuba(c1, alah, blbh, half_qw_len, half_qw_len, sec_buf);

        karatzuba_add3(c0, tmp, half_qw_len);
    }
}

void gf2x_mod_mul(GF2X *c, const GF2X *a, const GF2X *b) {
    assert(N_PADDED_BYTES % 2 == 0);

    TwoOfGF2X t = {0};
    uint64_t  secure_buffer[SECURE_BUFFER_QWORDS];

    karatzuba((uint64_t *) &t, (const uint64_t *) a, (const uint64_t *) b, N_QWORDS, N_PADDED_QWORDS, secure_buffer);

    gf2x_red(c, &t);
}
