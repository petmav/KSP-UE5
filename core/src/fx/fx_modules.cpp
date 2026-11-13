// WP-UE-16a: the FXModule* part modules (see ksp/fx/fx_modules.hpp).
#include "ksp/fx/fx_modules.hpp"

#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/modules/module_engines.hpp"
#include "ksp/modules/module_rcs.hpp"
#include "ksp/parts/cs_parse.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/space_center/high_logic.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

namespace {
Part* PartOfModule(PartModule& m) { return dynamic_cast<Part*>(m.part); }

// PartModuleList.FindEngineInterface (:177)
IEngineStatus* FindEngineInterface(const PartModuleList& modules, const std::string& engineName, int engineIndex, bool preferMultiMode) {
    const size_t count = modules.Count();
    int num = 0;
    if (preferMultiMode) {
        for (size_t i = 0; i < count; i++) {
            PartModule* m = modules.GetModule(i);
            if (dynamic_cast<IEngineStatus*>(m) != nullptr && dynamic_cast<MultiModeEngine*>(m) != nullptr) {
                if (num == engineIndex) return dynamic_cast<IEngineStatus*>(m);
                num++;
            }
        }
    }
    if (!engineName.empty()) {
        for (size_t j = 0; j < count; j++) {
            if (auto* e = dynamic_cast<IEngineStatus*>(modules.GetModule(j)))
                if (e->engineName() == engineName) return e;
        }
    } else {
        int idx = 0;
        for (size_t k = 0; k < count; k++) {
            if (auto* e = dynamic_cast<IEngineStatus*>(modules.GetModule(k))) {
                if (idx == engineIndex) return e;
                idx++;
            }
        }
    }
    return nullptr;
}

std::string StrField(PartModule& m, const char* name) { return m.Get<std::string>(name); }

FXTrackMode ParseTrackMode(const std::string& s) {  // Enum.Parse(typeof(TrackMode), value)
    const std::string t = dotnet::Trim(s);
    if (t == "FixedUpdate") return FXTrackMode::FixedUpdate;
    if (t == "Update") return FXTrackMode::Update;
    if (t == "LateUpdate") return FXTrackMode::LateUpdate;
    int32_t n = 0;
    if (!t.empty() && cs::IntTryParse(t, n)) return static_cast<FXTrackMode>(n);
    throw ArgumentException("Requested value '" + t + "' was not found.");
}
}  // namespace

// PartModuleList.cs:147 (`part.parent.Modules`: a part without a parent is a NullReferenceException)
IEngineStatus* FindEngineNearby(Part& part, const std::string& engineName, int engineIndex, bool preferMultiMode) {
    if (IEngineStatus* e = FindEngineInterface(part.Modules(), engineName, engineIndex, preferMultiMode)) return e;
    if (part.parent == nullptr) throw NullReferenceException();
    if (IEngineStatus* e = FindEngineInterface(part.parent->Modules(), engineName, engineIndex, preferMultiMode)) return e;
    for (Part* c : part.children)
        if (IEngineStatus* e = FindEngineInterface(c->Modules(), engineName, engineIndex, preferMultiMode)) return e;
    return nullptr;
}

// ---- FXModuleAnimateThrottle -----------------------------------------------------------------------------------------------

// FXModuleAnimateThrottle.cs:58
void FXModuleAnimateThrottle::OnStart(StartState) {
    animationComponent = -1;
    Part* p = PartOfModule(*this);
    const std::string animationName = StrField(*this, "animationName");
    if (p != nullptr && FXModuleEnvironment::findAnimations) {
        const std::vector<bool> list = FXModuleEnvironment::findAnimations(*p, animationName);
        for (size_t i = 0; i < list.size(); i++)
            if (list[i]) {
                animationComponent = static_cast<int>(i);
                break;
            }
    }
    if (animationComponent >= 0) {
        animation.wrapMode = static_cast<WrapMode>(Get<int32_t>("animWrapMode"));
        animation.weight = 1.0f;
        animation.speed = Get<float>("baseAnimSpeed");
        animation.normalizedTime = 0.0f;
        animation.layer = Get<int32_t>("layer");
        if (Get<bool>("dependOnEngineState")) {
            engineReference = FindEngineNearby(*p, StrField(*this, "engineName"), Get<int32_t>("engineIndex"), Get<bool>("preferMultiMode"));
            if (engineReference == nullptr) Set<bool>("dependOnEngineState", false);
        }
    } else {
        log::Info("FXModuleAnimateThrottle: Could not find animation " + animationName + " in part's animation components. Check the animationName and model file");
        behaviourEnabled = false;
    }
}

