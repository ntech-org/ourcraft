#pragma once

#include "items/Item.hpp"

class World;
class EntityPlayer;
struct ItemStack;

class ItemSeeds : public Item {
public:
    ItemSeeds(int id, int blockType);

    bool onItemUse(ItemStack& stack, EntityPlayer& player, World& world, int x, int y, int z, int side) override;

private:
    int m_blockType;
};
