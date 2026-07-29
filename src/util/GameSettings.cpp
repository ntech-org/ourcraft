#include "util/GameSettings.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <filesystem>

GameSettings::GameSettings() {
    loadOptions();
}

void GameSettings::setDefaults() {
    mouseSensitivity = 0.5f;
    invertMouse = false;
    renderDistanceChunks = 12;
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
    accounts.clear();
    serverList.clear();
    activeAccountIndex = 0;
}

std::string GameSettings::getOptionsFile() {
    return "options.txt";
}

std::string GameSettings::getAccountsFile() {
    return "accounts.json";
}

void GameSettings::loadOptions() {
    // Load legacy options.txt for visual/audio settings
    std::ifstream file(getOptionsFile());
    if (!file.is_open()) {
        // Use defaults
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        size_t colon = line.find(':');
        if (colon == std::string::npos) continue;

        std::string key = line.substr(0, colon);
        std::string value = line.substr(colon + 1);

        if (key == "mouseSensitivity") mouseSensitivity = std::stof(value);
        if (key == "invertYMouse") invertMouse = (value == "true");
        if (key == "renderDistance") renderDistanceChunks = std::stoi(value);
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
        if (key == "playerKey") {
            // Migrate legacy key to first account
            if (accounts.empty()) {
                accounts.push_back({"Player", "00000000-0000-0000-0000-000000000000", value});
                activeAccountIndex = 0;
            }
        }
    }

    // Load accounts.json if it exists
    std::ifstream accountsFile(getAccountsFile());
    if (!accountsFile.is_open()) {
        // If no accounts.json, create default account
        if (accounts.empty()) {
            accounts.push_back({"Player", "00000000-0000-0000-0000-000000000000", ""});
            activeAccountIndex = 0;
        }
        return;
    }

    // Simple line-based format for accounts and server entries
    // Format: account:name|uuid|key
    // Format: server:name|address|port|alias
    while (std::getline(accountsFile, line)) {
        if (line.empty()) continue;
        size_t colon = line.find(':');
        if (colon == std::string::npos) continue;

        std::string type = line.substr(0, colon);
        std::string rest = line.substr(colon + 1);

        if (type == "account") {
            size_t pipe1 = rest.find('|');
            if (pipe1 == std::string::npos) continue;
            std::string name = rest.substr(0, pipe1);
            size_t pipe2 = rest.find('|', pipe1 + 1);
            if (pipe2 == std::string::npos) continue;
            std::string uuid = rest.substr(pipe1 + 1, pipe2 - pipe1 - 1);
            std::string key = rest.substr(pipe2 + 1);
            accounts.push_back({name, uuid, key});
        } else if (type == "server") {
            size_t pipe1 = rest.find('|');
            if (pipe1 == std::string::npos) continue;
            std::string name = rest.substr(0, pipe1);
            size_t pipe2 = rest.find('|', pipe1 + 1);
            if (pipe2 == std::string::npos) continue;
            std::string address = rest.substr(pipe1 + 1, pipe2 - pipe1 - 1);
            size_t pipe3 = rest.find('|', pipe2 + 1);
            if (pipe3 == std::string::npos) continue;
            std::string portStr = rest.substr(pipe2 + 1, pipe3 - pipe2 - 1);
            std::string alias = rest.substr(pipe3 + 1);
            int port = 25565;
            try {
                port = std::stoi(portStr);
            } catch (...) {}
            serverList.push_back({name, address, port, alias});
        }
    }

    // If no accounts loaded, create default
    if (accounts.empty()) {
        accounts.push_back({"Player", "00000000-0000-0000-0000-000000000000", ""});
        activeAccountIndex = 0;
    }
}

void GameSettings::saveOptions() {
    // Save visual/audio settings to options.txt
    std::ofstream file(getOptionsFile());
    if (!file.is_open()) return;

    file << "mouseSensitivity:" << mouseSensitivity << "\n";
    file << "invertYMouse:" << (invertMouse ? "true" : "false") << "\n";
    file << "renderDistance:" << renderDistanceChunks << "\n";
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

    // If there's an active account, save its key for backward compat
    if (!accounts.empty() && activeAccountIndex < accounts.size()) {
        file << "playerKey:" << accounts[activeAccountIndex].key << "\n";
    }

    // Save accounts and serverList to accounts.json
    std::ofstream accountsFile(getAccountsFile());
    if (!accountsFile.is_open()) return;

    for (const auto& acc : accounts) {
        accountsFile << "account:" << acc.name << "|" << acc.uuid << "|" << acc.key << "\n";
    }
    for (const auto& server : serverList) {
        accountsFile << "server:" << server.name << "|" << server.address << "|" << server.port << "|" << server.alias << "\n";
    }
}
