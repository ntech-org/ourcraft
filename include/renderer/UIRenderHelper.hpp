#pragma once

#include "renderer/GameRenderer.hpp"
#include "entities/EntityPlayer.hpp"
#include "world/World.hpp"
#include "renderer/Shader.hpp"
#include "renderer/RenderEngine.hpp"
#include <glm/glm.hpp>

class GameRenderer;

void renderHUD(GameRenderer& renderer, EntityPlayer& player, Shader& uiShader, Shader& textShader, RenderEngine& renderEngine, float scaledWidth, float scaledHeight);
void renderCrosshair(GameRenderer& renderer, RenderEngine& renderEngine, float scaledWidth, float scaledHeight);
void renderUnderwaterOverlay(Shader& uiShader, RenderEngine& renderEngine, EntityPlayer& player, World& world, float scaledWidth, float scaledHeight);
