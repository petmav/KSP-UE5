// WP-29c: wheels on the reduced rigid-body model; see include/ksp/physics/vessel_wheels.hpp. C# citations are relative to
// decompiled/KSP-Decompiled/Assembly-CSharp/, PhysX ones to physx/source/physxvehicle/src/ of PhysX 4.1. The managed wheel code is WP-29a /
// WP-29b's; the native law is ksp/wheels/native_wheel_collider.hpp; this file is the engine glue between them and the WP-20b bindings
// (double precision, no ported KSP statement except where cited).
#include "ksp/physics/vessel_wheels.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <optional>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/flight_globals.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/config/parse_extensions.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/physics/mu_part_colliders.hpp"
#include "ksp/physics/physics_bubble.hpp"
#include "ksp/physics/wheel_visual.hpp"
#include "ksp/unity/quaternion_native.hpp"
#include "ksp/vessel/flight_scene.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp::physics {

using namespace VehiclePhysics;
using namespace ModuleWheels;

namespace {

ModuleWheelBase* WheelBaseOf(const Part& p) {
    for (size_t i = 0; i < p.Modules().Count(); ++i)
        if (auto* b = dynamic_cast<ModuleWheelBase*>(p.Modules()[i])) return b;
    return nullptr;
}

bool IsWheelPart(const Part& p) { return WheelBaseOf(p) != nullptr; }

// 3x3 inverse (cofactors); zero for a singular matrix
void Inverse3(const double a[3][3], double o[3][3]) {
    const double c00 = a[1][1] * a[2][2] - a[1][2] * a[2][1], c01 = a[1][2] * a[2][0] - a[1][0] * a[2][2], c02 = a[1][0] * a[2][1] - a[1][1] * a[2][0];
    const double det = a[0][0] * c00 + a[0][1] * c01 + a[0][2] * c02;
    if (!(std::fabs(det) > 0.0)) {
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j) o[i][j] = 0.0;
        return;
    }
    const double s = 1.0 / det;
    o[0][0] = c00 * s;
    o[1][0] = c01 * s;
    o[2][0] = c02 * s;
    o[0][1] = (a[0][2] * a[2][1] - a[0][1] * a[2][2]) * s;
    o[1][1] = (a[0][0] * a[2][2] - a[0][2] * a[2][0]) * s;
    o[2][1] = (a[0][1] * a[2][0] - a[0][0] * a[2][1]) * s;
    o[0][2] = (a[0][1] * a[1][2] - a[0][2] * a[1][1]) * s;
    o[1][2] = (a[0][2] * a[1][0] - a[0][0] * a[1][2]) * s;
    o[2][2] = (a[0][0] * a[1][1] - a[0][1] * a[1][0]) * s;
}
Vector3d Mul3(const double m[3][3], const Vector3d& v) {
    return {m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z, m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z, m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z};
}

// the rotation whose columns are x, y, z (an orthonormal basis)
QuaternionD Basis(const Vector3d& x, const Vector3d& y, const Vector3d& z) {
    const double m00 = x.x, m01 = y.x, m02 = z.x, m10 = x.y, m11 = y.y, m12 = z.y, m20 = x.z, m21 = y.z, m22 = z.z;
    const double tr = m00 + m11 + m22;
    QuaternionD q;
    if (tr > 0) {
        const double s = std::sqrt(tr + 1.0) * 2;
        q = {(m21 - m12) / s, (m02 - m20) / s, (m10 - m01) / s, 0.25 * s};
    } else if (m00 > m11 && m00 > m22) {
        const double s = std::sqrt(1.0 + m00 - m11 - m22) * 2;
        q = {0.25 * s, (m01 + m10) / s, (m02 + m20) / s, (m21 - m12) / s};
    } else if (m11 > m22) {
        const double s = std::sqrt(1.0 + m11 - m00 - m22) * 2;
        q = {(m01 + m10) / s, 0.25 * s, (m12 + m21) / s, (m02 - m20) / s};
    } else {
        const double s = std::sqrt(1.0 + m22 - m00 - m11) * 2;
        q = {(m02 + m20) / s, (m12 + m21) / s, 0.25 * s, (m10 - m01) / s};
    }
    return q;
}

// the wheel collider transform's pose in the transform frame: the part transform composed with the part-space geometry
void PoseFromPart(const PartTransform& pt, const WheelPartGeometry& g, NativeTransform& out) {
    NativeTransform part;
    part.position = pt.position;
    part.rotation = pt.rotation;
    out.position = part.TransformPoint(g.position);
    out.rotation = pt.rotation * g.rotation;
}

// a model transform's pose in the transform frame: the part transform composed with its part-space pose (PartModelRig::PartSpace)
void PoseOfRig(const PartTransform& pt, const mu::RigPose& rp, NativeTransform& out) {
    NativeTransform part;
    part.position = pt.position;
    part.rotation = pt.rotation;
    out.position = part.TransformPoint(rp.position);
    out.rotation = pt.rotation * rp.rotation;
}

bool SamePose(const NativeTransform& a, const NativeTransform& b) {
    return std::memcmp(&a.position, &b.position, sizeof(a.position)) == 0 && std::memcmp(&a.rotation, &b.rotation, sizeof(a.rotation)) == 0;
}

struct TimeScope {  // Unity's FixedUpdate: Time.deltaTime == Time.fixedDeltaTime == the step
    float fdt = Time::fixedDeltaTime, dt = Time::deltaTime;
    explicit TimeScope(float h) {
        Time::fixedDeltaTime = h;
        Time::deltaTime = h;
    }
    ~TimeScope() {
        Time::fixedDeltaTime = fdt;
        Time::deltaTime = dt;
    }
};

}  // namespace

KSPWheelController* WheelPart::Controller() const { return base != nullptr ? base->Wheel() : nullptr; }

// ===================================================================================== VesselWheels

VesselWheels::VesselWheels(WheelPhysicsHost& host, Vessel& vessel) : host_(&host), vessel_(&vessel) {}
VesselWheels::~VesselWheels() = default;

WheelPart* VesselWheels::Find(const Part& p) const {
    for (const auto& w : wheels_)
        if (w->part == &p) return w.get();
    return nullptr;
}

int VesselWheels::SetUpCount() const {
    int n = 0;
    for (const auto& w : wheels_) n += (w->base != nullptr && w->base->IsWheelSetUp()) ? 1 : 0;
    return n;
}

int VesselWheels::GroundedCount() const {
    int n = 0;
    for (const auto& w : wheels_) n += w->last.grounded ? 1 : 0;
    return n;
}

// ===================================================================================== the host

struct WheelPhysicsHost::Saved {
    CoroutineScheduler* scheduler = nullptr;
    std::function<double()> maximumSurfaceGeeASL;
    std::function<float()> timeSinceLevelLoad;
    std::function<WheelColliderInfo(const Collider*)> describeCollider;
    std::function<NativeTransform*(Part&, const std::string&)> findModelTransform;
    std::function<bool(const Vessel*)> isActiveVessel;
    std::function<bool(const Part&)> shieldedFromAirstream;
    std::function<void()> beforeFlightIntegrator, beforeVesselUpdate;
    std::function<void(float)> physicsStep;
    std::function<bool(const Part&)> massOnly;
    std::function<std::vector<ColliderProxy>(const Part&)> colliders;
    std::function<bool(const Part&)> mutableColliders;
    std::function<Animation*(Part&, const std::string&, const std::string&)> findModelAnimator;
    std::function<void(Animation&)> sampleAnimation;
    std::function<void(Part&, bool)> setRigidbodyFrozen;
    std::function<void(Part&, VPWheelCollider&)> wheelColliderCreated;
    std::function<std::vector<VesselEnvironment::ColliderPoint>(Part&, const Vector3f&)> colliderClosestPoints;
};

WheelPhysicsHost::WheelPhysicsHost(PhysicsBubble& bubble, const WheelPhysicsOptions& opts) : options(opts), bubble_(&bubble) {}

VehiclePhysics::Collider* WheelPhysicsHost::StaticColliderFor(const StaticColliderInfo& info) {
    const std::string key = std::to_string(info.layer) + "|" + info.tag + "|" + info.crashObjectName;
    auto it = staticColliders_.find(key);
    if (it == staticColliders_.end()) {
        auto sc = std::make_unique<StaticCollider>();
        sc->info = info;
        it = staticColliders_.emplace(key, std::move(sc)).first;
    }
    return &it->second->collider;
}

WheelPhysicsHost::~WheelPhysicsHost() {
    Uninstall();
    // the wheel modules keep raw pointers into the records: detach them from the parts that are still alive
    for (auto& vw : vessels_) vw->wheels_.clear();
    vessels_.clear();
}

