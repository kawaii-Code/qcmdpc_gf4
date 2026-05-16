#pragma once

#include <bits/stdc++.h>
#include <numbers>
#include <gmp.h>

struct Slice {
    std::uint8_t *ptr;
    std::uint64_t len;
};

#include "sparse_vector.hpp"

using Complex = std::complex<double>;

template <int Q>
struct CryptosystemParameters {
    int n;
    int t;
    int gamma;
    int threshold1;
    int threshold2;
};

template <int Q>
struct QCMDPC_Context {
    QCMDPC_Context(const CryptosystemParameters<Q> &_params)
        : params(_params)
    {
        for (int i = 0; i < Q; i++) {
            for (int j = 0; j < Q; j++) {
                multiplication_table[i][j] = (i * j) % Q;
            }
        }
        for (int i = 0; i < Q; i++) {
            for (int j = 0; j < Q; j++) {
                division_table[i][j] = 0;
                for (int k = 0; k < Q; k++) {
                    if (multiplication_table[j][k] == i) {
                        division_table[i][j] = k;
                    }
                }
            }
        }

        public_key_memory = new std::uint8_t[params.n];
        public_key = {
            public_key_memory,
            params.n,
        };

        // Hardcoded for n = 16651
        mpz_init_set_str(power, "35257702653609952657602775022924737085467046019577150863503298212415213246134495771475254130085054317114235538499206028916789871549094170454420291794818820648169907044092640106352103595795405395822749798542245071121327636886045841800350766903472042576547439533472335899053188585382862667785042312246508252621875778483817086320991258301436792215463047532423861530694918345924001927412290776525058591792569910007148557550180447951995890074208047133851362332080779194884701795805982577387865449808997862029039093139437806941744994772453193295143209910417946156925228711811258487619516804390635970431750652602682916114673616903019137680530548095703123", 10);
    }

    CryptosystemParameters<Q> params;

    std::uint8_t *public_key_memory;
    Slice public_key;

    Sparse_Vector<Q> private_key_v;
    Sparse_Vector<Q> private_key_w;

    mpz_t power;

    std::array<std::array<int, Q>, Q> multiplication_table = {};
    std::array<std::array<int, Q>, Q> division_table = {};
};

template <int Q>
struct DecodeResult {
    Simple_Vector<Q> decoded_error;
    bool success;
    int iterations;
};

template <int Q>
struct PublicKey {
    Simple_Vector<Q> key;
};

template <int Q>
struct PrivateKey {
    Sparse_Vector<Q> v;
    Sparse_Vector<Q> w;
};

int pmod(int a, int m);
int milliseconds_passed_since(std::chrono::time_point<std::chrono::steady_clock> time_point);
int round_to_next_pow2(int x);

template <int Q>
using Polynomial = Simple_Vector<Q>;
template <int Q>
int polynomial_weight(Simple_Vector<Q> &v) {
    int weight = 0;
    for (int i = 0; i < v.size(); i++) {
        if (v[i] != 0) {
            weight += 1;
        }
    }
    return weight;
}

template <int Q>
int polynomial_degree(const Polynomial<Q> &p) {
    int deg = 0;
    for (int i = 0; i < p.size(); i++) {
        if (p[i] != 0) {
            deg = i;
        }
    }
    return deg;
}

static uint64_t ops = 0;
static uint64_t calls = 0;

template <int Q>
Polynomial<Q> polynomial_multiply(const Polynomial<Q> &a, const Polynomial<Q> &b, int max_n) {
    int degree_a = polynomial_degree(a);
    int degree_b = polynomial_degree(b);

    int n = a.size();

    int result_degree = std::min(degree_a + degree_b, max_n - 1);
    Polynomial<Q> result(max_n);

    ops += (result_degree + 1) * (std::min(degree_a, degree_b) + 1);

    if (degree_b < degree_a) {
        for (int i = 0; i <= result_degree; i++) {
            for (int j = 0; j <= degree_b; j++) {
                int c = b[j] * a[pmod(i - j, max_n)];
                result[i] = (result[i] + c % Q) % Q;
            }
        }
    } else {
        for (int i = 0; i <= result_degree; i++) {
            for (int j = 0; j <= degree_a; j++) {
                int c = a[j] * b[pmod(i - j, max_n)];
                result[i] = (result[i] + c % Q) % Q;
            }
        }
    }

    std::cout << "ops = " << ops << std::endl;

    return result;
}

