#pragma once

#include "entities/Entity.hpp"

class EntityItem : public Entity {
public:
    EntityItem(World& world, int itemID, int count, uint8_t metadata = 0);

    EntityType getType() const override { return EntityType::Item; }
    void onUpdate() override;

    int itemID = 0;
    int count = 0;
    uint8_t metadata = 0;
    int age = 0;
    int pickupDelay = 10;
    float hoverStart = 0.0f;
    bool pickingUp = false;
    int pickupAnimationTicks = 0;
    int pickupAnimationTotalTicks = 10;
    double pickupStartX = 0.0;
    double pickupStartY = 0.0;
    double pickupStartZ = 0.0;
    double pickupTargetX = 0.0;
    double pickupTargetY = 0.0;
    double pickupTargetZ = 0.0;

    void startPickupAnimation(double targetX, double targetY, double targetZ);
};
