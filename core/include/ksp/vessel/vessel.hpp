// WP-14: Vessel (Vessel.cs, 4,664 lines), the vessel model.
//
// ksp::Vessel is the data model and engine-agnostic logic of the MonoBehaviour: identity and naming, the part list and rootPart,
// the reference transform, situations (checkLanded / checkSplashed / updateSituation, landedAt), the kinematic summary
// (UpdatePosVel: lat/lon/alt, obt/srf velocity, speeds, up/north/east), mass (GetTotalMass, the VesselPrecalculate
// CalculatePhysicsStats centre-of-mass / MOI maths over part rigid-body data), atmosphere values mirrored from the flight
// integrator, crew (RebuildCrewList, capacity, count), control level (GetControlLevel / CheckControllable), control state and
// action-group overrides, vessel modules, the orbit driver hookup (IOrbitDriverVessel) and the explicit state transitions
// Initialize / StartFromBackup / Load / Unload / BackupVessel / GoOnRails / GoOffRails with no physics.
//
// Unity, physics and UI that Vessel.cs does around that are NOT here (STATUS.md "WP-14" lists them by line range): rigid-body
// anchoring and joints, FixedUpdate / Update / LateUpdate integration (WP-21 / WP-20), ground raycasts, renderers and map
// objects, screen messages, input locks, the rename dialog. Where the logic needs a service another package owns, the Vessel
// reaches it through `Vessel::env` (VesselEnvironment: std::function hooks, unset = the answer for "not available").
//
// Naming: members keep KSP's names. C# properties become functions (`orbit()`, `mainBody()`, `LandedOrSplashed()`,
// `isCommandable()`, `isActiveVessel()`, `CurrentControlLevel()`, `IsControllable()`). A Vessel has an address (parts, modules and
// the orbit driver point at it): create it with std::make_unique and never move it.
//
// Ownership: the Vessel owns its parts once adopted (AdoptParts / Load); `parts` is the non-owning list of the C# field.
#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ksp/celestial/flight_globals.hpp"
#include "ksp/config/config_node.hpp"
#include "ksp/config/kspparseable.hpp"
#include "ksp/core/delegate.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/math/quaterniond.hpp"
#include "ksp/math/vector3d.hpp"
#include "ksp/modules/vessel_module.hpp"
#include "ksp/orbit/orbit.hpp"
#include "ksp/orbit/orbit_driver.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/physics/vessel_ranges.hpp"
#include "ksp/save/proto_vessel.hpp"
#include "ksp/save/save_enums.hpp"
#include "ksp/unity/color.hpp"
#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/transform.hpp"
#include "ksp/unity/vector.hpp"
#include "ksp/vessel/action_group_list.hpp"
#include "ksp/vessel/discovery_info.hpp"
#include "ksp/vessel/flight_ctrl_state.hpp"
#include "ksp/vessel/vessel_types.hpp"
#include "ksp/vessel/vessel_precalculate.hpp"
#include "ksp/vessel/vessel_values.hpp"

