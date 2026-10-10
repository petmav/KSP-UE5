// WP-23: ModuleRCS.cs, ModuleRCSFX.cs (see ksp/modules/module_rcs.hpp for scope).
#include "ksp/modules/module_rcs.hpp"

#include <algorithm>
#include <limits>

#include "ksp/aero/float_eval.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/curves/float_curve.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/math/util_math.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/resources/resource_flow_host.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/vessel/flight_ctrl_state.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

using aero::D;
using aero::FE;
using aero::MonoFloat;
using aero::S;
using E = FE<MonoFloat>;

namespace {
const FloatCurve& EmptyCurve() {
    static const FloatCurve c;
    return c;
}
// OnAwake's `PartResourceLibrary.Instance.GetDefinition(name)`: the game always has a library; without one (a unit test that
// replaced it) the entry is null, as for an unknown resource
const PartResourceDefinition* LookupDefinition(const std::string& name) {
    const PartResourceLibrary* lib = PartResourceLibrary::Instance();
    return lib ? lib->GetDefinition(std::string_view(name)) : nullptr;
}
PartResourceLibrary& Library() {
    PartResourceLibrary* lib = PartResourceLibrary::Instance();
    if (lib == nullptr) throw NullReferenceException();
    return *lib;
}
}  // namespace

Part* ModuleRCS::PartPtr() const { return dynamic_cast<Part*>(part); }

const FloatCurve& ModuleRCS::Curve(std::string_view name) const {
    const ModuleValue* v = Value(name);
    const FloatCurve* c = v ? std::get_if<FloatCurve>(v) : nullptr;
    return c ? *c : EmptyCurve();
}

// Unity Instantiate: the public non-KSPField members (ModuleRCS.cs: thrustForces, G, rcs_active, flameout, maxFuelFlow, flowMult,
// ispMult, propellants, mixtureDensity, mixtureDensityRecip)
void ModuleRCS::CopySerializedFrom(const PartModule& from) {
    PartModule::CopySerializedFrom(from);
    if (const auto* r = dynamic_cast<const ModuleRCS*>(&from)) {
        thrustForces = r->thrustForces;
        G = r->G;
        rcs_active = r->rcs_active;
        flameout = r->flameout;
        maxFuelFlow = r->maxFuelFlow;
        flowMult = r->flowMult;
        ispMult = r->ispMult;
        propellants = r->propellants;
        mixtureDensity = r->mixtureDensity;
        mixtureDensityRecip = r->mixtureDensityRecip;
    }
}

// ModuleRCS.cs:377
void ModuleRCS::OnAwake() {
    PartModule::OnAwake();
    Events().Bind("ToggleToggles", BaseEvent::Handler([this] { ToggleToggles(); }));
    Actions().Bind("ToggleAction", [this](KSPActionParam& p) { ToggleAction(p); });
    thrusterTransforms.clear();
    if (IsNull("atmosphereCurve")) Set<FloatCurve>("atmosphereCurve", FloatCurve{});
    consumedResources.clear();
    for (size_t i = 0; i < propellants.size(); i++) consumedResources.push_back(LookupDefinition(propellants[i].name));
    // PropellantGauges / stackIcon.onStageIconDestroy: UI
    Part* p = PartPtr();
    if (p == nullptr) throw NullReferenceException();
    if (p->partInfo == nullptr || p->partInfo->partPrefab == nullptr) Set<bool>("stagingEnabled", false);
}

// ModuleRCS.cs:419 (the synthesised PROPELLANT goes into a copy of the node: PartLoader adds it to the prefab's own node)
void ModuleRCS::OnLoad(const ConfigNode& node) {
    if (!node.HasNode("PROPELLANT") && propellants.empty()) {
        ConfigNode::NodePtr copy = node.CreateCopy();
        ConfigNode* configNode = copy->AddNode("PROPELLANT");
        configNode->AddValue("name", Get<std::string>("resourceName"));
        configNode->AddValue("ratio", "1.0");
        configNode->AddValue("resourceFlowMode", Get<std::string>("resourceFlowMode"));
        SetupPropellant(*copy);
    } else {
        SetupPropellant(node);
    }
    maxFuelFlow = getMaxFuelFlow(Curve("atmosphereCurve").Evaluate(0.0f));
}

