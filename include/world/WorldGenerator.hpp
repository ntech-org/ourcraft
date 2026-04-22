#pragma once

#include <memory>

class Chunk;
class World;

class WorldGenerator {
public:
    virtual ~WorldGenerator() = default;
    virtual void generateChunk(Chunk& chunk) = 0;
};
