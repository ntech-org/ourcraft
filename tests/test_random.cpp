#include <doctest/doctest.h>
#include "world/JavaRandom.hpp"

TEST_CASE("JavaRandom basic") {
    JavaRandom rand(12345);
    int v1 = rand.nextInt();
    int v2 = rand.nextInt();
    CHECK(v1 != v2);
}

TEST_CASE("JavaRandom deterministic") {
    JavaRandom rand1(42);
    JavaRandom rand2(42);
    for (int i = 0; i < 100; ++i) {
        CHECK(rand1.nextInt() == rand2.nextInt());
    }
}

TEST_CASE("JavaRandom nextInt with bound") {
    JavaRandom rand(999);
    for (int i = 0; i < 1000; ++i) {
        int v = rand.nextInt(10);
        CHECK(v >= 0);
        CHECK(v < 10);
    }
}

TEST_CASE("JavaRandom nextFloat") {
    JavaRandom rand(555);
    for (int i = 0; i < 100; ++i) {
        float v = rand.nextFloat();
        CHECK(v >= 0.0f);
        CHECK(v < 1.0f);
    }
}

TEST_CASE("JavaRandom nextDouble") {
    JavaRandom rand(777);
    for (int i = 0; i < 100; ++i) {
        double v = rand.nextDouble();
        CHECK(v >= 0.0);
        CHECK(v < 1.0);
    }
}
