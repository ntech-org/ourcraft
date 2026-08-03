#pragma once

#include "renderer/ItemRenderer.hpp"
#include "world/World.hpp"
#include "entities/Entity.hpp"
#include "entities/EntityPlayer.hpp"
#include "renderer/Camera.hpp"
#include "renderer/Shader.hpp"
#include "renderer/RenderEngine.hpp"
#include "renderer/ModelBiped.hpp"
#include "renderer/ModelZombie.hpp"
#include "renderer/ModelPig.hpp"
#include "renderer/ModelSheep.hpp"
#include "renderer/ModelSheepFur.hpp"
#include "renderer/ModelSkeleton.hpp"
#include "renderer/ModelSpider.hpp"
#include "renderer/ModelCreeper.hpp"
#include <glm/glm.hpp>

class GameRenderer;

glm::mat4 computeViewBobMatrix(EntityPlayer& player, float partialTicks);

void renderEntity(Entity& entity, float pTicks, World& world, Camera& camera, Shader& entityShader,
                  RenderEngine& renderEngine, ModelBiped& playerModel, ModelZombie& zombieModel,
                  ModelPig& pigModel, ModelSheep& sheepModel, ModelSheepFur& sheepFurModel,
                  ModelSkeleton& skeletonModel, ModelSpider& spiderModel, ModelCreeper& creeperModel);

void renderFirstPersonArm(GameRenderer& renderer, EntityPlayer& player, World& world, Camera& camera,
                          float partialTicks, const glm::mat4& projection, float equippedProgress,
                          float prevEquippedProgress, int itemToRenderID);

void renderThirdPersonHeldItem(EntityPlayer* player, float partialTicks, const glm::dvec3& cameraPos,
                               Shader& entityShader, RenderEngine& renderEngine, ModelBiped& playerModel);
