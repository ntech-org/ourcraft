#include "renderer/ChunkMesher.hpp"
#include "world/Block.hpp"
#include <cmath>
#include <vector>

namespace {
constexpr std::uint32_t kWhiteColor = 0xFFFFFFFFu;
constexpr int kSectionSize = Chunk::SECTION_HEIGHT;

enum class FaceDirection { Down = 0, Up = 1, North = 2, South = 3, West = 4, East = 5 };
struct FaceMaskCell { bool visible = false; int textureIndex = 0; float depth = 0.0f; float skyLight = 15.0f; float blockLight = 0.0f; };

bool isGreedyRenderable(std::uint8_t id) {
    if (!id) return false;
    const Block* b = Block::blocksList[id];
    return b && (b->getRenderLayer() == BlockRenderLayer::Opaque || b->getRenderLayer() == BlockRenderLayer::Cutout) && b->isFullCube();
}

FaceMaskCell makeMaskCell(std::uint8_t id, int face, float depth, std::pair<int, int> light) {
    if (!isGreedyRenderable(id)) return {};
    return { true, Block::blocksList[id]->getTexture(face), depth, (float)light.first, (float)light.second };
}

bool sameCell(const FaceMaskCell& l, const FaceMaskCell& r) {
    return l.visible == r.visible && l.textureIndex == r.textureIndex && std::abs(l.depth - r.depth) < 0.01f && l.skyLight == r.skyLight && l.blockLight == r.blockLight;
}

void appendVertex(ChunkMeshData::Pass& p, float x, float y, float z, float u, float v, int tex, FaceDirection dir, float depth, float skyLight, float blockLight) {
    p.vertices.push_back({x, y, z, u, v, kWhiteColor, (std::uint32_t)tex, (std::uint32_t)dir, -1000.0f, 0.0f, depth, skyLight, blockLight});
}

void appendIndices(ChunkMeshData::Pass& p) {
    std::uint32_t b = (std::uint32_t)p.vertices.size() - 4;
    p.indices.insert(p.indices.end(), {b, b + 1, b + 2, b, b + 2, b + 3}); p.quadCount++;
}

bool shouldCull(std::uint8_t bid, std::uint8_t nid) {
    if (!nid) return false;
    const Block* b = Block::blocksList[bid], * n = Block::blocksList[nid];
    if (!b || !n) return false;
    if (n->isOccluder()) return true;
    return false;
}

float getWaterDepth(const IBlockAccess& n, int x, int y, int z) {
    float d = 0;
    for (int i = 0; i < 64 && (y + i) < Chunk::HEIGHT; ++i) if (n.getBlockMaterial(x, y + i, z).isLiquid()) d += 1.0f;
    return d;
}
}

