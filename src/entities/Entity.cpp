#include "entities/Entity.hpp"
#include "world/World.hpp"
#include "world/Block.hpp"
#include "world/Material.hpp"
#include <cmath>

#include <algorithm>

Entity::Entity(World& world) 
    : worldObj(world), boundingBox(0, 0, 0, 0, 0, 0) 
{
    setPosition(0, 0, 0);
}

Entity::~Entity() {}
void Entity::setPosition(double x, double y, double z) {
    prevPosX = posX = x;
    prevPosY = posY = y;
    prevPosZ = posZ = z;
    float w2 = width / 2.0f;
    boundingBox = AxisAlignedBB(x - w2, y, z - w2, x + w2, y + height, z + w2);
}

void Entity::setSize(float w, float h) {
    width = w;
    height = h;
}

void Entity::onUpdate() {
    prevPosX = posX;
    prevPosY = posY;
    prevPosZ = posZ;
    prevRotationYaw = rotationYaw;
    prevRotationPitch = rotationPitch;
    prevDistanceWalkedModified = distanceWalkedModified;
}

void Entity::moveEntity(double dx, double dy, double dz) {
    double origDX = dx;
    double origDY = dy;
    double origDZ = dz;

    double oldX = posX;
    double oldZ = posZ;

    AxisAlignedBB oldBB = boundingBox;
    std::vector<AxisAlignedBB> list = worldObj.getCollidingBoundingBoxes(boundingBox.addCoord(dx, dy, dz));

    for (const auto& aabb : list) {
        dy = aabb.calculateYOffset(boundingBox, dy);
    }
    boundingBox.offset(0.0, dy, 0.0);

    bool var29 = onGround || (origDY != dy && origDY < 0.0);

    for (const auto& aabb : list) {
        dx = aabb.calculateXOffset(boundingBox, dx);
    }
    boundingBox.offset(dx, 0.0, 0.0);

    for (const auto& aabb : list) {
        dz = aabb.calculateZOffset(boundingBox, dz);
    }
    boundingBox.offset(0.0, 0.0, dz);

    if (stepHeight > 0.0f && var29 && ySize < 0.05f && (origDX != dx || origDZ != dz)) {
        double dX1 = dx;
        double dY1 = dy;
        double dZ1 = dz;
        dx = origDX;
        dy = (double)stepHeight;
        dz = origDZ;
        AxisAlignedBB stepBB = boundingBox;
        boundingBox = oldBB;

        std::vector<AxisAlignedBB> list2 = worldObj.getCollidingBoundingBoxes(boundingBox.addCoord(dx, dy, dz));
        for (const auto& aabb : list2) {
            dy = aabb.calculateYOffset(boundingBox, dy);
        }
        boundingBox.offset(0.0, dy, 0.0);
        for (const auto& aabb : list2) {
            dx = aabb.calculateXOffset(boundingBox, dx);
        }
        boundingBox.offset(dx, 0.0, 0.0);
        for (const auto& aabb : list2) {
            dz = aabb.calculateZOffset(boundingBox, dz);
        }
        boundingBox.offset(0.0, 0.0, dz);

        if (dX1 * dX1 + dZ1 * dZ1 >= dx * dx + dz * dz) {
            dx = dX1;
            dy = dY1;
            dz = dZ1;
            boundingBox = stepBB;
        } else {
            ySize += 0.5f;
        }
    }

    posX = (boundingBox.minX + boundingBox.maxX) / 2.0;
    posY = boundingBox.minY - (double)ySize;
    posZ = (boundingBox.minZ + boundingBox.maxZ) / 2.0;

    isCollidedHorizontally = origDX != dx || origDZ != dz;
    isCollided = isCollidedHorizontally || origDY != dy;
    onGround = origDY != dy && origDY < 0.0;

    if (onGround) {
        if (fallDistance > 0.0f) {
            fall(fallDistance);
            fallDistance = 0.0f;
        }
    } else if (dy < 0.0) {
        fallDistance -= (float)dy;
    }

    if (origDX != dx) motionX = 0.0;

    if (origDY != dy) motionY = 0.0;
    if (origDZ != dz) motionZ = 0.0;

    double moveX = posX - oldX;
    double moveZ = posZ - oldZ;
    distanceWalkedModified += (float)(std::sqrt(moveX * moveX + moveZ * moveZ) * 0.6);

    ySize *= 0.4f;
}


void Entity::preparePlayerToSpawn() {
    // Basic spawn logic: find first non-colliding Y
    while (posY > 0.0) {
        setPosition(posX, posY, posZ);
        if (worldObj.getCollidingBoundingBoxes(boundingBox).empty()) break;
        posY += 1.0;
    }
    motionX = motionY = motionZ = 0.0;
}

bool Entity::handleWaterMovement() {
    if (worldObj.handleMaterialAcceleration(boundingBox.expand(0.0, -0.1, 0.0), Material::water, this)) {
        inWater = true;
        fallDistance = 0.0f;
    } else {
        inWater = false;
    }
    return inWater;
}

bool Entity::isOffsetPositionInLiquid(double dx, double dy, double dz) {
    AxisAlignedBB offsetBB = boundingBox.getOffsetBoundingBox(dx, dy, dz);
    if (!worldObj.getCollidingBoundingBoxes(offsetBB).empty()) return false;
    return !worldObj.getIsAnyLiquid(offsetBB);
}

void Entity::fall(float distance) {}

bool Entity::isInsideOfMaterial(const Material& material) const {
    double eyeY = posY + (double)yOffset;
    int ix = (int)std::floor(posX);
    int iy = (int)std::floor(eyeY);
    int iz = (int)std::floor(posZ);
    uint8_t id = worldObj.getBlockID(ix, iy, iz);
    if (id == 0) return material == Material::air;
    return Block::blocksList[id]->blockMaterial == material;
}


bool Entity::isEntityInsideOpaqueBlock() const {
    double eyeY = posY + (double)yOffset;
    int ix = (int)std::floor(posX);
    int iy = (int)std::floor(eyeY);
    int iz = (int)std::floor(posZ);
    uint8_t id = worldObj.getBlockID(ix, iy, iz);
    if (id == 0) return false;
    return Block::blocksList[id]->isOccluder();
}
