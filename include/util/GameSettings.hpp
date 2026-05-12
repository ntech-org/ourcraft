#pragma once
#include <string>
#include <vector>

class GameSettings {
public:
    GameSettings();
    
    void loadOptions();
    void saveOptions();

    float mouseSensitivity = 0.5f;
    bool invertMouse = false;
    float renderDistance = 1.0f; // 0.5 = Tiny, 1.0 = Normal, 2.0 = Far
    bool viewBobbing = true;
    bool anaglyph = false;
    bool limitFramerate = false;
    bool fancyGraphics = true;
    bool ambientOcclusion = true;
    int guiScale = 0; // 0 = Auto
    float fov = 70.0f;
    float soundVolume = 1.0f;
    float musicVolume = 1.0f;

private:
    std::string getOptionsFile();
};