// ModuleRCS.cs:433 (stackIcon / SetupFX / UpdateToggles' guiName: UI and FX)
void ModuleRCS::OnStart(StartState) {
    Part* p = PartPtr();
    if (p == nullptr) throw NullReferenceException();
    if (findModelTransforms) {  // FindThrusters() :491
        thrusterTransforms = findModelTransforms(*p, Get<std::string>("thrusterTransformName"));
        if (thrustForces.size() != thrusterTransforms.size()) thrustForces.assign(thrusterTransforms.size(), 0.0f);
        if (thrusterTransforms.empty()) log::Info("RCS module unable to find any transforms in part named " + Get<std::string>("thrusterTransformName"));
    } else {
        if (thrustForces.size() != 0) thrustForces.assign(0, 0.0f);
        log::Info("RCS module unable to find any transforms in part named " + Get<std::string>("thrusterTransformName"));
    }
    const bool flag = Get<bool>("showToggles") && Get<bool>("currentShowToggles") && isEnabled;  // UpdateToggles :535
    for (const char* n : {"enableYaw", "enablePitch", "enableRoll", "enableX", "enableY", "enableZ", "useThrottle", "fullThrust"}) {
        BaseField* f = Fields()[n];
        f->guiActiveEditor = flag;
        f->guiActive = flag;
    }
    BaseEvent* e = Events()["ToggleToggles"];
    e->guiActive = e->guiActiveEditor = Get<bool>("showToggles") && isEnabled;
    Fields()["thrustCurveDisplay"]->guiActive = Get<bool>("useThrustCurve") && isEnabled;
    for (const char* n : {"rcsEnabled", "thrustPercentage"}) {
        BaseField* f = Fields()[n];
        f->guiActiveEditor = isEnabled;
        f->guiActive = isEnabled;
    }
    Actions()["ToggleAction"]->active = isEnabled;
}

// ModuleRCS.cs:468
void ModuleRCS::OnActive() {
    if (Get<bool>("stagingEnabled") && isEnabled) Set<bool>("rcsEnabled", true);
}

// ModuleRCS.cs:746
bool ModuleRCS::IsStageable() {
    if (!Get<bool>("stagingEnabled") && !Get<bool>("stagingToggleEnabledEditor")) return Get<bool>("stagingToggleEnabledFlight");
    return true;
}
// ModuleRCS.cs:755
std::string ModuleRCS::GetStagingEnableText() {
    const std::string t = Get<std::string>("stagingEnableText");
    if (!t.empty()) return t;
    return Localizer::Format("#autoLOC_218287");
}
// ModuleRCS.cs:764
std::string ModuleRCS::GetStagingDisableText() {
    const std::string t = Get<std::string>("stagingDisableText");
    if (!t.empty()) return t;
    return Localizer::Format("#autoLOC_218288");
}

// ModuleRCS.cs:199
void ModuleRCS::ToggleToggles() {
    Set<bool>("currentShowToggles", !Get<bool>("currentShowToggles"));
    // UpdateToggles(): PAW state (OnStart's block above)
}
// ModuleRCS.cs:206
void ModuleRCS::ToggleAction(KSPActionParam&) { Set<bool>("rcsEnabled", !Get<bool>("rcsEnabled")); }

// ModuleRCS.cs:485
void ModuleRCS::SetResource(const std::string& resource) {
    Set<std::string>("resourceName", resource);
    OnLoad(ConfigNode());
}

