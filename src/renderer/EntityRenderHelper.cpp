#include "renderer/EntityRenderHelper.hpp"
#include "renderer/GameRenderer.hpp"
#include "renderer/ItemRenderer.hpp"
#include "renderer/Tessellator.hpp"
#include "renderer/Shader.hpp"
#include "renderer/RenderEngine.hpp"
#include "renderer/ModelBiped.hpp"
#include "renderer/ModelZombie.hpp"
#include "renderer/Camera.hpp"
#include "world/World.hpp"
#include "world/Block.hpp"
#include "entities/Entity.hpp"
#include "entities/EntityItem.hpp"
#include "entities/EntityLiving.hpp"
#include "entities/EntityPlayer.hpp"
#include "entities/EntitySheep.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <algorithm>
#include <cmath>
#include <SDL3/SDL.h>

namespace {

float computeLightBrightness(int sky, int block, float daylightStrength) {
    float skyVar2 = 1.0f - std::clamp((float)sky, 0.0f, 15.0f) / 15.0f;
    float skyBr = (1.0f - skyVar2) / (skyVar2 * 3.0f + 1.0f) * 0.9f + 0.1f;
    float blockVar2 = 1.0f - std::clamp((float)block, 0.0f, 15.0f) / 15.0f;
    float blockBr = (1.0f - blockVar2) / (blockVar2 * 3.0f + 1.0f) * 0.9f + 0.1f;
    return std::max(skyBr * daylightStrength, blockBr);
}

float getEntityBrightness(World& world, double ex, double ey, double ez) {
    auto light1 = world.getLightPair((int)std::floor(ex), (int)std::floor(ey + 0.1), (int)std::floor(ez));
    auto light2 = world.getLightPair((int)std::floor(ex), (int)std::floor(ey + 0.9), (int)std::floor(ez));
    return computeLightBrightness(std::max(light1.first, light2.first), std::max(light1.second, light2.second), world.getDaylightStrength());
}

float getFirstPersonBrightness(World& world, EntityPlayer& player) {
    auto light1 = world.getLightPair((int)std::floor(player.posX), (int)std::floor(player.posY + 0.5), (int)std::floor(player.posZ));
    auto light2 = world.getLightPair((int)std::floor(player.posX), (int)std::floor(player.posY + 1.2), (int)std::floor(player.posZ));
    return computeLightBrightness(std::max(light1.first, light2.first), std::max(light1.second, light2.second), world.getDaylightStrength());
}

static constexpr float kFaceShade[6] = {0.5f, 1.0f, 0.8f, 0.8f, 0.6f, 0.6f};

} // namespace

