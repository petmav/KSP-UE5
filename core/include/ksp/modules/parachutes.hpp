// WP-27 parachutes (numeric / data side): ModuleParachute.cs and ModuleEvaChute.cs.
//
// Ported: the deployment state machine (STOWED -> ACTIVE -> SEMIDEPLOYED -> DEPLOYED -> CUT, persistentState / animTime), the arming rule
// (static pressure vs minAirPressureToOpen, ShouldDeploy altitude, IsMovingFastEnoughToDeploy), the animation time arithmetic (lerpTime =
// Pow(normalizedTime, deploymentCurve), maximum_drag Lerp), the PACKED / SEMIDEPLOYED / DEPLOYED drag cube weights, the chute heating model
// (SetConvectiveStats, DeploySafe, UpdateCut with the Stefan-Boltzmann cooling), the automatic cut (autoCutSpeed, no atmosphere, overheat),
// CalcBaseStats (areaDeployed / areaSemi from the drag cubes and maxSafeSpeedAtRef by bisection) and the EVA chute's activation / repack /
// yaw-pitch-roll rate maths. ModuleParachute.CalcDragAtDeployedAltitude does not exist in KSP 1.12.5 (the nearest quantities are GetArea /
// CalcBaseStats).
//
// Host hooks (ParachuteEnvironment, each documented): the rigidbody velocity plus Krakensbane frame velocity, static pressure and altitude at
// the part, the downward raycast against the terrain layer, the canopy transform orientation, time. The vessel comes through
// ResourceProcessingEnvironment::VesselOf (ProcessingVessel).
#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>

#include "ksp/aero/aero_interfaces.hpp"
#include "ksp/modules/deployables.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/unity/animation.hpp"
#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

class Part;
class ModuleParachute;
class Vessel;

struct ParachuteEnvironment {
    // Part.Rigidbody.velocity + Krakensbane.GetFrameVelocity() (Vector3 widened to Vector3d); unset: zero
    static inline std::function<Vector3d(Part&)> velocity;
    // FlightGlobals.getStaticPressure(part.transform.position, vessel.mainBody), kPa; unset: 0
    static inline std::function<double(Part&)> staticPressure;
    // FlightGlobals.getAltitudeAtPos(part.transform.position, vessel.mainBody); unset: vessel altitude
    static inline std::function<double(Part&)> altitudeAtPos;
    // Physics.Raycast(part.transform.position, -vessel.upAxis, deployAltitude, layer 32768 (terrain colliders)): true = hit; unset: false
    static inline std::function<bool(Part&, float distance)> raycastDown;
    // The canopy transform of the deployed phases (FixedUpdate: LookRotation of the drag vector, spreadAngle Rotate, Perlin wobble,
    // RotateTowards(lastRot, canopy.rotation, rotationSpeedDPS * fixedDeltaTime)). Returns the drag vector rotation
    // `Quaternion.LookRotation(part.partTransform.InverseTransformDirection(canopy.forward))`, or nullopt when there is nothing to set.
    // The arguments are the numbers the transform code reads: symmetryCount, the clamped spread angle `num2` after the /3 rules.
    struct CanopyInput {
        bool rotateParachute = true;     // !dontRotateParachute
        float spreadDegrees = 0.0f;      // Mathf.Clamp(spreadAngle * symmetryCount, 0, 45) after /3 or /(3 - 2 * normalizedTime); 0 = no spread rotate
        bool dragVectorNonZero = false;  // part.dragVectorDir != Vector3.zero
        float rotationStepDegrees = 0.0f;  // rotationSpeedDPS * TimeWarp.fixedDeltaTime
    };
    static inline std::function<std::optional<Quaternionf>(ModuleParachute&, const CanopyInput&)> orientCanopy;
    // ScreenMessages.PostScreenMessage texts; FlightLogger.fetch.LogEvent texts
    static inline std::vector<std::string> screenMessages, flightLog;
    // part.stackIcon colours (the staging UI): the last SetIconColor / SetBackgroundColor name; FXGroup.Burst count of the deploy fx
    static void Reset();
};

// ModuleParachute.cs:5
class ModuleParachute : public PartModule, public IMultipleDragCube {
public:
    explicit ModuleParachute(const ModuleClassInfo& cls) : PartModule(cls) {}
    ~ModuleParachute() override;

