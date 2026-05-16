#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdalign.h>

#include <immintrin.h>

#include "mdpc.h"

#define LOAD(mem)       _mm256_loadu_si256((const void *)(mem))
#define STORE(mem, reg) _mm256_storeu_si256((void *)(mem), (reg))

#define BYTES_IN_YMM 0x20
#define BYTES_IN_XMM 0x10
#define WORDS_IN_YMM (BYTES_IN_YMM / sizeof(uint16_t))
#define QWORDS_IN_XMM (BYTES_IN_XMM / sizeof(uint64_t))

// If R_BITS >= 32768 then adding two elements of the permutation map can
// exceed the size of uin16_t type. Therefore, in this case we have to work
// with uint32_t type and use appropriate AVX2 instructions to compute the
// permutation map. Otherwise, uint16_t suffices, which allows us to work with
// this type and have a more efficient implementation (a single AVX2 register
// can hold eight 32-bit elements or sixteen 16-bit elements).
#if(N < 32768)

#  define MAP_WORDS_IN_YMM WORDS_IN_YMM

#  define map_word_t  uint16_t
#  define SET1(x)     SET1_I16(x)
#  define SUB(x, y)   SUB_I16(x, y)
#  define ADD(x, y)   ADD_I16(x, y)
#  define CMPGT(x, y) CMPGT_I16(x, y)

#else

#  define MAP_WORDS_IN_YMM DWORDS_IN_YMM

#  define map_word_t  uint32_t
#  define SET1(x)     SET1_I32(x)
#  define SUB(x, y)   SUB_I32(x, y)
#  define ADD(x, y)   ADD_I32(x, y)
#  define CMPGT(x, y) CMPGT_I32(x, y)

#endif

#define NUM_YMMS    (2)
#define NUM_OF_VALS (NUM_YMMS * MAP_WORDS_IN_YMM)
#define REG_T __m256i
#define ALIGN_BYTES 32

#define SLLI_I64(a, imm) _mm256_slli_epi64(a, imm)
#define SRLI_I64(a, imm) _mm256_srli_epi64(a, imm)
#define REG_QWORDS (sizeof(REG_T) / sizeof(uint64_t)) // NOLINT
#define REG_DWORDS (sizeof(REG_T) / sizeof(uint32_t)) // NOLINT

#  define SET_I8(...)  _mm256_set_epi8(__VA_ARGS__)
#  define SET_I32(...) _mm256_set_epi32(__VA_ARGS__)
#  define SET_I64(...) _mm256_set_epi64x(__VA_ARGS__)
#  define SET1_I8(a)   _mm256_set1_epi8(a)
#  define SET1_I16(a)  _mm256_set1_epi16(a)
#  define SET1_I32(a)  _mm256_set1_epi32(a)
#  define SET1_I64(a)  _mm256_set1_epi64x(a)
#  define SET_ZERO     _mm256_setzero_si256()

#  define ADD_I8(a, b)     _mm256_add_epi8(a, b)
#  define SUB_I8(a, b)     _mm256_sub_epi8(a, b)
#  define ADD_I16(a, b)    _mm256_add_epi16(a, b)
#  define ADD_I32(a, b)    _mm256_add_epi32(a, b)
#  define SUB_I16(a, b)    _mm256_sub_epi16(a, b)
#  define SUB_I32(a, b)    _mm256_sub_epi32(a, b)
#  define ADD_I64(a, b)    _mm256_add_epi64(a, b)
#  define SRLI_I16(a, imm) _mm256_srli_epi16(a, imm)
#  define SLLI_I32(a, imm) _mm256_slli_epi32(a, imm)
#  define SLLV_I32(a, b)   _mm256_sllv_epi32(a, b)

#  define CMPGT_I16(a, b) _mm256_cmpgt_epi16(a, b)
#  define CMPGT_I32(a, b) _mm256_cmpgt_epi32(a, b)
#  define CMPEQ_I16(a, b) _mm256_cmpeq_epi16(a, b)
#  define CMPEQ_I32(a, b) _mm256_cmpeq_epi32(a, b)
#  define CMPEQ_I64(a, b) _mm256_cmpeq_epi64(a, b)

