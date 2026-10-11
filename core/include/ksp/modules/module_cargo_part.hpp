// ModuleCargoPart.cs (WP-53 slice): the cargo-part base class of ModuleGroundPart (Breaking Ground deployable parts).
//
// WP-06 only needed its OnLoad (stackableQuantity rule, module_behaviours.cpp); WP-53 moves the class here and adds what the ground-part
// subclasses reach: GetInfo / GetTooltip, IsDeployedSciencePart, EnableModule, OnPartPack / OnPartUnpack, MakePartSettle, the coroutine
// helpers. The EVA-construction highlight, the welding sparks (AudioSource) and the compound-part link bookkeeping are UI / audio / WP-33
// (EVA construction) and are not ported here; WP-33 (src/inventory/inventory.cpp) adds them to this class.
#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ksp/core/coroutine.hpp"
#include "ksp/modules/part_module.hpp"

namespace ksp {

class Part;
class Vessel;
class ModuleGroundSciencePart;
class KerbalEVA;
class CompoundPart;

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

    // ---- WP-33 (EVA construction side of ModuleCargoPart.cs; src/inventory/inventory.cpp) ----
    bool reinitResourcesOnStoreInVessel() const { return Get<bool>("reinitResourcesOnStoreInVessel"); }
    bool UIHidden = false;
    bool constructionModeHighlight = false;
    float distanceToEngineer = 0.0f;
    std::vector<Part*> linkedCompoundParts;
    // construction highlight output (Part.SetHighlight*): last applied state (tests / the shell)
    std::function<void(Part&, bool)> setConstructionHighlight;  // unset: recorded in constructionHighlightApplied
    std::optional<bool> constructionHighlightApplied;
    bool weldSparksPlaying = false;                           // weldingSparkingsFX.isPlaying
    // ExpansionsLoader.IsExpansionInstalled("Serenity") override for the EVA-construction code; unset: installed
    static inline std::function<bool(const std::string&)> isExpansionInstalled;

    void OnAwake() override;                                  // :89 (WP-33: empty)
    void OnStart(StartState state) override;                  // :107
    void OnInitialize() override;                             // :158
    void OnUpdate() override;                                 // :279
    void OnPartWillDie(Part* p);                              // :183
    void HideUI();                                            // :194
    void ShowUI();                                            // :201
    virtual void OnWeldStart(KerbalEVA* kerbalEVA);           // :229
    virtual void OnWeldFinish(KerbalEVA* kerbalEVA);          // :239
    void OnEVAConstructionMode(bool opened);                  // :248
    void OnUpdateHighlight();                                 // :286
    void SetConstructionHighlight(bool state);                // :264
    void OnCompoundPartLinked(CompoundPart* linkedPart);      // :358
    void OnCompoundPartDetached(CompoundPart* linkedPart);    // :371
    bool HasLinkedCompoundParts() const;                      // :384
    void RemoveEventHandlers();

    // MonoBehaviour.StartCoroutine / StopCoroutine(name) on the scheduler of ScienceEnvironment (unset: the process scheduler)
    std::shared_ptr<Coroutine> StartCoroutine(Enumerator e);
    void StopCoroutineRef(std::shared_ptr<Coroutine>& c);
    int CoroutineOwner() const { return static_cast<int>(reinterpret_cast<uintptr_t>(this) >> 4); }

    virtual Enumerator MakePartSettle(bool waitForMotionStop);  // :202
    std::shared_ptr<Coroutine>& settleCoroutine() { return settleCoroutine_; }  // the running MakePartSettle coroutine (tests)

protected:
    virtual void PlayDeployAnimation() {}                       // :359
    std::shared_ptr<Coroutine> settleCoroutine_;
    Part* PartPtr() const;
    Vessel* VesselPtr() const;
};

}  // namespace ksp
