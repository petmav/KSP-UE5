// WP-27: ModuleParachute and ModuleEvaChute. See ksp/modules/parachutes.hpp.
// Every function cites File.cs:line of decompiled/KSP-Decompiled/Assembly-CSharp (tools/wp28/check_cites.py verifies the citations).
#include "ksp/modules/parachutes.hpp"

#include "ksp/modules/cs_expr.hpp"

#include <algorithm>
#include <stdexcept>

#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/math/util_math.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/parts/part_module_data.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

using GE = events::GameEvents;

namespace {
NullReferenceException Nre() { return NullReferenceException("Object reference not set to an instance of an object"); }
}  // namespace

void ParachuteEnvironment::Reset() {
    velocity = nullptr;
    staticPressure = nullptr;
    altitudeAtPos = nullptr;
    raycastDown = nullptr;
    orientCanopy = nullptr;
    screenMessages.clear();
    flightLog.clear();
}

ModuleParachute::~ModuleParachute() { OnDestroy(); }

Part* ModuleParachute::PartPtr() const { return dynamic_cast<Part*>(part); }
const ProcessingVessel* ModuleParachute::VesselRecord() const { return ResourceProcessingEnvironment::VesselOf(PartPtr()); }
bool ModuleParachute::ShieldedFromAirstream() const {
    const Part* p = PartPtr();
    return p != nullptr && p->ShieldedFromAirstream();
}
DragCubeList* ModuleParachute::Cubes() const { return part != nullptr ? &part->DragCubes : nullptr; }
std::string ModuleParachute::Str(std::string_view field) const { return IsNull(field) ? std::string() : Get<std::string>(field); }

// ModuleParachute.cs:193
void ModuleParachute::DeployAction(KSPActionParam&) { Deploy(); }

// ModuleParachute.cs:199
void ModuleParachute::CutAction(KSPActionParam&) { CutParachute(); }

// ModuleParachute.cs:205
void ModuleParachute::Deploy() {
    if (deploymentState != deploymentStates::STOWED) return;
    if (ShieldedFromAirstream() && !Get<bool>("shieldedCanDeploy")) {
        const Part* p = PartPtr();
        ParachuteEnvironment::screenMessages.push_back(
            Localizer::Format("#autoLOC_214578", std::vector<std::string>{p != nullptr && p->partInfo != nullptr ? p->partInfo->title : std::string()}));
        return;
    }
    deploymentState = deploymentStates::ACTIVE;
    Set<std::string>("persistentState", "ACTIVE");
    SetIcon("LightCyan");
    Events()["Deploy"]->active = false;
    Events()["CutParachute"]->active = false;
    Events()["Disarm"]->active = true;
    // `this is ModuleEvaChute`: AnalyticsUtil.LogEVAChuteDeployed (analytics: not modelled)
}

// ModuleParachute.cs:229
void ModuleParachute::CutParachute() {
    if (deploymentState != deploymentStates::CUT && deploymentState != deploymentStates::ACTIVE) {
        deploymentState = deploymentStates::CUT;
        Set<std::string>("persistentState", "CUT");
        part->fields.maximum_drag = f32store(fx(baseDrag) + fx(Get<float>("stowedDrag")));
        SetDragCubes_Stowed();
        SetIcon("Red");
        Events()["CutParachute"]->active = false;
        Events()["Repack"]->active = true;
        Events()["Disarm"]->active = false;
        if (!hasCanopy) throw Nre();
        canopyActive = false;
    }
}

// ModuleParachute.cs:246 (Kerbal experience gate: the active vessel's RepairSkill is the host's `repairSkillOk`)
void ModuleParachute::Repack() {
    if (deploymentState != deploymentStates::CUT) return;
    if (hasCap) capActive = true;
    if (!hasCanopy) throw Nre();
    canopyActive = false;
    deploymentState = deploymentStates::STOWED;
    Set<std::string>("persistentState", "STOWED");
    // part.deactivate() (Part.cs): the staging state of the part is WP-16's
    if (deactivateOnRepack) deactivateCalls++;
    part->fields.maximum_drag = f32store(fx(baseDrag) + fx(Get<float>("stowedDrag")));
    SetDragCubes_Stowed();
    SetIcon("White");
    Events()["Deploy"]->active = true;
    Events()["CutParachute"]->active = false;
    Events()["Repack"]->active = false;
    Events()["Disarm"]->active = false;
}

// ModuleParachute.cs:278
void ModuleParachute::Disarm() {
    if (deploymentState == deploymentStates::ACTIVE) {
        deploymentState = deploymentStates::STOWED;
        Set<std::string>("persistentState", "STOWED");
        deactivateCalls++;  // part.deactivate()
        SetIcon("White");
        Events()["Deploy"]->active = true;
        Events()["CutParachute"]->active = false;
        Events()["Repack"]->active = false;
        Events()["Disarm"]->active = false;
    }
}

// ModuleParachute.cs:293 (`part.fxGroups.Add(deployFx)`: the FXGroup is a name on Part::fxGroupNames)
void ModuleParachute::OnLoad(const ConfigNode&) {
    if (Part* p = PartPtr()) p->fxGroupNames.push_back("deploy");
    SetEnumFromPersistentState();
    SetDragCubes_Stowed();
}