// ModuleRCS.cs:211
void ModuleRCS::Update(const FlightCtrlState& ctrl, const Quaternionf& referenceRotation, bool precisionMode) {
    const float num = S(E(Get<float>("EPSILON")) * E(Get<float>("EPSILON")));
    float num2 = ctrl.Z;
    if (S(E(num2) * E(num2)) < num) {
        if (Get<bool>("useThrottle")) {
            num2 = S(E(num2) - E(ctrl.mainThrottle));
            num2 = Mathf::Clamp(num2, -1.0f, 1.0f);
        } else {
            num2 = 0.0f;
        }
    }
    inputRot = Vector3f{Get<bool>("enablePitch") ? ctrl.pitch : 0.0f, Get<bool>("enableRoll") ? ctrl.roll : 0.0f, Get<bool>("enableYaw") ? ctrl.yaw : 0.0f};
    inputLin = Vector3f{Get<bool>("enableX") ? ctrl.X : 0.0f, Get<bool>("enableZ") ? num2 : 0.0f, Get<bool>("enableY") ? ctrl.Y : 0.0f};
    if (S(E(inputRot.x) * E(inputRot.x)) < num) inputRot.x = 0.0f;
    if (S(E(inputRot.y) * E(inputRot.y)) < num) inputRot.y = 0.0f;
    if (S(E(inputRot.z) * E(inputRot.z)) < num) inputRot.z = 0.0f;
    if (S(E(inputLin.x) * E(inputLin.x)) < num) inputLin.x = 0.0f;
    if (S(E(inputLin.z) * E(inputLin.z)) < num) inputLin.z = 0.0f;
    inputRot = aero::U<MonoFloat>::Rotate(referenceRotation, inputRot);
    inputLin = aero::U<MonoFloat>::Rotate(referenceRotation, inputLin);
    usePrecision = precisionMode;  // FlightInputHandler.fetch.precisionMode (when the handler exists)
}

// ModuleRCS.cs:264
float ModuleRCS::GetLeverDistanceOriginal(const Vector3f& thrusterPosition, const Vector3f& leverPivotPosition) const {
    return Vector3f::Distance(thrusterPosition, leverPivotPosition);
}

// ModuleRCS.cs:269
float ModuleRCS::GetLeverDistance(const RCSThruster& trf, const Vector3f& axis, const Vector3f& leverPivotPosition) const {
    const Vector3f direction = leverPivotPosition - trf.position;
    const float magnitude = aero::U<MonoFloat>::Magnitude(direction);
    const float num = aero::U<MonoFloat>::Dot(direction / magnitude, axis);
    return S(E(Mathf::Sqrt(S(E(1.0f) - E(num) * E(num)))) * E(magnitude));
}

