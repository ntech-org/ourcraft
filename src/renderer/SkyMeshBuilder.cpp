#include "renderer/SkyRenderer.hpp"
#include <cmath>
#include <glm/gtc/constants.hpp>

namespace {
constexpr unsigned int kWhite = 0xFFFFFFFFu;
class JavaRandom {
public:
    explicit JavaRandom(int64_t seed) : m_seed((seed ^ 0x5DEECE66DL) & ((1LL << 48) - 1)) {}
    int next(int bits) { m_seed = (m_seed * 0x5DEECE66DL + 0xBL) & ((1LL << 48) - 1); return static_cast<int>(m_seed >> (48 - bits)); }
    float nextFloat() { return static_cast<float>(next(24)) / static_cast<float>(1 << 24); }
    double nextDouble() { return ((static_cast<int64_t>(next(26)) << 27) + next(27)) / static_cast<double>(1LL << 53); }
private:
    int64_t m_seed;
};
}

void SkyRenderer::buildPlaneMeshes() {
    const float min = -384.0f, max = 384.0f, step = 64.0f;
    auto build = [&](SkyMesh& mesh, float y, bool flipX) {
        std::vector<SkyVertex> v; std::vector<unsigned int> idx;
        for (float x = min; x <= max; x += step) {
            for (float z = min; z <= max; z += step) {
                float x0 = flipX ? x + step : x, x1 = flipX ? x : x + step;
                unsigned int base = (unsigned int)v.size();
                v.push_back({x0, y, z, 0.0f, 0.0f, kWhite}); v.push_back({x1, y, z, 1.0f, 0.0f, kWhite});
                v.push_back({x1, y, z + step, 1.0f, 1.0f, kWhite}); v.push_back({x0, y, z + step, 0.0f, 1.0f, kWhite});
                idx.insert(idx.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
            }
        }
        uploadMesh(mesh, v, idx);
    };
    build(m_topSky, 16.0f, false); build(m_bottomSky, -16.0f, true);
}

void SkyRenderer::buildSunAndMoonMeshes() {
    std::vector<unsigned int> qi = {0, 1, 2, 0, 2, 3};
    {
        float s = 30.0f;
        std::vector<SkyVertex> v = {{-s, 100, -s, 0, 0, kWhite}, {s, 100, -s, 1, 0, kWhite}, {s, 100, s, 1, 1, kWhite}, {-s, 100, s, 0, 1, kWhite}};
        uploadMesh(m_sunMesh, v, qi);
    }
    {
        float s = 20.0f;
        std::vector<SkyVertex> v = {{-s, -100, s, 1, 1, kWhite}, {s, -100, s, 0, 1, kWhite}, {s, -100, -s, 0, 0, kWhite}, {-s, -100, -s, 1, 0, kWhite}};
        uploadMesh(m_moonMesh, v, qi);
    }
}

void SkyRenderer::buildStarMesh() {
    std::vector<SkyVertex> v; std::vector<unsigned int> idx;
    JavaRandom rand(10842LL);
    for (int i = 0; i < 1500; ++i) {
        double x = rand.nextFloat() * 2 - 1, y = rand.nextFloat() * 2 - 1, z = rand.nextFloat() * 2 - 1, size = 0.25 + rand.nextFloat() * 0.25;
        double l2 = x * x + y * y + z * z;
        if (l2 < 1.0 && l2 > 0.01) {
            double il = 1.0 / std::sqrt(l2); x *= il; y *= il; z *= il;
            double vx = x * 100.0, vy = y * 100.0, vz = z * 100.0;
            double yaw = std::atan2(x, z), pitch = std::atan2(std::sqrt(x * x + z * z), y), rot = rand.nextDouble() * M_PI * 2.0;
            double sy = std::sin(yaw), cy = std::cos(yaw), sp = std::sin(pitch), cp = std::cos(pitch), sr = std::sin(rot), cr = std::cos(rot);
            for (int j = 0; j < 4; ++j) {
                double ly = (double)((j & 2) - 1) * size, lz = (double)((j + 1 & 2) - 1) * size;
                double r1 = ly * cr - lz * sr, r2 = lz * cr + ly * sr, r3 = r1 * sp, r4 = -r1 * cp, r5 = r4 * sy - r2 * cy, r6 = r2 * sy + r4 * cy;
                v.push_back({(float)(vx + r5), (float)(vy + r3), (float)(vz + r6), 0.0f, 0.0f, kWhite});
            }
            unsigned int b = (unsigned int)v.size() - 4;
            idx.insert(idx.end(), {b, b + 1, b + 2, b, b + 2, b + 3});
        }
    }
    uploadMesh(m_starMesh, v, idx);
}
