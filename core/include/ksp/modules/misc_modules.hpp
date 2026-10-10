// WP-30: the miscellaneous part modules ModuleGPS.cs, FlagSite.cs, ModulePartFirework.cs.
//
// The schema (KSPFields, KSPEvent / KSPAction attribute data) comes from the generated module_schema.inc / module_members.inc, so every
// [KSPField] stays in the module's typed field store (Get<T>/Set<T>) and the part-database gate sees the same values as with the generic
// PartModule. The Unity messages (Start, Update, FixedUpdate, LateUpdate, OnDestroy) are public member functions the host calls.
//
// Documented seams (the engine's, not simulated): ModuleGPS: UIPartActionController.ItemListContains (`ModuleGPSEnvironment::itemListContains`),
// the vessel record comes from ResourceProcessingEnvironment::VesselOf, the biome from ResourceProcessingEnvironment::biomeAt.
// FlagSite: the Animation / Collider / GameObject / ConfigurableJoint / Physics calls and the PopupDialogs go through `FlagSiteEnvironment`
// (every hook unset = the object is absent), the Invoke / coroutine timers are recorded as `pendingInvokes` / `jointBreakPending` for the
// host to fire; FlagSite.CreateFlag (Object.Instantiate of the "flag" prefab) and ScanSurroundingTerrain (Physics.Raycast) are the engine's.
// ModulePartFirework: the PAW color pickers / FireworkFX particle systems / AudioFX / physicalObject rigidbody are the engine's; the shell's
// numbers (rigidbody mass, drag, forces, recoil, FireworkFX.Setup arguments) are computed here and handed to
// `FireworkEnvironment::launchShell` as a `FireworkShellLaunch`.
#pragma once

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "ksp/core/delegate.hpp"
#include "ksp/fsm/kerbal_fsm.hpp"
#include "ksp/math/vector3d.hpp"
#include "ksp/modules/animated_module.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/unity/color.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

class Part;
class Vessel;
class FlagSite;
enum class GameScenes : int;

// ================================================================ ModuleGPS

struct ModuleGPSEnvironment {
    // UIPartActionController.Instance != null && Instance.ItemListContains(part, includeSymmetryCounterparts: false). Unset: no PAW is open.
    static inline std::function<bool(Part&)> itemListContains;
    static inline std::vector<std::string> printed;  // MonoBehaviour.print texts, in order
    static void Reset();
};

// ModuleGPS.cs:5
class ModuleGPS : public PartModule, public IAnimatedModule {
public:
    using PartModule::PartModule;
    IAnimatedModule* AsAnimatedModule() override { return this; }

    bool isConnectedToAsteroid = false;  // :19
    bool isConnectedToComet = false;     // :21

    // ModuleGPS.cs:37 / :61 (the vessel's parts through the processing record of the module's part)
    virtual bool CheckForAsteroidConnect();
    virtual bool CheckForCometConnect();
    virtual void Update();  // :85 (Unity message)
    void EnableModule() override;           // :139
    void DisableModule() override;          // :144
    bool ModuleIsActive() override;         // :149
    bool IsSituationValid() override;       // :154
    std::string GetModuleDisplayName() override;  // :163

    float secondsSinceLastDisplayUpdate = 0.0f;  // :23
    float secondsUpdateRate = 0.1f;              // :25
    static void CacheLocalStrings();             // :168: the cacheAutoLOC_* statics are taken with Localizer.Format at use here
};

// ================================================================ FlagSite

