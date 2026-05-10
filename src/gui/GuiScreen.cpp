#include "gui/GuiScreen.hpp"
#include "Minecraft.hpp"
#include "renderer/Tessellator.hpp"

void GuiScreen::drawScreen(int mouseX, int mouseY, float partialTicks) {
    Shader& uiShader = mc->getGameRenderer().getUIShader();
    for (auto& button : controlList) {
        button->drawButton(mc, mc->getFont(), uiShader, mouseX, mouseY);
    }
}

void GuiScreen::drawString(Font& font, Shader& shader, const std::string& text, float x, float y, uint32_t color) {
    Gui::drawString(font, mc->getGameRenderer().getTextShader(), text, x, y, color);
}

void GuiScreen::drawCenteredString(Font& font, Shader& shader, const std::string& text, float x, float y, uint32_t color) {
    Gui::drawCenteredString(font, mc->getGameRenderer().getTextShader(), text, x, y, color);
}

void GuiScreen::handleEvent(const SDL_Event& event) {
    if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
        keyTyped(event.key.key, event.key.scancode, event.type == SDL_EVENT_KEY_DOWN);
    } else if (event.type == SDL_EVENT_TEXT_INPUT) {
        // Handle text input if needed (e.g. for text fields)
    } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        float mx = (float)event.button.x;
        float my = (float)event.button.y;
        
        int ww, wh, fw, fh;
        SDL_GetWindowSize(mc->getWindow(), &ww, &wh);
        SDL_GetWindowSizeInPixels(mc->getWindow(), &fw, &fh);

        mx *= (float)fw / (float)ww;
        my *= (float)fh / (float)wh;

        mx /= (float)mc->getGameRenderer().getGuiScale();
        my /= (float)mc->getGameRenderer().getGuiScale();

        mouseClicked((int)mx, (int)my, event.button.button);
    }
}

void GuiScreen::keyTyped(SDL_Keycode key, SDL_Scancode scancode, bool down) {
    if (key == SDLK_ESCAPE && down) {
        if (parentScreen) {
            mc->displayGuiScreen(parentScreen);
        } else if (mc->getGameState() != GameState::MainMenu) {
            mc->displayGuiScreen(nullptr);
        }
    }
}

void GuiScreen::mouseClicked(int mouseX, int mouseY, int button) {
    if (button == SDL_BUTTON_LEFT) {
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
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    int sw, sh;
    SDL_GetWindowSizeInPixels(mc->getWindow(), &sw, &sh);
    mc->getFont().setDisplayContext(sw, sh, (float)mc->getGameRenderer().getGuiScale());

    glm::mat4 projection = glm::ortho(0.0f, (float)width, (float)height, 0.0f, -1.0f, 1.0f);
    glm::mat4 view = glm::mat4(1.0f);

    Shader& uiShader = mc->getGameRenderer().getUIShader();
    uiShader.use();
    uiShader.setMat4("projection", projection);
    uiShader.setMat4("view", view);

    Shader& textShader = mc->getGameRenderer().getTextShader();
    textShader.use();
    textShader.setMat4("projection", projection);
    textShader.setMat4("view", view);

    if (mc->getGameState() == GameState::InGame || mc->getGameState() == GameState::Paused) {
        drawGradientRect(uiShader, 0, 0, (float)width, (float)height, 0xC0101010, 0xD0101010);
    } else {
        mc->getGameRenderer().getRenderEngine().bindTexture(mc->getGameRenderer().getRenderEngine().getTexture("/dirt.png"));
        uiShader.setBool("hasTexture", true);
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
    const bool* state = SDL_GetKeyboardState(nullptr);
    return state[SDL_SCANCODE_LCTRL] || state[SDL_SCANCODE_RCTRL];
}

bool GuiScreen::isShiftKeyDown() {
    const bool* state = SDL_GetKeyboardState(nullptr);
    return state[SDL_SCANCODE_LSHIFT] || state[SDL_SCANCODE_RSHIFT];
}
