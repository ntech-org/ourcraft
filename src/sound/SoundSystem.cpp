#include "sound/SoundSystem.hpp"
#include <iostream>
#include <cmath>
#include <cstdlib>

#include <stb/stb_vorbis.c>

SoundSystem::~SoundSystem() {
    shutdown();
}

bool SoundSystem::init() {
    m_device = alcOpenDevice(nullptr);
    if (!m_device) {
        std::cerr << "[Sound] Failed to open OpenAL device" << std::endl;
        return false;
    }

    m_context = alcCreateContext(m_device, nullptr);
    if (!m_context || !alcMakeContextCurrent(m_context)) {
        std::cerr << "[Sound] Failed to create OpenAL context" << std::endl;
        if (m_context) alcDestroyContext(m_context);
        alcCloseDevice(m_device);
        m_device = nullptr;
        m_context = nullptr;
        return false;
    }

    alListener3f(AL_POSITION, 0, 0, 0);
    alListener3f(AL_VELOCITY, 0, 0, 0);
    ALfloat orient[] = {0, 0, -1, 0, 1, 0};
    alListenerfv(AL_ORIENTATION, orient);
    alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);

    m_initialized = true;
    std::cout << "[Sound] OpenAL initialized" << std::endl;
    return true;
}

void SoundSystem::shutdown() {
    stopAll();

    for (auto& [path, buf] : m_cache) {
        if (buf) {
            if (buf->buffer) alDeleteBuffers(1, &buf->buffer);
            if (buf->monoBuffer) alDeleteBuffers(1, &buf->monoBuffer);
        }
    }
    m_cache.clear();

    if (m_context) {
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(m_context);
        m_context = nullptr;
    }
    if (m_device) {
        alcCloseDevice(m_device);
        m_device = nullptr;
    }
    m_initialized = false;
}

void SoundSystem::flushPending(const SoundBuffer* buf) {
    for (size_t i = 0; i < m_pendingPlays.size();) {
        auto& p = m_pendingPlays[i];
        if (p.buffer == buf) {
            if (p.is3D)
                createSource(buf, p.volume, p.pitch, true, p.x, p.y, p.z);
            else
                createSource(buf, p.volume, p.pitch, false, 0, 0, 0);
            m_pendingPlays.erase(m_pendingPlays.begin() + i);
        } else {
            ++i;
        }
    }
}

