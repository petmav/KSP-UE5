// Wheel spike: a wheeled / legged vessel on the REDUCED rigid-body model (core/PHYSICS-SPIKE.md): the whole vessel is one Jolt
// body (mass, CoM and inertia summed with KSP's per-part rules, parts' collider proxies welded into one compound; wheel and leg
// parts contribute mass but no collider: their ground contact is the wheel), and every ModuleWheelBase part is a suspension
// element on that body.
//
// Per physics tick, KSP's order (WHEEL-SPIKE.md 1.4):
//   1. VesselPrecalculate: body state, gravity (+ Coriolis + centrifugal, rotating frame) at the CoM
//   2. managed wheel code (wheel_model.hpp): SuspensionLoadBalancer, ModuleWheelBase / Suspension / Motor / Brakes, then
//      KSPWheelController (VehicleBase.FixedUpdate): reads last tick's WheelHit, integrates the wheel spin, applies the tyre force
//      at the contact point, writes the spring / damper / targetPosition for the native step
//   3. native suspension (what UnityEngine.WheelCollider = PhysX vehicle SDK does inside Physics.Simulate):
//        Explicit     a ray per wheel against the scene, PhysX's suspension law F = max(0, k jounce) + c d(jounce)/dt (gravity
//                     term zero: KSP's Physics.gravity is (0,0,0)), force along the hit normal at the contact point, the
//                     suspension-limit constraint as a projected Gauss-Seidel impulse; all explicit, like PhysX
//        JoltVehicle  Jolt's VehicleConstraint (ray tester, soft-constraint spring solved implicitly in the velocity solver,
//                     hard limit at full compression) with a null friction controller: Jolt does suspension only, the tyre
//                     forces stay KSP's (VPP); spring / damper / rest offset set from the managed values every tick
//   4. the Jolt step
//   5. WheelHit for the next tick (contact point + rb.velocity * dt, normal, PhysX-reported spring force)
#pragma once

#include <Jolt/Jolt.h>

#include <Jolt/Math/DVec3.h>
#include <Jolt/Physics/Body/Body.h>

#include <memory>
#include <string>
#include <vector>

#include "craft_model.hpp"
#include "jolt_world.hpp"
#include "terrain_patch.hpp"
#include "wheel_model.hpp"

namespace spike {
class KspContext;
}

namespace wspike {

enum class NativeModel { Explicit, JoltVehicle };
const char* NativeName(NativeModel m);

enum class Ground { PQS, Flat };

struct RoverConfig {
    NativeModel native = NativeModel::Explicit;
    TireFriction::Model friction = TireFriction::Model::Smooth;
    Ground ground = Ground::PQS;
    double dt = 0.02;               // 0.02 x physics warp rate (TimeWarp.cs:423)
    bool rotatingFrame = true;      // Kerbin body-fixed frame (below the inverse-rotation threshold)
    bool gravityEase = true;        // VesselPrecalculate easing: 16 ticks after unpack
    bool inertiaFloor = true;       // Part.cs:4263 per-part tensor x max(1, rb.mass)
    double gravityScale = 1.0;      // gravity x (Mun-like tests: 0.166) -- also scales GeeASL seen by the modules
    float spawnClearance = 0.05f;   // lowest point above the ground at spawn (m)
    double spawnVerticalSpeed = 0.0;  // m/s along up (touchdown: -3)
    double startOffsetEast = 0.0, startOffsetNorth = 0.0;  // spawn point relative to the pad (m)
    double headingDeg = 0.0;        // vessel forward: 0 = north, 90 = east
    // Leg geometry proxy: false = the bottom of the leg's "Deployed" drag cube is the foot at full extension (the wheel rule);
    // true = KSP's groundHeightOffset below the part origin (ModuleWheelBase.OnPutToGround). They differ by ~0.85 m on the LT-2.
    bool legGroundOffset = false;
    int limitIterations = 4;        // Explicit: PGS iterations of the suspension-limit rows
    float limitBaumgarte = 0.2f;
    // The editor's up axis in the assembled craft frame. AssembleVesselFromCraft makes the ROOT part's rotation identity
    // (orgPos / orgRot relative to the root, vessel_assembly.cpp:92-129, as ShipConstruction.AssembleForLaunch does), so for an
    // SPH craft (root rotated 90 deg about X in the file) the ground normal is inverse(root file rotation) * Y. Set by LoadCraft.
    JPH::Vec3 craftUp = JPH::Vec3::sAxisY();
    spike::WorldConfig world;
};

// Per-tick controls (FlightCtrlState + the brake action group)
struct Controls {
    float wheelThrottle = 0.0f;
    float brake = 0.0f;
};

struct PhaseTimes {
    double precalc = 0, managed = 0, nativePre = 0, step = 0, nativePost = 0, total = 0;
};

struct WheelGeom {
    int part = -1;
    JPH::Vec3 transformLocal;   // wheelCollider transform origin, body frame (origin = body CoM)
    JPH::Vec3 upLocal, forwardLocal, rightLocal;
    JPH::Vec3 partComLocal;     // the wheel part's CoM (rb.velocity point), body frame
    float partRbMass = 0.0f;
};

class Rover {
public:
    Rover(spike::KspContext& ctx, spike::CraftModel& craft, const RoverConfig& cfg);
    ~Rover();
    Rover(const Rover&) = delete;
    Rover& operator=(const Rover&) = delete;

