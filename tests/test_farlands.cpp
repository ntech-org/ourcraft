#include <doctest/doctest.h>
#include "world/InfdevWorldGenerator.hpp"
#include "world/NoiseGeneratorPerlin.hpp"
#include "world/JavaRandom.hpp"
#include "world/Chunk.hpp"
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

// Mirror of Java saturating (int) cast used by Far Lands path
static int32_t javaDoubleToIntRef(double d) {
    if (std::isnan(d)) return 0;
    if (d >= static_cast<double>(std::numeric_limits<int32_t>::max()))
        return std::numeric_limits<int32_t>::max();
    if (d <= static_cast<double>(std::numeric_limits<int32_t>::min()))
        return std::numeric_limits<int32_t>::min();
    return static_cast<int32_t>(d);
}

static void selectLatticeRef(double d, bool farLands, int32_t& outLattice, double& outFrac) {
    if (farLands) {
        int32_t X = javaDoubleToIntRef(d);
        if (d < static_cast<double>(X)) {
            X = static_cast<int32_t>(static_cast<uint32_t>(X) - 1u);
        }
        outLattice = X;
        outFrac = d - static_cast<double>(X);
    } else {
        double fl = std::floor(d);
        outLattice = static_cast<int32_t>(static_cast<int64_t>(fl));
        outFrac = d - fl;
    }
}

TEST_CASE("Java double-to-int saturates at INT bounds") {
    CHECK(javaDoubleToIntRef(0.9) == 0);
    CHECK(javaDoubleToIntRef(-0.9) == 0);
    CHECK(javaDoubleToIntRef(2147483647.9) == std::numeric_limits<int32_t>::max());
    CHECK(javaDoubleToIntRef(2147483648.0) == std::numeric_limits<int32_t>::max());
    CHECK(javaDoubleToIntRef(-2147483649.0) == std::numeric_limits<int32_t>::min());
}

TEST_CASE("Far Lands lattice fraction exceeds 1 past INT_MAX") {
    const double justBelow = 2147483647.5;
    const double justAbove = 2147483648.5;

    int32_t lattice = 0;
    double frac = 0.0;

    selectLatticeRef(justBelow, true, lattice, frac);
    CHECK(lattice == std::numeric_limits<int32_t>::max());
    CHECK(frac >= 0.0);
    CHECK(frac < 1.0);

    selectLatticeRef(justAbove, true, lattice, frac);
    CHECK(lattice == std::numeric_limits<int32_t>::max());
    CHECK(frac > 1.0);

    selectLatticeRef(justAbove, false, lattice, frac);
    CHECK(frac >= 0.0);
    CHECK(frac < 1.0);
}

TEST_CASE("Far Lands toggle changes distant terrain, not spawn") {
    const int64_t seed = 12345;

    auto hashChunk = [](Chunk& chunk) -> uint64_t {
        uint64_t h = 14695981039346656037ull;
        const uint8_t* blocks = chunk.getBlocks();
        for (int i = 0; i < Chunk::SIZE; ++i) {
            h ^= blocks[i];
            h *= 1099511628211ull;
        }
        return h;
    };

    InfdevWorldGenerator genOff(seed);
    genOff.setFarLands(false);
    Chunk spawnOff(0, 0);
    genOff.generateChunk(spawnOff);
    Chunk farOff(784426, 0); // ~12,550,816 blocks
    genOff.generateChunk(farOff);

    InfdevWorldGenerator genOn(seed);
    genOn.setFarLands(true);
    Chunk spawnOn(0, 0);
    genOn.generateChunk(spawnOn);
    Chunk farOn(784426, 0);
    genOn.generateChunk(farOn);

    CHECK(hashChunk(spawnOff) == hashChunk(spawnOn));
    CHECK(hashChunk(farOff) != hashChunk(farOn));
}