void SoundSystem::pollLoads() {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    for (auto& [path, buf] : m_cache) {
        if (!buf || !buf->loading || !buf->loadDone || !buf->loadDone->load()) continue;

        // Async decode finished. Upload PCM to OpenAL on main thread.
        if (buf->rawPCM && buf->rawTotalSamples > 0 && buf->rawChannels > 0) {
            int totalShorts = buf->rawTotalSamples * buf->rawChannels;
            ALsizei dataSize = totalShorts * (ALsizei)sizeof(short);
            ALenum format = (buf->rawChannels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;

            ALuint alBuffer = 0;
            alGenBuffers(1, &alBuffer);
            alGetError();
            alBufferData(alBuffer, format, buf->rawPCM, dataSize, buf->sampleRate);
            if (alGetError() == AL_NO_ERROR) {
                buf->buffer = alBuffer;
            } else {
                alDeleteBuffers(1, &alBuffer);
                std::cerr << "[Sound] alBufferData failed: " << path << std::endl;
            }

            // For stereo sources, also create a mono buffer for proper 3D spatialization
            if (buf->rawChannels == 2) {
                int monoCount = buf->rawTotalSamples;
                short* mono = (short*)malloc(monoCount * sizeof(short));
                for (int i = 0; i < monoCount; ++i) {
                    int l = buf->rawPCM[i * 2];
                    int r = buf->rawPCM[i * 2 + 1];
                    mono[i] = (short)((l + r) >> 1);
                }
                ALuint monoBuf = 0;
                alGenBuffers(1, &monoBuf);
                alGetError();
                alBufferData(monoBuf, AL_FORMAT_MONO16, mono, monoCount * (ALsizei)sizeof(short), buf->sampleRate);
                if (alGetError() == AL_NO_ERROR) {
                    buf->monoBuffer = monoBuf;
                } else {
                    alDeleteBuffers(1, &monoBuf);
                }
                free(mono);
            }
        }
        free(buf->rawPCM);
        buf->rawPCM = nullptr;

        buf->loading = false;
        flushPending(buf.get());
    }

    // Timeout stale pending plays (>300 frames ≈ 5s)
    for (size_t i = 0; i < m_pendingPlays.size();) {
        if (++m_pendingPlays[i].framesWaited > 300)
            m_pendingPlays.erase(m_pendingPlays.begin() + i);
        else
            ++i;
    }
}

const SoundBuffer* SoundSystem::loadOGG(const std::string& path) {
    if (!m_initialized) return nullptr;

    // Check cache (under lock for thread safety with async loads)
    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        auto it = m_cache.find(path);
        if (it != m_cache.end()) return it->second.get();
    }

    // Create a placeholder buffer with loading flag
    auto buf = std::make_unique<SoundBuffer>();
    buf->loading = true;
    buf->sampleRate = 0;
    auto loadDone = std::make_shared<std::atomic<bool>>(false);
    buf->loadDone = loadDone;
    SoundBuffer* rawBuf = buf.get();

    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        m_cache[path] = std::move(buf);
    }

    // Decode on background thread — PCM only, no AL calls (AL is main-thread only)
    std::thread([loadDone, rawBuf, path]() {
        int channels = 0, sampleRate = 0;
        short* pcm = nullptr;
        int totalSamples = stb_vorbis_decode_filename(path.c_str(), &channels, &sampleRate, &pcm);

        bool ok = false;
        if (totalSamples > 0 && pcm && channels > 0) {
            if (totalSamples * channels > 0) {
                rawBuf->rawPCM = pcm;
                rawBuf->rawChannels = channels;
                rawBuf->rawTotalSamples = totalSamples;
                rawBuf->sampleRate = sampleRate;
                pcm = nullptr;
                ok = true;
            }
        }
        free(pcm);
        if (!ok) std::cerr << "[Sound] Async load failed: " << path << std::endl;
        loadDone->store(true);
    }).detach();

    return rawBuf;
}

ALuint SoundSystem::createSource(const SoundBuffer* buf, float volume, float pitch,
                                  bool is3D, double x, double y, double z, bool addToSources) {
    if (!m_initialized || !buf || buf->empty()) return 0;
    ALuint src = 0;
    alGenSources(1, &src);
    if (alGetError() != AL_NO_ERROR) return 0;

    // Use mono buffer for 3D spatialization if available (stereo cannot be spatialized)
    ALuint useBuffer = (is3D && buf->monoBuffer) ? buf->monoBuffer : buf->buffer;
    alSourcei(src, AL_BUFFER, (ALint)useBuffer);
    alSourcef(src, AL_GAIN, volume);
    alSourcef(src, AL_PITCH, pitch);
    alSourcei(src, AL_LOOPING, AL_FALSE);

    if (is3D) {
        alSourcei(src, AL_SOURCE_RELATIVE, AL_FALSE);
        alSource3f(src, AL_POSITION,
                   static_cast<float>(x - m_listenerX),
                   static_cast<float>(y - m_listenerY),
                   static_cast<float>(z - m_listenerZ));
        alSourcef(src, AL_REFERENCE_DISTANCE, 1.5f);
        alSourcef(src, AL_MAX_DISTANCE, 24.0f);
        alSourcef(src, AL_ROLLOFF_FACTOR, 1.0f);
    } else {
        alSourcei(src, AL_SOURCE_RELATIVE, AL_TRUE);
        alSource3f(src, AL_POSITION, 0, 0, 0);
        alSourcef(src, AL_REFERENCE_DISTANCE, 1.0f);
        alSourcef(src, AL_MAX_DISTANCE, 1.0f);
        alSourcef(src, AL_ROLLOFF_FACTOR, 0.0f);
    }

    alSourcePlay(src);
    if (addToSources) {
        m_sources.push_back({src, volume, is3D, x, y, z});
    }
    return src;
}

void SoundSystem::playMusic(const SoundBuffer* buf, float volume, float pitch) {
    if (!buf || buf->empty() || !m_initialized) return;
    if (m_musicSource) {
        alSourceStop(m_musicSource);
        alDeleteSources(1, &m_musicSource);
    }
    m_musicSource = createSource(buf, volume, pitch, false, 0, 0, 0, false);
}

