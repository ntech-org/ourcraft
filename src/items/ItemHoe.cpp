#include "items/ItemHoe.hpp"
#include "world/Block.hpp"
#include "world/Material.hpp"

ItemHoe::ItemHoe(int id, int tier)
    : ItemTool(id, tier, 32 << tier, 1.0f) {
    if (tier == 3) maxDamage *= 2;
    if (tier == 4) {
        this->tier = 0;
        maxDamage = 32;
    }
}

float ItemHoe::getStrVsBlock(const Block& block) const {
    return 1.0f;
}

bool ItemHoe::canHarvestBlock(const Block& block) const {
    return false;
}
