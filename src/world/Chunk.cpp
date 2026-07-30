#include "world/Chunk.hpp"
#include "world/Block.hpp"
#include <cstring>
#include <algorithm>

Chunk::Chunk(int x, int z) : m_x(x), m_z(z) {
    m_blocks.resize(SIZE, 0);
    m_metadata.resize(SIZE / 2, 0);
    m_skylight.resize(SIZE / 2, 0);
    m_blocklight.resize(SIZE / 2, 0);
    m_heightMap.resize(WIDTH * DEPTH, 0);
}

uint8_t Chunk::getBlockID(int x, int y, int z) const {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT || z < 0 || z >= DEPTH) return 0;
    return m_blocks[getIndex(x, y, z)];
}

uint8_t Chunk::getBlockMetadata(int x, int y, int z) const {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT || z < 0 || z >= DEPTH) return 0;
    int idx = getIndex(x, y, z);
    return (idx & 1) == 0 ? (m_metadata[idx >> 1] & 0x0F) : ((m_metadata[idx >> 1] >> 4) & 0x0F);
}

void Chunk::setBlockMetadata(int x, int y, int z, uint8_t meta) {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT || z < 0 || z >= DEPTH) return;
    std::lock_guard<std::mutex> lock(m_blockMutex);
    int idx = getIndex(x, y, z);
    uint8_t oldMeta = (idx & 1) == 0 ? (m_metadata[idx >> 1] & 0x0F) : ((m_metadata[idx >> 1] >> 4) & 0x0F);
    if (oldMeta == meta) return;

    if ((idx & 1) == 0) {
        m_metadata[idx >> 1] = (m_metadata[idx >> 1] & 0xF0) | (meta & 0x0F);
    } else {
        m_metadata[idx >> 1] = (m_metadata[idx >> 1] & 0x0F) | ((meta & 0x0F) << 4);
    }

    const int sectionIndex = getSectionIndex(y);
    markSectionDirty(sectionIndex);
}

int Chunk::getLight(LightType type, int x, int y, int z) const {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT || z < 0 || z >= DEPTH) return 0;
    const std::vector<uint8_t>& data = (type == LightType::Sky) ? m_skylight : m_blocklight;
    return getLightValue(data, getIndex(x, y, z));
}

void Chunk::setLight(LightType type, int x, int y, int z, int val) {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT || z < 0 || z >= DEPTH) return;
    std::vector<uint8_t>& data = (type == LightType::Sky) ? m_skylight : m_blocklight;

    {
        std::lock_guard<std::mutex> lock(m_lightMutex);
        setLightValue(data, getIndex(x, y, z), val);
    }

    const int sectionIndex = getSectionIndex(y);
    markSectionDirty(sectionIndex);
}

void Chunk::setBlockID(int x, int y, int z, uint8_t id) {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT || z < 0 || z >= DEPTH) return;
    std::lock_guard<std::mutex> lock(m_blockMutex);
    uint8_t& block = m_blocks[getIndex(x, y, z)];
    if (block == id) return;

    int oldOpacity = Block::lightOpacity[block];
    const bool occupancyChanged = (block == 0) != (id == 0);
    const bool waterChanged = (block == 8 || block == 9) != (id == 8 || id == 9);
    block = id;
    int newOpacity = Block::lightOpacity[id];

    if (oldOpacity != newOpacity) {
        int h = getHeight(x, z);
        if (y >= h - 1) {
            int ty = HEIGHT - 1;
            while (ty >= 0 && Block::lightOpacity[getBlockID(x, ty, z)] == 0) ty--;
            setHeight(x, z, ty + 1);
        }
    }

    const int sectionIndex = getSectionIndex(y);
    markSectionDirty(sectionIndex);

    if (occupancyChanged) {
        if (id != 0) {
            m_sectionNonAirCounts[sectionIndex].fetch_add(1, std::memory_order_relaxed);
            m_sectionNonEmpty.fetch_or(1u << sectionIndex, std::memory_order_release);
        } else if (m_sectionNonAirCounts[sectionIndex].fetch_sub(1, std::memory_order_relaxed) == 1) {
            m_sectionNonEmpty.fetch_and(static_cast<uint16_t>(~(1u << sectionIndex)), std::memory_order_release);
        }
    }
    if (waterChanged) {
        float& level = m_waterLevels[x + z * WIDTH];
        if (id == 8 || id == 9) {
            level = std::max(level, static_cast<float>(y));
        } else if (level == static_cast<float>(y)) {
            level = -1.0f;
            for (int ty = y - 1; ty >= 0; --ty) {
                const uint8_t columnID = m_blocks[getIndex(x, ty, z)];
                if (columnID == 8 || columnID == 9) {
                    level = static_cast<float>(ty);
                    break;
                }
            }
        }
    }

    if (y % SECTION_HEIGHT == 0 && sectionIndex > 0) {
        markSectionDirty(sectionIndex - 1);
    }
    if (y % SECTION_HEIGHT == SECTION_HEIGHT - 1 && sectionIndex + 1 < SECTION_COUNT) {
        markSectionDirty(sectionIndex + 1);
    }
}

