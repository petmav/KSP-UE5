// WP-16: ModuleDecouplerBase.cs, ModuleDecouple.cs, ModuleAnchoredDecoupler.cs (+ the IStageSeparator stand-ins of LaunchClamp and
// ModuleDockingNode, whose own packages are WP-25 / WP-26).
//
// The schema (KSPFields, KSPEvent / KSPAction attribute data) comes from the generated module_schema.inc / module_members.inc.
// Not ported (Unity / other packages): FXGroup.Burst and setActive (`fx` is only "the group exists"), the anchor Transform
// reparenting of ModuleAnchoredDecoupler, Part.AddForce / AddForceAtPosition (installed as `addForce` hooks and applied two frames
// later, as the coroutines do), AdjusterDecoupleBase (Missions adjusters: `IsAdjusterBlockingDecouple` is false),
// ModuleJettison.OnStop (the engine plate's `OnJettison` is a public method the jettison module calls), Part.AddShield /
// RemoveShield (only Part.ShieldedFromAirstream is set), the PAW title callback. Coroutines are `Delayed` entries advanced by
// TickAll() (one Unity frame).
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ksp/modules/part_module.hpp"
#include "ksp/staging/staging_types.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

class Part;
class Vessel;
struct AttachNode;

class ModuleDecouplerBase : public PartModule, public IModuleInfo, public IStageSeparator, public IStageSeparatorChild, public IAirstreamShield {
public:
    explicit ModuleDecouplerBase(const ModuleClassInfo& cls);
    ~ModuleDecouplerBase() override;  // OnDestroy (:122)

    // Part.AddForce(Vector3) / AddForceAtPosition(force, position): the physics engine's (WP-20)
    static inline std::function<void(Part&, const Vector3f&)> addForce;
    static inline std::function<void(Part&, const Vector3f&, const Vector3f&)> addForceAtPosition;
    // FlightGlobals.ForceSetActiveVessel(vessel) (WP-19); unset: nothing happens
    static inline std::function<void(Vessel*)> forceSetActiveVessel;
    // One Unity frame for every live decoupler: LateUpdate (refreshStaging) and the coroutines started by OnDecouple
    static void TickAll();

    AttachNode* ExplosiveNode() { return explosiveNode; }          // :48
    void SetExplosiveNode(AttachNode* n) { explosiveNode = n; }

    void DecoupleAction(KSPActionParam& param);                    // :61 [KSPAction]
    void Decouple();                                               // :67 [KSPEvent]
    void OnAwake() override;                                       // :72
    void OnStart(StartState state) override;                       // :81
    void OnActive() override;                                      // :107
    void LateUpdate();                                             // :111
    void OnVesselWasModified(Vessel* vsl);                         // :131
    void OnJettison(float value);                                  // :151 (ModuleJettison.OnStop)
    virtual void OnDecouple();                                     // :182

    // IModuleInfo
    std::string GetModuleTitle() override;                         // :190
    std::string GetInfo() override { return PartModule::GetInfo(); }  // IModuleInfo.GetInfo is PartModule.GetInfo in C#
    std::string GetPrimaryField() override;                        // :204
    IModuleInfo* AsModuleInfo() override { return this; }
    // IStageSeparator / IStageSeparatorChild
    int GetStageIndex(int fallback) override;                      // :209
    bool PartDetaches(std::vector<Part*>& decoupledParts) override;  // :218
    bool IsEnginePlate() override;                                 // :239
    // IAirstreamShield
    bool ClosedAndLocked() override { return shroudOn; }           // :244
    Vessel* GetVessel() override;                                  // :249
    Part* GetPart() override;                                      // :254
    // PartModule staging queries
    bool IsStageable() override;                                   // :259
    bool StagingEnabled() override;                                // :264
    bool StagingToggleEnabledEditor() override;                    // :273
    bool StagingToggleEnabledFlight() override;                    // :278
    std::string GetStagingEnableText() override;                   // :283
    std::string GetStagingDisableText() override;                  // :292

    bool hasFx() const { return fxFound; }

protected:
    struct Delayed {
        int ticks;
        std::function<void()> fn;
    };
    bool IsAdjusterBlockingDecouple() { return false; }  // :317 (no adjusters are modelled)
    void StartCoroutine(int ticks, std::function<void()> fn) { delayed_.push_back({ticks, std::move(fn)}); }
    void SetAirstream(bool enclosed);                              // :159
    void CheckShielded();                                          // :139
    Part* PartOf() const;

    bool fxFound = false;                 // :36 `fx` FXGroup (only whether the group exists)
    AttachNode* explosiveNode = nullptr;  // :38
    PartModule* jettisonModule = nullptr;  // :40
    bool shroudOn = false;                // :42
    bool refreshStaging = false;          // :44
    bool subscribed_ = false;
    std::vector<Delayed> delayed_;

    static inline std::vector<ModuleDecouplerBase*> live_;
};

class ModuleDecouple : public ModuleDecouplerBase {
public:
    // ModuleDecouple.cs:9 Jettisonning
    struct Jettisonning {
        Part* part;
        Vector3f ejectionVector;
    };
    explicit ModuleDecouple(const ModuleClassInfo& cls) : ModuleDecouplerBase(cls) {}
    void OnAwake() override;                    // :31
    void OnStart(StartState state) override;    // :41
    void OnActive() override;                   // :55
    void OnDecouple() override;                 // :63
    void WaitAndSwitchFocus(Part* p, Vessel* oldVessel);  // :124 (the coroutine body after its first yield)
    std::string GetInfo() override;             // :165
    std::string GetModuleDisplayName() override;  // :170
    // the (part, force) pairs of the last OnDecouple, applied by addForce two frames later (:143)
    const std::vector<Jettisonning>& LastEjections() const { return lastEjections_; }

private:
    std::vector<Jettisonning> lastEjections_;
};

class ModuleAnchoredDecoupler : public ModuleDecouplerBase {
public:
    explicit ModuleAnchoredDecoupler(const ModuleClassInfo& cls) : ModuleDecouplerBase(cls) {}
    void OnStart(StartState state) override;    // :18
    void OnActive() override;                   // :33
    void OnDecouple() override;                 // :41
    std::string GetInfo() override;             // :91
    std::string GetModuleDisplayName() override;  // :96

private:
    Part* otherPart = nullptr;  // :14
};

namespace detail {
void RegisterDecouplerModules();  // called by RegisterBuiltinModules
}

}  // namespace ksp
