#include "renderer/ChunkMesher.hpp"
#include "world/Block.hpp"
#include "world/World.hpp"
#include <array>

namespace {
constexpr std::uint32_t kWhiteColor = 0xFFFFFFFFu;
constexpr int kSectionSize = Chunk::SECTION_HEIGHT;
constexpr int kMaskArea = Chunk::WIDTH * Chunk::DEPTH;

enum class FaceDirection {
    Down = 0,
    Up = 1,
    North = 2,
    South = 3,
    West = 4,
    East = 5
};

struct FaceMaskCell {
    bool visible = false;
    int textureIndex = 0;
};

bool isSolidOccluder(std::uint8_t blockId) {
    if (blockId == 0) {
        return false;
    }

    const Block* block = Block::blocksList[blockId];
    return block != nullptr && block->isOccluder();
}

bool isGreedyRenderable(std::uint8_t blockId) {
    if (blockId == 0) {
        return false;
    }

    const Block* block = Block::blocksList[blockId];
    return block != nullptr && block->isGreedyMergeable();
}

FaceMaskCell makeMaskCell(std::uint8_t blockId, int face) {
    if (!isGreedyRenderable(blockId)) {
        return {};
    }

    const Block* block = Block::blocksList[blockId];
    return {
        true,
        block->getTexture(face)
    };
}

bool sameCell(const FaceMaskCell& left, const FaceMaskCell& right) {
    return left.visible == right.visible && left.textureIndex == right.textureIndex;
}

void appendVertex(
    ChunkMeshData& meshData,
    float x,
    float y,
    float z,
    float tileU,
    float tileV,
    int textureIndex,
    FaceDirection direction
) {
    meshData.vertices.push_back({
        x,
        y,
        z,
        tileU,
        tileV,
        kWhiteColor,
        static_cast<std::uint32_t>(textureIndex),
        static_cast<std::uint32_t>(direction)
    });
}

float tileSpan(float extent) {
    return extent <= 1.0f ? 0.999f : extent - 0.001f;
}

void appendIndices(ChunkMeshData& meshData, bool flipWinding) {
    const std::uint32_t baseIndex = static_cast<std::uint32_t>(meshData.vertices.size() - 4);
    if (flipWinding) {
        meshData.indices.insert(meshData.indices.end(), {
            baseIndex + 0, baseIndex + 2, baseIndex + 1,
            baseIndex + 0, baseIndex + 3, baseIndex + 2
        });
    } else {
        meshData.indices.insert(meshData.indices.end(), {
            baseIndex + 0, baseIndex + 1, baseIndex + 2,
            baseIndex + 0, baseIndex + 2, baseIndex + 3
        });
    }
    ++meshData.quadCount;
}

void emitHorizontalQuad(
    ChunkMeshData& meshData,
    FaceDirection direction,
    float x0,
    float x1,
    float y,
    float z0,
    float z1,
    int textureIndex
) {
    const float spanU = tileSpan(x1 - x0);
    const float spanV = tileSpan(z1 - z0);
    if (direction == FaceDirection::Up) {
        appendVertex(meshData, x1, y, z1, spanU, spanV, textureIndex, direction);
        appendVertex(meshData, x1, y, z0, spanU, 0.0f, textureIndex, direction);
        appendVertex(meshData, x0, y, z0, 0.0f, 0.0f, textureIndex, direction);
        appendVertex(meshData, x0, y, z1, 0.0f, spanV, textureIndex, direction);
        appendIndices(meshData, false);
        return;
    }

    appendVertex(meshData, x0, y, z1, 0.0f, spanV, textureIndex, direction);
    appendVertex(meshData, x0, y, z0, 0.0f, 0.0f, textureIndex, direction);
    appendVertex(meshData, x1, y, z0, spanU, 0.0f, textureIndex, direction);
    appendVertex(meshData, x1, y, z1, spanU, spanV, textureIndex, direction);
    appendIndices(meshData, false);
}

void emitZQuad(
    ChunkMeshData& meshData,
    FaceDirection direction,
    float x0,
    float x1,
    float y0,
    float y1,
    float z,
    int textureIndex
) {
    const float spanU = tileSpan(x1 - x0);
    const float spanV = tileSpan(y1 - y0);
    if (direction == FaceDirection::North) {
        appendVertex(meshData, x0, y1, z, 0.0f, 0.0f, textureIndex, direction);
        appendVertex(meshData, x1, y1, z, spanU, 0.0f, textureIndex, direction);
        appendVertex(meshData, x1, y0, z, spanU, spanV, textureIndex, direction);
        appendVertex(meshData, x0, y0, z, 0.0f, spanV, textureIndex, direction);
        appendIndices(meshData, false);
        return;
    }

    appendVertex(meshData, x0, y1, z, 0.0f, 0.0f, textureIndex, direction);
    appendVertex(meshData, x0, y0, z, 0.0f, spanV, textureIndex, direction);
    appendVertex(meshData, x1, y0, z, spanU, spanV, textureIndex, direction);
    appendVertex(meshData, x1, y1, z, spanU, 0.0f, textureIndex, direction);
    appendIndices(meshData, false);
}

void emitXQuad(
    ChunkMeshData& meshData,
    FaceDirection direction,
    float x,
    float y0,
    float y1,
    float z0,
    float z1,
    int textureIndex
) {
    const float spanU = tileSpan(z1 - z0);
    const float spanV = tileSpan(y1 - y0);
    if (direction == FaceDirection::West) {
        appendVertex(meshData, x, y1, z1, spanU, 0.0f, textureIndex, direction);
        appendVertex(meshData, x, y1, z0, 0.0f, 0.0f, textureIndex, direction);
        appendVertex(meshData, x, y0, z0, 0.0f, spanV, textureIndex, direction);
        appendVertex(meshData, x, y0, z1, spanU, spanV, textureIndex, direction);
        appendIndices(meshData, false);
        return;
    }

    appendVertex(meshData, x, y0, z1, 0.0f, spanV, textureIndex, direction);
    appendVertex(meshData, x, y0, z0, spanU, spanV, textureIndex, direction);
    appendVertex(meshData, x, y1, z0, spanU, 0.0f, textureIndex, direction);
    appendVertex(meshData, x, y1, z1, 0.0f, 0.0f, textureIndex, direction);
    appendIndices(meshData, false);
}

void greedyMeshTopBottom(
    ChunkMeshData& meshData,
    const World& world,
    const Chunk& chunk,
    int sectionIndex,
    FaceDirection direction
) {
    std::array<FaceMaskCell, kMaskArea> mask {};
    const int baseX = chunk.getX() * Chunk::WIDTH;
    const int baseY = Chunk::getSectionMinY(sectionIndex);
    const int baseZ = chunk.getZ() * Chunk::DEPTH;
    const int neighborOffset = direction == FaceDirection::Up ? 1 : -1;
    const int faceIndex = static_cast<int>(direction);

    for (int localY = 0; localY < kSectionSize; ++localY) {
        for (int x = 0; x < Chunk::WIDTH; ++x) {
            for (int z = 0; z < Chunk::DEPTH; ++z) {
                const int worldX = baseX + x;
                const int worldY = baseY + localY;
                const int worldZ = baseZ + z;
                const std::uint8_t blockId = world.getBlockID(worldX, worldY, worldZ);
                const std::uint8_t neighborId = world.getBlockID(worldX, worldY + neighborOffset, worldZ);

                mask[x + z * Chunk::WIDTH] =
                    isSolidOccluder(neighborId) ? FaceMaskCell{} : makeMaskCell(blockId, faceIndex);
            }
        }

        for (int z = 0; z < Chunk::DEPTH; ++z) {
            for (int x = 0; x < Chunk::WIDTH;) {
                FaceMaskCell cell = mask[x + z * Chunk::WIDTH];
                if (!cell.visible) {
                    ++x;
                    continue;
                }

                int width = 1;
                while (x + width < Chunk::WIDTH && sameCell(mask[x + width + z * Chunk::WIDTH], cell)) {
                    ++width;
                }

                int height = 1;
                bool canGrow = true;
                while (z + height < Chunk::DEPTH && canGrow) {
                    for (int k = 0; k < width; ++k) {
                        if (!sameCell(mask[x + k + (z + height) * Chunk::WIDTH], cell)) {
                            canGrow = false;
                            break;
                        }
                    }
                    if (canGrow) {
                        ++height;
                    }
                }

                for (int dz = 0; dz < height; ++dz) {
                    for (int dx = 0; dx < width; ++dx) {
                        mask[x + dx + (z + dz) * Chunk::WIDTH] = {};
                    }
                }

                const float planeY = static_cast<float>(baseY + localY + (direction == FaceDirection::Up ? 1 : 0));
                emitHorizontalQuad(
                    meshData,
                    direction,
                    static_cast<float>(baseX + x),
                    static_cast<float>(baseX + x + width),
                    planeY,
                    static_cast<float>(baseZ + z),
                    static_cast<float>(baseZ + z + height),
                    cell.textureIndex
                );

                x += width;
            }
        }
    }
}

void greedyMeshNorthSouth(
    ChunkMeshData& meshData,
    const World& world,
    const Chunk& chunk,
    int sectionIndex,
    FaceDirection direction
) {
    std::array<FaceMaskCell, kMaskArea> mask {};
    const int baseX = chunk.getX() * Chunk::WIDTH;
    const int baseY = Chunk::getSectionMinY(sectionIndex);
    const int baseZ = chunk.getZ() * Chunk::DEPTH;
    const int neighborOffset = direction == FaceDirection::South ? 1 : -1;
    const int faceIndex = static_cast<int>(direction);

    for (int localZ = 0; localZ < Chunk::DEPTH; ++localZ) {
        for (int x = 0; x < Chunk::WIDTH; ++x) {
            for (int y = 0; y < kSectionSize; ++y) {
                const int worldX = baseX + x;
                const int worldY = baseY + y;
                const int worldZ = baseZ + localZ;
                const std::uint8_t blockId = world.getBlockID(worldX, worldY, worldZ);
                const std::uint8_t neighborId = world.getBlockID(worldX, worldY, worldZ + neighborOffset);

                mask[x + y * Chunk::WIDTH] =
                    isSolidOccluder(neighborId) ? FaceMaskCell{} : makeMaskCell(blockId, faceIndex);
            }
        }

        for (int y = 0; y < kSectionSize; ++y) {
            for (int x = 0; x < Chunk::WIDTH;) {
                FaceMaskCell cell = mask[x + y * Chunk::WIDTH];
                if (!cell.visible) {
                    ++x;
                    continue;
                }

                int width = 1;
                while (x + width < Chunk::WIDTH && sameCell(mask[x + width + y * Chunk::WIDTH], cell)) {
                    ++width;
                }

                int height = 1;
                bool canGrow = true;
                while (y + height < kSectionSize && canGrow) {
                    for (int k = 0; k < width; ++k) {
                        if (!sameCell(mask[x + k + (y + height) * Chunk::WIDTH], cell)) {
                            canGrow = false;
                            break;
                        }
                    }
                    if (canGrow) {
                        ++height;
                    }
                }

                for (int dy = 0; dy < height; ++dy) {
                    for (int dx = 0; dx < width; ++dx) {
                        mask[x + dx + (y + dy) * Chunk::WIDTH] = {};
                    }
                }

                const float planeZ = static_cast<float>(baseZ + localZ + (direction == FaceDirection::South ? 1 : 0));
                emitZQuad(
                    meshData,
                    direction,
                    static_cast<float>(baseX + x),
                    static_cast<float>(baseX + x + width),
                    static_cast<float>(baseY + y),
                    static_cast<float>(baseY + y + height),
                    planeZ,
                    cell.textureIndex
                );

                x += width;
            }
        }
    }
}

void greedyMeshWestEast(
    ChunkMeshData& meshData,
    const World& world,
    const Chunk& chunk,
    int sectionIndex,
    FaceDirection direction
) {
    std::array<FaceMaskCell, kMaskArea> mask {};
    const int baseX = chunk.getX() * Chunk::WIDTH;
    const int baseY = Chunk::getSectionMinY(sectionIndex);
    const int baseZ = chunk.getZ() * Chunk::DEPTH;
    const int neighborOffset = direction == FaceDirection::East ? 1 : -1;
    const int faceIndex = static_cast<int>(direction);

    for (int localX = 0; localX < Chunk::WIDTH; ++localX) {
        for (int z = 0; z < Chunk::DEPTH; ++z) {
            for (int y = 0; y < kSectionSize; ++y) {
                const int worldX = baseX + localX;
                const int worldY = baseY + y;
                const int worldZ = baseZ + z;
                const std::uint8_t blockId = world.getBlockID(worldX, worldY, worldZ);
                const std::uint8_t neighborId = world.getBlockID(worldX + neighborOffset, worldY, worldZ);

                mask[z + y * Chunk::DEPTH] =
                    isSolidOccluder(neighborId) ? FaceMaskCell{} : makeMaskCell(blockId, faceIndex);
            }
        }

        for (int y = 0; y < kSectionSize; ++y) {
            for (int z = 0; z < Chunk::DEPTH;) {
                FaceMaskCell cell = mask[z + y * Chunk::DEPTH];
                if (!cell.visible) {
                    ++z;
                    continue;
                }

                int width = 1;
                while (z + width < Chunk::DEPTH && sameCell(mask[z + width + y * Chunk::DEPTH], cell)) {
                    ++width;
                }

                int height = 1;
                bool canGrow = true;
                while (y + height < kSectionSize && canGrow) {
                    for (int k = 0; k < width; ++k) {
                        if (!sameCell(mask[z + k + (y + height) * Chunk::DEPTH], cell)) {
                            canGrow = false;
                            break;
                        }
                    }
                    if (canGrow) {
                        ++height;
                    }
                }

                for (int dy = 0; dy < height; ++dy) {
                    for (int dz = 0; dz < width; ++dz) {
                        mask[z + dz + (y + dy) * Chunk::DEPTH] = {};
                    }
                }

                const float planeX = static_cast<float>(baseX + localX + (direction == FaceDirection::East ? 1 : 0));
                emitXQuad(
                    meshData,
                    direction,
                    planeX,
                    static_cast<float>(baseY + y),
                    static_cast<float>(baseY + y + height),
                    static_cast<float>(baseZ + z),
                    static_cast<float>(baseZ + z + width),
                    cell.textureIndex
                );

                z += width;
            }
        }
    }
}
}

