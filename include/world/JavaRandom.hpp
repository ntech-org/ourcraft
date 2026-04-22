#pragma once

#include <cstdint>

class JavaRandom {
public:
    JavaRandom(int64_t seed = 0) {
        setSeed(seed);
    }

    void setSeed(int64_t seed) {
        m_seed = (seed ^ 0x5DEECE66DLL) & ((1LL << 48) - 1);
    }

    int next(int bits) {
        m_seed = (m_seed * 0x5DEECE66DLL + 0xBLL) & ((1LL << 48) - 1);
        return (int)(m_seed >> (48 - bits));
    }

    int nextInt() {
        return next(32);
    }

    int nextInt(int n) {
        if (n <= 0) return 0;
        if ((n & -n) == n) // i.e., n is a power of 2
            return (int)((n * (int64_t)next(31)) >> 31);

        int bits, val;
        do {
            bits = next(31);
            val = bits % n;
        } while (bits - val + (n - 1) < 0);
        return val;
    }

    int64_t nextLong() {
        return ((int64_t)next(32) << 32) + next(32);
    }

    double nextDouble() {
        return (((int64_t)next(26) << 27) + next(27)) / (double)(1LL << 53);
    }

    float nextFloat() {
        return next(24) / (float)(1 << 24);
    }

private:
    int64_t m_seed;
};
