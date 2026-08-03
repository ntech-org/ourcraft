#include "renderer/ModelSheep.hpp"

ModelSheep::ModelSheep() : ModelQuadruped(12, 0.0f) {
    head = std::make_unique<ModelRenderer>(0, 0);
    head->addBox(-3.0f, -4.0f, -6.0f, 6, 6, 8, 0.0f);
    head->setRotationPoint(0.0f, 6.0f, -8.0f);

    body = std::make_unique<ModelRenderer>(28, 8);
    body->addBox(-4.0f, -10.0f, -7.0f, 8, 16, 6, 0.0f);
    body->setRotationPoint(0.0f, 5.0f, 2.0f);
}