void ModuleParachute::SetEnumFromPersistentState() {
    // (deploymentStates)Enum.Parse(typeof(deploymentStates), persistentState)
    static const char* const names[] = {"STOWED", "ACTIVE", "SEMIDEPLOYED", "DEPLOYED", "CUT"};
    const std::string ps = Str("persistentState");
    for (int i = 0; i < 5; i++)
        if (ps == names[i]) {
            deploymentState = static_cast<deploymentStates>(i);
            return;
        }
    throw std::invalid_argument("Requested value '" + ps + "' was not found.");
}

// ModuleParachute.cs:301
void ModuleParachute::OnAwake() {
    PartModule::OnAwake();
    Set<double>("chuteTemp", Get<double>("startingTemp"));
    Events().Bind("Deploy", BaseEvent::Handler([this] { Deploy(); }));
    Events().Bind("CutParachute", BaseEvent::Handler([this] { CutParachute(); }));
    Events().Bind("Repack", BaseEvent::Handler([this] { Repack(); }));
    Events().Bind("Disarm", BaseEvent::Handler([this] { Disarm(); }));
    Actions().Bind("DeployAction", [this](KSPActionParam& p) { DeployAction(p); });
    Actions().Bind("CutAction", [this](KSPActionParam& p) { CutAction(p); });
}

// ModuleParachute.cs:308
void ModuleParachute::OnStart(StartState state) {
    if (Part* p = PartPtr()) {
        if (p->fields.stagingIcon && p->fields.stagingIcon->empty() && Get<bool>("overrideStagingIconIfBlank")) p->fields.stagingIcon = std::string("PARACHUTES");
    }
    hasCanopy = DeployablesEnvironment::hasModelTransform && DeployablesEnvironment::hasModelTransform(*this, Str("canopyName"));
    hasCap = DeployablesEnvironment::hasModelTransform && DeployablesEnvironment::hasModelTransform(*this, Str("capName"));
    baseDrag = part->fields.maximum_drag;
    lerpTime = 0.0f;
    Events()["Repack"]->active = false;
    // ((UI_FloatRange)Fields["minAirPressureToOpen"].uiControlEditor).minValue = clampMinAirPressure
    if (BaseField* f = Fields()["minAirPressureToOpen"]) {
        if (f->uiControlEditor)
            for (auto& kv : f->uiControlEditor->params)
                if (kv.first == "minValue") {
                    kv.second.kind = cs::CsValue::Kind::Float;
                    kv.second.f = Get<float>("clampMinAirPressure");
                }
    }
    SetupAnimation();
    if (Anim) Anim->wrapMode = WrapMode::Once;
    if (hasCap) capActive = true;
    if (hasCanopy) canopyActive = false;
    if (state != StartState::Editor) {
        const ProcessingVessel* v = VesselRecord();
        if (v != nullptr && PartPtr()->vessel != nullptr) {
            hasCurrentVessel = true;
            currentVessel = PartPtr()->vessel;
        }
        SetDragCubes_Stowed();
        SetUIEVents();
        if (Part* p = PartPtr()) p->fields.ActivatesEvenIfDisconnected = true;
        if (!hasCanopy) throw Nre();  // lastRot = canopy.rotation
        if (!subscribedVesselModified) {
            GE::onVesselWasModified.Add(this, &ModuleParachute::OnVesselWasModified);
            subscribedVesselModified = true;
        }
        UpdateSymmetry(PartPtr() != nullptr ? PartPtr()->vessel : nullptr);
    }
}

// ModuleParachute.cs:349 SetUIEVents
void ModuleParachute::SetUIEVents() {
    Events()["Deploy"]->active = false;
    Events()["CutParachute"]->active = false;
    Events()["Repack"]->active = false;
    Events()["Disarm"]->active = false;
    if (deploymentState == deploymentStates::STOWED) {
        part->fields.maximum_drag = f32store(fx(baseDrag) + fx(Get<float>("stowedDrag")));
        Events()["Deploy"]->active = true;
        return;
    }
    if (deploymentState == deploymentStates::ACTIVE) {
        SetIcon("LightCyan");
        Events()["Disarm"]->active = true;
        return;
    }
    if (deploymentState != deploymentStates::SEMIDEPLOYED && deploymentState != deploymentStates::DEPLOYED) {
        if (deploymentState == deploymentStates::CUT) {
            Events()["Repack"]->active = true;
            if (hasCap) capActive = false;
            SetIcon("Red");
        }
        return;
    }
    if (hasCanopy) canopyActive = true;
    if (hasCap) capActive = false;
    Events()["CutParachute"]->active = true;
    if (deploymentState == deploymentStates::SEMIDEPLOYED) {
        AnimationState* s = (*Anim)[Str("semiDeployedAnimation")];
        if (s == nullptr) throw Nre();
        s->speed = 1.0f;
        s->set_normalizedTime(Get<float>("animTime"));
        s->enabled = true;
        Anim->Play(Str("semiDeployedAnimation"));
        SetIcon("BrightYellow");
        SetDragCube_SemiDeployed(Mathf::Pow(Get<float>("animTime"), Get<float>("deploymentCurve")));
    } else if (deploymentState == deploymentStates::DEPLOYED) {
        AnimationState* s = (*Anim)[Str("fullyDeployedAnimation")];
        if (s == nullptr) throw Nre();
        s->speed = 1.0f;
        s->set_normalizedTime(Get<float>("animTime"));
        s->enabled = true;
        Anim->Play(Str("fullyDeployedAnimation"));
        SetIcon("RadioactiveGreen");
        SetDragCube_Deployed(Mathf::Pow(Get<float>("animTime"), Get<float>("deploymentCurve")));
    }
}

