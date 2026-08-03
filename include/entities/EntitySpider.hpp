#pragma once

#include "entities/EntityLiving.hpp"

class EntitySpider : public EntityLiving {
public:
    EntitySpider(World& world);
    EntityType getType() const override { return EntityType::Spider; }
    void updateEntityActionState() override;
    int getDropItemID() const override;
};
