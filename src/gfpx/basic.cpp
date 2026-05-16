#include <chrono>
#include <cassert>

std::uint8_t pmod(std::uint8_t a, std::uint8_t m) {
    return (a % m + m) % m;
}

int pmod(int a, int m) {
    return (a % m + m) % m;
}

int milliseconds_passed_since(std::chrono::time_point<std::chrono::steady_clock> time_point) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - time_point
    ).count();
}

int round_to_next_pow2(int x) {
    for (int i = 0; i < 32; i++) {
        if ((1 << i) >= x) {
            return (1 << i);
        }
    }

    assert(false);
    return -1;
}
