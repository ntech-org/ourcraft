#pragma once

#include "entities/EntityLiving.hpp"

class EntitySkeleton : public EntityLiving {
public:
    EntitySkeleton(World& world);
    EntityType getType() const override { return EntityType::Skeleton; }
    void updateEntityActionState() override;
    int attackTime = 0;
    int getDropItemID() const override;
};
