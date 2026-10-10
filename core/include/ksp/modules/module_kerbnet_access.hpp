// WP-39: ModuleKerbNetAccess.cs (a part's access to KerbNet: field-of-view bounds, anomaly detection chance, the situations that "enhance" the scanner,
// the required crew effects and animation).
//
// Ported: the [KSPField] schema, OnAwake / OnLoad (DISPLAY_MODES, REQUIRED_EFFECTS), OnStart / OnDestroy subscriptions, CheckVesselSituationFoV,
// SituationIsEnhanced / GetEnhancedSituations, the IAccessKerbNet getters (GetKerbNetMinimumFoV / MaximumFoV / AnomalyChance / DisplayModes /
// ErrorState), the crew effect test (PartHasRequiredEffects over the part's crew), OnAnimationGroupStateChanged, GetInfo.
//
// Seams (KerbNetEnvironment): the KerbNetDialog (display, FoV bounds, error state refresh, "dialog open for this module"), ScreenMessages (recorded),
// MonoBehaviour.Invoke / IsInvoking / CancelInvoke of the 0.2 s crew-effect check (the pending flag is modelled, `RunInvoke()` is the timer
// firing). The KerbNet map scan itself (altitude limits, biome / anomaly sampling) is KerbNetDialog's (not part of this package's file list).
#pragma once

#include <functional>
#include <string>
#include <vector>

#include "ksp/events/game_events.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/physics/vessel_ranges.hpp"

namespace ksp {

class Vessel;
class Part;
class ModuleAnimationGroup;
class ModuleKerbNetAccess;

// KSP.UI.Dialogs.IAccessKerbNet
class IAccessKerbNet {
public:
    virtual ~IAccessKerbNet() = default;
    virtual Vessel* GetKerbNetVessel() = 0;
    virtual Part* GetKerbNetPart() = 0;
    virtual std::vector<std::string> GetKerbNetDisplayModes() = 0;
    virtual float GetKerbNetMinimumFoV() = 0;
    virtual float GetKerbNetMaximumFoV() = 0;
    virtual float GetKerbNetAnomalyChance() = 0;
    virtual std::string GetKerbNetErrorState() = 0;
};

struct KerbNetEnvironment {
    // KerbNetDialog.isDisplaying && KerbNetDialog.Instance.KerbNetAccessor == module. Unset: false.
    static inline std::function<bool(ModuleKerbNetAccess&)> hasThisDialogOpen;
    // KerbNetDialog.Instance.SetFoVBounds(min, max). Unset: nothing.
    static inline std::function<void(float min, float max)> setFoVBounds;
    // KerbNetDialog.Instance.RefreshErrorState(module) (Instance != null). Unset: nothing.
    static inline std::function<void(ModuleKerbNetAccess&)> refreshErrorState;
    // KerbNetDialog.Display(this) + ResetMapPosition. Unset: nothing.
    static inline std::function<void(ModuleKerbNetAccess&)> openDialog;
    // ScreenMessages.PostScreenMessage(text, 2.5, UPPER_CENTER): recorded
    static inline std::vector<std::string> screenMessages;
    static void Reset();
};

// ModuleKerbNetAccess.cs:9
class ModuleKerbNetAccess : public PartModule, public IAccessKerbNet, public IModuleInfo {
public:
    // ModuleKerbNetAccess.cs:12
    struct Modes {
        std::string Mode;
        std::string displayMode;
        Modes(std::string mode, std::string displaymode) : Mode(std::move(mode)), displayMode(std::move(displaymode)) {}
    };

    explicit ModuleKerbNetAccess(const ModuleClassInfo& cls) : PartModule(cls) {}
    ~ModuleKerbNetAccess() override;  // Unity destroys the component: OnDestroy's event unsubscription must not be skipped

    std::vector<Modes> modes;
    std::vector<std::string> effects;
    bool isCheckingEffects = false;
    bool partHasEffects = false;
    bool partIsAnimating = false;
    bool isEnhanced = false;

    float MinimumFoV() const { return Get<float>("MinimumFoV"); }
    float MaximumFoV() const { return Get<float>("MaximumFoV"); }
    uint32_t EnhancedSituationMask() const { return Get<uint32_t>("EnhancedSituationMask"); }
    float EnhancedMinimumFoV() const { return Get<float>("EnhancedMinimumFoV"); }
    float EnhancedMaximumFoV() const { return Get<float>("EnhancedMaximumFoV"); }
    float AnomalyDetection() const { return Get<float>("AnomalyDetection"); }
    bool RequiresAnimation() const { return Get<bool>("RequiresAnimation"); }

    void OnAwake() override;                         // :78
    void CopySerializedFrom(const PartModule& from) override;  // Unity Instantiate: the public lists / flags
    void OnLoad(const ConfigNode& node) override;    // :90
    void LoadDisplayModes(const ConfigNode& node);   // :96
    void LoadRequiredEffects(const ConfigNode& node);  // :126
    void OnStart(StartState state) override;         // :146
    void OnDestroy();                                // :163
    void OpenKerbNetAction();                        // :181 (the KSPAction body)
    void OpenKerbNet();                              // :190
    std::string GetModuleTitle() override { return "KerbNet Access"; }  // :196
    std::string GetModuleDisplayName() override;     // :201
    std::string GetInfo() override;                  // :206
    std::string GetPrimaryField() override { return std::string(); }  // :257 (null)
    IModuleInfo* AsModuleInfo() override { return this; }

    Vessel* GetKerbNetVessel() override;             // :262
    Part* GetKerbNetPart() override;                 // :267
    std::vector<std::string> GetKerbNetDisplayModes() override;  // :272
    float GetKerbNetMinimumFoV() override;           // :282
    float GetKerbNetMaximumFoV() override;           // :291
    float GetKerbNetAnomalyChance() override;        // :300
    std::string GetKerbNetErrorState() override;     // :305
    bool SituationIsEnhanced(VesselSituations situation) const;  // :318
    std::vector<VesselSituations> GetEnhancedSituations() const;  // :323
    void OnVesselSituationChanged(const GameEvents::HostedFromToAction<Vessel*, Vessel_Situations>& vs);  // :339
    bool HasThisDialogOpen();                        // :347
    void CheckVesselSituationFoV(VesselSituations situation, bool firstRun);  // :356
    void InvokeCrewEffectCheck(Vessel* v);           // :391
    void CheckRequiredEffects();                     // :403
    void OnAnimationGroupStateChanged(ModuleAnimationGroup* group, bool state);  // :446

    // MonoBehaviour.Invoke("CheckRequiredEffects", 0.2f) / IsInvoking / CancelInvoke
    bool IsInvoking() const { return invokePending_; }
    void CancelInvoke() { invokePending_ = false; }
    void RunInvoke();  // the 0.2 s timer fires

private:
    bool PartHasRequiredEffects();                   // :412
    Vessel* VesselPtr() const;
    bool invokePending_ = false;
    bool subscribed_ = false;
};

}  // namespace ksp
