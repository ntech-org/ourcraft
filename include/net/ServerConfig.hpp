#pragma once

#include <string>
#include <unordered_map>
#include <cstdint>

class ServerConfig {
public:
    ServerConfig(const std::string& configPath = "server.toml");

    void load();
    void save();
    void setDefaults();

    std::string getString(const std::string& key, const std::string& defaultVal = "") const;
    int getInt(const std::string& key, int defaultVal = 0) const;
    bool getBool(const std::string& key, bool defaultVal = false) const;
    double getDouble(const std::string& key, double defaultVal = 0.0) const;

    void setString(const std::string& key, const std::string& value);
    void setInt(const std::string& key, int value);
    void setBool(const std::string& key, bool value);

    std::string getDataDir() const { return m_dataDir; }

private:
    std::string m_configPath;
    std::string m_dataDir;
    std::unordered_map<std::string, std::string> m_values;
};
