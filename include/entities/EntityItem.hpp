#pragma once

#include "entities/Entity.hpp"

class EntityItem : public Entity {
public:
    EntityItem(World& world, int itemID, int count, uint8_t metadata = 0);

    void onUpdate() override;

    int itemID = 0;
    int count = 0;
    uint8_t metadata = 0;
    int age = 0;
    int pickupDelay = 10;
    float hoverStart = 0.0f;
};
