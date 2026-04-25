#include "gui/GuiScreen.hpp"
#include "Minecraft.hpp"
#include "renderer/Tessellator.hpp"

void GuiScreen::drawScreen(int mouseX, int mouseY, float partialTicks) {
    Shader& shader = mc->getGameRenderer().getUIShader();
    for (auto& button : controlList) {
        button->drawButton(mc->getGameRenderer().getFontRenderer(), shader, mouseX, mouseY);
    }
}

void GuiScreen::keyTyped(int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        mc->displayGuiScreen(nullptr);
    }
}

void GuiScreen::mouseClicked(int mouseX, int mouseY, int button) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        for (size_t i = 0; i < controlList.size(); ++i) {
            if (controlList[i]->mousePressed(mouseX, mouseY)) {
                actionPerformed(controlList[i].get());
                if (mc->getCurrentScreen().get() != this) {
                    break;
                }
            }
        }
    }
}

void GuiScreen::setWorldAndResolution(Minecraft* mc, float width, float height) {
    this->mc = mc;
    this->width = width;
    this->height = height;
    controlList.clear();
    initGui();
}

void GuiScreen::drawDefaultBackground() {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    
    Shader& shader = mc->getGameRenderer().getUIShader();
    shader.use();
    shader.setMat4("projection", glm::ortho(0.0f, (float)width, (float)height, 0.0f, -1.0f, 1.0f));
    shader.setMat4("view", glm::mat4(1.0f));

    if (mc->getGameState() == GameState::InGame || mc->getGameState() == GameState::Paused) {
        drawGradientRect(shader, 0, 0, (float)width, (float)height, 0xC0101010, 0xD0101010);
    } else {
        mc->getGameRenderer().getRenderEngine().bindTexture(mc->getGameRenderer().getRenderEngine().getTexture("/dirt.png"));
        shader.setBool("hasTexture", true);
        Tessellator* t = Tessellator::instance;
        float s = 32.0f;
        t->startDrawingQuads();
        t->setColorOpaque(64, 64, 64);
        t->addVertexWithUV(0, (float)height, 0, 0, (float)height / s);
        t->addVertexWithUV((float)width, (float)height, 0, (float)width / s, (float)height / s);
        t->addVertexWithUV((float)width, 0, 0, (float)width / s, 0);
        t->addVertexWithUV(0, 0, 0, 0, 0);
        t->draw();
    }
}

bool GuiScreen::isCtrlKeyDown() {
    return false; // Implement later if needed using mc->window
}

bool GuiScreen::isShiftKeyDown() {
    return false; // Implement later if needed using mc->window
}
