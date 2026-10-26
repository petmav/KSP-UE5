// WP-29b: the KSP wheel part modules (Assembly-CSharp/ModuleWheelBase.cs, ModuleWheels/*.cs, SuspensionLoadBalancer.cs, WheelType.cs).
//
// Ported 1:1 from the decompiled C#: ModuleWheelBase (gee bias and friction, set-up of the KSPWheelController, FixedUpdate slip displacement,
// landing detection -> GroundContacts / landedAt, the rigid-body schloompa freeze, drift fix, OnPutToGround, docking / SOI / pack handlers,
// events and actions), the submodule host (ModuleWheelSubmodule, WheelSubsystem(s), WheelUtil), ModuleWheelMotor / MotorSteering (torque curve,
// torque rounded to 0.01, traction control, electric charge draw), ModuleWheelSteering (coordinate frame, Ackermann turn centre, steering
// curves), ModuleWheelBrakes, ModuleWheelLock, ModuleWheelSuspension (auto spring / damper, boost, high-gee override), ModuleWheelDamage (stress
// and impact model, repair), ModuleWheelDeployment (the KerbalFSM over the Unity Animation, TsubSys sub-system switch), ModuleWheelBogey and the
// SuspensionLoadBalancer vessel module. Persistent fields, defaults and cfg loading are the generic module schema (ksp/parts/module_schema.inc,
// WP-05 / WP-06): every KSPField is read and written with Get<T> / Set<T>.
//
// The M1 quirk (WHEEL-SPIKE.md Q3) is kept exactly: the wheel collider radius is `radius * part.rescaleFactor` (ModuleWheelBase.cs:333) while the
// motor torque curve, maxRpm and the slip displacement use the cfg `radius` (ModuleWheelMotor.cs:142, ModuleWheelBase.cs:430, :453).
//
// Float policy: MonoFloat (the game): expressions in double, rounded where the C# stores / passes / returns / casts them.
//
// Native seams (WheelModuleEnvironment): the engine objects the modules touch that are neither C# nor in the KSPWheelController port: the model
// hierarchy lookups (FindModelTransform / FindModelAnimator), the collider a wheel stands on (layer, tag, owning part), Debug / ScreenMessages /
// FlightLogger / explode / control locks / collision-ignore calls (recorded through `trace`), coroutines (CoroutineScheduler), the PSystemManager
// maximum surface gravity, Time.timeSinceLevelLoad. The WheelCollider ray and the PhysX suspension law stay WP-29c (wheel_native.hpp).
//
// Not ported: GetInfo / GetModuleDisplayName / OnGatherInfo text (the PAW part-info panel: PartModuleUtil.PrintResourceRequirements and the float
// "0.0#" formats are not modelled, as in ModuleResourceHandler), UI_FloatRange slider limits (SetSteeringTweakablesSliders), the
// UIPartActionController hooks, the Debug.DrawRay gizmos, CacheLocalStrings (texts are looked up where used), VPWheelCollider.AdjustToWheelMesh
// (FitWheelColliderToMesh: no stock part sets it), GetDragCubeNames / AssumeDragCubePosition (drag-cube generation).
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ksp/core/coroutine.hpp"
#include "ksp/fsm/kerbal_fsm.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/modules/base_event.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/parts/part_types.hpp"
#include "ksp/unity/animation.hpp"
#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"
#include "ksp/wheels/vehicle_physics.hpp"
#include "ksp/wheels/wheel_native.hpp"

