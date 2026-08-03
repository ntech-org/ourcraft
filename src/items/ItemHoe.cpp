#include "items/ItemHoe.hpp"
#include "world/Block.hpp"
#include "world/Material.hpp"
#include "world/World.hpp"
#include "entities/EntityItem.hpp"
#include "entities/EntityPlayer.hpp"

ItemHoe::ItemHoe(int id, int tier)
    : ItemTool(id, tier, 32 << tier, 1.0f) {
    maxStackSize = 1;
    if (tier == 3) maxDamage *= 2;
    if (tier == 4) {
        this->tier = 0;
        maxDamage = 32;
    }
}

bool ItemHoe::onItemUse(ItemStack& stack, EntityPlayer& player, World& world, int x, int y, int z, int side) {
    int blockID = world.getBlockID(x, y, z);
    if ((world.getBlockMaterial(x, y + 1, z).isSolid() || blockID != Block::grass->blockID) &&
        blockID != Block::dirt->blockID) {
        return false;
    }

    world.setBlockWithNotify(x, y, z, Block::farmland->blockID);
    if (player.gameMode == GameMode::SURVIVAL && ++stack.damage >= maxDamage) stack = {};
    if (!world.isRemote && blockID == Block::grass->blockID && std::rand() % 8 == 0 && Item::seeds) {
        auto item = std::make_unique<EntityItem>(world, Item::seeds->itemID, 1, 0);
        item->setPosition(x + 0.5, y + 1.2, z + 0.5);
        item->delayBeforeCanPickup = 10;
        world.spawnEntity(std::move(item));
    }
    return true;
}

float ItemHoe::getStrVsBlock(const Block& block) const {
    return 1.0f;
}

bool ItemHoe::canHarvestBlock(const Block& block) const {
    return false;
}
