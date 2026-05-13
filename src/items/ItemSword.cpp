#include "items/ItemSword.hpp"
#include "world/Block.hpp"
#include "world/Material.hpp"

ItemSword::ItemSword(int id, int tier)
    : ItemTool(id, tier, 32 << tier, (tier + 1) * 2.0f) {
    maxDamage = tier == 4 ? 32 : 32 << tier;
    if (tier == 3) maxDamage *= 2;
    efficiencyOnProperMaterial = 1.5f;
    if (tier == 4) {
        this->tier = 0;
        efficiencyOnProperMaterial = 1.5f;
        maxDamage = 32;
    }
}

float ItemSword::getStrVsBlock(const Block& block) const {
    return 1.5f;
}

bool ItemSword::canHarvestBlock(const Block& block) const {
    return false;
}
