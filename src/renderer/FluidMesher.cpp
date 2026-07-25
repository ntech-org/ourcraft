#include "renderer/ChunkMesher.hpp"
#include "world/Block.hpp"
#include "world/BlockFluid.hpp"
#include <cmath>

namespace {

void appendVertex(ChunkMeshData::Pass& p, float x, float y, float z, float u, float v, int tex, FaceDirection dir, float flow, float liq, float depth, float skyLight, float blockLight) {
    p.vertices.push_back({x, y, z, u, v, kWhiteColor, (std::uint32_t)tex, (std::uint32_t)dir, flow, liq, depth, skyLight, blockLight});
}

bool shouldCull(std::uint8_t bid, std::uint8_t nid) {
    if (!nid) return false;
    const Block* b = Block::blocksList[bid], * n = Block::blocksList[nid];
    if (!b || !n) return false;
    if (bid == nid && b->isSameTypeCulled()) return true;
    if (n->isOccluder()) return true;
    if (b->blockMaterial == n->blockMaterial) return true;
    return false;
}

float getCornerHeight(const IBlockAccess& n, int x, int y, int z, const Material& mat) {
    float s = 0; int c = 0;
    for (int i = 0; i < 4; ++i) {
        int nx = x - (i & 1), nz = z - (i >> 1 & 1);
        if (n.getBlockMaterial(nx, y + 1, nz) == mat) return 1.0f;
        const Material& m = n.getBlockMaterial(nx, y, nz);
        if (m != mat) { if (!m.isSolid()) { s += 1.0f; c++; } }
        else { int meta = n.getBlockMetadata(nx, y, nz); if (meta >= 8 || meta == 0) { s += BlockFluid::getPercentAir(meta) * 10; c += 10; } s += BlockFluid::getPercentAir(meta); c++; }
    }
    return 1.0f - s / (float)c;
}
}

