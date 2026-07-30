#pragma once

#include "world/IBlockAccess.hpp"
#include <array>
#include <cstdint>
#include <vector>
#include <mutex>
#include <atomic>
#include <functional>

enum class ChunkState {
    Empty,
    Generating,
    Generated,
    Lighting,
    Lighted,
    Decorating,
    Decorated,
    LightingFinal,
    LightingReady,
    Complete
};

class Chunk {
public:
    static constexpr int WIDTH = 16;
    static constexpr int HEIGHT = 128;
    static constexpr int DEPTH = 16;
    static constexpr int SECTION_HEIGHT = 16;
    static constexpr int SECTION_COUNT = HEIGHT / SECTION_HEIGHT;
    static constexpr int SIZE = WIDTH * HEIGHT * DEPTH;

    Chunk(int x, int z);
    ~Chunk() = default;

    uint8_t getBlockID(int x, int y, int z) const;
    void setBlockID(int x, int y, int z, uint8_t id);
    void setBlockIDSafe(int x, int y, int z, uint8_t id);

    uint8_t getBlockMetadata(int x, int y, int z) const;
    void setBlockMetadata(int x, int y, int z, uint8_t meta);

    int getLight(LightType type, int x, int y, int z) const;
    void setLight(LightType type, int x, int y, int z, int val);

    // Internal high-speed accessors (no locking, no dirty flagging)
    inline int getLightInternal(LightType type, int index) const {
        const std::vector<uint8_t>& data = (type == LightType::Sky) ? m_skylight : m_blocklight;
        int byteIndex = index >> 1;
        std::atomic_ref<const uint8_t> byteRef(data[byteIndex]);
        const uint8_t value = byteRef.load(std::memory_order_relaxed);
        return (index & 1) == 0 ? (value & 0x0F) : ((value >> 4) & 0x0F);
    }

    inline void setLightInternal(LightType type, int index, int val) {
        std::vector<uint8_t>& data = (type == LightType::Sky) ? m_skylight : m_blocklight;
        int byteIndex = index >> 1;
        std::atomic_ref<uint8_t> byteRef(data[byteIndex]);
        uint8_t expected = byteRef.load(std::memory_order_relaxed);
        uint8_t desired;
        do {
            if ((index & 1) == 0) {
                desired = (expected & 0xF0) | (val & 0x0F);
            } else {
                desired = (expected & 0x0F) | ((val & 0x0F) << 4);
            }
        } while (!byteRef.compare_exchange_weak(expected, desired, std::memory_order_relaxed, std::memory_order_relaxed));
    }

    // Fast non-atomic path for single-writer initial lighting (before wipe-complete).
    inline void setLightInternalFast(LightType type, int index, int val) {
        std::vector<uint8_t>& data = (type == LightType::Sky) ? m_skylight : m_blocklight;
        setLightValue(data, index, val);
    }

    // Fill sky light = 15 for y in [minY, maxY] in one column (Y-major, consecutive indices).
    void fillColumnSkyLight15(int x, int z, int minY, int maxY);

    void markSectionDirtyInternal(int sectionIndex);

    int getX() const { return m_x; }
    int getZ() const { return m_z; }

    const uint8_t* getBlocks() const { return m_blocks.data(); }
    const uint8_t* getMetadata() const { return m_metadata.data(); }
    const uint8_t* getSkylight() const { return m_skylight.data(); }
    const uint8_t* getBlocklight() const { return m_blocklight.data(); }
    
    uint8_t* getBlocks() { return m_blocks.data(); }
    uint8_t* getMetadata() { return m_metadata.data(); }
    uint8_t* getSkylight() { return m_skylight.data(); }
    uint8_t* getBlocklight() { return m_blocklight.data(); }
    
    int getHeight(int x, int z) const { return m_heightMap[x + z * WIDTH]; }
    void setHeight(int x, int z, int h) { m_heightMap[x + z * WIDTH] = (uint8_t)h; }
    void generateHeightMap();
    void generateBitmask();
    uint8_t getPrimaryBitmask() const { return m_primaryBitmask; }

    bool isSectionDirty(int sectionIndex) const;
    void clearSectionDirty(int sectionIndex);
    uint32_t getSectionVersion(int sectionIndex) const;
    void touchSection(int sectionIndex);
    void setSectionDirtyCallback(std::function<void(int)> callback) {
        m_sectionDirtyCallback = std::move(callback);
    }

    bool isSectionNonEmpty(int sectionIndex) const { return m_sectionNonEmpty.load(std::memory_order_acquire) & (1u << sectionIndex); }
    uint16_t getSectionNonEmptyMask() const { return m_sectionNonEmpty.load(std::memory_order_acquire); }
    void recomputeSectionNonEmpty();

    float getWaterLevel(int x, int z) const { return m_waterLevels[x + z * WIDTH]; }
    void computeWaterLevels();
    bool hasAnyWater() const { return m_hasAnyWater; }

    ChunkState getState() const { return m_state; }
    void setState(ChunkState state) { m_state = state; }

    bool isLightWipeComplete() const { return m_lightWipeComplete.load(std::memory_order_acquire); }
    void setLightWipeComplete(bool complete) { m_lightWipeComplete.store(complete, std::memory_order_release); }

    std::mutex& getBlockMutex() const { return m_blockMutex; }
    std::mutex& getLightMutex() const { return m_lightMutex; }

    static constexpr int getSectionIndex(int y) {
        return y / SECTION_HEIGHT;
    }

    static constexpr int getSectionMinY(int sectionIndex) {
        return sectionIndex * SECTION_HEIGHT;
    }

private:
    int m_x, m_z;
    std::vector<uint8_t> m_blocks;
    std::vector<uint8_t> m_metadata;
    std::vector<uint8_t> m_skylight;
    std::vector<uint8_t> m_blocklight;
    std::vector<uint8_t> m_heightMap;
    std::array<std::atomic<bool>, SECTION_COUNT> m_sectionDirty {};
    std::array<std::atomic<uint32_t>, SECTION_COUNT> m_sectionVersions {};
    std::array<std::atomic<uint16_t>, SECTION_COUNT> m_sectionNonAirCounts {};
    std::atomic<uint16_t> m_sectionNonEmpty {0};
    std::function<void(int)> m_sectionDirtyCallback;
    std::array<float, WIDTH * DEPTH> m_waterLevels {};
    bool m_hasAnyWater = false;
    
    mutable std::mutex m_blockMutex;
    mutable std::mutex m_lightMutex;
    std::atomic<ChunkState> m_state { ChunkState::Empty };
    std::atomic<bool> m_lightWipeComplete { false };
    uint8_t m_primaryBitmask = 0xFF;

    static inline int getIndex(int x, int y, int z) {
        return (x << 11) | (z << 7) | y;
    }

    static inline int getLightValue(const std::vector<uint8_t>& data, int index) {
        int byteIndex = index >> 1;
        return (index & 1) == 0 ? (data[byteIndex] & 0x0F) : ((data[byteIndex] >> 4) & 0x0F);
    }

    static inline void setLightValue(std::vector<uint8_t>& data, int index, int val) {
        int byteIndex = index >> 1;
        if ((index & 1) == 0) {
            data[byteIndex] = (data[byteIndex] & 0xF0) | (val & 0x0F);
        } else {
            data[byteIndex] = (data[byteIndex] & 0x0F) | ((val & 0x0F) << 4);
        }
    }

    void markSectionDirty(int sectionIndex);
};
