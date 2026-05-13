#include "items/ItemSpade.hpp"
#include "world/Block.hpp"
#include "world/Material.hpp"

ItemSpade::ItemSpade(int id, int tier)
    : ItemTool(id, tier, 32 << tier, (tier + 1) * 2.0f) {
    if (tier == 3) maxDamage *= 2;
    if (tier == 4) {
        this->tier = 0;
        efficiencyOnProperMaterial = 2.0f;
        maxDamage = 32;
    }
}

float ItemSpade::getStrVsBlock(const Block& block) const {
    if (block.blockMaterial == Material::ground ||
        block.blockMaterial == Material::sand) {
        return efficiencyOnProperMaterial;
    }
    return 1.0f;
}

bool ItemSpade::canHarvestBlock(const Block& block) const {
    return block.blockMaterial == Material::ground ||
           block.blockMaterial == Material::sand;
}