struct FlagSiteEnvironment {
    // ---- the prefab's serialized references (animationRoot / flagDeployAnimation / colliders / visibilityNodes / groundPivot) ----
    static inline bool hasAnimationRoot = true;          // (bool)animationRoot
    static inline double flagDeployAnimationLength = 3.0;  // flagDeployAnimation.length
    // ---- animation / colliders / visibility (every call is the engine's) ----
    static inline std::function<void(FlagSite&)> playDeployAnimation;      // animationRoot.Play(flagDeployAnimation.name)
    static inline std::function<void(FlagSite&)> showDeployedPose;         // Stop(); [clip].normalizedTime = 1; weight = 1; enabled = true
    static inline std::function<void(FlagSite&, bool)> setCollidersEnabled;  // colliders[i].enabled = value, every collider
    static inline std::function<void(FlagSite&, bool)> setVisible;        // visibilityNodes[i].SetActive(value)
    // ---- joint: SetJoint's Physics.OverlapSphere / AddComponent<ConfigurableJoint> (true = a joint was created), UnsetJoint's Destroy ----
    static inline std::function<bool(FlagSite&)> createJoint;
    static inline std::function<void(FlagSite&)> destroyJoint;
    static inline std::function<void(FlagSite&, float breakForce, float breakTorque)> setJointBreakForce;  // joint.breakForce / breakTorque
    // ---- EVA: FlightGlobals.ActiveVessel.GetComponent<KerbalEVA>() != null && !isRagdoll, then KerbalEVA.AddFlag() ----
    static inline std::function<bool()> evaAddFlag;  // returns whether the EVA kerbal exists and is not a ragdoll (AddFlag was called)
    // ---- dialogs (PopupDialog.SpawnPopupDialog / FlagPlaqueDialog.Spawn / Terminate) ----
    static inline std::function<void(FlagSite&)> spawnSiteRenameDialog;
    static inline std::function<void(FlagSite&)> dismissSiteRenameDialog;  // SiteRenameDialog.Dismiss()
    static inline std::function<void(FlagSite&, const std::string& vesselName, const std::string& plaque)> spawnPlaqueDialog;
    static inline std::function<void(FlagSite&)> terminatePlaqueDialog;    // FlagPlaqueDialog.Terminate()
    // ---- InputLockManager (lockID -> ControlTypes description) ----
    static inline std::map<std::string, std::string> controlLocks;
    static void Reset();
};

// FlagSite.cs:5
class FlagSite : public PartModule {
public:
    explicit FlagSite(const ModuleClassInfo& cls) : PartModule(cls) {}
    ~FlagSite() override;

    void OnAwake() override;  // binds the [KSPEvent] handlers (TakeDown, PickUp, ReadPlaque)
    void Start();                              // :67 (Unity message)
    void OnDestroy();                          // :79
    void onGameSceneLoadRequested(GameScenes scene);  // :84
    void OnLoad(const ConfigNode& node) override;     // :97
    void Update();                             // :105
    void FixedUpdate();                        // :113
    void LateUpdate();                         // :121
    void OnSave(ConfigNode& node) override;    // :129
    void SetupFSM();                           // :137
    void MakeVisible();                        // :252 (also the Invoke("MakeVisible") target)
    void MakeInvisible();                      // :261
    void TakeDown();                           // :271 [KSPEvent]
    void PickUp();                             // :277 [KSPEvent]
    void SetJoint();                           // :287
    void UnsetJoint();                         // :357
    void OnVesselPack();                       // :362
    void OnVesselUnpack();                     // :374
    void OnPlacementFail();                    // :382
    void OnPlacementComplete();                // :387
    void OnJointBreak(float breakForce);       // :400
    void RenameSite();                         // :457
    void RenameSite(std::function<void()> afterDialog);  // :462
    void AcceptSiteRename();                   // :493
    void CancelSiteRename();                   // :503
    void DismissSiteRename();                  // :510
    void ReadPlaque();                         // :518 [KSPEvent]
    void onFlagPlaqueDialogDismiss();          // :524
    // the dialog's button handlers (the DialogGUIButton lambdas of RenameSite :475 / :482) and text-input callbacks (:467 / :471)
    void SiteRenameInput(const std::string& n) { siteName = n; }
    void PlaqueInput(const std::string& n) { newPlaqueText = n; }
    bool SiteRenameAcceptEnabled() const;      // () => Vessel.IsValidVesselName(siteName)
    void SiteRenameAcceptPressed();
    void SiteRenameDismissPressed();
    // the delayed callback body of SetJoint's coroutine (:312-:319), fired by the host after unbreakablePeriodLength
    void ApplyJointBreakForce();

