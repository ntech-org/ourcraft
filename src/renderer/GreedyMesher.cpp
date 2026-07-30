#include "renderer/ChunkMesher.hpp"
#include "world/Block.hpp"
#include <cmath>
#include <vector>

constexpr int kSectionSize = Chunk::SECTION_HEIGHT;

struct FaceMaskCell {
    bool visible = false;
    int textureIndex = 0;
    float depth = 0.0f;
    float waterDepth = 0.0f;
    float skyLight = 15.0f;
    float blockLight = 0.0f;
};

static bool isGreedyRenderable(std::uint8_t id) {
    if (!id) return false;
    const Block* b = Block::blocksList[id];
    return b && (b->getRenderLayer() == BlockRenderLayer::Opaque || b->getRenderLayer() == BlockRenderLayer::Cutout) && b->isFullCube();
}

static FaceMaskCell makeMaskCell(std::uint8_t id, int face, float depth, float waterDepth, std::pair<int, int> light) {
    if (!isGreedyRenderable(id)) return {};
    return { true, Block::blocksList[id]->getTexture(face), depth, waterDepth, (float)light.first, (float)light.second };
}
static bool sameCell(const FaceMaskCell& l, const FaceMaskCell& r) {
    return l.visible == r.visible && l.textureIndex == r.textureIndex
        && std::abs(l.depth - r.depth) < 0.01f && std::abs(l.waterDepth - r.waterDepth) < 0.01f && l.skyLight == r.skyLight && l.blockLight == r.blockLight;
}
static void appendVertex(ChunkMeshData::Pass& p, float x, float y, float z, float u, float v,
    int tex, FaceDirection dir, float depth, float waterDepth, float skyLight, float blockLight) {
    p.vertices.push_back({x, y, z, u, v, kWhiteColor, (std::uint32_t)tex, (std::uint32_t)dir,
        -1000.0f, 0.0f, depth, skyLight, blockLight});
}

static bool shouldCull(std::uint8_t bid, std::uint8_t nid) {
    if (!nid) return false;
    const Block* b = Block::blocksList[bid], * n = Block::blocksList[nid];
    if (!b || !n) return false;
    if (n->isOccluder()) return true;
    return false;
}

void ChunkMesher::greedyMeshTopBottom(ChunkMeshData& md, const MeshingSnapshot& n, int si, int cx, int cz, bool up, const float* waterLevels) {
    int bx = cx * 16, by = si * 16, bz = cz * 16, off = up ? 1 : -1, f = up ? 1 : 0;
    FaceDirection dir = up ? FaceDirection::Up : FaceDirection::Down;
    thread_local FaceMaskCell tls_mask[256];
    for (int ly = 0; ly < 16; ++ly) {
        int gy = by + ly;
        for (int i = 0; i < 256; ++i) tls_mask[i] = {};
        for (int x = 0; x < 16; ++x) for (int z = 0; z < 16; ++z) {
            uint8_t bid = n.getBlockID(x, gy, z), nid = n.getBlockID(x, gy + off, z);
            float depth = waterLevels ? (gy < waterLevels[x * 16 + z] ? 1.0f : 0.0f) : getUnderwaterDepth(n, x, gy, z);
            if (!shouldCull(bid, nid)) tls_mask[x + z * 16] = makeMaskCell(bid, f, depth, 0.0f, n.getLightPair(bx + x, gy + off, bz + z));
        }
        for (int z = 0; z < 16; ++z) for (int x = 0; x < 16;) {
            FaceMaskCell c = tls_mask[x + z * 16]; if (!c.visible) { ++x; continue; }
            int w = 1; while (x + w < 16 && sameCell(tls_mask[x + w + z * 16], c)) ++w;
            int h = 1; bool grow = true;
            while (z + h < 16 && grow) { for (int k = 0; k < w; ++k) if (!sameCell(tls_mask[x + k + (z + h) * 16], c)) { grow = false; break; } if (grow) ++h; }
            for (int dx = 0; dx < w; ++dx) for (int dz = 0; dz < h; ++dz) tls_mask[x + dx + (z + dz) * 16] = {};
            float x0 = (float)x, x1 = (float)(x + w), yq = (float)(up ? ly + 1 : ly), z0 = (float)z, z1 = (float)(z + h);
            if (up) {
appendVertex(md.opaque, x1, yq, z1, (float)w, (float)h, c.textureIndex, dir, c.depth, c.waterDepth, c.skyLight, c.blockLight); appendVertex(md.opaque, x1, yq, z0, (float)w, 0, c.textureIndex, dir, c.depth, c.waterDepth, c.skyLight, c.blockLight);
                    appendVertex(md.opaque, x0, yq, z0, 0, 0, c.textureIndex, dir, c.depth, c.waterDepth, c.skyLight, c.blockLight); appendVertex(md.opaque, x0, yq, z1, 0, (float)h, c.textureIndex, dir, c.depth, c.waterDepth, c.skyLight, c.blockLight);
            } else {
                appendVertex(md.opaque, x0, yq, z1, 0, (float)h, c.textureIndex, dir, c.depth, c.waterDepth, c.skyLight, c.blockLight); appendVertex(md.opaque, x0, yq, z0, 0, 0, c.textureIndex, dir, c.depth, c.waterDepth, c.skyLight, c.blockLight);
                appendVertex(md.opaque, x1, yq, z0, (float)w, 0, c.textureIndex, dir, c.depth, c.waterDepth, c.skyLight, c.blockLight); appendVertex(md.opaque, x1, yq, z1, (float)w, (float)h, c.textureIndex, dir, c.depth, c.waterDepth, c.skyLight, c.blockLight);
            }
            appendIndices(md.opaque); x += w;
        }
    }
}

