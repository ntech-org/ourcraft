#pragma once

#include <array>
#include <cstdint>
#include <vector>
#include <mutex>
#include <atomic>

enum class ChunkState {
    Empty,
    Generating,
    Generated,
    Decorating,
    Decorated
};

class Chunk {
public:
    static constexpr int WIDTH = 16;
    static constexpr int HEIGHT = 128;
    static constexpr int DEPTH = 16;
    static constexpr int SECTION_HEIGHT = 16;
    static constexpr int SECTION_COUNT = HEIGHT / SECTION_HEIGHT;
    static constexpr int SIZE = WIDTH * HEIGHT * DEPTH;

    Chunk(int x, int z);
    ~Chunk() = default;

    uint8_t getBlockID(int x, int y, int z) const;
    void setBlockID(int x, int y, int z, uint8_t id);
    void setBlockIDSafe(int x, int y, int z, uint8_t id);

    int getX() const { return m_x; }
    int getZ() const { return m_z; }

    const uint8_t* getBlocks() const { return m_blocks.data(); }
    bool isSectionDirty(int sectionIndex) const;
    void clearSectionDirty(int sectionIndex);
    uint32_t getSectionVersion(int sectionIndex) const;
    void touchSection(int sectionIndex);

    ChunkState getState() const { return m_state; }
    void setState(ChunkState state) { m_state = state; }

    std::mutex& getBlockMutex() { return m_blockMutex; }

    static constexpr int getSectionIndex(int y) {
        return y / SECTION_HEIGHT;
    }

    static constexpr int getSectionMinY(int sectionIndex) {
        return sectionIndex * SECTION_HEIGHT;
    }

private:
    int m_x, m_z;
    std::vector<uint8_t> m_blocks;
    std::array<bool, SECTION_COUNT> m_sectionDirty {};
    std::array<uint32_t, SECTION_COUNT> m_sectionVersions {};
    
    std::mutex m_blockMutex;
    std::atomic<ChunkState> m_state { ChunkState::Empty };

    static inline int getIndex(int x, int y, int z) {
        return (x << 11) | (z << 7) | y;
    }

    void markSectionDirty(int sectionIndex);
};
