#include <doctest/doctest.h>
#include "world/World.hpp"
#include "world/Block.hpp"
#include "world/storage/SaveHandler.hpp"
#include <filesystem>

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

TEST_CASE("Skylight converges across chunk seams regardless of lighting order") {
    Block::init();

    auto run = [](bool lightRoofedChunkFirst) {
        World world;
        world.isRemote = true;
        auto open = std::make_shared<Chunk>(0, 0);
        auto roofed = std::make_shared<Chunk>(1, 0);
        world.addChunk(open);
        world.addChunk(roofed);

        for (int x = 0; x < 16; ++x) {
            for (int z = 0; z < 16; ++z) {
                roofed->setBlockIDSafe(x, 64, z, 1);
            }
        }

        if (lightRoofedChunkFirst) {
            world.calculateInitialSkylight(*roofed);
            world.calculateInitialSkylight(*open);
        } else {
            world.calculateInitialSkylight(*open);
            world.calculateInitialSkylight(*roofed);
        }
        return roofed->getLight(LightType::Sky, 0, 63, 8);
    };

    CHECK(run(true) == 14);
    CHECK(run(false) == 14);
}

TEST_CASE("Final relighting retracts stale skylight across a chunk seam") {
    Block::init();

    World world;
    world.isRemote = true;
    auto west = std::make_shared<Chunk>(0, 0);
    auto east = std::make_shared<Chunk>(1, 0);
    world.addChunk(west);
    world.addChunk(east);

    for (int x = 0; x < 16; ++x) {
        for (int z = 0; z < 16; ++z) {
            east->setBlockIDSafe(x, 64, z, 1);
        }
    }

    world.calculateInitialSkylight(*east);
    world.calculateInitialSkylight(*west);
    REQUIRE(east->getLight(LightType::Sky, 0, 63, 8) == 14);

    for (int x = 0; x < 16; ++x) {
        for (int z = 0; z < 16; ++z) {
            west->setBlockIDSafe(x, 64, z, 1);
        }
    }
    world.calculateInitialSkylight(*west);

    CHECK(east->getLight(LightType::Sky, 0, 63, 8) == 0);
    CHECK(east->getLight(LightType::Sky, 1, 63, 8) == 0);
}

TEST_CASE("Incomplete chunk lighting is not persisted") {
    namespace fs = std::filesystem;
    const fs::path worldDir = fs::temp_directory_path() / "ourcraft_incomplete_chunk_test";
    fs::remove_all(worldDir);

    {
        SaveHandler save(worldDir.string());
        Chunk incomplete(3, -2);
        incomplete.setBlockIDSafe(1, 20, 1, 1);
        incomplete.setState(ChunkState::LightingReady);
        save.saveChunk(incomplete);
        save.flush();

        Chunk loaded(3, -2);
        CHECK_FALSE(save.loadChunk(loaded));
    }

    fs::remove_all(worldDir);
}

TEST_CASE("Complete chunks retain validated lighting state when persisted") {
    namespace fs = std::filesystem;
    const fs::path worldDir = fs::temp_directory_path() / "ourcraft_complete_chunk_test";
    fs::remove_all(worldDir);

    {
        SaveHandler save(worldDir.string());
        Chunk complete(-4, 7);
        complete.setBlockIDSafe(2, 30, 5, 1);
        complete.setLight(LightType::Sky, 2, 31, 5, 12);
        complete.setState(ChunkState::Complete);
        save.saveChunk(complete);
        save.flush();

        Chunk loaded(-4, 7);
        REQUIRE(save.loadChunk(loaded));
        CHECK(loaded.getState() == ChunkState::Complete);
        CHECK(loaded.getBlockID(2, 30, 5) == 1);
        CHECK(loaded.getLight(LightType::Sky, 2, 31, 5) == 12);
    }

    fs::remove_all(worldDir);
}