// ModuleRCS.cs:277
void ModuleRCS::FixedUpdate(const RCSFlightInput& in) {
    isOperating = false;
    lastForces.clear();
    if (!in.flightScene) return;
    if (in.highWarp) {
        DeactivateFX();  // DeactivatePowerFX(): FX
        return;
    }
    const int tC = static_cast<int>(thrusterTransforms.size());
    if (static_cast<int>(thrustForces.size()) != tC) thrustForces.assign(static_cast<size_t>(tC), 0.0f);
    bool success = false;
    int num = tC;
    while (num-- > 0) thrustForces[static_cast<size_t>(num)] = 0.0f;
    totalThrustForce_ = 0.0f;
    const float realISP = Curve("atmosphereCurve").Evaluate(in.staticPressureAtm);
    Set<float>("realISP", realISP);
    exhaustVel_ = static_cast<double>(realISP) * G * ispMult;
    const float thrustForceRecip = S(E(1.0f) / E(Get<float>("thrusterPower")));
    (void)thrustForceRecip;  // only UpdatePowerFX reads it (FX)
    if (isEnabled && in.hasVessel && Get<bool>("rcsEnabled") && (!in.shieldedFromAirstream || Get<bool>("shieldedCanThrust"))) {
        const bool flag = in.rcsActionGroup;
        if (flag != rcs_active) rcs_active = flag;
        if (rcs_active && (inputRot != Vector3f::zero() || inputLin != Vector3f::zero())) {
            const Vector3f predictedCOM = in.currentCoM;
            for (int i = 0; i < tC; i++) {
                const RCSThruster& currentThruster = thrusterTransforms[static_cast<size_t>(i)];
                if (currentThruster.position == Vector3f::zero() || !currentThruster.activeInHierarchy) continue;
                const Vector3f vector = !Get<bool>("useZaxis") ? currentThruster.up : currentThruster.forward;
                const Vector3f rot = Vector3f::Cross(inputRot, Vector3f::ProjectOnPlane(currentThruster.position - predictedCOM, inputRot).normalized());
                currentThrustForce_ = Mathf::Max(Vector3f::Dot(vector, rot), 0.0f);
                currentThrustForce_ = S(E(currentThrustForce_) + E(Mathf::Max(Vector3f::Dot(vector, inputLin), 0.0f)));
                if (currentThrustForce_ > 1.0f) currentThrustForce_ = 1.0f;
                if (Get<bool>("fullThrust") && currentThrustForce_ >= Get<float>("fullThrustMin")) currentThrustForce_ = 1.0f;
                if (usePrecision) {
                    if (Get<bool>("useLever")) {
                        const float leverDistance = GetLeverDistance(currentThruster, -vector, predictedCOM);
                        if (leverDistance > 1.0f) currentThrustForce_ = S(E(currentThrustForce_) / E(leverDistance));
                    } else {
                        currentThrustForce_ = S(E(currentThrustForce_) * E(Get<float>("precisionFactor")));
                    }
                }
                UpdatePropellantStatus();
                currentThrustForce_ = CalculateThrust(currentThrustForce_, success, in.fixedDeltaTime);
                thrustForces[static_cast<size_t>(i)] = currentThrustForce_;
                const bool flag2 = currentThrustForce_ > 0.0f && success;
                isOperating |= flag2;
                // UpdatePowerFX(flag2, i, Mathf.Clamp(currentThrustForce * thrustForceRecip, 0.1f, 1f)): FX
                if (flag2 && !Get<bool>("isJustForShow")) {
                    totalThrustForce_ = S(E(totalThrustForce_) + E(currentThrustForce_));
                    RCSForce f;
                    f.force = (-vector) * currentThrustForce_;
                    f.position = currentThruster.position;
                    lastForces.push_back(f);
                }
            }
        } else {
            DeactivateFX();
        }
    } else {
        DeactivateFX();
    }
}

// ModuleRCS.cs:874
void ModuleRCS::DeactivateFX() {
    size_t count = thrusterTransforms.size();
    while (count-- > 0) thrustForces[count] = 0.0f;
}

// ModuleRCS.cs:566
float ModuleRCS::CalculateThrust(float totalForce, bool& success, float fixedDeltaTime) {
    if (!Get<bool>("isJustForShow") && Get<bool>("requiresFuel")) {
        if (totalForce > Get<float>("EPSILON")) {
            double num = flowMult;
            if (Get<bool>("useThrustCurve")) {
                float num2 = 1.0f;
                Set<float>("thrustCurveRatio", 1.0f);
                Set<float>("thrustCurveDisplay", num2);
                size_t count = propellants.size();
                while (count-- > 0) {
                    const Propellant& propellant = propellants[count];
                    if (!propellant.ignoreForThrustCurve) {
                        const float num3 = static_cast<float>(propellant.totalResourceAvailable() / propellant.totalResourceCapacity);
                        if (num3 < Get<float>("thrustCurveRatio")) Set<float>("thrustCurveRatio", num3);
                    }
                }
                Set<float>("thrustCurveDisplay", Curve("thrustCurve").Evaluate(Get<float>("thrustCurveRatio")));
                num *= static_cast<double>(Get<float>("thrustCurveDisplay"));
            }
            const double num4 = num * maxFuelFlow * static_cast<double>(totalForce) * (static_cast<double>(Get<float>("thrustPercentage")) * 0.01);
            double num5 = 1.0;
            if (!ResourceFlowEnvironment::InfinitePropellant) num5 = RequestPropellant(num4 * static_cast<double>(fixedDeltaTime));
            const float result = static_cast<float>(num4 * num5 * exhaustVel_);
            success = num5 > 0.0;
            flameout = !success;
            return result;
        }
        success = false;
        return 0.0f;
    }
    success = true;
    flameout = false;
    return 1.0f;
}

