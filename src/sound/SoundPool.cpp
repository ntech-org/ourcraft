#include "sound/SoundPool.hpp"
#include <filesystem>
#include <iostream>
#include <regex>
#include <fstream>
#include <algorithm>

namespace fs = std::filesystem;

void SoundPool::addEntry(const std::string& name, const std::string& path) {
    m_pools[name].push_back({path, nullptr});
}

std::vector<std::string> SoundPool::getPoolNames() const {
    std::vector<std::string> names;
    names.reserve(m_pools.size());
    for (const auto& [name, _] : m_pools)
        names.push_back(name);
    return names;
}

const SoundBuffer* SoundPool::getRandom(const std::string& name, SoundSystem& sys) {
    auto it = m_pools.find(name);
    if (it == m_pools.end() || it->second.empty()) return nullptr;

    auto& entries = it->second;
    int idx = std::rand() % (int)entries.size();
    auto& entry = entries[idx];

    if (!entry.buffer) {
        entry.buffer = sys.loadOGG(entry.path);
    }
    return entry.buffer;
}

static std::string stripNumberSuffix(const std::string& stem) {
    std::string s = stem;
    while (!s.empty() && s.back() >= '0' && s.back() <= '9')
        s.pop_back();
    if (s.empty()) return stem;
    return s;
}

void buildSoundPool(SoundPool& pool, const std::string& directory) {
    if (!fs::exists(directory)) {
        std::cerr << "[Sound] Directory not found: " << directory << std::endl;
        return;
    }

    int count = 0;
    for (const auto& entry : fs::recursive_directory_iterator(directory)) {
        if (!entry.is_regular_file()) continue;
        auto path = entry.path();
        std::string ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (ext != ".ogg") continue;

        std::string fullPath = path.string();
        std::string relative = path.lexically_relative(directory).string();

        // Remove extension
        std::string name = relative.substr(0, relative.size() - 4);

        // Replace directory separators with '.'
        std::replace(name.begin(), name.end(), '/', '.');
        std::replace(name.begin(), name.end(), '\\', '.');

        // Strip trailing number suffix for grouping (e.g. "step.stone1" -> "step.stone")
        std::string poolName = stripNumberSuffix(name);

        pool.addEntry(poolName, fullPath);
        count++;
    }
    std::cout << "[Sound] Loaded " << count << " sounds from " << directory << std::endl;
}

 // glass uses stone sounds in Infdev
