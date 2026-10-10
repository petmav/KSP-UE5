// WP-27 deployable parts (numeric / data side): ModuleDeployablePart.cs, ModuleDeployableAntenna.cs, ModuleDeployableRadiator.cs,
// ModuleDeployableSolarPanel.cs.
//
// Unity's Animation / AnimationState become the time-driven model of ksp/unity/animation.hpp (the host ticks it); transforms, raycasts and
// FX are host hooks (DeployablesEnvironment, each documented). Everything the C# decides from numbers (the DeployState machine, the
// animation normalizedTime / speed arithmetic, shielding and break rules, drag cube weights, the solar panel flux chain) is ported.
//
// Lifecycle (explicit calls in Unity's order): AddComponent -> Awake -> Load (PartLoader), then per module OnStart(state), FixedUpdate()
// per physics frame, LateUpdate() per frame; the host ticks the Animation components (modelAnimations) once per frame.
#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ksp/aero/aero_interfaces.hpp"
#include "ksp/events/event_data.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/modules/resource_processing.hpp"
#include "ksp/unity/animation.hpp"
#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

class Part;
class Vessel;
class ModuleDeployablePart;

// What the deployable modules read from outside the part: the model (animations, transform existence), the tracking geometry, the raycasts,
// scene flags and the screen messages. The vessel comes through ResourceProcessingEnvironment::VesselOf (ProcessingVessel).
struct DeployablesEnvironment {
    // ---- scene flags (HighLogic.LoadedSceneIsEditor / IsFlight / IsMissionBuilder); unset: the SaveEnvironment scene for Editor / Flight, false
    static inline std::function<bool()> loadedSceneIsMissionBuilder;
    // ---- model: Part.FindModelComponents<Animation>() / GetComponentsInChildren<Animation>() of a module's part (FindModelAnimators(name)
    // filters this list by clip name). Unset: the part has no animation component.
    static inline std::function<std::vector<std::shared_ptr<Animation>>(PartModule&)> modelAnimations;
    // Part.FindModelTransform(name) != null (pivot, break and secondary transforms; the parachute canopy / cap). Unset: false.
    static inline std::function<bool(PartModule&, const std::string&)> hasModelTransform;
    // ---- tracking geometry (transforms are the engine's). One frame of the numbers CalculateTracking / PostCalculateTracking read:
    struct TrackingFrame {
        Vector3f trackingDirection;      // (trackingTransformLocal.position - panelRotationTransform.position).normalized
        float trackingDistance = 0.0f;   // (trackingTransformLocal.position - panelRotationTransform.position).magnitude
        Vector3f trackingDotForward;     // trackingDotTransform.forward (the pivot's or the secondary transform's)
        Vector3f partRight, partUp, partForward;  // part.partTransform.right / up / forward
        Vector3f pivotForward;           // panelRotationTransform.forward (or panelBreakTransform.forward when !hasPivot): ShouldBreakFromPressure
        float panelEulerY = 0.0f;        // panelRotationTransform.rotation.eulerAngles.y (read AFTER alignPanel ran)
    };
    static inline std::function<std::optional<TrackingFrame>(ModuleDeployablePart&)> trackingFrame;
    // The pivot alignment of CalculateTracking (Atan2 of the local tracking vector, Quaternion.Lerp(rotation, target, t)): the transform write.
    // `t` = TimeWarp.deltaTime * trackingSpeed, `offset` = TrackingAlignmentOffset.
    static inline std::function<void(ModuleDeployablePart&, float offset, float t)> alignPanel;
    // CalculateTrackingLOS' two raycasts (scaled space against the planet layer, then local space against the default layers). Returns
    // true when the line of sight is clear; `blocker` receives the blocking object's name when it is not. Unset: clear.
    static inline std::function<bool(ModuleDeployablePart&, const Vector3f& trackingDirection, std::string& blocker)> trackingRaycast;
    // The pivot transform writes of updateFSM / startFSM (`panelRotationTransform.localRotation = ...`): LerpToOriginal is
    // Quaternion.Lerp(localRotation, originalRotation, t), SetOriginal / SetCurrent assign originalRotation / currentRotation.
    enum class PanelRotationOp { LerpToOriginal, SetOriginal, SetCurrent };
    static inline std::function<void(ModuleDeployablePart&, PanelRotationOp, float t)> panelRotation;
    // panelRotationTransform.localRotation (read into currentRotation at the end of updateFSM); unset: currentRotation is kept
    static inline std::function<std::optional<Quaternionf>(ModuleDeployablePart&)> panelLocalRotation;
    // Planetarium.fetch.Sun / Home / CurrentMainBody (OnStart tracking body); unset: Sun = index 0 "Sun", Home = index 1 "Kerbin" (the stock
    // system), CURRENT = the vessel's main body. nullopt result = Planetarium.fetch is null.
    static inline std::function<std::optional<ProcessingBody>(int trackingMode)> planetariumBody;
    // GameSettings.PART_REPAIR_MASS_PER_KIT / PART_REPAIR_MAX_KIT_AMOUNT (GameSettings.cs:1028)
    static inline float partRepairMassPerKit = 0.05f;
    static inline int partRepairMaxKitAmount = 4;
    // FlightGlobals.getAltitudeAtPos(secondaryTransform.position, vessel.mainBody) (solar panel submersion). Unset: 0.
    static inline std::function<double(ModuleDeployablePart&)> secondaryTransformAltitude;
    // UIPartActionController.Instance.ItemListContains(part, false): the PAW is open. Unset: no UI.
    static inline std::function<bool(const Part*)> partActionUIContains;
    // The physical break (recurse + physicalObject.ConvertToPhysicalObject of every renderer under the break transform, with the
    // Random-driven velocities): the engine's. Called by breakPanels after the animation stop; the state changes stay in the module.
    static inline std::function<void(ModuleDeployablePart&)> convertBreakObjects;
    // ---- outputs
    static inline std::vector<std::string> screenMessages;   // ScreenMessages.PostScreenMessage texts (already localised)
    static inline int scheduleSetCollisionIgnoresCalls = 0;  // Part.ScheduleSetCollisionIgnores
    static inline int breakNotificationCalls = 0;            // breakNotifications (ScreenMessage + FlightLogger + log)
    static inline std::vector<std::string> flightLog;        // FlightLogger.fetch.LogEvent
    static inline bool noCrashDamage = false;                // CheatOptions.NoCrashDamage
    static void Reset();
    static bool SceneIsEditor();
    static bool SceneIsFlight();
    static bool SceneIsMissionBuilder() { return loadedSceneIsMissionBuilder ? loadedSceneIsMissionBuilder() : false; }
};