void ChunkMesher::fluidMeshPass(ChunkMeshData& md, const IBlockAccess& n, int si, int cx, int cz, const float* waterLevels) {
    int bx = cx * 16, by = si * 16, bz = cz * 16;
    for (int y = 0; y < 16; ++y) {
        int gy = by + y;
        for (int x = 0; x < 16; ++x) for (int z = 0; z < 16; ++z) {
            uint8_t bid = n.getBlockID(x, gy, z); if (bid < 8 || bid > 11) continue;
            const Block* b = Block::blocksList[bid]; 
            if (!b) continue;
            const Material& mat = b->blockMaterial;
            float h00 = getCornerHeight(n, x, gy, z, mat), h01 = getCornerHeight(n, x, gy, z + 1, mat), h11 = getCornerHeight(n, x + 1, gy, z + 1, mat), h10 = getCornerHeight(n, x + 1, gy, z, mat);
            float fx = (float)x, fy = (float)y, fz = (float)z;
            bool water = (bid == 8 || bid == 9); float liq = water ? 1.0f : 2.0f;
            float d0, d1;
            if (waterLevels) {
                float wl = waterLevels[x * 16 + z];
                d0 = (gy < wl) ? 1.0f : 0.0f;
                d1 = ((gy + 1) < wl) ? 1.0f : 0.0f;
            } else {
                d0 = getUnderwaterDepth(n, x, gy, z);
                d1 = getUnderwaterDepth(n, x, gy + 1, z);
            }
            ChunkMeshData::Pass& pass = water ? md.translucent : md.opaque;
            uint8_t aboveID = n.getBlockID(x, gy + 1, z);
            bool cullTop = false;
            if (aboveID) {
                const Block* above = Block::blocksList[aboveID];
                if (above && b->blockMaterial == above->blockMaterial) cullTop = true;
            }
            if (!cullTop) {
                float flow = (float)BlockFluid::getFlowDirection(n, x, gy, z, mat); int tex = (flow > -999.0f) ? b->getTexture(2) : b->getTexture(1);
                auto light = n.getLightPair(bx + x, gy + 1, bz + z); float sl = (float)light.first, bl = (float)light.second;
                appendVertex(pass, fx, fy + h00, fz, 0, 0, tex, FaceDirection::Up, flow, liq, d1, sl, bl); appendVertex(pass, fx, fy + h01, fz + 1, 0, 1, tex, FaceDirection::Up, flow, liq, d1, sl, bl);
                appendVertex(pass, fx + 1, fy + h11, fz + 1, 1, 1, tex, FaceDirection::Up, flow, liq, d1, sl, bl); appendVertex(pass, fx + 1, fy + h10, fz, 1, 0, tex, FaceDirection::Up, flow, liq, d1, sl, bl);
                appendIndices(pass);

                // Render the same surface quad facing DOWN (for underwater view)
                if (water) {
                    appendVertex(pass, fx + 1, fy + h10, fz, 1, 0, tex, FaceDirection::Down, flow, liq, d1, sl, bl);
                    appendVertex(pass, fx + 1, fy + h11, fz + 1, 1, 1, tex, FaceDirection::Down, flow, liq, d1, sl, bl);
                    appendVertex(pass, fx, fy + h01, fz + 1, 0, 1, tex, FaceDirection::Down, flow, liq, d1, sl, bl);
                    appendVertex(pass, fx, fy + h00, fz, 0, 0, tex, FaceDirection::Down, flow, liq, d1, sl, bl);
                    appendIndices(pass);
                }
            }

            if (!shouldCull(bid, n.getBlockID(x, gy - 1, z))) {
                int tex = b->getTexture(0); auto light = n.getLightPair(bx + x, gy - 1, bz + z); float sl = (float)light.first, bl = (float)light.second;
                appendVertex(pass, fx, fy, fz + 1, 0, 1, tex, FaceDirection::Down, -1000.0f, liq, d0, sl, bl);
                appendVertex(pass, fx, fy, fz, 0, 0, tex, FaceDirection::Down, -1000.0f, liq, d0, sl, bl); appendVertex(pass, fx + 1, fy, fz, 1, 0, tex, FaceDirection::Down, -1000.0f, liq, d0, sl, bl);
                appendVertex(pass, fx + 1, fy, fz + 1, 1, 1, tex, FaceDirection::Down, -1000.0f, liq, d0, sl, bl); appendIndices(pass);
            }
            for (int f = 0; f < 4; ++f) {
                FaceDirection dir; int nx = x, nz = z; float v33, v35, v34, v36, h1s, h2s;
                if (f == 0) { dir = FaceDirection::North; nz--; h1s = h00; h2s = h10; v33 = fx; v35 = fx + 1; v34 = fz; v36 = fz; }
                else if (f == 1) { dir = FaceDirection::South; nz++; h1s = h11; h2s = h01; v33 = fx + 1; v35 = fx; v34 = fz + 1; v36 = fz + 1; }
                else if (f == 2) { dir = FaceDirection::West; nx--; h1s = h01; h2s = h00; v33 = fx; v35 = fx; v34 = fz + 1; v36 = fz; }
                else { dir = FaceDirection::East; nx++; h1s = h10; h2s = h11; v33 = fx + 1; v35 = fx + 1; v34 = fz; v36 = fz + 1; }
                if (!shouldCull(bid, n.getBlockID(nx, gy, nz))) {
                    int tex = b->getTexture(f + 2); 
                    auto light = n.getLightPair(bx + nx, gy, bz + nz);
                    float sl = (float)light.first, bl = (float)light.second;
                    appendVertex(pass, v33, fy + h1s, v34, 0, 1 - h1s, tex, dir, -1000.0f, liq, d1, sl, bl);
                    appendVertex(pass, v35, fy + h2s, v36, 1, 1 - h2s, tex, dir, -1000.0f, liq, d1, sl, bl); appendVertex(pass, v35, fy, v36, 1, 1, tex, dir, -1000.0f, liq, d0, sl, bl);
                    appendVertex(pass, v33, fy, v34, 0, 1, tex, dir, -1000.0f, liq, d0, sl, bl); appendIndices(pass);
                }

            }
        }
    }
}