void WheelPhysicsHost::Install() {
    if (installed_) return;
    saved_ = std::make_unique<Saved>();
    Saved& s = *saved_;
    s.scheduler = WheelModuleEnvironment::scheduler;
    s.maximumSurfaceGeeASL = WheelModuleEnvironment::maximumSurfaceGeeASL;
    s.timeSinceLevelLoad = WheelModuleEnvironment::timeSinceLevelLoad;
    s.describeCollider = WheelModuleEnvironment::describeCollider;
    s.findModelTransform = WheelModuleEnvironment::findModelTransform;
    s.isActiveVessel = WheelModuleEnvironment::isActiveVessel;
    s.shieldedFromAirstream = WheelModuleEnvironment::shieldedFromAirstream;
    s.findModelAnimator = WheelModuleEnvironment::findModelAnimator;
    s.sampleAnimation = WheelModuleEnvironment::sampleAnimation;
    s.colliderClosestPoints = Vessel::env.colliderClosestPoints;
    s.setRigidbodyFrozen = WheelModuleEnvironment::setRigidbodyFrozen;
    s.wheelColliderCreated = WheelModuleEnvironment::wheelColliderCreated;
    WheelModuleEnvironment::wheelColliderCreated = [this](Part& p, VPWheelCollider& vc) {
        WheelPart* rec = RecordOf(p);
        if (rec != nullptr) InstallVisual(*rec, vc);
    };
    WheelModuleEnvironment::scheduler = &scheduler_;
    WheelModuleEnvironment::setRigidbodyFrozen = [this](Part& p, bool freeze) {
        if (freeze) frozen_.insert(&p);
        else frozen_.erase(&p);
    };
    // PSystemManager.cs:239-245 MaximumSurfaceGeeASL: the largest GeeASL of the system's bodies
    WheelModuleEnvironment::maximumSurfaceGeeASL = [] {
        double m = 0.0;
        for (const CelestialBody* b : FlightGlobals::Bodies())
            if (b != nullptr) m = std::max(m, b->GeeASL);
        return m;
    };
    WheelModuleEnvironment::timeSinceLevelLoad = [this] { return timeSinceLevelLoad_; };
    WheelModuleEnvironment::describeCollider = [this](const Collider* c) {
        WheelColliderInfo info;
        info.tag = "Untagged";
        if (c == &terrain_) {
            info.layer = 15;  // the PQS quads' layer "Local Scenery" (PQS.cs:1015)
            return info;
        }
        for (const auto& kv : staticColliders_)   // WP-12c: a tagged static collider (the KSC runway, the launch pad ...)
            if (&kv.second->collider == c) {
                info.layer = kv.second->info.layer;
                info.tag = kv.second->info.tag;
                info.hasCrashObjectName = !kv.second->info.crashObjectName.empty();
                return info;
            }
        for (const auto& kv : others_)
            if (&kv.second->collider == c) {
                info.layer = 0;  // a part collider
                info.part = kv.second->part;
                return info;
            }
        return info;
    };
    WheelModuleEnvironment::findModelTransform = [this](Part& p, const std::string& name) { return FindModelTransform(p, name); };
    WheelModuleEnvironment::findModelAnimator = [this](Part& p, const std::string& trf, const std::string& state) { return FindAnimator(p, trf, state); };
    WheelModuleEnvironment::sampleAnimation = [this](Animation& a) { SampleAnimation(a); };
    // WP-29d: the landed placement (Vessel.CheckGroundCollision -> getLowestPoint) sees a wheel part colliders with their roles
    Vessel::env.colliderClosestPoints = [this, prev = s.colliderClosestPoints](Part& p, const Vector3f& point) -> std::vector<VesselEnvironment::ColliderPoint> {
        if (IsWheelPart(p) && options.modelColliders) {
            std::vector<VesselEnvironment::ColliderPoint> pts = PlacementPoints(p, point);
            if (!pts.empty()) return pts;
        }
        std::vector<VesselEnvironment::ColliderPoint> out;
        if (prev) return prev(p, point);
        if (Vessel::env.colliderClosestPointsZ)
            for (float z : Vessel::env.colliderClosestPointsZ(p, point)) out.push_back({z, VesselEnvironment::ColliderRole::Plain, 0.0f});
        return out;
    };
    WheelModuleEnvironment::isActiveVessel = [](const Vessel* v) { return v == FlightGlobals::ActiveVessel; };
    WheelModuleEnvironment::shieldedFromAirstream = [](const Part& p) { return p.ShieldedFromAirstream(); };
    // the wheel parts are welded without a collision shape (their tyre is the ray)
    VesselPhysicsHost& host = bubble_->Host();
    s.massOnly = host.options.massOnly;
    s.colliders = host.options.colliders;
    s.mutableColliders = host.options.mutableColliders;
    // WP-29d: with model data the wheel part's hub colliders (the .mu colliders at the part's animation state) weld into the cluster as
    // shapes; without, the part is mass-only
    host.options.massOnly = [this, prev = s.massOnly](const Part& p) {
        if (IsWheelPart(p) && !(options.modelColliders && !HubColliders(p).empty())) return true;
        return prev && prev(p);
    };
    host.options.mutableColliders = [this, prev = s.mutableColliders](const Part& p) {
        if (IsWheelPart(p) && HasMovingColliders(p)) return true;
        return prev && prev(p);
    };
    host.options.colliders = [this, prev = s.colliders](const Part& p) {
        if (IsWheelPart(p) && options.modelColliders) {
            std::vector<ColliderProxy> hub = HubColliders(p);
            if (!hub.empty()) return hub;
        }
        return prev ? prev(p) : DragCubeColliderProxy(p);
    };
    // the managed slot after the part modules' forces, the frame, the physics step
    s.beforeFlightIntegrator = FlightScene::hooks.beforeFlightIntegrator;
    FlightScene::hooks.beforeFlightIntegrator = [this, prev = s.beforeFlightIntegrator] {
        if (prev) prev();
        FixedUpdateManaged();
    };
    s.beforeVesselUpdate = FlightScene::hooks.beforeVesselUpdate;
    FlightScene::hooks.beforeVesselUpdate = [this, prev = s.beforeVesselUpdate] {
        if (prev) prev();
        Update();
    };
    s.physicsStep = bubble_->physicsStep;
    bubble_->physicsStep = [this](float dt) { PhysicsStep(dt); };
    installed_ = true;
}

void WheelPhysicsHost::Uninstall() {
    if (!installed_) return;
    Saved& s = *saved_;
    WheelModuleEnvironment::scheduler = s.scheduler;
    WheelModuleEnvironment::maximumSurfaceGeeASL = s.maximumSurfaceGeeASL;
    WheelModuleEnvironment::timeSinceLevelLoad = s.timeSinceLevelLoad;
    WheelModuleEnvironment::describeCollider = s.describeCollider;
    WheelModuleEnvironment::findModelTransform = s.findModelTransform;
    WheelModuleEnvironment::isActiveVessel = s.isActiveVessel;
    WheelModuleEnvironment::shieldedFromAirstream = s.shieldedFromAirstream;
    WheelModuleEnvironment::findModelAnimator = s.findModelAnimator;
    WheelModuleEnvironment::sampleAnimation = s.sampleAnimation;
    Vessel::env.colliderClosestPoints = s.colliderClosestPoints;
    WheelModuleEnvironment::setRigidbodyFrozen = s.setRigidbodyFrozen;
    WheelModuleEnvironment::wheelColliderCreated = s.wheelColliderCreated;
    for (auto& kv : freezes_) bubble_->World().RemoveConstraint(kv.second.id);
    freezes_.clear();
    frozen_.clear();
    bubble_->Host().options.massOnly = s.massOnly;
    bubble_->Host().options.colliders = s.colliders;
    bubble_->Host().options.mutableColliders = s.mutableColliders;
    FlightScene::hooks.beforeFlightIntegrator = s.beforeFlightIntegrator;
    FlightScene::hooks.beforeVesselUpdate = s.beforeVesselUpdate;
    bubble_->physicsStep = s.physicsStep;
    installed_ = false;
}

VesselWheels* WheelPhysicsHost::Find(const Vessel& v) const {
    for (const auto& vw : vessels_)
        if (vw->vessel_ == &v) return vw.get();
    return nullptr;
}