namespace ksp {

class Part;
class Vessel;
class PartModule;
struct KSPActionParam;
enum class KSPActionType : int;
class CelestialBody;
class ModuleInventoryPart;
struct PartHeightQuery;

// WheelType.cs
enum class WheelType : int { FREE = 0, MOTORIZED = 1, LEG = 2 };

// UnityEngine.Collider as ModuleWheelBase.LandingDetectionUpdate reads it (layer, tag, the part it belongs to, CrashObjectName upwards)
struct WheelColliderInfo {
    int layer = 0;
    std::string tag;                 // gameObject.tag ("Untagged" by default in Unity)
    Part* part = nullptr;            // FlightGlobals.GetPartUpwardsCached(gameObject)
    bool hasCrashObjectName = false; // Part.GetComponentUpwards<CrashObjectName>(gameObject) != null
    bool rocTerrain = false;         // ROCManager.Instance != null (the ROC branch): GetTerrainTag below applies when tag == "ROC"
    std::string rocTerrainTag;       // ROCManager.Instance.GetTerrainTag(...)
};

// the light of a ModuleStatusLight / the flare of a ModuleLight (not ported as classes: only the calls the wheel modules make)
class IWheelStatusLight {
public:
    virtual ~IWheelStatusLight() = default;
    virtual void SetStatus(bool status) = 0;
    virtual bool IsOn() const = 0;
};

// Expansions.Missions.Adjusters.AdjusterWheelBrakesBase / AdjusterWheelMotorBase (the member the wheel modules call)
class IWheelTorqueAdjuster {
public:
    virtual ~IWheelTorqueAdjuster() = default;
    virtual float ApplyTorqueAdjustment(float torque) = 0;
};

namespace ModuleWheels {

// ModuleWheels/WheelSubsystem.cs
class WheelSubsystem {
public:
    enum SystemTypes : int { None = 0, Tire = 1, Suspension = 2, Steering = 4, Motor = 8, Brakes = 16, WheelCollider = 32, Bogey = 64, All = -1, Any = -1 };
    std::string reason;
    int type;
    PartModule* owner;
    WheelSubsystem(std::string reason_, int type_, PartModule* owner_) : reason(std::move(reason_)), type(type_), owner(owner_) {}
    bool IsType(int t) const { return (t & type) != 0; }   // :32
};

// ModuleWheels/WheelSubsystems.cs. `OnModified` is a multicast delegate: (owner, callback) entries, invoked in order.
class WheelSubsystems {
public:
    using Callback = std::function<void(WheelSubsystems*)>;
    WheelSubsystems() = default;
    void AddSubsystem(WheelSubsystem* d);     // :22 (List.AddUnique)
    void RemoveSubsystem(WheelSubsystem* d);  // :29
    bool HasType(int t) const { return (types_ & t) != 0; }   // :46
    bool Contains(const WheelSubsystem* s) const;             // :51
    void GetReasons(std::string& o, const std::string& separator = ", ") const;  // :56
    int Types() const { return types_; }
    // OnModified = x (assignment) / Delegate.Combine / Delegate.Remove
    void SetOnModified(const void* owner, Callback cb);
    void CombineOnModified(const void* owner, Callback cb);
    void RemoveOnModified(const void* owner);
    bool HasOnModified() const { return !onModified_.empty(); }

private:
    void ResetFlags();
    void Fire();
    std::vector<WheelSubsystem*> subs_;
    int types_ = 0;
    std::vector<std::pair<const void*, Callback>> onModified_;
};

// ModuleWheels/WheelUtil.cs
struct WheelUtil {
    static float RpmToDegsSec(float rpm);                    // :7
    static float RpmToSpeed(float rpm, float wheelRadius);   // :12
    static float SpeedToRpm(float speed, float wheelRadius); // :17
};

class ModuleWheelSubmodule;

}  // namespace ModuleWheels

// What the wheel modules reach outside the part (see the file header).
struct WheelModuleEnvironment {
    static inline CoroutineScheduler* scheduler = nullptr;   // MonoBehaviour.StartCoroutine host
    // PSystemManager.Instance.MaximumSurfaceGeeASL (PSystemManager.cs:239-245: max of the local bodies' GeeASL)
    static inline std::function<double()> maximumSurfaceGeeASL;
    // Time.timeSinceLevelLoad (unset: 0)
    static inline std::function<float()> timeSinceLevelLoad;
    // Debug / ScreenMessages / FlightLogger / Part.explode / InputLockManager / CollisionManager / FX / drag-cube weights ... in call order
    static inline std::function<void(const std::string&)> trace;
    // the collider behind a WheelHit (layer, tag, owning part ...); unset: layer 0, "Untagged", no part
    static inline std::function<WheelColliderInfo(const VehiclePhysics::Collider*)> describeCollider;
    // Part.FindModelTransform(name): the transform of the part model with that name, nullptr when absent
    static inline std::function<VehiclePhysics::NativeTransform*(Part&, const std::string&)> findModelTransform;
    // Part.FindModelAnimator(transformName, stateName)
    static inline std::function<Animation*(Part&, const std::string& animationTrfName, const std::string& stateName)> findModelAnimator;
    // Animation.Sample(): applies the animation state to the model transforms (FindTransformAnimationTime samples at every candidate time)
    static inline std::function<void(Animation&)> sampleAnimation;
    // Part.Modules[index] as ModuleStatusLight / FindModuleImplementing<ModuleLight>()
    static inline std::function<IWheelStatusLight*(PartModule&)> asStatusLight;
    static inline std::function<IWheelStatusLight*(Part&)> findLightModule;
    // suspensionCollider.enabled = true; part.ScheduleSetCollisionIgnores() (ResetSuspensionCollider's delayed callback)
    static inline std::function<void(Part&)> suspensionColliderEnable;
    // WP-29d: part.Rigidbody.constraints = FreezeAll / None of ModuleWheelBase.toggleRbConstraints (the brake "schloompa", ModuleWheelBase.cs:785)
    static inline std::function<void(Part&, bool freeze)> setRigidbodyFrozen;
    // WP-29e: right after KSPWheelController.Create (ModuleWheelBase.wheelSetup, ModuleWheelBase.cs:282): the host gives the new VPWheelCollider its Transform
    // operations (VPWheelCollider::visual) and ray before the controller is enabled and its fields are assigned
    static inline std::function<void(Part&, VehiclePhysics::VPWheelCollider&)> wheelColliderCreated;
    // CheatOptions.NoCrashDamage
    static inline bool noCrashDamage = false;
    // FlightGlobals.ActiveVessel == vessel (CheckSubsteps)
    static inline std::function<bool(const Vessel*)> isActiveVessel;
    // Part.ShieldedFromAirstream
    static inline std::function<bool(const Part&)> shieldedFromAirstream;
    // TimeWarp.deltaTime is read from TimeWarp; Time.deltaTime / fixedDeltaTime from ksp::Time.
    static void Trace(const std::string& s) {
        if (trace) trace(s);
    }
    static void Reset();
};

namespace ModuleWheels {
class ModuleWheelDamage;
class ModuleWheelBrakes;
class ModuleWheelLock;
class ModuleWheelDeployment;
}  // namespace ModuleWheels

// ModuleWheelBase.cs
class ModuleWheelBase : public PartModule {
public:
    enum class DriftCorrectionState { Idle, Acquire, Fix };

