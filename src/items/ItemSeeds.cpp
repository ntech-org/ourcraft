#include "items/ItemSeeds.hpp"
#include "entities/EntityPlayer.hpp"
#include "entities/InventoryPlayer.hpp"
#include "world/Block.hpp"
#include "world/World.hpp"

ItemSeeds::ItemSeeds(int id, int blockType)
    : Item(id), m_blockType(blockType) {
}

bool ItemSeeds::onItemUse(ItemStack& stack, EntityPlayer& player, World& world, int x, int y, int z, int side) {
    (void)player;
    if (side != 1) return false;
    if (!Block::farmland) return false;
    int clickedID = world.getBlockID(x, y, z);
    if (clickedID != Block::farmland->blockID) return false;
    int aboveID = world.getBlockID(x, y + 1, z);
    if (aboveID != 0) return false;
    world.setBlockWithNotify(x, y + 1, z, (uint8_t)m_blockType);
    stack.count--;
    if (stack.count <= 0) {
        stack.itemID = 0;
        stack.count = 0;
        stack.metadata = 0;
        stack.damage = 0;
    }
    return true;
}