// The vessels with wheel parts among the loaded ones (FlightGlobals.Vessels order), their wheel parts in vessel part order. A wheel part keeps
// its record (and the Unity objects its modules point at) when it moves to another vessel (decouple, dock); records of parts that no loaded
// vessel holds go once the Part is destroyed (Part.allParts).
void WheelPhysicsHost::Refresh() {
    // every record, keyed by its part (a part keeps its record when it changes vessel)
    std::map<const Part*, std::unique_ptr<WheelPart>> pool;
    std::map<const Vessel*, std::unique_ptr<VesselWheels>> oldVessels;
    for (auto& vw : vessels_) {
        for (auto& r : vw->wheels_)
            if (r) pool[r->part] = std::move(r);
        vw->wheels_.clear();
        oldVessels[vw->vessel_] = std::move(vw);
    }
    for (auto& r : dormant_)
        if (r) pool[r->part] = std::move(r);
    dormant_.clear();
    vessels_.clear();
    for (Vessel* v : FlightGlobals::Vessels) {
        if (v == nullptr || !v->loaded || v->state == Vessel::State::DEAD) continue;
        std::vector<Part*> wheelParts;
        for (Part* p : v->parts)
            if (p != nullptr && IsWheelPart(*p)) wheelParts.push_back(p);
        if (wheelParts.empty()) continue;
        std::unique_ptr<VesselWheels> vw;
        bool changed = false;
        if (auto it = oldVessels.find(v); it != oldVessels.end() && it->second) {
            vw = std::move(it->second);
            changed = vw->lastParts_ != wheelParts;
        } else {
            vw = std::make_unique<VesselWheels>(*this, *v);
            changed = true;
        }
        for (Part* p : wheelParts) {
            std::unique_ptr<WheelPart> rec;
            if (auto it = pool.find(p); it != pool.end()) rec = std::move(it->second);
            if (!rec) {
                rec = std::make_unique<WheelPart>();
                rec->part = p;
                rec->base = WheelBaseOf(*p);
                for (size_t k = 0; k < p->Modules().Count(); ++k) {
                    PartModule* m = p->Modules()[k];
                    if (m == rec->base || dynamic_cast<ModuleWheelSubmodule*>(m) != nullptr) rec->modules.push_back(m);
                }
                // part.Rigidbody / its transform: the Unity objects wheelSetup and the controller hold (ModuleWheelBase.cs:279-287)
                rec->base->partRigidbody = &rec->rb;
                rec->base->partRigidbodyTransform = &rec->rbTransform;
                rec->rig = RigFor(*p);  // WP-29d
                if (rec->rig) rec->colliderNode = rec->rig->Find(rec->base->Get<std::string>("wheelColliderTransformName"));
            }
            vw->wheels_.push_back(std::move(rec));
        }
        vw->lastParts_ = wheelParts;
        // Part.Pack / Part.Unpack (Part.cs:3747 / :3784) of the wheel parts: `packed` and their OnPartPack / OnPartUnpack handlers (the
        // onPartPack / onPartUnpack events). Since WP-20g Vessel.GoOnRails / GoOffRails run Part.Pack / Unpack for every part, so this
        // mirror finds the flags equal and does nothing; it stays for a vessel a host made unpacked without Vessel.GoOffRails (the loader
        // leaves Part.packed set and ModuleWheelBase's set-up waits for an unpacked part, ModuleWheelBase.cs:226).
        for (auto& w : vw->wheels_) {
            if (w->part->fields.packed == v->packed) continue;
            w->part->fields.packed = v->packed;
            if (!w->started) continue;
            if (v->packed) w->base->OnPartPack(w->part);
            else w->base->OnPartUnpack(w->part);
        }
        if (!vw->balancer_) vw->balancer_ = std::make_unique<SuspensionLoadBalancer>(v);
        if (changed && vw->balancerStarted_) vw->balancer_->VesselChanged(v);  // SuspensionLoadBalancer.cs:66 (onVesselWasModified)
        vessels_.push_back(std::move(vw));
    }
    // records no loaded vessel holds stay (dormant: the modules point at them) until their Part is destroyed (Part.allParts)
    for (auto& kv : pool) {
        if (!kv.second) continue;
        const bool alive = std::find(Part::allParts.begin(), Part::allParts.end(), kv.first) != Part::allParts.end();
        if (alive) dormant_.push_back(std::move(kv.second));
    }
    // other vessels' parts the wheels stood on: drop the destroyed ones (and the WheelHits that still name their collider)
    for (auto it = others_.begin(); it != others_.end();) {
        const bool alive = std::find(Part::allParts.begin(), Part::allParts.end(), it->first) != Part::allParts.end();
        if (alive) {
            ++it;
            continue;
        }
        const Collider* gone = &it->second->collider;
        auto forget = [gone](WheelPart& w) {
            if (KSPWheelController* c = w.Controller(); c != nullptr && c->wheelCollider != nullptr) {
                NativeWheelCollider& wc = c->wheelCollider->wheelColliderData;
                if (wc.groundHit.collider == gone) ResetNativeWheel(wc);
                if (VehicleBase::WheelState* st = c->currentState(); st != nullptr && st->hit.collider == gone) {
                    st->hit = WheelHit();
                    st->grounded = false;
                }
            }
        };
        for (auto& vw : vessels_)
            for (auto& w : vw->wheels_) forget(*w);
        for (auto& w : dormant_) forget(*w);
        it = others_.erase(it);
    }
}

// Part.Start -> PartModule.OnStart of the wheel modules (Part.ModulesOnStart, Part.cs:1663 ff.: in module order), once the part has started
// (Part::fields.started: WP-20d's StartPartsInFlight). ModuleWheelBase.OnStart starts the wheelSetup coroutine (it waits for an unpacked part).
void WheelPhysicsHost::StartWheelModules(VesselWheels& vw) {
    Vessel& v = *vw.vessel_;
    for (auto& w : vw.wheels_) {
        if (w->started || !options.startModules) continue;
        if (!w->part->fields.started) continue;
        WheelPartGeometry og;
        if (w->rig && options.geometry && options.geometry(*w->part, *w->base, og)) {  // a host-supplied geometry overrides the model (WP-29c tests)
            w->rig.reset();
            w->colliderNode = -1;
        }
        if (w->rig && w->colliderNode >= 0) {
            const mu::RigPose cp = w->rig->PartSpace(w->colliderNode);
            w->geometry.position = cp.position;
            w->geometry.rotation = cp.rotation;
        } else {
            w->geometry = GeometryOf(*w->part, *w->base);
        }
        PoseFromPart(w->part->partTransform, w->geometry, w->colliderHost);
        w->colliderSet = w->colliderHost;
        const StartState state = v.LandedOrSplashed() ? StartState::Landed : StartState::Flying;
        for (PartModule* m : w->modules) m->OnStart(state);
        SampleAnimations(*w);  // the animation states OnStart posed apply to the model transforms (deployTgt of a deployed gear)
        // Part.Start (Part.cs:1915-1923): overrideSkillUpdate for a part with one of overrideSkillUpdateModules (ModuleSAS, ModuleWheelSteering):
        // Part.Update then refreshes its PartValues every frame (ModuleWheelSteering's SteeringRadius / CoMfwdLength, Part.cs:4328)
        const std::string names = w->part->fields.overrideSkillUpdateModules.value_or(std::string());
        for (const std::string& n : ParseExtensions::ParseArray(names))
            if (w->part->Modules().Contains(n)) w->part->fields.overrideSkillUpdate = true;
        w->started = true;
    }
    if (!vw.balancerStarted_ && vw.balancer_) {
        vw.balancer_->OnStart();  // SuspensionLoadBalancer.cs:24
        vw.balancerStarted_ = true;
    }
}

WheelPartGeometry WheelPhysicsHost::GeometryOf(const Part& part, const ModuleWheelBase& base) const {
    WheelPartGeometry g;
    if (options.geometry && options.geometry(part, base, g)) return g;
    const Vessel* v = part.vessel;
    Vector3d up(0.0, 1.0, 0.0);
    if (options.proxyUp && v != nullptr) {
        up = Vector3d(options.proxyUp(*v));
    } else if (v != nullptr && v->upAxis.sqrMagnitude() > 0.5) {
        up = QuaternionD::Inverse(QuaternionD(v->vesselTransform.rotation)) * v->upAxis;
    }
    up = up.normalized();
    Vector3d fwd(0.0, 1.0, 0.0);  // the root part's up (the vessel frame is the root part's)
    if (options.proxyForward && v != nullptr) fwd = Vector3d(options.proxyForward(*v));
    fwd = fwd - up * Vector3d::Dot(fwd, up);
    if (fwd.magnitude() < 0.5) {
        fwd = Vector3d(0.0, 0.0, 1.0) - up * up.z;
        if (fwd.magnitude() < 0.5) fwd = Vector3d(1.0, 0.0, 0.0) - up * up.x;
    }
    return ProxyGeometry(part, base, up.ToVector3f(), fwd.normalized().ToVector3f());
}