// ModuleParachute.cs:409
void ModuleParachute::OnDestroy() {
    if (subscribedVesselModified) {
        GE::onVesselWasModified.Remove(this, &ModuleParachute::OnVesselWasModified);
        subscribedVesselModified = false;
    }
}

// ModuleParachute.cs:414
void ModuleParachute::SetupAnimation() {
    if (Anim) return;
    std::vector<std::shared_ptr<Animation>> comps;
    if (DeployablesEnvironment::modelAnimations) comps = DeployablesEnvironment::modelAnimations(*this);
    if (comps.size() == 1) {
        Anim = comps[0];
    } else if (comps.size() > 1) {
        for (const auto& c : comps) {
            if ((*c)[Str("semiDeployedAnimation")] != nullptr && (*c)[Str("fullyDeployedAnimation")] != nullptr) {
                Anim = c;
                break;
            }
        }
        if (!Anim)
            log::Error("[ModuleParachute]: " + std::to_string(comps.size()) + " Animation components found in child objects, but none contain states " +
                       Str("semiDeployedAnimation") + " and " + Str("fullyDeployedAnimation"));
    } else {
        log::Error("[ModuleParachute]: No animation components found in child objects");
    }
}

// ModuleParachute.cs:447
void ModuleParachute::OnActive() {
    if (Get<bool>("stagingEnabled")) Deploy();
}

// ModuleParachute.cs:455 GetArea (Mono rule: dotNormalized / num2 / num3 / num are float locals)
double ModuleParachute::GetArea(const DragCube& cube) const {
    const Vector3f faceDirection = DragCubeList::GetFaceDirection(DragCube::DragFace::YN);
    float num = 0.0f;
    for (int i = 0; i < 6; i++) {
        const Vector3f rhs = DragCubeList::FaceDirections()[static_cast<size_t>(i)];
        const float dotNormalized = f32store((fx(Vector3f::Dot(faceDirection, rhs)) + 1.0) * 0.5);
        const float num2 = PhysicsGlobals::DragCurveValue(PhysicsGlobals::SurfaceCurves, dotNormalized, 0.0f);
        const float num3 = f32store(fx(cube.Area()[static_cast<size_t>(i)]) * fx(num2));
        num = f32store(fx(num) + fx(num3) * fx(cube.Drag()[static_cast<size_t>(i)]));
    }
    return static_cast<double>(num) * static_cast<double>(PhysicsGlobals::DragCubeMultiplier()) * static_cast<double>(PhysicsGlobals::DragMultiplier());
}

// ModuleParachute.cs:470
void ModuleParachute::CalcBaseStats() {
    if (part == nullptr || part->DragCubes.None()) return;
    const auto& cubes = part->DragCubes.Cubes();
    for (const DragCube& cube : cubes) {
        if (cube.Name() == "DEPLOYED") areaDeployed = GetArea(cube);
        else if (cube.Name() == "SEMIDEPLOYED") areaSemi = GetArea(cube);
    }
    invThermalMass = 1.0 / (areaDeployed * Get<double>("chuteThermalMassPerArea"));
    double num = 0.0;
    int num2 = 401;
    double num3 = 100.0;
    bool flag = true;
    const double num4 = 1.0 / Get<double>("refSpeedOfSound");
    const double num5 = Get<double>("chuteMaxTemp") * Get<double>("safeMult");
    SetConvectiveStats(Get<double>("refDensity"), num * PhysicsGlobals::NewtonianTemperatureFactor(), num * num4, 0.0);
    while (!(dotnet::Math::Abs(shockTemp - num5) <= 0.1) && --num2 >= 0) {
        if (shockTemp > num5) {
            if (flag) num3 *= 0.5;
            flag = false;
            num -= num3;
            if (num < 0.0) num = 0.0;
        } else {
            if (!flag) num3 *= 0.5;
            flag = true;
            num += num3;
        }
        SetConvectiveStats(Get<double>("refDensity"), num * PhysicsGlobals::NewtonianTemperatureFactor(), num * num4, 0.0);
    }
    maxSafeSpeedAtRef = dotnet::Math::Round(num);
}

// ModuleParachute.cs:526 (GetInfo: UI text, deferred; CalcBaseStats above is the numeric part)
std::string ModuleParachute::GetInfo() {
    CalcBaseStats();
    return "";
}