void ChunkMesher::greedyMeshNorthSouth(ChunkMeshData& md, const MeshingSnapshot& n, int si, int cx, int cz, bool south, const float* waterLevels) {
    int bx = cx * 16, by = si * 16, bz = cz * 16, off = south ? 1 : -1, f = south ? 3 : 2;
    FaceDirection dir = south ? FaceDirection::South : FaceDirection::North;
    thread_local FaceMaskCell tls_mask[256];
    for (int lz = 0; lz < 16; ++lz) {
        for (int i = 0; i < 256; ++i) tls_mask[i] = {};
        for (int x = 0; x < 16; ++x) for (int y = 0; y < 16; ++y) {
            int gy = by + y; uint8_t bid = n.getBlockID(x, gy, lz), nid = n.getBlockID(x, gy, lz + off);
            float depth = waterLevels ? (gy < waterLevels[x * 16 + lz] ? 1.0f : 0.0f) : getUnderwaterDepth(n, x, gy, lz);
            if (!shouldCull(bid, nid)) tls_mask[x + y * 16] = makeMaskCell(bid, f, depth, 0.0f, n.getLightPair(bx + x, gy, bz + lz + off));
        }
        for (int y = 0; y < 16; ++y) for (int x = 0; x < 16;) {
            FaceMaskCell c = tls_mask[x + y * 16]; if (!c.visible) { ++x; continue; }
            int w = 1; while (x + w < 16 && sameCell(tls_mask[x + w + y * 16], c)) ++w;
            int h = 1; bool grow = true;
            while (y + h < 16 && grow) { for (int k = 0; k < w; ++k) if (!sameCell(tls_mask[x + k + (y + h) * 16], c)) { grow = false; break; } if (grow) ++h; }
            for (int dx = 0; dx < w; ++dx) for (int dy = 0; dy < h; ++dy) tls_mask[x + dx + (y + dy) * 16] = {};
            float x0 = (float)x, x1 = (float)(x + w), y0 = (float)y, y1 = (float)(y + h), zq = (float)(lz + (south ? 1 : 0));
            if (!south) {
appendVertex(md.opaque, x0, y1, zq, 0, 0, c.textureIndex, dir, c.depth, c.waterDepth, c.skyLight, c.blockLight); appendVertex(md.opaque, x1, y1, zq, (float)w, 0, c.textureIndex, dir, c.depth, c.waterDepth, c.skyLight, c.blockLight);
                    appendVertex(md.opaque, x1, y0, zq, (float)w, (float)h, c.textureIndex, dir, c.depth, c.waterDepth, c.skyLight, c.blockLight); appendVertex(md.opaque, x0, y0, zq, 0, (float)h, c.textureIndex, dir, c.depth, c.waterDepth, c.skyLight, c.blockLight);
            } else {
                appendVertex(md.opaque, x0, y1, zq, 0, 0, c.textureIndex, dir, c.depth, c.waterDepth, c.skyLight, c.blockLight); appendVertex(md.opaque, x0, y0, zq, 0, (float)h, c.textureIndex, dir, c.depth, c.waterDepth, c.skyLight, c.blockLight);
                appendVertex(md.opaque, x1, y0, zq, (float)w, (float)h, c.textureIndex, dir, c.depth, c.waterDepth, c.skyLight, c.blockLight); appendVertex(md.opaque, x1, y1, zq, (float)w, 0, c.textureIndex, dir, c.depth, c.waterDepth, c.skyLight, c.blockLight);
            }
            appendIndices(md.opaque); x += w;
        }
    }
}