    explicit ModuleWheelBase(const ModuleClassInfo& cls);
    ~ModuleWheelBase() override;

    // ---- the host-owned engine objects (wired before OnStart / WheelSetup) ----
    VehiclePhysics::NativeRigidbody* partRigidbody = nullptr;        // part.Rigidbody
    VehiclePhysics::NativeTransform* partRigidbodyTransform = nullptr;  // part.Rigidbody.gameObject.transform
    VehiclePhysics::NativeRigidbody* parentRigidbody = nullptr;      // part.parent.Rigidbody
    VehiclePhysics::NativeTransform* wheelColliderHost = nullptr;    // public Transform wheelColliderHost (OnStart: FindModelTransform)
    VehiclePhysics::NativeTransform* wheelTransform = nullptr;       // public Transform wheelTransform

    // ---- state that is not a [KSPField] ----
    bool suspensionEnabled = true;     // :86
    Vector2f slipDisplacement{};       // :157
    std::shared_ptr<VehiclePhysics::KSPWheelController> wheel;   // [SerializeField] wheel (the controller this module created)
    bool setup = false;
    Vector3f WheelOrgPosR{};
    Quaternionf WheelOrgRotR = Quaternionf::identity();
    bool driftCorrection = false;                                 // [SerializeField] private
    bool rbBrakeConstraints = false;                              // [SerializeField] private
    float kd = 100.0f, ki = 0.1f;                                 // [SerializeField] private
    float schloompaTime = 2.0f;
    float acquireMaxSpeed = 0.1f;
    DriftCorrectionState driftCorrectionState = DriftCorrectionState::Acquire;
    Vector3f fixFwd{}, error{}, errorLast{};
    ModuleWheels::WheelSubsystems inopSystems;                    // :159 (OnAwake: new WheelSubsystems())
    // landing detection (private)
    const VehiclePhysics::Collider* gCollider = nullptr;
    const VehiclePhysics::Collider* gColliderPrev = nullptr;
    Vessel* vSrfContact = nullptr;
    std::string vLandedAt;
    std::string crashObjectName;  // vessel.crashObjectName (the CrashObjectName component, recorded as set / null)

    // ---- properties ----
    VehiclePhysics::KSPWheelController* Wheel() const { return wheel.get(); }
    ModuleWheels::WheelSubsystems* InopSystems() { return &inopSystems; }
    WheelType GetWheelType() const {
        const ModuleValue* v = Value("wheelType");
        const EnumValue* e = v ? std::get_if<EnumValue>(v) : nullptr;
        return e ? static_cast<WheelType>(e->value) : WheelType::FREE;
    }

    // ---- lifecycle ----
    void OnAwake() override;                       // :202
    void OnStart(StartState state) override;       // :208
    void WheelSetup();                             // :277 (private; the WaitUntil coroutine of OnStart calls it, a host may call it directly)
    void OnDestroy();                              // :411
    void FixedUpdate();                            // :421
    void ResetWheelGroundCheck() override;         // :666
    void OnInventoryModeDisable() override;        // :821
    void OnInventoryModeEnable() override;         // :840
    void DemoteToPhysicslessPart() override;       // :849
    void PromoteToPhysicalPart() override;         // :854
    bool IsWheelSetUp() const { return setup; }