// ModuleParachute.cs:546
void ModuleParachute::FixedUpdate() {
    deployfieldGuiActive = false;
    if (BaseField* f = Fields()["deploySafe"]) f->guiActive = false;
    Set<std::string>("deploySafe", "");
    const ProcessingVessel* vessel = VesselRecord();
    Part* p = PartPtr();
    if (p == nullptr) throw Nre();
    if (!p->fields.packed) {
        if (vessel != nullptr) SetConvectiveStats(vessel->atmDensity, vessel->externalTemperature, vessel->mach, vessel->convectiveCoefficient);
        else SetConvectiveStats(0.0, 4.0, 0.0, 0.0);
        deploymentSafeState = deploymentSafeStates::NONE;
        if (vessel != nullptr && vessel->atmDensity > 0.0 && (deploymentState == deploymentStates::STOWED || deploymentState == deploymentStates::ACTIVE)) {
            deployfieldGuiActive = true;
            if (BaseField* f = Fields()["deploySafe"]) f->guiActive = true;
            Set<std::string>("deploySafe", DeploySafe(deploymentSafeState));
        }
        switch (deploymentSafeState) {
            default: iconBackground = "White"; break;
            case deploymentSafeStates::UNSAFE: iconBackground = "Red"; break;
            case deploymentSafeStates::RISKY: iconBackground = "KSPNotSoGoodOrange"; break;
        }
        if (deploymentState != deploymentStates::STOWED) {
            bool flag = false;
            if (deploymentState != deploymentStates::CUT && deploymentState != deploymentStates::DEPLOYED) flag = ShouldDeploy();
            switch (deploymentState) {
                case deploymentStates::ACTIVE: {
                    Set<float>("animTime", 0.0f);
                    const double pressure = ParachuteEnvironment::staticPressure ? ParachuteEnvironment::staticPressure(*p) : 0.0;
                    if ((pressure * pressureRecip > static_cast<double>(Get<float>("minAirPressureToOpen")) || flag) && IsMovingFastEnoughToDeploy() &&
                        Get<int32_t>("automateSafeDeploy") >= static_cast<int>(deploymentSafeState) && PassedAdditionalDeploymentChecks()) {
                        if (hasCap) capActive = false;
                        if (!hasCanopy) throw Nre();
                        canopyActive = true;
                        deploymentState = deploymentStates::SEMIDEPLOYED;
                        Set<std::string>("persistentState", "SEMIDEPLOYED");
                        Events()["CutParachute"]->active = true;
                        Events()["Disarm"]->active = false;
                        SetIcon("BrightYellow");
                        lerpTime = 0.0f;
                        deployFxBursts++;
                        PlayDeployAnimation(Str("semiDeployedAnimation"), Get<float>("semiDeploymentSpeed"));
                        OnParachuteSemiDeployed();
                    }
                    break;
                }
                case deploymentStates::SEMIDEPLOYED:
                    if (IsAnimationPlaying()) {
                        const float t = GetAnimationNormalizedTime(Str("semiDeployedAnimation"));
                        Set<float>("animTime", t);
                        lerpTime = t;
                        lerpTime = Mathf::Pow(lerpTime, Get<float>("deploymentCurve"));
                        p->fields.maximum_drag = f32store(fx(baseDrag) + fx(Mathf::Lerp(Get<float>("stowedDrag"), Get<float>("semiDeployedDrag"), lerpTime)));
                        SetDragCube_SemiDeployed(lerpTime);
                    } else {
                        if (lerpTime < 1.0f) SetDragCube_SemiDeployed(1.0f);
                        lerpTime = 1.0f;
                        Set<float>("animTime", 1.0f);
                        if (flag) {
                            SetIcon("RadioactiveGreen");
                            deployFxBursts++;
                            deploymentState = deploymentStates::DEPLOYED;
                            Set<std::string>("persistentState", "DEPLOYED");
                            lerpTime = 0.0f;
                            Events()["CutParachute"]->active = true;
                            Events()["Disarm"]->active = false;
                            Set<float>("animTime", 0.0f);
                            PlayDeployAnimation(Str("fullyDeployedAnimation"), Get<float>("deploymentSpeed"));
                            OnParachuteFullyDeployed();
                        }
                    }
                    UpdateCut();
                    break;
                case deploymentStates::DEPLOYED:
                    if (IsAnimationPlaying()) {
                        const float t = GetAnimationNormalizedTime(Str("fullyDeployedAnimation"));
                        Set<float>("animTime", t);
                        lerpTime = t;
                        lerpTime = Mathf::Pow(lerpTime, Get<float>("deploymentCurve"));
                        p->fields.maximum_drag = f32store(fx(baseDrag) + fx(Mathf::Lerp(Get<float>("semiDeployedDrag"), Get<float>("fullyDeployedDrag"), lerpTime)));
                        SetDragCube_Deployed(lerpTime);
                    } else {
                        if (lerpTime < 1.0f) SetDragCube_Deployed(1.0f);
                        lerpTime = 1.0f;
                        Set<float>("animTime", 1.0f);
                    }
                    UpdateCut();
                    break;
                default: break;
            }
            if (deploymentState != deploymentStates::SEMIDEPLOYED && deploymentState != deploymentStates::DEPLOYED) {
                part->DragCubes.SetDragVectorRotation(false);
                // lastRot = LookRotation(...) / canopy.rotation: the canopy transform is the engine's
                return;
            }
            if (p->fields.dragVectorDir != Vector3f{0.0f, 0.0f, 0.0f}) {
                if (ParachuteEnvironment::orientCanopy) {
                    ParachuteEnvironment::CanopyInput in;
                    in.rotateParachute = !dontRotateParachute;
                    in.dragVectorNonZero = true;
                    float num2 = 0.0f;
                    if (symmetryCount > 0 && Get<float>("spreadAngle") > 0.0f) {
                        num2 = Mathf::Clamp(f32store(fx(Get<float>("spreadAngle")) * fx(static_cast<float>(symmetryCount))), 0.0f, 45.0f);
                        if (deploymentState == deploymentStates::SEMIDEPLOYED) num2 = f32store(fx(num2) / 3.0);
                        else if (IsAnimationPlaying()) num2 = f32store(fx(num2) / (3.0 - 2.0 * fx((*Anim)[Str("fullyDeployedAnimation")]->normalizedTime())));
                    }
                    in.spreadDegrees = num2;
                    in.rotationStepDegrees = f32store(fx(rotationSpeedDPS) * fx(TimeWarp::fixedDeltaTime()));
                    if (auto rot = ParachuteEnvironment::orientCanopy(*this, in)) part->DragCubes.SetDragVectorRotation(*rot);
                }
            }
        } else {
            Set<float>("animTime", 0.0f);
        }
    } else {
        if (DeployablesEnvironment::SceneIsFlight() && vessel != nullptr && vessel->LandedOrSplashed() &&
            (deploymentState == deploymentStates::DEPLOYED || deploymentState == deploymentStates::SEMIDEPLOYED)) {
            CutParachute();
        }
        lerpTime = Get<float>("animTime");
    }
}