template <int Q>
std::vector<Complex> fft(const Polynomial<Q> x) {
    int n = x.size();
    assert((n & (n - 1)) == 0);
    if (n == 1) {
        return std::vector<Complex>{Complex{(double)x[0], 0.0}};
    }

    int nhalf = n / 2;
    Polynomial<Q> even(nhalf);
    Polynomial<Q> odd(nhalf);
    for (int i = 0; i < nhalf; i++) {
        even[i] = x[2*i];
        odd[i] = x[2*i + 1];
    }

    std::vector<Complex> ye = fft(even);
    std::vector<Complex> yo = fft(odd);

    const Complex I = Complex(0, 1);
    const Complex omega_step = std::exp((2 * std::numbers::pi * I) / (double)n);
    Complex omega(1.0, 0.0);

    std::vector<Complex> result(n);
    for (int i = 0; i < nhalf; i++) {
        result[i] = ye[i] + omega * yo[i];
        result[i + nhalf] = ye[i] - omega * yo[i];
        omega *= omega_step;
    }

    return result;
}

std::vector<Complex> ifft_recursive(const std::vector<Complex> &x) {
    int n = x.size();
    assert((n & (n - 1)) == 0);
    if (n == 1) {
        return x;
    }

    int nhalf = n / 2;
    std::vector<Complex> even(nhalf);
    std::vector<Complex> odd(nhalf);
    for (int i = 0; i < nhalf; i++) {
        even[i] = x[2*i];
        odd[i] = x[2*i + 1];
    }

    std::vector<Complex> ye = ifft_recursive(even);
    std::vector<Complex> yo = ifft_recursive(odd);

    const Complex I = Complex(0, 1);
    const Complex omega_step = std::exp(-(2 * std::numbers::pi * I) / (double)n);
    Complex omega(1.0, 0.0);

    std::vector<Complex> result(n);
    for (int i = 0; i < nhalf; i++) {
        result[i] += (ye[i] + omega * yo[i]);
        result[i + nhalf] += (ye[i] - omega * yo[i]);
        omega *= omega_step;
    }

    return result;
}

std::vector<Complex> ifft(const std::vector<Complex> &x) {
    std::vector<Complex> result = ifft_recursive(x);
    for (int i = 0; i < result.size(); i++) {
        result[i] /= x.size();
    }
    return result;
}

void print_complex_vector(const std::vector<Complex> &v) {
    if (v.size() == 0) {
        std::cout << "[]" << std::endl;
        return;
    }

    std::cout << "[";
    for (int i = 0; i < v.size() - 1; i++) {
        std::cout << v[i] << " ";
    }
    std::cout << v[v.size() - 1] << "]" << std::endl;
}


std::vector<int> ifft_simple(std::vector<Complex> x) {
    int n = x.size();
    const Complex I = Complex(0, 1);

    std::vector<Complex> result(n);
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            Complex exp = (I * -(double)(2 * std::numbers::pi * j * i)) / (double)n;
            result[i] += x[j] * std::exp(exp);
        }
        result[i] /= (double)n;
    }

    std::vector<int> result_int(n);
    for (int i = 0; i < n; i++) {
        result_int[i] = (int)std::round(result[i].real());
    }
    return result_int;
}

template <int Q>
std::vector<Complex> fft_simple(const Polynomial<Q> &x) {
    int n = x.size();
    const Complex I = Complex(0, 1);

    std::vector<Complex> result(n);
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            Complex exp = (I * (double)(2 * std::numbers::pi * j * i)) / (double)n;
            result[i] += (double)x[j] * std::exp(exp);
        }
    }

    return result;
}

template <int Q>
Polynomial<Q> polynomial_multiply_fft(const Polynomial<Q> &a, const Polynomial<Q> &b) {
    calls += 1;
    std::cout << "calls = " << calls << std::endl;

    int n = a.size();
    n = round_to_next_pow2(n);
    n *= 2;

    Polynomial<Q> a_padded(n);
    Polynomial<Q> b_padded(n);
    for (int i = 0; i < a.size(); i++) {
        a_padded[i] = a[i];
    }
    for (int i = 0; i < b.size(); i++) {
        b_padded[i] = b[i];
    }

    std::vector<Complex> a_fft = fft(a_padded);
    std::vector<Complex> b_fft = fft(b_padded);

    for (int i = 0; i < n; i++) {
        a_fft[i] *= b_fft[i];
    }

    std::vector<Complex> product = ifft(a_fft);

    Polynomial<Q> result(a.size());
    for (int i = 0; i < n; i++) {
        result[i % a.size()] += (std::uint8_t)((std::uint64_t)std::round(product[i].real()) % Q);
        result[i % a.size()] %= Q;
    }

    return result;
}