void ChunkMesher::greedyMeshWestEast(ChunkMeshData& md, const MeshingSnapshot& n, int si, int cx, int cz, bool east, const float* waterLevels) {
    int bx = cx * 16, by = si * 16, bz = cz * 16, off = east ? 1 : -1, f = east ? 5 : 4;
    FaceDirection dir = east ? FaceDirection::East : FaceDirection::West;
    thread_local FaceMaskCell tls_mask[256];
    for (int lx = 0; lx < 16; ++lx) {
        for (int i = 0; i < 256; ++i) tls_mask[i] = {};
        for (int z = 0; z < 16; ++z) for (int y = 0; y < 16; ++y) {
            int gy = by + y; uint8_t bid = n.getBlockID(lx, gy, z), nid = n.getBlockID(lx + off, gy, z);
                        float depth = waterLevels ? (gy < waterLevels[lx * 16 + z] ? 1.0f : 0.0f) : getUnderwaterDepth(n, lx, gy, z);
            if (!shouldCull(bid, nid)) tls_mask[z + y * 16] = makeMaskCell(bid, f, depth, 0.0f, n.getLightPair(bx + lx + off, gy, bz + z));
        }
        for (int y = 0; y < 16; ++y) for (int z = 0; z < 16;) {
            FaceMaskCell c = tls_mask[z + y * 16]; if (!c.visible) { ++z; continue; }
            int w = 1; while (z + w < 16 && sameCell(tls_mask[z + w + y * 16], c)) ++w;
            int h = 1; bool grow = true;
            while (y + h < 16 && grow) { for (int k = 0; k < w; ++k) if (!sameCell(tls_mask[z + k + (y + h) * 16], c)) { grow = false; break; } if (grow) ++h; }
            for (int dx = 0; dx < w; ++dx) for (int dy = 0; dy < h; ++dy) tls_mask[z + dx + (y + dy) * 16] = {};
            float xq = (float)(lx + (east ? 1 : 0)), y0 = (float)y, y1 = (float)(y + h), z0 = (float)z, z1 = (float)(z + w);
            if (!east) {
appendVertex(md.opaque, xq, y1, z1, (float)w, 0, c.textureIndex, dir, c.depth, c.waterDepth, c.skyLight, c.blockLight); appendVertex(md.opaque, xq, y1, z0, 0, 0, c.textureIndex, dir, c.depth, c.waterDepth, c.skyLight, c.blockLight);
                    appendVertex(md.opaque, xq, y0, z0, 0, (float)h, c.textureIndex, dir, c.depth, c.waterDepth, c.skyLight, c.blockLight); appendVertex(md.opaque, xq, y0, z1, (float)w, (float)h, c.textureIndex, dir, c.depth, c.waterDepth, c.skyLight, c.blockLight);
            } else {
                appendVertex(md.opaque, xq, y0, z1, 0, (float)h, c.textureIndex, dir, c.depth, c.waterDepth, c.skyLight, c.blockLight); appendVertex(md.opaque, xq, y0, z0, (float)w, (float)h, c.textureIndex, dir, c.depth, c.waterDepth, c.skyLight, c.blockLight);
                appendVertex(md.opaque, xq, y1, z0, (float)w, 0, c.textureIndex, dir, c.depth, c.waterDepth, c.skyLight, c.blockLight); appendVertex(md.opaque, xq, y1, z1, 0, 0, c.textureIndex, dir, c.depth, c.waterDepth, c.skyLight, c.blockLight);
            }

            appendIndices(md.opaque); z += w;
        }
    }
}

