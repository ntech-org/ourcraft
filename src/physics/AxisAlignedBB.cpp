#include "physics/AxisAlignedBB.hpp"
#include <algorithm>
#include <glm/geometric.hpp>

AxisAlignedBB::AxisAlignedBB(double minX, double minY, double minZ, double maxX, double maxY, double maxZ)
    : minX(minX), minY(minY), minZ(minZ), maxX(maxX), maxY(maxY), maxZ(maxZ) {}

AxisAlignedBB AxisAlignedBB::getBoundingBox(double minX, double minY, double minZ, double maxX, double maxY, double maxZ) {
    return AxisAlignedBB(minX, minY, minZ, maxX, maxY, maxZ);
}

AxisAlignedBB AxisAlignedBB::addCoord(double x, double y, double z) const {
    double x0 = minX;
    double y0 = minY;
    double z0 = minZ;
    double x1 = maxX;
    double y1 = maxY;
    double z1 = maxZ;

    if (x < 0.0) x0 += x;
    if (x > 0.0) x1 += x;
    if (y < 0.0) y0 += y;
    if (y > 0.0) y1 += y;
    if (z < 0.0) z0 += z;
    if (z > 0.0) z1 += z;

    return AxisAlignedBB(x0, y0, z0, x1, y1, z1);
}

AxisAlignedBB AxisAlignedBB::expand(double x, double y, double z) const {
    return AxisAlignedBB(minX - x, minY - y, minZ - z, maxX + x, maxY + y, maxZ + z);
}

AxisAlignedBB AxisAlignedBB::getOffsetBoundingBox(double x, double y, double z) const {
    return AxisAlignedBB(minX + x, minY + y, minZ + z, maxX + x, maxY + y, maxZ + z);
}

double AxisAlignedBB::calculateXOffset(const AxisAlignedBB& other, double offsetX) const {
    if (other.maxY <= minY || other.minY >= maxY) return offsetX;
    if (other.maxZ <= minZ || other.minZ >= maxZ) return offsetX;

    if (offsetX > 0.0 && other.maxX <= minX) {
        double d = minX - other.maxX;
        if (d < offsetX) offsetX = d;
    }
    if (offsetX < 0.0 && other.minX >= maxX) {
        double d = maxX - other.minX;
        if (d > offsetX) offsetX = d;
    }
    return offsetX;
}

double AxisAlignedBB::calculateYOffset(const AxisAlignedBB& other, double offsetY) const {
    if (other.maxX <= minX || other.minX >= maxX) return offsetY;
    if (other.maxZ <= minZ || other.minZ >= maxZ) return offsetY;

    if (offsetY > 0.0 && other.maxY <= minY) {
        double d = minY - other.maxY;
        if (d < offsetY) offsetY = d;
    }
    if (offsetY < 0.0 && other.minY >= maxY) {
        double d = maxY - other.minY;
        if (d > offsetY) offsetY = d;
    }
    return offsetY;
}

double AxisAlignedBB::calculateZOffset(const AxisAlignedBB& other, double offsetZ) const {
    if (other.maxX <= minX || other.minX >= maxX) return offsetZ;
    if (other.maxY <= minY || other.minY >= maxY) return offsetZ;

    if (offsetZ > 0.0 && other.maxZ <= minZ) {
        double d = minZ - other.maxZ;
        if (d < offsetZ) offsetZ = d;
    }
    if (offsetZ < 0.0 && other.minZ >= maxZ) {
        double d = maxZ - other.minZ;
        if (d > offsetZ) offsetZ = d;
    }
    return offsetZ;
}

bool AxisAlignedBB::intersectsWith(const AxisAlignedBB& other) const {
    if (other.maxX <= minX || other.minX >= maxX) return false;
    if (other.maxY <= minY || other.minY >= maxY) return false;
    if (other.maxZ <= minZ || other.minZ >= maxZ) return false;
    return true;
}

void AxisAlignedBB::offset(double x, double y, double z) {
    minX += x;
    minY += y;
    minZ += z;
    maxX += x;
    maxY += y;
    maxZ += z;
}

bool AxisAlignedBB::isVecInside(const glm::dvec3& vec) const {
    return vec.x > minX && vec.x < maxX && vec.y > minY && vec.y < maxY && vec.z > minZ && vec.z < maxZ;
}

std::optional<RayHit> AxisAlignedBB::calculateIntercept(const glm::dvec3& start, const glm::dvec3& end) const {
    auto isVecInYZ = [&](const glm::dvec3& v) { return v.y >= minY && v.y <= maxY && v.z >= minZ && v.z <= maxZ; };
    auto isVecInXZ = [&](const glm::dvec3& v) { return v.x >= minX && v.x <= maxX && v.z >= minZ && v.z <= maxZ; };
    auto isVecInXY = [&](const glm::dvec3& v) { return v.x >= minX && v.x <= maxX && v.y >= minY && v.y <= maxY; };

    auto getIntersection = [&](double startV, double endV, double targetV, const glm::dvec3& s, const glm::dvec3& e) -> std::optional<glm::dvec3> {
        if (std::abs(endV - startV) < 1e-7) return std::nullopt;
        double t = (targetV - startV) / (endV - startV);
        if (t < 0.0 || t > 1.0) return std::nullopt;
        return s + (e - s) * t;
    };

    std::optional<glm::dvec3> vminX = getIntersection(start.x, end.x, minX, start, end);
    std::optional<glm::dvec3> vmaxX = getIntersection(start.x, end.x, maxX, start, end);
    std::optional<glm::dvec3> vminY = getIntersection(start.y, end.y, minY, start, end);
    std::optional<glm::dvec3> vmaxY = getIntersection(start.y, end.y, maxY, start, end);
    std::optional<glm::dvec3> vminZ = getIntersection(start.z, end.z, minZ, start, end);
    std::optional<glm::dvec3> vmaxZ = getIntersection(start.z, end.z, maxZ, start, end);

    if (vminX && !isVecInYZ(*vminX)) vminX = std::nullopt;
    if (vmaxX && !isVecInYZ(*vmaxX)) vmaxX = std::nullopt;
    if (vminY && !isVecInXZ(*vminY)) vminY = std::nullopt;
    if (vmaxY && !isVecInXZ(*vmaxY)) vmaxY = std::nullopt;
    if (vminZ && !isVecInXY(*vminZ)) vminZ = std::nullopt;
    if (vmaxZ && !isVecInXY(*vmaxZ)) vmaxZ = std::nullopt;

    std::optional<glm::dvec3> bestV = std::nullopt;
    int side = -1;

    auto updateBest = [&](const std::optional<glm::dvec3>& v, int s) {
        if (v) {
            if (!bestV || glm::distance(start, *v) < glm::distance(start, *bestV)) {
                bestV = v;
                side = s;
            }
        }
    };

    updateBest(vminX, 4);
    updateBest(vmaxX, 5);
    updateBest(vminY, 0);
    updateBest(vmaxY, 1);
    updateBest(vminZ, 2);
    updateBest(vmaxZ, 3);

    if (bestV) return RayHit{*bestV, side};
    return std::nullopt;
}