// ModuleParachute.cs:738
bool ModuleParachute::ShouldDeploy() {
    Part* p = PartPtr();
    if (p == nullptr) throw Nre();
    const ProcessingVessel* v = VesselRecord();
    if (v == nullptr) throw Nre();
    const double alt = ParachuteEnvironment::altitudeAtPos ? ParachuteEnvironment::altitudeAtPos(*p) : v->altitude;
    if (!(alt < static_cast<double>(Get<float>("deployAltitude")))) {
        return ParachuteEnvironment::raycastDown ? ParachuteEnvironment::raycastDown(*p, Get<float>("deployAltitude")) : false;
    }
    return true;
}

// ModuleParachute.cs:747
bool ModuleParachute::IsMovingFastEnoughToDeploy() {
    Part* p = PartPtr();
    if (p == nullptr) throw Nre();
    const Vector3d vel = ParachuteEnvironment::velocity ? ParachuteEnvironment::velocity(*p) : Vector3d{};
    return vel.sqrMagnitude() > 1.0;
}

// ModuleParachute.cs:757
bool ModuleParachute::IsAnimationPlaying() {
    if (!Anim) throw Nre();
    return Anim->isPlaying();
}

// ModuleParachute.cs:762
float ModuleParachute::GetAnimationNormalizedTime(const std::string& animationName) {
    AnimationState* s = Anim ? (*Anim)[animationName] : nullptr;
    if (s == nullptr) throw Nre();
    return s->normalizedTime();
}

// ModuleParachute.cs:767
void ModuleParachute::PlayDeployAnimation(const std::string& animationName, float deploymentSpeed) {
    AnimationState* s = Anim ? (*Anim)[animationName] : nullptr;
    if (s == nullptr) throw Nre();
    s->speed = deploymentSpeed;
    s->set_normalizedTime(0.0f);
    s->enabled = true;
    Anim->Play(animationName);
}

// ModuleParachute.cs:775
void ModuleParachute::OnVesselWasModified(Vessel* v) {
    Part* p = PartPtr();
    Vessel* myVessel = p != nullptr ? p->vessel : nullptr;
    if (!hasCurrentVessel && myVessel != nullptr) {
        hasCurrentVessel = true;
        currentVessel = myVessel;
    }
    if (!hasCurrentVessel || myVessel == nullptr) return;
    UpdateSymmetry(v);
    // C#: v.persistentId == currentVessel.persistentId && vessel.persistentId != currentVessel.persistentId. A loaded vessel keeps its
    // persistentId, so the comparison is by identity here (a TestVessel stand-in has no persistentId to read)
    if (v == currentVessel && myVessel != currentVessel) {
        currentVessel = myVessel;
        if (deploymentState == deploymentStates::STOWED || deploymentState == deploymentStates::ACTIVE) timeUnpacked = ResourceProcessingEnvironment::UniversalTime();
    }
}

// ModuleParachute.cs:796
void ModuleParachute::UpdateSymmetry(Vessel* v) {
    Part* p = PartPtr();
    if (p != nullptr && p->vessel != nullptr && v == p->vessel) {
        if (!p->symmetryCounterparts.empty()) symmetryCount = static_cast<int>(p->symmetryCounterparts.size()) + 1;
        else symmetryCount = 0;
    }
}

// ModuleParachute.cs:811
void ModuleParachute::OnPartUnpack() { timeUnpacked = ResourceProcessingEnvironment::UniversalTime(); }

// ModuleParachute.cs:816
void ModuleParachute::SetDragCubes_Stowed() {
    if (part == nullptr) return;
    DragCubeList& c = part->DragCubes;
    c.SetCubeWeight("PACKED", 1.0f);
    c.SetCubeWeight("SEMIDEPLOYED", 0.0f);
    c.SetCubeWeight("DEPLOYED", 0.0f);
    c.SetOcclusionMultiplier(1.0f);
}

