#include "world/Chunk.hpp"

Chunk::Chunk(int x, int z) : m_x(x), m_z(z) {
    m_blocks.resize(SIZE, 0);
    m_metadata.resize(SIZE, 0);
    m_sectionDirty.fill(true);
    m_sectionVersions.fill(1);
}

uint8_t Chunk::getBlockID(int x, int y, int z) const {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT || z < 0 || z >= DEPTH) return 0;
    return m_blocks[getIndex(x, y, z)];
}

uint8_t Chunk::getBlockMetadata(int x, int y, int z) const {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT || z < 0 || z >= DEPTH) return 0;
    return m_metadata[getIndex(x, y, z)];
}

void Chunk::setBlockMetadata(int x, int y, int z, uint8_t meta) {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT || z < 0 || z >= DEPTH) return;
    uint8_t& currentMeta = m_metadata[getIndex(x, y, z)];
    if (currentMeta == meta) return;

    currentMeta = meta;

    const int sectionIndex = getSectionIndex(y);
    markSectionDirty(sectionIndex);
}

void Chunk::setBlockID(int x, int y, int z, uint8_t id) {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT || z < 0 || z >= DEPTH) return;
    uint8_t& block = m_blocks[getIndex(x, y, z)];
    if (block == id) return;

    block = id;

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
    {
        std::lock_guard<std::mutex> lock(m_blockMutex);
        uint8_t& block = m_blocks[getIndex(x, y, z)];
        if (block == id) return;
        block = id;
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

bool Chunk::isSectionDirty(int sectionIndex) const {
    return m_sectionDirty[sectionIndex];
}

void Chunk::clearSectionDirty(int sectionIndex) {
    m_sectionDirty[sectionIndex] = false;
}

uint32_t Chunk::getSectionVersion(int sectionIndex) const {
    return m_sectionVersions[sectionIndex];
}

void Chunk::touchSection(int sectionIndex) {
    markSectionDirty(sectionIndex);
}

void Chunk::markSectionDirty(int sectionIndex) {
    if (sectionIndex < 0 || sectionIndex >= SECTION_COUNT) {
        return;
    }

    m_sectionDirty[sectionIndex] = true;
    ++m_sectionVersions[sectionIndex];
}