#  define SHUF_I8(a, b)         _mm256_shuffle_epi8(a, b)
#  define BLENDV_I8(a, b, mask) _mm256_blendv_epi8(a, b, mask)
#  define PERMVAR_I32(a, idx)   _mm256_permutevar8x32_epi32(a, idx)
#  define PERM_I64(a, imm)      _mm256_permute4x64_epi64(a, imm)

#  define MOVEMASK(a) _mm256_movemask_epi8(a)

#  define MSTORE32(mem, mask, reg) _mm256_maskstore_epi32((int*)(mem), (mask), (reg))

void generate_map(map_word_t *map, const map_word_t l_param)
{
  __m256i vmap[NUM_YMMS], vtmp[NUM_YMMS], vr, inc, zero;

  // The permutation map is generated in the following way:
  //   1. for i = 0 to map size:
  //   2.  map[i] = (i * l_param) % r
  // However, to avoid the expensive multiplication and modulo operations
  // we modify the algorithm to:
  //   1. map[0] = l_param
  //   2. for i = 1 to map size:
  //   3.   map[i] = map[i - 1] + l_param
  //   4.   if map[i] >= r:
  //   5.     map[i] = map[i] - r
  // This algorithm is parallelized with vector instructions by processing
  // certain number of values (NUM_OF_VALS) in parallel. Therefore,
  // in the beginning we need to initialize the first NUM_OF_VALS elements.
  for(size_t i = 0; i < NUM_OF_VALS; i++) {
    map[i] = (i * l_param) % N;
  }

  vr   = SET1(N);
  zero = SET_ZERO;

  // Set the increment vector such that adding it to vmap vectors
  // gives the next NUM_OF_VALS elements of the map. AVX2 does not
  // support comparison of vectors where vector elements are considered
  // as unsigned integers. This is a problem when r > 2^14 because
  // sum of two values can be greater than 2^15 which would make the it
  // a negative number when considered as a signed 16-bit integer,
  // and therefore, the condition in step 4 of the algorithm would be
  // evaluated incorrectly. So, we use the following trick:
  // we subtract R from the increment and modify the algorithm:
  //   1. map[0] = l_param
  //   2. for i = 1 to map size:
  //   3.   map[i] = map[i - 1] + (l_param - r)
  //   4.   if map[i] < 0:
  //   5.     map[i] = map[i] + r
  inc = SET1((l_param * NUM_OF_VALS) % N);
  inc = SUB(inc, vr);

  // Load the first NUM_OF_VALS elements in the vmap vectors
  for(size_t i = 0; i < NUM_YMMS; i++) {
    vmap[i] = LOAD(&map[i * MAP_WORDS_IN_YMM]);
  }

  for(size_t i = NUM_YMMS; i < (N_PADDED / MAP_WORDS_IN_YMM); i += NUM_YMMS) {
    for(size_t j = 0; j < NUM_YMMS; j++) {
      vmap[j] = ADD(vmap[j], inc);
      vtmp[j] = CMPGT(zero, vmap[j]);
      vmap[j] = ADD(vmap[j], vtmp[j] & vr);

      STORE(&map[(i + j) * MAP_WORDS_IN_YMM], vmap[j]);
    }
  }
}

// Convert from bytes representation, where every byte holds a single bit,
// of the polynomial, to a binary representation where every byte
// holds 8 bits of the polynomial.
void bytes_to_bin(GF2X *bin_buf, const uint8_t *bytes_buf)
{
  uint32_t *bin32 = (uint32_t *)bin_buf;

  for(size_t i = 0; i < N_QWORDS * 2; i++) {
    __m256i t = LOAD(&bytes_buf[i * BYTES_IN_YMM]);
    bin32[i]  = MOVEMASK(t);
  }
}

