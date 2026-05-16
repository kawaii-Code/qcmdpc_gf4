#include <print>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <random>

extern "C" {
#include "api.h"
#include "kem.h"
#include "gf2x.h"
}

auto now() {
    return std::chrono::steady_clock::now();
}

double elapsed_ms(std::chrono::steady_clock::time_point start) {
    return std::chrono::duration_cast<std::chrono::microseconds>(now() - start).count() / 1000.0;
}

void fill_random_pad_r(pad_r_t *a) {
    memset(a, 0, sizeof(*a));
    for (int i = 0; i < (int)(sizeof(pad_r_t)); i++) {
        ((uint8_t *)a)[i] = rand() & 0xFF;
    }
    a->val.raw[R_BYTES - 1] &= LAST_R_BYTE_MASK;
}

//void fill_random_pad_r(pad_r_t *a) {
//    static std::mt19937_64 rng(std::random_device{}());
//    uint64_t *ptr = (uint64_t*)a;
//    for (size_t i = 0; i < sizeof(pad_r_t) / sizeof(uint64_t); i++) {
//        ptr[i] = rng();
//    }
//}

void run_gf2x_mod_benchmark(const char* level_label, int iterations) {
    std::println("GF2X Operations");

    double inv_total = 0;
    double mul_total = 0;

    pad_r_t a, b, c;

    for (int i = 0; i < iterations; i++) {
        fill_random_pad_r(&a);
        fill_random_pad_r(&b);

        auto t0 = now();
        gf2x_mod_inv(&c, &a);
        inv_total += elapsed_ms(t0);

        auto t1 = now();
        gf2x_mod_mul(&c, &a, &b);
        mul_total += elapsed_ms(t1);
    }

    std::println("gf2x_mod_inv: avg {:.6f} ms (total {:.3f} ms)", inv_total / iterations, inv_total);
    std::println("gf2x_mod_mul: avg {:.6f} ms (total {:.3f} ms)", mul_total / iterations, mul_total);
}

void run_kem_benchmark(const char* level_label, int iterations) {
    std::println("KEM Operations");

    alignas(32) uint8_t pk[CRYPTO_PUBLICKEYBYTES];
    alignas(32) uint8_t sk[CRYPTO_SECRETKEYBYTES];
    alignas(32) uint8_t ct[CRYPTO_CIPHERTEXTBYTES];
    alignas(32) uint8_t ss_enc[CRYPTO_BYTES];
    alignas(32) uint8_t ss_dec[CRYPTO_BYTES];

    double keypair_total = 0;
    double enc_total = 0;
    double dec_total = 0;

    for (int i = 0; i < iterations; i++) {
        auto t0 = now();
        int rc = crypto_kem_keypair(pk, sk);
        keypair_total += elapsed_ms(t0);
        if (rc != 0) {
            std::println("  Iteration {}: keypair failed with rc={}", i, rc);
            return;
        }

        auto t1 = now();
        rc = crypto_kem_enc(ct, ss_enc, pk);
        enc_total += elapsed_ms(t1);
        if (rc != 0) {
            std::println("  Iteration {}: enc failed with rc={}", i, rc);
            return;
        }

        auto t2 = now();
        rc = crypto_kem_dec(ss_dec, ct, sk);
        dec_total += elapsed_ms(t2);
        if (rc != 0) {
            std::println("  Iteration {}: dec failed with rc={}", i, rc);
            return;
        }

        if (std::memcmp(ss_enc, ss_dec, CRYPTO_BYTES) != 0) {
            std::println("  Iteration {}: shared secret mismatch!", i);
            return;
        }
    }

    std::println("Keypair: avg {:.3f} ms (total {:.3f} ms)", keypair_total / iterations, keypair_total);
    std::println("Encaps:  avg {:.3f} ms (total {:.3f} ms)", enc_total / iterations, enc_total);
    std::println("Decaps:  avg {:.3f} ms (total {:.3f} ms)", dec_total / iterations, dec_total);
}

int main() {
    int iterations = 10;
    const char* label = std::to_string(LEVEL).c_str();

    std::println("LEVEL = {}", LEVEL);
    std::println("Iterations = {}", iterations);
    std::println("N = {}", R_BITS);
    std::println("");
    run_gf2x_mod_benchmark(label, iterations);
    std::println("");
    run_kem_benchmark(label, iterations);
}
