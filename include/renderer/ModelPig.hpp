#pragma once

#include "renderer/ModelQuadruped.hpp"

class ModelPig : public ModelQuadruped {
public:
    ModelPig() : ModelQuadruped(6, 0.0f) {}
};
