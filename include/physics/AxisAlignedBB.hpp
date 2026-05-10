#pragma once

#include <vector>
#include <glm/vec3.hpp>
#include <optional>

struct RayHit {
    glm::dvec3 hitVec;
    int side;
};

class AxisAlignedBB {
public:
    AxisAlignedBB(double minX, double minY, double minZ, double maxX, double maxY, double maxZ);

    static AxisAlignedBB getBoundingBox(double minX, double minY, double minZ, double maxX, double maxY, double maxZ);

    AxisAlignedBB addCoord(double x, double y, double z) const;
    AxisAlignedBB expand(double x, double y, double z) const;
    AxisAlignedBB getOffsetBoundingBox(double x, double y, double z) const;

    double calculateXOffset(const AxisAlignedBB& other, double offsetX) const;
    double calculateYOffset(const AxisAlignedBB& other, double offsetY) const;
    double calculateZOffset(const AxisAlignedBB& other, double offsetZ) const;

    bool intersectsWith(const AxisAlignedBB& other) const;
    void offset(double x, double y, double z);

    bool isVecInside(const glm::dvec3& vec) const;
    std::optional<RayHit> calculateIntercept(const glm::dvec3& start, const glm::dvec3& end) const;

    double minX, minY, minZ;
    double maxX, maxY, maxZ;
};
