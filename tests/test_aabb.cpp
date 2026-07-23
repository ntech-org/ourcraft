#include <doctest/doctest.h>
#include "physics/AxisAlignedBB.hpp"

TEST_CASE("AxisAlignedBB construction") {
    AxisAlignedBB bb(0, 0, 0, 1, 1, 1);
    CHECK(bb.minX == 0);
    CHECK(bb.minY == 0);
    CHECK(bb.minZ == 0);
    CHECK(bb.maxX == 1);
    CHECK(bb.maxY == 1);
    CHECK(bb.maxZ == 1);
}

TEST_CASE("AxisAlignedBB intersectsWith") {
    AxisAlignedBB bb1(0, 0, 0, 2, 2, 2);
    AxisAlignedBB bb2(1, 1, 1, 3, 3, 3);
    AxisAlignedBB bb3(5, 5, 5, 6, 6, 6);

    CHECK(bb1.intersectsWith(bb2));
    CHECK(bb2.intersectsWith(bb1));
    CHECK_FALSE(bb1.intersectsWith(bb3));
    CHECK_FALSE(bb3.intersectsWith(bb1));
}

TEST_CASE("AxisAlignedBB expand") {
    AxisAlignedBB bb(1, 1, 1, 2, 2, 2);
    AxisAlignedBB expanded = bb.expand(0.5, 0.5, 0.5);
    CHECK(expanded.minX == doctest::Approx(0.5));
    CHECK(expanded.minY == doctest::Approx(0.5));
    CHECK(expanded.minZ == doctest::Approx(0.5));
    CHECK(expanded.maxX == doctest::Approx(2.5));
    CHECK(expanded.maxY == doctest::Approx(2.5));
    CHECK(expanded.maxZ == doctest::Approx(2.5));
}

TEST_CASE("AxisAlignedBB calculateYOffset") {
    AxisAlignedBB block(0, 0, 0, 1, 1, 1);
    AxisAlignedBB moving(-0.5, 0.5, -0.5, 0.5, 1.5, 0.5);

    double dy = block.calculateYOffset(moving, -1.0);
    CHECK(dy == doctest::Approx(-1.0));
}

TEST_CASE("AxisAlignedBB offset") {
    AxisAlignedBB bb(0, 0, 0, 1, 1, 1);
    bb.offset(1.0, 2.0, 3.0);
    CHECK(bb.minX == doctest::Approx(1.0));
    CHECK(bb.minY == doctest::Approx(2.0));
    CHECK(bb.minZ == doctest::Approx(3.0));
    CHECK(bb.maxX == doctest::Approx(2.0));
    CHECK(bb.maxY == doctest::Approx(3.0));
    CHECK(bb.maxZ == doctest::Approx(4.0));
}

TEST_CASE("AxisAlignedBB addCoord") {
    AxisAlignedBB bb(0, 0, 0, 1, 1, 1);
    AxisAlignedBB result = bb.addCoord(0.5, 0.5, 0.5);
    CHECK(result.minX == doctest::Approx(0.0));
    CHECK(result.minY == doctest::Approx(0.0));
    CHECK(result.minZ == doctest::Approx(0.0));
    CHECK(result.maxX == doctest::Approx(1.5));
    CHECK(result.maxY == doctest::Approx(1.5));
    CHECK(result.maxZ == doctest::Approx(1.5));
}