// ModuleRCS.cs:613 (`num += (double)(definition.density * propellant2.ratio)`: the float product widened to double)
void ModuleRCS::SetupPropellant(const ConfigNode& node) {
    if (node.HasNode("PROPELLANT")) propellants.clear();
    for (ConfigNode* configNode : node.nodes().GetNodes()) {
        if (configNode->name != "PROPELLANT") continue;
        if (!configNode->HasValue("name")) {
            log::Info("Propellant must have value 'name'");
            continue;
        }
        Propellant propellant;
        propellant.Load(*configNode);
        propellants.push_back(std::move(propellant));
    }
    mixtureDensity = 0.0;
    mixtureDensityRecip = 1.0;
    double num = 0.0;
    for (size_t j = 0; j < propellants.size(); j++) {
        Propellant& propellant2 = propellants[j];
        const PartResourceDefinition* definition = Library().GetDefinition(MonoStringHash(propellant2.name));
        if (definition == nullptr) {
            log::Error(propellant2.name + " not found in resource database. Propellant Setup has failed.");
            Part* p = PartPtr();
            if (p != nullptr && p->partInfo != nullptr) const_cast<AvailablePart*>(p->partInfo)->amountAvailable = 0;
            continue;
        }
        if (!propellant2.ignoreForIsp) num += D(E(definition->density()) * E(propellant2.ratio));
        propellant2.GetFlowMode();
    }
    mixtureDensity = num;
    if (mixtureDensity > 0.0) mixtureDensityRecip = 1.0 / mixtureDensity;
}

// ModuleRCS.cs:510
double ModuleRCS::getMaxFuelFlow(float Isp) {
    exhaustVel_ = static_cast<double>(Isp) * G;
    if (mixtureDensity > 0.0) return static_cast<double>(Get<float>("thrusterPower")) / exhaustVel_;
    return 0.0;
}

// ModuleRCS.cs:520
double ModuleRCS::getMaxFuelFlow(const Propellant& p, float Isp) {
    exhaustVel_ = static_cast<double>(Isp) * G;
    const double num = getMaxFuelFlow(Isp);
    if (num > 0.0) {
        if (Library().GetDefinition(p.id) != nullptr) return num * mixtureDensityRecip * static_cast<double>(p.ratio);
        return std::numeric_limits<double>::quiet_NaN();
    }
    return 0.0;
}

// ModuleRCS.cs:663
void ModuleRCS::UpdatePropellantStatus() {
    Part* part_ = PartPtr();
    size_t count = propellants.size();
    while (count-- > 0) {
        if (part_ == nullptr) throw NullReferenceException();
        propellants[count].UpdateConnectedResources(*part_);
    }
}

