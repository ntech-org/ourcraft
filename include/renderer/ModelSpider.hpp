#pragma once

#include "renderer/ModelBase.hpp"
#include "renderer/ModelRenderer.hpp"
#include <memory>

class ModelSpider : public ModelBase {
public:
    ModelSpider();

    void render(Shader& shader, const glm::mat4& baseModel, float limbSwing, float limbSwingAmount, float ageInTicks, float netHeadYaw, float headPitch, float scale, float onGround) override;
    void setRotationAngles(float limbSwing, float limbSwingAmount, float ageInTicks, float netHeadYaw, float headPitch, float scale, float onGround);

    std::unique_ptr<ModelRenderer> spiderHead;
    std::unique_ptr<ModelRenderer> spiderNeck;
    std::unique_ptr<ModelRenderer> spiderBody;
    std::unique_ptr<ModelRenderer> spiderLeg1;
    std::unique_ptr<ModelRenderer> spiderLeg2;
    std::unique_ptr<ModelRenderer> spiderLeg3;
    std::unique_ptr<ModelRenderer> spiderLeg4;
    std::unique_ptr<ModelRenderer> spiderLeg5;
    std::unique_ptr<ModelRenderer> spiderLeg6;
    std::unique_ptr<ModelRenderer> spiderLeg7;
    std::unique_ptr<ModelRenderer> spiderLeg8;
};
