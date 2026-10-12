// WP-32: KerbalEVA.cs (the EVA kerbal controller: PartModule + KerbalFSM state machine), KerbalAnimationManager.cs / KerbalAnimationState.cs,
// KerbalRagdollNode.cs. Companion headers: retractable_ladder.hpp (RetractableLadder.cs), flight_eva.hpp (FlightEVA.cs), kerbal.hpp (Kerbal.cs).
//
// What is ported (decompiled/KSP-Decompiled/Assembly-CSharp/KerbalEVA.cs, 7,368 lines, every method; line numbers are cited at each definition):
//   * SetupFSM: all 41 states, all 76 events (conditions, OnEvent bodies, GoToStateOnEvent, update modes) and the exact callback lists of every
//     state, in registration order (wiring() reports them by name; the unit gate compares them with a table parsed from the C# source)
//   * ground movement: UpdateMovement, UpdateHeading, correctGroundedRotation, walk / run / low-g bound animation blending, jump, landing, recover
//   * jetpack: ToggleJetpack, UpdatePackLinear / Angular, the orientation PID, UpdatePackFuel, EVA propellant bookkeeping (ProcessEVAFuel,
//     SetupEVAFuel, UpdatePackModels), jetpack effect latching
//   * ladders: triggers, acquire / climb / descend / lean / pushoff / end reached, auto transition, ladder hold (anti-sliding), clamber paths
//   * boarding (BoardPart, checkExperiments, proceedAndBoard), flag planting (PlantFlag, CanPlantFlag, FlagSite flight-log credit), seats, grapple,
//     parachute states, construction mode (EnterConstructionMode / Weld / InterruptWeld / welding line of sight), visor / helmet / pressure checks
//   * persistence: OnLoad / OnSave (state, step, vInfo, packExt, lightOn, flags, physic material), OnStart event wiring, StartEVA
// What is a seam (engine_*: eva_engine.hpp): Unity Animation / AnimationState, Transform of the kerbal model, Rigidbody, Collider, Physics casts,
// ragdoll nodes, audio / FX groups, input polling, camera, FlagSite, KerbalSeat, portrait cameras. The kerbal's `transform` is the Part's partTransform.
// Deferred (needs the physics bubble WP-19b / 20d, animation or the UE5 shell): see STATUS.md "WP-32".
//
// Lifetime: states and events are owned by the KerbalEVA (unique_ptr vectors); the FSM holds raw pointers like the C# references.
#pragma once

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ksp/core/coroutine.hpp"
#include "ksp/eva/eva_engine.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/parts/docked_vessel_info.hpp"
#include "ksp/unity/color.hpp"
#include "ksp/fsm/kerbal_fsm.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

class ModuleInventoryPart;
class ModuleEvaChute;
class ScienceData;
class CelestialBody;
struct PartResource;
class ProtoPartResourceSnapshot;
class ProtoCrewMember;
class KerbalAnimationManager;

// KerbalAnimationState.cs
class KerbalAnimationState {
public:
    std::string animationName;      // :8
    bool hasJpVariant = false;      // :10
    bool scaleWithGravity = false;  // :12
    float speedAt1Gee = 0.0f;       // :14
    float start = 0.0f;             // :16
    float end = 1.0f;               // :18
    int layer = 0;                  // :20
    int wrapMode = 0;               // :22 (UnityEngine.WrapMode)
    int blendMode = 0;              // :24
    float weight = 0.0f;            // :26
    float wheeLevel = 0.0f, fearFactor = 0.0f, startExpressionTime = 0.0f, CutAnimationTime = 0.0f;  // :38-:44
    KerbalAnimationManager* manager = nullptr;

    // :52 `manager.evaController.GetComponent<Animation>()[GetAnimationString()]`
    EvaAnimState* State() const;
    void SetManager(KerbalAnimationManager* m) { manager = m; }   // :54
    // :66 "jp_" + animationName while the jetpack is deployed and the clip has a jp variant
    std::string GetAnimationString() const;
    operator std::string() const { return GetAnimationString(); }  // implicit operator string (:59)
};

// KerbalAnimationManager.cs
class KerbalAnimationManager {
public:
#define KSP_KERBAL_ANIM(n) KerbalAnimationState n;
#include "ksp/eva/kerbal_animations.inc"
#undef KSP_KERBAL_ANIM
    std::vector<KerbalAnimationState*> controlPanelAnims;  // :112
    int controlPanelAnimSelector = 0;                      // :114
    std::vector<int> syncLayers;                           // :144 (the prefab's)
    KerbalEVA* evaController = nullptr;                    // :146
    std::vector<KerbalAnimationState*> RandomIdleAnims;    // :148

    KerbalAnimationManager() = default;
    KerbalAnimationManager(const KerbalAnimationManager&) = delete;
    KerbalAnimationManager& operator=(const KerbalAnimationManager&) = delete;

    void Start(KerbalEVA* evaController);                  // :150
    void SyncAnimationLayers();                            // :189
    // :199 declaration order of the KerbalAnimationState fields
    std::vector<KerbalAnimationState*> GetAllAnimations();
};

// KerbalEVA.ClamberPath (:31)
struct ClamberPath {
    Vector3f edgeFaceNormal, srfNormal, p1, p2, p3;
    float clamberHeight = 0.0f, standOffDistance = 0.0f;
    ClamberPath(const Vector3f& edgeFaceNormal, const Vector3f& srfNormal, const Vector3f& p1, const Vector3f& p2, const Vector3f& p3, float height,
                float standOff);  // :47
};

// KerbalEVA.HelmetColliderSetup (:20)
struct HelmetColliderSetup {
    float helmetOnRadius = 0.27f;
    Vector3f helmetOnCenter = Vector3f::zero();
    float helmetOffRadius = 0.19f;
    Vector3f helmetOffCenter{0.03f, 0.03f, 0.0f};
};

// KerbalEVA.ResourceListItem (:12)
struct EvaResourceListItem {
    ProtoPartResourceSnapshot* pPResourceSnapshot = nullptr;
    int priority = 0;
};

// The callbacks wired into one state, by C# method name, in registration order (SetupFSM). Used by the unit gate.
struct EvaStateWiring {
    std::string onEnter, onLeave;
    std::vector<std::string> onUpdate, onFixedUpdate, onLateUpdate;
    int updateMode = 2;  // KFSMState.updateMode (FIXEDUPDATE default; ladder_idle sets UPDATE)
};
// One event as SetupFSM left it: the registrations (state names) and the static fields
struct EvaEventWiring {
    std::string gotoState;                  // GoToStateOnEvent at the end of SetupFSM (some are re-targeted by OnEvent at run time)
    int updateMode = 1;                     // KFSMEvent.updateMode
    std::vector<std::string> inStates;      // states whose StateEvents contain the event, in state-registration order
    bool timed = false;
    double timerDuration = 0.0;
};

