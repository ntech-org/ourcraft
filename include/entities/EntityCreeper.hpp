#pragma once

#include "entities/EntityLiving.hpp"

class EntityCreeper : public EntityLiving {
public:
    EntityCreeper(World& world);
    EntityType getType() const override { return EntityType::Creeper; }
    void updateEntityActionState() override;
    int timeSinceIgnited = 0;
    int fuseTime = 30;
    int creeperState = -1;
    int getDropItemID() const override;
};
