#include "world/WorldGenTrees.hpp"
#include "world/World.hpp"
#include "world/Block.hpp"
#include "world/Material.hpp"
#include <cstdlib>

bool WorldGenTrees::generate(World& world, JavaRandom& random, int x, int y, int z) const {
    int trunkHeight = 4 + random.nextInt(3);
    if (y < 1 || y + trunkHeight + 1 > 128) return false;

    for (int by = y; by <= y + trunkHeight + 1; ++by) {
        int radius = 1;
        if (by == y) radius = 0;
        if (by >= y + trunkHeight - 1) radius = 2;

        for (int bx = x - radius; bx <= x + radius; ++bx) {
            for (int bz = z - radius; bz <= z + radius; ++bz) {
                if (by < 0 || by >= 128) return false;
                int id = world.getBlockID(bx, by, bz);
                if (id != 0 && id != Block::leaves->blockID) return false;
            }
        }
    }

    int belowID = world.getBlockID(x, y - 1, z);
    if ((belowID != Block::dirt->blockID && belowID != Block::grass->blockID) || y >= 128 - trunkHeight - 1) {
        return false;
    }

    world.setBlockWithNotify(x, y - 1, z, Block::dirt->blockID);

    for (int leafY = y - 3 + trunkHeight; leafY <= y + trunkHeight; ++leafY) {
        int dy = leafY - (y + trunkHeight);
        int layerRadius = 1 - dy / 2;

        for (int bx = x - layerRadius; bx <= x + layerRadius; ++bx) {
            int dx = bx - x;
            for (int bz = z - layerRadius; bz <= z + layerRadius; ++bz) {
                int dz = bz - z;
                bool onCorner = std::abs(dx) == layerRadius && std::abs(dz) == layerRadius;
                bool randomSkip = (random.nextInt(2) == 0) && dy != 0;
                if ((!onCorner || randomSkip) && world.getBlockID(bx, leafY, bz) == 0) {
                    world.setBlockWithNotify(bx, leafY, bz, Block::leaves->blockID);
                }
            }
        }
    }

    for (int ty = 0; ty < trunkHeight; ++ty) {
        int id = world.getBlockID(x, y + ty, z);
        if (id == 0 || id == Block::leaves->blockID) {
            world.setBlockWithNotify(x, y + ty, z, Block::wood->blockID);
        }
    }

    return true;
}