namespace ksp {
namespace CommNet {
class CommNetVessel;
}  // namespace CommNet

// GameSettings values Vessel.cs reads (GameSettings.cs:978, 1008; the shim namespace is shared with ksp_util.hpp)
namespace GameSettings {
inline bool ADDITIONAL_ACTION_GROUPS = false;
inline bool RADAR_ALTIMETER_EXTENDED_CALCS = true;
inline bool ADVANCED_TWEAKABLES = false;  // GameSettings.cs:75 / :979 (BaseAction.AddAction, WP-16)
}  // namespace GameSettings

class CelestialBody;
struct FlightIntegratorAtmosphere;
enum class ClearToSaveStatus : int;  // ClearToSaveStatus.cs (defined by TimeWarp, WP-10)
class FlightState;
class PatchedConicSolver;
class VesselAutopilot;  // WP-24 ksp/vessel/vessel_autopilot.hpp
class Vessel;

// KerbalEVA (WP-32), as far as Vessel.cs reads it: the part the kerbal holds on to (EVALadderVessel, updateSituation), and
// OnALadder for TimeWarp (TimeWarp.cs:511 through IFlightVessel)
class IEvaController {
public:
    virtual ~IEvaController() = default;
    virtual Part* LadderPart() = 0;  // KerbalEVA.LadderPart
    virtual bool OnALadder() { return false; }  // KerbalEVA.cs:1205 (false while the FSM has not started)
};

// What the physics engine reports for one part's Rigidbody (Part.rb): the inputs of VesselPrecalculate.CalculatePhysicsStats
// (VesselPrecalculate.cs:387-498). `FromPart` fills what the part tree knows; velocities and the inertia tensor come from the engine.
struct PartRigidbodyState {
    bool present = false;                 // part.rb != null
    double physicsMass = 0.0;             // Part.physicsMass (== rb.mass)
    float mass = 0.0f;                    // rb.mass
    Vector3f worldCenterOfMass;           // rb.worldCenterOfMass
    Vector3f position;                    // rb.position
    Vector3f velocity;                    // rb.velocity
    Vector3f angularVelocity;             // rb.angularVelocity (world frame)
    Vector3f inertiaTensor;               // rb.inertiaTensor (principal moments)
    Quaternionf inertiaTensorRotation = Quaternionf::identity();  // rb.inertiaTensorRotation
    Quaternionf rotation = Quaternionf::identity();               // part.transform.rotation
    // Part.CoMOffset rotated into world space around the part transform (rb.centerOfMass = CoMOffset), no engine involved
    static PartRigidbodyState FromPart(const Part& p);
};

// VesselPhysics::ReadState() (PHYSICS-SPIKE.md 5.3, WP-20b): the vessel's rigid-body state in the DOUBLE physics frame, which is what
// VesselPrecalculate.CalculatePhysicsStats (VesselPrecalculate.cs:387-498) derives from the part Rigidbodies' float fields.
struct VesselPhysicsState {
    double totalMass = 0.0;       // sum of physicsMass over the parts that have a rigid body; <= 0 selects the mass-less branch
    Vector3d CoMD;                // world centre of mass (physics frame)
    Vector3d rb_velocityD;        // CoM velocity in the physics frame (Krakensbane's frame velocity is added by CalculatePhysicsStats)
    Vector3d angularVelocityD;    // Quaternion.Inverse(ReferenceTransform.rotation) * rb.angularVelocity, mass-weighted
    Vector3f MOI;                 // diagonal of the summed inertia tensor in the reference frame (KSPUtil.Diag(left))
};

// WP-20e: a Physics.Raycast hit as Vessel.CheckGroundCollision reads it (world frame, Unity floats)
struct VesselRayHit {
    bool hit = false;
    float distance = 0.0f;
    Vector3f point;
    Vector3f normal;
    bool isPart = false;               // FlightGlobals.GetPartUpwardsCached(hit.collider.transform.gameObject) != null
    bool isMobileLaunchPad = false;    // hit.transform.GetComponentInParent<PositionMobileLaunchPad>() != null
    std::string tag;                   // hit.collider.gameObject.tag (objectUnderVessel)
};

// Services of other packages / the engine that Vessel.cs reaches through statics. Every hook is optional; the comment says what an
// unset hook means.
struct VesselEnvironment {
    // HighLogic.LoadedSceneIsFlight
    bool LoadedSceneIsFlight = true;
    // Time.frameCount / Time.deltaTime / Time.timeSinceLevelLoad
    int frameCount = 0;
    float deltaTime = 0.02f;
    float timeSinceLevelLoad = 10.0f;
    // PSystemSetup.Instance.IsFacilityOrLaunchSite(landedAt, out isFacility, out displayName): unset = not a facility
    std::function<bool(const std::string& landedAt, bool& isFacility, std::string& displayName)> isFacilityOrLaunchSite;
    // ResearchAndDevelopment.GetMiniBiomedisplayNameByUnityTag(tag, formatted: false); unset = ""
    std::function<std::string(const std::string& unityTag)> miniBiomeDisplayNameByUnityTag;
    // ScienceUtil.GetBiomedisplayName(body, biome, formatted: false); unset = the biome name itself
    std::function<std::string(CelestialBody&, const std::string&)> biomeDisplayName;
    // mainBody.pqsController: minLevel / maxLevel and GetSurfaceHeight(relSurfaceNVector[, overrideQuadBuildCheck]) (WP-11 PQS).
    // unset = the body has no pqsController (PQSAltitude then returns -1)
    std::function<bool(CelestialBody&, int& minLevel, int& maxLevel)> pqsLevels;
    std::function<double(CelestialBody&, const Vector3d& relSurfaceNVector)> pqsSurfaceHeight;
    // Physics raycast of Vessel.GetHeightFromTerrain (WP-20): unset = no hit (heightFromTerrain stays -1)
    std::function<std::optional<float>(Vessel&, float maxDistance, Vector3f& hitNormalWorld)> heightFromTerrainProbe;
    // Vessel.CheckGroundCollision (WP-20): unset = true (the vessel is on the ground). WP-20e's contact host sets it to
    // Vessel::CheckGroundCollision, which reads the two hooks below.
    std::function<bool(Vessel&)> checkGroundCollision;
    // WP-20e: Physics.Raycast(origin, direction, out hit, maxDistance, 0x8000 | LayerUtil.DefaultEquivalent) (terrain and parts, world
    // frame) of CheckGroundCollision: unset = no hit
    std::function<VesselRayHit(Vessel&, const Vector3f& origin, const Vector3f& direction, float maxDistance)> groundRaycast;
    // WP-20e: getLowestPoint's collider scan (Vessel.cs:2589-2621): the z of collider.ClosestPoint(point) for every collider of the part (the
    // non-trigger, enabled colliders on the layers getLowestPoint keeps), in the vessel's current pose. Unset: none (getLowestPoint = +inf)
    std::function<std::vector<float>(Part&, const Vector3f& point)> colliderClosestPointsZ;
    // WP-20e: the RB anchor (Vessel.AddRBAnchor's FixedJoint on the vessel's GameObject = the root part's Rigidbody, Vessel.cs:1162):
    // create (false: no Rigidbody to fix), destroy, joint.currentForce (world, the force the joint applies to the vessel), joint.breakForce.
    // Unset: no joint is ever made (AddRBAnchor only counts its time).
    std::function<bool(Vessel&, float breakForce, float breakTorque)> rbAnchorCreate;
    std::function<void(Vessel&)> rbAnchorDestroy;
    std::function<Vector3f(Vessel&)> rbAnchorCurrentForce;
    std::function<void(Vessel&, float breakForce)> rbAnchorSetBreakForce;
    // Part.Pack / Part.Unpack / Part.ResumeVelocity (physics): unset = nothing to do
    std::function<void(Part&)> packPart, unpackPart, resumePartVelocity;
    // VesselPrecalculate.GoOnRails / GoOffRails, Krakensbane.AddFrameVelocity: unset = nothing to do
    std::function<void(Vessel&)> precalcGoOnRails, precalcGoOffRails;
    // CommNetScenario.CommNetEnabled and connection.GetControlLevel() (WP-39): unset = CommNet is off
    std::function<std::optional<ControlLevel>(Vessel&)> commNetControlLevel;
    // ProtoVessel.Clean (recovery, funds, removal from the flight state): unset = nothing happens
    std::function<void(Vessel&, const std::string&)> cleanVessel;
    // PartSet rebuild after the part list changed (WP-15): UpdateResourceSets
    std::function<void(Vessel&)> updateResourceSets;
    // CelestialBody.GetFullTemperature(Vector3d pos) (needs GetAtmoThermalStats, not ported): unset = -1 (see SetStaticPressureAndExternalTemp)
    std::function<double(CelestialBody&, const Vector3d&)> fullTemperatureAtPosition;
    // PhysicsGlobals / Krakensbane frame velocity added to rb velocity (Krakensbane.GetFrameVelocity): unset = zero
    std::function<Vector3d()> krakensbaneFrameVelocity;
    // Krakensbane.AddFrameVelocity (GoOffRails of the active vessel): unset = nothing to do
    std::function<void(const Vector3d&)> krakensbaneAddFrameVelocity;
    // WP-19a physics seams (VesselPhysics, WP-20b; PHYSICS-SPIKE.md 5.3). Unset = the C# answer for "no rigid body" (rb == null).
    // VesselPhysics::ReadState(): the double-frame state CalculatePhysicsStats reads; when it returns a state with totalMass > 0 it replaces
    // the per-part loop over partRigidbody (kept as the float oracle and for hosts without a VesselPhysics)
    std::function<std::optional<VesselPhysicsState>(Vessel&)> readPhysicsState;
    // Part.rb as the engine reports it for CalculatePhysicsStats (VesselPrecalculate.cs:387): default PartRigidbodyState::FromPart.
    std::function<PartRigidbodyState(Part&)> partRigidbody;
    // rootPart.rb.GetPointVelocity(point) and rootPart.rb.angularVelocity (world) for the mass-less / packed branch (VesselPrecalculate.cs:491-498);
    // returns false when rootPart == null or rootPart.rb == null
    std::function<bool(Vessel&, const Vector3f& point, Vector3f& pointVelocity, Vector3f& angularVelocity)> rootRigidbody;
    // Vessel.OffsetVelocity (Vessel.cs:3742): add `dv` to the velocity of every part rigid body (and servo rigid body).
    // Vessel.ChangeWorldVelocity (Vessel.cs:3197, WP-19b) uses the same hook for its rigid-body loop.
    std::function<void(Vessel&, const Vector3d& dv)> offsetVelocity;
    // Vessel.SetWorldVelocity (Vessel.cs:3219, WP-19b): set every part rigid body's velocity to `v` (unset: nothing)
    std::function<void(Vessel&, const Vector3d& v)> setWorldVelocity;
    // Part.explode(offset) (Vessel.CheckKill, Vessel.cs:2776): unset = nothing
    std::function<void(Part&, float offset)> explodePart;
    // HighLogic.CurrentGame.CurrenciesAvailable (Vessel.Update, Vessel.cs:3002): unset = true
    std::function<bool()> currenciesAvailable;
    // ShipConstruction.CalculateCraftSize (Vessel.UpdateVesselSize): unset = vesselSize stays
    std::function<Vector3f(const std::vector<Part*>&, Part*)> calculateCraftSize;
};

// FlightGlobals is the one flight-scene registry (ksp/celestial/flight_globals.hpp, WP-19a). `FlightRegistry` was WP-14's subset of it and
// is kept as an alias so that the code written against it keeps compiling. FlightRegistry::Clear() resets the registry (tests).
using FlightRegistry = FlightGlobals;

// FlightGlobals.getAltitudeAtPos / getUpAxis / getStaticPressure(pos, body) (FlightGlobals.cs:1406-1495), the statics Vessel.cs uses
namespace FlightMath {
double getAltitudeAtPos(const Vector3d& position, const CelestialBody& body);  // FlightGlobals.cs:1471
Vector3d getUpAxis(const CelestialBody& body, const Vector3d& position);        // FlightGlobals.cs:1491
double getStaticPressure(const Vector3d& position, const CelestialBody& body);  // FlightGlobals.cs:1435
}  // namespace FlightMath

class Vessel final : public ITargetable, public IDiscoverable {
public:
    using Situations = VesselSituations;  // Vessel.Situations (Vessel.cs:25)
    using State = VesselState;            // Vessel.State (Vessel.cs:12)

