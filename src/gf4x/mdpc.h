#ifndef MDPC_H_
#define MDPC_H_

#include <time.h>
#include <stdint.h>
#include <stdbool.h>

#define BIT(len)       (1ULL << (len))
#define MASK(len)      (BIT(len) - 1)
#define DIVIDE_AND_CEIL(x, d) (((x) + ((d) - 1)) / (d))

#define N 12323
#define GAMMA 71
#define T 132
#define THRESHOLD1 5
#define THRESHOLD2 ((GAMMA + 1) / 2)

#define BLOCK_BITS 16384

#define N_BYTES  DIVIDE_AND_CEIL(N, 8)
#define N_QWORDS DIVIDE_AND_CEIL(N, 8 * BYTES_IN_QWORD)

#define N_BLOCKS DIVIDE_AND_CEIL(N, BLOCK_BITS)
#define N_PADDED (N_BLOCKS * BLOCK_BITS)
#define N_PADDED_BYTES  (N_PADDED / 8)
#define N_PADDED_QWORDS (N_PADDED / 64)

#define LAST_N_QWORD_LEAD  (N & MASK(6))
#define LAST_N_QWORD_TRAIL (64 - LAST_N_QWORD_LEAD)
#define LAST_N_QWORD_MASK  MASK(LAST_N_QWORD_LEAD)

#define LAST_N_BYTE_LEAD  (N & MASK(3))
#define LAST_N_BYTE_TRAIL (8 - LAST_N_BYTE_LEAD)
#define LAST_N_BYTE_MASK  MASK(LAST_N_BYTE_LEAD)

#define LSB3(x) ((x) & 7)
#define SECURE_BUFFER_QWORDS (3 * N_PADDED_QWORDS)

#define REG_QWORDS 1

#define BYTES_IN_QWORD 0x8
#define BYTES_IN_XMM   0x10
#define BYTES_IN_YMM   0x20
#define BYTES_IN_ZMM   0x40

#define N_BYTES DIVIDE_AND_CEIL(N, 8)

/*
// For N = 16651
#define MAX_I (15)
#define EXP0_K_VALS 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384
#define EXP0_L_VALS 8326, 4163, 13529, 6049, 8154, 273, 7925, 14704, 11032, 2865, 15933, 15994, 15374, 15582, 10493
#define EXP1_K_VALS 0, 0, 0, 1, 0, 0, 0, 0, 9, 0, 0, 0, 0, 0, 265
#define EXP1_L_VALS 0, 0, 0, 8326, 0, 0, 0, 0, 11350, 0, 0, 0, 0, 0, 14331
#define EXP1_REMS 1, 1, 1, 2, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 2
*/
#define MAX_I 14
#define EXP0_K_VALS 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192
#define EXP0_L_VALS 6162, 3081, 3851, 5632, 22, 484, 119, 1838, 1742, 3106, 10650, 1608, 10157, 8816
#define EXP1_K_VALS 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 33, 4129
#define EXP1_L_VALS 0, 0, 0, 0, 0, 6162, 0, 0, 0, 0, 0, 0, 242, 5717
#define EXP1_REMS   1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2


typedef struct {
    uint8_t value[N_BYTES];
    uint8_t padding[N_PADDED_BYTES - N_BYTES];
} GF2X;

typedef struct {
    uint8_t value[2 * N_PADDED_BYTES];
} TwoOfGF2X;

typedef struct {
    GF2X real;
    GF2X imaginary;
} GF4X;

typedef struct {
    GF4X first;
    GF4X second;
} TwoOfGF4X;

typedef struct {
    uint32_t indices[GAMMA];
    uint8_t values[GAMMA];
} GF4XSparse;

typedef struct {
    GF4X v;
    GF4X w;
    GF4XSparse v_sparse;
    GF4XSparse w_sparse;
} PrivateKey;

typedef struct {
    GF4X public_key;
    PrivateKey private_key;
} KeyPair;

typedef struct {
    time_t seconds;
    long nanoseconds;
} Timestamp;


//
// Code for the following functions:
// karatzuba()
// karatzuba_add1()
// karatzuba_add2()
// karatzuba_add3()
// gf2x_mul_base_port()
// gf2x_mod_mul()
// gf2x_red()
// secure_cmpeq64_mask()
//
// was taken from https://github.com/awslabs/bike-kem
//
void gf2x_mod_mul(GF2X *c, const GF2X *a, const GF2X *b);
void gf2x_mul_base_pclmul(uint64_t *c, const uint64_t *a, const uint64_t *b);
void gf2x_red(GF2X *c, const TwoOfGF2X *a);
void gf2x_add(GF2X *c, GF2X *a, GF2X *b);
void gf2x_add_in_place(GF2X *c, GF2X *a);
void karatzuba_add3(uint64_t *c, const uint64_t *mid, const size_t qwords_len);
void karatzuba_add2(uint64_t *z, const uint64_t *x, const uint64_t *y, const size_t qwords_len);
void karatzuba_add1(uint64_t *alah, uint64_t *blbh, const uint64_t *a, const uint64_t *b, const size_t qwords_len);

void gf4x_k_sqr(GF4X *c, GF4X *a, int kpow2inv, bool special);
void gf4x_add(GF4X *c, GF4X *a, GF4X *b);
void gf4x_add_in_place(GF4X *c, GF4X *a);
void gf4x_mod_mul(GF4X *c, GF4X *a, GF4X *b);
void gf4x_mod_inv(GF4X *c, GF4X *a);
void gf4x_print(GF4X *a);
int  gf4x_value_at(GF4X *a, int index);
void gf4x_set_value(GF4X *a, int index, int value);

extern int division_table[4][4];
extern int multiplication_table[4][4];
extern int subtraction_table[4][4];
extern int addition_table[4][4];
extern int inv_table[4];

KeyPair generate_keys(void);
void encode(GF4X *ciphertext, TwoOfGF4X *plaintext, GF4X *public_key);
int counter_argmax(int counter[4]);
bool decode(TwoOfGF4X *plaintext, GF4X *ciphertext, PrivateKey *private_key);

int pmod(int a, int m);
void fill_with_random_values_of_weight(GF4X *a, int weight, bool non_zero_free_coeffecient);
void initialize_sparse_vector_with_random_values(GF4XSparse *a, bool non_zero_free_coefficient);
Timestamp get_current_timestamp();
double milliseconds_elapsed_from(Timestamp start_time);

#endif // MDPC_H_
