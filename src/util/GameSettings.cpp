#include "util/GameSettings.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

GameSettings::GameSettings() {
    loadOptions();
}

void GameSettings::setDefaults() {
    mouseSensitivity = 0.5f;
    invertMouse = false;
    renderDistance = 1.0f;
    viewBobbing = true;
    anaglyph = false;
    limitFramerate = false;
    fancyGraphics = true;
    ambientOcclusion = true;
    guiScale = 0;
    fov = 70.0f;
    soundVolume = 1.0f;
    musicVolume = 1.0f;
    cloudLevel = 2;
    maxFps = 60;
    enableVsync = true;
}

std::string GameSettings::getOptionsFile() {
    return "options.txt";
}

void GameSettings::loadOptions() {
    std::ifstream file(getOptionsFile());
    if (!file.is_open()) return;

    std::string line;
    while (std::getline(file, line)) {
        size_t colon = line.find(':');
        if (colon == std::string::npos) continue;

        std::string key = line.substr(0, colon);
        std::string value = line.substr(colon + 1);

        if (key == "mouseSensitivity") mouseSensitivity = std::stof(value);
        if (key == "invertYMouse") invertMouse = (value == "true");
        if (key == "renderDistance") renderDistance = std::stof(value);
        if (key == "viewBobbing") viewBobbing = (value == "true");
        if (key == "anaglyph3d") anaglyph = (value == "true");
        if (key == "limitFramerate") limitFramerate = (value == "true");
        if (key == "fancyGraphics") fancyGraphics = (value == "true");
        if (key == "ambientOcclusion") ambientOcclusion = (value == "true");
        if (key == "guiScale") guiScale = std::stoi(value);
        if (key == "fov") fov = std::stof(value);
        if (key == "soundVolume") soundVolume = std::stof(value);
        if (key == "musicVolume") musicVolume = std::stof(value);
        if (key == "cloudLevel") cloudLevel = std::stoi(value);
        if (key == "maxFps") maxFps = std::stoi(value);
        if (key == "enableVsync") enableVsync = (value == "true");
    }
}

void GameSettings::saveOptions() {
    std::ofstream file(getOptionsFile());
    if (!file.is_open()) return;

    file << "mouseSensitivity:" << mouseSensitivity << "\n";
    file << "invertYMouse:" << (invertMouse ? "true" : "false") << "\n";
    file << "renderDistance:" << renderDistance << "\n";
    file << "viewBobbing:" << (viewBobbing ? "true" : "false") << "\n";
    file << "anaglyph3d:" << (anaglyph ? "true" : "false") << "\n";
    file << "limitFramerate:" << (limitFramerate ? "true" : "false") << "\n";
    file << "fancyGraphics:" << (fancyGraphics ? "true" : "false") << "\n";
    file << "ambientOcclusion:" << (ambientOcclusion ? "true" : "false") << "\n";
    file << "guiScale:" << guiScale << "\n";
    file << "fov:" << fov << "\n";
    file << "soundVolume:" << soundVolume << "\n";
    file << "musicVolume:" << musicVolume << "\n";
    file << "cloudLevel:" << cloudLevel << "\n";
    file << "maxFps:" << maxFps << "\n";
    file << "enableVsync:" << (enableVsync ? "true" : "false") << "\n";
}