// FXModuleAnimateThrottle.cs:95
void FXModuleAnimateThrottle::FixedUpdate() {
    if (!behaviourEnabled) return;
    const bool loadedSceneIsEditor = HighLogic::LoadedSceneIsEditor;
    Part* p = PartOfModule(*this);
    if ((!loadedSceneIsEditor && !HighLogic::LoadedSceneIsFlight) || p == nullptr || p->vessel == nullptr) return;
    if (animationComponent < 0) return;  // anim[animationName] == null
    if (!animation.playing) animation.playing = true;  // anim.Play(animationName)
    const float dt = Time::fixedDeltaTime;  // TimeWarp.fixedDeltaTime
    const float responseSpeed = Get<float>("responseSpeed");
    const bool playInEditor = Get<bool>("playInEditor");
    const float throttle = p->vessel->ctrlState.mainThrottle;
    float num = 0.0f;
    float num2;
    if (!Get<bool>("dependOnEngineState")) {
        num = loadedSceneIsEditor ? (playInEditor ? 1.0f : 0.0f) : Mathf::Clamp01(throttle);
    } else {
        if (Get<bool>("weightOnOperational")) {
            num = (!loadedSceneIsEditor || !playInEditor) ? (engineReference->isOperational() ? 1.0f : 0.0f) : 1.0f;
            num2 = Mathf::Lerp(animation.weight, num, f32store(fx(responseSpeed) * fx(3.0f) * fx(25.0f) * fx(dt)));
            if (num2 > 0.999f && animation.weight < num) num2 = 1.0f;
            else if (num2 < 0.001f && num < animation.weight) num2 = 0.0f;
            animation.weight = num2;
        }
        num = (engineReference->isOperational() && !loadedSceneIsEditor)
                  ? (Get<bool>("dependOnOutput") ? Mathf::Clamp01(engineReference->normalizedOutput())
                                                 : (!Get<bool>("dependOnThrottle") ? Mathf::Clamp01(throttle) : Mathf::Clamp01(engineReference->throttleSetting())))
                  : ((!loadedSceneIsEditor || !playInEditor) ? 0.0f : 1.0f);
    }
    const float animState = Get<float>("animState");
    if (Get<bool>("affectTime")) {
        num2 = Mathf::Lerp(animState, num, f32store(fx(responseSpeed) * fx(25.0f) * fx(dt)));
        if (num2 > 0.99995f && animState < num) num2 = 1.0f;
        if (num2 < 5E-05f && num < animState) num2 = 0.0f;
        animation.normalizedTime = num2;
    } else {
        float num3 = 0.0f;
        if (num > 0.0f || (Get<bool>("dependOnEngineState") && engineReference->isOperational())) num3 = Get<float>("baseAnimSpeed");
        num2 = Mathf::Lerp(animState, f32store(fx(num3) + fx(num) * fx(Get<float>("baseAnimSpeedMult"))), f32store(fx(responseSpeed) * fx(dt)));
        if (num2 > 0.99995f) num2 = 1.0f;
        if (num2 < 5E-05f) num2 = 0.0f;
        animation.speed = num2;
    }
    if (!loadedSceneIsEditor) Set<float>("animState", num2);
}

// FXModuleAnimateThrottle.cs:172
void FXModuleAnimateThrottle::OnInventoryModeDisable() {
    animationComponent = -1;
    Part* p = PartOfModule(*this);
    if (p != nullptr && FXModuleEnvironment::findAnimations) {
        const std::vector<bool> list = FXModuleEnvironment::findAnimations(*p, StrField(*this, "animationName"));
        for (size_t i = 0; i < list.size(); i++)
            if (list[i]) {
                animationComponent = static_cast<int>(i);
                break;
            }
    }
    if (animationComponent >= 0) {
        animation.speed = 0.0f;
        animation.normalizedTime = 0.0f;
    }
}

// ---- FXModuleAnimateRCS ----------------------------------------------------------------------------------------------------

