// WP-29c test support: a stock rover on flat ground in the flight scene (the WP-19b bubble with WP-20d's FlightIntegrator tick, the WP-20d
// pad scene) with the wheels' host (ksp/physics/vessel_wheels.hpp).
//
// The rovers are KSP's own stock craft (harness/inputs/Ships/SPH: Prospector Rover, 4 x RoveMax M1; Bug-E Buggy, 8 x RoveMax S2), loaded as
// the WP-18 gate and the wheel spike load them (KSPUpgradePipeline.Process, ShipConstruct.LoadShip, AssembleVesselFromCraft). "Flat ground" is
// the pad scene's 30 x 30 terrain patch without a PQS: the static meshes of Kerbin's sea-level sphere (the curvature of a 1.5 km patch is
// 0.47 m). Geometry: the wheel spike's drag-cube proxy (WheelPhysicsHost::ProxyGeometry) with the suspension along the craft's editor up
// (the root part's file rotation inverted, as the spike) and each wheel rolling along its part's forward axis (mirrored wheels mirrored:
// ModuleWheelMotor.GetMotorOrientationSign drives them with the opposite sign).
//
// Declaration order in a test (the process rule "fixtures declare vessels last"): World, RoverScene, then the vessels.
#pragma once

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "flight_integrator_fixture.hpp"
#include "ksp/physics/vessel_wheels.hpp"
#include "ksp/save/upgrade_pipeline.hpp"
#include "ksp/wheels/wheel_modules.hpp"