// The spike's geometry proxy (core/spikes/wheels/src/rover.cpp BuildWheels): the tyre bottom at full extension is the bottom face of the
// part's drag cube ("Deployed" when the part has one), the suspension along `upVessel`, the rolling direction `forwardVessel` (vessel frame).
// The wheel collider values are those wheelSetup and the suspension module give the collider: radius and center x rescaleFactor
// (ModuleWheelBase.cs:333-334), suspensionDistance x rescaleFactor and the anchor = targetPosition (ModuleWheelSuspension OnWheelSetup), else
// wheelSetup's 0.02 / 0.5 (ModuleWheelBase.cs:306-307); VPWheelCollider puts the Unity centre at center + up D (1 - anchor) (VPWheelCollider.cs:421).
WheelPartGeometry WheelPhysicsHost::ProxyGeometry(const Part& part, const ModuleWheelBase& base, const Vector3f& upVessel, const Vector3f& forwardVessel) {
    const double rescale = part.fields.rescaleFactor;
    const double r = static_cast<double>(base.Get<float>("radius")) * rescale;
    const Vector3d center = Vector3d(base.Get<Vector3f>("center")) * rescale;
    double D = 0.02, anchor = 0.5;
    if (PartModule* s = const_cast<Part&>(part).FindModuleOfClass("ModuleWheelSuspension")) {
        D = static_cast<double>(s->Get<float>("suspensionDistance")) * rescale;
        anchor = s->Get<float>("targetPosition");
    }
    // the drag cube of the deployed pose
    Vector3d boxC, boxH(0.25, 0.25, 0.25);
    bool found = false;
    if (!part.DragCubes.None())
        for (const DragCube& dc : part.DragCubes.Cubes())
            if (dc.Name() == "Deployed") {
                boxC = Vector3d(dc.Center());
                boxH = Vector3d(std::max(dc.Size().x * 0.5, 0.05), std::max(dc.Size().y * 0.5, 0.05), std::max(dc.Size().z * 0.5, 0.05));
                found = true;
            }
    if (!found) {
        const ColliderProxy c = DragCubeColliderProxy(part).front();
        boxC = Vector3d(c.center);
        boxH = Vector3d(c.halfExtents);
    }
    const Vector3d up = Vector3d(upVessel).normalized();
    const Vector3d fwd = Vector3d(forwardVessel).normalized();
    const Vector3d right = Vector3d::Cross(up, fwd);
    const Vector3d orgPos(part.fields.orgPos);
    const QuaternionD orgRot(part.fields.orgRot);
    Vector3d corners[8];
    double minY = 1e30;
    for (int k = 0; k < 8; ++k) {
        const Vector3d c = boxC + Vector3d((k & 1) ? boxH.x : -boxH.x, (k & 2) ? boxH.y : -boxH.y, (k & 4) ? boxH.z : -boxH.z);
        corners[k] = orgPos + orgRot * c;
        minY = std::min(minY, Vector3d::Dot(corners[k], up));
    }
    std::sort(corners, corners + 8, [&](const Vector3d& a, const Vector3d& b) { return Vector3d::Dot(a, up) < Vector3d::Dot(b, up); });
    Vector3d bottom = (corners[0] + corners[1] + corners[2] + corners[3]) * 0.25;
    bottom = bottom + up * (minY - Vector3d::Dot(bottom, up));
    const Vector3d top = bottom + up * (D + r);  // the wheel centre at full compression
    const Vector3d cl = center + Vector3d(0.0, D * (1.0 - anchor), 0.0);
    const Vector3d transformPos = top - (right * cl.x + up * cl.y + fwd * cl.z);
    const QuaternionD rotVessel = Basis(right, up, fwd);
    const QuaternionD inv = QuaternionD::Inverse(orgRot);
    WheelPartGeometry g;
    g.position = (inv * (transformPos - orgPos)).ToVector3f();
    g.rotation = (inv * rotVessel).ToQuaternionf();
    return g;
}

NativeTransform* WheelPhysicsHost::FindModelTransform(Part& p, const std::string& name) {
    for (auto& vw : vessels_)
        for (auto& w : vw->wheels_)
            if (w->part == &p) {
                if (w->rig) {  // Part.FindModelTransform (Part.cs:5079) on the part model hierarchy
                    const int idx = w->rig->Find(name);
                    if (idx < 0) return nullptr;
                    if (idx == w->colliderNode) return &w->colliderHost;
                    auto& slot = w->rigTransforms[idx];
                    if (!slot) {
                        slot = std::make_unique<NativeTransform>();
                        PoseOfRig(p.partTransform, w->rig->PartSpace(idx), *slot);
                    }
                    return slot.get();
                }
                if (name == w->base->Get<std::string>("wheelColliderTransformName")) return &w->colliderHost;
                auto& slot = w->named[name];
                if (!slot) {
                    slot = std::make_unique<NativeTransform>();
                    *slot = w->colliderHost;
                }
                return slot.get();
            }
    return nullptr;
}

// ---- WP-29d: the model data ---------------------------------------------------------------------------------------------------------------
std::shared_ptr<mu::PartModelRig> WheelPhysicsHost::RigFor(const Part& part) const {
    if (options.modelRig) return options.modelRig(part);
    if (options.models) return options.models->BuildRig(WheelModelLibrary::NameOf(part), static_cast<float>(part.fields.rescaleFactor));
    return nullptr;
}

// Animation.Sample: every enabled state with weight > 0 applies its clip at the state time (WrapMode.ClampForever: clamped to the clip)
void WheelPhysicsHost::SampleAnimations(WheelPart& w) {
    if (!w.rig) return;
    for (WheelPart::AnimBinding& b : w.animations) {
        const mu::RigAnimation& ra = w.rig->Animations()[static_cast<size_t>(b.rigAnimation)];
        for (auto& st : b.anim->States()) {
            if (!st->enabled || !(st->weight > 0.0f)) continue;
            const int clip = ra.FindClip(st->name);
            if (clip < 0) continue;
            float time = st->time;
            if (time < 0.0f) time = 0.0f;
            if (time > st->length) time = st->length;
            w.rig->SampleClip(b.rigAnimation, clip, time);
        }
    }
}

void WheelPhysicsHost::TickAnimations(float h) {
    for (auto& vw : vessels_) {
        if (vw->vessel_->packed) continue;
        for (auto& w : vw->wheels_) {
            if (!w->rig || w->animations.empty()) continue;
            for (WheelPart::AnimBinding& b : w->animations) b.anim->Tick(h);
            SampleAnimations(*w);
        }
    }
}

// Part.FindModelAnimator(transformName, clipName): the Animation component of the model transform holding the clip
Animation* WheelPhysicsHost::FindAnimator(Part& p, const std::string& trf, const std::string& state) {
    for (auto& vw : vessels_)
        for (auto& w : vw->wheels_)
            if (w->part == &p) {
                if (!w->rig) return nullptr;
                const int ai = w->rig->FindAnimator(trf, state);
                if (ai < 0) return nullptr;
                for (WheelPart::AnimBinding& b : w->animations)
                    if (b.rigAnimation == ai) return b.anim.get();
                WheelPart::AnimBinding b;
                b.rigAnimation = ai;
                b.anim = std::make_unique<Animation>();
                for (const mu::AnimClip& cl : w->rig->Animations()[static_cast<size_t>(ai)].chunk->clips)
                    b.anim->AddClip(cl.name, cl.Length(), cl.wrapMode == 0 ? WrapMode::Default : static_cast<WrapMode>(cl.wrapMode));
                w->animations.push_back(std::move(b));
                return w->animations.back().anim.get();
            }
    return nullptr;
}

void WheelPhysicsHost::SampleAnimation(Animation& a) {
    for (auto& vw : vessels_)
        for (auto& w : vw->wheels_)
            for (WheelPart::AnimBinding& b : w->animations)
                if (b.anim.get() == &a) {
                    SampleAnimations(*w);
                    return;
                }
}

// Transform.SetParent(part) keeps the world pose (ModuleWheelBase.cs:241): from then the collider transform rides on the part; a later
// KSPWheelController.SetWheelTransform (ModuleWheelDeployment) moves it in world space, which is a new part-space pose.
void WheelPhysicsHost::WriteBackGeometry(WheelPart& w) {
    if (!w.rig || w.colliderNode < 0 || !w.base->IsWheelSetUp()) return;
    if (!SamePose(w.colliderHost, w.colliderSet)) {
        NativeTransform part;
        part.position = w.part->partTransform.position;
        part.rotation = w.part->partTransform.rotation;
        w.geometry.position = part.InverseTransformPoint(w.colliderHost.position);
        w.geometry.rotation = QuaternionExtensions::Inverse(w.part->partTransform.rotation) * w.colliderHost.rotation;
        w.colliderSet = w.colliderHost;
    }
    w.geometryLocked = true;
}

// The model rig posed at the part current ModuleWheelDeployment state (stateString "Retracted" -> 1 - deployedPosition, "Deployed" or none ->
// deployedPosition, else position); null without model data
std::shared_ptr<mu::PartModelRig> WheelPhysicsHost::SampledRig(const Part& part) const {
    std::shared_ptr<mu::PartModelRig> rig = RigFor(part);
    if (!rig) return nullptr;
    if (PartModule* m = const_cast<Part&>(part).FindModuleOfClass("ModuleWheelDeployment")) {
        std::string trf, clip, st;
        float deployed = 1.0f, pos = 0.0f;
        try {
            trf = m->Get<std::string>("animationTrfName");
            clip = m->Get<std::string>("animationStateName");
            st = m->Get<std::string>("stateString");
            deployed = m->Get<float>("deployedPosition");
        } catch (const std::exception&) {
        }
        if (auto* d = dynamic_cast<ModuleWheelDeployment*>(m)) pos = d->position;
        const int ai = rig->FindAnimator(trf, clip);
        if (ai >= 0) {
            float n = deployed;
            if (st == "Retracted") n = 1.0f - deployed;
            else if (!st.empty() && st != "Deployed") n = pos;
            const int ci = rig->Animations()[static_cast<size_t>(ai)].FindClip(clip);
            rig->SampleClip(ai, ci, n * rig->Animations()[static_cast<size_t>(ai)].Length(clip));
        }
    }
    return rig;
}

WheelPart* WheelPhysicsHost::RecordOf(const Part& part) const {
    for (const auto& vw : vessels_)
        for (const auto& w : vw->wheels_)
            if (w->part == &part) return w.get();
    for (const auto& w : dormant_)
        if (w && w->part == &part) return w.get();
    return nullptr;
}

namespace {
// The node paths of a model whose colliders are not in the compound: a disabled or inactive GameObject takes its whole subtree with it
std::string RigPath(const mu::PartModelRig& rig, int node) {
    std::string path;
    for (int i = node; i >= 0; i = rig.At(i).parent) path = path.empty() ? rig.At(i).name : rig.At(i).name + "/" + path;
    return path;
}
bool UnderPath(const std::string& path, const std::string& root) { return path == root || path.compare(0, root.size() + 1, root + "/") == 0; }
std::string ModuleField(const Part& part, const char* module, const char* name) {
    std::string v;
    if (PartModule* m = const_cast<Part&>(part).FindModuleOfClass(module)) {
        try {
            v = m->Get<std::string>(name);
        } catch (const std::exception&) {
        }
    }
    return v;
}
}  // namespace