ChunkMeshData ChunkMesher::buildSectionMesh(const World& world, const Chunk& chunk, int sectionIndex) {
    ChunkMeshData meshData;

    const float baseX = static_cast<float>(chunk.getX() * Chunk::WIDTH);
    const float baseY = static_cast<float>(Chunk::getSectionMinY(sectionIndex));
    const float baseZ = static_cast<float>(chunk.getZ() * Chunk::DEPTH);

    meshData.bounds.min = {baseX, baseY, baseZ};
    meshData.bounds.max = {
        baseX + static_cast<float>(Chunk::WIDTH),
        baseY + static_cast<float>(Chunk::SECTION_HEIGHT),
        baseZ + static_cast<float>(Chunk::DEPTH)
    };

    greedyMeshTopBottom(meshData, world, chunk, sectionIndex, FaceDirection::Down);
    greedyMeshTopBottom(meshData, world, chunk, sectionIndex, FaceDirection::Up);
    greedyMeshNorthSouth(meshData, world, chunk, sectionIndex, FaceDirection::North);
    greedyMeshNorthSouth(meshData, world, chunk, sectionIndex, FaceDirection::South);
    greedyMeshWestEast(meshData, world, chunk, sectionIndex, FaceDirection::West);
    greedyMeshWestEast(meshData, world, chunk, sectionIndex, FaceDirection::East);

    return meshData;
}
