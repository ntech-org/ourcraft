#include "gui/GuiLoading.hpp"
#include "Minecraft.hpp"
#include "world/World.hpp"

GuiLoading::GuiLoading() {}

void GuiLoading::updateScreen() {
    if (mc->getCurrentScreen().get() == this) {
        if (mc->getWorld()->getAllChunks().size() > 10) {
            mc->displayGuiScreen(nullptr);
        }
    }
}

void GuiLoading::drawScreen(int mouseX, int mouseY, float partialTicks) {
    drawDefaultBackground();
    drawCenteredString(mc->getFont(), mc->getGameRenderer().getTextShader(), "Loading terrain...", width / 2, height / 2, 0xFFFFFFFF);
    
    int chunkCount = mc->getWorld()->getAllChunks().size();
    drawCenteredString(mc->getFont(), mc->getGameRenderer().getTextShader(), std::to_string(chunkCount) + " chunks loaded", width / 2, height / 2 + 20, 0xFFA0A0A0);
    
    GuiScreen::drawScreen(mouseX, mouseY, partialTicks);
}
