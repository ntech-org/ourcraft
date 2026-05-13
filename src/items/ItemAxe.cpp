#include "items/ItemAxe.hpp"
#include "world/Block.hpp"
#include "world/Material.hpp"

ItemAxe::ItemAxe(int id, int tier)
    : ItemTool(id, tier, 32 << tier, (tier + 1) * 2.0f) {
    if (tier == 3) maxDamage *= 2;
    if (tier == 4) {
        this->tier = 0;
        efficiencyOnProperMaterial = 2.0f;
        maxDamage = 32;
    }
}

float ItemAxe::getStrVsBlock(const Block& block) const {
    if (block.blockMaterial == Material::wood) {
        return efficiencyOnProperMaterial;
    }
    return 1.0f;
}

bool ItemAxe::canHarvestBlock(const Block& block) const {
    return block.blockMaterial == Material::wood;
}
