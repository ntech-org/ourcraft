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
    ALuint monoBuffer = 0;
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
    double x = 0, y = 0, z = 0;
    int framesWaited = 0;
};

class SoundSystem {
public:
    ~SoundSystem();

    bool init();
    void shutdown();
    void update(double listenerX, double listenerY, double listenerZ,
                float listenerYaw, float listenerPitch,
                float soundVolume = 1.0f, float musicVolume = 1.0f);

    const SoundBuffer* loadOGG(const std::string& path);

    void play(const SoundBuffer* buf, float volume, float pitch);
    void play3D(const SoundBuffer* buf, double x, double y, double z,
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
    struct ActiveSource {
        ALuint source = 0;
        float baseVolume = 1.0f;
        bool is3D = false;
        double x = 0, y = 0, z = 0;
    };
    std::vector<ActiveSource> m_sources;
    std::vector<PendingPlay> m_pendingPlays;
    ALuint m_musicSource = 0;
    bool m_initialized = false;
    double m_listenerX = 0, m_listenerY = 0, m_listenerZ = 0;

    ALuint createSource(const SoundBuffer* buf, float volume, float pitch,
                        bool is3D, double x, double y, double z, bool addToSources = true);
    void cleanupSources();
    void flushPending(const SoundBuffer* buf);
};
