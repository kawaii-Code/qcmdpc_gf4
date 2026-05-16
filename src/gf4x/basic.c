#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "mdpc.h"

Timestamp get_current_timestamp() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    Timestamp current_time;
    current_time.seconds = ts.tv_sec;
    current_time.nanoseconds = ts.tv_nsec;
    return current_time;
}

int pmod(int a, int m) {
    return (a % m + m) % m;
}

double milliseconds_elapsed_from(Timestamp start_time) {
    Timestamp timestamp = get_current_timestamp();
    return (timestamp.seconds - start_time.seconds) * 1000.0 +
           (timestamp.nanoseconds - start_time.nanoseconds) / (1000.0*1000.0);
}

// TODO: Rewrite this
void initialize_sparse_vector_with_random_values(GF4XSparse *a, bool non_zero_free_coefficient) {
    for (int i = 0; i < GAMMA; i++) {
        a->values[i] = (rand() % 3) + 1;
    }

    int swapped[4 * GAMMA];
    memset(swapped, -1, sizeof(swapped));

    #define PROBE(key) ({                                           \
        int _k = (key), _slot = _k % (4 * GAMMA);                  \
        while (swapped[_slot] != -1 && swapped[_slot] != _k)       \
            _slot = (_slot + 1) % (4 * GAMMA);                     \
        swapped[_slot] != -1 ? swapped[(_k + 4*GAMMA) % (4*GAMMA)] \
                              : _k;                                  \
    })

    #define ASSIGN(key, val) do {                                   \
        int _k = (key), _v = (val), _slot = _k % (4 * GAMMA);     \
        while (swapped[_slot] != -1 && swapped[_slot] != _k)       \
            _slot = (_slot + 1) % (4 * GAMMA);                     \
        swapped[_slot] = _k;                                        \
        swapped[(_k + 4*GAMMA) % (4*GAMMA)] = _v; /* value cell */ \
    } while(0)

    int start = non_zero_free_coefficient ? 1 : 0;
    int pool  = N - start;

    for (int i = 0; i < GAMMA; i++) {
        int j = i + rand() % (pool - i);
        int vi = PROBE(i), vj = PROBE(j);
        ASSIGN(i, vj);
        ASSIGN(j, vi);
        a->indices[i] = vj + start;
    }

    if (non_zero_free_coefficient) {
        a->indices[0] = 0;
    }

    #undef PROBE
    #undef ASSIGN
}

void fill_with_random_values_of_weight(GF4X *a, int weight, bool non_zero_free_coeffecient) {
    assert(weight < N);

    int bit = 0;
    int byte_index = 0;
    for (int i = 0; i < weight; i++) {
        int value = 1 + rand() % 3;
        a->real.value[byte_index] |= (value % 2) << bit;
        a->imaginary.value[byte_index] |= (value / 2) << bit;

        bit += 1;
        if (bit == 8) {
            byte_index += 1;
            bit = 0;
        }
    }

    for (int i = 0; i < N_BYTES; i++) {
        for (uint8_t b = (i == 0 && non_zero_free_coeffecient) ? 1 : 0; b < 8; b++) {
            if (8*i + b >= N) {
                break;
            }

            int ji = (8*i + b) + rand() % (N - (8*i + b));
            int j = ji / 8;
            uint8_t jb = ji % 8;

            uint8_t re1 = a->real.value[i];
            uint8_t im1 = a->imaginary.value[i];
            uint8_t re2 = a->real.value[j];
            uint8_t im2 = a->imaginary.value[j];

            a->real.value[i]      &= ~(uint8_t)(1u << b);
            a->imaginary.value[i] &= ~(uint8_t)(1u << b);
            a->real.value[j]      &= ~(uint8_t)(1u << jb);
            a->imaginary.value[j] &= ~(uint8_t)(1u << jb);

            a->real.value[i]      |= (((re2 >> jb) & 1u) << b);
            a->imaginary.value[i] |= (((im2 >> jb) & 1u) << b);
            a->real.value[j]      |= (((re1 >> b)  & 1u) << jb);
            a->imaginary.value[j] |= (((im1 >> b)  & 1u) << jb);
        }
    }
}