bool SoundSystem::isMusicPlaying() const {
    if (!m_musicSource) return false;
    ALint state;
    alGetSourcei(m_musicSource, AL_SOURCE_STATE, &state);
    return state == AL_PLAYING;
}

void SoundSystem::play(const SoundBuffer* buf, float volume, float pitch) {
    if (!buf || !m_initialized) return;
    if (!buf->empty()) {
        createSource(buf, volume, pitch, false, 0, 0, 0);
        return;
    }
    if (buf->loading) {
        // Deduplicate: if same buffer already queued, update it (keeps latest)
        for (auto& p : m_pendingPlays)
            if (p.buffer == buf) { p.volume = volume; p.pitch = pitch; p.framesWaited = 0; return; }
        m_pendingPlays.push_back({buf, volume, pitch, false, 0, 0, 0, 0});
    }
}

void SoundSystem::play3D(const SoundBuffer* buf, double x, double y, double z,
                          float volume, float pitch) {
    if (!buf || !m_initialized) return;
    if (!buf->empty()) {
        createSource(buf, volume, pitch, true, x, y, z);
        return;
    }
    if (buf->loading) {
        for (auto& p : m_pendingPlays)
            if (p.buffer == buf) { p.volume = volume; p.pitch = pitch; p.x = x; p.y = y; p.z = z; p.framesWaited = 0; return; }
        m_pendingPlays.push_back({buf, volume, pitch, true, x, y, z, 0});
    }
}

void SoundSystem::stopAll() {
    for (auto& as : m_sources) {
        alSourceStop(as.source);
        alDeleteSources(1, &as.source);
    }
    m_sources.clear();
    if (m_musicSource) {
        alSourceStop(m_musicSource);
        alDeleteSources(1, &m_musicSource);
        m_musicSource = 0;
    }
}

void SoundSystem::cleanupSources() {
    m_sources.erase(std::remove_if(m_sources.begin(), m_sources.end(),
        [](ActiveSource& as) {
            if (!as.source) return true;
            ALint state;
            alGetSourcei(as.source, AL_SOURCE_STATE, &state);
            if (state == AL_STOPPED) {
                alDeleteSources(1, &as.source);
                return true;
            }
            return false;
        }), m_sources.end());

    // Clean up finished music source
    if (m_musicSource) {
        ALint state;
        alGetSourcei(m_musicSource, AL_SOURCE_STATE, &state);
        if (state == AL_STOPPED) {
            alDeleteSources(1, &m_musicSource);
            m_musicSource = 0;
        }
    }
}

void SoundSystem::update(double lx, double ly, double lz, float yaw, float pitch,
                          float soundVolume, float musicVolume) {
    if (!m_initialized) return;

    pollLoads();

    m_listenerX = lx;
    m_listenerY = ly;
    m_listenerZ = lz;
    alListener3f(AL_POSITION, 0, 0, 0);

    float radYaw = yaw * 3.14159265f / 180.0f;
    float radPitch = pitch * 3.14159265f / 180.0f;
    float lookX = -std::sin(radYaw) * std::cos(radPitch);
    float lookY = std::sin(radPitch);
    float lookZ = std::cos(radYaw) * std::cos(radPitch);
    ALfloat orient[] = {lookX, lookY, lookZ, 0, 1, 0};
    alListenerfv(AL_ORIENTATION, orient);

    cleanupSources();

    // Apply master volumes to all active sources (real-time slider support)
    for (auto& as : m_sources) {
        if (!as.source) continue;
        ALint state;
        alGetSourcei(as.source, AL_SOURCE_STATE, &state);
        if (state == AL_PLAYING || state == AL_PAUSED) {
            if (as.is3D) {
                alSource3f(as.source, AL_POSITION,
                           static_cast<float>(as.x - m_listenerX),
                           static_cast<float>(as.y - m_listenerY),
                           static_cast<float>(as.z - m_listenerZ));
            }
            alSourcef(as.source, AL_GAIN, as.baseVolume * soundVolume);
        }
    }
    if (m_musicSource) {
        ALint state;
        alGetSourcei(m_musicSource, AL_SOURCE_STATE, &state);
        if (state == AL_PLAYING || state == AL_PAUSED)
            alSourcef(m_musicSource, AL_GAIN, musicVolume);
    }
}