    // ---- private state of the C# class ----
    bool joint = false;                       // :7 (ConfigurableJoint != null)
    KerbalFSM* Fsm() { return fsm.get(); }
    KFSMState *st_placing = nullptr, *st_placed = nullptr, *st_retracting = nullptr, *st_toppled = nullptr;
    KFSMEvent *onPlacementFailed = nullptr, *onTopple = nullptr, *onRetractStart = nullptr;
    KFSMTimedEvent *onPlaceComplete = nullptr, *onRetractComplete = nullptr;
    std::optional<std::string> stateName;     // :52 (null until OnLoad reads `state`)
    std::string siteName, newPlaqueText;      // :58 / :60
    bool siteRenameDialogOpen = false;        // SiteRenameDialog != null
    bool flagPlaqueDialogOpen = false;        // FlagPlaqueDialog != null
    bool jointBreakPending = false;           // the DelayedCallback coroutine started by SetJoint has not fired yet
    std::vector<std::pair<std::string, float>> pendingInvokes;  // Invoke(method, delay), in order
    std::string PlaqueText() const { return Get<std::string>("PlaqueText"); }

private:
    std::unique_ptr<KerbalFSM> fsm;
    // owned FSM objects (the C# fields hold the references)
    std::vector<std::unique_ptr<KFSMState>> ownedStates_;
    std::vector<std::unique_ptr<KFSMEvent>> ownedEvents_;
    bool subscribed_ = false;
    bool bound_ = false;
    std::function<void()> afterDialog_;  // the `afterDialog` closure variable of the RenameSite call that opened the dialog
    Part* PartOf() const;
};

// ================================================================ ModulePartFirework

// KSP/FX/Fireworks/FireworkEffectType.cs
enum class FireworkEffectType : int { TRAIL = 0, BURST = 1 };

// KSP/FX/Fireworks/FireworkFXDefinition.cs (the fields the module reads; the definition list is DatabaseGameObject.fwFXList.fireworkFX)
struct FireworkFXDefinition {
    std::string name = "bareTrail";
    FireworkEffectType fwType = FireworkEffectType::TRAIL;
    std::string prefabName = "fx_fwTrail1", displayName = "Bare", color1Name = "Color 1", color2Name = "Color 2", color3Name = "none",
                crackleSFX = "none";
    bool randomizeBurstOrientation = false;
    float minTrailLifetime = -1.0f, maxTrailLifetime = -1.0f;
};

// What LaunchShell hands the engine (Instantiate(shellPrefab), physicalObject.ConvertToPhysicalObject, Rigidbody setup, AddForce,
// FireworkFX.Setup): the values the C# computes.
struct FireworkShellLaunch {
    Vector3f position;                   // cannon transform position (zero when the child is missing)
    float rbMass = 0.0f;                 // rb.mass = shellMass * shellRBMassScaleValue
    float rbDrag = 0.0f;                 // rb.drag = shellDrag
    float maxDistance = 10000.0f;        // physicalObject.maxDistance
    float origDrag = 0.0f;               // physicalObject.origDrag
    Vector3f shellForceDir;              // :509-:513
    Vector3f force;                      // :516 rb.AddForce(force, ForceMode.Force)
    std::optional<Vector3f> vesselVelocityForce;  // :518-:521 (second AddForce, only when the Krakensbane frame velocity is zero)
    Vector3f recoil;                     // :522 part.AddForce
    bool soundFX = false;                // :497 configureSoundFX ran (inside the atmosphere)
    // FireworkFX.Setup(...) arguments (:524)
    float shellDuration = 0.0f, shellVelocity = 0.0f, burstSpread = 0.0f, burstDuration = 0.0f, burstFlareSize = 0.0f;
    std::string trailPrefab, burstPrefab;  // getFXFromPrefab names ("Effects/Fireworks/" + prefabName), empty = null GameObject
    Color burstColor1, burstColor2, trailColor1, burstColor3, trailColor2;
    std::string crackleSFX;
    bool randomizeBurstOrientation = false;
    float minTrailLifetime = -1.0f, maxTrailLifetime = -1.0f;
};