    void Tick(const Controls& c);

    // ---- state / telemetry ----
    const RoverConfig& Config() const { return cfg_; }
    int WheelCount() const { return static_cast<int>(wheels_.size()); }
    const KspWheel& Wheel(int i) const { return wheels_[i]; }
    KspWheel& Wheel(int i) { return wheels_[i]; }
    const WheelGeom& Geom(int i) const { return geom_[i]; }
    double Mass() const { return mass_; }
    double Time() const { return time_; }
    int TickCount() const { return tick_; }
    JPH::DVec3 Com() const;              // body-fixed (Kerbin centre origin)
    JPH::Vec3 Velocity() const;          // physics frame (body-fixed)
    JPH::Vec3 AngularVelocity() const;
    JPH::DVec3 Up() const { return up_; }
    JPH::DVec3 Forward() const;          // vessel drive forward (physics frame, current)
    double Altitude() const;             // CoM height above the start ground level along the start up axis (m)
    double GroundAltitude() const { return groundRadius_; }
    double SpawnTerrainHeight() const { return terrainHeightAtSpawn_; }
    double LowestWheelClearanceAtSpawn() const { return spawnClearance_; }
    int GroundedCount() const;
    bool BodyTouchesGround() const;      // a non-wheel collider in contact with static geometry
    const PhaseTimes& LastTimes() const { return times_; }
    int StaticBodies() const { return statics_; }
    double TerrainMinHeight() const { return terrainMin_; }
    double TerrainMaxHeight() const { return terrainMax_; }
    const spike::CraftModel& Craft() const { return craft_; }
    // Explicit native model: wheels that hit the suspension limit this tick
    int BottomedCount() const;

private:
    void BuildBody();
    void BuildWheels();
    void BuildNative();
    void NativeExplicitPre(float dt, JPH::Vec3& force, JPH::Vec3& torque);
    void NativeJoltPre();
    void NativeJoltPost(float dt);
    void UpdateWheelFrames();

    spike::KspContext& ctx_;
    spike::CraftModel& craft_;
    RoverConfig cfg_;
    std::unique_ptr<spike::World> world_;
    JPH::Body* body_ = nullptr;
    JPH::BodyID bodyId_;
    std::vector<KspWheel> wheels_;
    std::vector<WheelGeom> geom_;
    std::vector<float> prevJounce_;   // PhysX prevJounces (Explicit); NaN = none (PX_MAX_F32)
    std::vector<float> limitLambda_;
    struct JoltVehicle;
    std::unique_ptr<JoltVehicle> jv_;
    double mass_ = 0.0;
    JPH::Vec3 comCraft_;              // CoM in the craft frame
    JPH::Mat44 inertiaBody_;          // about the CoM, body frame
    JPH::Quat craftToBody_ = JPH::Quat::sIdentity();
    JPH::DVec3 up_, east_, north_;
    JPH::DVec3 startCom_;
    double groundRadius_ = 0.0;       // PQS surface radius at the spawn point
    double terrainHeightAtSpawn_ = 0.0, spawnClearance_ = 0.0;
    double terrainMin_ = 0.0, terrainMax_ = 0.0;
    int statics_ = 0;
    double mu_ = 0.0;
    JPH::DVec3 omega_;
    double geeASL_ = 1.0;
    double time_ = 0.0;
    int tick_ = 0;
    PhaseTimes times_;
    VesselInputs inputs_;
    std::vector<bool> isWheelPart_;
    spike::TerrainPatch terrain_;
    JPH::Vec3 forwardCraft_ = JPH::Vec3::sAxisZ();  // vessel drive forward, craft frame (root part up, horizontal)
    JPH::Vec3 upCraft_ = JPH::Vec3::sAxisY();
    double lowestPartY_ = 0.0;                       // lowest non-wheel collider point, craft frame
    std::vector<JPH::RVec3> hitRaw_;                 // native contact points before the + rb.velocity * dt fix-up
    bool bodyTouches_ = false;
};

}  // namespace wspike
