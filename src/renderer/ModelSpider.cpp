#include "renderer/ModelSpider.hpp"
#include <glm/gtc/constants.hpp>
#include <cmath>

ModelSpider::ModelSpider() {
    float var1 = 0.0f;
    int var2 = 15;

    spiderHead = std::make_unique<ModelRenderer>(32, 4);
    spiderHead->addBox(-4.0f, -4.0f, -8.0f, 8, 8, 8, var1);
    spiderHead->setRotationPoint(0.0f, (float)(0 + var2), -3.0f);

    spiderNeck = std::make_unique<ModelRenderer>(0, 0);
    spiderNeck->addBox(-3.0f, -3.0f, -3.0f, 6, 6, 6, var1);
    spiderNeck->setRotationPoint(0.0f, (float)var2, 0.0f);

    spiderBody = std::make_unique<ModelRenderer>(0, 12);
    spiderBody->addBox(-5.0f, -4.0f, -6.0f, 10, 8, 12, var1);
    spiderBody->setRotationPoint(0.0f, (float)(0 + var2), 9.0f);

    spiderLeg1 = std::make_unique<ModelRenderer>(18, 0);
    spiderLeg1->addBox(-15.0f, -1.0f, -1.0f, 16, 2, 2, var1);
    spiderLeg1->setRotationPoint(-4.0f, (float)(0 + var2), 2.0f);

    spiderLeg2 = std::make_unique<ModelRenderer>(18, 0);
    spiderLeg2->addBox(-1.0f, -1.0f, -1.0f, 16, 2, 2, var1);
    spiderLeg2->setRotationPoint(4.0f, (float)(0 + var2), 2.0f);

    spiderLeg3 = std::make_unique<ModelRenderer>(18, 0);
    spiderLeg3->addBox(-15.0f, -1.0f, -1.0f, 16, 2, 2, var1);
    spiderLeg3->setRotationPoint(-4.0f, (float)(0 + var2), 1.0f);

    spiderLeg4 = std::make_unique<ModelRenderer>(18, 0);
    spiderLeg4->addBox(-1.0f, -1.0f, -1.0f, 16, 2, 2, var1);
    spiderLeg4->setRotationPoint(4.0f, (float)(0 + var2), 1.0f);

    spiderLeg5 = std::make_unique<ModelRenderer>(18, 0);
    spiderLeg5->addBox(-15.0f, -1.0f, -1.0f, 16, 2, 2, var1);
    spiderLeg5->setRotationPoint(-4.0f, (float)(0 + var2), 0.0f);

    spiderLeg6 = std::make_unique<ModelRenderer>(18, 0);
    spiderLeg6->addBox(-1.0f, -1.0f, -1.0f, 16, 2, 2, var1);
    spiderLeg6->setRotationPoint(4.0f, (float)(0 + var2), 0.0f);

    spiderLeg7 = std::make_unique<ModelRenderer>(18, 0);
    spiderLeg7->addBox(-15.0f, -1.0f, -1.0f, 16, 2, 2, var1);
    spiderLeg7->setRotationPoint(-4.0f, (float)(0 + var2), -1.0f);

    spiderLeg8 = std::make_unique<ModelRenderer>(18, 0);
    spiderLeg8->addBox(-1.0f, -1.0f, -1.0f, 16, 2, 2, var1);
    spiderLeg8->setRotationPoint(4.0f, (float)(0 + var2), -1.0f);
}

void ModelSpider::render(Shader& shader, const glm::mat4& baseModel, float limbSwing, float limbSwingAmount, float ageInTicks, float netHeadYaw, float headPitch, float scale, float onGround) {
    setRotationAngles(limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, headPitch, scale, onGround);
    spiderHead->render(shader, baseModel, scale);
    spiderNeck->render(shader, baseModel, scale);
    spiderBody->render(shader, baseModel, scale);
    spiderLeg1->render(shader, baseModel, scale);
    spiderLeg2->render(shader, baseModel, scale);
    spiderLeg3->render(shader, baseModel, scale);
    spiderLeg4->render(shader, baseModel, scale);
    spiderLeg5->render(shader, baseModel, scale);
    spiderLeg6->render(shader, baseModel, scale);
    spiderLeg7->render(shader, baseModel, scale);
    spiderLeg8->render(shader, baseModel, scale);
}

