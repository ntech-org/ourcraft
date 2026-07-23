#pragma once

#include <string>
#include <unordered_set>

class Permissions {
public:
    Permissions(const std::string& worldDir);

    void load();
    void save();

    bool isOp(const std::string& username) const;
    void addOp(const std::string& username);
    void removeOp(const std::string& username);
    const std::unordered_set<std::string>& getOps() const { return m_ops; }

private:
    std::string m_filePath;
    std::unordered_set<std::string> m_ops;
};
