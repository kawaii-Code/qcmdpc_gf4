#include <bits/stdc++.h>
#include <gmp.h>

#include "mdpc.hpp"

constexpr int Q = 5;

void write_t_random_nonzero_values(std::span<uint8_t> span, std::uint64_t t) {
    std::set<int> indices;
    while (indices.size() != t) {
        int idx = rand() % span.size();
        indices.insert(idx);
    }

    for (int idx : indices) {
        span[idx] = 1 + rand() % (Q - 1);
    }
}

int main() {
    int seed = time(0);
    srand(seed);
    std::println("Seed: {}", seed);

    CryptosystemParameters<Q> params;
    params.n = 12323;
    params.t = 132;
    params.gamma = 71;
    params.threshold1 = 5;
    params.threshold2 = (params.gamma + 1) / 2;

    QCMDPC_Context<Q> ctx(params);

    generate_keys(ctx);
    int iterations = 0;

try_again:
    auto encode_start_time = std::chrono::steady_clock::now();
    std::vector<uint8_t> error(2 * params.n);
    write_t_random_nonzero_values(error, params.t);
    Slice ciphertext = encode<Q>(ctx, Slice{error.data(), error.size()});
    std::println("Encode took: {}ms", milliseconds_passed_since(encode_start_time));

    auto decode_start_time = std::chrono::steady_clock::now();
    auto decode_result = decode(ctx, ciphertext);
    std::println("Decode took: {}ms", milliseconds_passed_since(decode_start_time));

    auto result = decode_result.decoded_error;
    bool success = decode_result.success;
    iterations += decode_result.iterations;
    assert(success);
    if (!success) {
        for (int i = 0; i < 2 * params.n; i++) {
            if (error[i] != result[i]) {
                std::cout << "e[" << i << "] = " << error[i] << std::endl;
                std::cout << "result[" << i << "] = " << result[i] << std::endl;
            }
        }
        std::cout << "Iterations = " << iterations << std::endl;
        std::cout << std::endl;
        goto try_again;
    }

    bool not_equal = false;
    for (int i = 0; i < 2 * params.n; i++) {
        if (error[i] != result[i]) {
            std::cout << "error[" << i << "] = " << error[i] << "  !=  "
                      << "result[" << i << "] = " << result[i] << std::endl;
            not_equal = true;
        }
    }

    std::cout << "Finished in " << iterations << " iterations" << std::endl;

    return 0;
}