void ChunkMesher::crossMeshPass(ChunkMeshData& md, const MeshingSnapshot& n, int si, int cx, int cz) {
    int bx = cx * 16, by = si * 16, bz = cz * 16;
    for (int y = 0; y < 16; ++y) {
        for (int z = 0; z < 16; ++z) {
            for (int x = 0; x < 16; ++x) {
                int gy = by + y;
                uint8_t bid = n.getBlockID(x, gy, z);
                if (!bid) continue;
                const Block* b = Block::blocksList[bid];
                if (!b || b->getRenderShape() != BlockRenderShape::Cross) continue;

                int tex = b->getTexture(0);
                auto light = n.getLightPair(bx + x, gy, bz + z);
                float sl = (float)light.first, bl = (float)light.second;

                float x0 = (float)x, x1 = x0 + 1.0f;
                float y0 = (float)y, y1 = y0 + 1.0f;
                float z0 = (float)z, z1 = z0 + 1.0f;

                // First plane (double sided)
                appendVertex(md.opaque, x0, y1, z0, 0, 0, tex, FaceDirection::Up, 0, 0.0f, sl, bl);
                appendVertex(md.opaque, x0, y0, z0, 0, 1, tex, FaceDirection::Up, 0, 0.0f, sl, bl);
                appendVertex(md.opaque, x1, y0, z1, 1, 1, tex, FaceDirection::Up, 0, 0.0f, sl, bl);
                appendVertex(md.opaque, x1, y1, z1, 1, 0, tex, FaceDirection::Up, 0, 0.0f, sl, bl);
                appendIndices(md.opaque);

                appendVertex(md.opaque, x1, y1, z1, 1, 0, tex, FaceDirection::Up, 0, 0.0f, sl, bl);
                appendVertex(md.opaque, x1, y0, z1, 1, 1, tex, FaceDirection::Up, 0, 0.0f, sl, bl);
                appendVertex(md.opaque, x0, y0, z0, 0, 1, tex, FaceDirection::Up, 0, 0.0f, sl, bl);
                appendVertex(md.opaque, x0, y1, z0, 0, 0, tex, FaceDirection::Up, 0, 0.0f, sl, bl);
                appendIndices(md.opaque);

                // Second plane (double sided)
                appendVertex(md.opaque, x0, y1, z1, 0, 0, tex, FaceDirection::Up, 0, 0.0f, sl, bl);
                appendVertex(md.opaque, x0, y0, z1, 0, 1, tex, FaceDirection::Up, 0, 0.0f, sl, bl);
                appendVertex(md.opaque, x1, y0, z0, 1, 1, tex, FaceDirection::Up, 0, 0.0f, sl, bl);
                appendVertex(md.opaque, x1, y1, z0, 1, 0, tex, FaceDirection::Up, 0, 0.0f, sl, bl);
                appendIndices(md.opaque);

                appendVertex(md.opaque, x1, y1, z0, 1, 0, tex, FaceDirection::Up, 0, 0.0f, sl, bl);
                appendVertex(md.opaque, x1, y0, z0, 1, 1, tex, FaceDirection::Up, 0, 0.0f, sl, bl);
                appendVertex(md.opaque, x0, y0, z1, 0, 1, tex, FaceDirection::Up, 0, 0.0f, sl, bl);
                appendVertex(md.opaque, x0, y1, z1, 0, 0, tex, FaceDirection::Up, 0, 0.0f, sl, bl);
                appendIndices(md.opaque);
            }
        }
    }
}
