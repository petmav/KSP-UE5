// FlightGlobals (FlightGlobals.cs, 2003 lines): the flight-scene registry and queries. WP-19a replaces the WP-08/WP-10 subset and
// the WP-14 `FlightRegistry` by this one class (`FlightRegistry` is an alias of it, see ksp/vessel/vessel.hpp).
//
// kspcore reading of the MonoBehaviour: every static of the C# is a `static inline` here; the instance fields reached through
// `FlightGlobals.fetch` (vessels, vesselsLoaded, vesselsUnloaded, activeVessel, bodies, bodyNames, homeBody ...) are statics too, because
// there is one FlightGlobals per flight scene and none outside it. `fetch == null` (no flight scene) is `FlightGlobals::bodies == nullptr`
// for the queries that guard on it (GetBodyByName); the registry mutators work without a scene object (tests, tools).
//
// The Vessel-typed members are defined in src/vessel/flight_globals_vessels.cpp (this header only forward declares Vessel / Part), the
// body-only ones in src/celestial/flight_globals.cpp.
//
// TimeWarp's view: WP-10's TimeWarp reads the active and loaded vessels through `IFlightVessel` (flight_globals.hpp:IFlightVessel).
// Those are the fields `ActiveVesselView` / `VesselsLoadedView` (the WP-10 names `ActiveVessel` / `VesselsLoaded` now belong to the
// registry proper, which holds `Vessel*`). Every registry mutation (AddVessel, RemoveVessel, SortVessel, SetActiveVessel, Clear)
// refreshes both views from `Vessel::FlightView()`, so a host that uses real Vessels never sets them; tests that drive TimeWarp with
// stand-in IFlightVessels set the views directly and leave the registry empty.
#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "ksp/math/vector3d.hpp"
#include "ksp/orbit/targeting_modes.hpp"
#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

class CelestialBody;
class Orbit;
class Vessel;
class Part;
class ProtoPartSnapshot;
class ITargetable;
class physicalObject;
struct Guid;
enum class ClearToSaveStatus : int;
namespace UnityEngine {
class Transform;
}

// FlightGlobals.SpeedDisplayModes (FlightGlobals.cs:12)
enum class FlightGlobals_SpeedDisplayModes : int { Orbit = 0, Surface = 1, Target = 2 };

// The members of Vessel that TimeWarp.cs reads through FlightGlobals.ActiveVessel / FlightGlobals.VesselsLoaded
// (WP-10). The Vessel package (WP-14/WP-19) implements it (Vessel::FlightView()); KSP's `activeVessel != null` is `!= nullptr`.
class IFlightVessel {
public:
    virtual ~IFlightVessel() = default;
    virtual bool LandedOrSplashed() const = 0;           // Vessel.LandedOrSplashed
    virtual CelestialBody* mainBody() const = 0;          // Vessel.mainBody
    virtual Vector3f TransformPosition() const = 0;       // vessel.transform.position (Unity float)
    virtual Orbit* orbit() const = 0;                     // Vessel.orbit
    virtual double altitude() const = 0;                  // Vessel.altitude
    virtual double geeForce() const = 0;                  // Vessel.geeForce
    virtual Vector3d srf_velocity() const = 0;            // Vessel.srf_velocity
    virtual bool PatchedConicsAttached() const = 0;       // Vessel.PatchedConicsAttached
    virtual bool isEVA() const = 0;                       // Vessel.isEVA
    virtual bool OnALadder() const = 0;                   // vessel.GetComponent<KerbalEVA>().OnALadder (EVA vessels)
};

struct FlightGlobals {
    using SpeedDisplayModes = FlightGlobals_SpeedDisplayModes;

    // ---- bodies and scene flags (FlightGlobals.cs:22-24, 68, 102-104) -------------------------------------------------------
    // FlightGlobals.fetch.bodies; registered by CelestialSystem::Spawn (cleared when that system is destroyed).
    // nullptr = no FlightGlobals instance (getMainBody then throws NullReferenceException, like KSP).
    static inline const std::vector<CelestialBody*>* bodies = nullptr;
    static inline bool ready = false;                       // :22
    static inline CelestialBody* currentMainBody = nullptr;  // :68
    static inline bool overrideOrbit = false;               // :102
    static inline bool warpDriveActive = false;             // :104

