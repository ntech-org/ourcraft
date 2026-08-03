#pragma once

#include "renderer/ModelBase.hpp"
#include "renderer/ModelRenderer.hpp"
#include <memory>

class ModelCreeper : public ModelBase {
public:
    ModelCreeper();

    void render(Shader& shader, const glm::mat4& baseModel, float limbSwing, float limbSwingAmount, float ageInTicks, float netHeadYaw, float headPitch, float scale, float onGround) override;
    void setRotationAngles(float limbSwing, float limbSwingAmount, float ageInTicks, float netHeadYaw, float headPitch, float scale, float onGround);

    std::unique_ptr<ModelRenderer> head;
    std::unique_ptr<ModelRenderer> body;
    std::unique_ptr<ModelRenderer> leg1;
    std::unique_ptr<ModelRenderer> leg2;
    std::unique_ptr<ModelRenderer> leg3;
    std::unique_ptr<ModelRenderer> leg4;
};
