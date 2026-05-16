#include <catch2/catch_amalgamated.hpp>

#include "mdpc.hpp"


///////////////////////////////////////////////////////////
// Positive modulus
//
TEST_CASE("Positive modulus - basic operations") {
    REQUIRE(pmod(7, 3) == 1);
    REQUIRE(pmod(10, 5) == 0);
    REQUIRE(pmod(3, 7) == 3);
}

TEST_CASE("Positive modulus - negative inputs") {
    REQUIRE(pmod(-1, 5) == 4);
    REQUIRE(pmod(-7, 3) == 2);
    REQUIRE(pmod(-10, 5) == 0);
}

TEST_CASE("Positive modulus - edge cases") {
    REQUIRE(pmod(0, 1) == 0);
    REQUIRE(pmod(0, 5) == 0);
    REQUIRE(pmod(5, 1) == 0);
}

TEST_CASE("Positive modulus - large inputs") {
    REQUIRE(pmod(100, 7) == 2);
    REQUIRE(pmod(-100, 7) == 5);
}

///////////////////////////////////////////////////////////
// Multiplication and division
//
TEST_CASE("Naive inverse basic") {

    SECTION("F_2") {
        constexpr int Q = 2;
        CryptosystemParameters<Q> params;
        init<Q>(params);
        auto div = params.division_table;
        REQUIRE(div[1][1] == 1);
    }

    SECTION("F_5") {
        constexpr int Q = 5;
        CryptosystemParameters<Q> params;
        init<Q>(params);
        auto div = params.division_table;
        REQUIRE(div[1][1] == 1);
        REQUIRE(div[1][2] == 3);
        REQUIRE(div[1][3] == 2);
        REQUIRE(div[1][4] == 4);
    }

    SECTION("F_7") {
        constexpr int Q = 7;
        CryptosystemParameters<Q> params;
        init<Q>(params);
        auto div = params.division_table;
        REQUIRE(div[1][1] == 1);
        REQUIRE(div[1][2] == 4);
        REQUIRE(div[1][3] == 5);
        REQUIRE(div[1][4] == 2);
        REQUIRE(div[1][5] == 3);
        REQUIRE(div[1][6] == 6);
    }

    SECTION("F_11") {
        constexpr int Q = 11;
        CryptosystemParameters<Q> params;
        init<Q>(params);
        auto div = params.division_table;
        REQUIRE(div[1][1] == 1);
        REQUIRE(div[1][2] == 6);
        REQUIRE(div[1][5] == 9);
        REQUIRE(div[1][7] == 8);
        REQUIRE(div[1][10] == 10);
    }
}

TEST_CASE("Naive inverse edge cases") {
    //SECTION("F_13 - Test 0 has no inverse") {
    //    constexpr int Q = 13;
    //    REQUIRE_THROWS(naive_inverse<Q>(0));
    //}

    SECTION("F_17 - Test all non-zero elements") {
        constexpr int Q = 17;
        CryptosystemParameters<Q> params;
        init<Q>(params);
        auto div = params.division_table;
        for (int a = 1; a < Q; ++a) {
            int inv = div[1][a];
            REQUIRE(inv >= 0);
            REQUIRE(inv < Q);
            REQUIRE(((a * inv) % Q) == 1);
        }
    }
}

TEST_CASE("Multiplication table - basic operations") {
    constexpr int Q = 5;
    CryptosystemParameters<Q> params;
    init<Q>(params);
    auto mul = params.multiplication_table;
    auto div = params.division_table;

    REQUIRE(mul[2][0] == 0);
    REQUIRE(mul[2][1] == 2);
    REQUIRE(mul[2][3] == 1);
    REQUIRE(mul[3][4] == 2);
    REQUIRE(mul[1][2] == 2);
    REQUIRE(mul[4][4] == 1);
}

TEST_CASE("Division table - basic operations") {
    constexpr int Q = 5;
    CryptosystemParameters<Q> params;
    init<Q>(params);
    auto mul = params.multiplication_table;
    auto div = params.division_table;

    REQUIRE(div[0][1] == 0);
    REQUIRE(div[2][2] == 1);
    REQUIRE(div[2][1] == 2);
    REQUIRE(div[1][2] == 3);
    REQUIRE(div[2][3] == 4);
    REQUIRE(div[4][2] == 2);
    REQUIRE(div[4][3] == 3);
}

TEST_CASE("Division table - consistency") {
    constexpr int Q = 11;
    CryptosystemParameters<Q> params;
    init<Q>(params);
    auto mul = params.multiplication_table;
    auto div = params.division_table;

    for (int i = 1; i < Q; i++) {
        for (int j = 1; j < Q; j++) {
            REQUIRE(mul[i][div[j][i]] == j);
        }
    }
}