    // ---- the vessel registry (FlightGlobals.cs:32-38, :154, :172-176) --------------------------------------------------------
    static inline std::vector<Vessel*> Vessels;          // fetch.vessels
    static inline std::vector<Vessel*> VesselsLoaded;    // fetch.vesselsLoaded
    static inline std::vector<Vessel*> VesselsUnloaded;  // fetch.vesselsUnloaded
    static inline Vessel* ActiveVessel = nullptr;        // fetch.activeVessel
    // TimeWarp's IFlightVessel view of ActiveVessel / VesselsLoaded (WP-10; see the file comment)
    static inline IFlightVessel* ActiveVesselView = nullptr;
    static inline std::vector<IFlightVessel*> VesselsLoadedView;

    // ---- information refreshed by UpdateInformation (FlightGlobals.cs:54-66, 94-96) -----------------------------------------
    static inline double ship_temp = 0.0, ship_dns = 0.0;
    static inline Quaternionf ship_orientation = Quaternionf::identity(), ship_orientation_offset = Quaternionf::identity();
    static inline float ship_heading = 0.0f;
    static inline double camera_altitude = 0.0;
    static inline Vector3d camera_position;
    static inline Vector3d ship_tgtVelocity;
    static inline double ship_tgtSpeed = 0.0;
    static inline FoRModes FoRMode = FoRModes::SRF_NORTH;  // :108
    static inline float TargetSwitchSqrThresh = 3.6E+09f;  // :126
    static inline std::vector<physicalObject*> physicalObjects;  // :98 (ksp/physics/physical_object.hpp; WP-19b's bubble owns the bodies)

    // ---- target (FlightGlobals.cs:82-92, 236-238) -------------------------------------------------------------------------------
    static inline ITargetable* _vesselTarget = nullptr;
    static inline UnityEngine::Transform* vesselTargetTransform = nullptr;
    static inline VesselTargetModes vesselTargetMode = VesselTargetModes::None;
    static inline Vector3f vesselTargetDelta, vesselTargetDirection;

    // ---- host seams (everything the C# reaches through Unity / UI) ------------------------------------------------------------
    struct Environment {
        // fetch.mainCameraRef.transform.position (UpdateInformation); unset = the active vessel's position
        std::function<Vector3d()> cameraPosition;
        // FlightLogger.IgnoreGeeForces(seconds) (Start, OnLevelLoaded); unset = nothing
        std::function<void(float)> ignoreGeeForces;
        // ScreenMessages.PostScreenMessage(localised text): the vessel-switch refusals and the target message; unset = dropped
        std::function<void(const std::string& message)> postScreenMessage;
        // FlightDriver.StartAndFocusVessel(game, index) after saving "persistent" (switching to an UNLOADED vessel): unset = nothing
        std::function<void(int vesselIndex)> startAndFocusVessel;
        // OrbitPhysicsManager.CheckReferenceFrame() / HoldVesselUnpack(): WP-19b / OrbitPhysicsManager (ksp/physics/orbit_physics_manager.hpp)
        std::function<void()> checkReferenceFrame;
        std::function<void(int releaseAfter)> holdVesselUnpack;
        // FlightCamera.SetTarget(vessel): unset = nothing
        std::function<void(Vessel*)> flightCameraSetTarget;
        // HighLogic.CurrentGame.Updated() persistence of "persistent" before switching to an unloaded vessel: unset = nothing
        std::function<void()> savePersistent;
    };
    static inline Environment env;