template <int Q>
Polynomial<Q> quick_polynomial_pow(const CryptosystemParameters<Q> &params, Polynomial<Q> a, mpz_t power) {
    int cmp1 = mpz_cmp_ui(power, 1);
    if (cmp1 == 0) {
        mpz_clear(power);
        return a;
    } else if (cmp1 < 0) {
        mpz_clear(power);
        Polynomial<Q> one(params.n);
        one[0] = 1;
        return one;
    }

    mpz_t power_div2;
    mpz_init(power_div2);

    int remainder = mpz_tstbit(power, 0);
    mpz_tdiv_q_2exp(power_div2, power, 1);

    mpz_clear(power);

    Polynomial<Q> p = quick_polynomial_pow(params, a, power_div2);
    if (remainder == 1) {
        return polynomial_multiply_fft(polynomial_multiply_fft(p, p), a);
    } else {
        return polynomial_multiply_fft(p, p);
    }
}

template <int Q>
Polynomial<Q> polynomial_inverse(const QCMDPC_Context<Q> &ctx, const Polynomial<Q> &p) {
    const CryptosystemParameters<Q> &params = ctx.params;
    assert(p.size() == params.n);

    bool non_invertible = true;
    for (int i = 0; i < p.size(); i++) {
        if (p[i] != 0) {
            non_invertible = false;
            break;
        }
    }
    assert(!non_invertible);

    int n = params.n;

    // v_0 = 0
    // v_1 = 1
    Polynomial<Q> v_old(n);
    Polynomial<Q> v_new(n);
    Polynomial<Q> v(n);
    v[0] = 1;

    // a = x^n - 1
    Polynomial<Q> a(n + 1);
    a[n] = 1;
    a[0] = Q - 1;

    Polynomial<Q> b = p;

    Polynomial<Q> q(n + 1);

    // ~12000 Iterations on n = 16651
    // qdeg on average ~2 (median 1)
    int adeg = n;
    int bdeg = polynomial_degree(b);
    while (true) {
        int qdeg = adeg - bdeg;
        for (int i = 0; i <= qdeg; i++) {
            q[i] = 0;
        }

        assert(adeg >= bdeg);
        do {
            int qq = ctx.division_table[a[adeg]][b[bdeg]];
            q[adeg - bdeg] = qq;
            for (int i = adeg - bdeg, j = 0; i <= adeg; i++, j++) {
                a[i] = pmod(a[i] - (qq * b[j]) % Q, Q);
            }
            adeg = polynomial_degree(a);

            bool all_zero = true;
            for (int i = 0; i <= adeg; i++) {
                if (a[i] != 0) {
                    all_zero = false;
                    break;
                }
            }
            if (all_zero) {
                int qq = ctx.division_table[1][b[0]];
                for (int i = 0; i < n; i++) {
                    v[i] = (v[i] * qq) % Q;
                }
                return v;
            }
        } while (adeg >= bdeg);

        for (int i = 0; i <= bdeg; i++) {
            std::swap(a[i], b[i]);
        }
        std::swap(adeg, bdeg);

        for (int i = 0; i < n; i++) {
            v_new[i] = v_old[i];
        }
        for (int i = 0; i < n; i++) {
            for (int j = 0; j <= qdeg; j++) {
                v_new[i] = pmod(v_new[i] - (v[pmod(i - j, n)] * q[j]) % Q, Q);
            }
        }

        v_old = v;
        v = v_new;
    }
}

template <int Q>
void convolve_sparse(Simple_Vector<Q> &c, const Sparse_Vector<Q> &a, const Simple_Vector<Q> &b) {
    assert(a.total_size() == b.size());
    assert(a.total_size() == c.size());

    int n = c.size();
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < a.sparse_size(); j++) {
            int cc = pmod(a.values[j] * b[pmod(i - a.indices[j], n)], Q);
            c[i] = pmod(c[i] + cc, Q);
        }
    }
}