    float ApplyGeeBias(float gee);                 // :376
    void UpdateFriction();                         // :404
    void OnPutToGround(PartHeightQuery& qry);       // :671
    void RegisterSubmodule(ModuleWheels::ModuleWheelSubmodule* m);     // :695
    void UnregisterSubmodule(ModuleWheels::ModuleWheelSubmodule* m);   // :704
    std::string GetContractObjectiveType() const { return "Wheel"; }   // :1116
    bool CheckContractObjectiveValidity() const { return GetWheelType() == WheelType::MOTORIZED; }   // :1121
    virtual void EnableSuspension(KSPActionParam&) { suspensionEnabled = true; }    // :968
    virtual void DisableSuspension(KSPActionParam&) { suspensionEnabled = false; }  // :973

    // ---- events (GameEvents handlers) ----
    void OnPartPack(Part* p);                      // :718
    void OnPartUnpack(Part* p);                    // :726
    void OnDockingCompleteEvent(GameEvents::FromToAction<Part*, Part*> fromTo) { OnDockingComplete(fromTo.from, fromTo.to); }   // :734
    void OnDockingComplete(Part* from, Part* to);
    void OnVesselUndocking(Vessel* fromVessel, Vessel* toVessel);   // :767
    void OnVesselModified(Vessel* v);              // :649
    void SOIChangeEvent(GameEvents::HostedFromToAction<Vessel*, CelestialBody*> fromTo) { SOIChange(fromTo.host, fromTo.from, fromTo.to); }   // :813
    void SOIChange(Vessel* host, CelestialBody* from, CelestialBody* to);
    void EvtAutoFrictionToggle();                  // :1055 [KSPEvent]
    void ActAutoFrictionToggle(KSPActionParam& act);   // :1063 [KSPAction]

    // private members the tests (and the host) reach
    bool LandedDetectionNeedsUpdate(const VehiclePhysics::Collider* hitCollider, const VehiclePhysics::Collider* hitColliderPrev, Vessel* vContact,
                                    bool isGrounded);   // :553
    void LandingDetectionUpdate();                 // :473
    void LSchloomphaVPPProcessing();               // :602
    void CheckSuspensionToggle();                  // :978
    void CheckSubsteps();                          // :465
    void InopUpdate(bool force = false);           // :927
    void InopWheelCollider(bool inop, bool force); // :933
    void UpdateDriftFix();                         // :990
    void ActionUIUpdate();                         // :1069
    void ToggleRbConstraints(bool freeze);         // :785
    void HoldWheelDamage(float seconds = 3.0f);    // :801

    // ModuleWheelBase.GetModuleTitle / GetModuleDisplayName / GetPrimaryField
    std::string GetModuleTitle() { return "Wheel"; }
    std::string GetPrimaryField() { return Get<std::string>("TooltipPrimaryField"); }
    std::string GetModuleDisplayName() override { return Get<std::string>("TooltipTitle"); }

private:
    void OnWheelSetup(VehiclePhysics::KSPWheelController* w);   // :709
    void OnSubsystemsModified(ModuleWheels::WheelSubsystems* sub);   // :919
    void InopWheelTransform(bool disable);          // :963
    void EnableWheelCollider();                     // :623
    void DisableWheelCollider(bool immediate = false);   // :631
    void ATsymPartUpdate();                         // :1105
    Vector3f GetFixFwd();                           // :1038
    void GetFixTorque(const Vector3f& fixOrt, const Vector3f& refOrt);   // :1047
    bool IgnoreForcesOnSameVesselContact(const VehiclePhysics::VehicleBase::WheelState& st);   // :578
    Part* PartPtr() const;
    Vessel* VesselPtr() const;
    void SubscribeEvents();
    void UnsubscribeEvents();
    std::vector<ModuleWheels::ModuleWheelSubmodule*> subModules_;
    ModuleWheels::WheelSubsystem inopOnRails_{"Part is on-rails", ModuleWheels::WheelSubsystem::WheelCollider, nullptr};
    ModuleWheels::WheelSubsystem inopSuspension_{"Suspension is disabled", ModuleWheels::WheelSubsystem::Suspension, nullptr};
    ModuleWheels::ModuleWheelBrakes* brakesSubmodule_ = nullptr;
    ModuleWheels::ModuleWheelLock* wheelLockSubmodule_ = nullptr;
    ModuleWheels::ModuleWheelDamage* wheelDamageSubmodule_ = nullptr;
    VehiclePhysics::NativeTransform* wheelHost_ = nullptr;
    std::vector<std::shared_ptr<Coroutine>> coroutines_;
    bool subscribed_ = false;
};

namespace ModuleWheels {

// ModuleWheels/ModuleWheelSubmodule.cs
class ModuleWheelSubmodule : public PartModule {
public:
    explicit ModuleWheelSubmodule(const ModuleClassInfo& cls) : PartModule(cls) {}
    ~ModuleWheelSubmodule() override;
    VehiclePhysics::KSPWheelController* wheel = nullptr;   // internal KSPWheelController wheel
    bool BaseSetup() const { return baseSetup; }
    ModuleWheelBase* WheelBase() const { return wheelBase; }

