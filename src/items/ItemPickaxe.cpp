#include "items/ItemPickaxe.hpp"
#include "world/Block.hpp"
#include "world/Material.hpp"

ItemPickaxe::ItemPickaxe(int id, int tier)
    : ItemTool(id, tier, 32 << tier, (tier + 1) * 2.0f) {
    if (tier == 3) maxDamage *= 2;
    if (tier == 4) {
        this->tier = 0;
        efficiencyOnProperMaterial = 2.0f;
        maxDamage = 32;
    }
}

float ItemPickaxe::getStrVsBlock(const Block& block) const {
    if (block.blockMaterial == Material::rock || block.blockMaterial == Material::iron) {
        return efficiencyOnProperMaterial;
    }
    return 1.0f;
}

bool ItemPickaxe::canHarvestBlock(const Block& block) const {
    if (block.blockID == 49) return tier >= 3;
    if (block.blockID == 56 || block.blockID == 57) return tier >= 2;
    if (block.blockID == 14 || block.blockID == 41) return tier >= 2;
    if (block.blockID == 15 || block.blockID == 42) return tier >= 1;
    if (block.blockID == 43 || block.blockID == 44) return tier >= 1;
    if (block.blockMaterial == Material::rock) return true;
    if (block.blockMaterial == Material::iron) return true;
    return false;
}