// The hub colliders of a wheel part: the model's colliders at the pose the model has now. The compound of a cluster follows them (WP-29e,
// UpdateColliders): the animation (ModuleWheelDeployment's clip moves the retract box), and the transforms VPWheelCollider.UpdateVisualWheel
// moves every frame (the suspension, the caliper / steering pivot, the wheel: VPWheelCollider.cs:275-300), with everything below them.
// Not colliders of the compound: the one ModuleWheelSuspension.OnStart disables (suspensionColliderName, ModuleWheelSuspension.cs:215), the
// subtree of the damaged / undamaged wheel transform that ModuleWheelDamage.SetDamaged has inactive (cs:370-402), and the wheel transform's
// subtree while VPWheelCollider hides the wheel (the wheel is off: hideWheelOnDisable, cs:245-253). Once the part's modules have started the
// live rig is used (the host's animation and visual writes); before, the rig posed at the part's ModuleWheelDeployment state.
std::vector<ColliderProxy> WheelPhysicsHost::HubColliders(const Part& part) {
    const WheelPart* rec = RecordOf(part);
    std::shared_ptr<mu::PartModelRig> rig = (rec != nullptr && rec->started && rec->rig) ? rec->rig : SampledRig(part);
    if (!rig) return {};
    std::vector<std::string> skipped;  // node paths whose subtree is left out
    auto skip = [&](const std::string& n) {
        if (n.empty()) return;
        const int i = rig->Find(n);
        if (i >= 0) skipped.push_back(RigPath(*rig, i));
    };
    skip(ModuleField(part, "ModuleWheelSuspension", "suspensionColliderName"));
    if (PartModule* dmg = const_cast<Part&>(part).FindModuleOfClass("ModuleWheelDamage")) {
        const bool damaged = dmg->Get<bool>("isDamaged");
        skip(ModuleField(part, "ModuleWheelDamage", damaged ? "undamagedTransformName" : "damagedTransformName"));
    }
    if (rec != nullptr && rec->wheelHidden) skip(ModuleField(part, "ModuleWheelBase", "wheelTransformName"));
    std::vector<mu::PartColliderShape> shapes = rig->Colliders();
    shapes.erase(std::remove_if(shapes.begin(), shapes.end(),
                                [&](const mu::PartColliderShape& s) {
                                    for (const std::string& sk : skipped)
                                        if (UnderPath(s.path, sk)) return true;
                                    return false;
                                }),
                 shapes.end());
    std::vector<std::string> moving;  // the subtrees VPWheelCollider.UpdateVisualWheel moves
    for (const char* f : {"suspensionTransformName", "caliperTransformName", "wheelTransformName"}) {
        const std::string name = ModuleField(part, f[0] == 's' ? "ModuleWheelSuspension" : (f[0] == 'c' ? "ModuleWheelSteering" : "ModuleWheelBase"), f);
        const int node = name.empty() ? -1 : rig->Find(name);
        if (node >= 0) moving.push_back(RigPath(*rig, node));
    }
    std::vector<ColliderProxy> out;
    for (const mu::PartColliderShape& s : shapes) {  // one proxy per shape (ProxiesFromShapes skips triggers and degenerate meshes)
        std::vector<ColliderProxy> one = ProxiesFromShapes({s});
        bool follows = false;
        for (const std::string& m : moving) follows = follows || UnderPath(s.path, m);
        for (ColliderProxy& p : one) {
            p.noInertia = follows;
            out.push_back(std::move(p));
        }
    }
    if (const std::optional<ColliderProxy> si = StandInProxy(part)) out.push_back(*si);
    return out;
}

// Does the cluster holding this wheel part need a mutable compound: colliders under a transform that animates (the part has an Animation) or
// that VPWheelCollider.UpdateVisualWheel moves (suspension / caliper / wheel transform)
bool WheelPhysicsHost::HasMovingColliders(const Part& part) const {
    if (!options.modelColliders) return false;
    std::shared_ptr<mu::PartModelRig> rig = RigFor(part);
    if (!rig) return false;
    const std::vector<mu::PartColliderShape> shapes = rig->Colliders();
    if (shapes.empty()) return false;
    if (!rig->Animations().empty()) return true;
    for (const char* f : {"suspensionTransformName", "caliperTransformName", "wheelTransformName"}) {
        const char* module = f[0] == 's' ? "ModuleWheelSuspension" : (f[0] == 'c' ? "ModuleWheelSteering" : "ModuleWheelBase");
        const std::string name = ModuleField(part, module, f);
        if (name.empty()) continue;
        const int node = rig->Find(name);
        if (node < 0) continue;
        const std::string root = RigPath(*rig, node);
        for (const mu::PartColliderShape& s : shapes)
            if (UnderPath(s.path, root)) return true;
    }
    return false;
}

// ModuleWheelBase.toggleRbConstraints (ModuleWheelBase.cs:785): the wheel part Rigidbody is FreezeAll for schloompaTime. The reduced model
// has one body per rigid cluster with the wheel part welded into it: a frozen wheel part holds its cluster, a weld to the world at the
// cluster centre of mass (as WP-20e RB anchor), made after the cluster stopped (the freeze only starts below 1e-5 m/s srfSpeed)
void WheelPhysicsHost::ApplyFreezes() {
    VesselPhysicsHost& host = bubble_->Host();
    for (auto& vw : vessels_) {
        VesselPhysics* vp = host.Find(*vw->vessel_);
        if (vp == nullptr) continue;
        int cluster = -1;
        for (auto& w : vw->wheels_)
            if (frozen_.count(w->part) != 0) {
                cluster = vp->ClusterOf(*w->part);
                break;
            }
        auto it = freezes_.find(vp);
        if (it != freezes_.end() && (cluster < 0 || vp->ClusterBody(static_cast<size_t>(cluster)) != it->second.body)) {
            bubble_->World().RemoveConstraint(it->second.id);  // unknown ids are ignored (a rebuilt cluster took its constraints with it)
            freezes_.erase(it);
            it = freezes_.end();
        }
        if (cluster >= 0 && it == freezes_.end()) {
            const BodyId body = vp->ClusterBody(static_cast<size_t>(cluster));
            if (body == kInvalidBody) continue;
            vp->StopMotion();
            Freeze f;
            f.body = body;
            f.id = bubble_->World().CreateFixedToWorldConstraint(body, vp->ClusterCenterOfMass(static_cast<size_t>(cluster)));
            if (f.id != kInvalidConstraint) freezes_[vp] = f;
        }
    }
    // a binding that is gone drops its weld record (the world removed the constraint with the body)
    for (auto it = freezes_.begin(); it != freezes_.end();) {
        bool alive = false;
        for (const auto& b : host.Bindings()) alive = alive || b.get() == it->first;
        it = alive ? std::next(it) : freezes_.erase(it);
    }
}

// ModuleWheelDeployment.CreateStandInCollider (ModuleWheelDeployment.cs:394): while the wheel is retracted or deploying (Position < TsubSys) a
// SphereCollider of the wheel collider radius sits at deployTgt.position under the part model; the module keeps it on deployTgt
std::optional<ColliderProxy> WheelPhysicsHost::StandInProxy(const Part& part) const {
    auto* d = dynamic_cast<ModuleWheelDeployment*>(const_cast<Part&>(part).FindModuleOfClass("ModuleWheelDeployment"));
    if (d == nullptr || !d->StandInColliderActive() || d->DeployTarget() == nullptr) return std::nullopt;
    NativeTransform pt;
    pt.position = part.partTransform.position;
    pt.rotation = part.partTransform.rotation;
    ColliderProxy c;
    c.kind = ShapeKind::Sphere;
    c.center = pt.InverseTransformPoint(d->DeployTarget()->position);
    ModuleWheelBase* b = WheelBaseOf(part);
    c.radius = b != nullptr ? b->Get<float>("radius") * static_cast<float>(part.fields.rescaleFactor) : 0.1f;  // w.wheelCollider.radius
    return c;
}

// WP-29e. The colliders of the wheel parts follow their models (the animation, the visual suspension / caliper / wheel, the stand-in sphere on
// deployTgt) in the cluster compounds: in place when only poses changed, a rebuild (VesselPhysics::Rebuild keeps the momentum) when a collider
// appeared or went (the stand-in is created / destroyed with the wheel's state, the wheel hidden, a wheel damaged).
void WheelPhysicsHost::UpdateColliders() {
    if (!options.modelColliders) return;
    for (auto& vw : vessels_) {
        if (vw->vessel_->packed) continue;
        VesselPhysics* vp = bubble_->Host().Find(*vw->vessel_);
        if (vp == nullptr) continue;
        bool rebuild = false;
        for (auto& w : vw->wheels_) {
            if (!w->rig) continue;
            const std::optional<ColliderProxy> si = StandInProxy(*w->part);
            const std::vector<ColliderProxy> hub = HubColliders(*w->part);
            if (hub.empty()) continue;
            if (vp->SetPartColliderPoses(*w->part, hub) == VesselPhysics::ColliderUpdate::Mismatch) rebuild = true;
            w->standInBuilt = si.has_value();
            if (si) w->standInCenter = si->center;
        }
        if (rebuild) {
            ++colliderRebuilds_;
            vp->Rebuild();
        }
    }
}