void ModelSpider::setRotationAngles(float limbSwing, float limbSwingAmount, float ageInTicks, float netHeadYaw, float headPitch, float scale, float onGround) {
    spiderHead->rotateAngleY = netHeadYaw / (180.0f / glm::pi<float>());
    spiderHead->rotateAngleX = headPitch / (180.0f / glm::pi<float>());
    float var7 = glm::pi<float>() * 0.25f;
    spiderLeg1->rotateAngleZ = -var7;
    spiderLeg2->rotateAngleZ = var7;
    spiderLeg3->rotateAngleZ = -var7 * 0.74f;
    spiderLeg4->rotateAngleZ = var7 * 0.74f;
    spiderLeg5->rotateAngleZ = -var7 * 0.74f;
    spiderLeg6->rotateAngleZ = var7 * 0.74f;
    spiderLeg7->rotateAngleZ = -var7;
    spiderLeg8->rotateAngleZ = var7;
    float var8 = -0.0f;
    float var9 = glm::pi<float>() * 0.125f;
    spiderLeg1->rotateAngleY = var9 * 2.0f + var8;
    spiderLeg2->rotateAngleY = -var9 * 2.0f - var8;
    spiderLeg3->rotateAngleY = var9 * 1.0f + var8;
    spiderLeg4->rotateAngleY = -var9 * 1.0f - var8;
    spiderLeg5->rotateAngleY = -var9 * 1.0f + var8;
    spiderLeg6->rotateAngleY = var9 * 1.0f - var8;
    spiderLeg7->rotateAngleY = -var9 * 2.0f + var8;
    spiderLeg8->rotateAngleY = var9 * 2.0f - var8;
    float var10 = -(std::cos(limbSwing * 0.6662f * 2.0f + 0.0f) * 0.4f) * limbSwingAmount;
    float var11 = -(std::cos(limbSwing * 0.6662f * 2.0f + glm::pi<float>()) * 0.4f) * limbSwingAmount;
    float var12 = -(std::cos(limbSwing * 0.6662f * 2.0f + glm::pi<float>() * 0.5f) * 0.4f) * limbSwingAmount;
    float var13 = -(std::cos(limbSwing * 0.6662f * 2.0f + glm::pi<float>() * 3.0f / 2.0f) * 0.4f) * limbSwingAmount;
    float var14 = std::abs(std::sin(limbSwing * 0.6662f + 0.0f) * 0.4f) * limbSwingAmount;
    float var15 = std::abs(std::sin(limbSwing * 0.6662f + glm::pi<float>()) * 0.4f) * limbSwingAmount;
    float var16 = std::abs(std::sin(limbSwing * 0.6662f + glm::pi<float>() * 0.5f) * 0.4f) * limbSwingAmount;
    float var17 = std::abs(std::sin(limbSwing * 0.6662f + glm::pi<float>() * 3.0f / 2.0f) * 0.4f) * limbSwingAmount;
    spiderLeg1->rotateAngleY += var10;
    spiderLeg2->rotateAngleY += -var10;
    spiderLeg3->rotateAngleY += var11;
    spiderLeg4->rotateAngleY += -var11;
    spiderLeg5->rotateAngleY += var12;
    spiderLeg6->rotateAngleY += -var12;
    spiderLeg7->rotateAngleY += var13;
    spiderLeg8->rotateAngleY += -var13;
    spiderLeg1->rotateAngleZ += var14;
    spiderLeg2->rotateAngleZ += -var14;
    spiderLeg3->rotateAngleZ += var15;
    spiderLeg4->rotateAngleZ += -var15;
    spiderLeg5->rotateAngleZ += var16;
    spiderLeg6->rotateAngleZ += -var16;
    spiderLeg7->rotateAngleZ += var17;
    spiderLeg8->rotateAngleZ += -var17;
}
