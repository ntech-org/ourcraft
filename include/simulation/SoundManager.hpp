#pragma once

#include <string>
#include <vector>

class SoundSystem;
class SoundPool;
class EntityPlayer;

class SoundManager {
public:
    SoundManager() = default;

    void tick(SoundSystem& sound, SoundPool& pool, float musicVolume, bool isMainMenu);
    void reset();

    float footstepAccum = 0.0f;
    float lastFallDistance = 0.0f;
    int lastHealth = 20;
    bool wasInWater = false;
    int musicTimer = 0;
    std::vector<std::string> musicPools;
    std::vector<std::string> menuMusicPools;
};
