#pragma once

#include "entities/EntityLiving.hpp"

class EntityPig : public EntityLiving {
public:
    EntityPig(World& world);
    EntityType getType() const override { return EntityType::Pig; }
    void updateEntityActionState() override;
    int getDropItemID() const override;
};
