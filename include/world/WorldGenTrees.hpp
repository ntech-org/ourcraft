#pragma once

#include "world/Block.hpp"
#include "world/JavaRandom.hpp"

class World;

class WorldGenTrees {
public:
    bool generate(World& world, JavaRandom& random, int x, int y, int z) const;
};