// ======================================================= ModuleDeployablePart ===================================================

// ModuleDeployablePart.cs:7
class ModuleDeployablePart : public PartModule, public IMultipleDragCube, public IScalarModule {
public:
    explicit ModuleDeployablePart(const ModuleClassInfo& cls);
    ~ModuleDeployablePart() override;

    enum class DeployState : int { RETRACTED, EXTENDED, RETRACTING, EXTENDING, BROKEN };  // :10
    enum class TrackingMode : int { SUN, HOME, CURRENT, VESSEL, NONE };                     // :19
    enum class PanelAlignType : int { PIVOT, X, Y, Z };                                      // :28

    // ---- non-KSPField state (ModuleDeployablePart.cs:143-219) ----
    bool hasPivot = false;                         // :149
    bool hasPanelRotationTransform = false;        // panelRotationTransform != null
    bool hasPanelBreakTransform = false;           // panelBreakTransform != null
    bool hasSecondaryTransform = false;            // secondaryTransform != null
    bool hasTrackingTransformLocal = false;        // trackingTransformLocal != null
    std::optional<ProcessingBody> trackingBody;    // CelestialBody trackingBody (null = nullopt)
    bool trackingVesselSet = false;                // trackingVessel != null
    std::string vesselID;                          // :155
    std::vector<std::string> destroyOnBreakObjects;  // :181
    std::shared_ptr<Animation> anim;               // :163 protected Animation anim
    bool stopAnimation = false;                    // :165
    bool playAnimationOnStart = false;             // :175 internal
    int repairKitsNecessary = 1;                   // :178
    bool trackingLOS = false;                      // :186
    std::string blockingObject;                    // :188
    bool overrideUIWriteState = true;              // :192
    bool bypassSetupAnimation = false;             // :99 private
    BaseEvent* eventRepairExternal = nullptr;      // :135 private
    std::vector<std::function<bool()>> stuckAdjusters;  // AdjusterDeployablePartBase.IsDeployablePartStuck (module adjusters: a list of predicates)
    // ---- events (IScalarModule)
    events::EventData<float, float>* OnMoving() override { return onMove.get(); }
    events::EventData<float>* OnStop() override { return onStop.get(); }
    std::unique_ptr<events::EventData<float, float>> onMove;  // :167
    std::unique_ptr<events::EventData<float>> onStop;         // :169

