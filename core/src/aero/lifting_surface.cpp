// WP-21: ModuleLiftingSurface / ModuleControlSurface / ModuleAeroSurface (see ksp/aero/lifting_surface.hpp).
#include "ksp/aero/lifting_surface.hpp"

#include <cmath>

#include "ksp/core/exceptions.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/unity/quaternion_native.hpp"

namespace ksp {

using aero::FE;
using aero::MonoFloat;
using aero::NetFloat;

namespace aero {

// ModuleLiftingSurface.cs:205
template <class P>
LiftingSurfaceCoefficients SetupCoefficients(const Vector3f& pointVelocity, const Vector3f& axis, float transformSign,
                                             bool omnidirectional) {
    LiftingSurfaceCoefficients c;
    c.nVel = U<P>::Normalize(pointVelocity);
    c.liftVector = axis;
    c.liftVector *= transformSign;
    c.liftDot = U<P>::Dot(c.nVel, c.liftVector);
    c.absDot = omnidirectional ? Mathf::Abs(c.liftDot) : Mathf::Clamp01(c.liftDot);
    return c;
}

// ModuleLiftingSurface.cs:232
template <class P>
Vector3f GetLiftVector(const LiftingSurfaceParams& m, const Vector3f& liftVector, float liftDot, float absDot, double Q,
                       float mach, float& liftScalar) {
    using E = FE<P>;
    if (m.nodeBlocked) {
        liftScalar = 0.0f;
        return Vector3f::zero();
    }
    if (m.liftCurve == nullptr || m.liftMachCurve == nullptr) throw NullReferenceException();
    liftScalar = S(E(Mathf::Sign(liftDot)) * E(m.liftCurve->Evaluate(absDot)) * E(m.liftMachCurve->Evaluate(mach)));
    liftScalar = S(E(liftScalar) * E(m.deflectionLiftCoeff));
    if (liftScalar != 0.0f && !std::isnan(liftScalar)) {
        liftScalar = static_cast<float>(Q * D(E(PhysicsGlobals::LiftMultiplier()) * E(liftScalar)));
        if (m.perpendicularOnly) {
            Vector3f vector = -liftVector * liftScalar;
            vector = U<P>::ProjectOnPlane(vector, -m.nVel);
            liftScalar = U<P>::Magnitude(vector);
            return vector;
        }
        return -liftVector * liftScalar;
    }
    liftScalar = 0.0f;
    return Vector3f::zero();
}

// ModuleLiftingSurface.cs:259
template <class P>
Vector3f GetDragVector(const LiftingSurfaceParams& m, const Vector3f& nVel, float absDot, double Q, float mach,
                       float& dragScalar) {
    using E = FE<P>;
    if (m.nodeBlocked) {
        dragScalar = 0.0f;
        return Vector3f::zero();
    }
    if (m.dragCurve == nullptr || m.dragMachCurve == nullptr) throw NullReferenceException();
    dragScalar = S(E(m.dragCurve->Evaluate(absDot)) * E(m.dragMachCurve->Evaluate(mach)));
    dragScalar = S(E(dragScalar) * E(m.deflectionLiftCoeff));
    if (dragScalar != 0.0f && !std::isnan(dragScalar)) {
        dragScalar = static_cast<float>(Q * D(E(dragScalar) * E(PhysicsGlobals::LiftDragMultiplier())));
        return -nVel * dragScalar;
    }
    dragScalar = 0.0f;
    return Vector3f::zero();
}

#define KSP_LS_INSTANTIATE(P)                                                                                            \
    template LiftingSurfaceCoefficients SetupCoefficients<P>(const Vector3f&, const Vector3f&, float, bool);           \
    template Vector3f GetLiftVector<P>(const LiftingSurfaceParams&, const Vector3f&, float, float, double, float, float&); \
    template Vector3f GetDragVector<P>(const LiftingSurfaceParams&, const Vector3f&, float, double, float, float&);
KSP_LS_INSTANTIATE(MonoFloat)
KSP_LS_INSTANTIATE(NetFloat)
#undef KSP_LS_INSTANTIATE

}  // namespace aero

namespace {
using E = FE<MonoFloat>;
using aero::S;
Part* PartOf(PartModule& m) { return dynamic_cast<Part*>(m.part); }
const Part* PartOf(const PartModule& m) { return dynamic_cast<const Part*>(m.part); }
bool SceneIsFlight() { return SaveEnvironment::Current().loadedScene == GameScenes::FLIGHT; }
bool SceneIsGame() { return SaveEnvironment::Current().loadedSceneIsGame; }
}  // namespace

// ---- ModuleLiftingSurface ------------------------------------------------------------------------------------------------

Vector3f ModuleLiftingSurface::AxisOf(const LiftingSurfaceFrame& frame) const {
    const int64_t dir = Get<EnumValue>("transformDir").value;
    if (dir == static_cast<int64_t>(TransformDir::Z)) return frame.baseForward;
    if (dir == static_cast<int64_t>(TransformDir::Y)) return frame.baseUp;
    return frame.baseRight;
}

aero::LiftingSurfaceParams ModuleLiftingSurface::Params() const {
    aero::LiftingSurfaceParams m;
    const ModuleValue* v = Value("liftCurve");
    m.liftCurve = v ? std::get_if<FloatCurve>(v) : nullptr;
    v = Value("liftMachCurve");
    m.liftMachCurve = v ? std::get_if<FloatCurve>(v) : nullptr;
    v = Value("dragCurve");
    m.dragCurve = v ? std::get_if<FloatCurve>(v) : nullptr;
    v = Value("dragMachCurve");
    m.dragMachCurve = v ? std::get_if<FloatCurve>(v) : nullptr;
    m.deflectionLiftCoeff = Get<float>("deflectionLiftCoeff");
    m.perpendicularOnly = Get<bool>("perpendicularOnly");
    bool blocked = false;
    if (Get<bool>("nodeEnabled")) {
        Part* p = const_cast<Part*>(PartOf(*this));
        AttachNode* node = p ? p->FindAttachNode(Get<std::string>("attachNodeName")) : nullptr;
        if (node == nullptr) throw NullReferenceException();  // attachNode.attachedPart on a null node
        blocked = node->attachedPart != nullptr;
    }
    m.nodeBlocked = blocked;
    m.nVel = nVel;
    return m;
}

// ModuleLiftingSurface.cs:129 OnStart (the Localizer / FindModelTransform parts are the shell's: baseTransform is the frame input)
void ModuleLiftingSurface::OnStart(StartState) {
    Set<float>("liftScalar", 0.0f);
    if (Get<bool>("nodeEnabled") && !Get<std::string>("attachNodeName").empty()) {
        Part* p = PartOf(*this);
        if (p == nullptr || p->FindAttachNode(Get<std::string>("attachNodeName")) == nullptr) Set<bool>("nodeEnabled", false);
    }
    const std::string curveName = Get<std::string>("liftingSurfaceCurve");
    if (curveName != "Custom") {
        if (std::shared_ptr<PhysicsGlobals::LiftingSurfaceCurve> c = PhysicsGlobals::GetLiftingSurfaceCurve(curveName)) {
            Set<FloatCurve>("liftCurve", c->liftCurve);
            Set<FloatCurve>("liftMachCurve", c->liftMachCurve);
            Set<FloatCurve>("dragCurve", c->dragCurve);
            Set<FloatCurve>("dragMachCurve", c->dragMachCurve);
        }
    }
}

// ModuleLiftingSurface.cs:205
void ModuleLiftingSurface::SetupCoefficients(const Vector3f& pv, const LiftingSurfaceFrame& frame, Vector3f& outNVel,
                                             Vector3f& outLiftVector, float& outLiftDot, float& outAbsDot) {
    const aero::LiftingSurfaceCoefficients c =
        aero::SetupCoefficients<MonoFloat>(pv, AxisOf(frame), Get<float>("transformSign"), Get<bool>("omnidirectional"));
    outNVel = c.nVel;
    outLiftVector = c.liftVector;
    outLiftDot = c.liftDot;
    outAbsDot = c.absDot;
}

// ModuleLiftingSurface.cs:232
Vector3f ModuleLiftingSurface::GetLiftVector(const Vector3f& lv, float ld, float ad, double Q, float mach) {
    float liftScalar = Get<float>("liftScalar");
    const Vector3f r = aero::GetLiftVector<MonoFloat>(Params(), lv, ld, ad, Q, mach, liftScalar);
    Set<float>("liftScalar", liftScalar);
    return r;
}

// ModuleLiftingSurface.cs:254
Vector3f ModuleLiftingSurface::GetDragVector(const Vector3f& nv, float ad, double Q) {
    const Part* p = PartOf(*this);
    if (p == nullptr) throw NullReferenceException();
    return GetDragVector(nv, ad, Q, static_cast<float>(p->fields.machNumber));
}

// ModuleLiftingSurface.cs:259
Vector3f ModuleLiftingSurface::GetDragVector(const Vector3f& nv, float ad, double Q, float mach) {
    float dragScalar = Get<float>("dragScalar");
    const Vector3f r = aero::GetDragVector<MonoFloat>(Params(), nv, ad, Q, mach, dragScalar);
    Set<float>("dragScalar", dragScalar);
    return r;
}

// ModuleLiftingSurface.cs:169 FixedUpdate
SurfaceForces ModuleLiftingSurface::FixedUpdateAero(const LiftingSurfaceFrame& frame) {
    SurfaceForces out;
    Part* p = PartOf(*this);
    if (!SceneIsFlight() || !frame.hasRigidbody) return out;
    if (p == nullptr) throw NullReferenceException();
    if (!p->ShieldedFromAirstream()) {
        pointVelocity = frame.pointVelocity;
        const PartFields& f = p->fields;
        const double submergedPortion = f.submergedPortion;
        const double num = 1.0 - submergedPortion;
        Qlift = (f.dynamicPressurekPa * num + f.submergedDynamicPressurekPa * submergedPortion * f.submergedLiftScalar) * 1000.0;
        Qdrag = (f.dynamicPressurekPa * num + f.submergedDynamicPressurekPa * submergedPortion * f.submergedDragScalar) * 1000.0;
        float num2 = 0.0f;
        SetupCoefficients(pointVelocity, frame, nVel, liftVector, liftDot, num2);
        liftForce = GetLiftVector(liftVector, liftDot, num2, Qlift, static_cast<float>(f.machNumber));
        // ApplyLiftForceAdjustments: no mission adjusters -> identity
        out.applied = true;
        out.liftForce = liftForce;
        out.liftPosition = frame.colWorld;
        if (Get<bool>("useInternalDragModel")) {
            dragForce = GetDragVector(nVel, num2, Qdrag);
            out.applyDrag = true;
            out.dragForce = dragForce;
            out.dragPosition = frame.copWorld;
        }
    } else {
        Qdrag = Qlift = 0.0;
        nVel = Vector3f::zero();
    }
    return out;
}

// ModuleLiftingSurface.cs:303
void ModuleLiftingSurface::OnCenterOfLiftQuery(CenterOfLiftQuery& qry, const LiftingSurfaceFrame& frame, double speedOfSound,
                                               const Vector3f& colWorldFromBase) {
    const Part* p = PartOf(*this);
    if (p == nullptr) throw NullReferenceException();
    if (p->ShieldedFromAirstream()) return;
    const float mach = static_cast<float>(static_cast<double>(qry.refVector.magnitude()) / speedOfSound);
    const double q = 0.5 * qry.refAirDensity * static_cast<double>(qry.refVector.sqrMagnitude());
    Vector3f unusedNVel, vector2;
    float num2 = 0.0f;
    SetupCoefficients(qry.refVector, frame, unusedNVel, vector2, liftDot, num2);
    qry.pos = colWorldFromBase;  // baseTransform.TransformPoint(part.CoLOffset)
    if (num2 < 0.001f) {
        qry.dir = Vector3f::zero();
        qry.lift = 1E-05f;
    } else {
        qry.dir = GetLiftVector(vector2, liftDot, num2, q, mach);
        qry.lift = qry.dir.magnitude();
        qry.dir = qry.dir.normalized();  // Vector3.Normalize(): same arithmetic as normalized
    }
}

void ModuleLiftingSurface::OnCenterOfLiftQuery(CenterOfLiftQuery& qry) {
    if (!hooks.speedOfSound) return;
    LiftingSurfaceFrame frame;
    const Part* p = PartOf(*this);
    const Vector3f col = p ? p->partTransform.TransformPoint(p->fields.CoLOffset) : Vector3f::zero();
    OnCenterOfLiftQuery(qry, frame, hooks.speedOfSound(qry.refStaticPressure, qry.refAirDensity), col);
}

// ---- ModuleControlSurface ------------------------------------------------------------------------------------------------

// ModuleControlSurface.cs:178 OnAwake (ctrlSurface / editor / PAW event hooks are the shell's)
void ModuleControlSurface::OnAwake() {
    ModuleLiftingSurface::OnAwake();
    originalActuatorSpeed = Get<float>("actuatorSpeed");
}

// ModuleControlSurface.cs:234
void ModuleControlSurface::ModifyAuthorityLimiter() {
    Set<float>("authorityLimiterUI", S(E(Get<float>("ctrlSurfaceRange")) * E(Get<float>("authorityLimiter")) * E(0.01f)));
}

// ModuleControlSurface.cs:246
void ModuleControlSurface::ModifyAuthorityLimiterUI() {
    Set<float>("authorityLimiter", S(E(100.0f) * E(Get<float>("authorityLimiterUI")) / E(Get<float>("ctrlSurfaceRange"))));
}

// ModuleControlSurface.cs:256 OnStart
void ModuleControlSurface::OnStart(StartState state) {
    ModuleLiftingSurface::OnStart(state);
    const float ctrlSurfaceRange = Get<float>("ctrlSurfaceRange");
    if (std::isnan(Get<float>("deployAngle"))) Set<float>("deployAngle", ctrlSurfaceRange);
    if (std::isnan(deployAngleLimits.x)) deployAngleLimits.x = S(-E(ctrlSurfaceRange) * E(1.5f));
    if (std::isnan(deployAngleLimits.y)) deployAngleLimits.y = S(E(ctrlSurfaceRange) * E(1.5f));
    if (SceneIsGame()) {
        // ctrlSurface = FindModelTransform(transformName); neutral = its localRotation: set by the model reader (`neutral`)
        ModifyAuthorityLimiter();
    }
}

// ModuleControlSurface.cs:563 (sic: the second test is `action < epsilon`)
float ModuleControlSurface::ActionSign(float action, float epsilon) {
    return (action > epsilon) ? 1.0f : ((action < epsilon) ? -1.0f : 0.0f);
}

// ModuleControlSurface.cs:743
void ModuleControlSurface::CtrlSurfaceUpdate(const ControlSurfaceFrame& frame) {
    // displaceVelocity: RotatingCtrlSurfaceUpdate (robotic rotor blades) is not ported; the fixed-surface update is used
    FixedCtrlSurfaceUpdate(frame);
}

// ModuleControlSurface.cs:704
void ModuleControlSurface::FixedCtrlSurfaceUpdate(const ControlSurfaceFrame& frame) {
    const Vector3f currentCoM = frame.currentCoM;
    const Vector3f vector = frame.localCoM;  // baseTransform.InverseTransformPoint(currentCoM)
    const Vector3f rhs(vector.x, 0.0f, vector.z);
    const bool ignorePitch = Get<bool>("ignorePitch"), ignoreYaw = Get<bool>("ignoreYaw"), ignoreRoll = Get<bool>("ignoreRoll");
    inputVector = frame.referenceRotation * Vector3f(ignorePitch ? 0.0f : frame.pitch, 0.0f, ignoreYaw ? 0.0f : frame.yaw);
    action = Vector3f::Dot(inputVector, frame.surface.baseRotation * Vector3f::right());
    const float ctrlSurfaceRange = Get<float>("ctrlSurfaceRange");
    if (ignoreRoll) {
        roll = 0.0f;
    } else {
        const Quaternionf invBase = aero::U<MonoFloat>::Inverse(frame.surface.baseRotation);
        const float a = Vector3f::Dot(Vector3f::right(), rhs);
        const float b = Mathf::Abs(Vector3f::Dot(rhs.normalized(), invBase * frame.referenceUp));
        const float c = Mathf::Sign(Vector3f::Dot(frame.surface.baseUp, frame.referenceUp));
        roll = S(E(a) * (E(1.0f) - (E(b) * E(0.5f) + E(0.5f))) * E(c) * E(frame.roll) * E(Mathf::Sign(ctrlSurfaceRange)));
    }
    if (vector.y < 0.0f) action = -action;
    action = S(E(deflectionDirection) * E(Mathf::Clamp(S(E(action) - E(roll)), -1.0f, 1.0f)));
    action = S(E(ctrlSurfaceRange) * E(action) * E(Get<float>("authorityLimiter")) * E(0.01f));
    if (Get<bool>("deploy")) {
        float num = 0.0f;
        const float invertA = Get<bool>("deployInvert") ? -1.0f : 1.0f;
        if (Get<bool>("usesMirrorDeploy")) {
            num = S(E(invertA) * E(Get<bool>("partDeployInvert") ? -1.0f : 1.0f) * E(Get<bool>("mirrorDeploy") ? -1.0f : 1.0f));
        } else {
            const Vector3f rel = aero::U<MonoFloat>::Inverse(frame.referenceRotation) * (frame.basePosition - currentCoM);
            num = S(E(invertA) * E(Mathf::Sign(rel.x)));
        }
        currentDeployAngle = S(-E(num) * E(Get<float>("deployAngle")));
        action = S(E(action) + E(currentDeployAngle));
    }
    action = Mathf::Clamp(action, S(E(-1.5f) * E(ctrlSurfaceRange)), S(E(1.5f) * E(ctrlSurfaceRange)));
    action = S(E(action) * E(deflectionDirection));
    const float step = S(E(Get<float>("actuatorSpeed")) * E(frame.fixedDeltaTime));
    deflection = Get<bool>("useExponentialSpeed") ? Mathf::Lerp(deflection, action, step) : Mathf::MoveTowards(deflection, action, step);
    ctrlSurfaceLocalRotation = UnityNative::AngleAxis(deflection, Vector3f::right()) * neutral;
}

// ModuleControlSurface.cs:366 FixedUpdate
SurfaceForces ModuleControlSurface::FixedUpdateAero(const ControlSurfaceFrame& frame) {
    SurfaceForces out;
    if (!SceneIsFlight()) return out;
    Part* p = PartOf(*this);
    if (p == nullptr) throw NullReferenceException();
    const float ctrlSurfaceArea = Get<float>("ctrlSurfaceArea");
    if (frame.surface.hasRigidbody && !p->ShieldedFromAirstream()) {
        pointVelocity = frame.surface.pointVelocity;
        const PartFields& f = p->fields;
        const double submergedPortion = f.submergedPortion;
        const double num = 1.0 - submergedPortion;
        const float mach = static_cast<float>(f.machNumber);
        // displaceVelocity (rotor blades, the vessel's speed of sound) is the robotics package's: not ported
        Qlift = (f.dynamicPressurekPa * num + f.submergedDynamicPressurekPa * submergedPortion * f.submergedLiftScalar) * 1000.0;
        Qdrag = (f.dynamicPressurekPa * num + f.submergedDynamicPressurekPa * submergedPortion * f.submergedDragScalar) * 1000.0;
        SetupCoefficients(pointVelocity, frame.surface, nVel, liftVector, liftDot, absDot);
        baseLiftForce = GetLiftVector(liftVector, liftDot, absDot, Qlift, mach);
        const float oneMinusArea = S(E(1.0f) - E(ctrlSurfaceArea));
        liftForce = baseLiftForce * oneMinusArea;
        const bool useInternalDragModel = Get<bool>("useInternalDragModel");
        if (useInternalDragModel) dragForce = GetDragVector(nVel, absDot, Qdrag, mach) * oneMinusArea;
        float fdot = liftDot;
        float num3 = absDot;
        if (frame.hasCtrlSurface) {
            Set<float>("actuatorSpeed", originalActuatorSpeed);  // ApplyActuatorSpeedAdjustments: identity without adjusters
            CtrlSurfaceUpdate(frame);
            if (Get<bool>("alwaysRecomputeLift") || ctrlSurfaceLocalRotation != neutral) {
                airflowIncidence = UnityNative::AngleAxis(deflection, frame.surface.baseRotation * Vector3f::right());
                liftVector = airflowIncidence * liftVector;
                fdot = Vector3f::Dot(nVel, liftVector);
                num3 = Mathf::Abs(fdot);
            }
        }
        const bool num4 = Get<float>("liftScalar") > 0.0f;
        const bool flag = Get<float>("dragScalar") > 0.0f;
        liftForce += GetLiftVector(liftVector, fdot, num3, Qlift, mach) * ctrlSurfaceArea;
        out.applied = true;
        out.liftForce = liftForce;
        out.liftPosition = frame.surface.colWorld;
        if (useInternalDragModel) {
            dragForce += GetDragVector(nVel, num3, Qdrag, mach) * ctrlSurfaceArea;
            out.applyDrag = true;
            out.dragForce = dragForce;
            out.dragPosition = frame.surface.copWorld;
        }
        if (num4) Set<float>("liftScalar", liftForce.magnitude());
        if (flag) Set<float>("dragScalar", dragForce.magnitude());
        const float num5 = S(E(deflection) / E(Get<float>("ctrlSurfaceRange")));
        p->DragCubes.SetCubeWeight("neutral", S(E(maxAuthority) * E(0.01f) - E(Mathf::Abs(num5))));
        p->DragCubes.SetCubeWeight("fullDeflectionPos", Mathf::Clamp01(num5));
        p->DragCubes.SetCubeWeight("fullDeflectionNeg", Mathf::Clamp01(-num5));
    } else {
        Qdrag = Qlift = 0.0;
        nVel = Vector3f::zero();
    }
    return out;
}

// ModuleControlSurface.cs:880
void ModuleControlSurface::AssumeDragCubePosition(const std::string& name) {
    if (name == "fullDeflectionNeg") action = S(-E(maxAuthority) * E(0.01f));
    else if (name == "fullDeflectionPos") action = S(E(maxAuthority) * E(0.01f));
    else action = 0.0f;
    ctrlSurfaceLocalRotation =
        UnityNative::AngleAxis(S(E(Get<float>("ctrlSurfaceRange")) * E(action)), Vector3f::right()) * neutral;
}

// ---- ModuleAeroSurface ---------------------------------------------------------------------------------------------------

// ModuleAeroSurface.cs:32
void ModuleAeroSurface::OnAwake() {
    ModuleControlSurface::OnAwake();
    Set<bool>("deployInvert", Get<bool>("brakeDeployInvert"));
}

// ModuleAeroSurface.cs:43
void ModuleAeroSurface::ModifyAeroAuthorityLimiter() {
    Set<float>("aeroAuthorityLimiterUI",
               S(E(Get<float>("ctrlSurfaceRange")) * E(Get<float>("aeroAuthorityLimiter")) * E(0.01f)));
}

// ModuleAeroSurface.cs:51
void ModuleAeroSurface::ModifyAeroAuthorityLimiterUI() {
    Set<float>("aeroAuthorityLimiter",
               S(E(100.0f) * E(Get<float>("aeroAuthorityLimiterUI")) / E(Get<float>("ctrlSurfaceRange"))));
}

// ModuleAeroSurface.cs:56 (field GUI flags are the shell's)
void ModuleAeroSurface::OnStart(StartState state) {
    ModuleControlSurface::OnStart(state);
    const float ctrlSurfaceRange = Get<float>("ctrlSurfaceRange");
    if (std::isnan(Get<float>("aeroDeployAngle"))) Set<float>("aeroDeployAngle", ctrlSurfaceRange);
    if (std::isnan(aeroDeployAngleLimits.x)) aeroDeployAngleLimits.x = 0.0f;
    if (std::isnan(aeroDeployAngleLimits.y)) aeroDeployAngleLimits.y = S(E(ctrlSurfaceRange) * E(1.5f));
    Set<bool>("deployInvert", Get<bool>("brakeDeployInvert"));
    Set<bool>("ignoreRoll", true);
    Set<float>("authorityLimiter", 100.0f);
    ModifyAeroAuthorityLimiter();
}

// ModuleAeroSurface.cs:177
void ModuleAeroSurface::CtrlSurfaceUpdate(const ControlSurfaceFrame& frame) {
    const Vector3f up = frame.referenceUp;
    const Vector3f vector = frame.currentCoM - frame.basePosition;
    inputVector = frame.referenceRotation *
                  Vector3f(Get<bool>("ignorePitch") ? 0.0f : frame.pitch, 0.0f, Get<bool>("ignoreYaw") ? 0.0f : frame.yaw);
    const Vector3f rhs = Vector3f::Cross(Vector3f::Project(frame.surface.baseUp, -up), Vector3f::ProjectOnPlane(vector, up));
    const float ctrlSurfaceRange = Get<float>("ctrlSurfaceRange");
    action = S(E(Mathf::Clamp01(Vector3f::Dot(inputVector, rhs))) * E(ctrlSurfaceRange) * E(Get<float>("aeroAuthorityLimiter")) *
               E(0.01f));
    if (Get<bool>("deploy")) action = S(E(action) + E(Get<bool>("deployInvert") ? -1.0f : 1.0f) * E(Get<float>("aeroDeployAngle")));
    const float num = S(E(1.5f) * E(ctrlSurfaceRange));
    action = Mathf::Clamp(action, -num, num);
    const float uncasedTemp = Get<float>("uncasedTemp"), casedTemp = Get<float>("casedTemp");
    if (uncasedTemp > 0.0f && casedTemp > 0.0f) {
        Part* p = PartOf(*this);
        if (p == nullptr) throw NullReferenceException();
        p->fields.skinMaxTemp = (std::fabs(static_cast<double>(action)) > 1E-09) ? uncasedTemp : casedTemp;
    }
    const float step = S(E(Get<float>("actuatorSpeed")) * E(frame.fixedDeltaTime));
    deflection = Get<bool>("useExponentialSpeed") ? Mathf::Lerp(deflection, action, step) : Mathf::MoveTowards(deflection, action, step);
    ctrlSurfaceLocalRotation = UnityNative::AngleAxis(deflection, Vector3f::right()) * neutral;
}

// called by detail::RegisterBuiltinModules (src/modules/module_behaviours.cpp)
void RegisterAeroModules() {
    ModuleRegistrar<ModuleLiftingSurface> a("ModuleLiftingSurface");
    ModuleRegistrar<ModuleControlSurface> b("ModuleControlSurface");
    ModuleRegistrar<ModuleAeroSurface> c("ModuleAeroSurface");
}

}  // namespace ksp
