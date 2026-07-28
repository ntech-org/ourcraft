#pragma once

#include <memory>

class Chunk;
class World;

class WorldGenerator {
public:
    virtual ~WorldGenerator() = default;
    virtual void generateChunk(Chunk& chunk) = 0;
    virtual void decorateChunk(Chunk& chunk, Chunk* chunkE, Chunk* chunkS, Chunk* chunkSE) {}
    virtual void setFarLands(bool enabled) {}
};
