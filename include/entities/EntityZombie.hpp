#pragma once

#include "entities/EntityLiving.hpp"

class EntityZombie : public EntityLiving {
public:
    EntityZombie(World& world);
    void updateEntityActionState() override;
};
