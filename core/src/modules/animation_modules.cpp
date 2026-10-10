// WP-27: ModuleAnimationSetter, ModuleAnimateGeneric, ModuleAnimationGroup, ModuleAnimatorLandingGear. See ksp/modules/animation_modules.hpp.
// Every function cites File.cs:line of decompiled/KSP-Decompiled/Assembly-CSharp (tools/wp28/check_cites.py verifies the citations).
#include "ksp/modules/animation_modules.hpp"

#include <cmath>
#include <stdexcept>

#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/parts/part_module_data.hpp"
#include "ksp/unity/mathf.hpp"

namespace ksp {

using GE = events::GameEvents;

namespace {
NullReferenceException Nre() { return NullReferenceException("Object reference not set to an instance of an object"); }
constexpr int64_t kReplaceWithDefault = -1;  // KSPActionGroup.REPLACEWITHDEFAULT

// Part.FindModelAnimators(clipName): the model's Animation components that have the clip
std::vector<std::shared_ptr<Animation>> FindModelAnimators(PartModule& m, const std::string& clipName) {
    std::vector<std::shared_ptr<Animation>> out;
    if (DeployablesEnvironment::modelAnimations)
        for (auto& a : DeployablesEnvironment::modelAnimations(m))
            if (a->GetClip(clipName)) out.push_back(a);
    return out;
}
std::vector<std::shared_ptr<Animation>> FindModelAnimators(PartModule& m) {  // Part.FindModelAnimators()
    return DeployablesEnvironment::modelAnimations ? DeployablesEnvironment::modelAnimations(m) : std::vector<std::shared_ptr<Animation>>{};
}
Part* PartOf(PartModule& m) { return dynamic_cast<Part*>(m.part); }
std::string StrField(const PartModule& m, std::string_view f) { return m.IsNull(f) ? std::string() : m.Get<std::string>(f); }
}  // namespace

void AnimationModulesEnvironment::Reset() {
    unscaledTime = 0.0f;
    unscaledDeltaTime = 0.02f;
    partEffects.clear();
    checkTransferDialogCalls = 0;
    screenMessages.clear();
    printed.clear();
}

// ============================================== ModuleAnimationSetter =============================================================

// ModuleAnimationSetter.cs:32
std::string ModuleAnimationSetter::ScalarModuleID() const { return Get<std::string>("moduleID"); }

// ModuleAnimationSetter.cs:42
void ModuleAnimationSetter::OnAwake() {
    if (IsNull("animStateCurve")) {
        FloatCurve c;
        c.Add(0.0f, 0.0f);
        c.Add(1.0f, 1.0f);
        Set<FloatCurve>("animStateCurve", std::move(c));
    }
}

// ModuleAnimationSetter.cs:54
void ModuleAnimationSetter::OnStart(StartState) { AnimStartup(); }

// ModuleAnimationSetter.cs:59
void ModuleAnimationSetter::Update() { UpdateAnim(); }

// ModuleAnimationSetter.cs:64
void ModuleAnimationSetter::AnimStartup() {
    const std::string animName = StrField(*this, "animName");
    auto array = FindModelAnimators(*this, animName);
    stateCount = static_cast<int>(array.size());
    animStates.assign(static_cast<size_t>(stateCount), nullptr);
    for (int i = 0; i < stateCount; i++) {
        Animation& obj = *array[static_cast<size_t>(i)];
        AnimationState* animationState = obj[animName];
        animationState->speed = 0.0f;
        animationState->enabled = true;
        obj[animName]->layer = Get<int32_t>("layer");
        obj.Play(animName);
        animStates[static_cast<size_t>(i)] = animationState;
    }
}

// ModuleAnimationSetter.cs:81
void ModuleAnimationSetter::SetScalar(float inputVal) {
    if (!std::isnan(inputVal)) {
        inputState = inputVal;
        animState = Get<FloatCurve>("animStateCurve").Evaluate(inputVal);
    }
}

// ModuleAnimationSetter.cs:88
void ModuleAnimationSetter::UpdateAnim() {
    if (!animStates.empty() || stateCount == 0) {
        for (int i = 0; i < stateCount; i++) animStates[static_cast<size_t>(i)]->set_normalizedTime(animState);
    }
}

// ModuleAnimationSetter.cs:112
std::string ModuleAnimationSetter::GetModuleDisplayName() { return Localizer::Format("#autoLoc_6003028"); }

// ============================================== ModuleAnimateGeneric ==============================================================

ModuleAnimateGeneric::animationStates ModuleAnimateGeneric::aniState() const { return static_cast<animationStates>(Get<EnumValue>("aniState").value); }
void ModuleAnimateGeneric::set_aniState(animationStates s) {
    EnumValue e = Get<EnumValue>("aniState");
    e.value = static_cast<int64_t>(s);
    Set<EnumValue>("aniState", e);
}
std::string ModuleAnimateGeneric::Str(std::string_view field) const { return StrField(*this, field); }

// ModuleAnimateGeneric.cs:155
std::string ModuleAnimateGeneric::ScalarModuleID() const { return Get<std::string>("moduleID"); }

// ModuleAnimateGeneric.cs:159
bool ModuleAnimateGeneric::CanMove() const {
    const Part* p = dynamic_cast<const Part*>(part);
    if (!DeployablesEnvironment::SceneIsEditor() && !Get<bool>("allowAnimationWhileShielded")) return !(p != nullptr && p->ShieldedFromAirstream());
    return true;
}

// ModuleAnimateGeneric.cs:178
void ModuleAnimateGeneric::ToggleAction(KSPActionParam&) {
    if (Get<bool>("allowManualControl")) Toggle();
}

// ModuleAnimateGeneric.cs:234
void ModuleAnimateGeneric::Toggle() {
    const Part* p = PartOf(*this);
    const bool shielded = p != nullptr && p->ShieldedFromAirstream();
    if (Get<bool>("allowManualControl") && (DeployablesEnvironment::SceneIsEditor() || !shielded || Get<bool>("allowAnimationWhileShielded"))) {
        Set<bool>("animSwitch", !Get<bool>("animSwitch"));
        UpdateAnimSwitch();
        if (DeployablesEnvironment::SceneIsFlight() && !Get<bool>("animationIsDisabled") && Get<bool>("disableAfterPlaying") && !Get<bool>("animSwitch"))
            Set<bool>("animationIsDisabled", true);
    }
}

// ModuleAnimateGeneric.cs:210
void ModuleAnimateGeneric::UpdateAnimSwitch() {
    if (!anim) FindAnimation();
    AnimationState& s = *GetState();
    if (Get<bool>("animSwitch")) {
        s.speed = (!Get<bool>("instantAnimInEditor") || !DeployablesEnvironment::SceneIsEditor()) ? (-1.0f) : f32store(-10.0 * fx(s.length));
        anim->Play(Str("animationName"));
        toggleEvt->SetGuiName(Get<std::string>("startEventGUIName"));
        onMove->Fire(1.0f, 0.0f);
    } else {
        s.speed = (!Get<bool>("instantAnimInEditor") || !DeployablesEnvironment::SceneIsEditor()) ? 1.0f : f32store(10.0 * fx(s.length));
        anim->Play(Str("animationName"));
        toggleEvt->SetGuiName(Get<std::string>("endEventGUIName"));
        onMove->Fire(0.0f, 1.0f);
    }
    set_aniState(animationStates::MOVING);
    Set<std::string>("status", Localizer::Format("#autoLOC_215506"));
}

// ModuleAnimateGeneric.cs:234
void ModuleAnimateGeneric::OnAwake() {
    toggleEvt = Events()["Toggle"];
    toggleAct = Actions()["ToggleAction"];
    Events().Bind("Toggle", BaseEvent::Handler([this] { Toggle(); }));
    Actions().Bind("ToggleAction", [this](KSPActionParam& p) { ToggleAction(p); });
    const int64_t defaultGroup = Get<EnumValue>("defaultActionGroup").value;
    if (toggleAct->actionGroup == kReplaceWithDefault) toggleAct->actionGroup = defaultGroup;
    if (toggleAct->defaultActionGroup == kReplaceWithDefault) toggleAct->defaultActionGroup = defaultGroup;
    std::string prefix;
    const Part* p = PartOf(*this);
    if (part != nullptr) {
        const auto& pn = part->fields.partName;
        prefix = (pn ? *pn : std::string()) + "_" + std::to_string(part->fields.flightID) + "_" + std::to_string(p ? p->Modules().IndexOf(this) : -1);
    }
    onMove = std::make_unique<events::EventData<float, float>>(prefix + "_onMove");
    onStop = std::make_unique<events::EventData<float>>(prefix + "_onStop");
}

// ModuleAnimateGeneric.cs:250
void ModuleAnimateGeneric::OnStart(StartState) {
    if (!anim) FindAnimation();
    if (!behaviourEnabled) return;
    toggleAct->SetGuiName(Get<std::string>("actionGUIName"));
    if (Get<bool>("animSwitch")) toggleEvt->SetGuiName(Get<std::string>("startEventGUIName"));
    else toggleEvt->SetGuiName(Get<std::string>("endEventGUIName"));
    AnimationState& s = *GetState();
    s.speed = 0.0f;
    s.enabled = true;
    s.weight = 1.0f;
    s.layer = Get<int32_t>("layer");
    if (Get<bool>("isOneShot") && aniState() == animationStates::FIXED) {
        s.set_normalizedTime(1.0f);
        SetDragState(1.0f);
    } else {
        s.set_normalizedTime(Get<float>("animTime"));
        SetDragState(Get<float>("animTime"));
        if (aniState() == animationStates::MOVING) {
            s.speed = Get<float>("animSpeed");
            anim->Play(Str("animationName"));
            onMove->Fire(Get<float>("animTime"), (Get<float>("animSpeed") > 0.0f) ? 1.0f : 0.0f);
        }
    }
    DeployablesEnvironment::scheduleSetCollisionIgnoresCalls++;
    scalarLerpMaxDelta = f32store(1.0 / fx(s.length));
    toggleAct->active = Get<bool>("actionAvailable");
    toggleEvt->guiActive = Get<bool>("eventAvailableFlight");
    toggleEvt->guiActiveEditor = Get<bool>("eventAvailableEditor");
    toggleEvt->guiActiveUnfocused = Get<bool>("eventAvailableEVA");
    toggleEvt->unfocusedRange = Get<float>("evaDistance");  // BaseEvent.unfocusedRange
    Fields()["deployPercent"]->guiActiveEditor = Get<bool>("allowDeployLimit") && Get<bool>("eventAvailableEditor");
    Fields()["deployPercent"]->guiActive = Get<bool>("allowDeployLimit") && Get<bool>("eventAvailableFlight");
    lastClamp = Get<float>("deployPercent");
    Fields()["status"]->guiActive = Get<bool>("showStatus");
    Set<std::string>("status", Localizer::Format(Get<std::string>("status")));
    if (!Get<bool>("allowManualControl")) {
        toggleAct->active = false;
        toggleEvt->guiActive = false;
    }
    CheckCrewState();
}

// ModuleAnimateGeneric.cs:309
void ModuleAnimateGeneric::FindAnimation() {
    auto array = FindModelAnimators(*this, Str("animationName"));
    if (!array.empty()) anim = array[0];
    if (!anim) {
        log::Info("ModuleAnimateGeneric: Could not find animation component '" + Str("animationName") + "' - on part " + (part ? part->gameObjectName : "") +
                  " failed to load! Check the model file");
        behaviourEnabled = false;
    } else if ((*anim)[Str("animationName")] == nullptr) {
        log::Info("ModuleAnimateGeneric: Could not find animation " + Str("animationName") + " in part's animation component. Check the animationName and model file");
        behaviourEnabled = false;
    }
}

// ModuleAnimateGeneric.cs:358
void ModuleAnimateGeneric::OnStoredInInventory() {
    Set<bool>("animSwitch", true);
    Set<float>("animTime", 0.0f);
}

// ModuleAnimateGeneric.cs:365
void ModuleAnimateGeneric::CheckForRestrictions() {
    bool flag = false;
    bool flag2 = false;
    if (!Str("restrictedNode").empty()) {
        Part* p = PartOf(*this);
        AttachNode* node = p != nullptr ? p->FindAttachNode(Str("restrictedNode")) : nullptr;
        if (node == nullptr) throw Nre();
        flag = node->attachedPart != nullptr;
    }
    const Part* p = PartOf(*this);
    if (Get<int32_t>("CrewCapacity") > 0 && DeployablesEnvironment::SceneIsFlight() && p != nullptr && !p->protoModuleCrew.empty()) flag2 = true;
    const bool disabled = Get<bool>("animationIsDisabled") || animationIsDisabledByVariant;
    toggleEvt->active = !disabled && aniState() != animationStates::MOVING && !flag && !flag2;
    toggleAct->active = !disabled && aniState() != animationStates::MOVING && !flag2;
}

// ModuleAnimateGeneric.cs:381
void ModuleAnimateGeneric::FixedUpdate() {
    if (!anim || (*anim)[Str("animationName")] == nullptr) return;
    CheckForRestrictions();
    AnimationState& s = *GetState();
    float num = f32store(fx(Get<float>("deployPercent")) * 0.01);
    if (num > 0.995f) num = 1.0f;
    if (Get<bool>("revClampPercent")) num = f32store(1.0 - fx(num));
    if (Mathf::Abs(f32store(fx(lastClamp) - fx(Get<float>("deployPercent")))) > 0.1f) {
        lastClamp = Get<float>("deployPercent");
        s.set_normalizedTime(num);
        Set<float>("animTime", num);
        s.speed = 0.0f;
        Set<float>("animSpeed", s.speed);
        anim->Play(Str("animationName"));
        set_aniState(animationStates::CLAMPED);
        Set<std::string>("status", Localizer::Format("#autoLOC_215639"));
        SetDragState(Get<float>("animTime"));
        if (Get<bool>("revClampDirection")) {
            if (Get<float>("deployPercent") > 0.0f) {
                Set<bool>("animSwitch", false);
                toggleEvt->SetGuiName(Get<std::string>("endEventGUIName"));
            } else {
                Set<bool>("animSwitch", true);
                toggleEvt->SetGuiName(Get<std::string>("startEventGUIName"));
            }
            onStop->Fire(f32store(1.0 - fx(Get<float>("deployPercent"))));
        } else {
            if (Get<float>("deployPercent") == 0.0f) {
                Set<bool>("animSwitch", false);
                toggleEvt->SetGuiName(Get<std::string>("endEventGUIName"));
            } else {
                Set<bool>("animSwitch", true);
                toggleEvt->SetGuiName(Get<std::string>("startEventGUIName"));
            }
            onStop->Fire(Get<float>("deployPercent"));
        }
        return;
    }
    if (Get<bool>("allowDeployLimit")) {
        bool flag = false;
        bool flag2 = false;
        if (Get<float>("animSpeed") == 0.0f && anim->IsPlaying(Str("animationName"))) {
            s.set_normalizedTime(num);
            Set<float>("animTime", s.normalizedTime());
        }
        if (Get<bool>("revClampDirection") && Get<float>("animTime") >= num) flag = true;
        if (!Get<bool>("revClampDirection") && Get<float>("animTime") <= num) flag = true;
        if (Get<bool>("revClampSpeed") && s.speed < 0.0f) flag2 = true;
        if (!Get<bool>("revClampSpeed") && s.speed > 0.0f) flag2 = true;
        if (flag && flag2) s.speed = 0.0f;
    }
    AnimationHandler();
}

// ModuleAnimateGeneric.cs:471
void ModuleAnimateGeneric::AnimationHandler() {
    AnimationState& s = *GetState();
    if (aniState() == animationStates::CLAMPED) {
        float num = f32store(fx(Get<float>("deployPercent")) / 100.0);
        if (Get<bool>("revClampPercent")) num = f32store(1.0 - fx(num));
        if (s.normalizedTime() != num) set_aniState(animationStates::MOVING);
        Set<std::string>("status", Localizer::Format("#autoLOC_215506"));
    }
    if (aniState() == animationStates::MOVING) {
        if (!anim->IsPlaying(Str("animationName"))) {
            if (Get<bool>("isOneShot")) {
                set_aniState(animationStates::FIXED);
                Set<std::string>("status", Localizer::Format("#autoLOC_215714"));
            } else {
                set_aniState(animationStates::LOCKED);
                Set<std::string>("status", Localizer::Format("#autoLOC_215719"));
            }
            if (!Get<bool>("animSwitch")) {
                s.set_normalizedTime(1.0f);
                Set<float>("animTime", 1.0f);
                SetDragState(1.0f);
            } else {
                s.set_normalizedTime(0.0f);
                Set<float>("animTime", 0.0f);
                SetDragState(0.0f);
            }
            DeployablesEnvironment::scheduleSetCollisionIgnoresCalls++;  // part.SetCollisionIgnores()
            onStop->Fire(Get<float>("animTime"));
        } else {
            Set<float>("animSpeed", s.speed);
            Set<float>("animTime", s.normalizedTime());
            SetDragState(Get<float>("animTime"));
        }
    }
    if (stopAnimation) {
        anim->Stop(Str("animationName"));
        DeployablesEnvironment::scheduleSetCollisionIgnoresCalls++;
        stopAnimation = false;
        onStop->Fire(s.normalizedTime());
    }
    if (aniState() != animationStates::LOCKED && Get<float>("animSpeed") == 0.0f && anim->IsPlaying(Str("animationName")) && !isManuallyMoving) {
        Set<std::string>("status", Localizer::Format("#autoLOC_215760"));
        set_aniState(animationStates::CLAMPED);
    }
    CheckCrewState();
}

// ModuleAnimateGeneric.cs:537
void ModuleAnimateGeneric::SetScalar(float t) {
    if (Get<bool>("isOneShot") && aniState() == animationStates::FIXED) {
        Set<float>("animTime", t);
        return;
    }
    AnimationState& s = *GetState();
    const std::string name = Str("animationName");
    if (anim->IsPlaying(name)) {
        anim->Stop(name);
        DeployablesEnvironment::scheduleSetCollisionIgnoresCalls++;
        onStop->Fire(s.normalizedTime());
    }
    s.speed = 0.0f;
    s.enabled = true;
    s.weight = 1.0f;
    DeployablesEnvironment::scheduleSetCollisionIgnoresCalls++;
    const Part* p = PartOf(*this);
    if (!DeployablesEnvironment::SceneIsEditor() && p != nullptr && p->ShieldedFromAirstream() && !Get<bool>("allowAnimationWhileShielded")) {
        AnimationModulesEnvironment::screenMessages.push_back(
            Localizer::Format("#autoLOC_215802", std::vector<std::string>{p->partInfo != nullptr ? p->partInfo->title : std::string()}));
        return;
    }
    t = Mathf::MoveTowards(Get<float>("animTime"), t, f32store(fx(scalarLerpMaxDelta) * fx(Time::deltaTime)));
    s.set_normalizedTime(t);
    Set<float>("animTime", t);
    if (Mathf::Abs(f32store(fx(t) - 1.0)) < 0.01f) {
        isManuallyMoving = false;
        if (Get<bool>("isOneShot")) {
            set_aniState(animationStates::FIXED);
            Set<std::string>("status", Localizer::Format("#autoLOC_215714"));
            return;
        }
        Set<bool>("animSwitch", false);
        set_aniState(animationStates::LOCKED);
        Set<std::string>("status", Localizer::Format("#autoLOC_215719"));
        toggleEvt->SetGuiName(Get<std::string>("endEventGUIName"));
    } else if (Mathf::Abs(t) < 0.01f) {
        isManuallyMoving = false;
        Set<bool>("animSwitch", true);
        set_aniState(animationStates::LOCKED);
        Set<std::string>("status", Localizer::Format("#autoLOC_215719"));
        toggleEvt->SetGuiName(Get<std::string>("startEventGUIName"));
        toggleEvt->active = true;
    } else {
        set_aniState(animationStates::MOVING);
        isManuallyMoving = true;
        Set<std::string>("status", Localizer::Format("#autoLOC_215506"));
        Set<bool>("animSwitch", t < Get<float>("animTime"));
        toggleEvt->active = false;
    }
}

// ModuleAnimateGeneric.cs:595
void ModuleAnimateGeneric::SetUIRead(bool state) { Fields()["status"]->guiActive = state && Get<bool>("showStatus"); }

// ModuleAnimateGeneric.cs:600
void ModuleAnimateGeneric::SetUIWrite(bool state) {
    toggleEvt->guiActive = state && Get<bool>("allowManualControl");
    toggleEvt->guiActiveUnfocused = state && Get<bool>("allowManualControl");
}

// ModuleAnimateGeneric.cs:606
bool ModuleAnimateGeneric::IsMoving() {
    if (anim->isPlaying()) return Mathf::Abs(GetState()->speed) != 0.0f;
    return false;
}

// ModuleAnimateGeneric.cs:615
void ModuleAnimateGeneric::SetDragState(float b) {
    if (part == nullptr) return;
    DragCubeList& c = part->DragCubes;
    c.SetCubeWeight("A", b);
    c.SetCubeWeight("B", f32store(1.0 - fx(b)));
    if (c.Procedural()) c.ForceUpdate(true, true);
}

// ModuleAnimateGeneric.cs:630
void ModuleAnimateGeneric::AssumeDragCubePosition(const std::string& name) {
    auto array = FindModelAnimators(*this, Str("animationName"));
    if (!array.empty()) anim = array[0];
    if (!anim) {
        log::Info("ModuleAnimateGeneric: Could not find animation component - on part " + (part ? part->gameObjectName : "") + " failed to load! Check the model file");
        behaviourEnabled = false;
        return;
    }
    if ((*anim)[Str("animationName")] == nullptr) {
        log::Info("ModuleAnimateGeneric: Could not find animation " + Str("animationName") + " in part's animation component. Check the animationName and model file");
        behaviourEnabled = false;
        return;
    }
    AnimationState& s = *GetState();
    s.speed = 0.0f;
    s.enabled = true;
    s.weight = 1.0f;
    if (name != "A") {
        if (name == "B") s.set_normalizedTime(0.0f);
    } else {
        s.set_normalizedTime(1.0f);
    }
}

// ModuleAnimateGeneric.cs:669
void ModuleAnimateGeneric::OnSave(ConfigNode& node) {
    PartModule::OnSave(node);
    if (node.HasValue("deployPercent") && revertDeployPercentOnSave >= 0.0f) {
        node.SetValue("deployPercent", revertDeployPercentOnSave);
        node.SetValue("animSpeed", revertAnimSpeedOnSave);
        set_aniState(animationStates::MOVING);
        node.SetValue("aniState", EnumName("aniState"));
    }
}

// ModuleAnimateGeneric.cs:681
void ModuleAnimateGeneric::CheckCrewState() {
    if (Get<int32_t>("CrewCapacity") > 0) {
        Part* p = PartOf(*this);
        if (p == nullptr) throw Nre();
        const int crewCapacity = p->fields.CrewCapacity;
        const bool flag = Mathf::Approximately(GetState()->normalizedTime(), 1.0f);
        if (flag && p->fields.CrewCapacity == 0) p->fields.CrewCapacity = Get<int32_t>("CrewCapacity");
        else if (!flag && p->fields.CrewCapacity > 0) p->fields.CrewCapacity = 0;
        if (crewCapacity != p->fields.CrewCapacity) AnimationModulesEnvironment::checkTransferDialogCalls++;
    }
}

// ModuleAnimateGeneric.cs:708
void ModuleAnimateGeneric::FailAnimationForCargoBay(float newDeployPercent, bool freezeMidMovement) {
    if (freezeMidMovement) {
        revertDeployPercentOnSave = newDeployPercent;
        revertAnimSpeedOnSave = Get<float>("animSpeed");
    }
    originalAllowManualControl = Get<bool>("allowManualControl");
    Set<bool>("allowManualControl", false);
    originalDeployPercentGUIActive = Fields()["deployPercent"]->guiActive;
    Fields()["deployPercent"]->guiActive = false;
}

// ModuleAnimateGeneric.cs:721
void ModuleAnimateGeneric::RestartFailedAnimationForCargoBay() {
    if (revertDeployPercentOnSave > 0.0f) {
        if (Get<bool>("animSwitch") && revertAnimSpeedOnSave > 0.0f && Get<float>("deployPercent") > 0.0f) {
            Set<bool>("animSwitch", false);
            toggleEvt->SetGuiName(Get<std::string>("endEventGUIName"));
        } else if (!Get<bool>("revClampPercent") && !Get<bool>("animSwitch") && revertAnimSpeedOnSave < 0.0f && Get<float>("deployPercent") == 0.0f) {
            Set<bool>("animSwitch", true);
            toggleEvt->SetGuiName(Get<std::string>("startEventGUIName"));
        }
        Set<float>("deployPercent", revertDeployPercentOnSave);
        lastClamp = Get<float>("deployPercent");
        Set<float>("animSpeed", revertAnimSpeedOnSave);
        GetState()->speed = Get<float>("animSpeed");
        set_aniState(animationStates::MOVING);
        revertDeployPercentOnSave = -1.0f;
    }
    Set<bool>("allowManualControl", originalAllowManualControl);
    Fields()["deployPercent"]->guiActive = originalDeployPercentGUIActive;
}

// ============================================== ModuleAnimationGroup ==============================================================

ModuleAnimationGroup::~ModuleAnimationGroup() { OnDestroy(); }

std::string ModuleAnimationGroup::Str(std::string_view field) const { return StrField(*this, field); }

void ModuleAnimationGroup::OnAwake() {
    PartModule::OnAwake();
    Events().Bind("DeployModule", BaseEvent::Handler([this] { DeployModule(); }));
    Events().Bind("RetractModule", BaseEvent::Handler([this] { RetractModule(); }));
    Actions().Bind("DeployModuleAction", [this](KSPActionParam& p) { DeployModuleAction(p); });
    Actions().Bind("RetractModuleAction", [this](KSPActionParam& p) { RetractModuleAction(p); });
    Actions().Bind("ToggleModuleAction", [this](KSPActionParam& p) { ToggleModuleAction(p); });
}

// ModuleAnimationGroup.cs:89
void ModuleAnimationGroup::DeployModule() {
    AnimationModulesEnvironment::partEffects.emplace_back(Str("deployEffectName"), 1.0f);
    SetDeployedState(1, true);
}

// ModuleAnimationGroup.cs:96
void ModuleAnimationGroup::RetractModule() {
    AnimationModulesEnvironment::partEffects.emplace_back(Str("activeEffectName"), 0.0f);
    AnimationModulesEnvironment::partEffects.emplace_back(Str("retractEffectName"), 1.0f);
    SetRetractedState(-1, true);
}

// ModuleAnimationGroup.cs:104
void ModuleAnimationGroup::DeployModuleAction(KSPActionParam&) {
    if (!IsDeployed()) DeployModule();
}

// ModuleAnimationGroup.cs:113
void ModuleAnimationGroup::RetractModuleAction(KSPActionParam&) {
    if (IsDeployed()) RetractModule();
}

// ModuleAnimationGroup.cs:122
void ModuleAnimationGroup::ToggleModuleAction(KSPActionParam&) {
    if (IsDeployed()) RetractModule();
    else DeployModule();
}

// ModuleAnimationGroup.cs:134
void ModuleAnimationGroup::Start() {
    FindModules();
    FindAnimations();
    StopAnimations();
    CheckAnimationState();
    if (Str("deployAnimationName") != "") (*DeployAnimation)[Str("deployAnimationName")]->layer = 3;
    if (Str("activeAnimationName") != "") (*ActiveAnimation)[Str("activeAnimationName")]->layer = 4;
    if (Str("deactivateAnimationName") != "") (*DeactivateAnimation)[Str("deactivateAnimationName")]->layer = 4;
    if (Get<bool>("autoDeploy") || Get<bool>("suppressActionsEvents")) {
        Events()["DeployModule"]->active = false;
        Events()["RetractModule"]->active = false;
        Actions()["DeployModuleAction"]->active = false;
        Actions()["RetractModuleAction"]->active = false;
        Actions()["ToggleModuleAction"]->active = false;
        if (Get<bool>("autoDeploy")) {
            Set<bool>("isDeployed", true);
            Set<bool>("suppressActionsEvents", true);
        }
    }
    if (!Get<bool>("displayActions")) {
        Actions()["DeployModuleAction"]->active = false;
        Actions()["RetractModuleAction"]->active = false;
        Actions()["ToggleModuleAction"]->active = false;
    }
    Setup();
}

// ModuleAnimationGroup.cs:174
void ModuleAnimationGroup::OnStart(StartState state) {
    PartModule::OnStart(state);
    if (!subscribedPause) {
        GE::onGamePause.Add(this, &ModuleAnimationGroup::OnPause);
        GE::onGameUnpause.Add(this, &ModuleAnimationGroup::OnUnpause);
        subscribedPause = true;
    }
}

// ModuleAnimationGroup.cs:181 (the snapshot value write is the host's)
void ModuleAnimationGroup::OnInventoryModeDisable() {
    if (DeployablesEnvironment::SceneIsFlight() && !IsDeployed()) SetRetractedState(-10000, true);
}

// ModuleAnimationGroup.cs:198
void ModuleAnimationGroup::OnStoredInInventory() {
    Part* p = PartOf(*this);
    if (p == nullptr || p->vessel == nullptr) SetRetractedState(-10000, true);
    Set<bool>("isDeployed", false);
}

// ModuleAnimationGroup.cs:208
void ModuleAnimationGroup::OnDestroy() {
    if (subscribedPause) {
        GE::onGamePause.Remove(this, &ModuleAnimationGroup::OnPause);
        GE::onGameUnpause.Remove(this, &ModuleAnimationGroup::OnUnpause);
        subscribedPause = false;
    }
}

std::shared_ptr<Animation> ModuleAnimationGroup::FirstAnimator(const std::string& clip) const {
    auto v = FindModelAnimators(const_cast<ModuleAnimationGroup&>(*this), clip);
    if (v.empty()) throw std::out_of_range("Index was outside the bounds of the array.");
    return v[0];
}

// ModuleAnimationGroup.cs:214
void ModuleAnimationGroup::FindAnimations() {
    if (!DeployAnimation) DeployAnimation = Str("deployAnimationName") == "" ? nullptr : FirstAnimator(Str("deployAnimationName"));
    if (!ActiveAnimation) ActiveAnimation = Str("activeAnimationName") == "" ? nullptr : FirstAnimator(Str("activeAnimationName"));
    if (!DeactivateAnimation) DeactivateAnimation = Str("deactivateAnimationName") == "" ? nullptr : FirstAnimator(Str("deactivateAnimationName"));
}

// ModuleAnimationGroup.cs:230
void ModuleAnimationGroup::StopAnimations() {
    for (auto& a : FindModelAnimators(*this)) a->Stop();
}

// ModuleAnimationGroup.cs:240
void ModuleAnimationGroup::LateUpdate() {
    if (!_lateSetupComplete) LateSetup();
}

// ModuleAnimationGroup.cs:248
void ModuleAnimationGroup::LateSetup() {
    _lateSetupComplete = true;
    if (IsDeployed()) EnableModules();
    else DisableModules();
}

// ModuleAnimationGroup.cs:262
void ModuleAnimationGroup::Setup() {
    const std::string moduleType = Get<std::string>("moduleType");
    Events()["DeployModule"]->SetGuiName(Localizer::Format(Get<std::string>("deployActionName"), std::vector<std::string>{moduleType}));
    Events()["RetractModule"]->SetGuiName(Localizer::Format(Get<std::string>("retractActionName"), std::vector<std::string>{moduleType}));
    Actions()["DeployModuleAction"]->SetGuiName(Localizer::Format(Get<std::string>("deployActionName"), std::vector<std::string>{moduleType}));
    Actions()["RetractModuleAction"]->SetGuiName(Localizer::Format(Get<std::string>("retractActionName"), std::vector<std::string>{moduleType}));
    Actions()["ToggleModuleAction"]->SetGuiName(Localizer::Format(Get<std::string>("toggleActionName"), std::vector<std::string>{moduleType}));
}

// ModuleAnimationGroup.cs:271
void ModuleAnimationGroup::OnLoad(const ConfigNode&) {
    FindModules();
    FindAnimations();
    CheckAnimationState();
}

// ModuleAnimationGroup.cs:278 (the C# wraps the body in try / catch (Exception): the failure is only printed)
void ModuleAnimationGroup::Update() {
    if (!DeployablesEnvironment::SceneIsFlight()) return;
    try {
        Part* p = PartOf(*this);
        if (p != nullptr && p->vessel != nullptr) {
            if (_animationLock) {
                if (!DeployAnimation->IsPlaying(Str("deployAnimationName"))) {
                    _animationLock = false;
                    Fields()["animationStatus"]->guiActive = false;
                    _postAnimationMethod();
                    GE::OnAnimationGroupRetractComplete.Fire(this);
                }
            } else if (IsDeployed() && p->partInfo != nullptr && p->partInfo->name != "groundAnchor") {
                CheckForActivity();
            }
            UpdateAnimationTime();
        }
        PartModule::OnUpdate();
    } catch (const std::exception&) {
        AnimationModulesEnvironment::printed.push_back("[RESOURCES] - ERROR IN OnUpdate of ModuleAnimationGroup");
    }
}

// ModuleAnimationGroup.cs:313
void ModuleAnimationGroup::UpdateAnimationTime() {
    if (!Get<bool>("realTimeAnimation") || !(AnimationModulesEnvironment::unscaledDeltaTime > Time::deltaTime)) return;
    AnimationState* s = GetCurrentPlayingAnimationState();
    if (s != nullptr) {
        if (s->speed > 0.0f) s->time = f32store((fx(AnimationModulesEnvironment::unscaledTime) - fx(animationStartTime)) / fx(s->speed));
        else if (s->speed < 0.0f) s->time = f32store(fx(s->length) + (fx(AnimationModulesEnvironment::unscaledTime) - fx(animationStartTime)) / fx(s->speed));
    }
}

// ModuleAnimationGroup.cs:332
AnimationState* ModuleAnimationGroup::GetCurrentPlayingAnimationState() {
    AnimationState* result = nullptr;
    if (DeployAnimation && savedAnimationName == Str("deployAnimationName")) result = (*DeployAnimation)[Str("deployAnimationName")];
    if (DeactivateAnimation && savedAnimationName == Str("deactivateAnimationName")) result = (*DeactivateAnimation)[Str("deactivateAnimationName")];
    if (ActiveAnimation && savedAnimationName == Str("activeAnimationName")) result = (*ActiveAnimation)[Str("activeAnimationName")];
    return result;
}

// ModuleAnimationGroup.cs:350
void ModuleAnimationGroup::OnPause() {
    if (Get<bool>("realTimeAnimation")) {
        pauseStartTime = AnimationModulesEnvironment::unscaledTime;
        AnimationState* s = GetCurrentPlayingAnimationState();
        if (s != nullptr) s->speed = 0.0f;
    }
}

// ModuleAnimationGroup.cs:363
void ModuleAnimationGroup::OnUnpause() {
    if (Get<bool>("realTimeAnimation")) {
        animationStartTime = f32store(fx(animationStartTime) + (fx(AnimationModulesEnvironment::unscaledTime) - fx(pauseStartTime)));
        AnimationState* s = GetCurrentPlayingAnimationState();
        if (s != nullptr) s->speed = savedAnimationSpeed;
    }
}

// ModuleAnimationGroup.cs:376
void ModuleAnimationGroup::CheckAnimationState() {
    if (IsDeployed()) SetDeployedState(1000, false);
    else SetRetractedState(-1000, false);
    ToggleEmmitters(false);
}

// ModuleAnimationGroup.cs:389
void ModuleAnimationGroup::FindModules() {
    Part* p = PartOf(*this);
    if (p != nullptr && p->vessel != nullptr) {
        _Modules.clear();
        for (PartModule* m : p->FindModulesImplementing([](PartModule& pm) { return pm.AsAnimatedModule() != nullptr; })) _Modules.push_back(m->AsAnimatedModule());
        hasModules = true;
    }
}

// ModuleAnimationGroup.cs:397
void ModuleAnimationGroup::CheckForActivity() {
    if (!hasModules) throw Nre();
    for (IAnimatedModule* m : _Modules) {
        if ((m->ModuleIsActive() || Get<bool>("alwaysActive")) && IsDeployed()) {
            ToggleActiveState(1, true);
            return;
        }
    }
    ToggleActiveState(-1, false);
}

// ModuleAnimationGroup.cs:418
void ModuleAnimationGroup::ToggleActiveState(int speed, bool state) {
    if (DeployablesEnvironment::SceneIsEditor()) speed *= 1000;
    try {
        const std::string active = Str("activeAnimationName"), deactivate = Str("deactivateAnimationName");
        if (active != "" && !ActiveAnimation->isPlaying() && state) {
            AnimationModulesEnvironment::partEffects.emplace_back(Str("activeEffectName"), 1.0f);
            savedAnimationSpeed = static_cast<float>(speed);
            animationStartTime = AnimationModulesEnvironment::unscaledTime;
            savedAnimationName = active;
            (*ActiveAnimation)[active]->speed = static_cast<float>(speed);
            ActiveAnimation->Play(active);
            if (speed > 0) SetDragCubes("ACTIVE_A");
            else SetDragCubes("ACTIVE_B");
            ToggleEmmitters(state);
        } else if (deactivate != "" && !DeactivateAnimation->isPlaying() && !state) {
            savedAnimationSpeed = static_cast<float>(speed);
            animationStartTime = AnimationModulesEnvironment::unscaledTime;
            savedAnimationName = deactivate;
            (*DeactivateAnimation)[deactivate]->speed = static_cast<float>(speed);
            DeactivateAnimation->Play(deactivate);
            if (speed > 0) SetDragCubes("DEACTIVATE_A");
            else SetDragCubes("DEACTIVATE_B");
            ToggleEmmitters(state);
        } else {
            ToggleEmmitters(state);
        }
        if (active != "" && Get<bool>("stopActiveAnimIfDisabled") && ActiveAnimation->isPlaying() && !state) {
            AnimationModulesEnvironment::partEffects.emplace_back(Str("activeEffectName"), 0.0f);
            ActiveAnimation->Stop();
        }
    } catch (const std::exception&) {
        AnimationModulesEnvironment::printed.push_back("[RESOURCES] ERROR in ToggleActiveState of ModuleAnimationGroup");
    }
}

// ModuleAnimationGroup.cs:477 (KSPParticleEmitter.emit / enabled of the model's emitters: the engine's; the last state is recorded)
void ModuleAnimationGroup::ToggleEmmitters(bool state) { emittersEnabled = state; }

// ModuleAnimationGroup.cs:488
void ModuleAnimationGroup::SetRetractedState(int speed, bool resetAnimationTime) {
    if (DeployablesEnvironment::SceneIsEditor()) speed *= 1000;
    Set<bool>("isDeployed", false);
    Events()["RetractModule"]->active = false;
    DisableModules();
    GE::OnAnimationGroupStateChanged.Fire(this, false);
    if (Str("deployAnimationName") != "") {
        PlayDeployAnimation(speed, [this] { RetractComplete(); }, resetAnimationTime);
        SetDragCubes("DEPLOY_A");
    } else {
        RetractComplete();
    }
}

// ModuleAnimationGroup.cs:510
void ModuleAnimationGroup::DeployComplete() {
    deployCompleteCalls++;
    if (!Get<bool>("suppressActionsEvents")) Events()["RetractModule"]->active = true;
    EnableModules();
    GE::OnAnimationGroupStateChanged.Fire(this, true);
}

// ModuleAnimationGroup.cs:521
void ModuleAnimationGroup::RetractComplete() {
    retractCompleteCalls++;
    if (!Get<bool>("suppressActionsEvents")) Events()["DeployModule"]->active = true;
}

// ModuleAnimationGroup.cs:529
void ModuleAnimationGroup::SetDeployedState(int speed, bool resetAnimationTime) {
    if (DeployablesEnvironment::SceneIsEditor()) speed *= 1000;
    Set<bool>("isDeployed", true);
    Events()["DeployModule"]->active = false;
    if (Str("deployAnimationName") != "") {
        PlayDeployAnimation(speed, [this] { DeployComplete(); }, resetAnimationTime);
        SetDragCubes("DEPLOY_B");
    } else {
        DeployComplete();
    }
}

// ModuleAnimationGroup.cs:548
void ModuleAnimationGroup::SetDragCubes(const std::string& cube) {
    if (part == nullptr) return;
    DragCubeList& c = part->DragCubes;
    if (!Str("deployAnimationName").empty()) {
        c.SetCubeWeight("DEPLOY_A", 0.0f);
        c.SetCubeWeight("DEPLOY_B", 0.0f);
    }
    if (!Str("deactivateAnimationName").empty()) {
        c.SetCubeWeight("DEACTIVATE_A", 0.0f);
        c.SetCubeWeight("DEACTIVATE_B", 0.0f);
    }
    if (!Str("activeAnimationName").empty()) {
        c.SetCubeWeight("ACTIVE_A", 0.0f);
        c.SetCubeWeight("ACTIVE_B", 0.0f);
    }
    c.SetCubeWeight(cube, 1.0f);
}

// ModuleAnimationGroup.cs:568
void ModuleAnimationGroup::PlayDeployAnimation(int speed, std::function<void()> postAnimate, bool resetAnimationTime) {
    const std::string deploy = Str("deployAnimationName");
    if (speed < 0) {
        if (Str("activeAnimationName") != "") ActiveAnimation->Stop(Str("activeAnimationName"));
        if (Str("deactivateAnimationName") != "") DeactivateAnimation->Stop(Str("deactivateAnimationName"));
        if (resetAnimationTime && deploy != "") {
            AnimationState& s = *(*DeployAnimation)[deploy];
            s.time = s.length;
        }
    }
    if (deploy != "") {
        if (DeployablesEnvironment::SceneIsFlight()) {
            _animationLock = true;
            Set<std::string>("animationStatus", Localizer::Format("#autoLOC_215840"));
            Fields()["animationStatus"]->guiActive = true;
            _postAnimationMethod = postAnimate;
        } else {
            postAnimate();
        }
        if (resetAnimationTime) {
            savedAnimationSpeed = static_cast<float>(speed);
            animationStartTime = AnimationModulesEnvironment::unscaledTime;
            savedAnimationName = deploy;
        }
        (*DeployAnimation)[deploy]->speed = static_cast<float>(speed);
        DeployAnimation->Play(deploy);
    }
}

// ModuleAnimationGroup.cs:609
void ModuleAnimationGroup::DisableModules() {
    Part* p = PartOf(*this);
    if (!(p == nullptr || p->vessel == nullptr) && hasModules) {
        for (IAnimatedModule* m : _Modules) m->DisableModule();
        ToggleEmmitters(false);
    }
}

// ModuleAnimationGroup.cs:622
void ModuleAnimationGroup::EnableModules() {
    Part* p = PartOf(*this);
    if (p == nullptr || p->vessel == nullptr || !hasModules) return;
    for (IAnimatedModule* m : _Modules)
        if (m->IsSituationValid()) m->EnableModule();
}

// ModuleAnimationGroup.cs:639
std::vector<std::string> ModuleAnimationGroup::GetDragCubeNames() const {
    std::vector<std::string> list;
    if (!Str("deployAnimationName").empty()) {
        list.push_back("DEPLOY_A");
        list.push_back("DEPLOY_B");
    }
    if (!Str("deactivateAnimationName").empty()) {
        list.push_back("DEACTIVATE_A");
        list.push_back("DEACTIVATE_B");
    }
    if (!Str("activeAnimationName").empty()) {
        list.push_back("ACTIVE_A");
        list.push_back("ACTIVE_B");
    }
    return list;
}

// ModuleAnimationGroup.cs:660 (the C# tests `DeployAnimation != null` before every ActiveAnimation / DeactivateAnimation access, sic)
void ModuleAnimationGroup::AssumeDragCubePosition(const std::string& name) {
    auto setup = [&](const std::shared_ptr<Animation>& a, const std::string& clip, float t) {
        if (!DeployAnimation) return;
        AnimationState& s = *(*a)[clip];
        s.set_normalizedTime(t);
        s.set_normalizedSpeed(0.0f);
        s.enabled = true;
        s.weight = 1.0f;
        a->Play(clip);
    };
    if (name == "ACTIVE_B") setup(ActiveAnimation, Str("activeAnimationName"), 1.0f);
    else if (name == "ACTIVE_A") setup(ActiveAnimation, Str("activeAnimationName"), 0.0f);
    else if (name == "DEACTIVATE_B") setup(DeactivateAnimation, Str("deactivateAnimationName"), 1.0f);
    else if (name == "DEACTIVATE_A") setup(DeactivateAnimation, Str("deactivateAnimationName"), 0.0f);
    else if (name == "DEPLOY_B") setup(DeployAnimation, Str("deployAnimationName"), 1.0f);
    else if (name == "DEPLOY_A") setup(DeployAnimation, Str("deployAnimationName"), 0.0f);
}

// ============================================== ModuleAnimatorLandingGear =========================================================

void ModuleAnimatorLandingGear::OnAwake() {
    PartModule::OnAwake();
    Events().Bind("InputGearToggle", BaseEvent::Handler([this] { InputGearToggle(); }));
}

// ModuleAnimatorLandingGear.cs:10
void ModuleAnimatorLandingGear::InputGearToggle() {
    if (Get<float>("gearExtension") == 0.0f) {
        Set<float>("gearExtension", 1.0f);
        Events()["InputGearToggle"]->SetGuiName("#autoLOC_6001869");
    } else {
        Set<float>("gearExtension", 0.0f);
        Events()["InputGearToggle"]->SetGuiName("#autoLOC_6001870");
    }
}

// ModuleAnimatorLandingGear.cs:21
void ModuleAnimatorLandingGear::OnStart(StartState) {
    if (Get<float>("gearExtension") == 0.0f) Events()["InputGearToggle"]->SetGuiName("#autoLOC_6001870");
    else Events()["InputGearToggle"]->SetGuiName("#autoLOC_6001869");
}

namespace detail {
void RegisterAnimationModules() {
    ModuleRegistrar<ModuleAnimationSetter>("ModuleAnimationSetter");
    ModuleRegistrar<ModuleAnimateGeneric>("ModuleAnimateGeneric");
    ModuleRegistrar<ModuleAnimationGroup>("ModuleAnimationGroup");
    ModuleRegistrar<ModuleAnimatorLandingGear>("ModuleAnimatorLandingGear");
}
}  // namespace detail

}  // namespace ksp