// Convert from binary representation where every byte holds 8 bits
// of the polynomial, to byte representation where
// every byte holds a single bit of the polynomial.
void bin_to_bytes(uint8_t *bytes_buf, const GF2X *bin_buf)
{
  // The algorithm works by taking every 32 bits of the input and converting
  // them to 32 bytes where each byte holds one of the bits. The first step is
  // to broadcast a 32-bit value (call it a)  to all elements of vector t.
  // Then t contains bytes of a in the following order:
  //   t = [ a3 a2 a1 a0 ... a3 a2 a1 a0 ]
  // where a0 contains the first 8 bits of a, a1 the second 8 bits, etc.
  // Let the output vector be [ out31 out30 ... out0 ]. We want to store
  // bit 0 of a in out0 byte, bit 1 of a in out1 byte, ect. (note that
  // we want to store the bit in the most significant position of a byte
  // because this is required by MOVEMASK instruction used in bytes_to_bin.)
  //
  // Ideally, we would shuffle the bytes of t such that the byte in
  // i-th position contains i-th bit of val, shift t appropriately and obtain
  // the result. However, AVX2 doesn't support shift operation on bytes, only
  // shifts of individual QWORDS (64 bit) and DWORDS (32 bit) are allowed.
  // Consider the two least significant DWORDS of t:
  //   t = [ ... | a3 a2 a1 a0 | a3 a2 a1 a0 ]
  // and shift them by 6 and 4 to the left, respectively, to obtain:
  //   t = [ ... | t7 t6 t5 t4 | t3 t2 t1 t0 ]
  // where t3 = a3 << 6, t2 = a2 << 6, t1 = a1 << 6, t0 = a0 << 6,
  // and   t7 = a3 << 4, t6 = a2 << 4, t5 = a1 << 4, t4 = a0 << 4.
  // Now we shuffle vector t to obtain vector p such that:
  //   p = [ ... | t12 t12 t8 t8 | t4 t4 t0 t0 ]
  // Note that in every even position of the vector p we have the right byte
  // of the input shifted by the required shift. The values in the odd
  // positions contain the right bytes of the input but they need to be shifted
  // one more time to the left by 1. By shifting each DWORD of p by 1 we get:
  //   q = [ ... | p7 p6 p5 p4 | p3 p2 p1 p0 ]
  // where p1 = t0 << 1 = a0 << 7, p3 = t4 << 1 = 5, etc. Therefore, by
  // blending p and q (taking even positions from p and odd positions from q)
  // we obtain the desired result.

  __m256i t, p, q;

  const __m256i shift_mask = SET_I32(0, 2, 4, 6, 0, 2, 4, 6);

  const __m256i shuffle_mask =
    SET_I8(15, 15, 11, 11, 7, 7, 3, 3, 14, 14, 10, 10, 6, 6, 2, 2, 13, 13, 9, 9,
           5, 5, 1, 1, 12, 12, 8, 8, 4, 4, 0, 0);

  const __m256i blend_mask = SET1_I16(0x00ff);

  const uint32_t *bin32 = (const uint32_t *)bin_buf;

  for(size_t i = 0; i < N_QWORDS * 2; i++) {
    t = SET1_I32(bin32[i]);
    t = SLLV_I32(t, shift_mask);

    p = SHUF_I8(t, shuffle_mask);
    q = SLLI_I32(p, 1);

    STORE(&bytes_buf[i * 32], BLENDV_I8(p, q, blend_mask));
  }
}