// ModuleParachute.cs:824
void ModuleParachute::SetDragCube_SemiDeployed(float semiDeployed) {
    DragCubeList& c = part->DragCubes;
    c.SetCubeWeight("PACKED", f32store(1.0 - fx(semiDeployed)));
    c.SetCubeWeight("SEMIDEPLOYED", semiDeployed);
    c.SetCubeWeight("DEPLOYED", 0.0f);
    c.SetOcclusionMultiplier(0.0f);
}

// ModuleParachute.cs:832
void ModuleParachute::SetDragCube_Deployed(float deployed) {
    DragCubeList& c = part->DragCubes;
    c.SetCubeWeight("PACKED", 0.0f);
    c.SetCubeWeight("SEMIDEPLOYED", f32store(1.0 - fx(deployed)));
    c.SetCubeWeight("DEPLOYED", deployed);
    c.SetOcclusionMultiplier(0.0f);
}

// ModuleParachute.cs:840
std::vector<std::string> ModuleParachute::GetDragCubeNames() const {
    const_cast<ModuleParachute*>(this)->SetupAnimation();
    if (!Anim) return {};  // C#: null
    return {"PACKED", "SEMIDEPLOYED", "DEPLOYED"};
}

// ModuleParachute.cs:850
void ModuleParachute::AssumeDragCubePosition(const std::string& name) {
    const std::string semi = Str("semiDeployedAnimation"), full = Str("fullyDeployedAnimation");
    auto st = [&](const std::string& n) {
        AnimationState* s = Anim ? (*Anim)[n] : nullptr;
        if (s == nullptr) throw Nre();
        return s;
    };
    if (name == "DEPLOYED") {
        st(semi)->set_normalizedTime(0.0f);
        st(semi)->set_normalizedSpeed(0.0f);
        st(semi)->enabled = false;
        st(full)->set_normalizedTime(1.0f);
        st(full)->set_normalizedSpeed(0.0f);
        st(full)->enabled = true;
        Anim->Play(full);
    } else if (name == "SEMIDEPLOYED") {
        st(semi)->set_normalizedTime(1.0f);
        st(semi)->set_normalizedSpeed(0.0f);
        st(semi)->enabled = true;
        st(full)->set_normalizedTime(0.0f);
        st(full)->set_normalizedSpeed(0.0f);
        st(full)->enabled = false;
        Anim->Play(semi);
    } else if (name == "PACKED") {
        st(semi)->set_normalizedTime(0.0f);
        st(semi)->set_normalizedSpeed(0.0f);
        st(semi)->enabled = true;
        st(full)->set_normalizedTime(0.0f);
        st(full)->set_normalizedSpeed(0.0f);
        st(full)->enabled = false;
        Anim->Play(semi);
    }
}

// ModuleParachute.cs:889
void ModuleParachute::SetConvectiveStats(double density, double extTemp, double mach, double convCoeff) {
    if (density > 0.0) {
        convectionArea = areaDeployed;
        machHeatMult = dotnet::Math::Pow(Get<double>("machHeatMultBase") + mach, Get<double>("machHeatMultPow"));
        const double fullToCrossSectionLerpStart = PhysicsGlobals::FullToCrossSectionLerpStart();
        machHeatMult = UtilMath::Lerp(1.0, machHeatMult, (mach * Get<double>("machHeatMultScalar") - fullToCrossSectionLerpStart) /
                                                              (PhysicsGlobals::FullToCrossSectionLerpEnd() - fullToCrossSectionLerpStart));
        machHeatMult = UtilMath::Lerp(1.0, machHeatMult, density * Get<double>("machHeatDensityFadeoutMult"));
        convectivekW = 0.001 * convCoeff * convectionArea * machHeatMult;
        shockTemp = extTemp * machHeatMult;
    } else {
        convectivekW = 0.0;
        shockTemp = PhysicsGlobals::SpaceTemperature();
    }
}

// ModuleParachute.cs:908
std::string ModuleParachute::DeploySafe(deploymentSafeStates& state) {
    if (ResourceProcessingEnvironment::UniversalTime() - timeUnpacked < 1.0) {
        state = deploymentSafeStates::UNSAFE;
        return Localizer::Format("#autoLOC_7003411");
    }
    if (shockTemp <= Get<double>("chuteMaxTemp") * Get<double>("safeMult")) {
        state = deploymentSafeStates::SAFE;
        return Localizer::Format("#autoLOC_7003409");
    }
    const double num = (shockTemp - Get<double>("chuteTemp")) * UtilMath::Clamp01(convectivekW * invThermalMass * Get<double>("secondsForRisky"));
    finalTemp = Get<double>("chuteTemp") + num;
    if (finalTemp <= Get<double>("chuteMaxTemp")) {
        state = deploymentSafeStates::RISKY;
        return Localizer::Format("#autoLOC_7003410");
    }
    state = deploymentSafeStates::UNSAFE;
    return Localizer::Format("#autoLOC_7003411");
}

