#pragma once

#include "renderer/Bounds.hpp"
#include <array>
#include <glm/glm.hpp>

class Frustum {
public:
    void update(const glm::mat4& viewProjection);
    bool intersects(const AABB& bounds) const;

private:
    std::array<glm::vec4, 6> m_planes {};
};