    // ---- statics ------------------------------------------------------------------------------------------------------
    static inline VesselEnvironment env;                      // see VesselEnvironment
    static inline int NumOverrideGroups = 4;                  // Vessel.cs:468 (mirrors ProtoVessel::NumOverrideGroups)
    static inline bool PartialControlHasSASRCS = true;        // Vessel.cs:326
    static inline std::vector<std::string> VesselAnchorExceptions;  // Vessel.cs:148 (filled by Awake)
    static inline double HeightFromPartOffsetGlobal = 0.5;    // Vessel.cs:394
    static inline double warningThresholdG = 0.8;             // Vessel.cs:428
    static inline double warningThresholdPres = 0.8;          // Vessel.cs:430

    // ---- fields (Vessel.cs:53-475, same names; Unity-only fields omitted) -------------------------------------------------
    Guid id;
    uint32_t persistentId = 0;
    std::string vesselName;
    std::vector<Part*> parts;
    Part* rootPart = nullptr;
    uint32_t referenceTransformId = 0;
    uint32_t referenceTransformIdRecall = 0;
    VesselPrecalculate precalc{this};  // Vessel.precalc (VesselPrecalculate component, WP-19a)
    std::unique_ptr<OrbitDriver> orbitDriver;
    PatchedConicSolver* patchedConicSolver = nullptr;  // attached by AttachPatchedConicsSolver (WP-08 owns the class; set by the host)
    FlightCtrlState ctrlState;
    std::vector<FlightCtrlState*> setControlStates;  // [0] is &ctrlState (same object in C#)
    int PQSminLevel = 0;
    int PQSmaxLevel = 0;
    int currentStage = 0;
    Quaternionf srfRelRotation = Quaternionf::identity();
    double longitude = 0.0;
    double latitude = 0.0;
    double altitude = 0.0;
    double radarAltitude = 0.0;
    ProtoVessel* protoVessel = nullptr;
    bool loaded = false;
    bool easingInToSurface = false;
    double easeInMultiplier = 1.0;  // Vessel.cs:236 (FlightGlobals.ToggleVesselEaseIn)
    double distanceTraveled = 0.0;
    Vector3f vesselSize;
    AltimeterDisplayState altimeterDisplayState = AltimeterDisplayState::DEFAULT;
    State state = State::INACTIVE;
    bool packed = false;
    bool Landed = false;
    bool Splashed = false;
    bool permanentGroundContact = false;
    std::vector<Part*> GroundContacts;
    bool skipGroundPositioning = false;
    bool skipGroundPositioningForDroppedPart = false;
    bool vesselSpawning = true;
    std::string launchedFrom = "";
    std::string landedAt = "";
    std::string displaylandedAt = "";
    std::string landedAtLast = "";
    Delegate<void()> OnJustAboutToBeDestroyed = Delegate<void()>::Static(&Vessel::NoCallback);
    // Vessel.cs:370 OnAutopilotUpdate (FlightInputCallback(FlightCtrlState), WP-24: VesselSAS connects ControlUpdate / UpdateVesselTorque). The host
    // invokes it from FeedInputFeed (Vessel.cs:2035) between OnPreAutopilotUpdate and OnPostAutopilotUpdate.
    Delegate<void(FlightCtrlState&)> OnAutopilotUpdate = Delegate<void(FlightCtrlState&)>::Static(&Vessel::NoFlightCallback);
    Situations situation = static_cast<Situations>(0);
    double missionTime = 0.0;
    double launchTime = 0.0;
    Vector3d obt_velocity, srf_velocity, srf_vel_direction;
    Vector3f rb_velocity;
    Vector3d rb_velocityD, velocityD;
    double obt_speed = 0.0;
    Vector3d acceleration, acceleration_immediate;
    Vector3d perturbation, perturbation_immediate;
    double specificAcceleration = 0.0;
    Vector3d upAxis;
    Vector3f CoM;
    Vector3f MOI;
    Vector3d CoMD;
    Vector3f angularVelocity;
    Vector3d angularVelocityD;
    Vector3f angularMomentum;
    double geeForce = 0.0;
    double geeForce_immediate = 0.0;
    double gravityMultiplier = 1.0;
    Vector3d graviticAcceleration, gravityForPos, gravityTrue;
    bool frameWasRotating = false;
    double verticalSpeed = 0.0;
    double horizontalSrfSpeed = 0.0;
    double srfSpeed = 0.0;
    double indicatedAirSpeed = 0.0;
    double mach = 0.0;
    double speed = 0.0;
    double externalTemperature = 0.0;
    double atmosphericTemperature = 0.0;
    double staticPressurekPa = 0.0;
    double dynamicPressurekPa = 0.0;
    double atmDensity = 0.0;
    double speedOfSound = 0.0;
    double distanceToSun = 0.0;
    Vector3d up, north, east;
    double convectiveMachFlux = 0.0;
    double convectiveCoefficient = 0.0;
    double solarFlux = 0.0;
    bool directSunlight = false;
    double totalMass = 0.0;
    double lastUT = -1.0;
    double waterOffset = 0.0;
    Vector3d lastVel, nextVel;
    CelestialBody* lastBody = nullptr;
    VesselType vesselType = VesselType::Debris;
    IEvaController* evaController = nullptr;  // KerbalEVA (WP-32): only LadderPart is read
    int crewedParts = 0;
    int crewableParts = 0;
    ControlLevel maxControlLevel = ControlLevel::FULL;
    bool EditableNodes = false;
    ConfigNode flightPlanNode{"FLIGHTPLAN"};
    ITargetable* targetObject = nullptr;
    std::vector<std::shared_ptr<VesselModule>> vesselModules;
    CommNet::CommNetVessel* connection = nullptr;  // Vessel.cs:346 (WP-39: set by CommNetVessel.OnAwake / OnStart)
    ProtoTargetInfo pTI;
    VesselRanges vesselRanges;
    double heightFromPartOffsetLocal = 0.0;
    float heightFromTerrain = -1.0f;
    float heightFromSurface = -1.0f;
    double terrainAltitude = 0.0;
    double pqsAltitude = 0.0;
    Vector3f terrainNormal;
    PartTransform vesselTransform;  // Vessel.vesselTransform (the GameObject's transform; position and rotation are Unity floats)
    // WP-20e: Vessel.cs:132 crashObjectName (internal, the CrashObjectName component's objectName; "" = null) and :408 objectUnderVessel
    // (the GameObject under the vessel: its tag, "" = null; set by the GetHeightFromTerrain probe)
    std::string crashObjectName;
    std::string objectUnderVesselTag;
    bool initialPosVelSet = false;
    float gThresh = 0.0f;
    float presThresh = 0.0f;
    double partMaxGThresh = 0.0;
    double partMaxPresThresh = 0.0;
    std::vector<PartModule*> dockingPorts;
    Vector3d krakensbaneAcc;
    Vector3f localCoM;
    bool updateResourcesOnEvent = true;
    bool resourcesDirty = false;
    int GroupOverride = 0;
    std::vector<bool> OverrideDefault;
    std::vector<KSPActionGroup> OverrideActionControl;
    std::vector<KSPAxisGroup> OverrideAxisControl;
    std::vector<std::optional<std::string>> OverrideGroupNames;
    bool isUnloading = false;
    bool isBackingUp = false;
    std::string gameObjectName;  // base.name
    ProtoWaypointInfo pWPI;
    // OnSaveFlightState / OnLoadFlightState (camera heading / pitch / mode of the active vessel)
    float camhdg = 0.0f, campth = 0.0f, camMode = -1.0f;

