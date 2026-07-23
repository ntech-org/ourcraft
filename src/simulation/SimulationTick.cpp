#include "simulation/SimulationTick.hpp"
#include "world/World.hpp"
#include "entities/Entity.hpp"
#include "entities/EntityLiving.hpp"

void SimulationTick::voidProtection(World& world) {
    for (auto& entity : world.getEntities()) {
        if (entity->posY < -64.0) {
            entity->setPosition(entity->posX, 66.0, entity->posZ);
            entity->motionY = 0.0;
            entity->fallDistance = 0.0f;
            if (auto* living = dynamic_cast<EntityLiving*>(entity.get())) {
                if (living->health < living->maxHealth / 2) {
                    living->health = living->maxHealth;
                }
            }
        }
    }
}
