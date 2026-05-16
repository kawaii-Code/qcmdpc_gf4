#include <catch2/catch_amalgamated.hpp>

#include "mdpc.hpp"

constexpr int Q = 5;


Polynomial<Q> from_initializer_list(std::vector<int> a) {
    int n = a.size();
    Polynomial<Q> result(n);
    for (int i = 0; i < n; i++) {
        result[i] = a[i] % Q;
    }
    return result;
}

TEST_CASE("FFT is equivalent to a convolution when polynomial has power of 2 length") {
    Polynomial<Q> a = from_initializer_list({ 3, 1, 0, 4, 2, 0, 1, 1 });
    Polynomial<Q> b = from_initializer_list({ 4, 0, 1, 2, 3, 3, 1, 3 });
    int n = a.size();

    Polynomial<Q> basic = polynomial_multiply(a, b, n);
    Polynomial<Q> with_fft = polynomial_multiply_fft(a, b);

    for (int i = 0; i < n; i++) {
        REQUIRE(basic[i] == with_fft[i]);
    }
}

TEST_CASE("FFT is equivalent to a convolution with arbitrary polynomials") {
    Polynomial<Q> a = from_initializer_list({ 3, 1, 0, 4, 2 });
    Polynomial<Q> b = from_initializer_list({ 4, 0, 1, 2, 3 });
    int n = a.size();

    Polynomial<Q> basic = polynomial_multiply(a, b, n);
    Polynomial<Q> with_fft = polynomial_multiply_fft(a, b);

    REQUIRE_THAT(basic.v, Catch::Matchers::Equals(with_fft.v));
}
