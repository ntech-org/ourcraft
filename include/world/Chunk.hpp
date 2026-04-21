#pragma once

#include <cstdint>
#include <vector>

class Chunk {
public:
    static constexpr int WIDTH = 16;
    static constexpr int HEIGHT = 128;
    static constexpr int DEPTH = 16;
    static constexpr int SIZE = WIDTH * HEIGHT * DEPTH;

    Chunk(int x, int z);
    ~Chunk() = default;

    uint8_t getBlockID(int x, int y, int z) const;
    void setBlockID(int x, int y, int z, uint8_t id);

    int getX() const { return m_x; }
    int getZ() const { return m_z; }

    const uint8_t* getBlocks() const { return m_blocks.data(); }

private:
    int m_x, m_z;
    std::vector<uint8_t> m_blocks;

    static inline int getIndex(int x, int y, int z) {
        return (x << 11) | (z << 7) | y;
    }
};
