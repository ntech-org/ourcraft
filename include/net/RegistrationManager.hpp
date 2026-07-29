#pragma once
#include <string>
#include <unordered_map>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <iomanip>

struct UserRecord {
    std::string publicKey;
    std::string uuid;
};

class RegistrationManager {
public:
    RegistrationManager(const std::string& worldDir) : m_filePath(worldDir + "/players.json") {}

    void load();
    void save();

    std::string registerUser(const std::string& username);
    std::string reissueKey(const std::string& username);
    bool verifyKey(const std::string& username, const std::string& key);
    bool isRegistered(const std::string& username) const;
    const UserRecord* getUser(const std::string& username) const;

private:
    std::string generateKey();
    std::string generateUUIDFromHash(const std::string& hash);
    std::string hashKey(const std::string& key);

    std::string m_filePath;
    std::unordered_map<std::string, UserRecord> m_users;
};
