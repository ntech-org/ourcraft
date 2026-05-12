#pragma once
#include "sound/SoundSystem.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdlib>

struct SoundPoolEntry {
    std::string path;
    const SoundBuffer* buffer = nullptr;
};

class SoundPool {
public:
    void addEntry(const std::string& name, const std::string& path);
    const SoundBuffer* getRandom(const std::string& name, SoundSystem& sys);
    std::vector<std::string> getPoolNames() const;

private:
    std::unordered_map<std::string, std::vector<SoundPoolEntry>> m_pools;
};

void buildSoundPool(SoundPool& pool, const std::string& directory);