// Vessel.getLowestPoint scan of a wheel part (Vessel.cs:2589-2637): every non-trigger collider of the part on a layer other than 21 / 16 / 20,
// the ones on the wheel layers 26 / 27 / 30 enabled for the measure (so the colliders ModuleWheelSuspension disabled count), at the pose the
// vessel has now. ClosestPoint of a far point below is the support point in -z; for the layer 26 / 27 / 30 ones Unity reads bounds.min.z, the
// same for a convex shape. A WheelCollider component bounds is taken as its centre (UNVERIFIED: the C# subtracts the radius afterwards, which
// is only right for a point), its radius the prefab chunk radius.
std::vector<VesselEnvironment::ColliderPoint> WheelPhysicsHost::PlacementPoints(Part& part, const Vector3f& measure) {
    (void)measure;
    std::vector<VesselEnvironment::ColliderPoint> out;
    std::shared_ptr<mu::PartModelRig> rig = SampledRig(part);
    if (!rig) return out;
    NativeTransform pt;
    pt.position = part.partTransform.position;
    pt.rotation = part.partTransform.rotation;
    auto worldZ = [&](const Vector3f& partSpace) { return pt.TransformPoint(partSpace).z; };
    for (const mu::PartColliderShape& s : rig->Colliders()) {
        if (s.trigger || s.layer == 21 || s.layer == 16 || s.layer == 20) continue;
        float z = std::numeric_limits<float>::infinity();
        switch (s.kind) {
            case mu::ColliderKind::Box:
                for (int k = 0; k < 8; ++k) {
                    const Vector3f c = mu::RotateNative(s.nodeRotation, Vector3f((k & 1) ? s.halfExtents.x : -s.halfExtents.x, (k & 2) ? s.halfExtents.y : -s.halfExtents.y,
                                                                              (k & 4) ? s.halfExtents.z : -s.halfExtents.z));
                    z = std::min(z, worldZ(s.center + c));
                }
                break;
            case mu::ColliderKind::Sphere:
                z = worldZ(s.center) - s.radius;
                break;
            case mu::ColliderKind::Capsule: {
                const Vector3f axis = mu::RotateNative(s.nodeRotation, s.axis == 0 ? Vector3f(1.0f, 0.0f, 0.0f) : s.axis == 2 ? Vector3f(0.0f, 0.0f, 1.0f) : Vector3f(0.0f, 1.0f, 0.0f));
                z = std::min(worldZ(s.center + axis * s.halfCylinder), worldZ(s.center - axis * s.halfCylinder)) - s.radius;
                break;
            }
            case mu::ColliderKind::Mesh:
                for (const Vector3f& v : s.vertices) z = std::min(z, worldZ(v));
                break;
            default:
                continue;
        }
        VesselEnvironment::ColliderPoint cp;
        cp.z = z;
        const size_t slash = s.path.rfind('/');
        const std::string leaf = slash == std::string::npos ? s.path : s.path.substr(slash + 1);
        if (leaf.find("collisionEnhancer") != std::string::npos) cp.role = VesselEnvironment::ColliderRole::CollisionEnhancer;
        out.push_back(cp);
    }
    // the WheelCollider components (layer 27, enabled for the measure): the prefab chunk centre and radius
    std::function<void(int)> walk = [&](int i) {
        const mu::RigNode& n = rig->At(i);
        if (n.src != nullptr)
            for (const mu::ColliderChunk& c : n.src->colliders) {
                if (c.kind != mu::ColliderKind::Wheel) continue;
                const mu::RigPose ps = rig->PartSpace(i);
                const Vector3f local(c.wheel.center.x * ps.scale.x, c.wheel.center.y * ps.scale.y, c.wheel.center.z * ps.scale.z);
                VesselEnvironment::ColliderPoint cp;
                cp.z = worldZ(ps.position + mu::RotateNative(ps.rotation, local));
                cp.role = VesselEnvironment::ColliderRole::WheelCollider;
                cp.radius = c.wheel.radius;
                out.push_back(cp);
            }
        for (int ch : n.children) walk(ch);
    };
    walk(0);
    return out;
}

// Unity's state the managed code reads: part.Rigidbody (the cluster's motion at the wheel part, transform-frame floats), its transform, the
// wheel collider transform; WheelCollider.sprungMass = rb.mass (one WheelCollider on the Rigidbody, Unity divides the mass over them).
void WheelPhysicsHost::SyncNative(VesselWheels& vw, VesselPhysics& vp) {
    for (auto& w : vw.wheels_) {
        const PartRigidbodyState s = vp.RigidbodyState(*w->part);
        w->rb.mass = s.mass;
        w->rb.velocity = s.velocity;
        w->rb.angularVelocity = s.angularVelocity;
        w->rb.worldCenterOfMass = s.worldCenterOfMass;
        w->rb.centerOfMass = w->part->fields.CoMOffset;
        w->rb.isKinematic = false;
        w->rbTransform.position = w->part->partTransform.position;
        w->rbTransform.rotation = w->part->partTransform.rotation;
        if (w->rig && w->colliderNode >= 0) {
            if (!w->geometryLocked) {  // before wheelSetup the collider transform is a node of the (animated) model
                const mu::RigPose cp = w->rig->PartSpace(w->colliderNode);
                w->geometry.position = cp.position;
                w->geometry.rotation = cp.rotation;
            }
            PoseFromPart(w->part->partTransform, w->geometry, w->colliderHost);
            w->colliderSet = w->colliderHost;
            for (auto& kv : w->rigTransforms) PoseOfRig(w->part->partTransform, w->rig->PartSpace(kv.first), *kv.second);
        } else {
            PoseFromPart(w->part->partTransform, w->geometry, w->colliderHost);
            for (auto& kv : w->named) *kv.second = w->colliderHost;
        }
        if (KSPWheelController* c = w->Controller(); c != nullptr && c->wheelCollider != nullptr) c->wheelCollider->wheelColliderData.sprungMass = w->rb.mass;
    }
}

// One FixedUpdate of the managed wheel code of a vessel, in the assumed script order (WHEEL-SPIKE.md 1.4, R5): the SuspensionLoadBalancer
// (VesselModule), then per wheel part its modules in module order and its KSPWheelController (VehicleBase.FixedUpdate, VehicleBase.cs:934);
// Unity calls FixedUpdate only on enabled components.
void WheelPhysicsHost::ManagedTick(VesselWheels& vw, VesselPhysics& vp) {
    SyncNative(vw, vp);
    for (auto& w : vw.wheels_) w->rb.forces.clear();
    for (auto& kv : others_) kv.second->rb.forces.clear();
    if (vw.balancer_ && vw.balancerStarted_) vw.balancer_->FixedUpdate();  // SuspensionLoadBalancer.cs:31
    for (auto& w : vw.wheels_) {
        if (!w->started) continue;
        for (PartModule* m : w->modules) {
            if (auto* b = dynamic_cast<ModuleWheelBase*>(m)) b->FixedUpdate();                  // ModuleWheelBase.cs:421
            else if (auto* mo = dynamic_cast<ModuleWheelMotor*>(m)) mo->FixedUpdate();          // ModuleWheelMotor.cs:131
            else if (auto* br = dynamic_cast<ModuleWheelBrakes*>(m)) br->FixedUpdate();         // ModuleWheelBrakes.cs:88
            else if (auto* su = dynamic_cast<ModuleWheelSuspension*>(m)) su->FixedUpdate();     // ModuleWheelSuspension.cs:315
            else if (auto* st = dynamic_cast<ModuleWheelSteering*>(m)) st->FixedUpdate();       // ModuleWheelSteering.cs:247
            else if (auto* da = dynamic_cast<ModuleWheelDamage*>(m)) da->FixedUpdate();         // ModuleWheelDamage.cs:289
            else if (auto* de = dynamic_cast<ModuleWheelDeployment*>(m)) de->FixedUpdate();     // ModuleWheelDeployment.cs:685
        }
        KSPWheelController* c = w->Controller();
        if (c != nullptr && c->enabled() && c->initialized()) c->FixedUpdate();
        WriteBackGeometry(*w);
    }
    CollectManagedForces(vw, vp);
}

// The Rigidbody.AddForceAtPosition calls of the managed tick (VPWheelCollider.ApplyForce, VPWheelCollider.cs:371: the tyre force on the
// wheel part, its reaction on another vessel's part) in the physics frame, applied in the physics step (a FloatingOrigin shift between the
// two moves the transform frame, not the physics frame)
void WheelPhysicsHost::CollectManagedForces(VesselWheels& vw, VesselPhysics& vp) {
    for (auto& w : vw.wheels_) {
        for (const NativeRigidbody::ForceCall& f : w->rb.forces)
            pending_.push_back({vw.vessel_, w->part, Vector3d(f.force), vp.PhysicsFromTransform(Vector3d(f.position))});
        w->rb.forces.clear();
    }
    if (!options.reactOnOtherVessels) return;
    for (auto& kv : others_) {
        OtherPart& o = *kv.second;
        if (o.rb.forces.empty()) continue;
        VesselPhysics* ov = bubble_->Host().Find(*o.vessel);
        if (ov != nullptr)
            for (const NativeRigidbody::ForceCall& f : o.rb.forces)
                pending_.push_back({o.vessel, o.part, Vector3d(f.force), ov->PhysicsFromTransform(Vector3d(f.position))});
        o.rb.forces.clear();
    }
}