    enum class deploymentStates : int { STOWED, ACTIVE, SEMIDEPLOYED, DEPLOYED, CUT };  // :7
    enum class deploymentSafeStates : int { SAFE, RISKY, UNSAFE, NONE };                  // :16

    // ---- non-KSPField members (ModuleParachute.cs:23-169) ----
    deploymentStates deploymentState = deploymentStates::STOWED;          // :23
    deploymentSafeStates deploymentSafeState = deploymentSafeStates::SAFE;  // :25 (default enum value)
    float rotationSpeedDPS = 90.0f;                                       // :37
    std::shared_ptr<Animation> Anim;                                      // :129 public Animation Anim
    Quaternionf lastRot = Quaternionf::identity();                        // :131
    int symmetryCount = 0;                                                // :127
    double areaSemi = 0.0, areaDeployed = 0.0, maxSafeSpeedAtRef = 0.0;    // :135-139
    double invThermalMass = 0.0, machHeatMult = 0.0, convectivekW = 0.0, shockTemp = 0.0, convectionArea = 0.0, finalTemp = 0.0;  // :141-151
    bool dontRotateParachute = false;                                     // :153 protected
    bool deactivateOnRepack = true;                                       // :155
    float lerpTime = 0.0f;                                                // :121 private
    float baseDrag = 0.0f;                                                // :123
    double timeUnpacked = 0.0;                                            // :157
    bool hasCurrentVessel = false;                                        // :159 currentVessel != null
    Vessel* currentVessel = nullptr;                                      // currentVessel (compared by identity: see OnVesselWasModified)
    int deactivateCalls = 0;                                              // Part.deactivate() calls (the staging state is WP-16's)
    bool hasCanopy = false, hasCap = false;                               // canopy / cap transforms found
    bool canopyActive = false, capActive = true;                          // gameObject.activeSelf of the two transforms
    bool deployfieldGuiActive = false;                                    // deployfield.guiActive
    std::string iconColor = "White", iconBackground = "White";            // part.stackIcon.SetIconColor / SetBackgroundColor (XKCDColors name)
    int deployFxBursts = 0;                                               // deployFx.Burst()
    bool subscribedVesselModified = false;

    // ---- typed accessors ----
    std::string Str(std::string_view field) const;  // a string KSPField (null -> "")

    bool IsMultipleCubesActive() const override { return true; }                        // :190
    std::vector<std::string> GetDragCubeNames() const override;                         // :840 (C# null when no animation)
    void AssumeDragCubePosition(const std::string& name) override;                      // :850
    bool UsesProceduralDragCubes() const override { return false; }                     // :884

    // ---- PartModule hooks / KSPActions / KSPEvents ----
    void OnAwake() override;                       // :301
    void OnLoad(const ConfigNode& node) override;  // :293
    void OnStart(StartState state) override;       // :308
    void OnActive() override;                      // :447
    std::string GetModuleDisplayName() override;   // :977
    void DeployAction(KSPActionParam& param);      // :193
    void CutAction(KSPActionParam& param);         // :199
    virtual void Deploy();                         // :205 [KSPEvent]
    virtual void CutParachute();                   // :229 [KSPEvent]
    virtual void Repack();                         // :246 [KSPEvent]
    virtual void Disarm();                         // :278 [KSPEvent]
    virtual void OnDestroy();                      // :409 (Unity message; the destructor calls it)
    void SetUIEVents();                            // :349 SetUIEVents

    // ---- the maths ----
    double GetArea(const DragCube& cube) const;    // :455
    void CalcBaseStats();                          // :470
    virtual void FixedUpdate();                    // :546 (Unity message)
    bool ShouldDeploy();                           // :738
    bool IsMovingFastEnoughToDeploy();             // :747
    void SetConvectiveStats(double density, double extTemp, double mach, double convCoeff);  // :889
    std::string DeploySafe(deploymentSafeStates& state);                                     // :908 (returns the localised text)
    void UpdateCut();                              // :931
    void OnVesselWasModified(Vessel* v);           // :775
    void UpdateSymmetry(Vessel* v);                // :796
    void OnPartUnpack();                           // :811
    void SetupAnimation();                         // :414
    std::string GetInfo() override;                // :526
    void SetDragCubes_Stowed();                    // :816
    void SetDragCube_SemiDeployed(float semiDeployed);  // :824
    void SetDragCube_Deployed(float deployed);     // :832
    const std::string& DeploySafeText() const { return deploySafeText_; }

