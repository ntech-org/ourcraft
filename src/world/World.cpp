#include "world/World.hpp"
#include <algorithm>
#include <cmath>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>

World::World() : m_worldTime(6000.0) {}

void World::setGenerator(std::unique_ptr<WorldGenerator> generator) {
    m_generator = std::move(generator);
    m_loader = std::make_unique<ChunkLoader>(*m_generator);
}

void World::addChunk(std::shared_ptr<Chunk> chunk) {
    std::unique_lock<std::shared_mutex> lock(m_chunkMutex);
    m_chunkLookup[chunkKey(chunk->getX(), chunk->getZ())] = chunk;
    m_chunks.push_back(std::move(chunk));
}

void World::requestChunk(int chunkX, int chunkZ) {
    if (isChunkLoaded(chunkX, chunkZ) || isChunkPending(chunkX, chunkZ)) return;
    
    m_pendingChunks.insert(chunkKey(chunkX, chunkZ));
    m_loader->requestChunk(chunkX, chunkZ);
}

void World::pollGeneratedChunks() {
    std::shared_ptr<Chunk> chunk;
    while (m_loader->tryPopResult(chunk)) {
        int cx = chunk->getX();
        int cz = chunk->getZ();

        m_pendingChunks.erase(chunkKey(cx, cz));
        addChunk(chunk); // chunk is copied, but that's fine since we std::move later into m_chunks, wait addChunk moves it.
        // Let's pass by copy to addChunk or change addChunk. Actually, addChunk takes by value. So we pass it by copy and it moves inside.
        // Let's fix addChunk call and neighbors.

        // When a new chunk is generated, notify neighbors to re-mesh for culling
        for (int i = 0; i < Chunk::SECTION_COUNT; ++i) {
            if (auto neighbor = getChunk(cx - 1, cz)) neighbor->touchSection(i);
            if (auto neighbor = getChunk(cx + 1, cz)) neighbor->touchSection(i);
            if (auto neighbor = getChunk(cx, cz - 1)) neighbor->touchSection(i);
            if (auto neighbor = getChunk(cx, cz + 1)) neighbor->touchSection(i);
        }
    }
}

void World::unloadFarChunks(int playerCX, int playerCZ, int keepDistance) {
    std::unique_lock<std::shared_mutex> lock(m_chunkMutex);
    auto it = m_chunks.begin();
    while (it != m_chunks.end()) {
        std::shared_ptr<Chunk>& chunk = *it;
        int cx = chunk->getX();
        int cz = chunk->getZ();

        if (std::abs(cx - playerCX) > keepDistance || std::abs(cz - playerCZ) > keepDistance) {
            m_chunkLookup.erase(chunkKey(cx, cz));
            it = m_chunks.erase(it);
        } else {
            ++it;
        }
    }
}

bool World::isChunkLoaded(int chunkX, int chunkZ) const {
    std::shared_lock<std::shared_mutex> lock(m_chunkMutex);
    return m_chunkLookup.find(chunkKey(chunkX, chunkZ)) != m_chunkLookup.end();
}

bool World::isChunkPending(int chunkX, int chunkZ) const {
    return m_pendingChunks.find(chunkKey(chunkX, chunkZ)) != m_pendingChunks.end();
}

std::shared_ptr<Chunk> World::getChunk(int chunkX, int chunkZ) {
    std::shared_lock<std::shared_mutex> lock(m_chunkMutex);
    const auto it = m_chunkLookup.find(chunkKey(chunkX, chunkZ));
    return it == m_chunkLookup.end() ? nullptr : it->second;
}

std::shared_ptr<const Chunk> World::getChunk(int chunkX, int chunkZ) const {
    std::shared_lock<std::shared_mutex> lock(m_chunkMutex);
    const auto it = m_chunkLookup.find(chunkKey(chunkX, chunkZ));
    return it == m_chunkLookup.end() ? nullptr : it->second;
}

uint8_t World::getBlockID(int worldX, int worldY, int worldZ) const {
    if (worldY < 0 || worldY >= Chunk::HEIGHT) {
        return 0;
    }

    const int chunkX = floorDiv(worldX, Chunk::WIDTH);
    const int chunkZ = floorDiv(worldZ, Chunk::DEPTH);
    std::shared_ptr<const Chunk> chunk = getChunk(chunkX, chunkZ);
    if (!chunk) {
        return 0;
    }

    return chunk->getBlockID(floorMod(worldX, Chunk::WIDTH), worldY, floorMod(worldZ, Chunk::DEPTH));
}