// WeldFX.cs as KerbalEVA reads it (the effect object is the shell's: this is its state)
struct EvaWeldFx {
    bool Active = false;                // :2987 weldFX.Active
    bool meshEnabled = false;           // weldFX.mesh.enabled
    bool IsFloating = false;            // :3047
    bool playing = false;
    float StartDelay = 0.0f;            // weldFX.StartDelay (OnWeldStart)
    std::vector<std::string> log;       // EnableMesh(d) / DisableMesh(d) / Play / Stop, in call order
    void EnableMesh(float delay);
    void DisableMesh(float delay);
    void Play();
    void Stop();
};

// KerbalEVA.VisorStates (:59)
enum class VisorStates : int { Raised, Lowered, Raising, Lowering };

class KerbalEVA : public PartModule, public IEvaController {
public:
    explicit KerbalEVA(const ModuleClassInfo& cls);
    ~KerbalEVA() override;

    // ======================================================================== [KSPField]s (the schema owns the values)
#define EVA_F(T, n)                              \
    T n() const { return Get<T>(#n); }           \
    void set_##n(T v) { Set<T>(#n, v); }
    EVA_F(float, lightR) EVA_F(float, lightG) EVA_F(float, lightB)
    EVA_F(float, walkSpeed) EVA_F(float, strafeSpeed) EVA_F(float, runSpeed) EVA_F(float, turnRate) EVA_F(float, maxJumpForce)
    EVA_F(float, jumpMultiplier) EVA_F(float, boundForce) EVA_F(float, boundSpeed) EVA_F(float, boundThreshold) EVA_F(float, SlopeForce)
    EVA_F(float, swimSpeed) EVA_F(float, waterAngularDragMultiplier) EVA_F(float, ladderClimbSpeed) EVA_F(float, ladderPushoffForce)
    EVA_F(float, minWalkingGee) EVA_F(float, minRunningGee) EVA_F(float, initialMass) EVA_F(float, massMultiplier)
    EVA_F(float, onFallHeightFromTerrain) EVA_F(float, clamberMaxAlt) EVA_F(bool, JetpackDeployed) EVA_F(bool, lampOn)
    EVA_F(bool, splatEnabled) EVA_F(float, splatSpeed) EVA_F(std::string, propellantResourceName) EVA_F(bool, isHelmetEnabled)
    EVA_F(bool, isNeckRingEnabled) EVA_F(float, helmetOffMinSafePressureAtm) EVA_F(float, helmetOffMaxSafePressureAtm)
    EVA_F(float, helmetOffMinSafeTempK) EVA_F(float, helmetOffMaxSafeTempK) EVA_F(float, helmetOffMaxOceanPressureAtm)
    EVA_F(float, helmetOffMinSafePressureAtmMargin) EVA_F(float, helmetOffMaxSafePressureAtmMargin) EVA_F(float, helmetOffMinSafeTempKMargin)
    EVA_F(float, helmetOffMaxSafeTempKMargin) EVA_F(float, helmetOffMaxOceanPressureAtmMargin) EVA_F(float, evaExitTemperature)
    EVA_F(bool, isVisorEnabled) EVA_F(float, thrustPercentage) EVA_F(float, boundFrequency) EVA_F(float, boundSharpness) EVA_F(float, boundAttack)
    EVA_F(float, boundRelease) EVA_F(float, boundFallThreshold) EVA_F(float, lastBoundStep) EVA_F(int32_t, _flags) EVA_F(float, flagReach)
    EVA_F(float, Kp) EVA_F(float, Ki) EVA_F(float, Kd) EVA_F(float, iC) EVA_F(float, rotPower) EVA_F(float, linPower)
    EVA_F(float, PropellantConsumption) EVA_F(float, stumbleThreshold) EVA_F(float, hopThreshold) EVA_F(float, recoverThreshold)
    EVA_F(double, recoverTime) EVA_F(float, splatThreshold) EVA_F(float, clamberReach) EVA_F(float, clamberStandoff)
    EVA_F(float, controlPanelReach) EVA_F(float, controlPanelStandoff) EVA_F(float, VisorAnimationSpeed) EVA_F(bool, useGlobalPhysicMaterial)
#undef EVA_F

    // ======================================================================== non-field state (KerbalEVA.cs:153-1124), public for the shell and tests
    IEvaEngine* engine = nullptr;               // the Unity side (Animation, Rigidbody, ray casts); owned by the caller
    std::unique_ptr<KerbalAnimationManager> animations = std::make_unique<KerbalAnimationManager>();  // :159 `Animations`
    KerbalAnimationManager& Animations() { return *animations; }
    std::vector<KerbalRagdollNode> ragdollNodes;  // :161
    std::unique_ptr<KerbalFSM> fsm;               // :175
    bool DebugFSMState = false;                   // :177
    bool HasJetpack = false;                      // :179
    bool jetpackDeployedForWelding = false;       // :184
    bool JetpackIsThrusting = false;              // :186
    bool autoGrabLadderOnStart = true;            // :188
    bool CharacterFrameMode = true;               // :203
    bool CharacterFrameModeToggle = false;        // :205
    bool loadedFromSFS = false;                   // :207
    std::string loadedStateName;                  // :209
    Vector3f ejectDirection = Vector3f::up();     // :211
    PartResource* propellantResource = nullptr;   // :220
    std::vector<EvaResourceListItem> inventoryPropellantResources;  // :222
    std::unique_ptr<DockedVesselInfo> kerbalVesselInfo;             // :224
    ModuleEvaChute* evaChute = nullptr;           // :232
    bool HasParachute = false;                    // :234
    bool helmetTransformExists = true, neckRingTransformExists = true;  // :246-:248 (helmetTransform / neckRingTransform != null)
    HelmetColliderSetup helmetColliderSetup;      // :242
    bool partPlacementMode = false;               // :291
    ModuleInventoryPart* moduleInventoryPartReference = nullptr;  // :293
    static inline bool alwaysShowInventory = false;  // :295
    float newidleTime = 10.0f;                    // :304
    float newidleCounter = 0.0f;                  // :306
    bool startTimer = false;                      // :308
    int idleAnimationsIndex = 0;                  // :310
    bool visibleInPortrait = false;               // :314
    bool isActiveVessel = false;                  // :334
    float standardCameraDistance = 0.519f, swimmingCameraDistance = 0.612f, runningCameraDistance = 0.654f, ragdollCameraDistance = 1.0f;  // :339-:348
    float KerbalAvatarUpdateInterval = 0.1f;      // :328
    bool sciencePanelAnimPlaying = false;         // :356
    float sciencePanelAnimCooldown = 0.0f;        // :358
    bool pickRocSampleAnimPlaying = false;        // :360
    float pickRocSampleAnimCooldown = 0.0f;       // :362
    float hammerAnimTimer = 0.0f;                 // :376
    bool alternateIdleDisabled = false;           // :406
    bool hasExpressionSystem = true;              // myExpressionSystem != null (kerbalExpressionSystem is the shell's)
    std::unique_ptr<EvaWeldFx> weldFX;            // :398 (created by enteringConstruction_OnEnter: weldTool == null)
    bool weldToolPresent = false;                 // :396
    bool weldingLasersPresent = false, weldingLasersPlaying = false;  // :400 weldingLasersFX (AudioSource)
    float boundForceMassFactor = 0.533f;          // :110
    float boundColliderYOffsetX = 0.01f, boundColliderYOffsetY = 0.04f;  // :112
    float boundColliderModifierThresholdTime = 0.25f, boundColliderModifierCounter = 0.0f;  // :114-:116
    bool topLadderEndReached = false, bottomLadderEndReached = false;     // LadderEndCheck.Reached (the end-check triggers are the shell's)
    static constexpr int kDefaultEquivalent = 8519681;                    // LayerUtil.DefaultEquivalent (LayerUtil.cs:3)
    std::map<std::string, bool> packModelActive;                          // BackpackTransform / ... gameObject.activeSelf (UpdatePackModels), keyed by name
    bool hammerPrefabPresent = false, hammerMeshEnabled = false, hammerAnimPlaying = false;  // :366-:374 (Serenity ROC hammer)
    float hammerClipLength = 1.0f;                                        // hammerAnimation.clip.length
    int bananaShardCount = 0;                                             // renderer children of bananaShards
    std::vector<std::pair<Vector3f, Vector3f>> bananaShardImpulses;      // the Random draws of SmashingBananaParticles (velocity add, angular add)
    std::string rocDisplayName;                                           // availableROC.displayName
    std::function<std::vector<std::string>(const std::string&)> effectNamesStartingWith;  // part.Effects.EffectsStartingWith
    std::vector<bool> ragdollNodeRootFlags;                               // ragdollNodes[i].rb == base.part.rb
    bool ragdollNodeIsRoot(size_t i) const { return i < ragdollNodeRootFlags.size() && ragdollNodeRootFlags[i]; }
    std::function<double()> partBuoyancyDisplacement;                     // part.partBuoyancy.displacement (unset: part.partBuoyancy == null)
    std::function<float()> realtimeSinceStartup;                          // Time.realtimeSinceStartup (unset: Time.time)
    std::function<Vector3f(Part&)> fromPartVelocity;                      // fromPart.Rigidbody.velocity
    bool helmetTransformPresent = true, neckRingTransformPresent = true;  // helmetTransform / neckRingTransform != null
    bool helmetActiveSelf = true, neckRingActiveSelf = true;              // their gameObject.activeSelf
    bool helmetColliderPresent = true;
    float helmetColliderRadius = 0.27f;
    Vector3f helmetColliderCenter = Vector3f::zero();
    bool hasReferenceTransform = true;                                    // referenceTransform != null
    std::function<float()> astronautComplexLevel;                         // ScenarioUpgradeableFacilities.GetFacilityLevel(SpaceCenterFacility.AstronautComplex)
    std::function<void(Part&)> partExplode;                               // Part.explode()
    // the state names / flags that the shell's HasModule-style queries need
    bool HasColliderLadder = true;

    // ---- state machine (:438-:672). The pointers are into `ownedStates` / `ownedEvents`.
    KFSMState *st_idle_gr = nullptr, *st_walk_acd = nullptr, *st_walk_fps = nullptr, *st_heading_acquire = nullptr, *st_bound_gr_acd = nullptr,
              *st_bound_gr_fps = nullptr, *st_bound_fl = nullptr, *st_run_acd = nullptr, *st_run_fps = nullptr, *st_ragdoll = nullptr,
              *st_recover = nullptr, *st_idle_fl = nullptr, *st_jump = nullptr, *st_land = nullptr, *st_packTransition = nullptr,
              *st_swim_idle = nullptr, *st_swim_fwd = nullptr, *st_ladder_idle = nullptr, *st_ladder_acquire = nullptr, *st_ladder_climb = nullptr,
              *st_ladder_descend = nullptr, *st_ladder_lean = nullptr, *st_ladder_pushoff = nullptr, *st_clamber_acquireP1 = nullptr,
              *st_clamber_acquireP2 = nullptr, *st_clamber_acquireP3 = nullptr, *st_flagAcquireHeading = nullptr, *st_flagPlant = nullptr,
              *st_seated_cmd = nullptr, *st_grappled = nullptr, *st_semi_deployed_parachute = nullptr, *st_fully_deployed_parachute = nullptr,
              *st_idle_b_gr = nullptr, *st_controlPanel_identified = nullptr, *st_picking_roc_sample = nullptr, *st_ladder_end_reached = nullptr,
              *st_playing_golf = nullptr, *st_smashing_banana = nullptr, *st_spinning_wingnut = nullptr, *st_enteringConstruction = nullptr,
              *st_exitingConstruction = nullptr, *st_weldAcquireHeading = nullptr, *st_weld = nullptr;
    KFSMEvent *On_MoveAcd = nullptr, *On_MoveFPS = nullptr, *On_startRun = nullptr, *On_endRun = nullptr, *On_stop = nullptr,
              *On_hdgAcquireStart = nullptr, *On_hdgAcquireComplete = nullptr, *On_stumble = nullptr, *On_recover_start = nullptr,
              *On_jump_start = nullptr, *On_fall = nullptr, *On_land_start = nullptr, *On_MoveLowG_Acd = nullptr, *On_MoveLowG_fps = nullptr,
              *On_bound = nullptr, *On_bound_land = nullptr, *On_bound_fall = nullptr, *On_packToggle = nullptr, *On_feet_wet = nullptr,
              *On_feet_dry = nullptr, *On_swim_fwd = nullptr, *On_swim_stop = nullptr, *On_ladderGrabStart = nullptr,
              *On_ladderGrabComplete = nullptr, *On_ladderClimb = nullptr, *On_ladderDescend = nullptr, *On_ladderStop = nullptr,
              *On_ladderLetGo = nullptr, *On_LadderLand = nullptr, *On_LadderTop = nullptr, *On_LadderEnd = nullptr, *On_LadderLeanStart = nullptr,
              *On_LadderLeanEnd = nullptr, *On_LadderPushOff = nullptr, *On_clamberGrabStart = nullptr, *On_clamberP1 = nullptr,
              *On_clamberP2 = nullptr, *On_clamberP3 = nullptr, *On_boardPart = nullptr, *On_flagPlantStart = nullptr,
              *On_flagPlantHdgAcquire = nullptr, *On_flagPlantFailed = nullptr, *On_flagPlantComplete = nullptr, *On_seatBoard = nullptr,
              *On_seatDeboard = nullptr, *On_seatEject = nullptr, *On_grapple = nullptr, *On_grappleRelease = nullptr,
              *On_semi_deploy_parachute = nullptr, *On_fully_deploy_parachute = nullptr, *On_parachute_cut = nullptr, *On_idle_b_gr = nullptr,
              *On_return_idle = nullptr, *On_control_panel_search = nullptr, *On_control_panel_interacting = nullptr, *On_chopping_roc = nullptr,
              *On_roc_sample_stored = nullptr, *On_ladderEndReached = nullptr, *On_Playing_Golf = nullptr, *On_Golf_Complete = nullptr,
              *On_Smashing_Banana = nullptr, *On_Banana_Complete = nullptr, *On_Spinning_Wingnut = nullptr, *On_Wingnut_Complete = nullptr,
              *On_constructionModeEnter = nullptr, *On_constructionModeExit = nullptr, *On_constructionModeTrigger_gr_Complete = nullptr,
              *On_constructionModeTrigger_fl_Complete = nullptr, *On_weldStart = nullptr, *On_weldHdgAcquired = nullptr, *On_weldComplete = nullptr;
    KFSMTimedEvent *On_recover_complete = nullptr, *On_jump_complete = nullptr, *On_land_complete = nullptr, *On_LadderPushoff_complete = nullptr;

    EvaAnimState* tmpBoundState = nullptr;        // :674
    float tgtBoundStep = 0.0f, boundStepSpeed = 0.0f;  // :691-:693
    float halfHeight = 0.269f;                    // :698
    IEvaFlag* flag = nullptr;                     // :706
    IEvaSeat* kerbalSeat = nullptr;               // :708
    Vector3f tgtRpos, lastTgtRPos, packTgtRPos, packRRot, cmdRot, cmdDir, ladderTgtRPos, flagSpot, parachuteInput;  // :710-:726
    float currentSpd = 0.0f, tgtSpeed = 0.0f, lastTgtSpeed = 0.0f, deltaHdg = 0.0f, lastDeltaHdg = 0.0f, jumpForce = 0.0f;  // :728-:738
    bool boundLeftFoot = false;                   // :740
    bool manualAxisControl = false;               // :742
    Quaternionf rd_rot, rd_tgtRot, manualRotation = Quaternionf::identity(), slopeRotation = Quaternionf::identity();  // :744-:750
    float normalizedBoundTime = 0.0f, colliderHeight = 0.0f;  // :752-:754
    Vector3f tgtFwd, error, integral, derivative, prev_error, tgtUp;  // :768-:778
    bool thrustPercentagePIDBoost = true;         // :780
    float pidBoostThreshold = 55.0f, pidBoostMultiplier = 0.00012f, pidBoostExponent = 2.7f;  // :782-:786
    float linFXLatch = 0.02f, rotFXLatch = 0.01f, linFXMinPower = 0.2f, linFXMaxPower = 0.9f, rotFXMinPower = 0.2f, rotFXMaxPower = 0.9f;  // :812-:822
    Vector3f packLinear;                          // :830
    float fuelFlowRate = 0.0f;                    // :835
    Vector3f fFwd = Vector3f::forward(), fUp = Vector3f::up(), fRgt = Vector3f::right();  // :837-:841
    Vector3f lastCollisionDirection, lastCollisionNormal;  // :846-:848
    bool isAnchored = false;                      // :854
    float kerbalAnchorTimeThreshold = 0.5f, kerbalAnchorTimeCounter = 0.0f;  // :856-:858
    bool isRagdoll = false;                       // :862
    bool canRecover = false;                      // :864
    Vector3d geeForce, coriolisForce, centrifugalForce;  // :866-:870
    int referenceFrameChanged_rdPhysHold = 0;     // :872
    double lastCollisionTime = 0.0;               // :883
    bool canClimb = false, canDescend = false;    // :892-:894
    std::vector<EvaCollider*> currentLadderTriggers;  // :898
    EvaCollider* currentLadder = nullptr;         // :900
    EvaCollider* secondaryLadder = nullptr;       // :902
    Part* _currentLadderPart = nullptr;           // :904
    Vector3f ladderPos, ladderUp, ladderFwd, Vtgt;  // :906-:912
    bool invLadderAxis = false;                   // :914
    bool onLadder = false;                        // :916
    float MinLadderForwardDot = 0.5f, MinLadderRightDot = 0.866f;  // :926-:930
    bool ladderTransition = false;                // :932
    Vector3f clamberOrigin, clamberTarget;        // :934-:936
    EvaRaycastHit clamberHitInfo;                 // :938
    std::unique_ptr<ClamberPath> clamberPath;     // :940
    Vector3f controlOrigin, controlTarget;        // :948-:950
    EvaRaycastHit controlHitInfo;                 // :952
    Part* constructionTarget = nullptr;           // :960 (internal)
    float constructionTargetPivotOffset = 0.05f;  // :962
    bool InConstructionMode = false;              // :1148
    bool wasHelmetEnabledBeforeWelding = true, removeHelmetAfterRaisingVisor = true, wasVisorEnabledBeforeWelding = false;  // :969-:973
    VisorStates visorState = VisorStates::Raised;  // :975
    float visorLoweredTargetOffset = 0.5f, visorRaisedTargetOffset = 0.0f, visorTargetOffset = 0.5f, visorCurrentOffset = 0.0f;  // :978-:984
    float visorTextureOffsetY = 0.0f;             // VisorRenderer.material.mainTextureOffset.y (the renderer is the shell's; the shell syncs this)
    bool replacedGolfBall = false;                // :989
    Vector3f ballForceDir = Vector3f::forward();  // :992
    float ballForce = 0.0f, ballDrag = 0.0f, ballTime = 0.0f, ballAngle = 0.0f, golfSoundTime = 0.0f;  // :994-:1002
    bool playingGolfAnimPlaying = false;          // :1004
    float playingGolfAnimCooldown = 0.0f;         // :1006
    Callback<> afterPlayGolf;                     // :1008
    std::vector<std::string> golfSoundFX;         // :1010
    std::string golfSound;                        // :1012
    bool golfSoundPlayed = false;                 // :1014
    bool replacedBanana = false;                  // :1017
    float bananaTime = 0.595f, bananaForce = 1.5f, bananaSoundTime = 0.59f;  // :1020-:1026
    Callback<> afterBanana;                       // :1028
    bool smashingBananaAnimPlaying = false;       // :1030
    float smashingBananaAnimCooldown = 0.0f;      // :1032
    std::vector<std::string> bananaSoundFX;       // :1036
    std::string bananaSound;                      // :1038
    bool bananaSoundPlayed = false;               // :1040
    float wingnutKerbalTorqueForce = 0.5f, wingnutTransitionTime = 4.5f, wingnutTorqueTime = 0.71f;  // :1043-:1049
    Callback<> afterWingnut;                      // :1051
    bool spinningWingnutAnimPlaying = false;      // :1053
    float spinningWingnutAnimCooldown = 0.0f;     // :1055
    bool appliedTorque = false;                   // :1057
    EvaCollider* currentAirlockTrigger = nullptr;  // :1063
    Part* currentAirlockPart = nullptr;           // :1065
    std::string helmetUnsafeReason;               // :1103
    bool atmosExistence = false;                  // :1105
    double kerbalStaticPressureAtm = 0.0, kerbalSkinTemp = 0.0, kerbalInternalTemp = 0.0;  // :1107-:1111
    int framesDelayForHelmetDeathCheck = 5, framesDelayForHelmetDeathCounter = 0;  // :1114-:1116
    Vector3f ladderPosition;                      // :1124
    bool isLadderJointed = false;                 // :436
    bool Ready = false;                           // :1126
    float PIDBoost = 1.0f;                        // :1177 (set to 1 in OnAwake)
    // Serenity-only part of the machine: prop / science objects are the shell's; these are their bookkeeping flags
    bool golfClubPresent = false, bananaPropPresent = false, wingnutPropPresent = false, hammerPresent = false;
    bool availableROCPresent = false, experimentROCPresent = false, availableROCSmall = true;  // ROC availability (the ROC component is Serenity's)
    // test / shell inputs: the ground science part under the clamber / control-panel ray (Part.GetComponentUpwards<ModuleGroundSciencePart>)
    std::function<bool(EvaCollider&, bool* deployedOnGround)> groundSciencePartAt;
    bool sciencePartPresent = false;
    // engine inputs that are not part of IEvaEngine because Vessel / Part carry them: sloped-ground hit under the kerbal (vessel.HeightFromSurfaceHit)
    Vector3f heightFromSurfaceHitNormal = Vector3f::up();
    float heightFromSurfaceHitDistance = 0.0f;
    bool vesselSurfaceAnchored = true;            // vessel.VesselSurface.IsAnchored (VesselSurface != null)
    // Part.currentCollisions: layers of the parts' current collisions (GetL19Contact reads layer 19)
    std::vector<int> currentCollisionLayers;
    // the experience / science containers of vessel parts the boarding check scans are Part modules; IScienceDataContainer of the C# is the host's:
    std::function<int(Part&)> scienceDataCount;   // IScienceDataContainer.GetScienceCount summed over a part's modules (unset: 0)
    std::function<bool(Part& boardTarget, Part& evaPart)> storeScienceInContainer;  // ModuleScienceContainer.StoreData(list, dumpRepeats: false)
    std::function<void(const std::string& title, std::function<void()> proceed)> popupStoreExperimentsIssue;  // PopupDialog (the dialog is UI)
    bool boardingPopupShown = false;

    // ======================================================================== properties (:1126-:1235)
    bool IsChuteState() const;                                     // :1130
    bool PartPlacementMode() const { return partPlacementMode; }   // :1142
    ModuleInventoryPart* ModuleInventoryPartReference() const { return moduleInventoryPartReference; }  // :1144
    bool IsLadderJointed() const { return isLadderJointed; }       // :1150
    bool VesselUnderControl() const;                               // :1152
    int flagItems() const { return _flags(); }                     // :1164 getter
    void set_flagItems(int value);                                 // :1164 setter
    double Fuel() const;                                           // :1179
    double FuelCapacity() const;                                   // :1181
    Part* currentLadderPart() const { return _currentLadderPart; } // :1183 getter
    void set_currentLadderPart(Part* value);                       // :1183 setter (onPartLadderExit / Enter)
    Part* CurrentLadderPart() const { return _currentLadderPart; } // :1203
    bool OnALadder_() const;                                       // :1205
    Part* LadderPart() override { return currentLadderPart(); }    // :1217 (IEvaController)
    bool OnALadder() override { return OnALadder_(); }             // :1205 (IEvaController)
    VisorStates VisorState() const { return visorState; }          // :1219
    bool IsVisorEnabled() const { return isVisorEnabled(); }       // :1221
    const std::string& HelmetUnsafeReason() const { return helmetUnsafeReason; }  // :1223

    // ======================================================================== PartModule lifecycle (:1426-:1846)
    void OnAwake() override;                                       // :6906
    void OnLoad(const ConfigNode& node) override;                  // :1426
    void OnStart(StartState state) override;                       // :1505
    void OnStartFinished(StartState state) override;               // :1673
    void OnSave(ConfigNode& node) override;                        // :1846
    void OnInactive() override;                                    // :6964
    void OnDestroy();                                              // :6915
    Enumerator StartEVA(StartState state);                         // :1753 (iterator; started by OnStart on the scheduler)
    void Update();                                                 // :4422
    void FixedUpdate();                                            // :4370
    void LateUpdate();                                             // :4493

    // ======================================================================== setup helpers
    void SetupFSM();                                               // :3268
    void SetupAnimations();                                        // :1968
    void SetupRagdoll(Part& part);                                 // :5336
    void ResetRagdollLinks();                                      // :5626
    void SetupJetpackEffects();                                    // :4915
    void SetupEVAFuel();                                           // :1344
    void ProcessEVAFuel();                                         // :1379
    void UpdatePackModels();                                       // :1249
    void OnModuleInventoryChanged(ModuleInventoryPart* moduleInventory);  // :1237
    void OnVesselChange(Vessel* vsl);                              // :1875
    void OnPartEvent(Part* part);                                  // :1892
    void UpdateInventoryPaw(KFSMState* oldState, KFSMState* newState, KFSMEvent* fsmEvent);  // :1900
    void OnFSMStateChange(KFSMState* oldState, KFSMState* newState, KFSMEvent* fsmEvent);    // :4354
    void OnFSMEventCalled(KFSMEvent* fsmEvent);                    // :4362
    void SetPortraitCameraDistance(float newDistance);             // :1908
    void SetVisibleInPortrait(bool visible);                       // :1948
    void CacheLocalStrings();                                      // :6979
    void SyncEvaChuteHost();                                       // C++: refreshes ModuleEvaChute::kerbalEVA (the KerbalEVA queries the chute makes)

    // ======================================================================== state callbacks (:1973-:3266)
    void idle_OnEnter(KFSMState* st);
    void idle_OnLeave(KFSMState* st);
    void idle_b_OnEnter(KFSMState* st);
    void idle_b_OnLeave(KFSMState* st);
    void walk_Acd_OnEnter(KFSMState* st);
    void walk_ccd_OnLeave(KFSMState* st);
    void walk_fps_OnUpdate();
    void walk_fps_OnLeave(KFSMState* st);
    void heading_acquire_OnEnter(KFSMState* st);
    void heading_acquire_OnLeave(KFSMState* st);
    void run_acd_OnEnter(KFSMState* st);
    void run_acd_OnLeave(KFSMState* st);
    void run_fps_OnUpdate();
    void run_fps_OnLeave(KFSMState* st);
    void bound_gr_acd_OnEnter(KFSMState* st);
    void bound_gr_acd_OnUpdate();
    void bound_gr_acd_OnLeave(KFSMState* st);
    void bound_gr_fps_OnEnter(KFSMState* st);
    void bound_gr_fps_OnUpdate();
    void bound_gr_fps_OnLeave(KFSMState* st);
    void bound_fl_OnEnter(KFSMState* st);
    void bound_fl_OnLeave(KFSMState* st);
    void ragdoll_OnEnter(KFSMState* st);
    void ragdoll_OnLeave(KFSMState* st);
    void recover_OnEnter(KFSMState* st);
    void recover_OnUpdate();
    void controlPanel_identified_OnEnter(KFSMState* st);
    void picking_roc_sample_OnEnter(KFSMState* st);
    void spawnHammer();
    void jump_OnEnter(KFSMState* st);
    void idle_fl_OnEnter(KFSMState* st);
    void idle_fl_OnLeave(KFSMState* st);
    void land_OnEnter(KFSMState* st);
    void land_OnUpdate();
    void swim_idle_OnEnter(KFSMState* st);
    void swim_idle_OnLeave(KFSMState* st);
    void swim_fwd_OnEnter(KFSMState* st);
    void swim_fwd_OnLeave(KFSMState* st);
    void ladder_acquire_OnEnter(KFSMState* st);
    void ladder_acquire_OnLeave(KFSMState* st);
    void ladder_idle_OnEnter(KFSMState* st);
    void ladder_idle_OnLeave(KFSMState* st);
    void ladder_lean_OnEnter(KFSMState* st);
    void ladder_lean_OnLateUpdate();
    void ladder_lean_OnLeave(KFSMState* st);
    void ladder_climb_OnEnter(KFSMState* st);
    void ladder_climb_OnLeave(KFSMState* st);
    void ladder_descend_OnEnter(KFSMState* st);
    void ladder_descend_OnLeave(KFSMState* st);
    void ladder_end_reached_OnEnter(KFSMState* st);
    void ladder_end_reached_OnLeave(KFSMState* st);
    void ladder_pushoff_OnLeave(KFSMState* st);
    void clamber_acquireP1_OnEnter(KFSMState* st);
    void ZeroRBVelocity();
    void clamber_acquireP1_OnLeave(KFSMState* st);
    void clamber_acquireP2_OnEnter(KFSMState* st);
    void clamber_acquireP2_OnLeave(KFSMState* st);
    void clamber_acquireP3_OnEnter(KFSMState* st);
    void clamber_acquireP3_OnLeave(KFSMState* st);
    void flagAcquireHeading_OnEnter(KFSMState* st);
    void flagAcquireHeading_OnLateUpdate();
    void flagAcquireHeading_OnLeave(KFSMState* st);
    void flagPlant_OnEnter(KFSMState* st);
    void flagPlant_OnLeave(KFSMState* st);
    void seated_cmd_OnEnter(KFSMState* st);
    void seated_cmd_OnLeave(KFSMState* st);
    void grappled_OnEnter(KFSMState* st);
    void grappled_OnLeave(KFSMState* st);
    void EnterConstructionMode();                                  // :2953 internal
    void ExitConstructionMode();                                   // :2962 internal
    void enteringConstruction_OnEnter(KFSMState* st);
    void exitingConstruction_OnEnter(KFSMState* st);
    void enteringExitingConstruction_OnFixedUpdate();
    void exitingConstruction_OnLeave(KFSMState* st);
    void ToggleWeldingGun(bool toggle);                            // :3036
    void weld_acquireHeading_OnEnter(KFSMState* st);
    void weld_acquireHeading_OnFixedUpdate();
    void weld_acquireHeading_OnLateUpdate();
    void weld_acquireHeading_OnLeave(KFSMState* st);
    void weld_OnEnter(KFSMState* st);
    bool HasWeldLineOfSight();                                     // :3146
    bool HasWeldLineOfSight(const Vector3f& gunPoint, const Vector3f& partAttachPoint);  // :3180
    void InterruptWeld();                                          // :3189
    void OnWeldStart(KerbalEVA* kerbalEVA);                        // :3199
    static void LockInput(uint64_t mask, const std::string& id);   // InputLockManager.SetControlLock
    static void UnlockInput(const std::string& id);                // InputLockManager.RemoveControlLock
    static constexpr uint64_t kAllButUiCamera = ~(0xF000000000000000ULL | 0x400ULL);  // ~(ControlTypes.UI | ControlTypes.CAMERACONTROLS)
    static double UT();
    void OnWeldFinish(KerbalEVA* kerbalEVA);                       // :3207
    void weld_OnFixedUpdate();
    void weld_OnLeave(KFSMState* st);
    void OnSemiDeployedParachuteModeEntered(KFSMState* st);        // :4695
    void UpdateSemiDeployedParachuteMovement();                    // :4702
    void OnSemiDeployedParachuteModeLeft(KFSMState* st);           // :4707
    void OnFullyDeployedParachuteModeEntered(KFSMState* st);       // :4725
    void UpdateFullyDeployedParachuteMovement();                   // :4734
    void OnFullyDeployedParachuteModeLeft(KFSMState* st);          // :4747
    void UpdateParachuteInCommandSeat();                           // :4671
    void OnParachuteSemiDeployed();                                // :4690 internal
    void OnParachuteFullyDeployed();                               // :4715 internal
    void OnParachuteCut();                                         // :4762 internal
    bool IsKerbalInStateAbleToDeployParachute();                   // :4662 internal
    void playing_Golf_OnEnter(KFSMState* st);
    void SpawnGolf();
    void PlayingGolfPhysicalBall();
    void FinishedPlayingGolf();
    void smashing_banana_OnEnter(KFSMState* st);
    void SpawnBanana();
    void SmashingBananaParticles();
    void FinishedSmashingBanana();
    void spinning_Wingnut_OnEnter(KFSMState* st);
    void SpawnWingnut();
    void ApplyWingnutTorque();
    void SpinningWingnutForever();
    void FinishedSpinningWingnut();
    void PlayGolf(Callback<> afterGolfCallback);                   // :6226
    void Banana(Callback<> afterBananaCallback);                   // :6342
    void Dzhanibekov(Callback<> afterWingnutCallback);             // :6484
    void SetupEVAScienceSoundFX();                                 // :6609

    // ======================================================================== movement / physics (:4502-:5100, :5148-:5800)
    void HandleMovementInput();                                    // :4502
    void SetWaypoint(const Vector3f& tgtPos);                      // :4655
    void UpdateMovement();                                         // :4776
    void UpdateHeading();                                          // :4793
    void UpdateLowGBodyColliders();                                // :4820
    void ResetOrientationPID();                                    // :4845
    void UpdateOrientationPID();                                   // :4853
    void UpdatePackAngular();                                      // :4873
    void correctGroundedRotation();                                // :4882
    void StartGroundedRotationRecover();                           // :4895
    void RecoverGroundedRotation(float duration);                  // :4901
    void SetFXGroupMinMaxPower();                                  // :4932
    void updateJetpackEffects();                                   // :4948
    void ToggleLamp();                                             // :4990
    void ToggleJetpack();                                          // :5012
    void ToggleJetpack(bool packState);                            // :5021
    void UpdatePackLinear();                                       // :5047
    void UpdatePackFuel();                                         // :5060
    void getCoordinateFrame();                                     // :5095
    void PickUpROC();                                              // :5112
    void OnROCExperimentFinished(ScienceData* experimentData);     // :5130
    void OnROCExperimentReset(ScienceData* experimentData);        // :5140
    struct EvaContact {
        Vector3f point, normal;
    };
    struct EvaCollision {
        int layer = 0;
        bool isRoc = false;
        float relativeVelocitySqrMagnitude = 0.0f;
        EvaCollider* collider = nullptr;
        std::vector<EvaContact> contacts;
    };
    void OnCollisionEnter(EvaCollision& c);                        // :5148
    int CompareContactsByNormalToSurface(const EvaContact& c1, const EvaContact& c2);  // :5185
    void OnCollisionStay(EvaCollision& c);                         // :5194
    void OnCollisionExit(EvaCollision& c);                         // :5199
    bool isSelfCollision(const EvaCollision& c);                   // :5209
    std::vector<EvaCollider*> characterColliders, otherRagdollColliders;  // :165-:167 (identity lists for isSelfCollision)
    std::vector<EvaCollider*> ragdollNodeColliders;                // ragdollNodes[i].collider
    bool SurfaceContact();                                         // :5243
    bool SurfaceOrSplashed();                                      // :5252
    bool GetL19Contact();                                          // :5261
    void AnchorUpdate();                                           // :5277
    void AddRBAnchor();                                            // :5317
    void RemoveRBAnchor();                                         // :5326
    void EnableCharacterAndLadderColliders(bool enable);           // :5361
    void SetRagdoll(bool ragDoll, bool preservePose = false);      // :5374
    void IntegrateRagdollRigidbodyForces();                        // :5422
    void updateRagdollVelocities();                                // :5459
    void CalculateGroundLevelAngle();                              // :5471
    int slopeMovementDirection = 0;                                // :121
    using HostedSituation = events::GameEvents::HostedFromToAction<Vessel*, Vessel_Situations>;
    using BodyChange = events::GameEvents::FromToAction<CelestialBody*, CelestialBody*>;
    using FrameChange = events::GameEvents::HostTargetAction<CelestialBody*, bool>;
    void OnVesselSituationChange(HostedSituation vcs);             // :5498
    void onReferencebodyChanged(BodyChange rChg);                  // :5506
    void onRotatingFrameChanged(FrameChange frm);                  // :5526
    void onFrameVelocityChange(const Vector3d& velOffset);         // :5552
    void OnVesselGoOnRails(Vessel* v);                             // :5566
    void OnVesselGoOffRails(Vessel* v);                            // :5581
    Enumerator waitAndHandleRagdollTimeWarp(int waitFrames);       // :5593
    void OnPartDie();                                              // :5641
    void Splat(const Vector3f& point, const Vector3f& normal);     // :5651
    bool CanRecover();                                             // :5657
    void SetEjectDirection();                                      // :5671
    void EjectFromSeat();                                          // :5696
    Enumerator AcquireRotation(Quaternionf tgtRot, float duration);  // :5717
    Enumerator AcquirePosition(Vector3f tgtPos, float duration);   // :5733
    void ModifyBodyColliderHeight(float newHeight);                // :7345
    float HeightFromSurface();                                     // vessel.GetHeightFromSurface() (the vessel's ray: physics bubble; unset: the field)
    std::function<float()> vesselGetHeightFromSurface;

    // ---- ladders (:5749-:5973, :7354)
    void correctLadderPosition();                                  // :5749
    void correctLadderRotation();                                  // :5776
    void UpdateLadderMovement();                                   // :5787
    void InterpolateLadders();                                     // :5802
    void AutoTransition();                                         // :5814
    void UpdateCurrentLadder();                                    // :5858
    void UpdateCurrentLadderIdle();                                // :5895
    void CheckLadderTriggers();                                    // :5935
    int SortTriggersByDistance(EvaCollider* c1, EvaCollider* c2);  // :5955 (returns -1 / 1: List.Sort comparison)
    int SortTriggersByAlignment(EvaCollider* c1, EvaCollider* c2); // :5966
    Enumerator StartNonCollidePeriod(float duration, float standoff, Part* fromPart, const PartTransform* airlockTrf);  // :5975
    void SetLadderHold();                                          // :7354
    void ClearLadderHold();                                        // :7364
    void OnTriggerStay(EvaCollider* o);                            // :6860
    void OnTriggerExit(EvaCollider* o);                            // :6881
    // ---- clamber / control panel (:6001-:6125)
    bool FindClamberSrf(float fwdOffset, float reach);             // :6001
    bool TestClamberSrf(EvaRaycastHit& hit);                       // :6016
    std::unique_ptr<ClamberPath> GetClamberPath(float fwdOffset, float reach);  // :6034
    bool FindControlPanel(float fwdOffset, float reach);           // :6066
    void ControlPanelInteractionFinished();                        // :6087
    void RandomControlPanelAnim();                                 // :6102
    void RocSampleStored();                                        // :6107

    // ======================================================================== interaction (:6127-:6860)
    void Weld(Part* targetPart);                                   // :6127
    void LowerVisor(bool forceHelmet);                             // :6141
    void RaiseVisor(bool restoreHelmet);                           // :6156
    void UpdateVisorPosition();                                    // :6164
    void OnVisorRaised(KerbalEVA* kerbal);                         // :6199
    void UpdateVisorEventStates();                                 // :6208
    void LowerVisor() { LowerVisor(false); }                       // :6215 [KSPEvent]
    void RaiseVisor() { RaiseVisor(false); }                       // :6221 [KSPEvent]
    void BoardPart(Part* p);                                       // :6615
    bool processInventory(Part* p, bool storeParts);               // :6635
    bool checkExperiments(Part* p);                                // :6646
    void proceedAndBoard(Part* p);                                 // :6698
    void PlantFlag();                                              // :6723 [KSPEvent]
    bool CanPlantFlag();                                           // :6729
    void AddFlag(int flagCount = 1);                               // :6738
    void MakeReference();                                          // :6745 [KSPEvent]
    bool BoardSeat(IEvaSeat* seat);                                // :6757
    bool IsSeated();                                               // :6770
    void OnGrapple();                                              // :6775
    void OnDeboardSeat();                                          // :6781 [KSPEvent]
    void RestoreVesselInfo(float delay);                           // :6793
    Enumerator restoreVesselInfo_afterWait(float delay);           // :6798
    void SwitchFocusIfActiveVesselUncontrollable(float delay);     // :6812
    Enumerator swichFocusIfActiveVesselUncontrollable_delay(float delay);  // :6817
    Vector3f GetEjectPoint(const Vector3f& ejectDirection, float maxDist, float capsuleRadius, float capsuleHeight);  // :6827
    void RenameVessel();                                           // :6855 [KSPEvent]
    void PostInteractionScreenMessage(const std::string& message, float delay = 0.1f);  // :6898
    // ---- helmet (:7001-:7252)
    void InitHelmetSetup();                                        // :7001
    void ToggleHelmet(bool enableHelmet);                          // :7037
    void ToggleNeckRing(bool enableNeckRing);                      // :7042
    void ToggleHelmetAndNeckRing(bool enableHelmet, bool enableNeckRing, bool storeToPCM = true, bool suppressTransformMessages = false);  // :7047
    void ChangeHelmet();                                           // :7110 [KSPEvent]
    void ChangeNeckRing();                                         // :7116 [KSPEvent]
    void OnHelmetChanged(KerbalEVA* changedKerbal, bool helmetVisible, bool neckRingVisible);  // :7124
    bool CheckHelmetOffSafe(bool includeSafetyMargins = true, bool startEVAChecks = false);  // :7158
    double GetPreLoadPressure(double loadValue, CelestialBody& vesselBody);  // :7208
    bool CanEVAWithoutHelmet();                                    // :7223
    bool CanSafelyRemoveHelmet();                                  // :7228
    bool WillDieWithoutHelmet();                                   // :7233
    void UpdateHelmetOffChecks();                                  // :7238
    // helmet seams: the loading screen test (LoadingBufferMask) and the part-level reads of CheckHelmetOffSafe
    std::function<bool()> loadingBufferMaskActive;                 // LoadingBufferMask.Instance.loadingObject.gameObject.activeInHierarchy (unset: false)
    // physic material (the material objects are the engine's)
    struct PhysicMaterialData {
        float dynamicFriction = 0.6f, staticFriction = 0.6f, bounciness = 0.0f;
        int frictionCombine = 0, bounceCombine = 0;
    };
    std::optional<PhysicMaterialData> physicMaterial;              // :1119
    void SetPhysicMaterial(const PhysicMaterialData& newPhysicMaterial);  // :7254
    void ResetPhysicMaterial();                                    // :7272
    Enumerator ReturnToIdle(float time);                           // :7290
    bool SetPartPlacementMode(bool mode, ModuleInventoryPart* moduleInventoryPartReference);  // :7296
    Color GetCurrentColor_();                                      // :7315
    void OnColorChanged(const Color& color);                       // :7330
    void UpdateSuitColors();                                       // :4393
    // the seams the ModuleColorChanger / suit renderers need (cosmetic)
    std::function<void(float r, float g, float b)> suitColorChanged;
    std::function<void(Vessel&)> vesselGetGroundLevelAngle;       // Vessel.GetGroundLevelAngle() (the vessel's ground ray: physics bubble)

    // ======================================================================== C++ host API
    // Coroutines (MonoBehaviour.StartCoroutine on the scheduler); unset scheduler: the process one used by ModuleCargoPart
    std::shared_ptr<Coroutine> StartCoroutine(Enumerator e);
    void StopCoroutine(std::shared_ptr<Coroutine>& c);
    int CoroutineOwner() const { return static_cast<int>(reinterpret_cast<uintptr_t>(this) >> 4); }
    Part* PartPtr() const;
    Vessel* VesselPtr() const;
    // the kerbal's Transform (= part.transform)
    PartTransform& transform();
    // Keys / axes through EvaEnvironment (GameSettings bindings by default)
    static bool Key(EvaKey k, KeyQuery q);
    static float Axis(EvaAxis a);
    // the structure SetupFSM built, by name (unit gate)
    const std::map<std::string, EvaStateWiring>& StateWiring() const { return stateWiring_; }
    const std::map<std::string, EvaEventWiring>& EventWiring() const { return eventWiring_; }
    // snapshot the wiring from the live FSM (state event lists) after SetupFSM
    void RecordEventWiring();
    // Installs the WP-33 / WP-53 / WP-25 hook statics that call into the (single) active kerbal: SetPartPlacementMode, JetpackDeployed,
    // ToggleJetpack, Weld, InterruptWeld, IsSeated, EnterConstructionMode / ExitConstructionMode, GrappleEnvironment::evaOnGrapple,
    // ModuleCargoPart weld sparks. `resolve` maps a vessel (or part) to its KerbalEVA; the default scans Vessel::evaController.
    static void InstallHostHooks();
    static void UninstallHostHooks();
    static KerbalEVA* Of(Vessel* v);
    static KerbalEVA* Of(Part* p);
    // live kerbals (the "GetComponent<KerbalEVA>()" registry)
    static const std::vector<KerbalEVA*>& Live() { return live_; }

    // ======================================================================== the C# string cache (CacheLocalStrings)
    std::string cacheAutoLOC_114130, cacheAutoLOC_114293, cacheAutoLOC_114297, cacheAutoLOC_114358, cacheAutoLOC_115662, cacheAutoLOC_115694,
        cacheAutoLOC_6010008, cacheAutoLOC_6010009, cacheAutoLOC_6010010, cacheAutoLOC_6010011, cacheAutoLOC_8003204, cacheAutoLOC_6010015,
        cacheAutoLOC_8002357, cacheAutoLOC_8002358, cacheAutoLOC_8002359, cacheAutoLOC_8002360, cacheAutoLOC_6012032, cacheAutoLOC_6006049;

    IEvaEngine& Engine();                                          // the seam (a HeadlessEvaEngine is created when none was assigned)

private:
    friend class KerbalAnimationManager;
    // state / event factories (SetupFSM helpers)
    KFSMState* NewState(const std::string& name);
    KFSMEvent* NewEvent(const std::string& name);
    KFSMTimedEvent* NewTimedEvent(const std::string& name, double time);
    void HookEnter(KFSMState* st, void (KerbalEVA::*fn)(KFSMState*), const char* name);
    void HookLeave(KFSMState* st, void (KerbalEVA::*fn)(KFSMState*), const char* name);
    void HookUpdate(KFSMState* st, void (KerbalEVA::*fn)(), const char* name);
    void HookFixed(KFSMState* st, void (KerbalEVA::*fn)(), const char* name);
    void HookLate(KFSMState* st, void (KerbalEVA::*fn)(), const char* name);
    void SetupFSMStates();
    void SetupFSMEvents();
    void SetupFSMEvents2();

    std::vector<std::unique_ptr<KFSMState>> ownedStates_;
    std::vector<std::unique_ptr<KFSMEvent>> ownedEvents_;
    std::map<std::string, EvaStateWiring> stateWiring_;
    std::map<std::string, EvaEventWiring> eventWiring_;
    std::unique_ptr<HeadlessEvaEngine> defaultEngine_;
    std::shared_ptr<Coroutine> returnIDLE_, startExpression_, updateAvatarCoroutine_;
    std::shared_ptr<Coroutine> startEvaCoroutine_;
    bool eventsSubscribed_ = false;
    bool localStringsEvent_ = false;
    std::unique_ptr<PartResource> propellantOwned_;  // the PartResource C# keeps after Resources.Remove (SetupEVAFuel)
    static inline std::vector<KerbalEVA*> live_;
};

}  // namespace ksp