    // ---- properties (FlightGlobals.cs:132-240) -----------------------------------------------------------------------------------
    static const std::vector<CelestialBody*>& Bodies();   // :132 (bodies, else the save environment's list)
    static float vacuumTemperature() { return 0.0f; }     // :146
    static bool RefFrameIsRotating();                     // :224
    static Vector3d upAxis();                             // :240
    static SpeedDisplayModes speedDisplayMode() { return speedMode; }  // :242
    static double ship_altitude();                        // :184
    static double ship_verticalSpeed();                   // :186
    static double ship_geeForce();                        // :188
    static Vector3d ship_velocity();                      // :190 (rb_velocity, float -> double)
    static Vector3d ship_obtVelocity();                   // :192
    static double ship_obtSpeed();                        // :194
    static Vector3d ship_srfVelocity();                   // :196
    static double ship_srfSpeed();                        // :198
    static Vector3d ship_acceleration();                  // :200
    static Vector3d ship_angularVelocity();               // :202
    static Vector3d ship_position();                      // :204
    static Vector3d ship_CoM();                           // :206
    static Vector3d ship_upAxis();                        // :208
    static Vector3f ship_MOI();                           // :210
    static Vector3f ship_angularMomentum();               // :212
    static double ship_latitude();                        // :218
    static double ship_longitude();                       // :220
    static double ship_stP();                             // :222
    static Orbit& ship_orbit();                           // :216
    static Part* activeTarget();                          // :152
    static ITargetable* VesselTarget() { return _vesselTarget; }  // :236

    // ---- the registry (FlightGlobals.cs:272-470) -------------------------------------------------------------------------------
    static void AddVessel(Vessel* vessel);                          // :272
    static void RemoveVessel(Vessel* vessel);                       // :305
    static void SortVessel(Vessel* vessel, bool loaded);            // :335
    static Vessel* FindVessel(const Guid& id);               // :366
    static bool FindVessel(uint32_t id, Vessel*& vessel);           // :387
    static int CountUntrackedSpaceObjects();                        // :403
    static int CountSpaceObjects();                                 // :408
    static int UntrackedObjectsCount();                             // :427
    static bool FindLoadedPart(uint32_t id, Part*& part);           // :441
    static bool FindUnloadedPart(uint32_t id, ProtoPartSnapshot*& part);  // :457
    static Part* FindPartByID(uint32_t flightID);                   // :1653
    static ProtoPartSnapshot* FindProtoPartByID(uint32_t flightID);  // :1681
    static Vessel* FindNearestControllableVessel(Vessel* currentVessel);  // :1705
    static std::vector<Vessel*> FindNearestVesselWhere(const Vector3d& refPos, const std::function<bool(Vessel&)>& criteria);  // :1726
    static void Clear();                                            // OnDestroy / tests: forget everything (:770)
    static void ClearAllVessels();                                  // :1066
    static void addPhysicalObject(physicalObject* pObject);         // :1080
    static void removePhysicalObject(physicalObject* pObject);      // :1085

    // ---- life cycle (FlightGlobals.cs:724-870) -----------------------------------------------------------------------------------
    static void Awake();                       // :724
    static void Start();                       // :733 (Invoke("disableOverride", 1f) is `disableOverride()` after RunDelayedCalls)
    static void OnLevelLoaded();               // :761
    static void OnSceneChange();               // :747
    static void FixedUpdate();                 // :789
    static void Update();                      // :802
    static void UpdateInformation(bool fixedUpdate);  // :811
    static void RunDelayedCalls(float time);   // Unity Invoke("disableOverride", ...) passes
    static void disableOverride();             // :1305

    // ---- active vessel (FlightGlobals.cs:940-1064) ---------------------------------------------------------------------------------
    static bool SetActiveVessel(Vessel* v);                              // :949
    static bool SetActiveVessel(Vessel* v, bool clearDeadVessels);       // :940
    static bool ForceSetActiveVessel(Vessel* v);                         // :958
    static void ClearDeadVessels();                                      // :1052
    static void SetVesselTarget(ITargetable* tgt, bool overrideInputLock = false);  // :668
    static void ToggleVesselEaseIn(Vessel* vessel, bool enableEaseIn, double easeInMultiplier = 1.0);  // :1266
    static void ClearInverseRotation();                                  // :1300