    void OnAwake() override;                                // :17
    void OnDestroy();                                       // :42 (virtual in C#)
    void OnWheelInit(VehiclePhysics::KSPWheelController* w);   // :55
    virtual void OnWheelSetup() = 0;                        // :64 (abstract)
    virtual void OnSubsystemsModified(WheelSubsystems*) {}  // :68
    void SetWheelBase();                                    // :24
    int baseModuleIndexValue() const;

protected:
    ModuleWheelBase* wheelBase = nullptr;
    bool baseSetup = false;
    Part* PartPtr() const;
    Vessel* VesselPtr() const;
    std::shared_ptr<Coroutine> StartCoroutine(Enumerator e);
    std::vector<std::shared_ptr<Coroutine>> coroutines_;
};

// ModuleWheels/ModuleWheelMotor.cs
class ModuleWheelMotor : public ModuleWheelSubmodule {
public:
    enum class MotorState { Inoperable = -2, NotEnoughResources, Disabled, Idle, Running };
    explicit ModuleWheelMotor(const ModuleClassInfo& cls) : ModuleWheelSubmodule(cls) {}

    float driveInput = 0.0f;       // protected
    float maxTorque = 0.0f;
    MotorState state = MotorState::Disabled;
    double resourceFraction = 0.0;
    double avgResRate = 0.0;
    std::vector<const PartResourceDefinition*> consumedResources;
    std::vector<IWheelTorqueAdjuster*> adjusterCache;

    void OnAwake() override;                                 // :89
    void OnStart(StartState state) override;                 // :111
    void OnWheelSetup() override;                            // :122
    void FixedUpdate();                                      // :131
    virtual float OnDriveUpdate(float motorInput) { return motorInput; }   // :178
    float GetMotorOrientationSign();                         // :183
    bool CheckMotorState(float driveInput, std::string& stateString, double& rOutput, float torqueScalar);   // :205
    virtual bool GetMotorEnabled(bool baseMotorEnabled, std::string& stateString);   // :240
    float GetMaxSpeed();                                     // :256
    void EvtAutoTorqueToggle();                              // :267
    void MotorToggle(KSPActionParam&);                       // :275
    void MotorEnable(KSPActionParam&);                       // :281
    void MotorDisable(KSPActionParam&);                      // :287
    void ActAutoTorqueToggle(KSPActionParam& act);           // :293
    void ActionUIUpdate();                                   // :299
    void AddAdjuster(IWheelTorqueAdjuster* a) { adjusterCache.push_back(a); }   // :322
    void RemoveAdjuster(IWheelTorqueAdjuster* a);            // :331
    float ApplyTorqueAdjustments(float torque);              // :338
    const std::vector<const PartResourceDefinition*>& GetConsumedResources() const { return consumedResources; }
    void CopySerializedFrom(const PartModule& from) override;

protected:
    void ATsymPartUpdate();                                  // :311
};

// ModuleWheels/ModuleWheelMotorSteering.cs
class ModuleWheelMotorSteering : public ModuleWheelMotor {
public:
    explicit ModuleWheelMotorSteering(const ModuleClassInfo& cls) : ModuleWheelMotor(cls) {}
    Vector3f wFwd{}, Rcom{}, wXcom{}, steerAxisInput{};
    float wDot = 0.0f;
    float OnDriveUpdate(float motorInput) override;          // :34
    bool GetMotorEnabled(bool baseMotorEnabled, std::string& stateString) override;   // :61
};

// ModuleWheels/ModuleWheelBrakes.cs
class ModuleWheelBrakes : public ModuleWheelSubmodule {
public:
    explicit ModuleWheelBrakes(const ModuleClassInfo& cls) : ModuleWheelSubmodule(cls) {}
    IWheelStatusLight* statusLight = nullptr;
    std::vector<IWheelTorqueAdjuster*> adjusterCache;
    void OnStart(StartState state) override;                 // :30
    void OnWheelSetup() override;                            // :55
    void BrakeAction(KSPActionParam& kPar);                  // :66
    void FixedUpdate();                                      // :88
    void OnSubsystemsModified(WheelSubsystems* s) override;  // :111
    void AddAdjuster(IWheelTorqueAdjuster* a) { adjusterCache.push_back(a); }
    void RemoveAdjuster(IWheelTorqueAdjuster* a);
    float ApplyTorqueAdjustments(float torque);              // :146
};

// ModuleWheels/ModuleWheelLock.cs
class ModuleWheelLock : public ModuleWheelSubmodule {
public:
    explicit ModuleWheelLock(const ModuleClassInfo& cls) : ModuleWheelSubmodule(cls) {}
    void OnWheelSetup() override;                            // :8
};

// ModuleWheels/ModuleWheelSuspension.cs
class ModuleWheelSuspension : public ModuleWheelSubmodule {
public:
    explicit ModuleWheelSuspension(const ModuleClassInfo& cls) : ModuleWheelSubmodule(cls) {}
    ~ModuleWheelSuspension() override;
    // [SerializeField] private
    float spring = 0.0f, damper = 0.0f;
    bool autoSpringDamperSet = false;
    float damperFudge = 0.0f, boost = 0.0f, vesselMass = 0.0f;
    float springClampMax = 5000.0f, damperClampMax = 5000.0f, damperLerpBase = 1.0f, oscillationDamper = 3.0f, boostClamp = 0.5f, autoBoostClamp = 1.0f;
    bool highGeeOverride = false;
    float suspCompression = 0.0f, suspMass = 0.0f, sprungMassGravity = 0.0f;   // public non-field

