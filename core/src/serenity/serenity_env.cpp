// WP-53: SerenityEnvironment (include/ksp/serenity/serenity_env.hpp)
#include "ksp/serenity/serenity_env.hpp"

#include "ksp/serenity/deployed_science.hpp"
#include "ksp/serenity/roc.hpp"

namespace ksp::Expansions::Serenity {

void SerenityEnvironment::Reset() {
    screenMessages.clear();
    vesselMarkerRefreshes = 0;
    mapObjectRefreshes = 0;
    activeKerbalInventory.reset();
    groundContact = nullptr;
    physicsAction = nullptr;
    physicsActions.clear();
    srfVelocityMagnitude = nullptr;
    partRbVelocityMagnitude = nullptr;
    trackingLineOfSight = nullptr;
    hasModelTransform = nullptr;
    panelAtOrigin = nullptr;
    rocPrefabBoundsY = nullptr;
}

}  // namespace ksp::Expansions::Serenity

namespace ksp {

void RegisterSerenityScenarioModules() {
    Expansions::Serenity::DeployedScience::Runtime::DeployedScience::Register();
    ROCScenario::Register();
}

}  // namespace ksp