void Chunk::setBlockIDSafe(int x, int y, int z, uint8_t id) {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT || z < 0 || z >= DEPTH) return;
    uint8_t& block = m_blocks[getIndex(x, y, z)];
    if (block == id) return;

    int oldOpacity = Block::lightOpacity[block];
    const bool occupancyChanged = (block == 0) != (id == 0);
    const bool waterChanged = (block == 8 || block == 9) != (id == 8 || id == 9);
    block = id;
    int newOpacity = Block::lightOpacity[id];

    if (oldOpacity != newOpacity) {
        int h = getHeight(x, z);
        if (y >= h - 1) {
            int ty = HEIGHT - 1;
            while (ty >= 0 && Block::lightOpacity[getBlockID(x, ty, z)] == 0) ty--;
            setHeight(x, z, ty + 1);
        }
    }

    const int sectionIndex = getSectionIndex(y);
    markSectionDirty(sectionIndex);

    if (occupancyChanged) {
        if (id != 0) {
            m_sectionNonAirCounts[sectionIndex].fetch_add(1, std::memory_order_relaxed);
            m_sectionNonEmpty.fetch_or(1u << sectionIndex, std::memory_order_release);
        } else if (m_sectionNonAirCounts[sectionIndex].fetch_sub(1, std::memory_order_relaxed) == 1) {
            m_sectionNonEmpty.fetch_and(static_cast<uint16_t>(~(1u << sectionIndex)), std::memory_order_release);
        }
    }
    if (waterChanged) {
        float& level = m_waterLevels[x + z * WIDTH];
        if (id == 8 || id == 9) {
            level = std::max(level, static_cast<float>(y));
        } else if (level == static_cast<float>(y)) {
            level = -1.0f;
            for (int ty = y - 1; ty >= 0; --ty) {
                const uint8_t columnID = m_blocks[getIndex(x, ty, z)];
                if (columnID == 8 || columnID == 9) {
                    level = static_cast<float>(ty);
                    break;
                }
            }
        }
    }

    if (y % SECTION_HEIGHT == 0 && sectionIndex > 0) {
        markSectionDirty(sectionIndex - 1);
    }
    if (y % SECTION_HEIGHT == SECTION_HEIGHT - 1 && sectionIndex + 1 < SECTION_COUNT) {
        markSectionDirty(sectionIndex + 1);
    }
}

void Chunk::generateHeightMap() {
    std::lock_guard<std::mutex> lock(m_blockMutex);
    for (int x = 0; x < WIDTH; ++x) {
        for (int z = 0; z < DEPTH; ++z) {
            int y = HEIGHT - 1;
            while (y >= 0 && Block::lightOpacity[getBlockID(x, y, z)] == 0) {
                y--;
            }
            setHeight(x, z, y + 1);
        }
    }
}

void Chunk::fillColumnSkyLight15(int x, int z, int minY, int maxY) {
    if (minY > maxY || minY < 0 || maxY >= HEIGHT) return;
    // Y-major: indices (x<<11)|(z<<7)|y are consecutive in y.
    int base = (x << 11) | (z << 7);
    int y = minY;
    // Align to even index so we can write full 0xFF bytes (two nibbles of 15).
    if (y & 1) {
        setLightValue(m_skylight, base + y, 15);
        ++y;
    }
    int end = maxY + 1;
    int evenEnd = end & ~1;
    for (; y < evenEnd; y += 2) {
        m_skylight[(base + y) >> 1] = 0xFF;
    }
    if (y <= maxY) {
        setLightValue(m_skylight, base + y, 15);
    }
}