void WheelPhysicsHost::ApplyPendingForces() {
    VesselPhysicsHost& host = bubble_->Host();
    for (const PendingForce& f : pending_) {
        VesselPhysics* vp = host.Find(*f.vessel);
        if (vp != nullptr && vp->PartIndex(*f.part) >= 0) vp->AddForceAtPhysicsPoint(*f.part, f.force, f.point);
    }
    pending_.clear();
}

bool WheelPhysicsHost::AnyWheelSetUp() const {
    for (const auto& vw : vessels_) {
        const Vessel& v = *vw->vessel_;
        if (v.packed || bubble_->Host().Find(v) == nullptr) continue;
        if (vw->SetUpCount() > 0) return true;
    }
    return false;
}

int WheelPhysicsHost::SubstepCount(float dt) const {
    if (!options.substeps || !(options.substepDt > 0.0f) || !(dt > options.substepDt * 1.0001f)) return 1;
    if (!AnyWheelSetUp()) return 1;
    return std::max(1, static_cast<int>(std::ceil(static_cast<double>(dt) / static_cast<double>(options.substepDt) - 1e-4)));
}

void WheelPhysicsHost::FixedUpdateManaged() {
    Refresh();
    for (auto& vw : vessels_) StartWheelModules(*vw);
    const int n = SubstepCount(Time::fixedDeltaTime);
    const TimeScope ts(Time::fixedDeltaTime / static_cast<float>(n));
    for (auto& vw : vessels_) {
        Vessel& v = *vw->vessel_;
        if (v.packed) continue;
        if (VesselPhysics* vp = bubble_->Host().Find(v)) ManagedTick(*vw, *vp);
    }
}

void WheelPhysicsHost::ManagedSubstep(float h) {
    Refresh();
    const TimeScope ts(h);
    for (auto& vw : vessels_) {
        Vessel& v = *vw->vessel_;
        if (v.packed) continue;
        if (VesselPhysics* vp = bubble_->Host().Find(v)) ManagedTick(*vw, *vp);
    }
}

WheelPhysicsHost::OtherPart* WheelPhysicsHost::OtherPartFor(VesselPhysics& owner, uint32_t partIndex) {
    Vessel* ov = owner.VesselPtr();
    if (ov == nullptr || partIndex >= ov->parts.size()) return nullptr;
    Part* p = ov->parts[partIndex];
    if (p == nullptr || owner.PartIndex(*p) != static_cast<int>(partIndex)) return nullptr;
    auto& slot = others_[p];
    if (!slot) slot = std::make_unique<OtherPart>();
    OtherPart& o = *slot;
    o.part = p;
    o.vessel = ov;
    o.collider.attachedRigidbody = &o.rb;
    const PartRigidbodyState s = owner.RigidbodyState(*p);
    o.rb.mass = s.mass;
    o.rb.velocity = s.velocity;
    o.rb.angularVelocity = s.angularVelocity;
    o.rb.worldCenterOfMass = s.worldCenterOfMass;
    o.rb.isKinematic = false;
    return &o;
}

namespace {
struct LimitRow {
    int cluster;
    Part* part;
    Vector3d u, point;  // push direction (-w, up the travel) and anchor (the wheel centre at rest), physics frame
    double error;       // PhysX's geometric error (w . n)(maxCompression - dx) > 0
    double lambda = 0.0;
};
}  // namespace

void WheelPhysicsHost::NativeStepVessel(VesselWheels& vw, VesselPhysics& vp, float h) {
    PhysicsWorld& world = bubble_->World();
    VesselPhysicsHost& host = bubble_->Host();
    SyncNative(vw, vp);
    std::vector<BodyId> ignore;
    for (size_t c = 0; c < vp.ClusterCount(); ++c)
        if (vp.ClusterBody(c) != kInvalidBody) ignore.push_back(vp.ClusterBody(c));
    // the PQS / KSC colliders live in the body frame: out of reach while Krakensbane's frame drift is folded out (WP-19b)
    const uint32_t layers = world.StaticCollisions() ? PhysicsWorld::kLayerAll : PhysicsWorld::kLayerMoving;
    std::vector<LimitRow> rows;
    for (auto& w : vw.wheels_) {
        KSPWheelController* ctl = w->Controller();
        if (ctl == nullptr || ctl->wheelCollider == nullptr) continue;
        NativeWheelCollider& wc = ctl->wheelCollider->wheelColliderData;
        const bool on = wc.enabled && ctl->enabled() && ctl->wheelCollider->isActiveAndEnabled() && w->base->IsWheelSetUp();
        if (!on) {
            if (w->nativeOn) ResetNativeWheel(wc);
            w->nativeOn = false;
            w->last = NativeWheelStep();
            continue;
        }
        if (!w->nativeOn) {
            ResetNativeWheel(wc);  // a new PhysX wheel: setToRestState (PxVehicleSuspWheelTire4.h:193)
            w->nativeOn = true;
        }
        const NativeWheelGeometry g = ComputeNativeWheelGeometry(wc, w->colliderHost);
        // the suspension ray (PxVehicleUpdate.cpp:7255) against the world without this vessel's bodies
        const Vector3d origin = vp.PhysicsFromTransform(Vector3d(g.rayOrigin));
        const PhysicsWorld::RayHitShape hit = world.RayCastIgnoring(origin, Vector3d(g.rayDir), static_cast<double>(g.rayLength), layers, ignore);
        NativeGroundQuery q;
        OtherPart* other = nullptr;
        VesselPhysics* otherVp = nullptr;
        if (hit.hit) {
            q.hit = true;
            q.distance = static_cast<float>(hit.distance);
            q.point = vp.TransformFromPhysics(hit.point);
            q.normal = hit.normal.ToVector3f();
            q.collider = &terrain_;
            if (world.IsStatic(hit.body)) {
                // WP-12c: a registered scenery collider answers with its own layer / tag; the PQS quads and unregistered keys stay `terrain_`
                const StaticColliderInfo& si = bubble_->Contacts().StaticInfo(hit.userData);
                if (!si.isPQ && (si.tag != "Untagged" || si.layer != 15 || !si.crashObjectName.empty())) q.collider = StaticColliderFor(si);
            } else {
                for (const auto& b : host.Bindings()) {
                    if (b.get() == &vp) continue;
                    for (size_t c = 0; c < b->ClusterCount() && otherVp == nullptr; ++c)
                        if (b->ClusterBody(c) == hit.body) otherVp = b.get();
                    if (otherVp != nullptr) break;
                }
                if (otherVp != nullptr) other = OtherPartFor(*otherVp, hit.subShapeUserData);
                if (other != nullptr) {
                    q.collider = &other->collider;
                    q.rigidbodyActor = true;
                    q.kinematicActor = false;
                    q.actorMass = other->rb.mass;
                    q.actorVelocity = other->rb.velocity;
                    q.actorAngularVelocity = other->rb.angularVelocity;
                    q.actorCenterOfMass = other->rb.worldCenterOfMass;
                }
            }
        }
        NativeChassis chassis;
        chassis.centerOfMass = w->rb.worldCenterOfMass;
        chassis.velocity = w->rb.velocity;
        chassis.angularVelocity = w->rb.angularVelocity;
        const NativeWheelStep s = SimulateNativeWheel(wc, g, q, chassis, wc.physx, h);
        CommitNativeWheel(wc, s);
        w->last = s;
        w->lastGeometry = g;
        if (!s.grounded) continue;
        // the suspension (and PhysX tyre) force on the chassis at the force point (PxVehicleUpdate.cpp:3433-3441, :3597-3604)
        const Vector3d fp = vp.PhysicsFromTransform(Vector3d(s.forcePoint));
        vp.AddForceAtPhysicsPoint(*w->part, Vector3d(s.force), fp);
        if (s.tireForce.x != 0.0f || s.tireForce.y != 0.0f || s.tireForce.z != 0.0f) vp.AddForceAtPhysicsPoint(*w->part, Vector3d(s.tireForce), fp);
        // the opposite force on a dynamic hit actor (PxVehicleUpdate.cpp:3406-3421)
        if (s.hitActorForceValid && options.reactOnOtherVessels && otherVp != nullptr && other != nullptr)
            otherVp->AddForceAtPhysicsPoint(*other->part, Vector3d(s.hitActorForce), otherVp->PhysicsFromTransform(Vector3d(s.hitActorForcePoint)));
        if (s.limitActive) {
            const int c = vp.ClusterOf(*w->part);
            if (c >= 0 && vp.ClusterBody(static_cast<size_t>(c)) != kInvalidBody)
                rows.push_back({c, w->part, -Vector3d(s.limitDir).normalized(), vp.PhysicsFromTransform(Vector3d(s.limitPoint)), static_cast<double>(s.limitError)});
        }
    }
    if (rows.empty()) return;
    // the travel limit: one unilateral row per bottomed wheel along the travel at the wheel centre (PxVehicleSuspLimitConstraintShader.h:130-142),
    // solved by projected Gauss-Seidel on the cluster's velocity predicted with every force of this step (the buffered forces, the
    // suspension forces above), the impulses applied as forces over the step; the geometric error is corrected by `limitBaumgarte` per step
    const VesselPhysics::BufferedForces bf = vp.PeekBufferedForces();
    const double hd = static_cast<double>(h);
    std::vector<int> clusters;
    for (const LimitRow& r : rows)
        if (std::find(clusters.begin(), clusters.end(), r.cluster) == clusters.end()) clusters.push_back(r.cluster);
    for (int c : clusters) {
        const size_t ci = static_cast<size_t>(c);
        const double m = vp.ClusterMass(ci);
        if (!(m > 0.0)) continue;
        double I[3][3], invI[3][3];
        vp.ClusterInertia(ci, I);
        Inverse3(I, invI);
        const Vector3d com = vp.ClusterCenterOfMass(ci);
        const Vector3d F = (ci < bf.clusterForce.size() ? bf.clusterForce[ci] : Vector3d()) + bf.uniformAcceleration * m;
        const Vector3d T = ci < bf.clusterMoment.size() ? bf.clusterMoment[ci] : Vector3d();
        Vector3d v = vp.ClusterVelocity(ci) + F * (hd / m);
        Vector3d om = vp.ClusterAngularVelocity(ci) + Mul3(invI, T) * hd;
        for (int it = 0; it < options.limitIterations; ++it)
            for (LimitRow& r : rows) {
                if (r.cluster != c) continue;
                const Vector3d arm = r.point - com;
                const Vector3d rn = Vector3d::Cross(arm, r.u);
                const double meff = 1.0 / (1.0 / m + Vector3d::Dot(rn, Mul3(invI, rn)));
                const double vn = Vector3d::Dot(r.u, v + Vector3d::Cross(om, arm));
                const double target = static_cast<double>(options.limitBaumgarte) * r.error / hd;
                const double nl = std::max(0.0, r.lambda + (target - vn) * meff);
                const double d = nl - r.lambda;
                r.lambda = nl;
                v = v + r.u * (d / m);
                om = om + Mul3(invI, rn) * d;
            }
    }
    for (const LimitRow& r : rows)
        if (r.lambda > 0.0) vp.AddForceAtPhysicsPoint(*r.part, r.u * (r.lambda / hd), r.point);
}