// The k-squaring function computes c = a^(2^k) % (x^r - 1).
// By [1](Observation 1), if
//     a = sum_{j in supp(a)} x^j,
// then
//     a^(2^k) % (x^r - 1) = sum_{j in supp(a)} x^((j * 2^k) % r).
// Therefore, k-squaring can be computed as permutation of the bits of "a":
//     pi0 : j --> (j * 2^k) % r.
// For improved performance, we compute the result by inverted permutation pi1:
//     pi1 : (j * 2^-k) % r --> j.
// Input argument l_param is defined as the value (2^-k) % r.
void k_sqr_avx2(GF2X *c, const GF2X *a, const size_t l_param)
{
  alignas(ALIGN_BYTES) map_word_t map[N_PADDED];
  alignas(ALIGN_BYTES) uint8_t    a_bytes[N_PADDED];
  alignas(ALIGN_BYTES) uint8_t    c_bytes[N_PADDED] = {0};

  // Generate the permutation map defined by pi1 and l_param.
  generate_map(map, l_param);

  bin_to_bytes(a_bytes, a);

  // Permute "a" using the generated permutation map.
  for(size_t i = 0; i < N; i++) {
    c_bytes[i] = a_bytes[map[i]];
  }

  bytes_to_bin(c, c_bytes);

  //secure_clean(a_bytes, sizeof(a_bytes));
  //secure_clean(c_bytes, sizeof(c_bytes));
}


void karatzuba_add1(uint64_t *alah,
                    uint64_t *blbh,
                    const uint64_t *a,
                    const uint64_t *b,
                    const size_t    qwords_len)
{
  assert(qwords_len % REG_QWORDS == 0);

  REG_T va0, va1, vb0, vb1;

  for(size_t i = 0; i < qwords_len; i += REG_QWORDS) {
    va0 = LOAD(&a[i]);
    va1 = LOAD(&a[i + qwords_len]);
    vb0 = LOAD(&b[i]);
    vb1 = LOAD(&b[i + qwords_len]);

    STORE(&alah[i], va0 ^ va1);
    STORE(&blbh[i], vb0 ^ vb1);
  }
}

void karatzuba_add2(uint64_t *z,
                         const uint64_t *x,
                         const uint64_t *y,
                         const size_t    qwords_len)
{
  assert(qwords_len % REG_QWORDS == 0);

  REG_T vx, vy;

  for(size_t i = 0; i < qwords_len; i += REG_QWORDS) {
    vx = LOAD(&x[i]);
    vy = LOAD(&y[i]);

    STORE(&z[i], vx ^ vy);
  }
}

void karatzuba_add3(uint64_t *c,
                         const uint64_t *mid,
                         const size_t    qwords_len)
{
  assert(qwords_len % REG_QWORDS == 0);

  REG_T vr0, vr1, vr2, vr3, vt;

  uint64_t *c0 = c;
  uint64_t *c1 = &c[qwords_len];
  uint64_t *c2 = &c[2 * qwords_len];
  uint64_t *c3 = &c[3 * qwords_len];

  for(size_t i = 0; i < qwords_len; i += REG_QWORDS) {
    vr0 = LOAD(&c0[i]);
    vr1 = LOAD(&c1[i]);
    vr2 = LOAD(&c2[i]);
    vr3 = LOAD(&c3[i]);
    vt  = LOAD(&mid[i]);

    STORE(&c1[i], vt ^ vr0 ^ vr1);
    STORE(&c2[i], vt ^ vr2 ^ vr3);
  }
}

// c = a mod (x^r - 1)
void gf2x_red(GF2X *c, const TwoOfGF2X *a)
{
  const uint64_t *a64 = (const uint64_t *)a;
  uint64_t *      c64 = (uint64_t *)c;

  for(size_t i = 0; i < N_QWORDS; i += REG_QWORDS) {
    REG_T vt0 = LOAD(&a64[i]);
    REG_T vt1 = LOAD(&a64[i + N_QWORDS]);
    REG_T vt2 = LOAD(&a64[i + N_QWORDS - 1]);

    vt1 = SLLI_I64(vt1, LAST_N_QWORD_TRAIL);
    vt2 = SRLI_I64(vt2, LAST_N_QWORD_LEAD);

    vt0 ^= (vt1 | vt2);

    STORE(&c64[i], vt0);
  }

  c64[N_QWORDS - 1] &= LAST_N_QWORD_MASK;

  // Clean the secrets from the upper part of c
  //secure_clean((uint8_t *)&c64[N_QWORDS],
  //             (N_PADDED_QWORDS - N_QWORDS) * sizeof(uint64_t));
}