    Vessel();  // AddComponent<Vessel> + Awake (Vessel.cs:1520)
    ~Vessel();
    Vessel(const Vessel&) = delete;
    Vessel& operator=(const Vessel&) = delete;

    // ---- indexers and properties (Vessel.cs:538-905) ------------------------------------------------------------------------
    Part* byFlightID(uint32_t flightID);      // this[uint flightID]  (Vessel.cs:538; searches from the end)
    Part* byIndex(int index);                 // this[int index]      (Vessel.cs:558)
    std::vector<Part*>& Parts() { return parts; }                     // Vessel.cs:570
    PartTransform& ReferenceTransform();      // Vessel.cs:572 (the reference part's transform, else vesselTransform)
    Orbit& orbit() const { return *orbitDriver->orbit; }              // Vessel.cs:584 orbitDriver.orbit
    CelestialBody* mainBody() const { return orbitDriver->orbit->referenceBody; }  // Vessel.cs:586
    float GroundLevelAngle() const { return groundLevelAngle_; }      // Vessel.cs:588
    bool LandedOrSplashed() const { return Landed || Splashed; }      // Vessel.cs:590
    bool LandedInKSC() const;                 // Vessel.cs:604
    bool HoldPhysics() const { return physicsHoldLock_; }  // Vessel.cs:640
    bool isCommandable() const { return static_cast<int>(vesselType) > static_cast<int>(VesselType::Debris); }  // Vessel.cs:642
    bool isPersistent();                      // Vessel.cs:644
    void set_isPersistent(bool v) { persistent_ = v; }
    bool isActiveVessel() const { return FlightRegistry::ActiveVessel == this; }  // Vessel.cs:677
    bool IsRecoverable();                     // Vessel.cs:679
    Situations BestSituation() const;         // Vessel.cs:691
    bool IgnoreSpeedActive() const { return ignoreSpeedFrames_ > 0; }  // Vessel.cs:731
    int IgnoreCollisionsFrames() const { return ignoreCollisionsFrames; }
    // the C# field is `internal` (Vessel.cs:248; written by the landed placement :2543-2557 and CometManager.cs:658)
    void SetIgnoreCollisionsFrames(int frames) { ignoreCollisionsFrames = frames; }
    const Vector3f& CurrentCoM() const { return CoM; }                 // Vessel.cs:735
    bool isEVA() const { return vesselType == VesselType::EVA; }       // Vessel.cs:737
    Vessel* EVALadderVessel();                // Vessel.cs:739
    ControlLevel CurrentControlLevel() const { return controlLevel_; }  // Vessel.cs:753
    bool IsControllable() const { return isControllable_; }            // Vessel.cs:755
    VesselAutopilot& Autopilot() { return *autopilot_; }               // Vessel.cs:751
    VesselValues& GetVesselValues() { return *vesselValues_; }         // Vessel.cs:757 VesselValues
    bool PatchedConicsAttached() const { return patchedConicsAttached_; }  // Vessel.cs:795
    void set_PatchedConicsAttached(bool v) { patchedConicsAttached_ = v; }
    ActionGroupList& ActionGroups() { return *actionGroups_; }         // Vessel.cs:801
    DiscoveryInfo& GetDiscoveryInfo() override { return *discoveryInfo_; }      // Vessel.cs:903 DiscoveryInfo
    bool AutoClean() const { return autoClean_; }                      // Vessel.cs:905
    const std::string& AutoCleanReason() const { return autoCleanReason_; }  // Vessel.cs:907
    // Obsolete range accessors (Vessel.cs:804-868)
    float distancePackThreshold() const { return vesselRanges.orbit.pack; }
    void set_distancePackThreshold(float v);
    float distanceUnpackThreshold() const { return vesselRanges.orbit.unpack; }
    void set_distanceUnpackThreshold(float v);
    float distanceLandedPackThreshold() const { return vesselRanges.landed.pack; }
    void set_distanceLandedPackThreshold(float v);
    float distanceLandedUnpackThreshold() const { return vesselRanges.landed.unpack; }
    void set_distanceLandedUnpackThreshold(float v);

