#pragma once

#include "entities/Entity.hpp"

class EntityLiving : public Entity {
public:
    EntityLiving(World& world);
    
    void onUpdate() override;
    
    float limbSwing;
    float prevLimbSwing;
    float limbSwingAmount;
    float prevLimbSwingAmount;
    float swingProgress;
    float prevSwingProgress;
    bool isSwinging;
    int swingProgressInt;
    
    float renderYawOffset = 0.0f;
    float prevRenderYawOffset = 0.0f;
    
    int deathTime = 0;
    int attackTime = 0;
    
    virtual void swing();
    
protected:
    virtual void updateEntityActionState();
};
