#pragma once

#include "renderer/ModelBiped.hpp"

class ModelZombie : public ModelBiped {
public:
    ModelZombie() : ModelBiped() {}

    void setRotationAngles(float limbSwing, float limbSwingAmount, float ageInTicks, float netHeadYaw, float headPitch, float scale) {
        ModelBiped::setRotationAngles(limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, headPitch, scale);
        float f = std::sin(attackTime * glm::pi<float>() / 10.0f); // dummy attack time
        float f1 = std::sin((1.0f - (1.0f - attackTime) * (1.0f - attackTime)) * glm::pi<float>() / 10.0f);
        
        bipedRightArm->rotateAngleZ = 0.0f;
        bipedLeftArm->rotateAngleZ = 0.0f;
        bipedRightArm->rotateAngleY = -(0.1f - f * 0.6f);
        bipedLeftArm->rotateAngleY = 0.1f - f * 0.6f;
        bipedRightArm->rotateAngleX = -glm::pi<float>() / 2.0f;
        bipedLeftArm->rotateAngleX = -glm::pi<float>() / 2.0f;
        bipedRightArm->rotateAngleX -= f * 1.2f - f1 * 0.4f;
        bipedLeftArm->rotateAngleX -= f * 1.2f - f1 * 0.4f;
        bipedRightArm->rotateAngleZ += std::cos(ageInTicks * 0.09f) * 0.05f + 0.05f;
        bipedLeftArm->rotateAngleZ -= std::cos(ageInTicks * 0.09f) * 0.05f + 0.05f;
        bipedRightArm->rotateAngleX += std::sin(ageInTicks * 0.067f) * 0.05f;
        bipedLeftArm->rotateAngleX -= std::sin(ageInTicks * 0.067f) * 0.05f;
    }
    
    float attackTime = 0.0f;
};