template <int Q>
void convolve_basic(Simple_Vector<Q> &c, const Simple_Vector<Q> &a, const Simple_Vector<Q> &b) {
    assert(a.size() == b.size());
    assert(a.size() == c.size());

    int n = c.size();
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            int cc = pmod(a[j] * b[pmod(i - j, n)], Q);
            c[i] = pmod(c[i] + cc, Q);
        }
    }
}

template <int Q>
int counter_argmax(const std::vector<int> &counter) {
    int result = 1;
    for (int i = 2; i < Q; i++) {
        if (counter[i] > counter[result]) {
            result = i;
        }
    }
    return result;
}

template <int Q>
void generate_keys(QCMDPC_Context<Q> &ctx) {
    int n = ctx.params.n;
    int gamma = ctx.params.gamma;

try_again:
    auto v = Sparse_Vector<Q>::random_vector_of_weight2(gamma, n);
    auto w = Sparse_Vector<Q>::random_vector_of_weight2(gamma, n);
    PrivateKey private_key = { v, w };

    Simple_Vector<Q> non_sparse_v(n);
    for (int i = 0; i < v.sparse_size(); i++) {
        non_sparse_v[v.indices[i]] = v.values[i];
    }

    auto start = std::chrono::steady_clock::now();
    Simple_Vector<Q> v_inv = polynomial_inverse(ctx, non_sparse_v);
    std::cout << "polynomial_inverse took " << milliseconds_passed_since(start) << "ms" << std::endl;

    //start = std::chrono::steady_clock::now();
    //Simple_Vector<Q> v_inv_2 = quick_polynomial_pow(ctx.params, non_sparse_v, ctx.power);
    //std::cout << "quick_polynomial_pow took " << milliseconds_passed_since(start) << "ms" << std::endl;
    //for (int i = 0; i < v_inv.size(); i++) {
    //    assert(v_inv[i] == v_inv_2[i]);
    //}

    Simple_Vector<Q> v_v_inv(n);
    convolve_sparse(v_v_inv, v, v_inv);
    int degree = polynomial_degree(v_v_inv);
    if (degree != 0 || v_v_inv[0] != 1) {
        // Not an inverse
        goto try_again;
    }

    Simple_Vector<Q> c(n);
    convolve_sparse(c, w, v_inv);
    for (int i = 0; i < n; i++) {
        ctx.public_key.ptr[i] = c[i];
    }

    ctx.private_key_v = v;
    ctx.private_key_w = w;
}


template <int Q>
Slice encode(QCMDPC_Context<Q> &ctx, Slice plaintext) {
    int n = ctx.params.n;

    // The keys should already be generated
    assert(plaintext.len == 2 * n);

    std::uint8_t *result_memory = new std::uint8_t[n];
    Slice result = {result_memory, (std::uint64_t)n};
    for (int i = 0; i < n; i++) {
        result.ptr[i] = plaintext.ptr[i];
    }

    std::uint8_t *plaintext_half2 = plaintext.ptr + n;
    Slice public_key = ctx.public_key;

    // FFT variant, which is faster, but is not suitable for cryptography
    //Polynomial<Q> pubkey_poly(public_key.len);
    //for (int i = 0; i < public_key.len; i++) {
    //    pubkey_poly[i] = public_key.ptr[i];
    //}
    //Polynomial<Q> plaintext_poly(n);
    //for (int i = 0; i < n; i++) {
    //    plaintext_poly[i] = plaintext_half2[i];
    //}
    //Polynomial<Q> product = polynomial_multiply_fft(pubkey_poly, plaintext_poly);
    //for (int i = 0; i < n; i++) {
    //    product[i] += plaintext.ptr[i];
    //    product[i] %= Q;
    //}
    //for (int i = 0; i < n; i++) {
    //    result.ptr[i] = product[i];
    //}

    for (int i = 0; i < n; i++) {
        for (int j = 0; j <= i; j++) {
            std::uint8_t c = public_key.ptr[j] * plaintext_half2[i - j];
            result.ptr[i] = pmod(result.ptr[i] + c, Q);
        }
        for (int j = i + 1; j < n; j++) {
            std::uint8_t c = public_key.ptr[j] * plaintext_half2[n - (j - i)];
            result.ptr[i] = pmod(result.ptr[i] + c, Q);
        }
    }

    // Simpler code, but runs x1.5 slower
    //for (int i = 0; i < n; i++) {
    //    for (int j = 0; j < n; j++) {
    //        int c = pmod(key.key[j] * plaintext[n + pmod(i - j, n)], Q);
    //        result[i] = pmod(result[i] + c, Q);
    //    }
    //}

    return result;
}

