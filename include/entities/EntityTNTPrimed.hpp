#pragma once

#include "entities/Entity.hpp"

class EntityTNTPrimed : public Entity {
public:
    EntityTNTPrimed(World& world);
    EntityTNTPrimed(World& world, float x, float y, float z);

    void onUpdate() override;

    int fuse = 80;
};
