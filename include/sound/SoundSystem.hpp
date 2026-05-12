#pragma once
#include <AL/al.h>
#include <AL/alc.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <algorithm>
#include <future>
#include <mutex>
#include <chrono>

struct SoundBuffer {
    ALuint buffer = 0;
    int sampleRate = 0;
    bool loading = false;
    std::shared_ptr<std::atomic<bool>> loadDone;
    bool empty() const { return buffer == 0; }

    // Raw PCM data produced by background thread, consumed by main thread
    short* rawPCM = nullptr;
    int rawChannels = 0;
    int rawTotalSamples = 0;
};

struct PendingPlay {
    const SoundBuffer* buffer = nullptr;
    float volume = 1.0f;
    float pitch = 1.0f;
    bool is3D = false;
    float x = 0, y = 0, z = 0;
    int framesWaited = 0;
};

class SoundSystem {
public:
    ~SoundSystem();

    bool init();
    void shutdown();
    void update(float listenerX, float listenerY, float listenerZ,
                float listenerYaw, float listenerPitch,
                float soundVolume = 1.0f, float musicVolume = 1.0f);

    const SoundBuffer* loadOGG(const std::string& path);

    void play(const SoundBuffer* buf, float volume, float pitch);
    void play3D(const SoundBuffer* buf, float x, float y, float z,
                float volume, float pitch);

    void stopAll();
    void playMusic(const SoundBuffer* buf, float volume, float pitch);
    bool isMusicPlaying() const;

    void pollLoads();

private:
    ALCdevice* m_device = nullptr;
    ALCcontext* m_context = nullptr;
    std::unordered_map<std::string, std::unique_ptr<SoundBuffer>> m_cache;
    std::mutex m_cacheMutex;
    std::vector<ALuint> m_sources;
    std::vector<PendingPlay> m_pendingPlays;
    ALuint m_musicSource = 0;
    bool m_initialized = false;

    ALuint createSource(const SoundBuffer* buf, float volume, float pitch,
                        bool is3D, float x, float y, float z, bool addToSources = true);
    void cleanupSources();
    void flushPending(const SoundBuffer* buf);
};
