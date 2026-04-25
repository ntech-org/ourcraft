#include "renderer/ChunkMesher.hpp"
#include "world/World.hpp"
#include "world/Block.hpp"
#include <algorithm>

namespace {
struct Neighborhood : public IBlockAccess {
    std::shared_ptr<const Chunk> chunks[3][3];
    const World& world;
    int baseCX, baseCZ;
    Neighborhood(const World& w, int cx, int cz) : world(w), baseCX(cx), baseCZ(cz) {}

    uint8_t getBlockID(int x, int y, int z) const override {
        if (y < 0 || y >= 128) return 0;
        int cx = 1, cz = 1;
        if (x < 0) { cx--; x += 16; } else if (x >= 16) { cx++; x -= 16; }
        if (z < 0) { cz--; z += 16; } else if (z >= 16) { cz++; z -= 16; }
        return chunks[cx][cz] ? chunks[cx][cz]->getBlockID(x, y, z) : 0;
    }
    uint8_t getBlockMetadata(int x, int y, int z) const override {
        if (y < 0 || y >= 128) return 0;
        int cx = 1, cz = 1;
        if (x < 0) { cx--; x += 16; } else if (x >= 16) { cx++; x -= 16; }
        if (z < 0) { cz--; z += 16; } else if (z >= 16) { cz++; z -= 16; }
        return chunks[cx][cz] ? chunks[cx][cz]->getBlockMetadata(x, y, z) : 0;
    }
    const Material& getBlockMaterial(int x, int y, int z) const override {
        uint8_t id = getBlockID(x, y, z);
        return id == 0 ? Material::air : Block::blocksList[id]->blockMaterial;
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
}

ChunkMeshData ChunkMesher::buildSectionMesh(const World& world, const Chunk& chunk, int si) {
    ChunkMeshData md; int cx = chunk.getX(), cz = chunk.getZ();
    float bx = (float)(cx * 16), by = (float)(si * 16), bz = (float)(cz * 16);
    md.bounds.min = {bx, by, bz}; md.bounds.max = {bx + 16, by + 16, bz + 16};
    Neighborhood n(world, cx, cz); 
    for (int dx = -1; dx <= 1; ++dx) 
        for (int dz = -1; dz <= 1; ++dz) 
            n.chunks[dx+1][dz+1] = world.getChunk(cx + dx, cz + dz);
    
    greedyMeshTopBottom(md, n, si, cx, cz, false); greedyMeshTopBottom(md, n, si, cx, cz, true);
    greedyMeshNorthSouth(md, n, si, cx, cz, false); greedyMeshNorthSouth(md, n, si, cx, cz, true);
    greedyMeshWestEast(md, n, si, cx, cz, false); greedyMeshWestEast(md, n, si, cx, cz, true);
    fluidMeshPass(md, n, si, cx, cz);
    return md;
}