    void OnAwake() override;                                 // :178
    void OnStart(StartState state) override;                 // :184
    void OnDestroy();                                        // :222
    void OnInventoryModeDisable() override;                  // :228
    void OnInventoryModeEnable() override;                   // :255
    void OnWheelSetup() override;                            // :289
    void FixedUpdate();                                      // :315
    void LateUpdate();                                       // :324
    void OnVesselPartCountChanged(Vessel* v);                // :281
    void OnSubsystemsModified(WheelSubsystems* s) override;  // :446
    void EvtAutoSpringDamperToggle();                        // :144
    float BoostCurve(float b, float power);                  // :350
    void SuspensionSpringUpdate(float sprungMass);           // :355
    void UpdateAutoBoost(float st);                          // :421
    bool HighGeeOverride() const { return highGeeOverride; }
    void ResetSuspensionCollider(float delay);               // :454

private:
    void ATsymPartUpdate();                                  // :163
    void ActionSpringDamperUIUpdate();                       // :152
    bool subscribed_ = false;
    VehiclePhysics::NativeTransform* suspensionTransform_ = nullptr;
};

// ModuleWheels/ModuleWheelSteering.cs
class ModuleWheelSteering : public ModuleWheelSubmodule {
public:
    explicit ModuleWheelSteering(const ModuleClassInfo& cls) : ModuleWheelSubmodule(cls) {}
    ~ModuleWheelSteering() override;
    float steeringInput = 0.0f, steeringInputLast = 0.0f;
    float steerAngle = 0.0f, steerRange = 0.0f;
    Vector3f tPivot{}, tOrt{}, tCenter{};
    float h = 0.0f;

