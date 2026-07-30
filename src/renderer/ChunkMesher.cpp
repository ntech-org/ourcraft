#include "renderer/ChunkMesher.hpp"
#include "world/World.hpp"
#include "world/Block.hpp"
#include "util/Profiler.hpp"
#include <algorithm>
#include <cmath>
#include <mutex>
#include <vector>

MeshingSnapshot::MeshingSnapshot(const World& world, int chunkX, int chunkZ, int sectionIndex)
    : m_baseX(chunkX * Chunk::WIDTH), m_baseZ(chunkZ * Chunk::DEPTH),
      m_minY(sectionIndex * Chunk::SECTION_HEIGHT - 1) {
    std::shared_ptr<const Chunk> chunks[3][3];
    std::vector<std::unique_lock<std::mutex>> blockLocks;
    blockLocks.reserve(9);

    for (int dx = -1; dx <= 1; ++dx) {
        for (int dz = -1; dz <= 1; ++dz) {
            chunks[dx + 1][dz + 1] = world.getChunk(chunkX + dx, chunkZ + dz);
        }
    }
    for (auto& column : chunks) {
        for (auto& chunk : column) {
            if (chunk) blockLocks.emplace_back(chunk->getBlockMutex());
        }
    }

    if (const auto& center = chunks[1][1]) {
        for (int x = 0; x < Chunk::WIDTH; ++x) {
            for (int z = 0; z < Chunk::DEPTH; ++z) {
                m_waterLevels[x * Chunk::DEPTH + z] = center->getWaterLevel(x, z);
            }
        }
    }

    for (int y = std::max(0, m_minY); y < std::min(Chunk::HEIGHT, m_minY + HALO_HEIGHT); ++y) {
        for (int z = -1; z <= 16; ++z) {
            for (int x = -1; x <= 16; ++x) {
                const int dx = x < 0 ? -1 : (x >= 16 ? 1 : 0);
                const int dz = z < 0 ? -1 : (z >= 16 ? 1 : 0);
                const auto& chunk = chunks[dx + 1][dz + 1];
                if (!chunk) continue;

                const int lx = x & 15;
                const int lz = z & 15;
                const int sourceIndex = (lx << 11) | (lz << 7) | y;
                const int targetIndex = localIndex(x, y, z);
                m_blocks[targetIndex] = chunk->getBlockID(lx, y, lz);
                m_metadata[targetIndex] = chunk->getBlockMetadata(lx, y, lz);
                if (chunk->isLightWipeComplete()) {
                    m_skylight[targetIndex] = chunk->getLightInternal(LightType::Sky, sourceIndex);
                    m_blocklight[targetIndex] = chunk->getLightInternal(LightType::Block, sourceIndex);
                } else {
                    m_skylight[targetIndex] = 15;
                }
            }
        }
    }
}

const Material& MeshingSnapshot::getBlockMaterial(int x, int y, int z) const {
    const uint8_t id = getBlockID(x, y, z);
    const Block* block = id ? Block::blocksList[id] : nullptr;
    return block ? block->blockMaterial : Material::air;
}

ChunkMeshData ChunkMesher::buildSectionMesh(const World& world, const Chunk& chunk, int si) {
    OC_ZONE_SCOPED;
    ChunkMeshData md;
    md.bounds.min = glm::vec3(0.0f);
    md.bounds.max = glm::vec3(16.0f);

    int cx = chunk.getX(), cz = chunk.getZ();

    MeshingSnapshot n(world, cx, cz, si);

    float waterLevels[16][16];
    for (int x = 0; x < 16; ++x) {
        for (int z = 0; z < 16; ++z) {
            waterLevels[x][z] = n.getWaterLevel(x, z);
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
