#include "world/Chunk.hpp"
#include "world/Block.hpp"
#include <cstring>
#include <algorithm>

Chunk::Chunk(int x, int z) : m_x(x), m_z(z) {
    m_blocks.resize(SIZE, 0);
    m_metadata.resize(SIZE / 2, 0);
    m_skylight.resize(SIZE / 2, 0xFF);
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
    uint8_t& block = m_blocks[getIndex(x, y, z)];
    if (block == id) return;

    int oldOpacity = Block::lightOpacity[block];
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

    if (y % SECTION_HEIGHT == 0 && sectionIndex > 0) {
        markSectionDirty(sectionIndex - 1);
    }
    if (y % SECTION_HEIGHT == SECTION_HEIGHT - 1 && sectionIndex + 1 < SECTION_COUNT) {
        markSectionDirty(sectionIndex + 1);
    }
}

void Chunk::generateHeightMap() {
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

void Chunk::generateBitmask() {
    m_primaryBitmask = 0;
    for (int i = 0; i < SECTION_COUNT; ++i) {
        bool hasBlocks = false;
        for (int j = 0; j < WIDTH * SECTION_HEIGHT * DEPTH; ++j) {
            if (m_blocks[i * WIDTH * SECTION_HEIGHT * DEPTH + j] != 0) {
                hasBlocks = true;
                break;
            }
        }
        if (hasBlocks) m_primaryBitmask |= (1 << i);
    }
}

bool Chunk::isSectionDirty(int sectionIndex) const {
    if (sectionIndex < 0 || sectionIndex >= SECTION_COUNT) return false;
    return m_sectionDirty[sectionIndex];
}

void Chunk::clearSectionDirty(int sectionIndex) {
    if (sectionIndex < 0 || sectionIndex >= SECTION_COUNT) return;
    m_sectionDirty[sectionIndex] = false;
}

uint32_t Chunk::getSectionVersion(int sectionIndex) const {
    return m_sectionVersions[sectionIndex];
}

void Chunk::touchSection(int sectionIndex) {
    markSectionDirty(sectionIndex);
}

void Chunk::markSectionDirtyInternal(int sectionIndex) {
    if (sectionIndex < 0 || sectionIndex >= SECTION_COUNT) return;
    m_sectionDirty[sectionIndex] = true;
    ++m_sectionVersions[sectionIndex];
}

void Chunk::markSectionDirty(int sectionIndex) {
    if (sectionIndex < 0 || sectionIndex >= SECTION_COUNT) {
        return;
    }

    m_sectionDirty[sectionIndex] = true;
    ++m_sectionVersions[sectionIndex];
}
