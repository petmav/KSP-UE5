#include "rover.hpp"

#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyFilter.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/OffsetCenterOfMassShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/Vehicle/VehicleCollisionTester.h>
#include <Jolt/Physics/Vehicle/VehicleConstraint.h>
#include <Jolt/Physics/Vehicle/VehicleController.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <limits>
#include <stdexcept>

#include "ksp_context.hpp"
#include "shapes.hpp"

namespace wspike {

using namespace JPH;
using Clock = std::chrono::steady_clock;

const char* NativeName(NativeModel m) { return m == NativeModel::Explicit ? "explicit" : "jolt-vc"; }

namespace {

double Ms(Clock::time_point a, Clock::time_point b) { return std::chrono::duration<double, std::milli>(b - a).count(); }
Vec3 V(const DVec3& d) { return Vec3(static_cast<float>(d.GetX()), static_cast<float>(d.GetY()), static_cast<float>(d.GetZ())); }

// Jolt does the suspension only: no longitudinal / lateral friction parts (the tyre is KSP's VPP model, applied as forces).
class NullController;
class NullControllerSettings final : public VehicleControllerSettings {
public:
    void SaveBinaryState(StreamOut&) const override {}
    void RestoreBinaryState(StreamIn&) override {}
    VehicleController* ConstructController(VehicleConstraint& c) const override;
};
class NullController final : public VehicleController {
public:
    explicit NullController(VehicleConstraint& c) : VehicleController(c) {}
    Ref<VehicleControllerSettings> GetSettings() const override { return new NullControllerSettings(); }

protected:
    Wheel* ConstructWheel(const WheelSettings& w) const override { return new Wheel(w); }
    bool AllowSleep() const override { return false; }
    void PreCollide(float, PhysicsSystem&) override {}
    void PostCollide(float, PhysicsSystem&) override {}
    bool SolveLongitudinalAndLateralConstraints(float) override { return false; }
    void SaveState(StateRecorder&) const override {}
    void RestoreState(StateRecorder&) override {}
#ifdef JPH_DEBUG_RENDERER
    void Draw(DebugRenderer*) const override {}
#endif
};
VehicleController* NullControllerSettings::ConstructController(VehicleConstraint& c) const { return new NullController(c); }

// Flags contacts of the vessel body with static geometry (a non-wheel collider on the ground)
class GroundContactListener final : public ContactListener {
public:
    BodyID vessel;
    std::atomic<int> touches{0};
    void OnContactAdded(const Body& a, const Body& b, const ContactManifold&, ContactSettings&) override { Check(a, b); }
    void OnContactPersisted(const Body& a, const Body& b, const ContactManifold&, ContactSettings&) override { Check(a, b); }

private:
    void Check(const Body& a, const Body& b) {
        if ((a.GetID() == vessel && b.IsStatic()) || (b.GetID() == vessel && a.IsStatic())) touches++;
    }
};

}  // namespace

struct Rover::JoltVehicle {
    Ref<VehicleConstraint> constraint;
    Array<Ref<WheelSettings>> settings;
    std::vector<float> prevJounce;
};

namespace {
GroundContactListener g_listener;
}

Vec3 CraftUpOf(const spike::CraftModel& m);  // scenarios.cpp (recorded by LoadCraft)

Rover::Rover(spike::KspContext& ctx, spike::CraftModel& craft, const RoverConfig& cfg) : ctx_(ctx), craft_(craft), cfg_(cfg) {
    cfg_.craftUp = CraftUpOf(craft);
    world_ = std::make_unique<spike::World>(cfg_.world);
    const ksp::CelestialBody& kerbin = ctx_.Kerbin();
    mu_ = kerbin.gMagnitudeAtCenter;
    omega_ = cfg_.rotatingFrame ? DVec3(0.0, -2.0 * JPH_PI / kerbin.rotationPeriod, 0.0) : DVec3::sZero();
    geeASL_ = kerbin.GeeASL * cfg_.gravityScale;

    // ---- ground: the 900-quad PQS patch around the pad (WP-11 heights) or a flat plane at the spawn point
    const spike::SurfaceFrame pad = spike::PadFrame(ctx_);
    const DVec3 spawnDir = (pad.up * pad.radius + pad.east * cfg_.startOffsetEast + pad.north * cfg_.startOffsetNorth).Normalized();
    up_ = spawnDir;
    north_ = (pad.north - up_ * pad.north.Dot(up_)).Normalized();
    east_ = up_.Cross(north_);
    groundRadius_ = ctx_.SurfaceRadius(ksp::Vector3d(up_.GetX(), up_.GetY(), up_.GetZ()));
    terrainHeightAtSpawn_ = groundRadius_ - kerbin.Radius;
    if (cfg_.ground == Ground::PQS) {
        terrain_ = spike::BuildTerrainPatch(*world_, ctx_, pad, DVec3::sZero(), spike::TerrainConfig());
        statics_ = static_cast<int>(terrain_.bodies.size());
        terrainMin_ = terrain_.minHeight;
        terrainMax_ = terrain_.maxHeight;
    } else {
        const DVec3 c = up_ * (groundRadius_ - 5.0);
        const Quat q = Mat44(Vec4(V(east_), 0), Vec4(V(up_), 0), Vec4(V(north_), 0), Vec4(0, 0, 0, 1)).GetQuaternion().Normalized();
        BodyCreationSettings b(new BoxShape(Vec3(3000.0f, 5.0f, 3000.0f)), RVec3(c), q, EMotionType::Static, spike::Layers::STATIC);
        b.mFriction = 0.6f;
        world_->Bodies().CreateAndAddBody(b, EActivation::DontActivate);
        statics_ = 1;
        terrainMin_ = terrainMax_ = terrainHeightAtSpawn_;
    }
    BuildBody();
    BuildWheels();
    BuildNative();
    world_->System().OptimizeBroadPhase();
    g_listener.vessel = bodyId_;
    world_->System().SetContactListener(&g_listener);
}

Rover::~Rover() {
    if (world_) world_->System().SetContactListener(nullptr);
    if (jv_ && jv_->constraint) {
        world_->System().RemoveStepListener(jv_->constraint);
        world_->System().RemoveConstraint(jv_->constraint);
    }
}

void Rover::BuildBody() {
    const size_t n = craft_.parts.size();
    isWheelPart_.assign(n, false);
    for (size_t i = 0; i < n; i++)
        if (craft_.parts[i].part->Modules().GetModule("ModuleWheelBase")) isWheelPart_[i] = true;
    // mass and CoM: KSP's rb.mass per physical part (physicsless children folded in), CoM at the part's CoMOffset
    double m = 0.0;
    DVec3 c = DVec3::sZero();
    for (const spike::PartSpec& s : craft_.parts) {
        if (!s.physical) continue;
        const DVec3 pc = s.pos + DVec3(s.rot * s.comLocal);
        c += pc * static_cast<double>(s.rbMass);
        m += s.rbMass;
    }
    c /= m;
    mass_ = m;
    comCraft_ = V(c);
    // inertia about the CoM in the craft axes (= body axes): per-part tensor (drag-cube box) x max(1, rb.mass) (Part.cs:4263)
    double I[3][3] = {};
    for (const spike::PartSpec& s : craft_.parts) {
        if (!s.physical) continue;
        const double pm = s.rbMass;
        const double scale = cfg_.inertiaFloor ? std::max(1.0, pm) : pm;
        const Mat44 R = Mat44::sRotation(s.rot);
        const Vec3 d = s.inertiaPerMass * static_cast<float>(scale);
        for (int a = 0; a < 3; a++)
            for (int b = 0; b < 3; b++) {
                double v = 0.0;
                for (int k = 0; k < 3; k++) v += static_cast<double>(R(a, k)) * static_cast<double>(d[k]) * static_cast<double>(R(b, k));
                I[a][b] += v;
            }
        const DVec3 r = s.pos + DVec3(s.rot * s.comLocal) - c;
        const double rr = r.Dot(r);
        const double rv[3] = {r.GetX(), r.GetY(), r.GetZ()};
        for (int a = 0; a < 3; a++)
            for (int b = 0; b < 3; b++) I[a][b] += pm * ((a == b ? rr : 0.0) - rv[a] * rv[b]);
    }
    inertiaBody_ = Mat44::sIdentity();
    for (int a = 0; a < 3; a++)
        for (int b = 0; b < 3; b++) inertiaBody_(a, b) = static_cast<float>(I[a][b]);

    // collider: the drag-cube boxes of every part except wheel / leg parts (their ground contact is the WheelCollider ray)
    StaticCompoundShapeSettings sc;
    int boxes = 0;
    for (size_t i = 0; i < n; i++) {
        const spike::PartSpec& s = craft_.parts[i];
        if (isWheelPart_[i]) continue;
        sc.AddShape(V(s.pos - c), s.rot, spike::PartBoxShape(s, i));
        boxes++;
    }
    if (boxes == 0) sc.AddShape(Vec3::sZero(), Quat::sIdentity(), new BoxShape(Vec3::sReplicate(0.25f)));
    auto cr = sc.Create();
    if (cr.HasError()) throw std::runtime_error(cr.GetError().c_str());
    RefConst<Shape> compound = cr.Get();
    OffsetCenterOfMassShapeSettings oc(-compound->GetCenterOfMass(), compound);
    auto orr = oc.Create();
    if (orr.HasError()) throw std::runtime_error(orr.GetError().c_str());

    // placement: the craft's up axis -> local up, the vessel's drive forward -> the heading (0 = north, 90 = east)
    upCraft_ = cfg_.craftUp.Normalized();
    // vessel forward in the craft frame: the root part's up (Vessel.ReferenceTransform), projected on the ground plane
    const spike::PartSpec& root = craft_.parts[craft_.root];
    Vec3 fc = root.rot * Vec3::sAxisY();
    fc = fc - upCraft_ * fc.Dot(upCraft_);
    if (fc.Length() < 0.5f) {
        fc = Vec3::sAxisZ() - upCraft_ * upCraft_.GetZ();
        if (fc.Length() < 0.5f) fc = Vec3::sAxisX() - upCraft_ * upCraft_.GetX();
    }
    fc = fc.Normalized();
    forwardCraft_ = fc;
    const double heading = cfg_.headingDeg * JPH_PI / 180.0;
    const Vec3 fw = V(north_ * std::cos(heading) + east_ * std::sin(heading));
    const Vec3 uw = V(up_);
    // R maps (right, up, forward) of the craft onto the same triple in the world; right = up x forward in both
    const Mat44 C(Vec4(upCraft_.Cross(fc), 0), Vec4(upCraft_, 0), Vec4(fc, 0), Vec4(0, 0, 0, 1));
    const Mat44 W(Vec4(uw.Cross(fw), 0), Vec4(uw, 0), Vec4(fw, 0), Vec4(0, 0, 0, 1));
    craftToBody_ = (W * C.Transposed3x3()).GetQuaternion().Normalized();

    // lowest point at full suspension extension (wheels: computed in BuildWheels; parts: box corners) -> spawn height
    double lowest = 1e30;
    for (size_t i = 0; i < n; i++) {
        const spike::PartSpec& s = craft_.parts[i];
        if (isWheelPart_[i]) continue;
        for (int k = 0; k < 8; k++) {
            const Vec3 corner = s.boxCenter + Vec3((k & 1) ? s.boxHalf.GetX() : -s.boxHalf.GetX(), (k & 2) ? s.boxHalf.GetY() : -s.boxHalf.GetY(),
                                                   (k & 4) ? s.boxHalf.GetZ() : -s.boxHalf.GetZ());
            lowest = std::min(lowest, static_cast<double>((V(s.pos) + s.rot * corner).Dot(upCraft_)));
        }
    }
    lowestPartY_ = lowest;

    BodyCreationSettings b(orr.Get(), RVec3::sZero(), craftToBody_, EMotionType::Dynamic, spike::Layers::MOVING);
    spike::KspBodyDefaults(b, 50.0f);
    b.mMassPropertiesOverride.mMass = static_cast<float>(mass_);
    b.mMassPropertiesOverride.mInertia = inertiaBody_;
    b.mCollisionGroup = CollisionGroup(world_->VesselFilter(), 1, 0);
    body_ = world_->Bodies().CreateBody(b);
    if (!body_) throw std::runtime_error("CreateBody failed");
    bodyId_ = body_->GetID();
    world_->Bodies().AddBody(bodyId_, EActivation::Activate);
}

void Rover::BuildWheels() {
    const size_t n = craft_.parts.size();
    const Vec3 upC = upCraft_;  // the editor's up in the assembled craft frame
    double lowestWheel = 1e30;
    for (size_t i = 0; i < n; i++) {
        if (!isWheelPart_[i]) continue;
        const spike::PartSpec& s = craft_.parts[i];
        WheelConfig wc;
        if (!ReadWheelConfig(*s.part, wc)) continue;
        KspWheel w;
        w.Setup(wc, static_cast<float>(geeASL_), true);
        w.tire.model = cfg_.friction;
        w.sprungMassUnity = s.rbMass;
        // ---- geometry proxy (the .mu wheelCollider / deployTgt transforms are not in the repository):
        // the tyre bottom at FULL extension = the bottom face of the part's drag-cube box in the craft frame (legs: KSP's own
        // groundHeightOffset, ModuleWheelBase.OnPutToGround :671-693); suspension axis = craft up; rolling direction = forward.
        // the drag cube of the deployed pose ("Deployed" for ModuleWheelDeployment parts; the part's only cube otherwise)
        Vec3 boxC = s.boxCenter, boxH = s.boxHalf;
        for (const ksp::DragCube& dc : s.part->DragCubes.Cubes())
            if (dc.Name() == "Deployed") {
                boxC = Vec3(dc.Center().x, dc.Center().y, dc.Center().z);
                boxH = Vec3::sMax(Vec3(dc.Size().x, dc.Size().y, dc.Size().z) * 0.5f, Vec3::sReplicate(0.05f));
            }
        Vec3 corners[8];
        float minY = 1e30f;
        for (int k = 0; k < 8; k++) {
            const Vec3 corner = boxC + Vec3((k & 1) ? boxH.GetX() : -boxH.GetX(), (k & 2) ? boxH.GetY() : -boxH.GetY(),
                                            (k & 4) ? boxH.GetZ() : -boxH.GetZ());
            corners[k] = V(s.pos) + s.rot * corner;
            minY = std::min(minY, corners[k].Dot(upC));
        }
        std::sort(corners, corners + 8, [&](Vec3Arg a, Vec3Arg b) { return a.Dot(upC) < b.Dot(upC); });
        Vec3 bottom = (corners[0] + corners[1] + corners[2] + corners[3]) * 0.25f;
        float bottomH = minY;
        if (cfg_.legGroundOffset && wc.wheelType == 2 && wc.groundHeightOffset > 0.0f) bottomH = V(s.pos).Dot(upC) - wc.groundHeightOffset;
        bottom = bottom + upC * (bottomH - bottom.Dot(upC));
        const float D = w.suspensionDistance, r = w.wcRadius;
        const Vec3 unityCenter = bottom + upC * (D + r);
        const Vec3 fwd = forwardCraft_;
        const Vec3 right = upC.Cross(fwd);
        const Vec3 cl = w.UnityCenterLocal();  // in the transform frame (x right, y up, z forward)
        const Vec3 transformPos = unityCenter - (right * cl.GetX() + upC * cl.GetY() + fwd * cl.GetZ());
        WheelGeom g;
        g.part = static_cast<int>(i);
        g.transformLocal = transformPos - comCraft_;
        g.upLocal = upC;
        g.forwardLocal = fwd;
        g.rightLocal = right;
        g.partComLocal = V(s.pos + DVec3(s.rot * s.comLocal)) - comCraft_;
        g.partRbMass = s.rbMass;
        geom_.push_back(g);
        wheels_.push_back(w);
        lowestWheel = std::min(lowestWheel, static_cast<double>(bottomH));
    }
    prevJounce_.assign(wheels_.size(), std::numeric_limits<float>::quiet_NaN());
    limitLambda_.assign(wheels_.size(), 0.0f);
    hitRaw_.assign(wheels_.size(), RVec3::sZero());
    // spawn: lowest point (wheel at full extension or a part box) cfg.spawnClearance above the ground at the spawn point
    const double lowest = std::min(lowestWheel, lowestPartY_);
    spawnClearance_ = cfg_.spawnClearance;
    const double comHeight = static_cast<double>(comCraft_.Dot(upCraft_)) - lowest + cfg_.spawnClearance;
    const DVec3 comPos = up_ * (groundRadius_ + comHeight);
    world_->Bodies().SetPosition(bodyId_, RVec3(comPos), EActivation::Activate);
    startCom_ = comPos;
    world_->Bodies().SetLinearVelocity(bodyId_, V(up_) * static_cast<float>(cfg_.spawnVerticalSpeed));
}

void Rover::BuildNative() {
    if (cfg_.native != NativeModel::JoltVehicle || wheels_.empty()) return;
    jv_ = std::make_unique<JoltVehicle>();
    VehicleConstraintSettings vs;
    vs.mUp = upCraft_;
    vs.mForward = forwardCraft_;
    for (size_t i = 0; i < wheels_.size(); i++) {
        const KspWheel& w = wheels_[i];
        const WheelGeom& g = geom_[i];
        Ref<WheelSettings> ws = new WheelSettings();
        const Vec3 cl = w.UnityCenterLocal();
        ws->mPosition = g.transformLocal + g.rightLocal * cl.GetX() + g.upLocal * cl.GetY() + g.forwardLocal * cl.GetZ();  // top of travel
        ws->mSuspensionDirection = -g.upLocal;
        ws->mSteeringAxis = g.upLocal;
        ws->mWheelUp = g.upLocal;
        ws->mWheelForward = g.forwardLocal;
        ws->mSuspensionMinLength = 0.0f;
        ws->mSuspensionMaxLength = w.suspensionDistance;
        ws->mSuspensionPreloadLength = 0.0f;
        ws->mSuspensionSpring = SpringSettings(ESpringMode::StiffnessAndDamping, 1.0f, 0.1f);
        ws->mRadius = w.wcRadius;
        ws->mWidth = 0.05f;
        ws->mEnableSuspensionForcePoint = false;  // forces at the contact point (KSP's accurateSuspensionForces)
        vs.mWheels.push_back(ws);
    }
    vs.mController = new NullControllerSettings();
    jv_->constraint = new VehicleConstraint(*body_, vs);
    jv_->constraint->SetVehicleCollisionTester(new VehicleCollisionTesterRay(spike::Layers::MOVING, V(up_), DegreesToRadians(80.0f)));
    world_->System().AddConstraint(jv_->constraint);
    world_->System().AddStepListener(jv_->constraint);
    jv_->prevJounce.assign(wheels_.size(), std::numeric_limits<float>::quiet_NaN());
}

DVec3 Rover::Com() const { return DVec3(body_->GetCenterOfMassPosition()); }
Vec3 Rover::Velocity() const { return body_->GetLinearVelocity(); }
Vec3 Rover::AngularVelocity() const { return body_->GetAngularVelocity(); }
DVec3 Rover::Forward() const { return DVec3(body_->GetRotation() * forwardCraft_); }
double Rover::Altitude() const { return (Com() - up_ * groundRadius_).Dot(up_); }
double Rover::TiltDeg() const {
    const Vec3 bu = body_->GetRotation() * upCraft_;
    const double c = std::clamp(static_cast<double>(bu.Dot(V(up_))), -1.0, 1.0);
    return std::acos(c) * 180.0 / JPH_PI;
}

int Rover::GroundedCount() const {
    int c = 0;
    for (const KspWheel& w : wheels_) c += w.hit.grounded ? 1 : 0;
    return c;
}
int Rover::BottomedCount() const {
    int c = 0;
    for (const KspWheel& w : wheels_) c += w.hit.bottomed ? 1 : 0;
    return c;
}
bool Rover::BodyTouchesGround() const { return bodyTouches_; }

void Rover::UpdateWheelFrames() {
    const RMat44 T = body_->GetWorldTransform();  // origin = CoM (OffsetCenterOfMassShape)
    const Quat q = body_->GetRotation();
    for (size_t i = 0; i < wheels_.size(); i++) {
        KspWheel& w = wheels_[i];
        const WheelGeom& g = geom_[i];
        w.transformPos = T * g.transformLocal;
        w.up = q * g.upLocal;
        w.forward = q * g.forwardLocal;
    }
}

void Rover::NativeExplicitPre(float dt, Vec3& force, Vec3& torque) {
    const NarrowPhaseQuery& npq = world_->System().GetNarrowPhaseQuery();
    const BodyLockInterfaceNoLock& locks = world_->System().GetBodyLockInterfaceNoLock();
    const RVec3 com = body_->GetCenterOfMassPosition();
    const Quat q = body_->GetRotation();
    IgnoreSingleBodyFilter self(bodyId_);
    struct Row {
        int wheel;
        Vec3 n, r;
        float err;
    };
    Row rows[64];
    int nrows = 0;
    for (size_t i = 0; i < wheels_.size(); i++) {
        KspWheel& w = wheels_[i];
        const WheelGeom& g = geom_[i];
        const Vec3 up = w.up;
        const Vec3 right = q * g.rightLocal;
        const Vec3 cl = w.UnityCenterLocal();
        const RVec3 center = w.transformPos + right * cl.GetX() + up * cl.GetY() + w.forward * cl.GetZ();
        const float D = w.nativeSpring.suspensionDistance, r = w.wcRadius;
        const float len = D + 2.0f * r;
        const RRayCast ray(center + up * r, -up * len);
        RayCastResult res;
        const float c0 = w.nativeSpring.targetPosition * D;  // rest compression: Unity targetPosition 0 = full extension
        NativeHit& h = w.hit;
        if (npq.CastRay(ray, res, {}, {}, self)) {
            const float c = len - res.mFraction * len;  // compression from full extension
            const RVec3 p = ray.GetPointOnRay(res.mFraction);
            Vec3 nrm = up;
            {
                BodyLockRead lock(locks, res.mBodyID);
                if (lock.Succeeded()) nrm = lock.GetBody().GetWorldSpaceSurfaceNormal(res.mSubShapeID2, p);
            }
            const float j = std::min(c, D) - c0;  // PhysX jounce, clamped to mMaxCompression
            const float js = std::isnan(prevJounce_[i]) ? 0.0f : (j - prevJounce_[i]) / dt;
            prevJounce_[i] = j;
            const float fw = std::max(0.0f, w.nativeSpring.spring * j) + js * w.nativeSpring.damper;  // mSprungMass * alpha = 0
            const float mag = nrm.Dot(up) * fw;                 // hitNorm.dot(-w * F)
            const Vec3 f = nrm * mag;
            force += f;
            torque += Vec3(p - com).Cross(f);
            h.grounded = true;
            hitRaw_[i] = p;
            h.normal = nrm;
            h.force = mag;
            h.compression = c;
            h.bottomed = c > D;
            if (h.bottomed && nrows < 64) rows[nrows++] = {static_cast<int>(i), nrm, Vec3(p - com), c - D};
        } else {
            prevJounce_[i] = -c0;  // jounce = -mMaxDroop when nothing is hit
            h.grounded = false;
            h.force = 0.0f;
            h.compression = 0.0f;
            h.bottomed = false;
        }
    }
    if (nrows == 0) return;
    // suspension-limit constraint (PhysX: suspLimitActiveFlags when dx > mMaxCompression): projected Gauss-Seidel on the
    // body's predicted velocity (this tick's forces included), impulses applied before the step
    const Mat44 invIw = body_->GetInverseInertia();
    const float invM = 1.0f / static_cast<float>(mass_);
    Vec3 v = body_->GetLinearVelocity() + force * (invM * dt);
    Vec3 wv = body_->GetAngularVelocity() + invIw.Multiply3x3(torque) * dt;
    float lambda[64] = {};
    float meff[64];
    for (int k = 0; k < nrows; k++) {
        const Vec3 rn = rows[k].r.Cross(rows[k].n);
        meff[k] = 1.0f / (invM + rn.Dot(invIw.Multiply3x3(rn)));
    }
    for (int it = 0; it < cfg_.limitIterations; it++)
        for (int k = 0; k < nrows; k++) {
            const Row& rw = rows[k];
            const float vn = rw.n.Dot(v + wv.Cross(rw.r));
            const float target = cfg_.limitBaumgarte * rw.err / dt;
            const float nl = std::max(0.0f, lambda[k] + (target - vn) * meff[k]);
            const float d = nl - lambda[k];
            lambda[k] = nl;
            v += rw.n * (d * invM);
            wv += invIw.Multiply3x3(rw.r.Cross(rw.n)) * d;
        }
    BodyInterface& bi = world_->Bodies();
    for (int k = 0; k < nrows; k++)
        if (lambda[k] > 0.0f) bi.AddImpulse(bodyId_, rows[k].n * lambda[k], com + rows[k].r);
}

void Rover::NativeJoltPre() {
    for (size_t i = 0; i < wheels_.size(); i++) {
        const KspWheel& w = wheels_[i];
        WheelSettings* ws = const_cast<WheelSettings*>(jv_->constraint->GetWheel(static_cast<uint>(i))->GetSettings());
        ws->mSuspensionSpring = SpringSettings(ESpringMode::StiffnessAndDamping, w.nativeSpring.spring, w.nativeSpring.damper);
        // KSP's rest offset (targetPosition): zero spring force at compression c0 -> natural length = max length - c0
        ws->mSuspensionPreloadLength = -w.nativeSpring.targetPosition * w.nativeSpring.suspensionDistance;
    }
}

void Rover::NativeJoltPost(float dt) {
    for (size_t i = 0; i < wheels_.size(); i++) {
        KspWheel& w = wheels_[i];
        const JPH::Wheel* jw = jv_->constraint->GetWheel(static_cast<uint>(i));
        NativeHit& h = w.hit;
        const float D = w.nativeSpring.suspensionDistance;
        const float c0 = w.nativeSpring.targetPosition * D;
        if (jw->HasContact()) {
            const float c = D - jw->GetSuspensionLength();
            const float j = std::min(c, D) - c0;
            const float js = std::isnan(jv_->prevJounce[i]) ? 0.0f : (j - jv_->prevJounce[i]) / dt;
            jv_->prevJounce[i] = j;
            const Vec3 nrm = jw->GetContactNormal();
            // what KSP's managed code reads: PhysX's reported spring force for this compression (the force Jolt applied is
            // the implicit soft-constraint impulse, GetSuspensionLambda() / dt)
            h.force = nrm.Dot(w.up) * (std::max(0.0f, w.nativeSpring.spring * j) + js * w.nativeSpring.damper);
            h.grounded = true;
            hitRaw_[i] = jw->GetContactPosition();
            h.normal = nrm;
            h.compression = c;
            h.bottomed = jw->HasHitHardPoint();
        } else {
            jv_->prevJounce[i] = -c0;
            h.grounded = false;
            h.force = 0.0f;
            h.compression = 0.0f;
            h.bottomed = false;
        }
    }
}

void Rover::Tick(const Controls& ctl) {
    const auto t0 = Clock::now();
    const float dt = static_cast<float>(cfg_.dt);
    BodyInterface& bi = world_->Bodies();
    // ---- 1. VesselPrecalculate: state, gravity at the CoM (VesselPrecalculate.cs:281-368)
    const RVec3 comR = body_->GetCenterOfMassPosition();
    const DVec3 com(comR);
    const Vec3 v = body_->GetLinearVelocity();
    const Vec3 wv = body_->GetAngularVelocity();
    const double rl = com.Length();
    const DVec3 gPure = com * (-mu_ / (rl * rl * rl)) * cfg_.gravityScale;
    DVec3 g = gPure;
    if (!omega_.IsNearZero()) {
        g += omega_.Cross(DVec3(v)) * -2.0;            // FlightGlobals.getCoriolisAcc
        g += omega_.Cross(omega_.Cross(-com));         // FlightGlobals.getCentrifugalAcc
    }
    const double ease = cfg_.gravityEase ? std::min(1.0, 0.0625 * (tick_ + 1)) : 1.0;
    Vec3 force = V(g * (ease * mass_));
    Vec3 torque = Vec3::sZero();
    inputs_.dt = dt;
    inputs_.gravityForPos = V(gPure);
    inputs_.geeASL = geeASL_;
    inputs_.totalMass = craft_.totalMass;
    inputs_.landed = GroundedCount() > 0 || bodyTouches_;
    inputs_.wheelThrottle = ctl.wheelThrottle;
    inputs_.brakeInput = ctl.brake;
    UpdateWheelFrames();
    const auto t1 = Clock::now();
    // ---- 2. managed wheel code (KSP script order: vessel modules, part modules, then the KSPWheelController)
    for (size_t i = 0; i < wheels_.size(); i++) {  // WheelHit post-processing of VPWheelCollider.GetGroundHit: + rb.velocity * dt
        KspWheel& w = wheels_[i];
        if (w.hit.grounded) {
            const Vec3 partVel = v + wv.Cross(body_->GetRotation() * geom_[i].partComLocal);
            w.hit.point = hitRaw_[i] + partVel * dt;
        }
    }
    SuspensionLoadBalancerFixedUpdate(wheels_.data(), static_cast<int>(wheels_.size()), inputs_);
    for (KspWheel& w : wheels_) {
        w.WheelBaseFixedUpdate(inputs_);
        w.SuspensionFixedUpdate(inputs_);
        w.MotorFixedUpdate(inputs_);
        w.BrakesFixedUpdate(inputs_);
    }
    BodyView bv;
    bv.com = comR;
    bv.v = v;
    bv.w = wv;
    for (size_t i = 0; i < wheels_.size(); i++) {
        KspWheel& w = wheels_[i];
        bv.partRbMass = geom_[i].partRbMass;
        Vec3 f;
        RVec3 p;
        if (w.VehicleFixedUpdate(inputs_, bv, f, p)) {
            force += f;
            torque += Vec3(p - comR).Cross(f);
        }
        w.DamageFixedUpdate(inputs_);
    }
    const auto t2 = Clock::now();
    // ---- 3. native suspension (inside Physics.Simulate in Unity)
    if (cfg_.native == NativeModel::Explicit) NativeExplicitPre(dt, force, torque);
    else if (jv_) NativeJoltPre();
    bi.AddForce(bodyId_, force, EActivation::Activate);
    bi.AddTorque(bodyId_, torque, EActivation::Activate);
    const auto t3 = Clock::now();
    // ---- 4. the step
    g_listener.touches = 0;
    world_->Step(dt, 1);
    bodyTouches_ = g_listener.touches.load() > 0;
    const auto t4 = Clock::now();
    // ---- 5. WheelHit for the next tick
    if (cfg_.native == NativeModel::JoltVehicle && jv_) NativeJoltPost(dt);
    const auto t5 = Clock::now();
    times_.precalc = Ms(t0, t1);
    times_.managed = Ms(t1, t2);
    times_.nativePre = Ms(t2, t3);
    times_.step = Ms(t3, t4);
    times_.nativePost = Ms(t4, t5);
    times_.total = Ms(t0, t5);
    time_ += cfg_.dt;
    tick_++;
}

}  // namespace wspike
