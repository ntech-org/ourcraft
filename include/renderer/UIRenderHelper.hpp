#pragma once

#include "renderer/GameRenderer.hpp"
#include "renderer/Tessellator.hpp"
#include "renderer/Shader.hpp"
#include "entities/EntityPlayer.hpp"
#include "world/Material.hpp"
#include "gui/Gui.hpp"
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

inline void renderHUD(GameRenderer& renderer, EntityPlayer& player, Shader& uiShader, Shader& textShader, RenderEngine& renderEngine, float scaledWidth, float scaledHeight) {
    glm::mat4 projection = glm::ortho(0.0f, scaledWidth, scaledHeight, 0.0f, -1.0f, 1.0f);
    glm::mat4 view = glm::mat4(1.0f);

    uiShader.use();
    uiShader.setMat4("projection", projection);
    uiShader.setMat4("view", view);

    textShader.use();
    textShader.setMat4("projection", projection);
    textShader.setMat4("view", view);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    renderEngine.bindTexture(renderEngine.getTexture("/gui/gui.png"));
    float centerX = scaledWidth / 2.0f;
    renderer.drawTexturedModalRect(centerX - 91.0f, scaledHeight - 22.0f, 0, 0, 182, 22);
    renderer.drawTexturedModalRect(centerX - 91.0f - 1.0f + (float)player.inventory.currentSlot * 20.0f, scaledHeight - 22.0f - 1.0f, 0, 22, 24, 22);

    for (int slot = 0; slot < InventoryPlayer::HOTBAR_SIZE; ++slot) {
        const ItemStack& stack = player.inventory.mainInventory[slot];
        if (stack.itemID <= 0 || stack.count <= 0) continue;

        const float iconX = centerX - 91.0f + (float)slot * 20.0f + 3.0f;
        const float iconY = scaledHeight - 19.0f;
        Gui::drawItemStack(&player.getMinecraft(), stack, iconX, iconY);
    }

    if (player.gameMode == GameMode::SURVIVAL) {
        uiShader.use();
        uiShader.setMat4("projection", projection);
        uiShader.setMat4("view", view);
        uiShader.setBool("hasTexture", true);

        renderEngine.bindTexture(renderEngine.getTexture("/gui/icons.png"));
        for (int i = 0; i < 10; ++i) {
            float x = centerX - 91.0f + (float)i * 8.0f;
            float y = scaledHeight - 32.0f;
            renderer.drawTexturedModalRect(x, y, 16, 0, 9, 9);
            if (i * 2 + 1 < player.health) renderer.drawTexturedModalRect(x, y, 52, 0, 9, 9);
            else if (i * 2 + 1 == player.health) renderer.drawTexturedModalRect(x, y, 61, 0, 9, 9);
        }

        if (player.isInsideOfMaterial(Material::water)) {
            int air = (int)std::ceil((double)(player.air - 2) * 10.0 / 300.0);
            int extraAir = (int)std::ceil((double)player.air * 10.0 / 300.0) - air;
            for (int i = 0; i < air + extraAir; ++i) {
                if (i < air) {
                    renderer.drawTexturedModalRect(centerX - 91.0f + (float)i * 8.0f, scaledHeight - 32.0f - 9.0f, 16, 18, 9, 9);
                } else {
                    renderer.drawTexturedModalRect(centerX - 91.0f + (float)i * 8.0f, scaledHeight - 32.0f - 9.0f, 25, 18, 9, 9);
                }
            }
        }
    }

    glDisable(GL_BLEND);
}

inline void renderCrosshair(GameRenderer& renderer, RenderEngine& renderEngine, float scaledWidth, float scaledHeight) {
    renderEngine.bindTexture(renderEngine.getTexture("/gui/icons.png"));
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_SRC_COLOR);
    float x = std::floor(scaledWidth / 2.0f) - 8.0f;
    float y = std::floor(scaledHeight / 2.0f) - 8.0f;
    renderer.drawTexturedModalRect(x, y, 0, 0, 16, 16);
    glDisable(GL_BLEND);
}

inline void renderUnderwaterOverlay(Shader& uiShader, RenderEngine& renderEngine, EntityPlayer& player, World& world, float scaledWidth, float scaledHeight) {
    renderEngine.bindTexture(renderEngine.getTexture("/water.png"));

    Tessellator* t = Tessellator::instance;
    float b = world.getDaylightStrength();
    uiShader.use();
    uiShader.setBool("hasTexture", true);
    t->startDrawingQuads();
    t->setColorRGBA((int)(b * 255), (int)(b * 255), (int)(b * 255), 128);

    float warp = 4.0f;
    float uOff = -player.rotationYaw / 64.0f;
    float vOff = player.rotationPitch / 64.0f;

    t->addVertexWithUV(0, scaledHeight, -90, uOff, vOff + warp);
    t->addVertexWithUV(scaledWidth, scaledHeight, -90, uOff + warp, vOff + warp);
    t->addVertexWithUV(scaledWidth, 0, -90, uOff + warp, vOff);
    t->addVertexWithUV(0, 0, -90, uOff, vOff);
    t->draw();
}