    // ---- parts, reference transform (Vessel.cs:909-980) ---------------------------------------------------------------------
    Part* PartsContain(uint32_t partPersistentId);  // :909
    Part* GetReferenceTransformPart() { return referenceTransformPart_; }  // :926
    void SetReferenceTransform(Part* p, bool storeRecall = true);  // :931
    void RecallReferenceTransform();                               // :952
    void FallBackReferenceTransform();                             // :975
    Orbit* GetCurrentOrbit();                                      // :983
    Vector3d GetWorldPos3D();                                      // :996
    void SetLoaded(bool loaded);                                   // :1005

    // ---- situations (Vessel.cs:1015-1062, 1185-1232, 3133-3195) ------------------------------------------------------------
    bool checkLanded();                                            // :1015
    bool checkSplashed();                                          // :1044
    void SetLandedAt(const std::string& landedAt, const std::string& gameObjectTag = "", const std::string& inputdisplaylandedAt = "");  // :1185
    void updateSituation();                                        // :3133
    void UpdateLandedSplashed();                                   // :3190
    static std::string GetSituationString(Situations situation);   // :3466
    static std::string GetSituationString(Vessel& v);              // :3491 (localisation keys resolved through Localizer)
    static std::string GetLandedAtString(const std::string& landedAt);  // :3532 MiniBiome.ConvertTagtoLandedAt
    static Vessel* GetDominantVessel(Vessel* v1, Vessel* v2);      // :2235

    // ---- WP-20e: the landed RB anchor (Vessel.cs:1105-1183; src/vessel/vessel_ground.cpp) ----------------------------------------
    void AddRBAnchor();                                            // :1105 (private in C#; LateUpdate calls it)
    void ResetRBAnchor();                                          // :1169
    void OnJointBreak(float breakForce);                           // :1180 (the anchor joint broke: PhysX's OnJointBreak message)
    bool HasRBAnchor() const { return rbAnchor_; }                 // joint != null
    float AnchorTimeCounter() const { return anchorTimeCounter_; }
    float AnchorForceAngle() const { return anchorForceAngle_; }
    float AnchorDeltaAngleTimeCounter() const { return anchorDeltaAngleTimeCounter_; }
    float PrevAnchorForceAngle() const { return prevAnchorForceAngle_; }
    float AnchorDeltaAngle() const { return deltaAngle_; }
    // ---- WP-20e: CheckGroundCollision (Vessel.cs:2493) and getLowestPoint (:2578) -------------------------------------------------
    bool CheckGroundCollision();
    float getLowestPoint();
    const VesselRayHit& GroundCollisionHit() const { return groundCollisionHit_; }