// FXModuleAnimateRCS.cs:38
void FXModuleAnimateRCS::OnStart(StartState) {
    Part* p = PartOfModule(*this);
    moduleRCSReference = nullptr;
    if (p != nullptr) moduleRCSReference = dynamic_cast<ModuleRCS*>(p->FindModuleImplementing([](PartModule& m) { return dynamic_cast<ModuleRCS*>(&m) != nullptr; }));
    if (moduleRCSReference == nullptr) {
        log::Info("[FXModuleAnimateRCS]: Could not ModuleRCS on Part. Check the Part and it's cfg file. Mult contain ModuleRCS/ModuleRCSFX. Module disabled.");
        behaviourEnabled = false;
        return;
    }
    anims.clear();
    const std::string animationName = StrField(*this, "animationName");
    std::vector<bool> list;
    if (FXModuleEnvironment::findAnimations) list = FXModuleEnvironment::findAnimations(*p, animationName);
    bool any = false;
    if (!list.empty()) {
        anims.assign(list.size(), std::nullopt);
        animState.assign(list.size(), 0.0f);
        for (size_t i = 0; i < list.size(); i++)
            if (list[i]) anims[i] = AnimationStateData{};
        any = true;
    }
    if (!any) {
        log::Info("[FXModuleAnimateRCS]: Could not find animation " + animationName + " in part's animation components. Check the animationName and model file. Module disabled.");
        behaviourEnabled = false;
        return;
    }
    for (auto& a : anims) {
        if (!a) throw NullReferenceException();  // anims[j][animationName] on a null entry
        a->wrapMode = static_cast<WrapMode>(Get<int32_t>("animWrapMode"));
        a->weight = 1.0f;
        a->speed = Get<float>("baseAnimSpeed");
        a->normalizedTime = 0.0f;
        a->layer = Get<int32_t>("layer");
    }
    const int thrusters = FXModuleEnvironment::findModelTransformCount ? FXModuleEnvironment::findModelTransformCount(*p, moduleRCSReference->Get<std::string>("thrusterTransformName")) : 0;
    if (static_cast<int>(anims.size()) != thrusters) {
        log::Info("[FXModuleAnimateRCS]: Found " + std::to_string(anims.size()) + " Animations and " + std::to_string(moduleRCSReference->thrusterTransforms.size()) +
                  " Thrust Transforms. Check the Part model file. Module disabled.");
        behaviourEnabled = false;
    }
}

// FXModuleAnimateRCS.cs:85 (`text += animState[i]`: float.ToString())
void FXModuleAnimateRCS::OnSave(ConfigNode& node) {
    if (animState.empty()) return;  // animState == null
    std::string text;
    for (size_t i = 0; i < animState.size(); i++) {
        if (i != 0) text += ",";
        text += dotnet::FormatSingleG(animState[i]);
    }
    node.AddValue("animStates", text);
}

// FXModuleAnimateRCS.cs:103
void FXModuleAnimateRCS::OnLoad(const ConfigNode& node) {
    std::string value;
    if (node.TryGetValue("animStates", value)) {
        const std::vector<std::string> array = dotnet::Split(value, ",", false);
        animState.assign(array.size(), 0.0f);
        for (size_t i = 0; i < array.size(); i++) {
            float f = 0.0f;
            if (cs::FloatTryParse(array[i], f)) animState[i] = f;
            else animState[i] = 0.0f;
        }
    }
}

// FXModuleAnimateRCS.cs:117
void FXModuleAnimateRCS::FixedUpdate() {
    if (!behaviourEnabled) return;
    const bool loadedSceneIsEditor = HighLogic::LoadedSceneIsEditor;
    if (!loadedSceneIsEditor && !HighLogic::LoadedSceneIsFlight) return;
    const float dt = Time::fixedDeltaTime;
    const float responseSpeed = Get<float>("responseSpeed");
    for (size_t i = 0; i < anims.size(); i++) {
        if (!anims[i]) throw NullReferenceException();  // anims[i][animationName]
        AnimationStateData& st = *anims[i];
        if (!st.playing) st.playing = true;  // anims[i].Play(animationName)
        const float num = Mathf::Clamp01(f32store(fx(moduleRCSReference->thrustForces.at(i)) * fx(Get<float>("thrustForceMult"))));
        float num2 = 0.0f;
        if (Get<bool>("affectTime")) {
            num2 = Mathf::Lerp(animState.at(i), num, f32store(fx(responseSpeed) * fx(25.0f) * fx(dt)));
            if (num2 > 0.99995f && animState[i] < num) num2 = 1.0f;
            if (num2 < 5E-05f && num < animState[i]) num2 = 0.0f;
            st.normalizedTime = num2;
        } else {
            float num3 = 0.0f;
            if (num > 0.0f) num3 = Get<float>("baseAnimSpeed");
            num2 = Mathf::Lerp(animState.at(i), f32store(fx(num3) + fx(num) * fx(Get<float>("baseAnimSpeedMult"))), f32store(fx(responseSpeed) * fx(dt)));
            if (num2 > 0.99995f) num2 = 1.0f;
            if (num2 < 5E-05f) num2 = 0.0f;
            st.speed = num2;
        }
        if (!loadedSceneIsEditor) animState[i] = num2;
    }
}

// ---- FXModuleThrottleEffects / PartModuleFXSetter --------------------------------------------------------------------------

