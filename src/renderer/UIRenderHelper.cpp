#include "renderer/UIRenderHelper.hpp"
#include "renderer/GameRenderer.hpp"
#include "renderer/Tessellator.hpp"
#include "renderer/Shader.hpp"
#include "renderer/RenderEngine.hpp"
#include "entities/EntityPlayer.hpp"
#include "world/World.hpp"
#include "world/Material.hpp"
#include "items/Item.hpp"
#include "items/ItemTool.hpp"
#include "gui/Gui.hpp"
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

void renderHUD(GameRenderer& renderer, EntityPlayer& player, Shader& uiShader, Shader& textShader, RenderEngine& renderEngine, float scaledWidth, float scaledHeight) {
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
    renderEngine.bindTexture(renderEngine.getTexture(TEX_GUI));
    float centerX = scaledWidth / 2.0f;
    renderer.drawTexturedModalRect(centerX - 91.0f, scaledHeight - 22.0f, 0, 0, 182, 22);
    renderer.drawTexturedModalRect(centerX - 91.0f - 1.0f + (float)player.inventory.currentSlot * 20.0f, scaledHeight - 22.0f - 1.0f, 0, 22, 24, 22);

    for (int slot = 0; slot < InventoryPlayer::HOTBAR_SIZE; ++slot) {
        const ItemStack& stack = player.inventory.mainInventory[slot];
        if (stack.itemID <= 0 || stack.count <= 0) continue;

        const float iconX = centerX - 91.0f + (float)slot * 20.0f + 3.0f;
        const float iconY = scaledHeight - 19.0f;
        Gui::drawItemStack(&player.getMinecraft(), stack, iconX, iconY);

        if (stack.damage > 0 && stack.itemID >= 256) {
            Item* item = Item::itemsList[stack.itemID];
            if (auto* tool = dynamic_cast<ItemTool*>(item)) {
                float ratio = 1.0f - (float)stack.damage / (float)tool->maxDamage;
                float barWidth = 13.0f;
                float filled = barWidth * ratio;
                uint32_t barColor;
                if (ratio > 0.5f) barColor = 0xFF00FF00;
                else if (ratio > 0.25f) barColor = 0xFFFFFF00;
                else barColor = 0xFFFF0000;
                Gui::drawRect(uiShader, iconX, iconY + 16.0f, iconX + barWidth, iconY + 17.0f, 0xFF000000);
                Gui::drawRect(uiShader, iconX, iconY + 16.0f, iconX + filled, iconY + 17.0f, barColor);
            }
        }
    }

    if (player.gameMode == GameMode::SURVIVAL) {
        uiShader.use();
        uiShader.setMat4("projection", projection);
        uiShader.setMat4("view", view);
        uiShader.setBool("hasTexture", true);

        renderEngine.bindTexture(renderEngine.getTexture(TEX_ICONS));
        bool hurtFlash = player.hurtTime > 0 && ((player.hurtTime / 2) % 2 == 0);
        for (int i = 0; i < 10; ++i) {
            float x = centerX - 91.0f + (float)i * 8.0f;
            float y = scaledHeight - 32.0f;
            renderer.drawTexturedModalRect(x, y, 16, 0, 9, 9);
            if (hurtFlash && i * 2 + 1 <= player.health) {
                renderer.drawTexturedModalRect(x, y, 16, 0, 9, 9);
            } else if (i * 2 + 1 < player.health) {
                renderer.drawTexturedModalRect(x, y, 52, 0, 9, 9);
            } else if (i * 2 + 1 == player.health) {
                renderer.drawTexturedModalRect(x, y, 61, 0, 9, 9);
            }
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

void renderCrosshair(GameRenderer& renderer, RenderEngine& renderEngine, float scaledWidth, float scaledHeight) {
    renderEngine.bindTexture(renderEngine.getTexture(TEX_ICONS));
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_SRC_COLOR);
    float x = std::floor(scaledWidth / 2.0f) - 8.0f;
    float y = std::floor(scaledHeight / 2.0f) - 8.0f;
    renderer.drawTexturedModalRect(x, y, 0, 0, 16, 16);
    glDisable(GL_BLEND);
}

void renderUnderwaterOverlay(Shader& uiShader, RenderEngine& renderEngine, EntityPlayer& player, World& world, float scaledWidth, float scaledHeight) {
    renderEngine.bindTexture(renderEngine.getTexture(TEX_WATER));

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
