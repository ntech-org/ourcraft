#include "net/RegistrationManager.hpp"
#include <fstream>
#include <iostream>

void RegistrationManager::load() {
    m_users.clear();
    std::ifstream file(m_filePath);
    if (!file.is_open()) return;

    std::string line;
    while (std::getline(file, line)) {
        auto colon = line.find(':');
        if (colon == std::string::npos) continue;
        std::string username = line.substr(0, colon);
        std::string rest = line.substr(colon + 1);
        auto comma = rest.find(',');
        if (comma == std::string::npos) continue;
        UserRecord rec;
        rec.publicKey = rest.substr(0, comma);
        rec.uuid = rest.substr(comma + 1);
        m_users[username] = rec;
    }
}

void RegistrationManager::save() {
    std::ofstream file(m_filePath, std::ios::trunc);
    for (const auto& [name, rec] : m_users) {
        file << name << ":" << rec.publicKey << "," << rec.uuid << "\n";
    }
}

std::string RegistrationManager::generateKey() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    std::ostringstream oss;
    for (int i = 0; i < 64; ++i) {
        oss << std::hex << dis(gen);
    }
    return oss.str();
}

std::string RegistrationManager::generateUUIDFromHash(const std::string& hash) {
    std::hash<std::string> hasher;
    size_t h = hasher(hash);
    std::mt19937 gen(static_cast<uint32_t>(h));
    std::uniform_int_distribution<> dis(0, 15);
    std::ostringstream oss;
    oss << std::hex;
    for (int i = 0; i < 8; ++i) oss << dis(gen);
    oss << "-";
    for (int i = 0; i < 4; ++i) oss << dis(gen);
    oss << "-4";
    for (int i = 0; i < 3; ++i) oss << dis(gen);
    oss << "-";
    oss << std::hex << (8 + (gen() % 4));
    for (int i = 0; i < 3; ++i) oss << dis(gen);
    oss << "-";
    for (int i = 0; i < 12; ++i) oss << dis(gen);
    return oss.str();
}

std::string RegistrationManager::hashKey(const std::string& key) {
    std::hash<std::string> hasher;
    size_t h = hasher(key);
    std::ostringstream oss;
    oss << std::hex << h;
    return oss.str();
}

std::string RegistrationManager::registerUser(const std::string& username) {
    if (isRegistered(username)) return "";

    std::string key = generateKey();
    std::string hashedKey = hashKey(key);
    std::string uuid = generateUUIDFromHash(hashedKey);

    m_users[username] = {hashedKey, uuid};
    save();

    return key;
}

std::string RegistrationManager::reissueKey(const std::string& username) {
    auto it = m_users.find(username);
    if (it == m_users.end()) return "";

    std::string newKey = generateKey();
    it->second.publicKey = hashKey(newKey);
    save();

    return newKey;
}

bool RegistrationManager::verifyKey(const std::string& username, const std::string& key) {
    auto it = m_users.find(username);
    if (it == m_users.end()) return false;
    return it->second.publicKey == hashKey(key);
}

bool RegistrationManager::isRegistered(const std::string& username) const {
    return m_users.count(username) > 0;
}

const UserRecord* RegistrationManager::getUser(const std::string& username) const {
    auto it = m_users.find(username);
    return it != m_users.end() ? &it->second : nullptr;
}