template <int Q>
DecodeResult<Q> decode(const QCMDPC_Context<Q> &ctx, Slice ciphertext_) {
    int n = ctx.params.n;
    const CryptosystemParameters<Q> &params = ctx.params;

    DecodeResult<Q> result = {
        .decoded_error = Simple_Vector<Q>(2 * n),
        .success = false,
        .iterations = 0,
    };

    const Sparse_Vector<Q> &v = ctx.private_key_v;
    const Sparse_Vector<Q> &w = ctx.private_key_w;

    Simple_Vector<Q> ciphertext(n);
    for (int i = 0; i < n; i++) {
        ciphertext[i] = ciphertext_.ptr[i];
    }

    Simple_Vector<Q> et(2 * n);
    Simple_Vector<Q> s_orig(n);
    convolve_sparse(s_orig, v, ciphertext);
    Simple_Vector<Q> s = s_orig;

    result.iterations += 1;
    for (int i = 0; i < n; i++) {
        std::vector<int> counter(Q, 0);
        for (int j = 0; j < v.sparse_size(); j++) {
            int inv = ctx.division_table[1][v.values[j]];
            int diff = (s[pmod(v.indices[j] + i, n)] * inv) % Q;
            counter[diff] += 1;
        }

        int mxi = counter_argmax<Q>(counter);
        if (counter[mxi] - counter[0] >= params.threshold1) {
            et[i] = mxi;
        }
    }
    for (int i = n; i < 2 * n; i++) {
        std::vector<int> counter(Q, 0);
        for (int j = 0; j < w.sparse_size(); j++) {
            int inv = ctx.division_table[1][w.values[j]];
            int diff = (s[pmod(i + w.indices[j], n)] * inv) % Q;
            counter[diff] += 1;
        }

        int mxi = counter_argmax<Q>(counter);
        if (counter[mxi] - counter[0] >= params.threshold1) {
            et[i] = mxi;
        }
    }

    s = s_orig;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < v.sparse_size(); j++) {
            s[i] = pmod(s[i] - et[pmod(i - v.indices[j], n)] * v.values[j], Q);
        }
        for (int j = 0; j < w.sparse_size(); j++) {
            s[i] = pmod(s[i] - et[n + pmod(i - w.indices[j], n)] * w.values[j], Q);
        }
    }

    int weight_s = polynomial_weight(s);
    int weight_e = polynomial_weight(et);
    if (weight_s == 0) {
        result.decoded_error = et;
        result.success = true;
        return result;
    }
    std::cout << "e weight 1: " << weight_e << std::endl;
    std::cout << "s weight 1: " << weight_s << std::endl;

    result.iterations += 1;
    for (int i = 0; i < n; i++) {
        std::vector<int> counter(Q, 0);
        for (int j = 0; j < v.sparse_size(); j++) {
            int inv = ctx.division_table[1][v.values[j]];
            int diff = (s[pmod(v.indices[j] + i, n)] * inv) % Q;
            counter[diff] += 1;
        }

        int mxi = counter_argmax<Q>(counter);
        if (counter[mxi] >= params.threshold2) {
            et[i] = mxi;
        }
    }
    for (int i = n; i < 2 * n; i++) {
        std::vector<int> counter(Q, 0);
        for (int j = 0; j < w.sparse_size(); j++) {
            int inv = ctx.division_table[1][w.values[j]];
            int diff = (s[pmod(w.indices[j] + i, n)] * inv) % Q;
            counter[diff] += 1;
        }

        int mxi = counter_argmax<Q>(counter);
        if (counter[mxi] >= params.threshold2) {
            et[i] = mxi;
        }
    }

    s = s_orig;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < v.sparse_size(); j++) {
            s[i] = pmod(s[i] - et[pmod(i - v.indices[j], n)] * v.values[j], Q);
        }
        for (int j = 0; j < w.sparse_size(); j++) {
            s[i] = pmod(s[i] - et[n + pmod(i - w.indices[j], n)] * w.values[j], Q);
        }
    }

    weight_s = polynomial_weight(s);
    if (weight_s == 0) {
        result.decoded_error = et;
        result.success = true;
        return result;
    }
    weight_e = polynomial_weight(et);

    std::cout << "e weight 2: " << weight_e << std::endl;
    std::cout << "s weight 2: " << weight_s << std::endl;

    result.decoded_error = et;
    return result;
}
