#include "world/Chunk.hpp"

Chunk::Chunk(int x, int z) : m_x(x), m_z(z) {
    m_blocks.resize(SIZE, 0);
}

uint8_t Chunk::getBlockID(int x, int y, int z) const {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT || z < 0 || z >= DEPTH) return 0;
    return m_blocks[getIndex(x, y, z)];
}

void Chunk::setBlockID(int x, int y, int z, uint8_t id) {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT || z < 0 || z >= DEPTH) return;
    m_blocks[getIndex(x, y, z)] = id;
}