void ChunkMesher::greedyMeshTopBottom(ChunkMeshData& md, const IBlockAccess& n, int si, int cx, int cz, bool up) {
    int bx = cx * 16, by = si * 16, bz = cz * 16, off = up ? 1 : -1, f = up ? 1 : 0;
    FaceDirection dir = up ? FaceDirection::Up : FaceDirection::Down;
    for (int ly = 0; ly < 16; ++ly) {
        std::vector<FaceMaskCell> mask(256); int gy = by + ly;
        for (int x = 0; x < 16; ++x) for (int z = 0; z < 16; ++z) {
            uint8_t bid = n.getBlockID(x, gy, z), nid = n.getBlockID(x, gy + off, z);
            if (!shouldCull(bid, nid)) mask[x + z * 16] = makeMaskCell(bid, f, getWaterDepth(n, x, gy + (up ? 1 : 0), z), n.getLightPair(bx + x, gy + off, bz + z));
        }
        for (int z = 0; z < 16; ++z) for (int x = 0; x < 16;) {
            FaceMaskCell c = mask[x + z * 16]; if (!c.visible) { ++x; continue; }
            int w = 1; while (x + w < 16 && sameCell(mask[x + w + z * 16], c)) ++w;
            int h = 1; bool grow = true;
            while (z + h < 16 && grow) { for (int k = 0; k < w; ++k) if (!sameCell(mask[x + k + (z + h) * 16], c)) { grow = false; break; } if (grow) ++h; }
            for (int dx = 0; dx < w; ++dx) for (int dz = 0; dz < h; ++dz) mask[x + dx + (z + dz) * 16] = {};
            float x0 = (float)(bx + x), x1 = (float)(bx + x + w), yq = (float)(gy + (up ? 1 : 0)), z0 = (float)(bz + z), z1 = (float)(bz + z + h);
            if (up) {
                appendVertex(md.opaque, x1, yq, z1, (float)w, (float)h, c.textureIndex, dir, c.depth, c.skyLight, c.blockLight); appendVertex(md.opaque, x1, yq, z0, (float)w, 0, c.textureIndex, dir, c.depth, c.skyLight, c.blockLight);
                appendVertex(md.opaque, x0, yq, z0, 0, 0, c.textureIndex, dir, c.depth, c.skyLight, c.blockLight); appendVertex(md.opaque, x0, yq, z1, 0, (float)h, c.textureIndex, dir, c.depth, c.skyLight, c.blockLight);
            } else {
                appendVertex(md.opaque, x0, yq, z1, 0, (float)h, c.textureIndex, dir, c.depth, c.skyLight, c.blockLight); appendVertex(md.opaque, x0, yq, z0, 0, 0, c.textureIndex, dir, c.depth, c.skyLight, c.blockLight);
                appendVertex(md.opaque, x1, yq, z0, (float)w, 0, c.textureIndex, dir, c.depth, c.skyLight, c.blockLight); appendVertex(md.opaque, x1, yq, z1, (float)w, (float)h, c.textureIndex, dir, c.depth, c.skyLight, c.blockLight);
            }
            appendIndices(md.opaque); x += w;
        }
    }
}

void ChunkMesher::greedyMeshNorthSouth(ChunkMeshData& md, const IBlockAccess& n, int si, int cx, int cz, bool south) {
    int bx = cx * 16, by = si * 16, bz = cz * 16, off = south ? 1 : -1, f = south ? 3 : 2;
    FaceDirection dir = south ? FaceDirection::South : FaceDirection::North;
    for (int lz = 0; lz < 16; ++lz) {
        std::vector<FaceMaskCell> mask(256);
        for (int x = 0; x < 16; ++x) for (int y = 0; y < 16; ++y) {
            int gy = by + y; uint8_t bid = n.getBlockID(x, gy, lz), nid = n.getBlockID(x, gy, lz + off);
            if (!shouldCull(bid, nid)) mask[x + y * 16] = makeMaskCell(bid, f, getWaterDepth(n, x, gy, lz + (south ? 1 : 0)), n.getLightPair(bx + x, gy, bz + lz + off));
        }
        for (int y = 0; y < 16; ++y) for (int x = 0; x < 16;) {
            FaceMaskCell c = mask[x + y * 16]; if (!c.visible) { ++x; continue; }
            int w = 1; while (x + w < 16 && sameCell(mask[x + w + y * 16], c)) ++w;
            int h = 1; bool grow = true;
            while (y + h < 16 && grow) { for (int k = 0; k < w; ++k) if (!sameCell(mask[x + k + (y + h) * 16], c)) { grow = false; break; } if (grow) ++h; }
            for (int dx = 0; dx < w; ++dx) for (int dy = 0; dy < h; ++dy) mask[x + dx + (y + dy) * 16] = {};
            float x0 = (float)(bx + x), x1 = (float)(bx + x + w), y0 = (float)(by + y), y1 = (float)(by + y + h), zq = (float)(bz + lz + (south ? 1 : 0));
            if (!south) {
                appendVertex(md.opaque, x0, y1, zq, 0, 0, c.textureIndex, dir, c.depth, c.skyLight, c.blockLight); appendVertex(md.opaque, x1, y1, zq, (float)w, 0, c.textureIndex, dir, c.depth, c.skyLight, c.blockLight);
                appendVertex(md.opaque, x1, y0, zq, (float)w, (float)h, c.textureIndex, dir, c.depth, c.skyLight, c.blockLight); appendVertex(md.opaque, x0, y0, zq, 0, (float)h, c.textureIndex, dir, c.depth, c.skyLight, c.blockLight);
            } else {
                appendVertex(md.opaque, x0, y1, zq, 0, 0, c.textureIndex, dir, c.depth, c.skyLight, c.blockLight); appendVertex(md.opaque, x0, y0, zq, 0, (float)h, c.textureIndex, dir, c.depth, c.skyLight, c.blockLight);
                appendVertex(md.opaque, x1, y0, zq, (float)w, (float)h, c.textureIndex, dir, c.depth, c.skyLight, c.blockLight); appendVertex(md.opaque, x1, y1, zq, (float)w, 0, c.textureIndex, dir, c.depth, c.skyLight, c.blockLight);
            }
            appendIndices(md.opaque); x += w;
        }
    }
}

