#include <catch2/catch_amalgamated.hpp>

#include "mdpc.hpp"

///////////////////////////////////////////////////////////
// Basic
//
TEST_CASE("Polynomial degree and weight") {
    constexpr int Q = 7;

    Simple_Vector<Q> v(4);
    REQUIRE(polynomial_degree(v) == 0);
    REQUIRE(polynomial_weight(v) == 0);
    v[0] = 1;
    REQUIRE(polynomial_degree(v) == 0);
    REQUIRE(polynomial_weight(v) == 1);
    v[1] = 1;
    REQUIRE(polynomial_degree(v) == 1);
    REQUIRE(polynomial_weight(v) == 2);
    v[2] = 1;
    REQUIRE(polynomial_degree(v) == 2);
    REQUIRE(polynomial_weight(v) == 3);
    v[3] = 1;
    REQUIRE(polynomial_degree(v) == 3);
    REQUIRE(polynomial_weight(v) == 4);
    v[2] = 0;
    v[0] = 0;
    REQUIRE(polynomial_degree(v) == 3);
    REQUIRE(polynomial_weight(v) == 2);
}

///////////////////////////////////////////////////////////
// Polynomial Inverse
//
TEST_CASE("Polynomial inverse - identity polynomial") {
    constexpr int Q = 7;
    const int n = 5;
    CryptosystemParameters<Q> params = {.n = n};
    init<Q>(params);

    Polynomial<Q> identity(n);
    identity[0] = 1;

    auto inv = polynomial_inverse(params, identity);

    REQUIRE(inv.size() == n);
    REQUIRE(inv[0] == 1);
    for (int i = 1; i < n; i++) {
        REQUIRE(inv[i] == 0);
    }
}

TEST_CASE("Polynomial inverse - constant polynomial") {
    constexpr int Q = 13;
    const int n = 8;
    CryptosystemParameters<Q> params = {.n = n};
    init<Q>(params);

    Polynomial<Q> constant(n);
    constant[0] = 5;

    auto inv = polynomial_inverse(params, constant);

    REQUIRE(inv.size() == n);
    REQUIRE(inv[0] == 8);
    for (int i = 1; i < n; i++) {
        REQUIRE(inv[i] == 0);
    }
}

// NON INVERTIBLE
//TEST_CASE("Polynomial inverse - linear polynomial") {
//    constexpr int Q = 5;
//    const int n = 4;
//    CryptosystemParameters<Q> params = {.n = n};
//    init<Q>(params);
//
//    Polynomial<Q> linear(n);
//    linear[0] = 2;
//    linear[1] = 3;
//
//    auto inv = polynomial_inverse(params, linear);
//    std::cout << "inv = " << inv << std::endl;
//
//    Polynomial<Q> product(n);
//    for (int i = 0; i < n; i++) {
//        for (int j = 0; j < n; j++) {
//            product[(i + j) % n] = (product[(i + j) % n] + linear[i] * inv[j]) % Q;
//        }
//    }
//    std::cout << product << std::endl;
//
//    REQUIRE(product[0] == 1);
//    for (int i = 1; i < n; i++) {
//        REQUIRE(product[i] == 0);
//    }
//}

//TEST_CASE("Polynomial inverse - non-invertible polynomial") {
//    constexpr int Q = 7;
//    const int n = 6;
//    CryptosystemParameters<Q> params = {.n = n};
//    init<Q>(params);
//
//    Polynomial<Q> zero_poly(n);
//    zero_poly[0] = 0;
//
//    REQUIRE_THROWS(polynomial_inverse(params, zero_poly));
//}

TEST_CASE("Polynomial inverse - random polynomial") {
    constexpr int Q = 11;
    const int n = 7;
    CryptosystemParameters<Q> params = {.n = n};
    init<Q>(params);

    Polynomial<Q> random_poly(n);
    random_poly[0] = 3;
    random_poly[1] = 8;
    random_poly[3] = 5;
    random_poly[5] = 2;

    auto inv = polynomial_inverse(params, random_poly);

    Polynomial<Q> product(n + 1);
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            int idx = i + j;
            product[idx % n] = (product[idx % n] + random_poly[i] * inv[j]) % Q;
        }
    }

    REQUIRE(product[0] == 1);
    for (int i = 1; i < n; i++) {
        REQUIRE(product[i] == 0);
    }
}


///////////////////////////////////////////////////////////
// Convolutions
//
TEST_CASE("Convolve basic - zero vectors") {
    constexpr int Q = 7;
    Simple_Vector<Q> a(5);
    Simple_Vector<Q> b(5);
    Simple_Vector<Q> c(5);

    convolve_basic(c, a, b);

    for (int i = 0; i < 5; i++) {
        REQUIRE(c[i] == 0);
    }
}

TEST_CASE("Convolve basic - constant vectors") {
    constexpr int Q = 13;
    int n = 4;

    Simple_Vector<Q> a(n);
    for (int i = 0; i < n; i++) {
        a[i] = 3;
    }

    Simple_Vector<Q> b(n);
    for (int i = 0; i < n; i++) {
        b[i] = 5;
    }

    Simple_Vector<Q> c(n);
    convolve_basic(c, a, b);

    int expected = (3 * 5 * 4) % Q;
    for (int i = 0; i < n; i++) {
        REQUIRE(c[i] == expected);
    }
}

TEST_CASE("Convolve basic - linear vectors") {
    constexpr int Q = 5;
    int n = 4;

    Simple_Vector<Q> a(n);
    a[0] = 1;
    a[1] = 2;

    Simple_Vector<Q> b(n);
    b[0] = 3;
    b[1] = 4;

    Simple_Vector<Q> c(n);
    convolve_basic(c, a, b);

    REQUIRE(c[0] == 3);
    REQUIRE(c[1] == 0);
    REQUIRE(c[2] == 3);
    REQUIRE(c[3] == 0);
}

TEST_CASE("Convolve basic - random vectors") {
    constexpr int Q = 11;
    int n = 4;

    Simple_Vector<Q> a(n);
    a[0] = 2;
    a[1] = 3;
    a[2] = 5;
    a[3] = 7;

    Simple_Vector<Q> b(n);
    b[0] = 1;
    b[1] = 4;
    b[2] = 6;
    b[3] = 8;

    Simple_Vector<Q> c(n);
    convolve_basic(c, a, b);

    REQUIRE(c[0] == 7);
    REQUIRE(c[1] == 5);
    REQUIRE(c[2] == 8);
    REQUIRE(c[3] == 6);
}

TEST_CASE("Convolve basic - modulo wraparound") {
    constexpr int Q = 17;
    int n = 4;

    Simple_Vector<Q> a(n);
    a[n - 1] = 1;

    Simple_Vector<Q> b(n);
    b[n - 1] = 1;

    Simple_Vector<Q> c(n);

    convolve_basic(c, a, b);

    REQUIRE(c[2] == 1);
    for (int i = 0; i < n; i++) {
        if (i != 2) {
            REQUIRE(c[i] == 0);
        }
    }
}