// PartModuleFXSetter.cs:12
void FXModuleThrottleEffects::OnAwake() {
    PartModule::OnAwake();
    fxModules.clear();
}

// FXModuleThrottleEffects.cs:33 -> PartModuleFXSetter.OnStart :21
void FXModuleThrottleEffects::OnStart(StartState state) {
    Part* p = PartOfModule(*this);
    if (Get<bool>("dependOnEngineState")) {
        engineReference = FindEngineNearby(*p, StrField(*this, "engineName"), Get<int32_t>("engineIndex"), Get<bool>("preferMultiMode"));
        if (engineReference == nullptr) Set<bool>("dependOnEngineState", false);
    }
    const std::string names = StrField(*this, "fxModuleNames");
    if (names.empty()) return;
    (void)state;
    fxModules.clear();
    for (const std::string& n : dotnet::Split(names, ",", false)) {
        if (IScalarModule* m = p->Modules().GetScalarModule(dotnet::Trim(n))) fxModules.push_back(m);
    }
}

// PartModuleFXSetter.cs:41
void FXModuleThrottleEffects::SetFXModules(float scalar) {
    for (IScalarModule* m : fxModules) m->SetScalar(scalar);
}

// FXModuleThrottleEffects.cs:46
void FXModuleThrottleEffects::FixedUpdate() {
    Part* p = PartOfModule(*this);
    if (!(HighLogic::LoadedSceneIsFlight && p != nullptr && p->vessel != nullptr)) return;
    const float throttle = p->vessel->ctrlState.mainThrottle;
    float num = 0.0f;
    if (!Get<bool>("dependOnEngineState")) {
        num = Mathf::Clamp01(throttle);
    } else if (!engineReference->isOperational()) {
        num = 0.0f;
    } else if (Get<bool>("dependOnOutput")) {
        num = Mathf::Clamp01(engineReference->normalizedOutput());
    } else {
        num = !Get<bool>("dependOnThrottle") ? Mathf::Clamp01(throttle) : Mathf::Clamp01(engineReference->throttleSetting());
    }
    const float state = Get<float>("state");
    float num2 = Mathf::Lerp(state, num, f32store(fx(Get<float>("responseSpeed")) * fx(25.0f) * fx(Time::fixedDeltaTime)));
    if (num2 > 0.99995f && state < num) num2 = 1.0f;
    if (num2 < 5E-05f && num < state) num2 = 0.0f;
    Set<float>("state", num2);
}

// FXModuleThrottleEffects.cs:65
void FXModuleThrottleEffects::Update() {
    Part* p = PartOfModule(*this);
    if (HighLogic::LoadedSceneIsFlight && p != nullptr && p->vessel != nullptr) SetFXModules(Get<float>("state"));
}

// ---- FXModuleLookAtConstraint / FXModuleConstrainPosition ------------------------------------------------------------------

namespace {
int TransformCount(PartModule& m, const std::string& name) {
    Part* p = PartOfModule(m);
    if (p == nullptr || !FXModuleEnvironment::findModelTransformCount) return 0;
    return FXModuleEnvironment::findModelTransformCount(*p, name);
}
}  // namespace

