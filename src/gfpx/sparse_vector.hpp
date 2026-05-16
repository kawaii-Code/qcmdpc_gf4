#pragma once

#include <bits/stdc++.h>

template <int Q>
struct Sparse_Vector {
    Sparse_Vector() {}
    Sparse_Vector(int _n, std::vector<int> _values, std::vector<int> _indices)
        : n(_n), values(_values), indices(_indices) { }

    static int random_non_zero_element() {
        return 1 + rand() % (Q - 1);
    }

    static Sparse_Vector<Q> random_vector_of_weight(int t, int n) {
        std::vector<int> values(t);
        for (int i = 0; i < t; i++) {
            values[i] = random_non_zero_element();
        }

        std::set<int> indices_set;
        for (int i = n - t; i < n; i++) {
            int r = rand() % (i + 1);
            if (!indices_set.insert(r).second) {
                indices_set.insert(i);
            }
        }

        std::vector<int> indices(indices_set.begin(), indices_set.end());

        return Sparse_Vector<Q>(n, values, indices);
    }

    static Sparse_Vector<Q> random_vector_of_weight2(int t, int n) {
        std::vector<int> values(t);
        for (int i = 0; i < t; i++) {
            values[i] = random_non_zero_element();
        }

        std::set<int> indices_set;
        while (indices_set.size() < t) {
            indices_set.insert(rand() % n);
        }

        std::vector<int> indices(indices_set.begin(), indices_set.end());

        return Sparse_Vector<Q>(n, values, indices);
    }

    int total_size() const {
        return n;
    }

    int sparse_size() const {
        return values.size();
    }

    int n;
    std::vector<int> values;
    std::vector<int> indices;
};

template <int Q>
struct Simple_Vector {
    Simple_Vector(int _n) : v(_n, 0) { }

    // @copypasta from Sparse_Vector
    static int random_non_zero_element() {
        return 1 + rand() % (Q - 1);
    }

    // @copypasta from Sparse_Vector
    static Simple_Vector<Q> random_vector_of_weight(int t, int n) {
        std::vector<int> values(t);
        for (int i = 0; i < t; i++) {
            values[i] = random_non_zero_element();
        }

        std::set<int> indices_set;
        for (int i = n - t; i < n; i++) {
            int r = rand() % (i + 1);
            if (!indices_set.insert(r).second) {
                indices_set.insert(i);
            }
        }

        Simple_Vector<Q> result(n);
        int j = 0;
        for (auto i : indices_set) {
            result[i] = values[j];
            j++;
        }
        return result;
    }

    static Simple_Vector<Q> random_vector_of_weight2(int t, int n) {
        std::set<int> indices;
        while (indices.size() != t) {
            int idx = rand() % n;
            indices.insert(idx);
        }

        Simple_Vector<Q> result(n);
        for (int idx : indices) {
            result[idx] = random_non_zero_element();
        }

        return result;
    }

    int size() const {
        return (int)v.size();
    }

    int &operator[](int i) {
        return v[i];
    }

    int operator[](int i) const {
        return v[i];
    }

    std::vector<int> v;
};

template <int Q>
std::ostream &operator<<(std::ostream &os, const Sparse_Vector<Q> &sv) {
    os << "SV<" << Q << ">(";
    int n = sv.total_size();
    int j = 0;
    for (int i = 0; i < n - 1; i++) {
        if (std::find(sv.indices.begin(), sv.indices.end(), i) == sv.indices.end()) {
            os << "0, ";
        } else {
            os << sv.values[j] << ", ";
            j += 1;
        }
    }
    if (std::find(sv.indices.begin(), sv.indices.end(), n - 1) == sv.indices.end()) {
        os << "0";
    } else {
        os << sv.values[j];
    }
    os << ")";
    return os;
}

template <int Q>
std::ostream &operator<<(std::ostream &os, const Simple_Vector<Q> &v) {
    int n = v.size();
    os << "S<" << Q << ">(";
    for (int i = 0; i < n - 1; i++) {
        os << v[i] << ", ";
    }
    os << v[n - 1] << ")";
    return os;
}
