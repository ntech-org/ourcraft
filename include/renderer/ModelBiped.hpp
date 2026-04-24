#pragma once

#include "renderer/ModelBase.hpp"
#include "renderer/ModelRenderer.hpp"
#include <memory>

class ModelBiped : public ModelBase {
public:
    ModelBiped(float scale = 0.0f, float yOffset = 0.0f);

    virtual void render(Shader& shader, const glm::mat4& baseModel, float limbSwing, float limbSwingAmount, float ageInTicks, float netHeadYaw, float headPitch, float scale, float onGround);
    virtual void setRotationAngles(float limbSwing, float limbSwingAmount, float ageInTicks, float netHeadYaw, float headPitch, float scale, float onGround);

    void renderFirstPersonArm(Shader& shader, const glm::mat4& baseView, float scale);

    std::unique_ptr<ModelRenderer> bipedHead;
    std::unique_ptr<ModelRenderer> bipedHeadwear;
    std::unique_ptr<ModelRenderer> bipedBody;
    std::unique_ptr<ModelRenderer> bipedRightArm;
    std::unique_ptr<ModelRenderer> bipedLeftArm;
    std::unique_ptr<ModelRenderer> bipedRightLeg;
    std::unique_ptr<ModelRenderer> bipedLeftLeg;
};
