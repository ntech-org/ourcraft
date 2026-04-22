#include "physics/AxisAlignedBB.hpp"
#include <algorithm>

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
