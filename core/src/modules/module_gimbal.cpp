// WP-23: ModuleGimbal.cs (see ksp/modules/module_gimbal.hpp for scope).
#include "ksp/modules/module_gimbal.hpp"

#include "ksp/aero/float_eval.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/modules/module_engines.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/unity/quaternion_native.hpp"
#include "ksp/vessel/flight_ctrl_state.hpp"

namespace ksp {

using aero::FE;
using aero::MonoFloat;
using aero::S;
using E = FE<MonoFloat>;

Part* ModuleGimbal::PartPtr() const { return dynamic_cast<Part*>(part); }

// ModuleGimbal.cs:296
void ModuleGimbal::OnLoad(const ConfigNode&) {
    if (Get<float>("gimbalRangeXP") < 0.0f) Set<float>("gimbalRangeXP", Get<float>("gimbalRange"));
    if (Get<float>("gimbalRangeYP") < 0.0f) Set<float>("gimbalRangeYP", Get<float>("gimbalRange"));
    if (Get<float>("gimbalRangeXN") < 0.0f) Set<float>("gimbalRangeXN", Get<float>("gimbalRangeXP"));
    if (Get<float>("gimbalRangeYN") < 0.0f) Set<float>("gimbalRangeYN", Get<float>("gimbalRangeYP"));
}

// the [KSPEvent] / [KSPAction] delegates
void ModuleGimbal::OnAwake() {
    PartModule::OnAwake();
    Events().Bind("ToggleToggles", BaseEvent::Handler([this] { ToggleToggles(); }));
    Actions().Bind("ToggleAction", [this](KSPActionParam& p) { ToggleAction(p); });
    Actions().Bind("LockAction", [this](KSPActionParam& p) { LockAction(p); });
    Actions().Bind("FreeAction", [this](KSPActionParam& p) { FreeAction(p); });
    Actions().Bind("TogglePitchAction", [this](KSPActionParam& p) { TogglePitchAction(p); });
    Actions().Bind("ToggleYawAction", [this](KSPActionParam& p) { ToggleYawAction(p); });
    Actions().Bind("ToggleRollAction", [this](KSPActionParam& p) { ToggleRollAction(p); });
}

// ModuleGimbal.cs:112
void ModuleGimbal::ToggleToggles() {
    Set<bool>("currentShowToggles", !Get<bool>("currentShowToggles"));
    UpdateToggles();
}
// ModuleGimbal.cs:119
void ModuleGimbal::ToggleAction(KSPActionParam& param) {
    if (Get<bool>("gimbalLock")) FreeAction(param);
    else LockAction(param);
}
void ModuleGimbal::LockAction(KSPActionParam&) { Set<bool>("gimbalLock", true); }    // :118
void ModuleGimbal::FreeAction(KSPActionParam&) { Set<bool>("gimbalLock", false); }   // :124
void ModuleGimbal::TogglePitchAction(KSPActionParam&) { Set<bool>("enablePitch", !Get<bool>("enablePitch")); }  // :130
void ModuleGimbal::ToggleYawAction(KSPActionParam&) { Set<bool>("enableYaw", !Get<bool>("enableYaw")); }        // :136
void ModuleGimbal::ToggleRollAction(KSPActionParam&) { Set<bool>("enableRoll", !Get<bool>("enableRoll")); }     // :142

// ModuleGimbal.cs:161 FixedUpdate. The scene guards (HighLogic.LoadedSceneIsFlight, vessel / ReferenceTransform non-null) are the
// caller's: it passes the inputs only when they hold.
bool ModuleGimbal::FixedUpdate(const PartTransform& referenceTransform, const Vector3f& currentCoM, const FlightCtrlState& ctrlState,
                               float fixedDeltaTime) {
    if (!Get<bool>("gimbalActive") || !isEnabled) return false;
    if (gimbalTransforms.empty() && initRots.empty()) OnStart(StartState::Flying);  // `gimbalTransforms == null`
    const Vector3f localCoM = referenceTransform.InverseTransformPoint(currentCoM);
    const float num = S(E(Get<float>("gimbalResponseSpeed")) * E(fixedDeltaTime));
    newLocalRotations.assign(gimbalTransforms.size(), Quaternionf::identity());
    for (size_t i = 0; i < gimbalTransforms.size(); i++) {
        const PartTransform& t = gimbalTransforms[i];
        newLocalRotations[i] = initRots[i];  // t.localRotation = initRots[i]
        if (!Get<bool>("gimbalLock")) {
            actuation.x = Get<bool>("enablePitch") ? ctrlState.pitch : 0.0f;
            actuation.y = Get<bool>("enableRoll") ? ctrlState.roll : 0.0f;
            actuation.z = Get<bool>("enableYaw") ? ctrlState.yaw : 0.0f;
            actuation = ApplyControlAdjustments(actuation);
            actuationLocal = GimbalRotation(referenceTransform, t, actuation, localCoM);
            if (Get<bool>("useGimbalResponseSpeed")) {
                actuationLocal.x = Mathf::Lerp(oldActuationLocal[i].x, actuationLocal.x, num);
                actuationLocal.y = Mathf::Lerp(oldActuationLocal[i].y, actuationLocal.y, num);
                oldActuationLocal[i] = actuationLocal;
            }
            // initRots[i] * Quaternion.AngleAxis(actuationLocal.x, xMult * Vector3.right) * Quaternion.AngleAxis(actuationLocal.y, yMult * (flipYZ ? Vector3.forward : Vector3.up))
            const Vector3f axisX = Vector3f::right() * Get<float>("xMult");
            const Vector3f axisY = (Get<bool>("flipYZ") ? Vector3f::forward() : Vector3f::up()) * Get<float>("yMult");
            const Quaternionf qx = UnityNative::AngleAxis(actuationLocal.x, axisX);
            const Quaternionf qy = UnityNative::AngleAxis(actuationLocal.y, axisY);
            newLocalRotations[i] = aero::U<MonoFloat>::Mul(aero::U<MonoFloat>::Mul(initRots[i], qx), qy);
        }
    }
    return true;
}

// ModuleGimbal.cs:196
Vector3f ModuleGimbal::GimbalRotation(const PartTransform& referenceTransform, const PartTransform& t, Vector3f PYR,
                                      const Vector3f& localCoM) const {
    const Vector3f vector = referenceTransform.InverseTransformPoint(t.position);
    float num = 1.0f;
    if (localCoM.y < vector.y) num = -1.0f;
    PYR.x = S(E(PYR.x) * E(num));
    PYR.z = S(E(PYR.z) * E(num));
    const float minRollOffset = Get<float>("minRollOffset");
    if (PYR.y != 0.0f && Get<bool>("enableRoll")) {
        if (vector.x > minRollOffset) PYR.x = S(E(PYR.x) + E(PYR.y));
        else if (vector.x < S(E(0.0f) - E(minRollOffset))) PYR.x = S(E(PYR.x) - E(PYR.y));
        if (vector.z > minRollOffset) PYR.z = S(E(PYR.z) + E(PYR.y));
        else if (vector.z < S(E(0.0f) - E(minRollOffset))) PYR.z = S(E(PYR.z) - E(PYR.y));
    }
    Vector3f result = t.InverseTransformDirection(referenceTransform.TransformDirection(PYR));
    const float limiter = Get<float>("gimbalLimiter");
    result.x = S(E(Mathf::Clamp(result.x, -1.0f, 1.0f)) * E(result.x > 0.0f ? Get<float>("gimbalRangeXP") : Get<float>("gimbalRangeXN")) *
                 E(limiter) * E(0.01f));
    result.y = S(E(Mathf::Clamp(result.y, -1.0f, 1.0f)) * E(result.y > 0.0f ? Get<float>("gimbalRangeYP") : Get<float>("gimbalRangeYN")) *
                 E(limiter) * E(0.01f));
    return result;
}

// ModuleGimbal.cs:231
void ModuleGimbal::UpdateToggles() {
    const bool flag = Get<bool>("showToggles") && Get<bool>("currentShowToggles") && isEnabled;
    for (const char* n : {"enableYaw", "enablePitch", "enableRoll"}) {
        BaseField* f = Fields()[n];
        f->guiActiveEditor = flag;
        f->guiActive = flag;
    }
    BaseEvent* e = Events()["ToggleToggles"];
    e->guiActive = e->guiActiveEditor = Get<bool>("showToggles") && isEnabled;
    // base.Events["ToggleToggles"].guiName = currentShowToggles ? #autoLOC_221352 : #autoLOC_7000023 (PAW text)
}

// ModuleGimbal.cs:279
void ModuleGimbal::UpdatePAWUI() {
    for (const char* n : {"gimbalLock", "gimbalLimiter"}) {
        BaseField* f = Fields()[n];
        f->guiActiveEditor = isEnabled;
        f->guiActive = isEnabled;
    }
    for (const char* n : {"ToggleAction", "LockAction", "FreeAction", "TogglePitchAction", "ToggleYawAction", "ToggleRollAction"})
        Actions()[n]->active = isEnabled;
    UpdateToggles();
}

// ModuleGimbal.cs:247 (SetActionsSuffix's guiName rewrites are PAW text)
void ModuleGimbal::OnStart(StartState) {
    gimbalTransforms.clear();
    initRots.clear();
    if (findModelTransforms) {
        if (Part* p = PartPtr()) {
            for (auto& pr : findModelTransforms(*p, Get<std::string>("gimbalTransformName"))) {
                gimbalTransforms.push_back(pr.first);
                initRots.push_back(pr.second);
            }
        }
    }
    oldActuationLocal.assign(gimbalTransforms.size(), Vector3f::zero());
    UpdatePAWUI();
}

// ModuleGimbal.cs:331
void ModuleGimbal::OnActive() { Set<bool>("gimbalActive", true); }

// ModuleGimbal.cs:336
void ModuleGimbal::CreateEngineList() {
    engineMultsList.clear();
    engineMultsListCreated = true;
    Part* p = PartPtr();
    if (p == nullptr) throw NullReferenceException();
    for (size_t i = 0; i < gimbalTransforms.size(); i++) {
        const PartTransform& parent = gimbalTransforms[i];
        std::vector<std::pair<ModuleEngines*, float>> list;
        size_t count2 = p->Modules().Count();
        while (count2-- > 0) {
            auto* moduleEngines = dynamic_cast<ModuleEngines*>(p->Modules().GetModule(count2));
            if (moduleEngines == nullptr) continue;
            std::pair<ModuleEngines*, float> item{moduleEngines, 0.0f};
            size_t count3 = moduleEngines->thrustTransforms.size();
            while (count3-- > 0) {
                const PartTransform& find = moduleEngines->thrustTransforms[count3];
                const bool inChildren = isThrustTransformInChildren ? isThrustTransformInChildren(*p, *this, i, *moduleEngines, count3)
                                        : isTransformInChildren       ? isTransformInChildren(parent, find)
                                                                      : (parent.position == find.position && parent.rotation == find.rotation);
                if (inChildren) item.second = S(E(item.second) + E(moduleEngines->thrustTransformMultipliers[count3]));
            }
            list.push_back(item);
        }
        engineMultsList.push_back(std::move(list));
    }
}

// ModuleGimbal.cs:374
void ModuleGimbal::GetPotentialTorque(Vector3f& pos, Vector3f& neg, const PartTransform& referenceTransform, const Vector3f& currentCoM) {
    pos = neg = Vector3f::zero();
    if (Get<bool>("gimbalLock") || !isEnabled) return;
    if (!engineMultsListCreated) CreateEngineList();
    const Vector3f localCoM = referenceTransform.InverseTransformPoint(currentCoM);
    const float minRollOffset = Get<float>("minRollOffset");
    const float deg2rad = 0.0174532924f;  // (float)Math.PI / 180f
    auto sinSum = [&](const Vector3f& v) {
        return S(E(Mathf::Sin(S(E(Mathf::Abs(v.x)) * E(deg2rad)))) + E(Mathf::Sin(S(E(Mathf::Abs(v.y)) * E(deg2rad)))));
    };
    size_t count = gimbalTransforms.size();
    while (count-- > 0) {
        const PartTransform& transform = gimbalTransforms[count];
        const Vector3f vector = transform.position - currentCoM;
        const float magnitude = aero::U<MonoFloat>::Magnitude(vector);
        // Vector3.ProjectOnPlane(vector, vessel.ReferenceTransform.up).magnitude
        const float magnitude2 = aero::U<MonoFloat>::Magnitude(aero::U<MonoFloat>::ProjectOnPlane(vector, referenceTransform.up()));
        size_t count2 = engineMultsList[count].size();
        while (count2-- > 0) {
            const auto& kv = engineMultsList[count][count2];
            const float num = S(E(kv.second) * E(kv.first->finalThrust()));
            if (num > 0.0f) {
                const float num2 = S(E(magnitude) * E(num));
                float num3 = sinSum(GimbalRotation(referenceTransform, transform, Vector3f::right(), localCoM));
                pos.x = S(E(pos.x) + E(num3) * E(num2));
                num3 = sinSum(GimbalRotation(referenceTransform, transform, -Vector3f::right(), localCoM));
                neg.x = S(E(neg.x) + E(num3) * E(num2));
                num3 = sinSum(GimbalRotation(referenceTransform, transform, Vector3f::forward(), localCoM));
                pos.z = S(E(pos.z) + E(num3) * E(num2));
                num3 = sinSum(GimbalRotation(referenceTransform, transform, -Vector3f::forward(), localCoM));
                neg.z = S(E(neg.z) + E(num3) * E(num2));
                if (magnitude2 > minRollOffset) {
                    const float num4 = S(E(magnitude2) * E(num));
                    num3 = sinSum(GimbalRotation(referenceTransform, transform, Vector3f::up(), localCoM));
                    pos.y = S(E(pos.y) + E(num3) * E(num4));
                    num3 = sinSum(GimbalRotation(referenceTransform, transform, -Vector3f::up(), localCoM));
                    neg.y = S(E(neg.y) + E(num3) * E(num4));
                }
            }
        }
    }
}

namespace detail {
void RegisterGimbalModule() {
    ModuleFactory::Register("ModuleGimbal", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleGimbal>(c)); });
}
}  // namespace detail

}  // namespace ksp
