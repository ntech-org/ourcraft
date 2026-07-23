#include <doctest/doctest.h>
#include "world/Chunk.hpp"

TEST_CASE("Chunk construction") {
    Chunk chunk(0, 0);
    CHECK(chunk.getX() == 0);
    CHECK(chunk.getZ() == 0);
    CHECK(chunk.getState() == ChunkState::Empty);
}

TEST_CASE("Chunk block get/set") {
    Chunk chunk(5, 10);
    CHECK(chunk.getX() == 5);
    CHECK(chunk.getZ() == 10);

    chunk.setBlockID(0, 0, 0, 1);
    CHECK(chunk.getBlockID(0, 0, 0) == 1);

    chunk.setBlockID(15, 127, 15, 42);
    CHECK(chunk.getBlockID(15, 127, 15) == 42);

    chunk.setBlockID(7, 64, 7, 0);
    CHECK(chunk.getBlockID(7, 64, 7) == 0);
}

TEST_CASE("Chunk metadata get/set") {
    Chunk chunk(0, 0);

    chunk.setBlockMetadata(0, 0, 0, 5);
    CHECK(chunk.getBlockMetadata(0, 0, 0) == 5);

    chunk.setBlockMetadata(15, 127, 15, 15);
    CHECK(chunk.getBlockMetadata(15, 127, 15) == 15);
}

TEST_CASE("Chunk light get/set") {
    Chunk chunk(0, 0);

    chunk.setLight(LightType::Sky, 0, 0, 0, 15);
    CHECK(chunk.getLight(LightType::Sky, 0, 0, 0) == 15);

    chunk.setLight(LightType::Block, 8, 64, 8, 7);
    CHECK(chunk.getLight(LightType::Block, 8, 64, 8) == 7);

    chunk.setLight(LightType::Sky, 15, 127, 15, 0);
    CHECK(chunk.getLight(LightType::Sky, 15, 127, 15) == 0);
}

TEST_CASE("Chunk section index") {
    CHECK(Chunk::getSectionIndex(0) == 0);
    CHECK(Chunk::getSectionIndex(15) == 0);
    CHECK(Chunk::getSectionIndex(16) == 1);
    CHECK(Chunk::getSectionIndex(127) == 7);
    CHECK(Chunk::getSectionMinY(0) == 0);
    CHECK(Chunk::getSectionMinY(1) == 16);
    CHECK(Chunk::getSectionMinY(7) == 112);
}

TEST_CASE("Chunk state") {
    Chunk chunk(0, 0);
    CHECK(chunk.getState() == ChunkState::Empty);

    chunk.setState(ChunkState::Generated);
    CHECK(chunk.getState() == ChunkState::Generated);

    chunk.setState(ChunkState::Complete);
    CHECK(chunk.getState() == ChunkState::Complete);
}

TEST_CASE("Chunk height map") {
    Chunk chunk(0, 0);
    chunk.setHeight(0, 0, 64);
    CHECK(chunk.getHeight(0, 0) == 64);

    chunk.setHeight(15, 15, 128);
    CHECK(chunk.getHeight(15, 15) == 128);
}
