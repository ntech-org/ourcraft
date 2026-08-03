#pragma once

#include "entities/EntityLiving.hpp"

class EntityZombie : public EntityLiving {
public:
    EntityZombie(World& world);
    EntityType getType() const override { return EntityType::Zombie; }
    void updateEntityActionState() override;
    int getDropItemID() const override;
};
