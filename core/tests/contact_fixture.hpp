// WP-20e test support: scenes for contact, landing and water on the physics bubble (WP-19b) with the FlightIntegrator tick (WP-20d) and the
// contact host (ksp/physics/contact_host.hpp): the WP-20d pad scene (Kerbin's PQS terrain patch around the LaunchPad), crafts and single parts
// placed at any point of Kerbin (on the terrain patch, or over the ocean), released with a chosen velocity.
//
// Declaration order in a test (the process rule "fixtures declare vessels last"): World, then the scene, then the vessels.
#pragma once

#include <algorithm>
#include <cmath>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "doctest/doctest.h"
#include "flight_integrator_fixture.hpp"
#include "ksp/input/game_settings.hpp"
#include "ksp/physics/contact_host.hpp"
#include "ksp/terrain/pqs_cache.hpp"
#include "ksp/terrain/pqs_mods_quad.hpp"
#include "ksp/terrain/pqs_prefab.hpp"

namespace wp20e {

using namespace ksp;
using namespace ksp::physics;
using wp20b::World;
using wp20d::PadScene;

// GameSettings.SetDefaultValues' VESSEL_ANCHOR_* (GameSettings.cs:1121-1126) for the duration of a test (the statics are 0 until a settings
// load; other tests run with them so: restored on exit)
struct AnchorSettings {
    float v = GameSettings::VESSEL_ANCHOR_VELOCITY_THRESHOLD, t = GameSettings::VESSEL_ANCHOR_TIME_THRESHOLD,
          a = GameSettings::VESSEL_ANCHOR_ANGLE_CHANGE_THRESHOLD, at = GameSettings::VESSEL_ANCHOR_ANGLE_TIME_THRESHOLD,
          f = GameSettings::VESSEL_ANCHOR_BREAK_FORCE_FACTOR, tq = GameSettings::VESSEL_ANCHOR_BREAK_TORQUE;
    bool wheel = GameSettings::WHEEL_DAMAGE_IMPACTCOLLIDER_ENABLED;
    explicit AnchorSettings(bool kspDefaults = true) {
        if (!kspDefaults) return;
        GameSettings::VESSEL_ANCHOR_VELOCITY_THRESHOLD = 0.05f;
        GameSettings::VESSEL_ANCHOR_TIME_THRESHOLD = 1.5f;
        GameSettings::VESSEL_ANCHOR_ANGLE_CHANGE_THRESHOLD = 20.0f;
        GameSettings::VESSEL_ANCHOR_ANGLE_TIME_THRESHOLD = 0.1f;
        GameSettings::VESSEL_ANCHOR_BREAK_FORCE_FACTOR = 1.05f;
        GameSettings::VESSEL_ANCHOR_BREAK_TORQUE = 100.0f;
        GameSettings::WHEEL_DAMAGE_IMPACTCOLLIDER_ENABLED = true;
    }
    ~AnchorSettings() {
        GameSettings::VESSEL_ANCHOR_VELOCITY_THRESHOLD = v;
        GameSettings::VESSEL_ANCHOR_TIME_THRESHOLD = t;
        GameSettings::VESSEL_ANCHOR_ANGLE_CHANGE_THRESHOLD = a;
        GameSettings::VESSEL_ANCHOR_ANGLE_TIME_THRESHOLD = at;
        GameSettings::VESSEL_ANCHOR_BREAK_FORCE_FACTOR = f;
        GameSettings::VESSEL_ANCHOR_BREAK_TORQUE = tq;
        GameSettings::WHEEL_DAMAGE_IMPACTCOLLIDER_ENABLED = wheel;
    }
};

// Kerbin in its rotating frame with the bubble and no terrain of its own (the pad scene without its terrain patch): the scene of the
// TerrainColliders-per-body test
struct KerbinScene {
    World& w;
    wp19b::BubbleScene s;  // WP-20g: the resource sets are the bubble's
    explicit KerbinScene(World& w_, const PhysicsBubbleOptions& o = {}, const PhysicsWorldConfig& cfg = {}) : w(w_), s(w_, o, cfg) {
        wp20d::StartPhysicsGlobals(w);
        SaveEnvironment::Current().loadedScene = GameScenes::FLIGHT;
        w.Kerbin().inverseRotation = true;
        w.planetarium.UpdateCBs();
        GameSettings::PHYSICS_EASE = true;
    }
    ~KerbinScene() {
        Vessel::SpawnedVessels().clear();
        GameSettings::PHYSICS_EASE = true;
        w.Kerbin().inverseRotation = false;
    }
};

// The events of a test (GameEvents listeners, removed with the object)
struct EventLog {
    std::vector<std::string> crashes, collisions, splashCrashes;
    std::vector<Part*> groundExplosions;  // onPartExplodeGroundCollision
    std::vector<std::pair<Vessel_Situations, Vessel_Situations>> situations;
    int explosions = 0;
    EventLog() {
        events::GameEvents::onCrash.Add(this, &EventLog::OnCrash);
        events::GameEvents::onCollision.Add(this, &EventLog::OnCollision);
        events::GameEvents::onCrashSplashdown.Add(this, &EventLog::OnSplash);
        events::GameEvents::onPartExplodeGroundCollision.Add(this, &EventLog::OnGround);
        events::GameEvents::onVesselSituationChange.Add(this, &EventLog::OnSituation);
        events::GameEvents::onPartExplode.Add(this, &EventLog::OnExplode);
    }
    ~EventLog() {
        events::GameEvents::onCrash.Remove(this, &EventLog::OnCrash);
        events::GameEvents::onCollision.Remove(this, &EventLog::OnCollision);
        events::GameEvents::onCrashSplashdown.Remove(this, &EventLog::OnSplash);
        events::GameEvents::onPartExplodeGroundCollision.Remove(this, &EventLog::OnGround);
        events::GameEvents::onVesselSituationChange.Remove(this, &EventLog::OnSituation);
        events::GameEvents::onPartExplode.Remove(this, &EventLog::OnExplode);
    }
    void OnCrash(EventReport* r) { crashes.push_back(r->sender + " / " + r->other); }
    void OnCollision(EventReport* r) { collisions.push_back(r->sender + " / " + r->other); }
    void OnSplash(EventReport* r) { splashCrashes.push_back(r->sender); }
    void OnGround(Part* p) { groundExplosions.push_back(p); }
    void OnSituation(events::GameEvents::HostedFromToAction<Vessel*, Vessel_Situations> a) { situations.emplace_back(a.from, a.to); }
    void OnExplode(events::GameEvents::ExplosionReaction) { ++explosions; }
};

// A point of Kerbin: the world axes there (craft X, Y, Z -> east, up, north as the pad scene) and the surface radius (PQS; for a sea point the
// sea level, Kerbin's radius)
struct Site {
    double lat = 0.0, lon = 0.0;
    Vector3d up, east, north;
    double surfaceRadius = 0.0, terrainRadius = 0.0;
    Vector3d SurfaceWorld(const CelestialBody& k) const { return k.position + up * surfaceRadius; }
};

template <class Scene>
Site SiteAt(Scene& pad, double lat, double lon, bool sea) {
    World& w = pad.w;
    CelestialBody& kerbin = w.Kerbin();
    Site s;
    s.lat = lat;
    s.lon = lon;
    const Vector3d rel = terrain::GetRelSurfaceNVector(lat, lon).normalized();
    s.terrainRadius = w.pqs ? w.pqs->GetSurfaceHeight(rel, true) : kerbin.Radius;
    s.surfaceRadius = sea ? kerbin.Radius : s.terrainRadius;
    s.up = kerbin.GetSurfaceNVector(lat, lon).normalized();
    const Vector3d polar = (kerbin.rotation * Vector3d(0.0, 1.0, 0.0)).normalized();
    s.north = (polar - s.up * Vector3d::Dot(polar, s.up)).normalized();
    s.east = Vector3d::Cross(s.up, s.north);
    return s;
}

// The lowest point of a vessel's proxy colliders below its root, in the vessel frame (y up)
inline double LowestProxyY(const Vessel& v) {
    double minY = 1e30;
    for (Part* p : v.parts) {
        if (!p->hasRigidbody) continue;
        for (const ColliderProxy& b : DragCubeColliderProxy(*p))
            for (int c = 0; c < 8; ++c) {
                const Vector3f corner = b.center + Vector3f((c & 1) ? b.halfExtents.x : -b.halfExtents.x, (c & 2) ? b.halfExtents.y : -b.halfExtents.y,
                                                            (c & 4) ? b.halfExtents.z : -b.halfExtents.z);
                minY = std::min(minY, static_cast<double>(p->fields.orgPos.y) + static_cast<double>((p->fields.orgRot * corner).y));
            }
    }
    return minY;
}

// PadScene::Place at any site: the vessel upright with its lowest proxy point `clearance` above the site's surface, packed, its OrbitDriver
// started (IDLE: VesselPrecalculate keeps it there until GoOffRails). landed: PRELAUNCH on the ground (as the pad scene); else FLYING.
template <class Scene>
void PlaceAt(Scene& pad, Vessel& v, const Site& s, double clearance, bool landed) {
    CelestialBody& kerbin = pad.w.Kerbin();
    const double minY = LowestProxyY(v);
    const QuaternionD rot = wp20b::BasisRotation(s.east, s.up, s.north);
    const Vector3d rootWorld = s.SurfaceWorld(kerbin) + s.up * (clearance - minY);
    v.loaded = true;
    v.packed = true;
    v.SetRotation(rot.ToQuaternionf(), true);
    v.SetPosition(rootWorld, true);
    v.Landed = landed;
    v.Splashed = false;
    v.situation = landed ? Vessel::Situations::PRELAUNCH : Vessel::Situations::FLYING;
    if (!landed) v.SetLandedAt("");  // the craft assembly launched it from the LaunchPad (AssembleOptions)
    v.staticPressurekPa = 101.0;  // FLYING until the FlightIntegrator's mirror runs
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

// One frame of the scene with Unity's level clock: Time.timeSinceLevelLoad advances by the frame time (Vessel.LateUpdate's bounce check at
// 5 s and updateSituation after 0.5 s read it; the bubble fixtures leave it at 10 s, which runs the bounce check in the first frame, before any
// contact). Call ResetLevelClock() when the scene starts.
inline void ResetLevelClock() { Vessel::env.timeSinceLevelLoad = 0.0f; }
template <class Scene>
void Tick(Scene& pad) {
    Vessel::env.timeSinceLevelLoad += Vessel::env.deltaTime;
    pad.s.Tick();
}

// Ticks until the vessel has its rigid bodies (GoOffRails), at most `maxTicks`; true when it has
template <class Scene>
bool TickUntilUnpacked(Scene& pad, Vessel& v, int maxTicks = 10) {
    for (int k = 0; k < maxTicks; ++k) {
        if (pad.s.bubble.Host().Find(v) != nullptr && !v.packed) return true;
        Tick(pad);
    }
    return pad.s.bubble.Host().Find(v) != nullptr && !v.packed;
}

// The vessel's centre of mass and velocity in the body-fixed frame (the world axes; frame velocity added back)
template <class Scene>
Vector3d VesselVelocity(Scene& pad, Vessel& v) {
    VesselPhysics* vp = pad.s.bubble.Host().Find(v);
    if (vp == nullptr) return Vector3d();
    return vp->ReadState().rb_velocityD + Krakensbane::GetFrameVelocity();
}

// The impact speeds the contact host reported to `p` (Enter, |Collision.relativeVelocity|)
inline std::vector<float> ImpactSpeeds(const ContactHost& ch, const Part* receiver) {
    std::vector<float> out;
    for (const ContactHost::DispatchRecord& r : ch.LastDispatch())
        if (r.kind == PhysicsWorld::ContactEvent::Kind::Enter && r.receiver == receiver) out.push_back(r.collision.relativeVelocity.magnitude());
    return out;
}

}  // namespace wp20e
