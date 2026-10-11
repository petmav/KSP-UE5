// ModuleCargoPart.cs (WP-53 slice): the cargo-part base class of ModuleGroundPart (Breaking Ground deployable parts).
//
// WP-06 only needed its OnLoad (stackableQuantity rule, module_behaviours.cpp); WP-53 moves the class here and adds what the ground-part
// subclasses reach: GetInfo / GetTooltip, IsDeployedSciencePart, EnableModule, OnPartPack / OnPartUnpack, MakePartSettle, the coroutine
// helpers. The EVA-construction highlight, the welding sparks (AudioSource) and the compound-part link bookkeeping are UI / audio / WP-33
// (EVA construction) and are not ported; WP-33 extends this class.
#pragma once

#include <memory>
#include <string>

#include "ksp/core/coroutine.hpp"
#include "ksp/modules/part_module.hpp"

namespace ksp {

class Part;
class Vessel;
class ModuleGroundSciencePart;

// ModuleCargoPart.cs:8
class ModuleCargoPart : public PartModule {
public:
    explicit ModuleCargoPart(const ModuleClassInfo& cls) : PartModule(cls) {}
    ~ModuleCargoPart() override;

    // [KSPField] accessors (the schema owns the values)
    float packedVolume() const { return Get<float>("packedVolume"); }                               // :11
    float kinematicDelay() const { return Get<float>("kinematicDelay"); }                           // :14
    float placementMaxRivotVelocity() const { return Get<float>("placementMaxRivotVelocity"); }     // :17
    int32_t stackableQuantity() const { return Get<int32_t>("stackableQuantity"); }                 // :23
    bool beingAttached() const { return Get<bool>("beingAttached"); }                               // :36 internal
    bool beingSettled() const { return Get<bool>("beingSettled"); }                                 // :39 internal

    float preSettlingPrefabMass = 0.0f;                  // :44 protected
    ModuleGroundSciencePart* groundScienceModule = nullptr;  // :25 protected (set by ModuleGroundPart.OnStart)
    bool inConstructionMode = false;                     // :47 protected (EVA construction: WP-33)

    float MassForWeightTesting() const;                  // :55 internal

    virtual void OnDestroy();                            // :67
    virtual void OnPartPack() {}                         // :80
    virtual void OnPartUnpack();                         // :84
    void OnLoad(const ConfigNode& node) override;        // :96
    std::string GetInfo() override;                      // :164
    virtual std::string GetTooltip();                    // :336
    bool IsDeployedSciencePart();                        // :345
    virtual void EnableModule();                         // :354
    std::string GetModuleDisplayName() override;         // :396

    // MonoBehaviour.StartCoroutine / StopCoroutine(name) on the scheduler of ScienceEnvironment (unset: the process scheduler)
    std::shared_ptr<Coroutine> StartCoroutine(Enumerator e);
    void StopCoroutineRef(std::shared_ptr<Coroutine>& c);
    int CoroutineOwner() const { return static_cast<int>(reinterpret_cast<uintptr_t>(this) >> 4); }

protected:
    virtual Enumerator MakePartSettle(bool waitForMotionStop);  // :202
    virtual void PlayDeployAnimation() {}                       // :359
    std::shared_ptr<Coroutine> settleCoroutine_;
    Part* PartPtr() const;
    Vessel* VesselPtr() const;
};

}  // namespace ksp