void World::setBlockID(int worldX, int worldY, int worldZ, uint8_t id) {
    if (worldY < 0 || worldY >= Chunk::HEIGHT) {
        return;
    }

    const int chunkX = floorDiv(worldX, Chunk::WIDTH);
    const int chunkZ = floorDiv(worldZ, Chunk::DEPTH);
    std::shared_ptr<Chunk> chunk = getChunk(chunkX, chunkZ);
    if (!chunk) {
        return;
    }

    const int localX = floorMod(worldX, Chunk::WIDTH);
    const int localZ = floorMod(worldZ, Chunk::DEPTH);
    const int sectionIndex = Chunk::getSectionIndex(worldY);

    chunk->setBlockID(localX, worldY, localZ, id);

    if (localX == 0) {
        if (auto neighbor = getChunk(chunkX - 1, chunkZ)) {
            neighbor->touchSection(sectionIndex);
        }
    } else if (localX == Chunk::WIDTH - 1) {
        if (auto neighbor = getChunk(chunkX + 1, chunkZ)) {
            neighbor->touchSection(sectionIndex);
        }
    }

    if (localZ == 0) {
        if (auto neighbor = getChunk(chunkX, chunkZ - 1)) {
            neighbor->touchSection(sectionIndex);
        }
    } else if (localZ == Chunk::DEPTH - 1) {
        if (auto neighbor = getChunk(chunkX, chunkZ + 1)) {
            neighbor->touchSection(sectionIndex);
        }
    }
}

int World::floorDiv(int value, int divisor) {
    int quotient = value / divisor;
    int remainder = value % divisor;
    if (remainder != 0 && ((remainder < 0) != (divisor < 0))) {
        --quotient;
    }
    return quotient;
}

int World::floorMod(int value, int divisor) {
    int remainder = value % divisor;
    if (remainder < 0) {
        remainder += divisor;
    }
    return remainder;
}

std::uint64_t World::chunkKey(int chunkX, int chunkZ) {
    return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(chunkX)) << 32) |
           static_cast<std::uint32_t>(chunkZ);
}

void World::update(float deltaTime) {
    // Match the original day length: 20 ticks per second, 24000 ticks per full day.
    m_worldTime += static_cast<double>(deltaTime) * 20.0;
    if (m_worldTime >= 24000.0) {
        m_worldTime = std::fmod(m_worldTime, 24000.0);
    }
}

float World::getCelestialAngle(float partialTick) const {
    float timeOfDay = static_cast<float>(std::fmod(m_worldTime, 24000.0));
    float angle = (timeOfDay + partialTick) / 24000.0f - 0.25f;
    if (angle < 0.0f) {
        angle += 1.0f;
    }
    if (angle > 1.0f) {
        angle -= 1.0f;
    }

    float base = angle;
    angle = 1.0f - static_cast<float>((std::cos(static_cast<double>(angle) * glm::pi<double>()) + 1.0) * 0.5);
    angle = base + (angle - base) / 3.0f;
    return angle;
}

glm::vec3 World::getSkyColor(float partialTick) const {
    const float angle = getCelestialAngle(partialTick);
    const float daylight = std::clamp(std::cos(angle * glm::two_pi<float>()) * 2.0f + 0.5f, 0.0f, 1.0f);
    return unpackColor(m_skyColor) * daylight;
}

glm::vec3 World::getFogColor(float partialTick) const {
    const float angle = getCelestialAngle(partialTick);
    const float daylight = std::clamp(std::cos(angle * glm::two_pi<float>()) * 2.0f + 0.5f, 0.0f, 1.0f);
    const glm::vec3 fog = unpackColor(m_fogColor);
    return {
        fog.r * (daylight * 0.94f + 0.06f),
        fog.g * (daylight * 0.94f + 0.06f),
        fog.b * (daylight * 0.91f + 0.09f)
    };
}

float World::getBrightness(int x, int y, int z) const {
    static float lightBrightnessTable[16];
    static bool initialized = false;
    if (!initialized) {
        float var0 = 0.05f;
        for (int i = 0; i <= 15; ++i) {
            float var2 = 1.0f - static_cast<float>(i) / 15.0f;
            lightBrightnessTable[i] = (1.0f - var2) / (var2 * 3.0f + 1.0f) * (1.0f - var0) + var0;
        }
        initialized = true;
    }

    if (y < 0) {
        return lightBrightnessTable[0];
    }

    if (y >= 64) {
        return lightBrightnessTable[15];
    }

    // Check if any block above is solid
    for (int ty = y + 1; ty < Chunk::HEIGHT; ++ty) {
        if (getBlockID(x, ty, z) != 0) {
            return lightBrightnessTable[0];
        }
    }

    return lightBrightnessTable[15];
}

float World::getStarBrightness(float partialTick) const {
    const float angle = getCelestialAngle(partialTick);
    float brightness = 1.0f - (std::cos(angle * glm::two_pi<float>()) * 2.0f + 12.0f / 16.0f);
    brightness = std::clamp(brightness, 0.0f, 1.0f);
    return brightness * brightness * 0.5f;
}

float World::getDaylightStrength(float partialTick) const {
    const float angle = getCelestialAngle(partialTick);
    return std::clamp(std::cos(angle * glm::two_pi<float>()) * 2.0f + 0.5f, 0.0f, 1.0f);
}

glm::vec3 World::getSunDirection(float partialTick) const {
    const float angle = getCelestialAngle(partialTick) * glm::two_pi<float>();
    glm::vec3 direction(0.0f, std::cos(angle), std::sin(angle));
    return glm::normalize(direction);
}

glm::vec3 World::unpackColor(std::uint32_t rgb) {
    return {
        static_cast<float>((rgb >> 16) & 255u) / 255.0f,
        static_cast<float>((rgb >> 8) & 255u) / 255.0f,
        static_cast<float>(rgb & 255u) / 255.0f
    };
}
