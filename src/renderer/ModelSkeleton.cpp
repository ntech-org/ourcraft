#include "renderer/ModelSkeleton.hpp"

ModelSkeleton::ModelSkeleton() : ModelBiped() {
    bipedRightArm = std::make_unique<ModelRenderer>(40, 16);
    bipedRightArm->addBox(-1.0f, -2.0f, -1.0f, 2, 12, 2, 0.0f);
    bipedRightArm->setRotationPoint(-5.0f, 2.0f, 0.0f);

    bipedLeftArm = std::make_unique<ModelRenderer>(40, 16);
    bipedLeftArm->mirror = true;
    bipedLeftArm->addBox(-1.0f, -2.0f, -1.0f, 2, 12, 2, 0.0f);
    bipedLeftArm->setRotationPoint(5.0f, 2.0f, 0.0f);

    bipedRightLeg = std::make_unique<ModelRenderer>(0, 16);
    bipedRightLeg->addBox(-1.0f, 0.0f, -1.0f, 2, 12, 2, 0.0f);
    bipedRightLeg->setRotationPoint(-2.0f, 12.0f, 0.0f);

    bipedLeftLeg = std::make_unique<ModelRenderer>(0, 16);
    bipedLeftLeg->mirror = true;
    bipedLeftLeg->addBox(-1.0f, 0.0f, -1.0f, 2, 12, 2, 0.0f);
    bipedLeftLeg->setRotationPoint(2.0f, 12.0f, 0.0f);
}
