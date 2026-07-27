#include "simulation/SoundManager.hpp"
#include "sound/SoundSystem.hpp"
#include "sound/SoundPool.hpp"
#include <cstdlib>

void SoundManager::tick(SoundSystem& sound, SoundPool& pool, float musicVolume, bool isMainMenu) {
    // Keep a long quiet gap after each track, matching the sparse Infdev soundtrack.
    if (!sound.isMusicPlaying() && ++musicTimer > 7200) {
        musicTimer = 0;
        const auto& pools = isMainMenu ? menuMusicPools : musicPools;
        if (!pools.empty()) {
            int idx = std::rand() % (int)pools.size();
            if (auto* snd = pool.getRandom(pools[idx], sound))
                sound.playMusic(snd, musicVolume, 1.0f);
        }
    }
}

void SoundManager::reset() {
    footstepAccum = 0.0f;
    lastFallDistance = 0.0f;
    lastHealth = 20;
    wasInWater = false;
    musicTimer = 0;
}
