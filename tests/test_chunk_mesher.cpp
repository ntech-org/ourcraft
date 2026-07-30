#include <doctest/doctest.h>
#include "renderer/ChunkMesher.hpp"
#include "world/Block.hpp"
#include "world/World.hpp"

TEST_CASE("Top world face samples full skylight above the height limit") {
    Block::init();

    World world;
    world.isRemote = true;
    auto chunk = std::make_shared<Chunk>(0, 0);
    chunk->setBlockIDSafe(4, Chunk::HEIGHT - 1, 6, 1);
    chunk->setLightWipeComplete(true);
    world.addChunk(chunk);

    const ChunkMeshData mesh = ChunkMesher::buildSectionMesh(
        world, *chunk, Chunk::SECTION_COUNT - 1);

    bool foundTopFace = false;
    for (const TerrainVertex& vertex : mesh.opaque.vertices) {
        if (vertex.faceId != static_cast<std::uint32_t>(FaceDirection::Up) ||
            vertex.y != static_cast<float>(Chunk::SECTION_HEIGHT)) {
            continue;
        }
        foundTopFace = true;
        CHECK(vertex.skyLight == 15.0f);
        CHECK(vertex.blockLight == 0.0f);
    }
    CHECK(foundTopFace);
}