void ChunkMesher::greedyMeshWestEast(ChunkMeshData& md, const IBlockAccess& n, int si, int cx, int cz, bool east) {
    int bx = cx * 16, by = si * 16, bz = cz * 16, off = east ? 1 : -1, f = east ? 5 : 4;
    FaceDirection dir = east ? FaceDirection::East : FaceDirection::West;
    for (int lx = 0; lx < 16; ++lx) {
        std::vector<FaceMaskCell> mask(256);
        for (int z = 0; z < 16; ++z) for (int y = 0; y < 16; ++y) {
            int gy = by + y; uint8_t bid = n.getBlockID(lx, gy, z), nid = n.getBlockID(lx + off, gy, z);
            if (!shouldCull(bid, nid)) mask[z + y * 16] = makeMaskCell(bid, f, getWaterDepth(n, lx + (east ? 1 : 0), gy, z), n.getLightPair(bx + lx + off, gy, bz + z));
        }
        for (int y = 0; y < 16; ++y) for (int z = 0; z < 16;) {
            FaceMaskCell c = mask[z + y * 16]; if (!c.visible) { ++z; continue; }
            int w = 1; while (z + w < 16 && sameCell(mask[z + w + y * 16], c)) ++w;
            int h = 1; bool grow = true;
            while (y + h < 16 && grow) { for (int k = 0; k < w; ++k) if (!sameCell(mask[z + k + (y + h) * 16], c)) { grow = false; break; } if (grow) ++h; }
            for (int dx = 0; dx < w; ++dx) for (int dy = 0; dy < h; ++dy) mask[z + dx + (y + dy) * 16] = {};
            float xq = (float)(bx + lx + (east ? 1 : 0)), y0 = (float)(by + y), y1 = (float)(by + y + h), z0 = (float)(bz + z), z1 = (float)(bz + z + w);
            if (!east) {
                appendVertex(md.opaque, xq, y1, z1, (float)w, 0, c.textureIndex, dir, c.depth, c.skyLight, c.blockLight); appendVertex(md.opaque, xq, y1, z0, 0, 0, c.textureIndex, dir, c.depth, c.skyLight, c.blockLight);
                appendVertex(md.opaque, xq, y0, z0, 0, (float)h, c.textureIndex, dir, c.depth, c.skyLight, c.blockLight); appendVertex(md.opaque, xq, y0, z1, (float)w, (float)h, c.textureIndex, dir, c.depth, c.skyLight, c.blockLight);
            } else {
                appendVertex(md.opaque, xq, y0, z1, 0, (float)h, c.textureIndex, dir, c.depth, c.skyLight, c.blockLight); appendVertex(md.opaque, xq, y0, z0, (float)w, (float)h, c.textureIndex, dir, c.depth, c.skyLight, c.blockLight);
                appendVertex(md.opaque, xq, y1, z0, (float)w, 0, c.textureIndex, dir, c.depth, c.skyLight, c.blockLight); appendVertex(md.opaque, xq, y1, z1, 0, 0, c.textureIndex, dir, c.depth, c.skyLight, c.blockLight);
            }
            appendIndices(md.opaque); z += w;
        }
    }
}
