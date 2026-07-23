#include "net/Permissions.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>

Permissions::Permissions(const std::string& worldDir)
    : m_filePath(worldDir + "/ops.txt") {}

void Permissions::load() {
    m_ops.clear();
    std::ifstream file(m_filePath);
    if (!file.is_open()) {
        std::cout << "[Permissions] No ops file found at " << m_filePath << std::endl;
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            auto pos = line.find_first_not_of(" \t\r\n");
            if (pos != std::string::npos) {
                line = line.substr(pos);
                auto end = line.find_last_not_of(" \t\r\n");
                if (end != std::string::npos) line = line.substr(0, end + 1);
                if (!line.empty()) m_ops.insert(line);
            }
        }
    }
    std::cout << "[Permissions] Loaded " << m_ops.size() << " ops" << std::endl;
}

void Permissions::save() {
    std::ofstream file(m_filePath, std::ios::trunc);
    if (!file.is_open()) {
        std::cerr << "[Permissions] Failed to save ops to " << m_filePath << std::endl;
        return;
    }
    for (const auto& name : m_ops) {
        file << name << "\n";
    }
}

bool Permissions::isOp(const std::string& username) const {
    return m_ops.count(username) > 0;
}

void Permissions::addOp(const std::string& username) {
    m_ops.insert(username);
    save();
}

void Permissions::removeOp(const std::string& username) {
    m_ops.erase(username);
    save();
}
