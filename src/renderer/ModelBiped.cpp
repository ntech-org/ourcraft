#include "renderer/ModelBiped.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

ModelBiped::ModelBiped(float scale, float yOffset) {
    bipedHead = std::make_unique<ModelRenderer>(0, 0);
    bipedHead->addBox(-4.0f, -8.0f, -4.0f, 8, 8, 8, scale);
    bipedHead->setRotationPoint(0.0f, 0.0f + yOffset, 0.0f);

    bipedHeadwear = std::make_unique<ModelRenderer>(32, 0);
    bipedHeadwear->addBox(-4.0f, -8.0f, -4.0f, 8, 8, 8, scale + 0.5f);
    bipedHeadwear->setRotationPoint(0.0f, 0.0f + yOffset, 0.0f);

    bipedBody = std::make_unique<ModelRenderer>(16, 16);
    bipedBody->addBox(-4.0f, 0.0f, -2.0f, 8, 12, 4, scale);
    bipedBody->setRotationPoint(0.0f, 0.0f + yOffset, 0.0f);

    bipedRightArm = std::make_unique<ModelRenderer>(40, 16);
    bipedRightArm->addBox(-3.0f, -2.0f, -2.0f, 4, 12, 4, scale);
    bipedRightArm->setRotationPoint(-5.0f, 2.0f + yOffset, 0.0f);

    bipedLeftArm = std::make_unique<ModelRenderer>(40, 16);
    bipedLeftArm->mirror = true;
    bipedLeftArm->addBox(-1.0f, -2.0f, -2.0f, 4, 12, 4, scale);
    bipedLeftArm->setRotationPoint(5.0f, 2.0f + yOffset, 0.0f);

    bipedRightLeg = std::make_unique<ModelRenderer>(0, 16);
    bipedRightLeg->addBox(-2.0f, 0.0f, -2.0f, 4, 12, 4, scale);
    bipedRightLeg->setRotationPoint(-2.0f, 12.0f + yOffset, 0.0f);

    bipedLeftLeg = std::make_unique<ModelRenderer>(0, 16);
    bipedLeftLeg->mirror = true;
    bipedLeftLeg->addBox(-2.0f, 0.0f, -2.0f, 4, 12, 4, scale);
    bipedLeftLeg->setRotationPoint(2.0f, 12.0f + yOffset, 0.0f);
}

void ModelBiped::render(Shader& shader, const glm::mat4& baseModel, float limbSwing, float limbSwingAmount, float ageInTicks, float netHeadYaw, float headPitch, float scale) {
    setRotationAngles(limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, headPitch, scale);
    bipedHead->render(shader, baseModel, scale);
    bipedBody->render(shader, baseModel, scale);
    bipedRightArm->render(shader, baseModel, scale);
    bipedLeftArm->render(shader, baseModel, scale);
    bipedRightLeg->render(shader, baseModel, scale);
    bipedLeftLeg->render(shader, baseModel, scale);
    bipedHeadwear->render(shader, baseModel, scale);
}

void ModelBiped::setRotationAngles(float limbSwing, float limbSwingAmount, float ageInTicks, float netHeadYaw, float headPitch, float scale) {
    bipedHead->rotateAngleY = netHeadYaw / (180.0f / glm::pi<float>());
    bipedHead->rotateAngleX = headPitch / (180.0f / glm::pi<float>());
    bipedHeadwear->rotateAngleY = bipedHead->rotateAngleY;
    bipedHeadwear->rotateAngleX = bipedHead->rotateAngleX;

    bipedRightArm->rotateAngleX = std::cos(limbSwing * 0.6662f + glm::pi<float>()) * 1.4f * limbSwingAmount;
    bipedRightArm->rotateAngleZ = 0.0f;
    bipedLeftArm->rotateAngleX = std::cos(limbSwing * 0.6662f) * 1.4f * limbSwingAmount;
    bipedLeftArm->rotateAngleZ = 0.0f;

    bipedRightLeg->rotateAngleX = std::cos(limbSwing * 0.6662f) * 1.4f * limbSwingAmount;
    bipedLeftLeg->rotateAngleX = std::cos(limbSwing * 0.6662f + glm::pi<float>()) * 1.4f * limbSwingAmount;

    bipedRightArm->rotateAngleZ += std::cos(ageInTicks * 0.09f) * 0.05f + 0.05f;
    bipedLeftArm->rotateAngleZ -= std::cos(ageInTicks * 0.09f) * 0.05f + 0.05f;
    bipedRightArm->rotateAngleX += std::sin(ageInTicks * 0.067f) * 0.05f;
    bipedLeftArm->rotateAngleX -= std::sin(ageInTicks * 0.067f) * 0.05f;
}

void ModelBiped::renderFirstPersonArm(Shader& shader, const glm::mat4& baseModel, float scale) {
    bipedRightArm->rotateAngleX = 0.0f;
    bipedRightArm->rotateAngleY = 0.0f;
    bipedRightArm->rotateAngleZ = 0.0f;
    
    bipedRightArm->render(shader, baseModel, scale);
}