// ModuleParachute.cs:931
void ModuleParachute::UpdateCut() {
    Part* p = PartPtr();
    const ProcessingVessel* v = VesselRecord();
    if (p == nullptr || v == nullptr) throw Nre();
    const Vector3d vel = ParachuteEnvironment::velocity ? ParachuteEnvironment::velocity(*p) : Vector3d{};
    const float autoCutSpeed = Get<float>("autoCutSpeed");
    if (vel.sqrMagnitude() < static_cast<double>(f32store(fx(autoCutSpeed) * fx(autoCutSpeed))) && v->LandedOrSplashed()) {
        CutParachute();
    } else if (p->fields.atmDensity == 0.0) {
        CutParachute();
    } else if (v->situation == VesselSituations::SPLASHED) {
        Set<double>("chuteTemp", v->atmosphericTemperature);
    } else if ((!ShieldedFromAirstream() || Get<bool>("shieldedCanDeploy")) && (deploymentState == deploymentStates::SEMIDEPLOYED || deploymentState == deploymentStates::DEPLOYED)) {
        if (Get<double>("chuteTemp") < PhysicsGlobals::SpaceTemperature()) Set<double>("chuteTemp", Get<double>("startingTemp"));
        const double num = Get<double>("chuteTemp");
        const double num2 = 0.001 * PhysicsGlobals::StefanBoltzmanConstant() * areaDeployed * Get<double>("chuteEmissivity") * PhysicsGlobals::RadiationFactor();
        double num3 = convectivekW * (shockTemp - Get<double>("chuteTemp"));
        double num4 = std::min(Get<double>("chuteTemp"), Get<double>("chuteMaxTemp"));
        num4 *= num4;
        num3 -= num2 * num4 * num4;
        const double fdt = static_cast<double>(TimeWarp::fixedDeltaTime());
        Set<double>("chuteTemp", Get<double>("chuteTemp") + num3 * invThermalMass * fdt);
        num3 = convectivekW * (shockTemp - Get<double>("chuteTemp"));
        num4 = std::min(Get<double>("chuteTemp"), Get<double>("chuteMaxTemp"));
        num4 *= num4;
        num3 -= num2 * num4 * num4;
        Set<double>("chuteTemp", Get<double>("chuteTemp") + num3 * invThermalMass * fdt);
        Set<double>("chuteTemp", (num + Get<double>("chuteTemp")) * 0.5);
        if (std::isnan(Get<double>("chuteTemp")) || Get<double>("chuteTemp") < PhysicsGlobals::SpaceTemperature()) Set<double>("chuteTemp", PhysicsGlobals::SpaceTemperature());
        if (Get<double>("chuteTemp") > Get<double>("chuteMaxTemp")) {
            const std::string title = p->partInfo != nullptr ? p->partInfo->title : std::string();
            ParachuteEnvironment::screenMessages.push_back("<color=orange>" + Localizer::Format("#autoLOC_215326", std::vector<std::string>{title}) + "</color>");
            ParachuteEnvironment::flightLog.push_back(Localizer::Format("#autoLOC_215327", std::vector<std::string>{title}));
            CutParachute();
        }
    }
}

// ModuleParachute.cs:977
std::string ModuleParachute::GetModuleDisplayName() { return Localizer::Format("#autoLoc_6003048"); }

// ================================================= ModuleEvaChute ==================================================================

// ModuleEvaChute.cs:61 (the shader assignments are rendering; the ChuteNode Load, the flags and the canopy-rotation re-sync are ported)
void ModuleEvaChute::OnStart(StartState state) {
    Part* p = PartPtr();
    if (p != nullptr && !p->protoModuleCrew.empty() && p->protoModuleCrew[0] && p->protoModuleCrew[0]->ChuteNode) Load(*p->protoModuleCrew[0]->ChuteNode);
    Events()["Repack"]->guiActive = true;
    ModuleParachute::OnStart(state);
    dontRotateParachute = true;
    deactivateOnRepack = false;
    Fields()["spreadAngle"]->guiActive = false;
    if (!CanCrewMemberUseParachute()) SetEVAChuteActive(false);
}

// ModuleEvaChute.cs:145
void ModuleEvaChute::FixedUpdate() {
    // canopy.rotation = GetFullyDeployedCanopyRotation() / GetSemiDeployedCanopyRotation(): the canopy transform is the engine's
    ModuleParachute::FixedUpdate();
}

// helper split out of ModuleEvaChute.UpdateFullyDeployedParachuteMovement (ModuleEvaChute.cs line 155) (the gravity-alignment term uses Quaternion.AngleAxis / FromToRotation: the engine's; the rate arithmetic is Mono float code)
ModuleEvaChute::ChuteRates ModuleEvaChute::ComputeChuteRates(const Vector3f& parachuteInput, double vesselSpeed) const {
    ChuteRates r;
    const float minS = Get<float>("chuteMinSpeedForYawRate"), maxS = Get<float>("chuteMaxSpeedForYawRate");
    const float yawMin = Get<float>("chuteYawRateAtMinSpeed"), yawMax = Get<float>("chuteYawRateAtMaxSpeed");
    r.num = f32store((fx(Mathf::Clamp(static_cast<float>(vesselSpeed), minS, maxS)) - fx(minS)) / (fx(maxS) - fx(minS)) * (fx(yawMax) - fx(yawMin)) + fx(yawMin));
    const bool pitching = parachuteInput.y != 0.0f;
    const bool turning = parachuteInput.x != 0.0f;
    r.num3 = turning ? f32store(fx(Get<float>("chuteRollRate")) / fx(Get<float>("chuteRollRateDivisorWhenPitching"))) : Get<float>("chuteRollRate");
    r.num4 = pitching ? f32store(fx(Get<float>("chutePitchRate")) / fx(Get<float>("chutePitchRateDivisorWhenTurning"))) : Get<float>("chutePitchRate");
    r.num5 = turning ? f32store(fx(r.num) / fx(Get<float>("chuteYawRateDivisorWhenPitching"))) : r.num;
    Vector3f zero{0.0f, 0.0f, 0.0f};
    zero = zero + kerbalEVA.right * parachuteInput.x * r.num4;
    zero = zero + kerbalEVA.up * parachuteInput.y * r.num5;
    zero = zero + kerbalEVA.forward * -1.0f * parachuteInput.y * r.num3;
    r.zero = zero;
    return r;
}

