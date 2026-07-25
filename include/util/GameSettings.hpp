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
    int renderDistanceChunks = 12; // 2-128 chunk radius
    bool viewBobbing = true;
    bool anaglyph = false;
    bool limitFramerate = false;
    bool fancyGraphics = true;
    bool ambientOcclusion = true;
    int guiScale = 0; // 0 = Auto
    float fov = 70.0f;
    float soundVolume = 1.0f;
    float musicVolume = 1.0f;
    int cloudLevel = 2; // 0 = Off, 1 = Fast, 2 = Fancy
    int maxFps = 60;
    bool enableVsync = true;

    std::string username = "Player";
    std::string uuid = "00000000-0000-0000-0000-000000000000";
    std::string playerKey = "";

    void setDefaults();

private:
    std::string getOptionsFile();
};
