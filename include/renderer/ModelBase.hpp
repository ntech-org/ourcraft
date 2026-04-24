#pragma once

#include "renderer/Shader.hpp"
#include <glm/glm.hpp>

class ModelBase {
public:
    virtual ~ModelBase() = default;
    virtual void render(Shader& shader, const glm::mat4& baseModel, float limbSwing, float limbSwingAmount, float ageInTicks, float netHeadYaw, float headPitch, float scale, float onGround) = 0;
};