    // ---- crew (Vessel.cs:1234-1376) ------------------------------------------------------------------------------------------
    using PCM = std::shared_ptr<ProtoCrewMember>;
    const std::vector<PCM>& GetVesselCrew();                       // :1234
    void CrewListSetDirty() { crewCachedPartCount_ = -1; }         // :1247
    void RebuildCrewList();                                        // :1252
    bool RemoveCrew(const PCM& pcm);                               // :1293
    bool RemoveCrew(const std::string& name);                      // :1298
    void RemoveCrewList(const std::vector<PCM>& pcmList, bool updatePartCount);  // :1313
    int GetCrewCapacity();                                         // :1341
    int GetCrewCount() const { return static_cast<int>(crew_.size()); }  // :1353
    static void CrewWasModified(Vessel* vessel);                   // :1358
    static void CrewWasModified(Vessel* vessel1, Vessel* vessel2);  // :1367
    void onCrewTransferred(events::GameEvents::HostedFromToAction<ProtoCrewMember*, Part*> data);  // :1373
    void MurderCrew();                                             // :3376

    // ---- control level (Vessel.cs:1378-1485) ---------------------------------------------------------------------------------
    ControlLevel GetControlLevel();                                // :1378
    void CheckControllable();                                      // :1405 (the InputLockManager part is UI: control-lock bookkeeping flags only)
    bool controlLockSet() const { return controlLockSet_; }
    bool controlLockPartial() const { return controlLockPartial_; }
    void SetGroupOverride(int newGroup);                           // :1994
    void SetControlState(const FlightCtrlState& st);               // :2017
    void GetControlState(FlightCtrlState& st);                     // :2030
    bool ActionControlBlocked(KSPActionGroup actionGroup) const;   // :4045
    bool AxisControlBlocked(KSPAxisGroup axisGroup) const;         // :4054
    void CopyOverrides(const Vessel& v);                           // :4063

    // ---- vessel modules (Vessel.cs:1488-1518, 3870) ---------------------------------------------------------------------------
    void LoadVesselModules(const ConfigNode* node);                // :1488
    void UpdateVesselModuleActivation();                           // :1507
    VesselModule* FindVesselModuleByName(const std::string& className);  // FindVesselModuleImplementing<T>() for a named type (:3870)
    PartModule* FindPartModuleImplementingClass(const std::string& className);  // FindPartModuleImplementing<ModuleX>() (:3851)
    std::vector<PartModule*> FindPartModulesImplementing(const std::function<bool(PartModule&)>& is);  // :3831

    // ---- lifecycle and transitions (Vessel.cs:1583-2000, 2356-2490) ------------------------------------------------------------
    void SetStaticPressureAndExternalTemp();                       // :1583
    void Start();                                                  // :1597
    bool PhysicsHoldExpired();                                     // :1634
    void SetPhysicsHoldExpiryOverride(int newHoldCountValue = 5);  // :1645
    // :1653 / :1658. The root part and its tree must be linked; takes ownership of `ownedParts` (the whole tree, root first).
    bool Initialize(bool fromShipAssembly = false);                // :1653
    bool Initialize(bool fromShipAssembly, bool preCreate, bool orbiting, bool setActiveVessel);  // :1658
    void AdoptParts(std::vector<std::unique_ptr<Part>> ownedParts, Part* root);
    void StartFromBackup(ProtoVessel* pv);                         // :1793
    void StartFromBackup(ProtoVessel* pv, FlightState* st);        // :1798
    void Load();                                                   // :1813
    void Unload();                                                 // :1868
    ProtoVessel* BackupVessel();                                   // :1969
    void MakeActive();                                             // :2047
    void MakeInactive();                                           // :2081
    void GoOnRails();                                              // :2356
    void GoOffRails();                                             // :2395
    void Die();                                                    // :3240 (state change, events and crew; scene teardown is the host's)
    bool IsFirstFrame() const { return false; }                    // :4364 (needs the FlightIntegrator component: WP-21)

    // ---- position, altitude, kinematics (Vessel.cs:2652-2730, 2786-2975, 3678-3760) ------------------------------------------------
    float GetHeightFromTerrain(bool overRidePacked = false);       // :2652 (ray probe through env.heightFromTerrainProbe)
    double PQSAltitude();                                          // :2721
    double getCorrectedLandedAltitude(double lat, double lon, double alt, CelestialBody& body);  // :2714
    void UpdatePosVel();                                           // :2786
    void UpdateDistanceTraveled();                                 // :2873
    void UpdateAcceleration(double fdtRecip, bool fromUpdate);     // :2892
    void IgnoreGForces(int frames);                                // :2975
    void IgnoreSpeed(int frames);                                  // :2983
    void SetPosition(const Vector3d& position);                    // :3678
    void SetPosition(const Vector3d& position, bool usePristineCoords);  // :3683
    void Translate(const Vector3d& offset);                        // :3713
    void SetRotation(const Quaternionf& rotation);                 // :3718
    void SetRotation(const Quaternionf& rotation, bool setPos);    // :3723
    // srfSpeed / horizontalSrfSpeed / srf_vel_direction from srf_velocity and verticalSpeed (the block of UpdatePosVel at :2845-2865)
    void UpdateSurfaceSpeeds();
    void UpdateCaches();                                           // :3104