void WheelPhysicsHost::NativeStep(float h) {
    for (auto& vw : vessels_) {
        Vessel& v = *vw->vessel_;
        if (v.packed) continue;
        if (VesselPhysics* vp = bubble_->Host().Find(v)) NativeStepVessel(*vw, *vp, h);
    }
}

// The physics step of one tick (seq 291). n substeps of dt / n (SubstepCount); the tick's buffered forces are taken before the first
// substep and added again before every later one; each substep: the managed wheel tick (from the second substep on: the first ran in the
// module slot), the managed forces, the native step, one VesselPhysicsHost::Step.
void WheelPhysicsHost::PhysicsStep(float dt) {
    VesselPhysicsHost& host = bubble_->Host();
    const int n = SubstepCount(dt);
    lastSubsteps_ = n;
    const float h = dt / static_cast<float>(n);
    std::vector<std::pair<VesselPhysics*, VesselPhysics::BufferedForces>> held;
    ApplyFreezes();
    const size_t rebinds0 = host.RebindCount();
    if (n > 1)
        for (const auto& b : host.Bindings()) held.emplace_back(b.get(), b->PeekBufferedForces());
    for (int k = 0; k < n; ++k) {
        if (k > 0) {
            // a structural change since the snapshot (a joint break in a substep) invalidates it: the rest of the tick runs without them
            if (host.RebindCount() != rebinds0) held.clear();
            for (auto& [vp, f] : held)
                for (const auto& b : host.Bindings())
                    if (b.get() == vp) vp->AddBufferedForces(f);
            ManagedSubstep(h);
        }
        TickAnimations(h);  // animatePhysics (ModuleWheelDeployment.cs:239): the clips advance in the physics step
        UpdateColliders();  // WP-29e: the colliders follow the model this step has posed (PhysX syncs the transforms before it simulates)
        ApplyPendingForces();
        NativeStep(h);
        host.Step(h);
    }
}

int WheelPhysicsHost::RigNodeOf(WheelPart& w, const NativeTransform* t) const {
    if (t == nullptr || !w.rig) return -1;
    if (t == &w.colliderHost) return w.colliderNode;
    for (const auto& kv : w.rigTransforms)
        if (kv.second.get() == t) return kv.first;
    return -1;
}

// VPWheelCollider.UpdateVisualWheel's Transform writes (VPWheelCollider.cs:275-300) and its Physics.Raycast (:261) on the part's model rig. The
// rig is the Transform hierarchy: Transform.position = p stores parent.InverseTransformPoint(p) as the local position (Unity's TRS inverse:
// rotate back, divide by the accumulated scale), Transform.localRotation = q stores q; GameObject.SetActive(false) on the wheel transform
// hides the wheel (HubColliders leaves its colliders out). The ray is the wheel's layerMask Physics.Raycast (terrain and scenery, no parts) against
// the static world, in the physics frame.
void WheelPhysicsHost::InstallVisual(WheelPart& w, VPWheelCollider& vc) {
    if (!w.rig || w.visualInstalled) return;
    WheelPart* wp = &w;
    vc.visual = MakeRigVisualHost(
        *w.rig,
        [wp] {
            NativeTransform pt;
            pt.position = wp->part->partTransform.position;
            pt.rotation = wp->part->partTransform.rotation;
            return pt;
        },
        [this, wp](const NativeTransform* t) { return RigNodeOf(*wp, t); }, &w.wheelHidden);
    vc.visualRaycast = [this, wp](const Vector3f& origin, const Vector3f& direction, float maxDistance) -> float {
        VesselPhysics* vp = bubble_->Host().Find(*wp->part->vessel);
        if (vp == nullptr || !bubble_->World().StaticCollisions()) return -1.0f;
        const PhysicsWorld::RayHit h = bubble_->World().RayCast(vp->PhysicsFromTransform(Vector3d(origin)), Vector3d(direction), static_cast<double>(maxDistance), PhysicsWorld::kLayerStatic);
        return h.hit ? static_cast<float>(h.distance) : -1.0f;
    };
    w.visualInstalled = true;
}

void WheelPhysicsHost::Update() {
    timeSinceLevelLoad_ += Time::deltaTime;
    Refresh();
    VesselPhysicsHost& host = bubble_->Host();
    for (auto& vw : vessels_) {
        StartWheelModules(*vw);
        if (VesselPhysics* vp = host.Find(*vw->vessel_)) SyncNative(*vw, *vp);
    }
    // the coroutine pass of the wheel modules (Update.ScriptRunDelayedDynamicFrameRate): wheelSetup once the part is unpacked
    scheduler_.AdvanceFrame(Time::deltaTime);
    scheduler_.RunDelayedDynamicFrameRate();
    for (auto& vw : vessels_)
        for (auto& w : vw->wheels_) WriteBackGeometry(*w);  // wheelSetup re-parented the collider transform; OnWheelSetup may have moved it
    for (auto& vw : vessels_) {
        const bool unpacked = !vw->vessel_->packed;
        for (auto& w : vw->wheels_) {
            KSPWheelController* c = w->Controller();
            if (c != nullptr && options.tyreCurve == TyreCurve::Lineal && w->base->IsWheelSetUp() && c->tireFriction != nullptr &&
                c->tireFriction->model == TireFriction::Model::Lineal && dynamic_cast<TireFriction::LinealFriction*>(c->tireFriction->curve()) == nullptr)
                c->tireFriction->SetupFrictionCurves();  // the switch: rebuild the curve for the model ModuleWheelBase.cs:361 assigns
            if (!w->started) continue;
            // Part.Update's PartValues refresh of the wheel part (Part.cs:4319-4331)
            if (FlightGlobals::ready && w->part->fields.started && (w->part->fields.overrideSkillUpdate || w->part->fields.CrewCapacity > 0))
                w->part->partValues.Update();
            if (!unpacked) continue;
            for (PartModule* m : w->modules)
                if (auto* de = dynamic_cast<ModuleWheelDeployment*>(m)) de->Update();  // ModuleWheelDeployment.cs:693
            if (c != nullptr && c->enabled() && c->initialized()) c->Update();       // VehicleBase.cs:1035 (steer angle, visual wheel)
        }
    }
    for (auto& vw : vessels_)
        for (auto& w : vw->wheels_) WriteBackGeometry(*w);
    // LateUpdate (ModuleWheelSuspension.cs:324, ModuleWheelDeployment.cs:701, ModuleWheelBogey.cs:149)
    for (auto& vw : vessels_)
        for (auto& w : vw->wheels_) {
            if (!w->started) continue;
            for (PartModule* m : w->modules) {
                if (auto* su = dynamic_cast<ModuleWheelSuspension*>(m)) su->LateUpdate();
                else if (auto* de = dynamic_cast<ModuleWheelDeployment*>(m)) de->LateUpdate();
                else if (auto* bo = dynamic_cast<ModuleWheelBogey*>(m)) bo->LateUpdate();
            }
        }
}

}  // namespace ksp::physics
