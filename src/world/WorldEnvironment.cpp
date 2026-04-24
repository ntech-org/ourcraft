#include "world/World.hpp"
#include "world/Block.hpp"
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <cmath>

float World::getCelestialAngle(float partialTick) const {
    float timeOfDay = static_cast<float>(std::fmod(m_worldTime, 24000.0));
    float angle = (timeOfDay + partialTick) / 24000.0f - 0.25f;
    if (angle < 0.0f) angle += 1.0f;
    if (angle > 1.0f) angle -= 1.0f;

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

    if (y < 0) return lightBrightnessTable[0];
    if (y >= Chunk::HEIGHT) return lightBrightnessTable[15];

    int currentLight = 15;
    for (int ty = Chunk::HEIGHT - 1; ty > y; --ty) {
        uint8_t bid = getBlockID(x, ty, z);
        if (bid == 0) continue;
        int opacity = Block::lightOpacity[bid];
        if (opacity <= 0) continue;
        currentLight -= opacity;
        if (currentLight <= 0) { currentLight = 0; break; }
    }
    return lightBrightnessTable[currentLight];
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
    return glm::normalize(glm::vec3(0.0f, std::cos(angle), std::sin(angle)));
}

glm::vec3 World::unpackColor(std::uint32_t rgb) {
    return {
        static_cast<float>((rgb >> 16) & 255u) / 255.0f,
        static_cast<float>((rgb >> 8) & 255u) / 255.0f,
        static_cast<float>(rgb & 255u) / 255.0f
    };
}