    // ---- mass properties ------------------------------------------------------------------------------------------------------------
    float GetTotalMass();                                          // :3422
    // FlightIntegrator.UpdateMassStats, the mass part (FlightIntegrator.cs:452-491, 700-713): Part.resourceMass, physicsMass with the
    // massless children's mass folded into the part that carries them. The thermal-mass half is WP-22's, rb.mass is the engine's.
    void UpdateMassStats();
    double GetUnloadedVesselMass();                                // :3441
    // VesselPrecalculate.CalculatePhysicsStats (VesselPrecalculate.cs:387-498) over per-part rigid-body data: total physics mass,
    // CoMD / CoM / localCoM, rb velocity, angular velocity, MOI and angular momentum. `rb(part)` is the engine's answer (default:
    // PartRigidbodyState::FromPart with zero velocities and no inertia tensor).
    void CalculatePhysicsStats(const std::function<PartRigidbodyState(Part&)>& rb = nullptr);
    // WP-20b: the MOI loop of CalculatePhysicsStats (VesselPrecalculate.cs:436-453) over given part rigid-body states, for
    // VesselPhysics::ReadState in the double frame: `states[i]` / `relPosition[i]` = (Vector3d)rb.position - CoMD of the i-th entry
    // (entries with !present are skipped), `inverseReference` = QuaternionD.Inverse(ReferenceTransform.rotation). The same float
    // statements as the loop inside CalculatePhysicsStats (which is left untouched); returns KSPUtil.Diag of the sum.
    static Vector3f MOIFromRigidbodies(const std::vector<PartRigidbodyState>& states, const std::vector<Vector3d>& relPosition,
                                       const QuaternionD& inverseReference, const PartTransform& referenceTransform);

    // ---- atmosphere (mirror of FlightIntegrator.cs:621-623, 863-864, 886-914) ----------------------------------------------------
    void ApplyFlightIntegratorAtmosphere(const FlightIntegratorAtmosphere& fi);

    // ---- flight state, naming, misc ---------------------------------------------------------------------------------------------------
    void OnSaveFlightState(KSPParseableDictionary& dataPool);      // :3615
    void OnLoadFlightState(KSPParseableDictionary& dataPool);      // :3628
    ClearToSaveStatus IsClearToSave();                             // :3644
    bool checkVisibility();                                        // :3657
    static bool IsValidVesselName(const std::string& name);        // :4137
    bool UpdateVesselNaming(bool noGameEvent = false);             // :4162
    void RunVesselNamingUpdates(Part* pNewName, bool noGameEvent);  // :4174
    VesselType FindDefaultVesselType();                            // :4202
    void SetAutoClean(const std::string& reason = "");             // :4412
    void Clean(const std::string& reason = "");                    // :4421
    void UpdateVesselSize();                                       // :3908
    void UpdateResourceSets();                                     // :3913 (PartSet rebuild: WP-15 via env.updateResourceSets)
    void UpdateResourceSetsIfDirty();                              // :3935
    std::vector<Part*> GetActiveParts();                           // :3816
    static std::string AutoRename(Vessel& v, const std::string& baseName);  // :3568 (names resolved through Localizer)
    static std::string GetMETString(const Vessel& v);              // :3537
    static double GetNextManeuverTime(Vessel* v, bool& hasManeuver);  // :3542
    void CheckAirstreamShields();                                  // :3604

    // ---- ITargetable (Vessel.cs:3761-3815) / IDiscoverable (Vessel.cs:4373-4410) -------------------------------------------------------
    // Vessel.cs:3761 `return ReferenceTransform`: the Transform stand-in owned by the Vessel, refreshed from ReferenceTransform()
    // (name, position, rotation) on every call; the pointer is stable for the Vessel's lifetime
    UnityEngine::Transform* GetTransform() override;
    Vector3f GetObtVelocity() override { return obt_velocity.ToVector3f(); }
    Vector3f GetSrfVelocity() override { return srf_velocity.ToVector3f(); }
    Vector3f GetFwdVector() override;
    Vessel* GetVessel() override { return this; }
    std::string GetName() override { return vesselName; }
    std::string GetDisplayName() override;
    Orbit* GetOrbit() override { return orbitDriver ? orbitDriver->orbit.get() : nullptr; }
    OrbitDriver* GetOrbitDriver() override { return orbitDriver.get(); }
    VesselTargetModes GetTargetingMode() override { return VesselTargetModes::DirectionAndVelocity; }
    bool GetActiveTargetable() override { return false; }
    std::string RevealName() override { return vesselName; }
    std::string RevealDisplayName() override;
    double RevealSpeed() override;
    double RevealAltitude() override { return altitude; }
    std::string RevealSituationString() override;
    std::string RevealType() override;
    float RevealMass() override { return GetTotalMass(); }

    // IOrbitDriverVessel for OrbitDriver (OrbitDriver.cs): the interface names CoMD() / velocityD() clash with the fields
    IOrbitDriverVessel& OrbitDriverView() { return *orbitDriverView_; }
    // IFlightVessel for TimeWarp's FlightGlobals.ActiveVessel / VesselsLoaded (WP-10): the interface names altitude() / geeForce() /
    // srf_velocity() clash with the fields and orbit() returns Orbit* there, so it is an adapter like OrbitDriverView. Since WP-19a the registry
    // (FlightGlobals) refreshes FlightGlobals::ActiveVesselView / VesselsLoadedView from it on every registry change.
    IFlightVessel& FlightView() { return *flightView_; }

    // ---- the MonoBehaviour tick (WP-19a; Vessel.cs:2730-2790, 2990-3103, 3742) --------------------------------------------------------
    void FixedUpdate();                                            // :2730 UpdateCaches + the ignore-frame counters
    void CheckKill();                                              // :2746
    void Update();                                                 // :2990 mission time, load / unload by range, autopilot
    void LateUpdate();                                             // :3055 (situation, g / pressure warning thresholds; AddRBAnchor is the physics seam)
    void OffsetVelocity(const Vector3d& correction);               // :3742 (through env.offsetVelocity)
    // WP-19b: :3197 (not DEAD / packed: every rigid body += velOffset through env.offsetVelocity, krakensbaneAcc += velOffset / dt)
    void ChangeWorldVelocity(const Vector3d& velOffset);
    void SetWorldVelocity(const Vector3d& vel);                    // :3219 (not DEAD / packed: through env.setWorldVelocity)
    // Krakensbane.GetFrameVelocity() (env.krakensbaneFrameVelocity when set)
    Vector3d FrameVelocity() const;

