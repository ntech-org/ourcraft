#include "renderer/Frustum.hpp"
#include <glm/geometric.hpp>

namespace {
glm::vec4 normalizePlane(const glm::vec4& plane) {
    const glm::vec3 normal(plane.x, plane.y, plane.z);
    const float length = glm::length(normal);
    if (length == 0.0f) {
        return plane;
    }
    return plane / length;
}
}

void Frustum::update(const glm::mat4& viewProjection) {
    const glm::mat4 matrix = glm::transpose(viewProjection);

    m_planes[0] = normalizePlane(matrix[3] + matrix[0]);
    m_planes[1] = normalizePlane(matrix[3] - matrix[0]);
    m_planes[2] = normalizePlane(matrix[3] + matrix[1]);
    m_planes[3] = normalizePlane(matrix[3] - matrix[1]);
    m_planes[4] = normalizePlane(matrix[3] + matrix[2]);
    m_planes[5] = normalizePlane(matrix[3] - matrix[2]);
}

bool Frustum::intersects(const AABB& bounds) const {
    for (const glm::vec4& plane : m_planes) {
        glm::vec3 positive = bounds.min;
        if (plane.x >= 0.0f) positive.x = bounds.max.x;
        if (plane.y >= 0.0f) positive.y = bounds.max.y;
        if (plane.z >= 0.0f) positive.z = bounds.max.z;

        if (glm::dot(glm::vec3(plane), positive) + plane.w < 0.0f) {
            return false;
        }
    }

    return true;
}