void gf2x_add_in_place(GF2X *c, GF2X *a) {
    uint64_t *a64 = (uint64_t *)a;
    uint64_t *c64 = (uint64_t *)c;

    __m256i a256;
    __m256i c256;

    for (int i = 0; i < N_QWORDS; i += 4) {
        a256 = _mm256_loadu_si256((const void *)&a64[i]);
        c256 = _mm256_loadu_si256((const void *)&c64[i]);
        _mm256_storeu_si256((void *)&c64[i], a256 ^ c256);
    }
}

void gf2x_add(GF2X *c, GF2X *a, GF2X *b) {
    memset(c->value, 0, sizeof(c->value));

    uint64_t *a64 = (uint64_t *)a;
    uint64_t *b64 = (uint64_t *)b;
    uint64_t *c64 = (uint64_t *)c;

    __m256i a256;
    __m256i b256;

    for (int i = 0; i < N_QWORDS; i += 4) {
        a256 = _mm256_loadu_si256((const void *)&a64[i]);
        b256 = _mm256_loadu_si256((const void *)&b64[i]);
        _mm256_storeu_si256((void *)&c64[i], a256 ^ b256);
    }
}


#define LOAD128(mem)       _mm_loadu_si128((const void *)(mem))
#define STORE128(mem, reg) _mm_storeu_si128((void *)(mem), (reg))
#define UNPACKLO(x, y)     _mm_unpacklo_epi64((x), (y))
#define UNPACKHI(x, y)     _mm_unpackhi_epi64((x), (y))
#define CLMUL(x, y, imm)   _mm_clmulepi64_si128((x), (y), (imm))
#define BSRLI(x, imm)      _mm_bsrli_si128((x), (imm))
#define BSLLI(x, imm)      _mm_bslli_si128((x), (imm))

// 4x4 Karatsuba multiplication
void gf2x_mul4_int(__m128i      c[4],
                            const __m128i a_lo,
                            const __m128i a_hi,
                            const __m128i b_lo,
                            const __m128i b_hi)
{
    // a_lo = [a1 | a0]; a_hi = [a3 | a2];
    // b_lo = [b1 | b0]; b_hi = [b3 | b2];
    // 4x4 Karatsuba requires three 2x2 multiplications:
    //   (1) a_lo * b_lo
    //   (2) a_hi * b_hi
    //   (3) aa * bb = (a_lo + a_hi) * (b_lo + b_hi)
    // Each of the three 2x2 multiplications requires three 1x1 multiplications:
    //   (1) is computed by a0*b0, a1*b1, (a0+a1)*(b0+b1)
    //   (2) is computed by a2*b2, a3*b3, (a2+a3)*(b2+b3)
    //   (3) is computed by aa0*bb0, aa1*bb1, (aa0+aa1)*(bb0+bb1)
    // All the required additions are performed in the end.

    __m128i aa, bb;
    __m128i xx, yy, uu, vv, m;
    __m128i lo[2], hi[2], mi[2];
    __m128i t[9];

    aa = a_lo ^ a_hi;
    bb = b_lo ^ b_hi;

    // xx <-- [(a2+a3) | (a0+a1)]
    // yy <-- [(b2+b3) | (b0+b1)]
    xx = UNPACKLO(a_lo, a_hi);
    yy = UNPACKLO(b_lo, b_hi);
    xx = xx ^ UNPACKHI(a_lo, a_hi);
    yy = yy ^ UNPACKHI(b_lo, b_hi);

    // uu <-- [ 0 | (aa0+aa1)]
    // vv <-- [ 0 | (bb0+bb1)]
    uu = aa ^ BSRLI(aa, 8);
    vv = bb ^ BSRLI(bb, 8);

    // 9 multiplications
    t[0] = CLMUL(a_lo, b_lo, 0x00);
    t[1] = CLMUL(a_lo, b_lo, 0x11);
    t[2] = CLMUL(a_hi, b_hi, 0x00);
    t[3] = CLMUL(a_hi, b_hi, 0x11);
    t[4] = CLMUL(xx, yy, 0x00);
    t[5] = CLMUL(xx, yy, 0x11);
    t[6] = CLMUL(aa, bb, 0x00);
    t[7] = CLMUL(aa, bb, 0x11);
    t[8] = CLMUL(uu, vv, 0x00);

    t[4] ^= (t[0] ^ t[1]);
    t[5] ^= (t[2] ^ t[3]);
    t[8] ^= (t[6] ^ t[7]);

    lo[0] = t[0] ^ BSLLI(t[4], 8);
    lo[1] = t[1] ^ BSRLI(t[4], 8);
    hi[0] = t[2] ^ BSLLI(t[5], 8);
    hi[1] = t[3] ^ BSRLI(t[5], 8);
    mi[0] = t[6] ^ BSLLI(t[8], 8);
    mi[1] = t[7] ^ BSRLI(t[8], 8);

    m = lo[1] ^ hi[0];

    c[0] = lo[0];
    c[1] = lo[0] ^ mi[0] ^ m;
    c[2] = hi[1] ^ mi[1] ^ m;
    c[3] = hi[1];
}