    // ---- queries (FlightGlobals.cs:1311-1495) --------------------------------------------------------------------------------------
    static CelestialBody* getMainBody(const Vector3d& refPos);  // :1311
    static CelestialBody* getMainBody();                        // :1333
    static Vector3d getGeeForceAtPosition(const Vector3d& pos);  // :1342
    static Vector3d getGeeForceAtPosition(const Vector3d& pos, const CelestialBody& mainBody);  // :1347
    static Vector3d getCentrifugalAcc(const Vector3d& pos, const CelestialBody& body);  // :1356
    static Vector3d getCoriolisAcc(const Vector3d& vel, const CelestialBody& body);     // :1366
    static double getExternalTemperature();                      // :1375
    static double getExternalTemperature(double altitude, CelestialBody* body = nullptr);  // :1380
    static double getExternalTemperature(const Vector3d& pos, CelestialBody* body = nullptr);  // :1393
    static double getStaticPressure(double altitude, CelestialBody* body = nullptr);  // :1406
    static double getStaticPressure(const Vector3d& position);   // :1419
    static double getStaticPressure(const Vector3d& position, const CelestialBody& body);  // :1429
    static double getStaticPressure();                           // :1434
    static double getAtmDensity(double pressure, double temperature, CelestialBody* body = nullptr);  // :1443
    static double getAltitudeAtPos(const Vector3d& position);     // :1456
    static float getAltitudeAtPos(const Vector3f& position);      // :1461
    static float getAltitudeAtPos(const Vector3f& position, CelestialBody* body);        // :1466 (float overload)
    static double getAltitudeAtPos(const Vector3d& position, CelestialBody* body);       // :1471
    static double GetSqrAltitude(const Vector3d& position, const CelestialBody& body);   // :1476
    static Vector3d getUpAxis();                                  // :1481
    static Vector3d getUpAxis(const Vector3d& position);          // :1486
    static Vector3d getUpAxis(const CelestialBody& body, const Vector3d& position);  // :1491
    static ClearToSaveStatus ClearToSave(bool logMsg = true);     // :1496 / :1501
    static std::string GetNotClearToSaveStatusReason(ClearToSaveStatus status, const std::string& attempt);  // :1574

    // ---- bodies by name / index, home body, comet perturber (FlightGlobals.cs:1736-1930) ---------------------------------------------
    static CelestialBody* GetBodyByName(const std::string& bodyName);  // :1736
    static int GetBodyIndex(const CelestialBody* body);                // :1768
    static int GetHomeBodyIndex();                                     // :1788
    static CelestialBody* GetHomeBody();                               // :1797
    static std::string GetHomeBodyName();                              // :1823
    static std::string GetHomeBodyDisplayName();                       // :1832
    static int GetCometPerturberIndex();                               // :1841
    static CelestialBody* GetCometPerturber();                         // :1850
    static std::string GetCometPerturberName();                        // :1914
    static std::string GGetCometPerturberDisplayName();                // :1923

    // ---- speed display (FlightGlobals.cs:1932-2003) -----------------------------------------------------------------------------------
    static void UpdateSpeedMode();                  // :1932
    static void SetSpeedMode(SpeedDisplayModes newSpeedMode);  // :1959
    static void CycleSpeedModes();                  // :1965
    static double GetDisplaySpeed();                // :1978
    static Vector3d GetDisplayVelocity();           // :1991

    // ---- the persistent-id dictionaries are SaveEnvironment's (WP-04, FlightGlobals.cs:473-660) -------------------------------------------------

private:
    static CelestialBody* inSOI(const Vector3d& pos, CelestialBody* body);  // :1316
    static void RefreshViews();
    static bool setActiveVessel(Vessel* v, bool force, bool clearDeadVessels);  // :967

    static inline SpeedDisplayModes speedMode = SpeedDisplayModes::Surface;          // :130
    static inline SpeedDisplayModes commandedSpeedMode = SpeedDisplayModes::Surface;  // :128
    static inline std::unordered_map<std::string, CelestialBody*> bodyNames;          // :116
    static inline CelestialBody* homeBody = nullptr;                                  // :118
    static inline int homeBodyIndex = 0;                                              // :120
    static inline CelestialBody* cometPerturber = nullptr;                            // :122
    static inline int cometPerturberIndex = 0;                                        // :124
    static inline bool disableOverridePending_ = false;
    static inline float disableOverrideAt_ = 0.0f;
};

}  // namespace ksp