glm::mat4 computeViewBobMatrix(EntityPlayer& player, float partialTicks) {
    float bobDist = player.prevDistanceWalkedModified + (player.distanceWalkedModified - player.prevDistanceWalkedModified) * partialTicks;
    float bobStr = player.prevCameraYaw + (player.cameraYaw - player.prevCameraYaw) * partialTicks;
    float bobPitch = player.prevCameraPitch + (player.cameraPitch - player.prevCameraPitch) * partialTicks;

    glm::mat4 mat = glm::mat4(1.0f);
    mat = glm::translate(mat, glm::vec3(std::sin(bobDist * glm::pi<float>()) * bobStr * 0.5f, -std::abs(std::cos(bobDist * glm::pi<float>()) * bobStr), 0.0f));
    mat = glm::rotate(mat, glm::radians(std::sin(bobDist * glm::pi<float>()) * bobStr * 3.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    mat = glm::rotate(mat, glm::radians(std::abs(std::cos(bobDist * glm::pi<float>() + 0.2f) * bobStr) * 5.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    mat = glm::rotate(mat, glm::radians(bobPitch), glm::vec3(1.0f, 0.0f, 0.0f));
    return mat;
}

void renderEntity(Entity& entity, float pTicks, World& world, Camera& camera, Shader& entityShader,
                  RenderEngine& renderEngine, ModelBiped& playerModel, ModelZombie& zombieModel,
                  ModelPig& pigModel, ModelSheep& sheepModel, ModelSheepFur& sheepFurModel,
                  ModelSkeleton& skeletonModel, ModelSpider& spiderModel, ModelCreeper& creeperModel) {
    double ex = entity.prevPosX + (entity.posX - entity.prevPosX) * (double)pTicks;
    double ey = entity.prevPosY + (entity.posY - entity.prevPosY) * (double)pTicks;
    double ez = entity.prevPosZ + (entity.posZ - entity.prevPosZ) * (double)pTicks;

    float b = getEntityBrightness(world, ex, ey, ez);
    entityShader.setVec3("colorTint", glm::vec3(b));

    glm::vec3 relativePos = glm::vec3(glm::dvec3(ex, ey, ez) - camera.position);

    if (auto* item = dynamic_cast<EntityItem*>(&entity)) {
        const bool isBlockItem = isInventoryBlockModel(item->itemID);
        if (isBlockItem) {
            renderEngine.bindTexture(renderEngine.getTexture(TEX_TERRAIN));
        } else {
            renderEngine.bindTexture(renderEngine.getTexture(item->itemID < 256 ? TEX_TERRAIN : TEX_ITEMS));
        }

        const int tex = getItemIconTexture(item->itemID);
        const float spin = (((float)item->age + pTicks) / 20.0f + item->hoverStart) * 57.29578f;
        float bob = std::sin(((float)item->age + pTicks) / 10.0f + item->hoverStart) * 0.1f + 0.38f;
        glm::mat4 modelMat = glm::translate(glm::mat4(1.0f), glm::vec3(relativePos.x, relativePos.y + bob, relativePos.z));
        if (isBlockItem) {
            modelMat = glm::rotate(modelMat, glm::radians(spin), glm::vec3(0.0f, 1.0f, 0.0f));
        } else {
            modelMat = glm::rotate(modelMat, glm::radians(-camera.yaw + 90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        }
        float itemScale = isBlockItem ? 0.25f : 0.5f;
        if (item->pickupAnimationTicks > 0 && item->pickupAnimationTotalTicks > 0) {
            float pickupProgress = 1.0f - ((float)item->pickupAnimationTicks / (float)item->pickupAnimationTotalTicks);
            itemScale *= 1.0f - pickupProgress * 0.55f;
        }
        modelMat = glm::scale(modelMat, glm::vec3(itemScale, itemScale, itemScale));
        entityShader.setMat4("model", modelMat);

        Tessellator* t = Tessellator::instance;
        if (isBlockItem) {
            Block* block = Block::blocksList[item->itemID];
            t->startDrawingQuads();
            for (int side = 0; side < 6; ++side) {
                addFace(t, side, getTextureUV(block->getTexture(side)), kFaceShade[side]);
            }
            t->draw();
        } else {
            const FaceUV uv = getTextureUV(tex);
            t->startDrawingQuads();
            t->setColorOpaque(255, 255, 255);
            t->addVertexWithUV(-0.5f, 0.0f, 0.0f, uv.u0, uv.v1);
            t->addVertexWithUV(0.5f, 0.0f, 0.0f, uv.u1, uv.v1);
            t->addVertexWithUV(0.5f, 1.0f, 0.0f, uv.u1, uv.v0);
            t->addVertexWithUV(-0.5f, 1.0f, 0.0f, uv.u0, uv.v0);
            t->draw();
        }
        return;
    }

    const char* texturePath = "/mob/zombie.png";
    if (dynamic_cast<EntityPlayer*>(&entity)) {
        texturePath = TEX_CHAR;
    } else if (entity.getType() == EntityType::Pig) {
        texturePath = "/mob/pig.png";
    } else if (entity.getType() == EntityType::Sheep) {
        texturePath = "/mob/sheep.png";
    } else if (entity.getType() == EntityType::Skeleton) {
        texturePath = "/mob/skeleton.png";
    } else if (entity.getType() == EntityType::Spider) {
        texturePath = "/mob/spider.png";
    } else if (entity.getType() == EntityType::Creeper) {
        texturePath = "/mob/creeper.png";
    }
    renderEngine.bindTexture(renderEngine.getTexture(texturePath));

    float renderYaw = 0.0f, interpYaw = entity.rotationYaw, headPitch = entity.rotationPitch;
    if (auto living = dynamic_cast<EntityLiving*>(&entity)) {
        renderYaw = interpAngle(living->prevRenderYawOffset, living->renderYawOffset, pTicks);
        interpYaw = interpAngle(living->prevRotationYaw, living->rotationYaw, pTicks);
        headPitch = living->prevRotationPitch + (living->rotationPitch - living->prevRotationPitch) * pTicks;
    }

    float netHeadYaw = std::clamp(interpYaw - renderYaw, -75.0f, 75.0f);
    glm::mat4 modelMat = glm::translate(glm::mat4(1.0f), relativePos);
    modelMat = glm::rotate(modelMat, glm::radians(180.0f - renderYaw), glm::vec3(0.0f, 1.0f, 0.0f));
    modelMat = glm::scale(modelMat, glm::vec3(-1.0f, -1.0f, 1.0f));
    modelMat = glm::translate(modelMat, glm::vec3(0.0f, -1.5f, 0.0f));

    float limbSwing = 0.0f, limbSwingAmount = 0.0f, swing = 0.0f;
    if (auto living = dynamic_cast<EntityLiving*>(&entity)) {
        limbSwing = living->prevLimbSwing + (living->limbSwing - living->prevLimbSwing) * pTicks;
        limbSwingAmount = living->prevLimbSwingAmount + (living->limbSwingAmount - living->prevLimbSwingAmount) * pTicks;
        if (living->isSwinging) swing = ((float)living->swingProgressInt + pTicks) / 8.0f;
    }

    float ageInTicks = (float)((double)SDL_GetTicksNS() / 1e9);

    auto renderModel = [&]() {
        if (dynamic_cast<EntityPlayer*>(&entity)) {
            playerModel.render(entityShader, modelMat, limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, -headPitch, 0.0625f, swing);
        } else if (entity.getType() == EntityType::Zombie) {
            zombieModel.render(entityShader, modelMat, limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, -headPitch, 0.0625f, swing);
        } else if (entity.getType() == EntityType::Pig) {
            pigModel.render(entityShader, modelMat, limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, -headPitch, 0.0625f, swing);
        } else if (entity.getType() == EntityType::Sheep) {
            sheepModel.render(entityShader, modelMat, limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, -headPitch, 0.0625f, swing);
            auto* sheep = dynamic_cast<EntitySheep*>(&entity);
            if (sheep && !sheep->sheared) {
                renderEngine.bindTexture(renderEngine.getTexture("/mob/sheep_fur.png"));
                sheepFurModel.render(entityShader, modelMat, limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, -headPitch, 0.0625f, swing);
            }
        } else if (entity.getType() == EntityType::Skeleton) {
            skeletonModel.render(entityShader, modelMat, limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, -headPitch, 0.0625f, swing);
        } else if (entity.getType() == EntityType::Spider) {
            spiderModel.render(entityShader, modelMat, limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, -headPitch, 0.0625f, swing);
        } else if (entity.getType() == EntityType::Creeper) {
            creeperModel.render(entityShader, modelMat, limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, -headPitch, 0.0625f, swing);
        } else {
            zombieModel.render(entityShader, modelMat, limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, -headPitch, 0.0625f, swing);
        }
    };

    renderModel();

    // Hurt animation: red tint overlay
    if (auto* living = dynamic_cast<EntityLiving*>(&entity)) {
        if (living->hurtTime > 0) {
            glDepthFunc(GL_EQUAL);
            glDisable(GL_TEXTURE_2D);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glColor4f(b, 0.0f, 0.0f, 0.4f);
            renderModel();
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            glDisable(GL_BLEND);
            glEnable(GL_TEXTURE_2D);
            glDepthFunc(GL_LEQUAL);
        }
    }
}

void renderFirstPersonArm(GameRenderer& renderer, EntityPlayer& player, World& world, Camera& camera,
                          float partialTicks, const glm::mat4& projection, float equippedProgress,
                          float prevEquippedProgress, int itemToRenderID) {
    glClear(GL_DEPTH_BUFFER_BIT);
    Shader& entityShader = renderer.getEntityShader();
    RenderEngine& renderEngine = renderer.getRenderEngine();
    ModelBiped& playerModel = renderer.getPlayerModel();

    entityShader.use();
    entityShader.setMat4("projection", projection);
    entityShader.setMat4("view", glm::mat4(1.0f));
    entityShader.setVec3("cameraPos", glm::vec3(0.0f));
    entityShader.setFloat("daylightFactor", world.getDaylightStrength());
    glm::vec3 sunDir = world.getSunDirection();
    entityShader.setVec3("sunDirection", glm::mat3(camera.getViewMatrix()) * sunDir);

    entityShader.setVec3("colorTint", glm::vec3(getFirstPersonBrightness(world, player)));

    glm::mat4 baseBobMat = computeViewBobMatrix(player, partialTicks);

    float eqProgress = prevEquippedProgress + (equippedProgress - prevEquippedProgress) * partialTicks;
    float swingProgress = player.isSwinging ? ((float)player.swingProgressInt + partialTicks) / 8.0f : 0.0f;

    if (itemToRenderID <= 0) {
        renderEngine.bindTexture(renderEngine.getTexture(TEX_CHAR));
        glm::mat4 armMat = baseBobMat;
        float var5 = 0.8f;
        if (swingProgress > 0.0f) {
            float f1 = std::sin(swingProgress * glm::pi<float>());
            float f2 = std::sin(std::sqrt(swingProgress) * glm::pi<float>());
            armMat = glm::translate(armMat, glm::vec3(-f2 * 0.3f, std::sin(std::sqrt(swingProgress) * glm::pi<float>() * 2.0f) * 0.4f, -f1 * 0.4f));
        }

        armMat = glm::translate(armMat, glm::vec3(0.8f * var5, -0.75f * var5 - (1.0f - eqProgress) * 0.6f, -0.9f * var5));
        armMat = glm::rotate(armMat, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        if (swingProgress > 0.0f) {
            float f2 = std::sin(std::sqrt(swingProgress) * glm::pi<float>());
            float f3 = std::sin(swingProgress * swingProgress * glm::pi<float>());
            armMat = glm::rotate(armMat, glm::radians(f2 * 70.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            armMat = glm::rotate(armMat, glm::radians(-f3 * 20.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        }

        armMat = glm::translate(armMat, glm::vec3(-1.0f, 3.6f, 3.5f));
        armMat = glm::rotate(armMat, glm::radians(120.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        armMat = glm::rotate(armMat, glm::radians(200.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        armMat = glm::rotate(armMat, glm::radians(-135.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        armMat = glm::translate(armMat, glm::vec3(5.6f, 0.0f, 0.0f));

        playerModel.renderFirstPersonArm(entityShader, armMat, 0.0625f);
    } else {
        glDisable(GL_CULL_FACE);
        glm::mat4 heldMat = baseBobMat;
        float var5 = 0.8f;
        if (swingProgress > 0.0f) {
            float var7 = std::sin(swingProgress * glm::pi<float>());
            float var8 = std::sin(std::sqrt(swingProgress) * glm::pi<float>());
            heldMat = glm::translate(heldMat, glm::vec3(-var8 * 0.4f, std::sin(std::sqrt(swingProgress) * glm::pi<float>() * 2.0f) * 0.2f, -var7 * 0.2f));
        }

        heldMat = glm::translate(heldMat, glm::vec3(0.7f * var5, -0.65f * var5 - (1.0f - eqProgress) * 0.6f, -0.9f * var5));
        heldMat = glm::rotate(heldMat, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        if (swingProgress > 0.0f) {
            float var7 = std::sin(swingProgress * swingProgress * glm::pi<float>());
            float var8 = std::sin(std::sqrt(swingProgress) * glm::pi<float>());
            heldMat = glm::rotate(heldMat, glm::radians(-var7 * 20.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            heldMat = glm::rotate(heldMat, glm::radians(-var8 * 20.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            heldMat = glm::rotate(heldMat, glm::radians(-var8 * 80.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        }

        heldMat = glm::scale(heldMat, glm::vec3(0.4f, 0.4f, 0.4f));
        const bool isBlockItem = isInventoryBlockModel(itemToRenderID);
        if (isBlockItem) {
            renderEngine.bindTexture(renderEngine.getTexture(TEX_TERRAIN));
            entityShader.setMat4("model", heldMat);
            entityShader.setMat3("normalMatrix", glm::mat3(glm::inverseTranspose(heldMat)));

            Tessellator* t = Tessellator::instance;
            Block* block = Block::blocksList[itemToRenderID];
            t->startDrawingQuads();
            for (int side = 0; side < 6; ++side) {
                addFace(t, side, getTextureUV(block->getTexture(side)), kFaceShade[side]);
            }
            t->draw();
        } else {
            heldMat = glm::translate(heldMat, glm::vec3(0.0f, -0.3f, 0.08f));
            heldMat = glm::scale(heldMat, glm::vec3(1.5f, 1.5f, 1.5f));
            heldMat = glm::rotate(heldMat, glm::radians(50.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            heldMat = glm::rotate(heldMat, glm::radians(335.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            heldMat = glm::translate(heldMat, glm::vec3(-(15.0f / 16.0f), -(1.0f / 16.0f), 0.0f));
            entityShader.setMat4("model", heldMat);
            entityShader.setMat3("normalMatrix", glm::mat3(glm::inverseTranspose(heldMat)));

            Tessellator* t = Tessellator::instance;
            const int tex = getItemIconTexture(itemToRenderID);
            const FaceUV uv = getTextureUV(tex);
            renderEngine.bindTexture(renderEngine.getTexture(itemToRenderID < 256 ? TEX_TERRAIN : TEX_ITEMS));
            t->startDrawingQuads();
            t->setColorOpaque(255, 255, 255);
            renderFlatHeldItem(t, uv);
            t->draw();
        }
        glEnable(GL_CULL_FACE);
    }
}

void renderThirdPersonHeldItem(EntityPlayer* player, float partialTicks, const glm::dvec3& cameraPos,
                               Shader& entityShader, RenderEngine& renderEngine, ModelBiped& playerModel) {
    const ItemStack& stack = player->inventory.getCurrentStack();
    if (stack.isEmpty()) return;

    double ex = player->prevPosX + (player->posX - player->prevPosX) * (double)partialTicks;
    double ey = player->prevPosY + (player->posY - player->prevPosY) * (double)partialTicks;
    double ez = player->prevPosZ + (player->posZ - player->prevPosZ) * (double)partialTicks;
    glm::vec3 relativePos = glm::vec3(glm::dvec3(ex, ey, ez) - cameraPos);
    float renderYaw = interpAngle(player->prevRenderYawOffset, player->renderYawOffset, partialTicks);

    glm::mat4 heldMat = glm::translate(glm::mat4(1.0f), relativePos);
    heldMat = glm::rotate(heldMat, glm::radians(180.0f - renderYaw), glm::vec3(0.0f, 1.0f, 0.0f));
    heldMat = glm::scale(heldMat, glm::vec3(-1.0f, -1.0f, 1.0f));
    heldMat = glm::translate(heldMat, glm::vec3(0.0f, -1.5f, 0.0f));
    heldMat = glm::translate(heldMat, glm::vec3(-5.0f / 16.0f, 2.0f / 16.0f, 0.0f));

    heldMat = glm::rotate(heldMat, playerModel.bipedRightArm->rotateAngleZ, glm::vec3(0.0f, 0.0f, 1.0f));
    heldMat = glm::rotate(heldMat, playerModel.bipedRightArm->rotateAngleY, glm::vec3(0.0f, 1.0f, 0.0f));
    heldMat = glm::rotate(heldMat, playerModel.bipedRightArm->rotateAngleX, glm::vec3(1.0f, 0.0f, 0.0f));

    heldMat = glm::translate(heldMat, glm::vec3(0.0f, 0.45f, 0.0f));

    const bool isBlock = isInventoryBlockModel(stack.itemID);
    if (isBlock) {
        heldMat = glm::translate(heldMat, glm::vec3(0.0f, 0.1875f, -0.3125f));
        heldMat = glm::rotate(heldMat, glm::radians(20.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        heldMat = glm::rotate(heldMat, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        float s = 0.375f;
        heldMat = glm::scale(heldMat, glm::vec3(s, -s, s));

        renderEngine.bindTexture(renderEngine.getTexture(TEX_TERRAIN));
        entityShader.setMat4("model", heldMat);
        entityShader.setMat3("normalMatrix", glm::mat3(glm::inverseTranspose(heldMat)));

        Tessellator* t = Tessellator::instance;
        Block* block = Block::blocksList[stack.itemID];
        t->startDrawingQuads();
        for (int side = 0; side < 6; ++side) {
            int tex = block->getTexture(side);
            float u0 = (float)((tex & 15) << 4) / 256.0f;
            float v0 = (float)((tex & 240)) / 256.0f;
            FaceUV uv = {u0, v0, u0 + 16.0f / 256.0f, v0 + 16.0f / 256.0f};
            addFace(t, side, uv, kFaceShade[side]);
        }
        t->draw();
    } else {
        heldMat = glm::translate(heldMat, glm::vec3(0.0f, 0.1875f, 0.0f));
        float s = 0.4f;
        heldMat = glm::scale(heldMat, glm::vec3(s, s, s));
        heldMat = glm::rotate(heldMat, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        heldMat = glm::rotate(heldMat, glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));

        int tex = getItemIconTexture(stack.itemID);
        float u0 = (float)((tex & 15) << 4) / 256.0f;
        float v0 = (float)((tex & 240)) / 256.0f;
        FaceUV uv = {u0, v0, u0 + 16.0f / 256.0f, v0 + 16.0f / 256.0f};

        renderEngine.bindTexture(renderEngine.getTexture(stack.itemID < 256 ? TEX_TERRAIN : TEX_ITEMS));
        entityShader.setMat4("model", heldMat);
        entityShader.setMat3("normalMatrix", glm::mat3(glm::inverseTranspose(heldMat)));

        Tessellator* t = Tessellator::instance;
        t->startDrawingQuads();
        t->setColorOpaque(255, 255, 255);
        renderFlatHeldItem(t, uv);
        t->draw();
    }
}
