#pragma once

#include "renderer/Bounds.hpp"
#include <array>
#include <glm/glm.hpp>

class Frustum {
public:
    void update(const glm::mat4& viewProjection);
    bool intersects(const AABB& bounds, const glm::vec3& offset = glm::vec3(0.0f)) const;
    const std::array<glm::vec4, 6>& getPlanes() const { return m_planes; }

private:
    std::array<glm::vec4, 6> m_planes {};
};