// ModuleEvaChute.cs:191
bool ModuleEvaChute::CanCrewMemberUseParachute(int experienceLevel) {
    if (hasInventory && inventoryItemCount == 0) return false;
    return experienceLevel >= evaChuteSkill;
}

// ModuleEvaChute.cs:191
bool ModuleEvaChute::CanCrewMemberUseParachute() {
    const Part* p = PartPtr();
    if (p != nullptr && p->protoModuleCrew.size() >= 1) {
        const int level = p->protoModuleCrew[0] ? p->protoModuleCrew[0]->experienceLevel : crewExperienceLevel;
        return CanCrewMemberUseParachute(level);
    }
    return false;
}

// ModuleEvaChute.cs:220
void ModuleEvaChute::ShowEVAChuteParamsChanged() {
    showEVAChuteParams = !showEVAChuteParams;
    ToggleEVAChuteParams(showEVAChuteParams);
}

// ModuleEvaChute.cs:218
void ModuleEvaChute::ToggleEVAChuteParams(bool show) {
    for (const char* f : {"chuteYawRateAtMaxSpeed", "chuteMaxSpeedForYawRate", "chuteYawRateAtMinSpeed", "chuteMinSpeedForYawRate", "chuteRollRate", "chutePitchRate",
                          "chuteDefaultForwardPitch", "semiDeployedChuteForwardPitch", "chutePitchRateDivisorWhenTurning", "chuteRollRateDivisorWhenPitching",
                          "chuteYawRateDivisorWhenPitching", "rotationSpeedDPS"}) {
        if (BaseField* bf = Fields()[f]) bf->guiActive = show;
    }
}

// ModuleEvaChute.cs:242
void ModuleEvaChute::SetEVAChuteActive(bool active) {
    Set<bool>("moduleIsEnabled", active);
    isEnabled = active;  // base.enabled
    for (const char* f : {"minAirPressureToOpen", "deployAltitude", "spreadAngle", "automateSafeDeploy"}) Fields()[f]->guiActive = active;
    Events()["ShowEVAChuteParamsChanged"]->active = active;
    if (active) {
        SetUIEVents();
        return;
    }
    Events()["Deploy"]->active = false;
    Events()["CutParachute"]->active = false;
    Events()["Repack"]->active = false;
    Events()["Disarm"]->active = false;
}

// ModuleEvaChute.cs:263
void ModuleEvaChute::OnParachuteSemiDeployed() {
    if (hasKerbalEVA) {
        if (!kerbalEVA.IsChuteState) kerbalEVA.onParachuteSemiDeployedCalls++;
    }
    // canopy.rotation / lastRot: the engine's
    ModuleParachute::OnParachuteSemiDeployed();
}

// ModuleEvaChute.cs:279
void ModuleEvaChute::OnParachuteFullyDeployed() {
    if (hasKerbalEVA) kerbalEVA.onParachuteFullyDeployedCalls++;
    ModuleParachute::OnParachuteFullyDeployed();
}

// ModuleEvaChute.cs:286
bool ModuleEvaChute::PassedAdditionalDeploymentChecks() {
    const ProcessingVessel* v = VesselRecord();
    Part* p = PartPtr();
    if (v == nullptr || p == nullptr) throw Nre();
    if (kerbalEVA.ableToDeployParachute && !kerbalEVA.JetpackDeployed &&
        (!v->Splashed || (kerbalEVA.seated && (!kerbalEVA.seated || !(p->fields.submergedPortion > 0.0)))) && (!v->Landed || kerbalEVA.seated)) {
        return ModuleParachute::PassedAdditionalDeploymentChecks();
    }
    return false;
}

// ModuleEvaChute.cs:296
void ModuleEvaChute::CutParachute() {
    if (deploymentState != deploymentStates::CUT && deploymentState != deploymentStates::ACTIVE) {
        ModuleParachute::CutParachute();
        if (kerbalEVA.IsChuteState || kerbalEVA.seated) kerbalEVA.onParachuteCutCalls++;
    }
}

// ModuleEvaChute.cs:307
void ModuleEvaChute::AllowRepack(bool allowRepack) {
    if (allowRepack && deploymentState == deploymentStates::CUT) {
        Events()["Repack"]->active = true;
        Events()["Repack"]->guiActive = true;
    } else {
        Events()["Repack"]->active = false;
    }
}

namespace detail {
void RegisterParachuteModules() {
    ModuleRegistrar<ModuleParachute>("ModuleParachute");
    ModuleRegistrar<ModuleEvaChute>("ModuleEvaChute");
}
}  // namespace detail

}  // namespace ksp