    void OnAwake() override;                                 // :146
    void OnStart(StartState state) override;                 // :167
    void OnDestroy();                                        // :211
    void OnWheelSetup() override;                            // :220
    void FixedUpdate();                                      // :247
    void OnSubsystemsModified(WheelSubsystems* s) override;  // :418
    void SteeringToggle(KSPActionParam& act);                // :206
    void EvtAutoSteeringAdjustToggle();                      // :104
    float GetSteeringResponseScale(float steerDelta);        // :272
    float GetCoMFwdLength();                                 // :360
    void OnWheelRepaired(Part* p);                           // :446
    bool CaliperUpdating() const { return caliperUpdating_; }
    float CoMfwdLength() const { return CoMfwdLength_; }
    // the wheel transform's right vector (wheelBase.wheelTransform.right) is read from the base's transform seam

private:
    void ActionSteeringAdjustUIUpdate();                     // :111
    void ATsymPartUpdate();                                  // :132
    void UpdateCoordFrame();                                 // :298
    float UpdateSteering(float input, float steeringRange);  // :323
    float FindCoMfwdLength(const Vector3f& vesselCoM, const Vector3f& wheelCenter, const Vector3f& fwdAxis);   // :355
    Vector3f FindTurnCenter(float steerAngle, float length, const Vector3f& CoM);   // :366
    void SetCaliperUpdate(bool update);                      // :426
    void ResetCaliper(bool resetTransform);                  // :436
    VehiclePhysics::NativeTransform* caliperTransform_ = nullptr;  // ModuleWheelSteering.caliperTransform (WP-29e: handed to the VPWheelCollider)
    bool caliperUpdating_ = false;
    Vector3f upAxis_{}, fwdAxis_{}, leftAxis_{}, CoM_{}, wCenter_{}, wLeft0_{}, wLeft_{}, wRight_{}, sAxis_{};
    float CoMfwdLength_ = 0.0f;
    bool steeringAngleCurveDefined_ = false, steeringCurveDefined_ = false;
    Vector2f prevLocalWheelVelocity_{};
    bool subscribed_ = false;
    bool radiusRegistered_ = false;
};

// ModuleWheels/ModuleWheelDamage.cs
class ModuleWheelDamage : public ModuleWheelSubmodule, public IConstruction {
public:
    explicit ModuleWheelDamage(const ModuleClassInfo& cls) : ModuleWheelSubmodule(cls), subsystemDamage_("Wheel Damage", 33, nullptr) {}
    ~ModuleWheelDamage() override;
    float currentDeflection = 0.0f, lastDeflection = 0.0f, currentSlip = 0.0f, lastSlip = 0.0f, currentDownForce = 0.0f, lastDownForce = 0.0f;
    float startupTime = 8.0f;                 // internal
    int repairKitsNecessary = 1;              // internal [SerializeField]
    bool initialized = false;
    float totalStress = 0.0f;
    float stressVariability = 0.0f;
    bool impactDamageColliderPresent = false; // impactDamageCollider != null (the model lookup under the undamaged transform)

    void OnStart(StartState state) override;                 // :121
    void OnInventoryModeEnable() override;                   // :208
    void OnInventoryModeDisable() override;                  // :219
    void OnDestroy();                                        // :230
    void OnWheelSetup() override;                            // :269
    void FixedUpdate();                                      // :289
    void SetDamaged(bool damaged);                           // :368
    void ResetImmunity();                                    // :263 (internal)
    void DamageWheel() { SetDamaged(true); }                 // :446
    void EventRepairExternal();                              // :452 (the EVA inventory / skill checks are the host's: Repair())
    void Repair() { SetDamaged(false); }
    bool CanBeDetached() override { return !Get<bool>("isDamaged"); }   // :483
    bool CanBeOffset() override { return !Get<bool>("isDamaged"); }     // :488
    bool CanBeRotated() override { return !Get<bool>("isDamaged"); }    // :493
    bool IsImpactDamageEnable();                             // :105
    void HandleCollisionVelocity(const VehiclePhysics::Collider* other, const Vector3f& velocity, const Vector3f& normal);   // :529
    IConstruction* AsConstruction() override { return this; }
    float repairImmunityTimeLeft() const { return repairImmunityTimeLeft_; }
    float repairImmunityTimeTotal() const { return repairImmunityTimeTotal_; }
    float stressTime() const { return stressTime_; }

private:
    void Initialize();                                       // :278
    WheelSubsystem subsystemDamage_;
    float repairImmunityTimeTotal_ = 30.0f;
    float repairImmunityTimeLeft_ = 0.0f;
    float stressTime_ = 0.0f;
    bool subscribed_ = false;
};

// ModuleWheels/ModuleWheelDeployment.cs
class ModuleWheelDeployment : public ModuleWheelSubmodule {
public:
    explicit ModuleWheelDeployment(const ModuleClassInfo& cls) : ModuleWheelSubmodule(cls) {}
    ~ModuleWheelDeployment() override;
    float retractedPosition = 0.0f;
    float position = 0.0f;
    float animLength = 1.0f;
    std::vector<int> slaveModuleIndices;
    bool subsystemsDisabled = false;
    bool slaveModuleIndicesLoaded = false;
    KerbalFSM fsm;
    KFSMState *st_deployed = nullptr, *st_retracted = nullptr, *st_deploying = nullptr, *st_retracting = nullptr, *st_inoperable = nullptr;
    KFSMEvent *on_deploy = nullptr, *on_deployed = nullptr, *on_retract = nullptr, *on_retracted = nullptr, *on_inoperable = nullptr;

