#include "net/ServerConfig.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

ServerConfig::ServerConfig(const std::string& configPath)
    : m_configPath(configPath) {
    m_dataDir = "server_data";
}

static std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

void ServerConfig::load() {
    m_values.clear();
    setDefaults();

    std::ifstream file(m_configPath);
    if (!file.is_open()) {
        std::cout << "[Config] No config file found at " << m_configPath << ", using defaults." << std::endl;
        save();
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#' || line[0] == '[') continue;

        auto eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key = trim(line.substr(0, eq));
        std::string value = trim(line.substr(eq + 1));

        if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
            value = value.substr(1, value.size() - 2);
        } else if (value.size() >= 2 && value.front() == '\'' && value.back() == '\'') {
            value = value.substr(1, value.size() - 2);
        }

        m_values[key] = value;
    }

    std::cout << "[Config] Loaded " << m_values.size() << " settings from " << m_configPath << std::endl;
}

void ServerConfig::save() {
    std::ofstream file(m_configPath, std::ios::trunc);
    if (!file.is_open()) {
        std::cerr << "[Config] Failed to save config to " << m_configPath << std::endl;
        return;
    }

    file << "# OurCraft Server Configuration\n";
    file << "# Edit values below. Strings should be quoted.\n\n";

    file << "# Network settings\n";
    file << "port = " << getInt("port", 25565) << "\n";
    file << "max-players = " << getInt("max-players", 32) << "\n\n";

    file << "# Server info\n";
    file << "motd = \"" << getString("motd", "OurCraft Server") << "\"\n";
    file << "online-mode = " << (getBool("online-mode", true) ? "true" : "false") << "\n\n";

    file << "# World settings\n";
    file << "spawn-protection = " << getInt("spawn-protection", 16) << "\n";
    file << "view-distance = " << getInt("view-distance", 12) << "\n";

    file.flush();
}

void ServerConfig::setDefaults() {
    m_values["port"] = "25565";
    m_values["max-players"] = "32";
    m_values["motd"] = "OurCraft Server";
    m_values["online-mode"] = "true";
    m_values["spawn-protection"] = "16";
    m_values["view-distance"] = "12";
}

std::string ServerConfig::getString(const std::string& key, const std::string& defaultVal) const {
    auto it = m_values.find(key);
    return it != m_values.end() ? it->second : defaultVal;
}

int ServerConfig::getInt(const std::string& key, int defaultVal) const {
    auto it = m_values.find(key);
    if (it == m_values.end()) return defaultVal;
    try { return std::stoi(it->second); } catch (...) { return defaultVal; }
}

bool ServerConfig::getBool(const std::string& key, bool defaultVal) const {
    auto it = m_values.find(key);
    if (it == m_values.end()) return defaultVal;
    std::string v = it->second;
    std::transform(v.begin(), v.end(), v.begin(), ::tolower);
    return v == "true" || v == "1" || v == "yes";
}

double ServerConfig::getDouble(const std::string& key, double defaultVal) const {
    auto it = m_values.find(key);
    if (it == m_values.end()) return defaultVal;
    try { return std::stod(it->second); } catch (...) { return defaultVal; }
}

void ServerConfig::setString(const std::string& key, const std::string& value) {
    m_values[key] = value;
}

void ServerConfig::setInt(const std::string& key, int value) {
    m_values[key] = std::to_string(value);
}

void ServerConfig::setBool(const std::string& key, bool value) {
    m_values[key] = value ? "true" : "false";
}