    // virtual extension points of ModuleEvaChute (:885-905)
    virtual void OnParachuteSemiDeployed() {}      // :730
    virtual void OnParachuteFullyDeployed() {}     // :734
    virtual bool PassedAdditionalDeploymentChecks() { return true; }  // :752
    virtual bool IsAnimationPlaying();             // :757
    virtual float GetAnimationNormalizedTime(const std::string& animationName);  // :762
    virtual void PlayDeployAnimation(const std::string& animationName, float deploymentSpeed);  // :767

    static constexpr double pressureRecip = 0.009869232667160128;  // :75

protected:
    Part* PartPtr() const;
    const ProcessingVessel* VesselRecord() const;
    bool ShieldedFromAirstream() const;
    DragCubeList* Cubes() const;
    void SetIcon(const char* color) { iconColor = color; }
    std::string deploySafeText_;
    void SetEnumFromPersistentState();
};

// ModuleEvaChute.cs:5
class ModuleEvaChute : public ModuleParachute {
public:
    using ModuleParachute::ModuleParachute;
    // KerbalEVA (the EVA controller) as the chute sees it: state queries and callbacks (KerbalEVA is the EVA package's)
    struct KerbalEVAHost {
        bool IsChuteState = false;
        bool JetpackDeployed = false;
        bool seated = false;
        bool ableToDeployParachute = true;               // IsKerbalInStateAbleToDeployParachute
        int onParachuteSemiDeployedCalls = 0, onParachuteFullyDeployedCalls = 0, onParachuteCutCalls = 0;
        // WP-32: the real KerbalEVA.OnParachuteSemiDeployed / FullyDeployed / Cut (KerbalEVA.cs:4690 / :4715 / :4762); unset: only the counters
        std::function<void()> onSemiDeployed, onFullyDeployed, onCut;
        // kerbalEVA.transform.up / right / forward
        Vector3f up{0, 1, 0}, right{1, 0, 0}, forward{0, 0, 1};
    };
    KerbalEVAHost kerbalEVA;
    bool hasKerbalEVA = true;
    int inventoryItemCount = 1;      // inventory.InventoryItemCount (hasInventory: ModuleInventoryPart present)
    bool hasInventory = false;
    int crewExperienceLevel = 0;     // part.protoModuleCrew[0].experienceLevel
    int evaChuteSkill = 0;           // vessel.VesselValues.EVAChuteSkill.value
    bool showEVAChuteParams = false;

    void OnStart(StartState state) override;                 // :61
    bool IsStageable() override { return false; }            // :136
    void OnActive() override {}                              // :141
    void FixedUpdate() override;                             // :145
    void OnParachuteSemiDeployed() override;                 // :262
    void OnParachuteFullyDeployed() override;                // :280
    bool PassedAdditionalDeploymentChecks() override;        // :286
    void CutParachute() override;                            // :295
    void AllowRepack(bool allowRepack);                      // :307
    bool CanCrewMemberUseParachute();                        // :191
    bool CanCrewMemberUseParachute(int experienceLevel);     // :191
    void SetEVAChuteActive(bool active);                     // :242
    void ShowEVAChuteParamsChanged();                        // :220 [KSPEvent]
    void ToggleEVAChuteParams(bool show);                    // :226

    // The pure maths of UpdateFullyDeployedParachuteMovement (:155): the commanded angular velocity contribution `zero` and the yaw/pitch/roll
    // rates; the gravity-alignment term (axis * angle * Deg2Rad, Quaternion.AngleAxis / FromToRotation) is the engine's rotation maths.
    struct ChuteRates {
        float num = 0.0f;   // yaw rate at the current speed
        float num3 = 0.0f;  // roll rate
        float num4 = 0.0f;  // pitch rate
        float num5 = 0.0f;  // yaw rate with the pitch divisor
        Vector3f zero;      // the input contribution to the angular velocity
    };
    ChuteRates ComputeChuteRates(const Vector3f& parachuteInput, double vesselSpeed) const;  // :155 (the part that does not need rotations)
};

namespace detail {
void RegisterParachuteModules();  // parachutes.cpp
}

}  // namespace ksp
