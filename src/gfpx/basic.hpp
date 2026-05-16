#pragma once

template <typename T>
void shuffle(std::vector<T> &v) {
    int n = v.size();
    for (int i = n - 1; i >= 1; i--) {
        std::swap(v[i], v[rand() % i]);
    }
}
