#include <doctest/doctest.h>
#include "world/World.hpp"
#include "world/Block.hpp"

TEST_CASE("World block operations") {
    World world;
    world.isRemote = true;

    Block::init();

    world.addChunk(std::make_shared<Chunk>(0, 0));

    CHECK(world.getBlockID(0, 0, 0) == 0);

    world.setBlockID(0, 0, 0, 1);
    CHECK(world.getBlockID(0, 0, 0) == 1);

    world.setBlockID(15, 64, 15, 42);
    CHECK(world.getBlockID(15, 64, 15) == 42);
}

TEST_CASE("World chunk management") {
    World world;
    world.isRemote = true;

    auto chunk = std::make_shared<Chunk>(5, 10);
    world.addChunk(chunk);

    auto retrieved = world.getChunk(5, 10);
    CHECK(retrieved != nullptr);
    CHECK(retrieved->getX() == 5);
    CHECK(retrieved->getZ() == 10);

    CHECK(world.getChunk(0, 0) == nullptr);

    world.removeChunk(5, 10);
    CHECK(world.getChunk(5, 10) == nullptr);
}

TEST_CASE("World scheduled block updates") {
    World world;
    world.isRemote = true;

    world.addChunk(std::make_shared<Chunk>(0, 0));

    world.scheduleBlockUpdate(0, 0, 0, 8, 10);
    world.scheduleBlockUpdate(0, 0, 0, 8, 20);

    world.setBlockID(0, 0, 0, 8);
}
