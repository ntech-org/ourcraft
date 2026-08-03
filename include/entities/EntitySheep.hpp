#pragma once

#include "entities/EntityLiving.hpp"

class EntitySheep : public EntityLiving {
public:
    EntitySheep(World& world);
    EntityType getType() const override { return EntityType::Sheep; }
    void updateEntityActionState() override;
    bool sheared = false;
    int getDropItemID() const override;
};
