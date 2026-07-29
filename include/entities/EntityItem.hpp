#pragma once

#include "entities/Entity.hpp"

class EntityItem : public Entity {
public:
    EntityItem(World& world, int itemID, int count, uint8_t metadata = 0);
    EntityItem(World& world);

    EntityType getType() const override { return EntityType::Item; }
    void onUpdate() override;
    void onCollideWithPlayer(class EntityPlayer& player);

    int itemID = 0;
    int count = 0;
    uint8_t metadata = 0;
    int age = 0;
    int delayBeforeCanPickup = 10;
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

    static constexpr double MERGE_RADIUS = 0.5;
    static constexpr int MAX_MERGE_COUNT = 64;

    bool tryMergeWithNearby();

private:
    void pushOutOfBlocks();
    int m_health = 5;
    int m_ageTicks = 0;
};
