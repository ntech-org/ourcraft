#pragma once
#include "gui/GuiScreen.hpp"
#include "gui/GuiTextField.hpp"

class GuiMultiplayer : public GuiScreen {
public:
    GuiMultiplayer(std::shared_ptr<GuiScreen> parent);
    virtual ~GuiMultiplayer() = default;

    void initGui() override;
    void updateScreen() override;
    void drawScreen(int mouseX, int mouseY, float partialTicks) override;
    void actionPerformed(GuiButton* button) override;
    void keyTyped(int key, int scancode, int action, int mods) override;
    void mouseClicked(int mouseX, int mouseY, int button) override;

private:
    std::shared_ptr<GuiScreen> m_parent;
    std::unique_ptr<GuiTextField> m_serverAddressField;
};