struct FireworkEnvironment {
    // AssetBase.GetPrefab(shellName) != null. Unset: the prefab exists.
    static inline std::function<bool(const std::string&)> shellPrefabExists;
    // DatabaseGameObject.fwFXList.fireworkFX.Values, in dictionary order
    static inline std::vector<FireworkFXDefinition> fxDefinitions;
    // base.gameObject.GetChild(cannonName).transform: position and up; nullopt = no such child (NullReferenceException). Unset: a cannon at the origin pointing up
    struct Cannon {
        Vector3f position, up;
    };
    static inline std::function<std::optional<Cannon>(PartModule&, const std::string&)> cannon;
    // FlightGlobals.ActiveVessel.orbit.referenceBody.atmosphere && altitude < atmosphereDepth. Unset: false.
    static inline std::function<bool()> activeVesselInAtmosphere;
    static inline std::function<void(PartModule&)> configureSoundFX;  // :527 (AudioFX components: the engine's)
    // Krakensbane.GetFrameVelocity().magnitude and base.vessel.rb_velocityD. Unset: 0 / zero.
    static inline std::function<double()> krakensbaneFrameSpeed;
    static inline std::function<Vector3d(PartModule&)> vesselRbVelocityD;
    // the instantiation + rigidbody + FireworkFX.Setup
    static inline std::function<void(PartModule&, const FireworkShellLaunch&)> launchShell;
    // UI_ColorPicker side effects of OnColorChanged / setAllColors / configureColorFields (UIPartActionColorPicker.colorPicker.SetColor ...)
    static inline std::function<void(PartModule&, const std::string& picker, const Color&)> setPickerColor;
    static inline std::function<void(PartModule&, bool visible)> setTertiaryColorVisible;  // tertiaryBurstColorChanger.SetSceneVisibility
    static inline std::vector<Color> presetColors;  // GameSettings.fireworkPresetColors
    static void Reset();
};

// ModulePartFirework.cs:9
class ModulePartFirework : public PartModule, public IPartMassModifier {
public:
    explicit ModulePartFirework(const ModuleClassInfo& cls) : PartModule(cls) {}
    IPartMassModifier* AsMassModifier() override { return this; }

    // state (:85-:120)
    bool shellPrefabPresent = false;           // shellPrefab != null
    Vector3f shellForceDir = Vector3f(0.0f, 0.0f, 1.0f);  // :95 Vector3.forward
    std::vector<FireworkFXDefinition> availableFX;        // :97
    bool availableFXPresent = false;                       // availableFX != null
    // UI_ChooseOption.options / .display of the trailType / burstType controls and UI_FloatRange.maxValue of fireworkShots (:99-:103)
    bool trailChooseOptionPresent = false, burstChooseOptionPresent = false, fireworkShotsFloatRangePresent = false, tertiaryFieldPresent = false;
    std::vector<std::string> trailOptions, trailDisplay, burstOptions, burstDisplay;
    float fireworkShotsRangeMax = 32.0f;
    std::vector<Color> fireworkColors;         // :108 (5 entries: burst 1..3, trail 1..2)
    std::vector<std::string> fieldModifiedLog; // names of the fields whose OnValueModified handler ran (test aid)

    void OnAwake() override;                    // :167
    void OnSave(ConfigNode& node) override;     // :230
    void OnLoad(const ConfigNode& node) override;  // :250
    void OnStart(StartState state) override;    // :261
    void OnDestroy();                           // :306
    std::vector<Color> PresetColors();          // :312
    void OnColorChanged(const Color& color, const std::string& pickerName);  // :317
    void LaunchShellAction(KSPActionParam& param);  // :384 [KSPAction]
    void LaunchShellEvent();                    // :390 [KSPEvent]
    void LaunchShell();                         // :473
    Color GetCurrentColor(const std::string& fieldName);  // :750
    float GetModuleMass(float defaultMass, ModifierStagingSituation sit) override;  // :769
    ModifierChangeWhen GetModuleMassChangeWhen() override { return ModifierChangeWhen::CONSTANTLY; }  // :774

    const FireworkFXDefinition* getCurrentFXByType(FireworkEffectType type);  // :589
    std::string getCurrentPSByType(FireworkEffectType type);                 // :559 (the prefab name; empty = null)
    void updateFireworkShotLabel();             // :624
    void configureColorFields();                // :639
    void setAllColors(const Color& colB1, const Color& colB2, const Color& colB3, const Color& colT1, const Color& colT2);  // :711
    void updatePickerColor(FireworkEffectType type, int number);  // :430 (with the pickers the host reports through pickerIds)
    void UpdateFireworksColors();               // :420
    void OnPAWInit(Part* p);                    // :411 (the persistentId compare is the caller's)
    // the picker ids the PAW reports (UIPartActionColorPicker.id) per field; empty = no picker open (matchColorPickers :697)
    std::map<std::string, std::string> pickerIds;

private:
    Part* PartOf() const;
    bool bound_ = false;
};

namespace detail {
void RegisterMiscModules();  // misc_modules.cpp: ModuleGPS, FlagSite, ModulePartFirework
}

}  // namespace ksp
