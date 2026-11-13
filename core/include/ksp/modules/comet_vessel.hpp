// wpfix9: CometVessel.cs (the VesselModule of a comet / asteroid vessel), the engine-agnostic start-up: OnAwake (vessel.Comet), OnStart (the
// removal of the module from a vessel that carries no comet part, the comet type, the home body SMA, the vessel ranges override).
//
// The [KSPField(isPersistant)] members (typeName, cometPartId, the VFX ratios ...) live in the module's FieldStore like every KSPField, so
// the save / load round trip is the generic field list (WP-14). Not ported (scene side, STATUS WP-38d): the GameEvents subscriptions of OnAwake
// and their handlers (OnKnowledgeChange, OnVesselPersistentIdChanged, OnPartDecouple, OnPartPersistentIdChanged), the WaitAndUpdateVFXDimensions
// coroutine and the VFX / collider setters.
#pragma once

#include <memory>

#include "ksp/modules/vessel_module.hpp"

namespace ksp {
class Vessel;
class CometOrbitType;

class CometVessel : public VesselModule {
public:
    explicit CometVessel(const ModuleClassInfo& cls) : VesselModule(cls) {}

    // the MonoBehaviour callbacks are explicit calls (public here, protected in VesselModule)
    void OnAwake() override;  // CometVessel.cs:112
    void OnStart() override;  // CometVessel.cs:126

    Vessel* VesselPtr() const { return static_cast<Vessel*>(vessel); }
    bool destroyed() const { return destroyed_; }                    // UnityEngine.Object.Destroy(this)
    bool isCometTypeReady() const { return isCometTypeReady_; }      // :65
    double homeBodySMA() const { return homeBodySMA_; }              // :63
    double lastUpdateUT() const { return lastUpdateUT_; }            // :72
    const std::shared_ptr<CometOrbitType>& CometType() const { return cometType_; }  // :98

private:
    std::shared_ptr<CometOrbitType> cometType_;
    double homeBodySMA_ = 0.0;
    bool isCometTypeReady_ = false;
    double lastUpdateUT_ = 0.0;
    bool destroyed_ = false;
};

namespace detail {
void RegisterCometVessel();  // VesselModuleFactory::Register("CometVessel", ...)
}  // namespace detail
}  // namespace ksp
