#include "renderer/ModelQuadruped.hpp"
#include <glm/gtc/constants.hpp>
#include <cmath>

ModelQuadruped::ModelQuadruped(int legHeight, float yOffset) {
    head = std::make_unique<ModelRenderer>(0, 0);
    head->addBox(-4.0f, -4.0f, -8.0f, 8, 8, 8, 0.0f);
    head->setRotationPoint(0.0f, (float)(18 - legHeight) + yOffset, -6.0f);

    body = std::make_unique<ModelRenderer>(28, 8);
    body->addBox(-5.0f, -10.0f, -7.0f, 10, 16, 8, 0.0f);
    body->setRotationPoint(0.0f, (float)(17 - legHeight) + yOffset, 2.0f);

    leg1 = std::make_unique<ModelRenderer>(0, 16);
    leg1->addBox(-2.0f, 0.0f, -2.0f, 4, legHeight, 4, 0.0f);
    leg1->setRotationPoint(-3.0f, (float)(24 - legHeight) + yOffset, 7.0f);

    leg2 = std::make_unique<ModelRenderer>(0, 16);
    leg2->addBox(-2.0f, 0.0f, -2.0f, 4, legHeight, 4, 0.0f);
    leg2->setRotationPoint(3.0f, (float)(24 - legHeight) + yOffset, 7.0f);

    leg3 = std::make_unique<ModelRenderer>(0, 16);
    leg3->addBox(-2.0f, 0.0f, -2.0f, 4, legHeight, 4, 0.0f);
    leg3->setRotationPoint(-3.0f, (float)(24 - legHeight) + yOffset, -5.0f);

    leg4 = std::make_unique<ModelRenderer>(0, 16);
    leg4->addBox(-2.0f, 0.0f, -2.0f, 4, legHeight, 4, 0.0f);
    leg4->setRotationPoint(3.0f, (float)(24 - legHeight) + yOffset, -5.0f);
}

void ModelQuadruped::render(Shader& shader, const glm::mat4& baseModel, float limbSwing, float limbSwingAmount, float ageInTicks, float netHeadYaw, float headPitch, float scale, float onGround) {
    setRotationAngles(limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, headPitch, scale, onGround);
    head->render(shader, baseModel, scale);
    body->render(shader, baseModel, scale);
    leg1->render(shader, baseModel, scale);
    leg2->render(shader, baseModel, scale);
    leg3->render(shader, baseModel, scale);
    leg4->render(shader, baseModel, scale);
}

void ModelQuadruped::setRotationAngles(float limbSwing, float limbSwingAmount, float ageInTicks, float netHeadYaw, float headPitch, float scale, float onGround) {
    head->rotateAngleY = netHeadYaw / (180.0f / glm::pi<float>());
    body->rotateAngleX = glm::pi<float>() * 0.5f;
    leg1->rotateAngleX = std::cos(limbSwing * 0.6662f) * 1.4f * limbSwingAmount;
    leg2->rotateAngleX = std::cos(limbSwing * 0.6662f + glm::pi<float>()) * 1.4f * limbSwingAmount;
    leg3->rotateAngleX = std::cos(limbSwing * 0.6662f + glm::pi<float>()) * 1.4f * limbSwingAmount;
    leg4->rotateAngleX = std::cos(limbSwing * 0.6662f) * 1.4f * limbSwingAmount;
}