    // ---- typed accessors over the KSPFields ----
    DeployState deployState() const;
    void set_deployState(DeployState s);
    TrackingMode trackingMode() const;
    PanelAlignType alignType() const;
    std::string Str(std::string_view field) const;  // a string KSPField (null -> "")
    bool useAnimation() const { return Get<bool>("useAnimation"); }

    // ---- IScalarModule (:222-281)
    std::string ScalarModuleID() const override;  // :220 moduleID
    float GetScalar() const override;             // :222 (virtual property getter)
    bool CanMove() const override;                // :251 (virtual property getter)
    void SetScalar(float t) override;             // :1216
    void SetUIRead(bool state) override;          // :1231
    void SetUIWrite(bool state) override;         // :1239
    bool IsMoving() override;                     // :1249
    IScalarModule* AsScalarModule() override { return this; }
    virtual float MinAoAForQCheck() const { return 0.1875f; }  // :218

    // ---- IMultipleDragCube (:1234-1306)
    bool IsMultipleCubesActive() const override;
    std::vector<std::string> GetDragCubeNames() const override;  // C# returns null (no animation, not tracking): the empty vector
    void AssumeDragCubePosition(const std::string& name) override;
    bool UsesProceduralDragCubes() const override { return false; }

    // ---- KSPActions / KSPEvents (:286-440)
    void OnAwake() override;                       // :542 (+ binds the [KSPEvent] / [KSPAction] handlers)
    void OnStart(StartState state) override;       // :566
    void OnLoad(const ConfigNode& node) override;  // :732
    void OnSave(ConfigNode& node) override;        // :759
    std::string GetInfo() override;                // :710
    std::string GetModuleDisplayName() override;   // :1384
    virtual void ExtendPanelsAction(KSPActionParam& param);  // :292
    void ControlPanelsWithoutUsingSymmetry(KSPActionType action);  // :304
    virtual void ExtendAction(KSPActionParam& param);   // :317
    virtual void RetractAction(KSPActionParam& param);  // :323
    virtual void Extend();                         // :329 [KSPEvent]
    virtual void Retract();                        // :371 [KSPEvent]
    virtual void DoExtend();                       // :347 (protected)
    virtual void DoRetract();                      // :389 (protected)
    void EventRepairExternal();                    // :407 [KSPEvent]: the EVA kit inventory is the host's (`repairKitAvailable`)
    void CheatRepair();                            // :443
    virtual bool DoRepair();                       // :448 (the prefab clone + transform re-parenting is the engine's: `repairTransforms`)
    // EVA repair inputs (KerbalExperience / inventory): see DoRepair; unset: the repair succeeds
    std::function<bool(ModuleDeployablePart&)> repairTransforms;

    // ---- Unity messages
    virtual void FixedUpdate();                    // :810
    virtual void LateUpdate();                     // :838
    virtual void OnCollisionEnter(float relativeVelocityMagnitude);  // :854 (Collision.relativeVelocity.magnitude)
    virtual void startFSM();                       // :867
    virtual void breakPanels();                    // :963
    virtual void breakNotifications(const std::string& partTitle, const std::string& breakMessage);  // :1007
    virtual void PostFSMUpdate() {}                // :1017
    virtual void updateFSM();                      // :1021
    virtual bool ShouldBreakFromPressure();        // :1112
    virtual bool ShouldBreakFromG();               // :1136
    virtual void PostCalculateTracking(bool trackingLOS, const Vector3f& trackingDirection) { (void)trackingLOS; (void)trackingDirection; }  // :1141
    virtual bool CalculateTrackingLOS(const Vector3f& trackingDirection, std::string& blocker);  // :1145
    virtual void CalculateTracking();              // :1155
    virtual void GetTrackingBodyTransforms();      // :1200
    void FindAnimations();                         // :681 (private in C#)
    void SetDragCubes(float retracted, float angle);  // :1335 (private in C#)
    bool IsDeployablePartStuck() const;            // :1365
    bool CanBeDetached() const { return AllowConstructionDeployState(); }   // :1389 (IConstruction)
    bool CanBeOffset() const { return AllowConstructionDeployState(); }     // :1394
    bool CanBeRotated() const { return AllowConstructionDeployState(); }    // :1399
    virtual bool AllowConstructionDeployState() const;  // :1404
    virtual void OnInventoryModeDisable();         // :771 (the ProtoPartModuleSnapshot value write is the host's)
    void OnStoredInInventory();                    // :801