// FXModuleLookAtConstraint.cs:57
void FXModuleLookAtConstraint::OnStart(StartState state) {
    if (state != StartState::Editor) ready_ = true;
    if (Get<bool>("runInEditor")) ready_ = true;
}
// FXModuleLookAtConstraint.cs:69 / :77 / :85
void FXModuleLookAtConstraint::OnUpdate() {
    if (trackingMode == FXTrackMode::Update) Track();
}
void FXModuleLookAtConstraint::OnFixedUpdate() {
    if (trackingMode == FXTrackMode::FixedUpdate) Track();
}
void FXModuleLookAtConstraint::LateUpdate() {
    if (trackingMode == FXTrackMode::LateUpdate) Track();
}
// FXModuleLookAtConstraint.cs:93 / :102
void FXModuleLookAtConstraint::OnConstructionModeUpdate() {
    PartModule::OnConstructionModeUpdate();
    if (trackingMode == FXTrackMode::Update || trackingMode == FXTrackMode::LateUpdate) Track();
}
void FXModuleLookAtConstraint::OnConstructionModeFixedUpdate() {
    PartModule::OnConstructionModeFixedUpdate();
    if (trackingMode == FXTrackMode::FixedUpdate) Track();
}
// FXModuleLookAtConstraint.cs:111: movers[j].LookAt(target, base.transform.up)
void FXModuleLookAtConstraint::Track() {
    if (!ready_) return;
    for (const ConstrainedObject& co : ObjectsList) {
        for (int j = 0; j < co.moverCount; j++) {
            ConstraintDescription d;
            d.kind = "lookAt";
            d.targetName = co.targetName;
            d.moverName = co.moversName;
            d.moverIndex = j;
            d.upIsPartUp = true;
            tracked.push_back(d);
        }
    }
}
// FXModuleLookAtConstraint.cs:129
void FXModuleLookAtConstraint::SetupObjects(ConstrainedObject co) {
    co.hasTarget = TransformCount(*this, co.targetName) > 0;
    co.moverCount = TransformCount(*this, co.moversName);
    if (co.hasTarget && co.moverCount >= 1) ObjectsList.push_back(co);
}
// FXModuleLookAtConstraint.cs:139
void FXModuleLookAtConstraint::OnLoad(const ConfigNode& node) {
    trackingMode = ParseTrackMode(StrField(*this, "trackingModeString"));
    if (!node.HasNode("CONSTRAINLOOKFX")) return;
    ObjectsList.clear();
    for (int i = 0; i < node.nodes().Count(); i++) {
        const ConfigNode& cn = *node.nodes()[i];
        if (cn.name == "CONSTRAINLOOKFX") {
            ConstrainedObject co;
            co.targetName = cn.GetValue("targetName").value_or("");
            co.moversName = cn.GetValue("rotatorsName").value_or("");
            SetupObjects(co);
        }
    }
}
std::vector<ConstraintDescription> FXModuleLookAtConstraint::TakeTracked() {
    std::vector<ConstraintDescription> out;
    out.swap(tracked);
    return out;
}

// FXModuleConstrainPosition.cs:56 / :64 / :72
void FXModuleConstrainPosition::OnUpdate() {
    if (trackingMode == FXTrackMode::Update) Track();
}
void FXModuleConstrainPosition::OnFixedUpdate() {
    if (trackingMode == FXTrackMode::FixedUpdate) Track();
}
void FXModuleConstrainPosition::LateUpdate() {
    if (trackingMode == FXTrackMode::LateUpdate) Track();
}
// FXModuleConstrainPosition.cs:80: mover.position = target.position (matchPosition); mover.rotation = target.rotation (matchRotation)
void FXModuleConstrainPosition::Track() {
    for (const ConstrainedObject& co : ObjectsList) {
        for (int j = 0; j < co.moverCount; j++) {
            ConstraintDescription d;
            d.kind = "constrain";
            d.targetName = co.targetName;
            d.moverName = co.moversName;
            d.moverIndex = j;
            d.matchPosition = Get<bool>("matchPosition");
            d.matchRotation = Get<bool>("matchRotation");
            tracked.push_back(d);
        }
    }
}
// FXModuleConstrainPosition.cs:102
void FXModuleConstrainPosition::SetupObjects(ConstrainedObject co) {
    co.hasTarget = TransformCount(*this, co.targetName) > 0;
    co.moverCount = TransformCount(*this, co.moversName);
    if (co.hasTarget && co.moverCount >= 1) ObjectsList.push_back(co);
}
// FXModuleConstrainPosition.cs:112
void FXModuleConstrainPosition::OnLoad(const ConfigNode& node) {
    trackingMode = ParseTrackMode(StrField(*this, "trackingModeString"));
    if (!node.HasNode("CONSTRAINFX")) return;
    ObjectsList.clear();
    for (int i = 0; i < node.nodes().Count(); i++) {
        const ConfigNode& cn = *node.nodes()[i];
        if (cn.name == "CONSTRAINFX") {
            ConstrainedObject co;
            co.targetName = cn.GetValue("targetName").value_or("");
            co.moversName = cn.GetValue("moversName").value_or("");
            SetupObjects(co);
        }
    }
}
std::vector<ConstraintDescription> FXModuleConstrainPosition::TakeTracked() {
    std::vector<ConstraintDescription> out;
    out.swap(tracked);
    return out;
}

namespace detail {
void RegisterFxModules() {
    ModuleFactory::Register("FXModuleAnimateThrottle", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<FXModuleAnimateThrottle>(c)); });
    ModuleFactory::Register("FXModuleAnimateRCS", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<FXModuleAnimateRCS>(c)); });
    ModuleFactory::Register("FXModuleThrottleEffects", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<FXModuleThrottleEffects>(c)); });
    ModuleFactory::Register("FXModuleLookAtConstraint", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<FXModuleLookAtConstraint>(c)); });
    ModuleFactory::Register("FXModuleConstrainPosition", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<FXModuleConstrainPosition>(c)); });
}
}  // namespace detail

}  // namespace ksp
