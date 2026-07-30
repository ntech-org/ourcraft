#pragma once
#include <string>
#include <vector>

struct Account {
    std::string name;
    std::string uuid;
    std::string key;
};

struct ServerEntry {
    std::string name;
    std::string address;
    int port;
    std::string alias; // which account to use for this server
};

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
    bool limitFramerate = true;
    bool fancyGraphics = true;
    bool ambientOcclusion = true;
    int guiScale = 0; // 0 = Auto
    float fov = 70.0f;
    float soundVolume = 1.0f;
    float musicVolume = 1.0f;
    int cloudLevel = 2; // 0 = Off, 1 = Fast, 2 = Fancy
    int maxFps = 60;
    bool enableVsync = true;

    std::vector<Account> accounts;
    std::vector<ServerEntry> serverList;
    int activeAccountIndex = 0;

    void setDefaults();

private:
    std::string getOptionsFile();
    std::string getAccountsFile();
};
