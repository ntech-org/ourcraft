#pragma once

#include "entities/Entity.hpp"

class EntityLiving : public Entity {
public:
    EntityLiving(World& world);
    
    void onUpdate() override;
    
    float limbSwing;
    float limbSwingAmount;
    float prevLimbSwingAmount;
    int deathTime = 0;
    int attackTime = 0;
    
protected:
    virtual void updateEntityActionState();
};
