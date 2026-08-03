#include "renderer/ModelCreeper.hpp"
#include <glm/gtc/constants.hpp>
#include <cmath>

ModelCreeper::ModelCreeper() {
    float var1 = 0.0f;
    int var2 = 4;

    head = std::make_unique<ModelRenderer>(0, 0);
    head->addBox(-4.0f, -8.0f, -4.0f, 8, 8, 8, var1);
    head->setRotationPoint(0.0f, (float)var2, 0.0f);

    body = std::make_unique<ModelRenderer>(16, 16);
    body->addBox(-4.0f, 0.0f, -2.0f, 8, 12, 4, var1);
    body->setRotationPoint(0.0f, (float)var2, 0.0f);

    leg1 = std::make_unique<ModelRenderer>(0, 16);
    leg1->addBox(-2.0f, 0.0f, -2.0f, 4, 6, 4, var1);
    leg1->setRotationPoint(-2.0f, (float)(12 + var2), 4.0f);

    leg2 = std::make_unique<ModelRenderer>(0, 16);
    leg2->addBox(-2.0f, 0.0f, -2.0f, 4, 6, 4, var1);
    leg2->setRotationPoint(2.0f, (float)(12 + var2), 4.0f);

    leg3 = std::make_unique<ModelRenderer>(0, 16);
    leg3->addBox(-2.0f, 0.0f, -2.0f, 4, 6, 4, var1);
    leg3->setRotationPoint(-2.0f, (float)(12 + var2), -4.0f);

    leg4 = std::make_unique<ModelRenderer>(0, 16);
    leg4->addBox(-2.0f, 0.0f, -2.0f, 4, 6, 4, var1);
    leg4->setRotationPoint(2.0f, (float)(12 + var2), -4.0f);
}

void ModelCreeper::render(Shader& shader, const glm::mat4& baseModel, float limbSwing, float limbSwingAmount, float ageInTicks, float netHeadYaw, float headPitch, float scale, float onGround) {
    setRotationAngles(limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, headPitch, scale, onGround);
    head->render(shader, baseModel, scale);
    body->render(shader, baseModel, scale);
    leg1->render(shader, baseModel, scale);
    leg2->render(shader, baseModel, scale);
    leg3->render(shader, baseModel, scale);
    leg4->render(shader, baseModel, scale);
}

void ModelCreeper::setRotationAngles(float limbSwing, float limbSwingAmount, float ageInTicks, float netHeadYaw, float headPitch, float scale, float onGround) {
    head->rotateAngleY = netHeadYaw / (180.0f / glm::pi<float>());
    head->rotateAngleX = headPitch / (180.0f / glm::pi<float>());
    leg1->rotateAngleX = std::cos(limbSwing * 0.6662f) * 1.4f * limbSwingAmount;
    leg2->rotateAngleX = std::cos(limbSwing * 0.6662f + glm::pi<float>()) * 1.4f * limbSwingAmount;
    leg3->rotateAngleX = std::cos(limbSwing * 0.6662f + glm::pi<float>()) * 1.4f * limbSwingAmount;
    leg4->rotateAngleX = std::cos(limbSwing * 0.6662f) * 1.4f * limbSwingAmount;
}