// 512x512bit multiplication performed by Karatsuba algorithm
// where a and b are considered as having 8 digits of size 64 bits.
void gf2x_mul_base_pclmul(uint64_t *c,
                          const uint64_t *a,
                          const uint64_t *b)
{
    __m128i va[4], vb[4];
    __m128i aa[2], bb[2];
    __m128i lo[4], hi[4], mi[4], m[2];

    for(size_t i = 0; i < 4; i++) {
        va[i] = LOAD128(&a[QWORDS_IN_XMM * i]);
        vb[i] = LOAD128(&b[QWORDS_IN_XMM * i]);
    }

    // Multiply the low and the high halves of a and b
    // lo <-- a_lo * b_lo
    // hi <-- a_hi * b_hi
    gf2x_mul4_int(lo, va[0], va[1], vb[0], vb[1]);
    gf2x_mul4_int(hi, va[2], va[3], vb[2], vb[3]);

    // Compute the middle multiplication
    // aa <-- a_lo + a_hi
    // bb <-- b_lo + b_hi
    // mi <-- aa * bb
    aa[0] = va[0] ^ va[2];
    aa[1] = va[1] ^ va[3];
    bb[0] = vb[0] ^ vb[2];
    bb[1] = vb[1] ^ vb[3];
    gf2x_mul4_int(mi, aa[0], aa[1], bb[0], bb[1]);

    m[0] = lo[2] ^ hi[0];
    m[1] = lo[3] ^ hi[1];

    STORE128(&c[0 * QWORDS_IN_XMM], lo[0]);
    STORE128(&c[1 * QWORDS_IN_XMM], lo[1]);
    STORE128(&c[2 * QWORDS_IN_XMM], mi[0] ^ lo[0] ^ m[0]);
    STORE128(&c[3 * QWORDS_IN_XMM], mi[1] ^ lo[1] ^ m[1]);
    STORE128(&c[4 * QWORDS_IN_XMM], mi[2] ^ hi[2] ^ m[0]);
    STORE128(&c[5 * QWORDS_IN_XMM], mi[3] ^ hi[3] ^ m[1]);
    STORE128(&c[6 * QWORDS_IN_XMM], hi[2]);
    STORE128(&c[7 * QWORDS_IN_XMM], hi[3]);
}

void gf2x_sqr_pclmul(TwoOfGF2X *c, const GF2X *a)
{
    __m128i va, vr0, vr1;

    const uint64_t *a64 = (const uint64_t *)a;
    uint64_t *      c64 = (uint64_t *)c;

    for(size_t i = 0; i < N; i += 64*QWORDS_IN_XMM) {
        va = LOAD128(&a64[i]);

        vr0 = CLMUL(va, va, 0x00);
        vr1 = CLMUL(va, va, 0x11);

        STORE128(&c64[i * 2], vr0);
        STORE128(&c64[i * 2 + QWORDS_IN_XMM], vr1);
    }
}