// ModuleRCS.cs:702
double ModuleRCS::RequestPropellant(double mass) {
    if (mixtureDensity <= 0.0) return 0.0;
    double result = 0.0;
    const double num = mass * mixtureDensityRecip;
    double num2 = 0.0;
    bool flag = false;
    const int count = static_cast<int>(propellants.size());
    int index = count;
    while (index-- > 0) {
        Propellant& p = propellants[static_cast<size_t>(index)];
        p.currentRequirement = num * static_cast<double>(p.ratio);
        if (p.totalResourceAvailable() >= p.currentRequirement) {
            p.isDeprived = false;
            continue;
        }
        if (p.totalResourceAvailable() >= p.currentRequirement * static_cast<double>(ignitionThreshold_)) {
            p.isDeprived = false;
            continue;
        }
        p.isDeprived = true;
        flag = true;
    }
    if (!flag) {
        Part* part_ = PartPtr();
        if (part_ == nullptr) throw NullReferenceException();
        int index2 = count;
        while (index2-- > 0) {
            Propellant& p = propellants[static_cast<size_t>(index2)];
            p.currentAmount = UtilMath::Clamp(
                part_->RequestResource(p.id, std::min(p.totalResourceAvailable(), p.currentRequirement), static_cast<ResourceFlowMode>(p.GetFlowMode())), 0.0,
                p.currentRequirement);
            num2 += p.currentAmount / p.currentRequirement;
        }
        result = num2 / static_cast<double>(count);
        result = UtilMath::Clamp(result, 0.0, 1.0);
    }
    return result;
}

// ModuleRCS.cs:773
void ModuleRCS::GetPotentialTorque(Vector3f& pos, Vector3f& neg, const Vector3f& currentCoM, bool precisionMode) {
    pos = neg = Vector3f::zero();
    const bool enablePitch = Get<bool>("enablePitch"), enableYaw = Get<bool>("enableYaw"), enableRoll = Get<bool>("enableRoll");
    if (!isEnabled || !Get<bool>("rcsEnabled") || !rcs_active || Get<bool>("isJustForShow") || flameout || (!enablePitch && !enableYaw && !enableRoll) ||
        (shieldedFromAirstream && !Get<bool>("shieldedCanThrust")))
        return;
    const Vector3f lhs{enablePitch ? 1.0f : 0.0f, enableRoll ? 1.0f : 0.0f, enableYaw ? 1.0f : 0.0f};
    const Vector3f predictedCOM = currentCoM;
    const int tC = static_cast<int>(thrusterTransforms.size());
    const float num = S(E(Get<float>("thrusterPower")) * (E(Get<float>("thrustPercentage")) * E(0.01f)));
    int index = tC;
    while (index-- > 0) {
        const RCSThruster& currentThruster = thrusterTransforms[static_cast<size_t>(index)];
        const Vector3f vector = currentThruster.position - predictedCOM;
        const Vector3f vector2 = Get<bool>("useZaxis") ? -currentThruster.forward : -currentThruster.up;
        float num2 = num;
        if (precisionMode) {
            if (Get<bool>("useLever")) {
                const float num3 = GetLeverDistance(currentThruster, vector2, predictedCOM);
                if (num3 > 1.0f) num2 = S(E(num2) / E(num3));
            } else {
                num2 = S(E(num2) * E(Get<float>("precisionFactor")));
            }
        }
        const Vector3f rot = Vector3f::Cross(lhs, Vector3f::ProjectOnPlane(vector, inputRot).normalized());
        currentThrustForce_ = Mathf::Min(1.0f, Mathf::Max(Vector3f::Dot(vector2, rot), 0.0f));
        const Vector3f vector3 = Vector3f::Cross(vector, vector2 * num2 * currentThrustForce_);
        currentThrustForce_ = Mathf::Min(1.0f, Mathf::Max(Vector3f::Dot(vector2, -rot), 0.0f));
        const Vector3f vector4 = -Vector3f::Cross(vector, vector2 * num2 * currentThrustForce_);
        if (vector3.x > 0.0f) pos.x = vector3.x;
        if (vector3.y > 0.0f) pos.y = vector3.y;
        if (vector3.z > 0.0f) pos.z = vector3.z;
        if (vector4.x > 0.0f) neg.x = vector4.x;
        if (vector4.y > 0.0f) neg.y = vector4.y;
        if (vector4.z > 0.0f) neg.z = vector4.z;
    }
}

namespace detail {
void RegisterRcsModules() {
    ModuleFactory::Register("ModuleRCS", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleRCS>(c)); });
    ModuleFactory::Register("ModuleRCSFX", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleRCSFX>(c)); });
}
}  // namespace detail

}  // namespace ksp
