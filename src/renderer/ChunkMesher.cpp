#include "renderer/ChunkMesher.hpp"
#include "world/World.hpp"
#include "world/Block.hpp"
#include <algorithm>
#include <cmath>

namespace {
struct Neighborhood : public IBlockAccess {
    std::shared_ptr<const Chunk> chunks[3][3];
    const World& world;
    int baseCX, baseCZ;
    Neighborhood(const World& w, int cx, int cz) : world(w), baseCX(cx), baseCZ(cz) {}

    uint8_t getBlockID(int x, int y, int z) const override {
        if (y < 0 || y >= 128) return 0;
        int ncx = (x >> 4) + 1;
        int ncz = (z >> 4) + 1;
        if (ncx < 0 || ncx > 2 || ncz < 0 || ncz > 2) return 0;
        const Chunk* c = chunks[ncx][ncz].get();
        return c ? c->getBlockID(x & 15, y, z & 15) : 0;
    }
    uint8_t getBlockMetadata(int x, int y, int z) const override {
        if (y < 0 || y >= 128) return 0;
        int ncx = (x >> 4) + 1;
        int ncz = (z >> 4) + 1;
        if (ncx < 0 || ncx > 2 || ncz < 0 || ncz > 2) return 0;
        const Chunk* c = chunks[ncx][ncz].get();
        return c ? c->getBlockMetadata(x & 15, y, z & 15) : 0;
    }
    const Material& getBlockMaterial(int x, int y, int z) const override {
        uint8_t id = getBlockID(x, y, z);
        if (id == 0) return Material::air;
        Block* b = Block::blocksList[id];
        return b ? b->blockMaterial : Material::air;
    }
    std::pair<int, int> getLightPair(int x, int y, int z) const override {
        int sky = 15;
        int block = 0;

        if (y >= 0 && y < 128) {
            int cx_off = (x >= 0 ? x / 16 : (x - 15) / 16) - baseCX;
            int cz_off = (z >= 0 ? z / 16 : (z - 15) / 16) - baseCZ;
            int lx = x & 15;
            int lz = z & 15;
            
            if (cx_off >= -1 && cx_off <= 1 && cz_off >= -1 && cz_off <= 1) {
                auto& chunk = chunks[cx_off + 1][cz_off + 1];
                if (chunk) {
                    sky = chunk->getLightInternal(LightType::Sky, (lx << 11) | (lz << 7) | y);
                    block = chunk->getLightInternal(LightType::Block, (lx << 11) | (lz << 7) | y);
                } else {
                    sky = world.getSavedLightValue(LightType::Sky, x, y, z);
                    block = world.getSavedLightValue(LightType::Block, x, y, z);
                }
            } else {
                sky = world.getSavedLightValue(LightType::Sky, x, y, z);
                block = world.getSavedLightValue(LightType::Block, x, y, z);
            }
        } else {
            sky = world.getSavedLightValue(LightType::Sky, x, y, z);
            block = world.getSavedLightValue(LightType::Block, x, y, z);
        }

        return {sky, block};
    }
};

static thread_local float tls_waterLevels[16][16];

struct SectionWaterLevels {
    float levels[16][16];
};
}

ChunkMeshData ChunkMesher::buildSectionMesh(const World& world, const Chunk& chunk, int si) {
    ChunkMeshData md;
    md.bounds.min = glm::vec3(0.0f);
    md.bounds.max = glm::vec3(16.0f);

    if (!chunk.isSectionNonEmpty(si)) return md;

    int cx = chunk.getX(), cz = chunk.getZ();

    Neighborhood n(world, cx, cz);
    for (int dx = -1; dx <= 1; ++dx)
        for (int dz = -1; dz <= 1; ++dz)
            n.chunks[dx+1][dz+1] = world.getChunk(cx + dx, cz + dz);

    float waterLevels[16][16];
    for (int x = 0; x < 16; ++x) {
        for (int z = 0; z < 16; ++z) {
            waterLevels[x][z] = chunk.getWaterLevel(x, z);
        }
    }

    int prevX = cx, prevZ = cz - 1;
    int nextX = cx, nextZ = cz + 1;
    std::shared_ptr<const Chunk> prevChunkSp = world.getChunk(prevX, prevZ);
    std::shared_ptr<const Chunk> nextChunkSp = world.getChunk(nextX, nextZ);
    const Chunk* prevChunk = prevChunkSp.get();
    const Chunk* nextChunk = nextChunkSp.get();
    if (prevChunk) {
        for (int x = 0; x < 16; ++x) {
            waterLevels[x][0] = std::max(waterLevels[x][0], prevChunk->getWaterLevel(x, 15));
        }
    }
    if (nextChunk) {
        for (int x = 0; x < 16; ++x) {
            waterLevels[x][15] = std::max(waterLevels[x][15], nextChunk->getWaterLevel(x, 0));
        }
    }

    const float* wl = &waterLevels[0][0];

    greedyMeshTopBottom(md, n, si, cx, cz, false, wl);
    greedyMeshTopBottom(md, n, si, cx, cz, true, wl);
    greedyMeshNorthSouth(md, n, si, cx, cz, false, wl);
    greedyMeshNorthSouth(md, n, si, cx, cz, true, wl);
    greedyMeshWestEast(md, n, si, cx, cz, false, wl);
    greedyMeshWestEast(md, n, si, cx, cz, true, wl);
    crossMeshPass(md, n, si, cx, cz);
    fluidMeshPass(md, n, si, cx, cz, wl);

    if (!md.opaque.vertices.empty() || !md.translucent.vertices.empty()) {
        md.bounds.min = glm::vec3(0.0f, 0.0f, 0.0f);
        md.bounds.max = glm::vec3(16.0f, 16.0f, 16.0f);
    }

    return md;
}