    // ---- staging and vessel splitting (WP-16; Vessel.cs:2130-2153, and the Vessel components Part.decouple / Couple / Undock create) ----
    void ClearStaging();                                           // :2130 (every part's stack icon is removed, onVesselClearStaging)
    void ResumeStaging();                                          // :2140
    // `part.gameObject.AddComponent<Vessel>()` for Part.decouple / Undock: a Vessel kept alive by SpawnedVessels()
    static Vessel* SpawnVessel();
    // UnityEngine.Object.DestroyImmediate(vessel) (Part.Couple): the vessel leaves the registry and loses its parts; a spawned
    // vessel is freed, a vessel owned by the caller (std::unique_ptr) stays valid but empty
    static void DestroyImmediate(Vessel* v);
    static std::vector<std::unique_ptr<Vessel>>& SpawnedVessels();
    // Part ownership follows the tree when it moves between vessels: the unique_ptrs of `which` that this vessel owns
    std::vector<std::unique_ptr<Part>> ReleaseOwnedParts(const std::vector<Part*>& which);
    void AcceptOwnedParts(std::vector<std::unique_ptr<Part>> moved);
    // `StartCoroutine(CallbackUtil.DelayedCallback(1, ...))` (Part.Undock: oldVessel.ResumeStaging one frame later): the host
    // calls RunDelayedFrameCalls() once per frame
    std::vector<std::function<void()>> delayedFrameCalls;
    void RunDelayedFrameCalls();

    // ---- bookkeeping the host needs ---------------------------------------------------------------------------------------------------
    // Part.vessel hooks (isEVA, partCount, removeCrew, crewListSetDirty, snapshotSavesModules) pointing at the Vessel
    static void InstallPartHooks();
    // The parts the Vessel owns (after AdoptParts / Load); cleared by Unload
    size_t OwnedPartCount() const { return ownedParts_.size(); }
    ProtoVessel* OwnedProtoVessel() const { return protoVesselOwner_.get(); }

private:
    friend class ProtoVessel;
    friend class VesselOrbitDriverView;
    static void NoCallback() {}
    static void NoFlightCallback(FlightCtrlState&) {}
    void FindVesselParts(Part* part, bool checkAttached);  // findVesselParts (:1776)
    void SyncVesselTransform();  // the Vessel component lives on the root part's GameObject: vesselTransform follows rootPart
    Part* FindReferenceTransform(uint32_t refId);          // :963
    void OnVesselNamingVesselWasModified(Vessel* v);       // :4154
    void OnVesselPartCountChanged(Vessel* v);              // :3892 (WP-16)
    void UpdateCrossfeedResourceSetFL(events::GameEvents::HostedFromToAction<bool, Part*> data);  // :3943
    void UpdateResourceSetsEventCheckPart(Part* p);        // :3951
    void OnPartVesselNamingChanged(Part* p);               // :4146

    Part* referenceTransformPart_ = nullptr;
    Part* referenceTransformPartRecall_ = nullptr;
    float groundLevelAngle_ = -1.0f;
    bool persistent_ = false;
    Situations lastSituation_ = static_cast<Situations>(0);
    bool wasLadder_ = false;
    std::vector<PCM> crew_;
    int crewCachedPartCount_ = -1;
    bool isControllable_ = false;
    ControlLevel controlLevel_ = ControlLevel::NONE;
    bool controlLockSet_ = false;
    bool controlLockPartial_ = false;
    std::unique_ptr<VesselValues> vesselValues_;
    std::unique_ptr<VesselAutopilot> autopilot_;  // Vessel.cs:310
    std::unique_ptr<ActionGroupList> actionGroups_;
    std::unique_ptr<DiscoveryInfo> discoveryInfo_;
    bool patchedConicsAttached_ = false;
    bool autoClean_ = false;
    std::string autoCleanReason_;
    bool physicsHoldLock_ = false;
    bool useFramesAtStartupOverride_ = false;
    int hold_count_override_ = 5;
    int framesAtStartupOverride_ = 0;
    int framesAtStartup_ = 0;
    Vector3f rootOrgPos_;
    Quaternionf rootOrgRot_ = Quaternionf::identity();
    int ignoreGeeforceFrames_ = 0;
    int ignoreSpeedFrames_ = 0;
    int ignoreCollisionsFrames = 0;
    // WP-20e: Vessel.cs:136-146 (joint != null, the anchor timers and angles) and :392 groundCollisionHit
    bool rbAnchor_ = false;
    float anchorTimeCounter_ = 0.0f;
    float anchorForceAngle_ = 0.0f;
    float prevAnchorForceAngle_ = -1.0f;
    float deltaAngle_ = 0.0f;
    float anchorDeltaAngleTimeCounter_ = 0.0f;
    VesselRayHit groundCollisionHit_;
    int oldPartCount_ = -1;
    bool bouncedOffCheck_ = false;
    Part* vesselNamedBy_ = nullptr;
    std::vector<std::unique_ptr<Part>> ownedParts_;
    std::shared_ptr<ProtoVessel> protoVesselOwner_;
    std::vector<Vector3d> accelSmoothing_;
    int accelSmoothingCursor_ = 0;
    static inline int accelSmoothingLength_ = 20;
    double accelSmoothingLengthRecip_ = 0.0;
    std::unique_ptr<IOrbitDriverVessel> orbitDriverView_;
    std::unique_ptr<IFlightVessel> flightView_;
    UnityEngine::Transform targetTransform_;  // what GetTransform() hands out (the ReferenceTransform mirror)
    std::vector<std::unique_ptr<FlightCtrlState>> ownedControlStates_;
};

}  // namespace ksp