namespace wp29c {

using namespace ksp;
using namespace ksp::physics;
using wp20b::World;

// WP-29d: the stock wheel / gear / leg models (core/data/wheel_models) as the geometry source of the wheel host
inline WheelPhysicsOptions RealGeometry(WheelPhysicsOptions o = {}) {
    static const auto lib = std::make_shared<const WheelModelLibrary>(std::string(KSPCORE_DATA_DIR) + "/wheel_models");
    o.models = lib;
    return o;
}

inline std::string StockCraftPath(const std::string& name) {  // the SPH craft, else the VAB ones (the lander)
    const std::string sph = std::string(KSPCORE_HARNESS_DIR) + "/inputs/Ships/SPH/" + name + ".craft";
    if (std::filesystem::exists(sph)) return sph;
    return std::string(KSPCORE_HARNESS_DIR) + "/inputs/Ships/VAB/" + name + ".craft";
}

struct LoadedRover {
    std::unique_ptr<Vessel> vessel;
    Vector3f craftUp{0.0f, 1.0f, 0.0f};    // the editor's up in the vessel (root part) frame
    Vector3f craftFwd{0.0f, 0.0f, 1.0f};   // the drive direction: the root part's up projected on the ground plane
};

// KSPUpgradePipeline.Process + ShipConstruct.LoadShip + AssembleVesselFromCraft (World::Assemble); the editor up = inverse(root file rot) * Y
// `gearRetracted` (WP-29e): the craft as a save with its gear up, i.e. every ModuleWheelDeployment persisted as stateString = Retracted,
// position = 0 (ModuleWheelDeployment.OnStart then runs FSMSetup("Retracted"): the animation at its retracted end, TsubSys not reached, the
// stand-in collider created at the wheel set-up)
inline LoadedRover LoadRover(World& w, const std::string& name, bool gearRetracted = false) {
    LoadedRover out;
    const std::string path = StockCraftPath(name);
    ConfigNode::NodePtr root = ConfigNode::Load(path);
    REQUIRE(root != nullptr);
    const std::string* sn = root->GetValuePtr("ship");
    ConfigNode::NodePtr up;
    KSPUpgradePipeline::Process(root, sn ? *sn : std::string(), SaveUpgradePipeline::LoadContext::Craft, [&](const ConfigNode::NodePtr& n) { up = n; },
                                [&](KSPUpgradePipeline::UpgradeFailOption, const ConfigNode::NodePtr&) {});
    REQUIRE(up != nullptr);
    if (gearRetracted)
        for (ConfigNode* part : up->nodes().GetNodes("PART"))
            for (ConfigNode* mod : part->nodes().GetNodes("MODULE")) {
                const std::string* mn = mod->GetValuePtr("name");
                if (mn == nullptr || *mn != "ModuleWheelDeployment") continue;
                mod->values().SetValue("stateString", "Retracted", std::nullopt, 0, true);
                mod->values().SetValue("position", "0", std::nullopt, 0, true);
            }
    QuaternionD rootRot = QuaternionD::identity();
    if (ConfigNode* part = up->nodes().GetNode("PART"))
        if (const std::string* rs = part->GetValuePtr("rot")) {
            double q[4] = {0, 0, 0, 1};
            std::stringstream ss(*rs);
            std::string tok;
            for (int k = 0; k < 4 && std::getline(ss, tok, ','); ++k) q[k] = std::stod(tok);
            const double n = std::sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
            rootRot = QuaternionD(q[0] / n, q[1] / n, q[2] / n, q[3] / n);
        }
    const Vector3d upV = (QuaternionD::Inverse(rootRot) * Vector3d(0.0, 1.0, 0.0)).normalized();
    Vector3d fwd = Vector3d(0.0, 1.0, 0.0) - upV * upV.y;  // the root's up, projected
    if (fwd.magnitude() < 0.5) fwd = Vector3d(0.0, 0.0, 1.0) - upV * upV.z;
    out.craftUp = upV.ToVector3f();
    out.craftFwd = fwd.normalized().ToVector3f();
    out.vessel = w.Assemble(*up, name);
    return out;
}

// the rotation with columns x, y, z
inline QuaternionD Columns(const Vector3d& x, const Vector3d& y, const Vector3d& z) { return wp20b::BasisRotation(x, y, z); }

// The wheel parts' proxy geometry with the craft's up and each part's own forward (see the file header)
inline WheelPhysicsOptions RoverOptions(const LoadedRover& r, WheelPhysicsOptions o = {}) {
    if (o.models || o.modelRig) return o;  // WP-29d: the wheel collider transforms come from the model data, no proxy
    const Vector3f up = r.craftUp;
    o.geometry = [up](const Part& part, const ModuleWheelBase& base, WheelPartGeometry& g) {
        const Vector3d u = Vector3d(up).normalized();
        Vector3d f = QuaternionD(part.fields.orgRot) * Vector3d(0.0, 0.0, 1.0);
        f = f - u * Vector3d::Dot(f, u);
        if (f.magnitude() < 0.3) f = QuaternionD(part.fields.orgRot) * Vector3d(1.0, 0.0, 0.0), f = f - u * Vector3d::Dot(f, u);
        g = WheelPhysicsHost::ProxyGeometry(part, base, up, f.normalized().ToVector3f());
        return true;
    };
    return o;
}

// GameSettings' wheel block at KSP's defaults (GameSettings.SetDefaultValues, GameSettings.cs:1059-1067), restored afterwards
struct WheelSettingsScope {
    float weight = GameSettings::WHEEL_WEIGHT_STRESS_MULTIPLIER, slip = GameSettings::WHEEL_SLIP_STRESS_MULTIPLIER;
    int active = GameSettings::WHEEL_SUBSTEPS_ACTIVE, inactive = GameSettings::WHEEL_SUBSTEPS_INACTIVE;
    bool spring = GameSettings::WHEEL_AUTO_SPRINGDAMPER, steer = GameSettings::WHEEL_AUTO_STEERINGADJUST, legs = GameSettings::LEGS_ADVANCED_SUSPENSIONDAMPER;
    bool impact = GameSettings::WHEEL_DAMAGE_IMPACTCOLLIDER_ENABLED, collider = GameSettings::WHEEL_DAMAGE_WHEELCOLLIDER_ENABLED;
    WheelSettingsScope() {
        GameSettings::WHEEL_WEIGHT_STRESS_MULTIPLIER = 1.0f;
        GameSettings::WHEEL_SLIP_STRESS_MULTIPLIER = 1.0f;
        GameSettings::WHEEL_SUBSTEPS_ACTIVE = 8;
        GameSettings::WHEEL_SUBSTEPS_INACTIVE = 4;
        GameSettings::WHEEL_AUTO_SPRINGDAMPER = true;
        GameSettings::WHEEL_AUTO_STEERINGADJUST = true;
        GameSettings::LEGS_ADVANCED_SUSPENSIONDAMPER = true;
        GameSettings::WHEEL_DAMAGE_IMPACTCOLLIDER_ENABLED = true;
        GameSettings::WHEEL_DAMAGE_WHEELCOLLIDER_ENABLED = true;
    }
    ~WheelSettingsScope() {
        GameSettings::WHEEL_WEIGHT_STRESS_MULTIPLIER = weight;
        GameSettings::WHEEL_SLIP_STRESS_MULTIPLIER = slip;
        GameSettings::WHEEL_SUBSTEPS_ACTIVE = active;
        GameSettings::WHEEL_SUBSTEPS_INACTIVE = inactive;
        GameSettings::WHEEL_AUTO_SPRINGDAMPER = spring;
        GameSettings::WHEEL_AUTO_STEERINGADJUST = steer;
        GameSettings::LEGS_ADVANCED_SUSPENSIONDAMPER = legs;
        GameSettings::WHEEL_DAMAGE_IMPACTCOLLIDER_ENABLED = impact;
        GameSettings::WHEEL_DAMAGE_WHEELCOLLIDER_ENABLED = collider;
    }
};

struct RoverScene {
    World& w;
    WheelSettingsScope settings;
    wp20d::PadScene pad;
    WheelPhysicsHost wheels;
    // WP-20e: this flat ground lies at Kerbin's sea level (no PQS: PadScene's pad radius is the body radius), where KSP's PartBuoyancy
    // (PartBuoyancy.cs:231) finds the wheels below the water line and floats / drags them as in the ocean. The rover tests are about
    // ground, so the bubble's PartBuoyancy slot is off here; every other piece of the contact host runs. Crash damage is off too
    // (CheatOptions.NoCrashDamage, Part.cs:3613 HandleCollision): at 4x physics warp WITHOUT substeps (KSP's single 0.08 s step) the
    // Prospector's body bounces onto the ground at up to 7.1 m/s and KSP's crash tolerance explodes its root decoupler, which these wheel
    // measurements do not follow.
    static PhysicsBubbleOptions BubbleOptions() {
        PhysicsBubbleOptions b;
        b.contacts.buoyancy = false;
        return b;
    }
    // `colliders` (WP-21b): VesselPhysicsOptions::colliders of the bubble's host (the wheel host chains it for the parts that are not wheels)
    explicit RoverScene(World& w_, const WheelPhysicsOptions& o = {}, std::function<std::vector<ColliderProxy>(const Part&)> colliders = nullptr)
        : w(w_), pad(w_, BubbleOptions()), wheels(pad.s.bubble, o) {
        if (colliders) pad.s.bubble.Host().options.colliders = std::move(colliders);
        PhysicsGlobals::SetApplyDrag(false);  // the references below have no aero (the spike's); drag is FlightIntegrator's, gated in WP-20d
        PartContactEnvironment::noCrashDamage = true;
        wheels.Install();
    }
    ~RoverScene() {
        wheels.Uninstall();
        PhysicsGlobals::SetApplyDrag(true);
        PartContactEnvironment::noCrashDamage = false;
    }
    // the rover on the pad, its craft up along the local vertical, its drive direction `headingDeg` from north, the lowest proxy point
    // `clearance` above the ground (the wheels at full extension touch it at 0); then as wp20d::PadScene::Place (landed, packed, orbit driver)
    void Place(const LoadedRover& r, double headingDeg = 0.0, double clearance = 0.0, bool airborne = false) {
        Vessel& v = *r.vessel;
        CelestialBody& kerbin = w.Kerbin();
        const Vector3d upC = Vector3d(r.craftUp).normalized(), fwdC = Vector3d(r.craftFwd).normalized();
        const QuaternionD C = Columns(Vector3d::Cross(upC, fwdC), upC, fwdC);
        const double h = headingDeg * 3.14159265358979323846 / 180.0;
        const Vector3d fwdW = (pad.padNorth * std::cos(h) + pad.padEast * std::sin(h)).normalized();
        const QuaternionD W = Columns(Vector3d::Cross(pad.padUp, fwdW), pad.padUp, fwdW);
        const QuaternionD rot = W * QuaternionD::Inverse(C);
        double minY = 1e30;
        for (Part* p : v.parts) {
            if (!p->hasRigidbody) continue;
            for (const ColliderProxy& b : DragCubeColliderProxy(*p))
                for (int c = 0; c < 8; ++c) {
                    const Vector3f corner = b.center + Vector3f((c & 1) ? b.halfExtents.x : -b.halfExtents.x, (c & 2) ? b.halfExtents.y : -b.halfExtents.y,
                                                                (c & 4) ? b.halfExtents.z : -b.halfExtents.z);
                    const Vector3d pt = Vector3d(p->fields.orgPos) + QuaternionD(p->fields.orgRot) * Vector3d(corner);
                    minY = std::min(minY, Vector3d::Dot(pt, upC));
                }
        }
        v.loaded = true;
        v.packed = true;
        v.SetRotation(rot.ToQuaternionf(), true);
        const Vector3d rootWorld = pad.padWorld + pad.padUp * (clearance - minY);
        v.SetPosition(rootWorld, true);
        v.Landed = !airborne;  // airborne: a drop (WP-29d), no landed placement (Vessel.CheckGroundCollision) moves it to the ground
        v.Splashed = false;
        v.situation = airborne ? Vessel::Situations::FLYING : Vessel::Situations::PRELAUNCH;
        kerbin.GetLatLonAlt(Vector3d(v.vesselTransform.position), v.latitude, v.longitude, v.altitude);
        v.srfRelRotation = QuaternionExtensions::Inverse(kerbin.rotation.ToQuaternionf()) * v.vesselTransform.rotation;
        v.localCoM = wp19b::VesselFrameCom(v).ToVector3f();
        v.lastBody = &kerbin;
        const double ut = Planetarium::GetUniversalTime();
        const Vector3d rel = Vector3d(v.vesselTransform.position) - kerbin.position;
        auto o = std::make_shared<Orbit>();
        o->UpdateFromStateVectors(rel.xzy(), kerbin.getRFrmVel(Vector3d(v.vesselTransform.position)).xzy(), &kerbin, ut);
        v.orbitDriver->orbit = o;
        v.orbitDriver->updateMode = OrbitDriver::UpdateMode::IDLE;
        if (std::find(FlightGlobals::Vessels.begin(), FlightGlobals::Vessels.end(), &v) == FlightGlobals::Vessels.end()) FlightGlobals::AddVessel(&v);
        FlightGlobals::SortVessel(&v, true);
        StartPartsInFlight(v);
        v.UpdateResourceSets();
        v.CoMD = rootWorld + rot * Vector3d(v.localCoM);
        v.orbitDriver->Start();
    }
    void Tick() { pad.s.Tick(); }
    // physics warp: Time.fixedDeltaTime = 0.02 x rate (TimeWarp.cs:423), the Planetarium clock the same, one frame per fixed step
    void SetPhysicsWarp(float rate) {
        pad.s.tw.Mode = rate > 1.0f ? TimeWarp::Modes::LOW : TimeWarp::Modes::HIGH;
        pad.s.tw.curr_rate = rate;
        Time::fixedDeltaTime = 0.02f * rate;
        Time::deltaTime = Time::fixedDeltaTime;
        w.planetarium.fixedDeltaTime = static_cast<double>(Time::fixedDeltaTime);
    }
};

// ---- measurements -------------------------------------------------------------------------------------------------------------------------
inline Vector3d VesselCoM(VesselPhysics& vp) {
    double m = 0.0;
    Vector3d s;
    for (size_t c = 0; c < vp.ClusterCount(); ++c) {
        s += vp.ClusterCenterOfMass(c) * vp.ClusterMass(c);
        m += vp.ClusterMass(c);
    }
    return s * (1.0 / m);
}
inline Vector3d VesselVelocity(VesselPhysics& vp) {
    double m = 0.0;
    Vector3d s;
    for (size_t c = 0; c < vp.ClusterCount(); ++c) {
        s += vp.ClusterVelocity(c) * vp.ClusterMass(c);
        m += vp.ClusterMass(c);
    }
    return s * (1.0 / m);
}
// WP-29e: the largest distance between a collider centre the cluster compound holds for a wheel part (Jolt's child pose composed with the body pose) and the
// nearest collider of the part's live model rig. Zero when the compound follows the model (the hull colliders of a gear leg ride the animation clip, the colliders
// under a rover's suspension ride the suspension; the stand-in sphere counts at its recorded centre). Hull children (a mesh collider: centre at the frame origin) are skipped.
inline double MaxColliderModelError(VesselPhysics& vp, PhysicsWorld& world, WheelPart& w) {
    const Part& part = *w.part;
    const int cl = vp.ClusterOf(part);
    if (cl < 0 || !w.rig) return 1e9;
    const BodyId body = vp.ClusterBody(static_cast<size_t>(cl));
    const Vector3d origin = world.GetPosition(body);
    const QuaternionD rot = world.GetRotation(body);
    VehiclePhysics::NativeTransform pt;
    pt.position = part.partTransform.position;
    pt.rotation = part.partTransform.rotation;
    const auto live = w.rig->Colliders();
    double worst = 0.0;
    Vector3f cp;
    Quaternionf cr;
    for (size_t k = 0; vp.PartColliderPose(part, k, cp, cr); ++k) {
        const Vector3d cw = origin + rot * Vector3d(cp);
        double best = 1e9;
        for (const auto& s : live) best = std::min(best, (cw - vp.PhysicsFromTransform(Vector3d(pt.TransformPoint(s.center)))).magnitude());
        if (w.standInBuilt) best = std::min(best, (cw - vp.PhysicsFromTransform(Vector3d(pt.TransformPoint(w.standInCenter)))).magnitude());  // the stand-in sphere is not in the model
        if (Vector3d(cp).magnitude() > 0.0) worst = std::max(worst, best);
    }
    return worst;
}

// the brakes action group: ModuleWheelBrakes.BrakeAction(Activate / Deactivate) on every brake module (ModuleWheelBrakes.cs:66)
inline void SetBrakes(Vessel& v, bool on) {
    for (Part* p : v.parts)
        for (size_t i = 0; i < p->Modules().Count(); ++i)
            if (auto* b = dynamic_cast<ModuleWheels::ModuleWheelBrakes*>(p->Modules()[i])) {
                KSPActionParam param(0, on ? KSPActionType::Activate : KSPActionType::Deactivate);
                b->BrakeAction(param);
            }
}
// the batteries full (the drive references assume ElectricCharge is always there, as the spike's)
inline void FillElectricCharge(Vessel& v) {
    for (Part* p : v.parts)
        if (PartResource* r = p->GetResource("ElectricCharge")) r->amount = r->maxAmount;
}

}  // namespace wp29c