    float Position() const;                                  // :139
    // WP-29d: the host reads the stand-in collider (ModuleWheelDeployment.standInCollider != null) and the deploy target transform
    bool StandInColliderActive() const { return standInCollider_; }
    VehiclePhysics::NativeTransform* DeployTarget() const { return deployTgt_; }
    void OnLoad(const ConfigNode& node) override;            // :153
    void OnSave(ConfigNode& node) override;                  // :166
    void OnStart(StartState state) override;                 // :175
    void OnInventoryModeDisable() override;                  // :278
    void OnStoredInInventory(ModuleInventoryPart& m) override;   // :328
    void OnWheelSetup() override;                            // :342
    void FixedUpdate();                                      // :685
    void Update();                                           // :693
    void LateUpdate();                                       // :701
    void EventToggle();                                      // :710
    void ActionToggle(KSPActionParam& kAct);                 // :765
    void ActionToggle(KSPActionType action);                 // :775
    float FindTransformAnimationTime(VehiclePhysics::NativeTransform* t);   // :841
    void CopySerializedFrom(const PartModule& from) override;

private:
    void SetInopSubsystems(bool disable);                    // :355
    void FSMSetup(const std::string& startStateName);        // :405
    void OnFSMStateChange(KFSMState* from, KFSMState* to, KFSMEvent* evt);   // :662
    void ToggleDeployment(bool affectSymCParts);             // :715
    std::string GetToggleEventName();                        // :743
    void SetWheelTransformFromTarget();
    bool retractStarted_ = false;
    Animation* anim_ = nullptr;
    AnimationState* animationState_ = nullptr;
    VehiclePhysics::NativeTransform* deployTgt_ = nullptr;
    VehiclePhysics::NativeTransform* retractTransform_ = nullptr;
    std::vector<IScalarModule*> slaveModules_;
    WheelSubsystem deploymentSubsystems_{"Wheel Retracted", WheelSubsystem::All, nullptr};
    bool standInCollider_ = false;
    std::vector<std::unique_ptr<KFSMState>> stateStore_;
    std::vector<std::unique_ptr<KFSMEvent>> eventStore_;
};

// ModuleWheels/ModuleWheelBogey.cs (the managed math; Quaternion.RotateTowards / Lerp / Slerp are native Unity code, written from the documented
// formulation and UNVERIFIED)
class ModuleWheelBogey : public ModuleWheelSubmodule {
public:
    explicit ModuleWheelBogey(const ModuleClassInfo& cls) : ModuleWheelSubmodule(cls) {}
    Vector3f bogeyUp{};
    Quaternionf bogeyRot = Quaternionf::identity();
    float bogeyAngle = 0.0f;
    ModuleWheelDeployment* deployModule = nullptr;
    // the bogey transform tree (Transform.localRotation / parent.rotation); host-wired
    VehiclePhysics::NativeTransform* bogey = nullptr;
    Quaternionf bogeyParentRotation = Quaternionf::identity();   // bogey.parent.rotation
    Quaternionf bogeyLocalRotation = Quaternionf::identity();    // bogey.localRotation (written by LateUpdate)
    Quaternionf bogeyRefLocalRotation = Quaternionf::identity(); // bogeyRef.localRotation
    bool hasBogeyRef = false;

    void OnStart(StartState state) override;                 // :64
    void OnWheelSetup() override;                            // :134
    void LateUpdate();                                       // :149

private:
    Vector3f bogeyUp0_{};
    Quaternionf bogeyRot0_ = Quaternionf::identity();
    Quaternionf bogeyRotLast_ = Quaternionf::identity();
};

}  // namespace ModuleWheels

// SuspensionLoadBalancer.cs (a VesselModule in the game; kspcore's VesselModule host is schema driven and this class has no KSPField, so it is a
// plain object the vessel owns: Create / FixedUpdate / VesselChanged are the host's calls)
class SuspensionLoadBalancer {
public:
    float multiplier = 1.0f;     // public field
    float minMass = 0.5f;
    explicit SuspensionLoadBalancer(Vessel* v) : vessel(v) {}
    Vessel* vessel;
    void OnStart();                                          // :24
    void FixedUpdate();                                      // :31
    void VesselChanged(Vessel* vsl);                         // :66
    void UpdateSuspensionModules();                          // :74
    const std::vector<ModuleWheels::ModuleWheelSuspension*>& SuspensionModules() const { return suspensionModules_; }
    bool IsReady() const { return isReady_; }

private:
    std::vector<ModuleWheels::ModuleWheelSuspension*> suspensionModules_;
    bool isReady_ = false;
};

namespace detail {
void NoteLiveWheelBase(const ModuleWheelBase* b, bool alive);   // the submodules check that their base still exists
void RegisterWheelModules();   // ModuleWheelBase + the ModuleWheels.* submodules (called from RegisterBuiltinModules)
}

}  // namespace ksp