    // the tracking frame of the last CalculateTracking (testing / host read)
    std::optional<DeployablesEnvironment::TrackingFrame> lastFrame;

protected:
    Part* PartPtr() const;
    bool ShieldedFromAirstream() const;            // part.ShieldedFromAirstream (false without a Part)
    AnimationState& AnimState() const;             // anim[animationName] (NullReferenceException when absent, like C#)
    void onVesselFocusChange(Vessel* v);           // :526 onVesselFocusChange
    const ProcessingVessel* VesselRecord() const;
    std::string cache(const char* autoLoc) const;  // the CacheLocalStrings statics: Localizer.Format at use
    void BindHandlers();
};

// ModuleDeployableAntenna.cs:5
class ModuleDeployableAntenna : public ModuleDeployablePart {
public:
    using ModuleDeployablePart::ModuleDeployablePart;
    bool CanMove() const override;                        // :5
    float MinAoAForQCheck() const override { return 0.5f; }  // :21
    void OnLoad(const ConfigNode& node) override;         // :23
    void Extend() override;                               // :30
    std::string GetModuleDisplayName() override;          // :55
};

// ModuleDeployableRadiator.cs:5
class ModuleDeployableRadiator : public ModuleDeployablePart {
public:
    using ModuleDeployablePart::ModuleDeployablePart;
    void OnLoad(const ConfigNode& node) override;         // :9
    std::string GetModuleDisplayName() override;          // :27
};

// ModuleDeployableSolarPanel.cs:8
class ModuleDeployableSolarPanel : public ModuleDeployablePart {
public:
    using ModuleDeployablePart::ModuleDeployablePart;
    enum class PanelType : int { FLAT, CYLINDRICAL, SPHERICAL };  // :10

    double _flowRate = 0.0;      // :44 public double
    double _distMult = 0.0;      // :80
    double _efficMult = 0.0;     // :82
    float originalEfficiencyMultiplier = 1.0f;  // :71 private
    bool hasTrackingDotTransform = false;       // trackingDotTransform != null
    std::vector<std::function<float(float)>> efficiencyAdjusters;  // AdjusterDeployableSolarPanelBase.ApplyEfficiencyAdjustment chain

    ~ModuleDeployableSolarPanel() override;
    PanelType panelType() const;
    void OnAwake() override;                       // :97
    void OnStart(StartState state) override;       // :129
    std::string GetInfo() override;                // :164
    void OnLoad(const ConfigNode& node) override;  // :173
    void FixedUpdate() override;                   // :197
    void PostFSMUpdate() override;                 // :210
    bool CalculateTrackingLOS(const Vector3f& trackingDirection, std::string& blocker) override;  // :219
    void PostCalculateTracking(bool trackingLOS, const Vector3f& trackingDirection) override;     // :267
    std::string GetModuleDisplayName() override;   // :361
    void OnPartRepaired(Part* p);                  // :346
    float ApplyEfficiencyAdjustments(float efficiency);  // :382
    std::string GetContractObjectiveType() const { return "Generator"; }  // :336
    bool CheckContractObjectiveValidity() const { return true; }          // :341
    void OnInventoryModeDisable() override;             // :388 (the snapshot write is the host's)

private:
    bool repairedSubscribed_ = false;
};

namespace detail {
void RegisterDeployableModules();  // deployables.cpp
}

}  // namespace ksp