void Chunk::generateBitmask() {
    std::lock_guard<std::mutex> lock(m_blockMutex);
    m_primaryBitmask = 0;
    uint16_t nonEmptyMask = 0;
    for (int si = 0; si < SECTION_COUNT; ++si) {
        uint16_t nonAirCount = 0;
        for (int x = 0; x < WIDTH; ++x) {
            for (int z = 0; z < DEPTH; ++z) {
                for (int y = si * SECTION_HEIGHT; y < (si + 1) * SECTION_HEIGHT; ++y) {
                    nonAirCount += m_blocks[getIndex(x, y, z)] != 0;
                }
            }
        }
        m_sectionNonAirCounts[si].store(nonAirCount, std::memory_order_relaxed);
        if (nonAirCount > 0) {
            m_primaryBitmask |= (1 << si);
            nonEmptyMask |= (1u << si);
        }
    }
    m_sectionNonEmpty.store(nonEmptyMask, std::memory_order_release);
}

bool Chunk::isSectionDirty(int sectionIndex) const {
    if (sectionIndex < 0 || sectionIndex >= SECTION_COUNT) return false;
    return m_sectionDirty[sectionIndex].load(std::memory_order_acquire);
}

void Chunk::clearSectionDirty(int sectionIndex) {
    if (sectionIndex < 0 || sectionIndex >= SECTION_COUNT) return;
    m_sectionDirty[sectionIndex].store(false, std::memory_order_release);
}

uint32_t Chunk::getSectionVersion(int sectionIndex) const {
    return m_sectionVersions[sectionIndex].load(std::memory_order_acquire);
}

void Chunk::touchSection(int sectionIndex) {
    markSectionDirty(sectionIndex);
}

void Chunk::markSectionDirtyInternal(int sectionIndex) {
    if (sectionIndex < 0 || sectionIndex >= SECTION_COUNT) return;
    bool expected = false;
    if (m_sectionDirty[sectionIndex].compare_exchange_strong(
            expected, true, std::memory_order_acq_rel)) {
        m_sectionVersions[sectionIndex].fetch_add(1, std::memory_order_release);
        if (m_sectionDirtyCallback) m_sectionDirtyCallback(sectionIndex);
    }
}

void Chunk::markSectionDirty(int sectionIndex) {
    if (sectionIndex < 0 || sectionIndex >= SECTION_COUNT) {
        return;
    }

    bool expected = false;
    if (m_sectionDirty[sectionIndex].compare_exchange_strong(
            expected, true, std::memory_order_acq_rel)) {
        m_sectionVersions[sectionIndex].fetch_add(1, std::memory_order_release);
        if (m_sectionDirtyCallback) m_sectionDirtyCallback(sectionIndex);
    }
}

void Chunk::computeWaterLevels() {
    std::lock_guard<std::mutex> lock(m_blockMutex);
    m_hasAnyWater = false;
    uint16_t nonEmptyMask = 0;
    for (int x = 0; x < WIDTH; ++x) {
        for (int z = 0; z < DEPTH; ++z) {
            float wl = -1.0f;
            for (int y = HEIGHT - 1; y >= 0; --y) {
                uint8_t bid = m_blocks[getIndex(x, y, z)];
                if (bid == 8 || bid == 9) {
                    wl = (float)y;
                    m_hasAnyWater = true;
                    break;
                }
            }
            m_waterLevels[x + z * WIDTH] = wl;
        }
    }
    for (int si = 0; si < SECTION_COUNT; ++si) {
        uint16_t nonAirCount = 0;
        for (int x = 0; x < WIDTH; ++x) {
            for (int z = 0; z < DEPTH; ++z) {
                for (int y = si * SECTION_HEIGHT; y < (si + 1) * SECTION_HEIGHT; ++y) {
                    nonAirCount += m_blocks[getIndex(x, y, z)] != 0;
                }
            }
        }
        m_sectionNonAirCounts[si].store(nonAirCount, std::memory_order_relaxed);
        if (nonAirCount > 0) nonEmptyMask |= (1u << si);
    }
    m_sectionNonEmpty.store(nonEmptyMask, std::memory_order_release);
}

void Chunk::recomputeSectionNonEmpty() {
    std::lock_guard<std::mutex> lock(m_blockMutex);
    uint16_t nonEmptyMask = 0;
    for (int si = 0; si < SECTION_COUNT; ++si) {
        uint16_t nonAirCount = 0;
        for (int x = 0; x < WIDTH; ++x) {
            for (int z = 0; z < DEPTH; ++z) {
                for (int y = si * SECTION_HEIGHT; y < (si + 1) * SECTION_HEIGHT; ++y) {
                    nonAirCount += m_blocks[getIndex(x, y, z)] != 0;
                }
            }
        }
        m_sectionNonAirCounts[si].store(nonAirCount, std::memory_order_relaxed);
        if (nonAirCount > 0) nonEmptyMask |= (1u << si);
    }
    m_sectionNonEmpty.store(nonEmptyMask, std::memory_order_release);
}
