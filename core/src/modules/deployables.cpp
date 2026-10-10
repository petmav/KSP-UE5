// WP-27: ModuleDeployablePart, ModuleDeployableAntenna, ModuleDeployableRadiator, ModuleDeployableSolarPanel. See ksp/modules/deployables.hpp.
// Every function cites File.cs:line of decompiled/KSP-Decompiled/Assembly-CSharp (tools/wp28/check_cites.py verifies the citations).
#include "ksp/modules/deployables.hpp"

#include <algorithm>

#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/math/util_math.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/parts/cs_parse.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/parts/part_module_data.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/resources/resource_flow_host.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/util/ksp_util.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

using GE = events::GameEvents;

namespace {

constexpr float kDeg2Rad = 0.017453292f;  // (float)Math.PI / 180f folded by the C# compiler
constexpr float kInvPi = 1.0f / 3.14159274f;  // 1f / (float)Math.PI
}  // namespace

// ---------------------------------------------------------------------------------------------------------------------------------

void DeployablesEnvironment::Reset() {
    loadedSceneIsMissionBuilder = nullptr;
    modelAnimations = nullptr;
    hasModelTransform = nullptr;
    trackingFrame = nullptr;
    alignPanel = nullptr;
    trackingRaycast = nullptr;
    panelRotation = nullptr;
    panelLocalRotation = nullptr;
    planetariumBody = nullptr;
    partRepairMassPerKit = 0.05f;
    partRepairMaxKitAmount = 4;
    secondaryTransformAltitude = nullptr;
    partActionUIContains = nullptr;
    convertBreakObjects = nullptr;
    screenMessages.clear();
    scheduleSetCollisionIgnoresCalls = 0;
    breakNotificationCalls = 0;
    flightLog.clear();
    noCrashDamage = false;
}
bool DeployablesEnvironment::SceneIsEditor() { return ResourceFlowEnvironment::LoadedSceneIsEditor(); }
bool DeployablesEnvironment::SceneIsFlight() { return ResourceFlowEnvironment::LoadedSceneIsFlight(); }

// ================================================= ModuleDeployablePart ===========================================================

ModuleDeployablePart::ModuleDeployablePart(const ModuleClassInfo& cls) : PartModule(cls) {}

ModuleDeployablePart::~ModuleDeployablePart() {
    // ModuleDeployablePart.cs:660 OnDestroy
    GE::onVesselChange.Remove(this, &ModuleDeployablePart::onVesselFocusChange);
}

Part* ModuleDeployablePart::PartPtr() const { return dynamic_cast<Part*>(part); }
const ProcessingVessel* ModuleDeployablePart::VesselRecord() const { return ResourceProcessingEnvironment::VesselOf(PartPtr()); }
std::string ModuleDeployablePart::cache(const char* autoLoc) const { return Localizer::Format(autoLoc); }

std::string ModuleDeployablePart::Str(std::string_view field) const { return IsNull(field) ? std::string() : Get<std::string>(field); }
ModuleDeployablePart::DeployState ModuleDeployablePart::deployState() const { return static_cast<DeployState>(Get<EnumValue>("deployState").value); }
void ModuleDeployablePart::set_deployState(DeployState s) {
    EnumValue e = Get<EnumValue>("deployState");
    e.value = static_cast<int64_t>(s);
    Set<EnumValue>("deployState", e);
}
ModuleDeployablePart::TrackingMode ModuleDeployablePart::trackingMode() const { return static_cast<TrackingMode>(Get<EnumValue>("trackingMode").value); }
ModuleDeployablePart::PanelAlignType ModuleDeployablePart::alignType() const { return static_cast<PanelAlignType>(Get<EnumValue>("alignType").value); }

bool ModuleDeployablePart::ShieldedFromAirstream() const {
    const Part* p = PartPtr();
    return p != nullptr && p->ShieldedFromAirstream();
}

AnimationState& ModuleDeployablePart::AnimState() const {
    if (!anim) throw NullReferenceException(std::string("Object reference not set to an instance of an object (") + __func__ + ")");
    AnimationState* s = (*anim)[Str("animationName")];
    if (s == nullptr) throw NullReferenceException(std::string("Object reference not set to an instance of an object (") + __func__ + ")");
    return *s;
}

// ModuleDeployablePart.cs:220
std::string ModuleDeployablePart::ScalarModuleID() const { return Get<std::string>("moduleID"); }

// ModuleDeployablePart.cs:222
float ModuleDeployablePart::GetScalar() const {
    if (ShieldedFromAirstream() && Get<bool>("applyShielding")) return 0.0f;
    if (useAnimation()) {
        switch (deployState()) {
            case DeployState::EXTENDED: return 1.0f;
            case DeployState::RETRACTED:
            case DeployState::BROKEN: return 0.0f;
            default: return AnimState().normalizedTime();
        }
    }
    if (deployState() == DeployState::BROKEN) return 0.0f;
    return 1.0f;
}

// ModuleDeployablePart.cs:251
bool ModuleDeployablePart::CanMove() const {
    if (deployState() != DeployState::BROKEN && !IsDeployablePartStuck()) {
        if (useAnimation()) {
            if (!DeployablesEnvironment::SceneIsEditor() && ShieldedFromAirstream()) {
                if (!Get<bool>("applyShielding")) return !Get<bool>("applyShieldingExtend");
                return false;
            }
            return true;
        }
        return true;
    }
    return false;
}

// ModuleDeployablePart.cs:279
bool ModuleDeployablePart::IsMultipleCubesActive() const {
    if (!useAnimation()) return Get<bool>("isTracking");
    return true;
}

// ModuleDeployablePart.cs:292
void ModuleDeployablePart::ExtendPanelsAction(KSPActionParam& param) {
    if (param.type != 0 && (param.type != static_cast<int>(KSPActionType::Toggle) || deployState() != DeployState::RETRACTED)) Retract();
    else Extend();
}

// ModuleDeployablePart.cs:304
void ModuleDeployablePart::ControlPanelsWithoutUsingSymmetry(KSPActionType action) {
    if (action == KSPActionType::Activate) DoExtend();
    else DoRetract();
}

// ModuleDeployablePart.cs:317
void ModuleDeployablePart::ExtendAction(KSPActionParam&) { Extend(); }

// ModuleDeployablePart.cs:323
void ModuleDeployablePart::RetractAction(KSPActionParam&) { Retract(); }

// ModuleDeployablePart.cs:328 (the symmetry loop: `symmetryCounterparts[count].Modules[Modules.IndexOf(this)] as ModuleDeployablePart`)
void ModuleDeployablePart::Extend() {
    DoExtend();
    if (!Get<bool>("eventsInSymmwtryAlways") && (!DeployablesEnvironment::SceneIsEditor() || !Get<bool>("eventsInSymmwtryEditor"))) return;
    Part* p = PartPtr();
    if (p == nullptr) throw NullReferenceException(std::string("Object reference not set to an instance of an object (") + __func__ + ")");
    int count = static_cast<int>(p->symmetryCounterparts.size());
    const int index = p->Modules().IndexOf(this);
    while (count-- > 0) {
        Part* sym = p->symmetryCounterparts[static_cast<size_t>(count)];
        if (sym == nullptr || index < 0 || static_cast<size_t>(index) >= sym->Modules().Count()) continue;
        if (auto* m = dynamic_cast<ModuleDeployablePart*>(sym->Modules()[static_cast<size_t>(index)])) m->DoExtend();
    }
}

// ModuleDeployablePart.cs:347
void ModuleDeployablePart::DoExtend() {
    if (deployState() != DeployState::RETRACTED) return;
    if (!DeployablesEnvironment::SceneIsEditor() && ShieldedFromAirstream() && Get<bool>("applyShieldingExtend")) {
        DeployablesEnvironment::screenMessages.push_back(Localizer::Format("#autoLOC_234422", std::vector<std::string>{Get<std::string>("partType")}));
        return;
    }
    if (useAnimation()) {
        AnimationState& s = AnimState();
        s.speed = DeployablesEnvironment::SceneIsEditor() ? Get<float>("editorAnimationSpeedMult") : 1.0f;
        s.set_normalizedTime(0.0f);
        s.enabled = true;
        anim->Play(Str("animationName"));
    }
    set_deployState(DeployState::EXTENDING);
    Events()["Extend"]->active = false;
    onMove->Fire(0.0f, 1.0f);
}

// ModuleDeployablePart.cs:371
void ModuleDeployablePart::Retract() {
    DoRetract();
    if (!Get<bool>("eventsInSymmwtryAlways") && (!DeployablesEnvironment::SceneIsEditor() || !Get<bool>("eventsInSymmwtryEditor"))) return;
    Part* p = PartPtr();
    if (p == nullptr) throw NullReferenceException(std::string("Object reference not set to an instance of an object (") + __func__ + ")");
    int count = static_cast<int>(p->symmetryCounterparts.size());
    const int index = p->Modules().IndexOf(this);
    while (count-- > 0) {
        Part* sym = p->symmetryCounterparts[static_cast<size_t>(count)];
        if (sym == nullptr || index < 0 || static_cast<size_t>(index) >= sym->Modules().Count()) continue;
        if (auto* m = dynamic_cast<ModuleDeployablePart*>(sym->Modules()[static_cast<size_t>(index)])) m->DoRetract();
    }
}

// ModuleDeployablePart.cs:389
void ModuleDeployablePart::DoRetract() {
    if ((Get<bool>("retractable") || DeployablesEnvironment::SceneIsEditor()) && deployState() == DeployState::EXTENDED) {
        if (useAnimation()) {
            AnimationState& s = AnimState();
            s.speed = DeployablesEnvironment::SceneIsEditor() ? (0.0f - Get<float>("editorAnimationSpeedMult")) : (-1.0f);
            s.set_normalizedTime(1.0f);
            s.enabled = true;
            anim->Play(Str("animationName"));
        }
        set_deployState(DeployState::RETRACTING);
        Events()["Retract"]->active = false;
        onMove->Fire(1.0f, 0.0f);
    }
}

// ModuleDeployablePart.cs:406 (KerbalExperience / FlightGlobals.ActiveVessel / the EVA inventory are the host's: a successful repair kit
// removal is assumed, the DoRepair state change is ported)
void ModuleDeployablePart::EventRepairExternal() {
    if (deployState() == DeployState::BROKEN) DoRepair();
}

// ModuleDeployablePart.cs:445
void ModuleDeployablePart::CheatRepair() { DoRepair(); }

// ModuleDeployablePart.cs:450 (partInfo.partPrefab Instantiate + FindModelTransform re-parenting is `repairTransforms`; unset: succeeds)
bool ModuleDeployablePart::DoRepair() {
    bool flag = false;
    Part* p = PartPtr();
    if (p != nullptr && p->partInfo != nullptr && p->partInfo->partPrefab != nullptr) flag = repairTransforms ? repairTransforms(*this) : true;
    if (flag) {
        // breakObjects = new List<GameObject>() (the engine's); the pivot re-read:
        hasPanelRotationTransform = DeployablesEnvironment::hasModelTransform && DeployablesEnvironment::hasModelTransform(*this, Str("pivotName"));
        hasPivot = hasPanelRotationTransform;
        FindAnimations();
        if (!Str("secondaryTransformName").empty()) {
            hasSecondaryTransform = DeployablesEnvironment::hasModelTransform && DeployablesEnvironment::hasModelTransform(*this, Str("secondaryTransformName"));
            if (!hasSecondaryTransform) log::Error("Couldn't access secondaryTransform");
        }
        set_deployState(DeployState::RETRACTED);
        if (eventRepairExternal != nullptr) eventRepairExternal->active = false;
        GE::onPartRepaired.Fire(p);
        startFSM();
    }
    return flag;
}

// ModuleDeployablePart.cs:518
void ModuleDeployablePart::onVesselFocusChange(Vessel* v) {
    const bool isEVA = Part::vesselHooks.isEVA ? Part::vesselHooks.isEVA(*v) : false;
    if (isEVA) {
        if (eventRepairExternal != nullptr)
            eventRepairExternal->SetGuiName(Localizer::Format("#autoLOC_6005092", std::vector<std::string>{std::to_string(repairKitsNecessary)}));
        if (!Get<bool>("retractable")) Events()["Retract"]->guiActive = false;
    } else {
        if (eventRepairExternal != nullptr) eventRepairExternal->SetGuiName(cache("#autoLOC_6005093"));
    }
}

void ModuleDeployablePart::BindHandlers() {
    Events().Bind("Extend", BaseEvent::Handler([this] { Extend(); }));
    Events().Bind("Retract", BaseEvent::Handler([this] { Retract(); }));
    Events().Bind("EventRepairExternal", BaseEvent::Handler([this] { EventRepairExternal(); }));
    Actions().Bind("ExtendPanelsAction", [this](KSPActionParam& p) { ExtendPanelsAction(p); });
    Actions().Bind("ExtendAction", [this](KSPActionParam& p) { ExtendAction(p); });
    Actions().Bind("RetractAction", [this](KSPActionParam& p) { RetractAction(p); });
}

// ModuleDeployablePart.cs:542
void ModuleDeployablePart::OnAwake() {
    PartModule::OnAwake();
    BindHandlers();
    std::string prefix = "?";
    const Part* p = PartPtr();
    if (part != nullptr) {
        const auto& pn = part->fields.partName;
        prefix = (pn ? *pn : std::string()) + "_" + std::to_string(part->fields.flightID) + "_" + std::to_string(p ? p->Modules().IndexOf(this) : -1);
    }
    onMove = std::make_unique<events::EventData<float, float>>(prefix + "_onMove");
    onStop = std::make_unique<events::EventData<float>>(prefix + "_onStop");
    hasPanelRotationTransform = DeployablesEnvironment::hasModelTransform && DeployablesEnvironment::hasModelTransform(*this, Str("pivotName"));
    destroyOnBreakObjects.clear();  // breakObjects = new List<GameObject>()
    if (hasPanelRotationTransform) {
        // originalRotation = currentRotation = panelRotationTransform.localRotation (the model's transform: excluded from the part
        // database gate, WP-06; the host reads it through panelLocalRotation)
        if (DeployablesEnvironment::panelLocalRotation) {
            if (auto q = DeployablesEnvironment::panelLocalRotation(*this)) {
                Set<Quaternionf>("originalRotation", *q);
                Set<Quaternionf>("currentRotation", *q);
            }
        }
        hasPivot = true;
    } else {
        hasPivot = false;
    }
}

// ModuleDeployablePart.cs:681
void ModuleDeployablePart::FindAnimations() {
    std::vector<std::shared_ptr<Animation>> comps;
    if (DeployablesEnvironment::modelAnimations) comps = DeployablesEnvironment::modelAnimations(*this);
    anim.reset();
    const std::string name = Str("animationName");
    for (const auto& c : comps) {
        if (c->GetClip(name)) {
            anim = c;
            break;
        }
    }
    if (!comps.empty() && !anim) anim = comps[0];
    if (anim && useAnimation()) Set<bool>("useAnimation", true);
    else Set<bool>("useAnimation", false);
    if (useAnimation() && !(*anim)[name]) Set<bool>("useAnimation", false);
}

// ModuleDeployablePart.cs:566
void ModuleDeployablePart::OnStart(StartState state) {
    FindAnimations();
    if (DeployablesEnvironment::SceneIsMissionBuilder()) {
        AnimationState& s = AnimState();
        s.set_normalizedTime(0.0f);
        s.enabled = true;
        s.weight = 1.0f;
        anim->Stop(Str("animationName"));
        return;
    }
    const std::string partType = Get<std::string>("partType");
    {
        BaseEvent* e = Events()["Extend"];
        const std::string text = Localizer::Format(Get<std::string>("extendActionName"), std::vector<std::string>{partType});
        Actions()["ExtendAction"]->SetGuiName(text);
        e->SetGuiName(text);
    }
    {
        BaseEvent* e = Events()["Retract"];
        const std::string text = Localizer::Format(Get<std::string>("retractActionName"), std::vector<std::string>{partType});
        Actions()["RetractAction"]->SetGuiName(text);
        e->SetGuiName(text);
    }
    Events()["Retract"]->externalToEVAOnly = Get<bool>("retractable");
    Actions()["ExtendPanelsAction"]->SetGuiName(Localizer::Format(Get<std::string>("extendpanelsActionName"), std::vector<std::string>{partType}));
    {
        const float mass = part != nullptr ? part->fields.mass : 0.0f;
        const int kits = static_cast<int>(fx(mass) / fx(DeployablesEnvironment::partRepairMassPerKit));
        repairKitsNecessary = std::min(std::max(kits, 1), DeployablesEnvironment::partRepairMaxKitAmount);
    }
    eventRepairExternal = Events()["EventRepairExternal"];
    eventRepairExternal->active = deployState() == DeployState::BROKEN;
    if (DeployablesEnvironment::SceneIsFlight()) {
        // FlightGlobals.ActiveVessel.isEVA: the active vessel record (isActiveVessel) is the host's through VesselRecord
        const ProcessingVessel* v = VesselRecord();
        const bool activeIsEVA = v != nullptr && v->isActiveVessel && Part::vesselHooks.isEVA && PartPtr()->vessel && Part::vesselHooks.isEVA(*PartPtr()->vessel);
        if (activeIsEVA) {
            eventRepairExternal->SetGuiName(Localizer::Format("#autoLOC_6005092", std::vector<std::string>{std::to_string(repairKitsNecessary)}));
            if (!Get<bool>("retractable")) Events()["Retract"]->guiActive = false;
        } else {
            eventRepairExternal->SetGuiName(cache("#autoLOC_6005093"));
        }
        Set<std::string>("brokenStatusWarning", cache("#autoLOC_6005093"));
        GE::onVesselChange.Add(this, &ModuleDeployablePart::onVesselFocusChange);
    }
    if (!Get<bool>("retractable")) {
        Events()["Retract"]->guiActive = false;
        Actions()["RetractAction"]->active = false;
        Actions()["ExtendPanelsAction"]->active = false;
    }
    if (state == StartState::None) return;
    // Planetarium.fetch: trackingBody per mode
    const int mode = static_cast<int>(trackingMode());
    {
        std::optional<ProcessingBody> body;
        bool have = true;
        if (DeployablesEnvironment::planetariumBody) {
            body = DeployablesEnvironment::planetariumBody(mode);
            have = body.has_value();
            if (!have && mode != static_cast<int>(TrackingMode::VESSEL) && mode != static_cast<int>(TrackingMode::NONE)) have = false;
        } else {
            switch (trackingMode()) {
                case TrackingMode::HOME: { ProcessingBody b; b.flightGlobalsIndex = 1; b.bodyName = b.displayName = "Kerbin"; body = b; break; }
                case TrackingMode::CURRENT: {
                    if (const ProcessingVessel* v = VesselRecord()) body = v->mainBody;
                    break;
                }
                case TrackingMode::VESSEL:
                case TrackingMode::NONE: break;
                default: { ProcessingBody b; b.flightGlobalsIndex = 0; b.bodyName = b.displayName = "Sun"; body = b; break; }
            }
        }
        if (have) {
            if (trackingMode() == TrackingMode::VESSEL) { trackingVesselSet = !vesselID.empty(); }
            else if (trackingMode() == TrackingMode::NONE) { trackingBody.reset(); trackingVesselSet = false; }
            else trackingBody = body;
        }
    }
    GetTrackingBodyTransforms();
    if (!Str("secondaryTransformName").empty()) {
        hasSecondaryTransform = DeployablesEnvironment::hasModelTransform && DeployablesEnvironment::hasModelTransform(*this, Str("secondaryTransformName"));
        if (!hasSecondaryTransform) log::Error("Couldn't access secondaryTransform");
    }
    hasPanelRotationTransform = DeployablesEnvironment::hasModelTransform && DeployablesEnvironment::hasModelTransform(*this, Str("pivotName"));
    if (Str("breakName").empty()) Set<std::string>("breakName", Str("pivotName"));
    hasPanelBreakTransform = DeployablesEnvironment::hasModelTransform && DeployablesEnvironment::hasModelTransform(*this, Str("breakName"));
    if (!hasPanelRotationTransform) {
        if (alignType() == PanelAlignType::PIVOT) log::Error("Could not find the pivotName, " + Str("pivotName") + ", in heirarchy");
        hasPivot = false;
    } else {
        hasPivot = true;
    }
    {
        BaseField* f = Fields()["status"];
        const bool showStatus = Get<bool>("showStatus");
        f->guiActiveEditor = showStatus;
        f->guiActive = showStatus;
    }
    startFSM();
    DeployablesEnvironment::scheduleSetCollisionIgnoresCalls++;  // base.part.ScheduleSetCollisionIgnores()
}

// ModuleDeployablePart.cs:710 (GetInfo: UI text, deferred)
std::string ModuleDeployablePart::GetInfo() { return PartModule::GetInfo(); }

// ModuleDeployablePart.cs:732
void ModuleDeployablePart::OnLoad(const ConfigNode& node) {
    Set<std::string>("subPartName", Localizer::Format(Get<std::string>("subPartName")));
    Set<std::string>("partType", Localizer::Format(Get<std::string>("partType")));
    if (node.HasValue("stateString")) SetEnumByName("deployState", *node.GetValue("stateString"));
    if (node.HasValue("sunTracking")) {
        if (cs::BoolParse(*node.GetValue("sunTracking"))) {
            Set<bool>("isTracking", true);
            SetEnumByName("trackingMode", "SUN");
        } else {
            Set<bool>("isTracking", false);
        }
    }
    if (node.HasValue("vesselID")) vesselID = *node.GetValue("vesselID");
    destroyOnBreakObjects = node.GetValues("destroyOnBreakObject");
}

// ModuleDeployablePart.cs:759 (`trackingVessel.id.ToString("N")` is the host's vessel id: vesselID text is written when a vessel is tracked)
void ModuleDeployablePart::OnSave(ConfigNode& node) {
    if (trackingVesselSet) node.AddValue("vesselID", vesselID);
    for (const std::string& s : destroyOnBreakObjects) node.AddValue("destroyOnBreakObject", s);
}

// ModuleDeployablePart.cs:771 (the ProtoPartModuleSnapshot.moduleValues write of deployState is the host's)
void ModuleDeployablePart::OnInventoryModeDisable() {
    bypassSetupAnimation = true;
    FindAnimations();
    startFSM();
    if (anim && (*anim)[Str("animationName")] != nullptr) {
        float normalizedTime = 0.0f;
        if (deployState() == DeployState::EXTENDED) normalizedTime = 1.0f;
        AnimationState& s = AnimState();
        s.enabled = true;
        s.weight = 1.0f;
        s.speed = 0.0f;
        if (DeployablesEnvironment::SceneIsFlight()) s.set_normalizedTime(normalizedTime);
    }
}

// ModuleDeployablePart.cs:801
void ModuleDeployablePart::OnStoredInInventory() {
    if (DeployablesEnvironment::SceneIsFlight()) set_deployState(DeployState::RETRACTED);
}

// ModuleDeployablePart.cs:810
void ModuleDeployablePart::FixedUpdate() {
    const ProcessingVessel* v = VesselRecord();
    if (DeployablesEnvironment::SceneIsFlight() && v != nullptr) {
        if (trackingMode() == TrackingMode::CURRENT) {
            if (!trackingBody || trackingBody->flightGlobalsIndex != v->mainBody.flightGlobalsIndex) {
                trackingBody = v->mainBody;
                GetTrackingBodyTransforms();
            }
        } else if (trackingMode() == TrackingMode::VESSEL && !hasTrackingTransformLocal) {
            if (!trackingVesselSet) trackingVesselSet = true;
            hasTrackingTransformLocal = true;  // trackingTransformLocal = trackingVessel.vesselTransform
        }
        updateFSM();
    } else if (DeployablesEnvironment::SceneIsEditor()) {
        updateFSM();
    }
}

// ModuleDeployablePart.cs:838
void ModuleDeployablePart::LateUpdate() {
    if (!DeployablesEnvironment::SceneIsMissionBuilder()) {
        if (deployState() == DeployState::BROKEN) {
            if (eventRepairExternal != nullptr) eventRepairExternal->active = true;
        } else if (useAnimation() && stopAnimation) {
            anim->Stop(Str("animationName"));
            stopAnimation = false;
        }
    }
}

// ModuleDeployablePart.cs:854
void ModuleDeployablePart::OnCollisionEnter(float relativeVelocityMagnitude) {
    float num = Get<float>("impactResistance");
    if (Get<bool>("isTracking") && deployState() == DeployState::RETRACTED) num = Get<float>("impactResistanceRetracted");
    if (relativeVelocityMagnitude > num) breakPanels();
}

// ModuleDeployablePart.cs:867
void ModuleDeployablePart::startFSM() {
    if (useAnimation()) {
        AnimationState& s = AnimState();
        s.wrapMode = WrapMode::ClampForever;
        switch (deployState()) {
            case DeployState::RETRACTED:
                s.set_normalizedTime(0.0f);
                s.enabled = true;
                s.weight = 1.0f;
                anim->Stop(Str("animationName"));
                bypassSetupAnimation = true;
                Events()["Retract"]->active = false;
                Events()["Extend"]->active = true;
                break;
            case DeployState::EXTENDED:
                s.set_normalizedTime(1.0f);
                s.enabled = true;
                s.speed = 0.0f;
                s.weight = 1.0f;
                Events()["Extend"]->active = false;
                Events()["Retract"]->active = Get<bool>("retractable") || DeployablesEnvironment::SceneIsEditor();
                if (hasPivot && DeployablesEnvironment::panelRotation) DeployablesEnvironment::panelRotation(*this, DeployablesEnvironment::PanelRotationOp::SetCurrent, 0.0f);
                break;
            case DeployState::RETRACTING:
            case DeployState::EXTENDING:
                Events()["Retract"]->active = false;
                Events()["Extend"]->active = false;
                break;
            default: break;
        }
        if (deployState() == DeployState::RETRACTING || deployState() == DeployState::EXTENDING || deployState() == DeployState::BROKEN) {
            s.set_normalizedTime(Get<float>("storedAnimationTime"));
            s.speed = Get<float>("storedAnimationSpeed");
        }
        if (!bypassSetupAnimation) anim->Play(Str("animationName"));
        if (!playAnimationOnStart && deployState() != DeployState::EXTENDING && deployState() != DeployState::RETRACTING) stopAnimation = true;
    } else {
        if (hasPivot && DeployablesEnvironment::panelRotation) DeployablesEnvironment::panelRotation(*this, DeployablesEnvironment::PanelRotationOp::SetOriginal, 0.0f);
        if (deployState() != DeployState::BROKEN) set_deployState(DeployState::EXTENDED);
        Events()["Retract"]->active = false;
        Events()["Extend"]->active = false;
        Actions()["ExtendPanelsAction"]->active = false;
        Actions()["ExtendAction"]->active = false;
        Actions()["RetractAction"]->active = false;
        Fields()["status"]->guiActiveEditor = false;
    }
    if (deployState() == DeployState::BROKEN) {
        Events()["Retract"]->active = false;
        Events()["Extend"]->active = false;
        // panelBreakTransform.gameObject.SetActive(false): the engine's
    }
}

// ModuleDeployablePart.cs:963 (the Random-driven physicalObject conversion is `convertBreakObjects`)
void ModuleDeployablePart::breakPanels() {
    Part* p = PartPtr();
    if ((!hasPivot && !hasPanelBreakTransform) || (p != nullptr && p->fields.packed) || deployState() == DeployState::BROKEN || DeployablesEnvironment::noCrashDamage) return;
    if (useAnimation() && anim->IsPlaying(Str("animationName"))) anim->Stop(Str("animationName"));
    if (DeployablesEnvironment::convertBreakObjects) DeployablesEnvironment::convertBreakObjects(*this);
    Events()["Retract"]->active = false;
    Events()["Extend"]->active = false;
    set_deployState(DeployState::BROKEN);
    if (p != nullptr) {
        // part.RefreshHighlighter / ResetCollisions: the engine's
        GE::onVesselWasModified.Fire(p->vessel);
    }
}

// ModuleDeployablePart.cs:1007
void ModuleDeployablePart::breakNotifications(const std::string& partTitle, const std::string& breakMessage) {
    const ProcessingVessel* v = VesselRecord();
    if (v != nullptr && v->isActiveVessel) DeployablesEnvironment::screenMessages.push_back("<color=orange>[" + partTitle + "]: " + breakMessage + "</color>");
    DeployablesEnvironment::flightLog.push_back(partTitle + ": " + breakMessage);
    DeployablesEnvironment::breakNotificationCalls++;
    log::Info(partTitle + ": " + breakMessage);
}

// ModuleDeployablePart.cs:1021
void ModuleDeployablePart::updateFSM() {
    const bool flag = (hasTrackingTransformLocal && (hasPivot || alignType() != PanelAlignType::PIVOT)) || (hasTrackingTransformLocal && DeployablesEnvironment::SceneIsEditor());
    const float dt = TimeWarp::deltaTime();
    switch (deployState()) {
        case DeployState::RETRACTED:
            Set<std::string>("status", cache("#autoLOC_234861"));
            SetDragCubes(1.0f, 0.0f);
            break;
        case DeployState::EXTENDED:
            Set<std::string>("status", cache("#autoLOC_234828"));
            if (flag) CalculateTracking();
            else SetDragCubes(0.0f, 0.0f);
            break;
        case DeployState::RETRACTING: {
            if (flag && DeployablesEnvironment::panelRotation) {
                const float t = f32store((1.0 - fx(AnimState().normalizedTime())) * fx(dt));
                DeployablesEnvironment::panelRotation(*this, DeployablesEnvironment::PanelRotationOp::LerpToOriginal, t);
            }
            if (AnimState().normalizedTime() <= 0.0f) {
                anim->Stop(Str("animationName"));
                if (flag && DeployablesEnvironment::panelRotation) DeployablesEnvironment::panelRotation(*this, DeployablesEnvironment::PanelRotationOp::SetOriginal, 0.0f);
                set_deployState(DeployState::RETRACTED);
                DeployablesEnvironment::scheduleSetCollisionIgnoresCalls++;
                Events()["Extend"]->active = true;
                onStop->Fire(0.0f);
            }
            Set<std::string>("status", cache("#autoLOC_234856"));
            SetDragCubes(f32store(1.0 - fx(AnimState().normalizedTime())), 0.0f);
            break;
        }
        case DeployState::EXTENDING:
            if (AnimState().normalizedTime() >= 1.0f) {
                anim->Stop(Str("animationName"));
                set_deployState(DeployState::EXTENDED);
                DeployablesEnvironment::scheduleSetCollisionIgnoresCalls++;
                Events()["Retract"]->active = Get<bool>("retractable") || DeployablesEnvironment::SceneIsEditor();
                onStop->Fire(1.0f);
            }
            Set<std::string>("status", cache("#autoLOC_234841"));
            SetDragCubes(f32store(1.0 - fx(AnimState().normalizedTime())), 0.0f);
            break;
        case DeployState::BROKEN:
            // panelRotationTransform.gameObject.SetActive(false): the engine's
            Set<std::string>("status", cache("#autoLOC_234868"));
            SetDragCubes(1.0f, 0.0f);
            break;
    }
    const ProcessingVessel* v = VesselRecord();
    if (DeployablesEnvironment::SceneIsFlight() && deployState() != DeployState::RETRACTED && deployState() != DeployState::BROKEN && Get<bool>("isBreakable") &&
        !(v != nullptr && v->holdPhysics)) {
        std::string title = cache("#autoLOC_6001017");
        Part* p = PartPtr();
        if (p != nullptr && p->partInfo != nullptr) title = p->partInfo->title;
        if (ShouldBreakFromPressure()) {
            breakNotifications(title, Localizer::Format("#autoLOC_6001018", std::vector<std::string>{Get<std::string>("partType")}));
            breakPanels();
        } else if (ShouldBreakFromG()) {
            breakNotifications(title, Localizer::Format("#autoLOC_6001019", std::vector<std::string>{Get<std::string>("partType")}));
            breakPanels();
        }
    }
    if (hasPivot && hasPanelRotationTransform && DeployablesEnvironment::panelLocalRotation) {
        if (auto q = DeployablesEnvironment::panelLocalRotation(*this)) Set<Quaternionf>("currentRotation", *q);
    }
    if (useAnimation() && deployState() != DeployState::BROKEN) {
        Set<float>("storedAnimationTime", AnimState().normalizedTime());
        Set<float>("storedAnimationSpeed", AnimState().speed);
    }
    PostFSMUpdate();
}

// ModuleDeployablePart.cs:1112 (`vessel.velocityD.normalized` dot the pivot's forward: the frame supplies the forward)
bool ModuleDeployablePart::ShouldBreakFromPressure() {
    const Part* p = PartPtr();
    const ProcessingVessel* v = VesselRecord();
    if (!ShieldedFromAirstream() && v != nullptr && p != nullptr) {
        const float num = static_cast<float>(p->fields.dynamicPressurekPa + p->fields.submergedDynamicPressurekPa);
        if (num < Get<float>("windResistance")) return false;
        if ((!hasPivot || !hasPanelRotationTransform) && !hasPanelBreakTransform) return false;
        if (!DeployablesEnvironment::trackingFrame) return false;
        auto frame = DeployablesEnvironment::trackingFrame(*this);
        if (!frame) return false;
        const Vector3d vel = v->velocityD.normalized();
        const Vector3f fwd = frame->pivotForward.normalized();
        // Vector3.Dot(Vector3d -> Vector3 implicit float cast, Vector3): float product
        const Vector3f velF{static_cast<float>(vel.x), static_cast<float>(vel.y), static_cast<float>(vel.z)};
        float num2 = Mathf::Abs(Vector3f::Dot(velF, fwd));
        const float minAoAForQCheck = MinAoAForQCheck();
        if (num2 < minAoAForQCheck) num2 = minAoAForQCheck;
        return fx(num2) * fx(num) > fx(Get<float>("windResistance"));
    }
    return false;
}

// ModuleDeployablePart.cs:1137
bool ModuleDeployablePart::ShouldBreakFromG() {
    const ProcessingVessel* v = VesselRecord();
    if (v == nullptr) throw NullReferenceException(std::string("Object reference not set to an instance of an object (") + __func__ + ")");
    return v->geeForce > Get<double>("gResistance");
}

// ModuleDeployablePart.cs:1146
bool ModuleDeployablePart::CalculateTrackingLOS(const Vector3f&, std::string& blocker) {
    if (ShieldedFromAirstream() && Get<bool>("applyShielding")) {
        blocker = "aero shielding";
        return false;
    }
    return true;
}

// ModuleDeployablePart.cs:1157
void ModuleDeployablePart::CalculateTracking() {
    if (!DeployablesEnvironment::SceneIsFlight()) return;
    auto frame = DeployablesEnvironment::trackingFrame ? DeployablesEnvironment::trackingFrame(*this) : std::nullopt;
    if (!frame) throw NullReferenceException(std::string("Object reference not set to an instance of an object (") + __func__ + ")");  // trackingTransformLocal / panelRotationTransform are required here
    const Vector3f normalized = frame->trackingDirection;
    trackingLOS = CalculateTrackingLOS(normalized, blockingObject);
    if (Get<bool>("isTracking") && DeployablesEnvironment::alignPanel) {
        DeployablesEnvironment::alignPanel(*this, Get<float>("TrackingAlignmentOffset"), f32store(fx(TimeWarp::deltaTime()) * fx(Get<float>("trackingSpeed"))));
        frame = DeployablesEnvironment::trackingFrame(*this);  // panelEulerY after the alignment
    }
    lastFrame = frame;
    const bool pawOpen = DeployablesEnvironment::partActionUIContains && DeployablesEnvironment::partActionUIContains(PartPtr());
    if (pawOpen) {
        if (trackingLOS) {
            if (Get<bool>("isTracking"))
                Set<std::string>("status", Localizer::Format("#autoLOC_234989", std::vector<std::string>{trackingBody ? trackingBody->displayName : std::string()}));
        } else {
            Set<std::string>("status", Localizer::Format("#autoLOC_234994", std::vector<std::string>{blockingObject}));
        }
    }
    if (Get<bool>("isTracking") || useAnimation()) {
        SetDragCubes(0.0f, hasPanelRotationTransform ? Mathf::Abs(Mathf::Sin(f32store(fx(kDeg2Rad) * fx(frame->panelEulerY)))) : 0.0f);
    }
    PostCalculateTracking(trackingLOS, normalized);
}

// ModuleDeployablePart.cs:1200
void ModuleDeployablePart::GetTrackingBodyTransforms() {
    if (trackingBody.has_value()) hasTrackingTransformLocal = true;
    else if (trackingVesselSet) hasTrackingTransformLocal = true;
}

// ModuleDeployablePart.cs:1216
void ModuleDeployablePart::SetScalar(float t) {
    if (useAnimation() && deployState() != DeployState::BROKEN &&
        (DeployablesEnvironment::SceneIsEditor() || !ShieldedFromAirstream() || (!Get<bool>("applyShielding") && !Get<bool>("applyShieldingExtend")))) {
        if (t > 0.0f) Extend();
        else Retract();
    }
}

// ModuleDeployablePart.cs:1231
void ModuleDeployablePart::SetUIRead(bool state) {
    if (!Get<bool>("showStatus")) Fields()["status"]->guiActive = state;
}

// ModuleDeployablePart.cs:1239
void ModuleDeployablePart::SetUIWrite(bool state) {
    overrideUIWriteState = state;
    Events()["Extend"]->active = useAnimation() && state && deployState() == DeployState::RETRACTED;
    Events()["Retract"]->active = useAnimation() && state && deployState() == DeployState::EXTENDED && Get<bool>("retractable");
    Actions()["ExtendPanelsAction"]->active = useAnimation() && state;
    Actions()["ExtendAction"]->active = useAnimation() && state;
    Actions()["RetractAction"]->active = useAnimation() && state;
}

// ModuleDeployablePart.cs:1249
bool ModuleDeployablePart::IsMoving() {
    if (useAnimation()) {
        if (deployState() != DeployState::EXTENDING) return deployState() == DeployState::RETRACTING;
        return true;
    }
    return false;
}

// ModuleDeployablePart.cs:1262 (anim = GetComponentInChildren<Animation>(): the first animation component of the model)
std::vector<std::string> ModuleDeployablePart::GetDragCubeNames() const {
    std::shared_ptr<Animation> first;
    if (DeployablesEnvironment::modelAnimations) {
        auto comps = DeployablesEnvironment::modelAnimations(const_cast<ModuleDeployablePart&>(*this));
        if (!comps.empty()) first = comps[0];
    }
    const_cast<ModuleDeployablePart*>(this)->anim = first;  // `anim = GetComponentInChildren<Animation>()` (sic: the getter assigns)
    if (first && useAnimation()) {
        if (!Get<bool>("isTracking")) return {"RETRACTED", "EXTENDED_A"};
        return {"RETRACTED", "EXTENDED_A", "EXTENDED_B"};
    }
    if (Get<bool>("isTracking")) return {"EXTENDED_A", "EXTENDED_B"};
    return {};  // C#: null
}

// ModuleDeployablePart.cs:1280 (the pivot `rotation *= Quaternion.Euler(...)` is the engine's)
void ModuleDeployablePart::AssumeDragCubePosition(const std::string& name) {
    if (name == "EXTENDED_B" || name == "EXTENDED_A") {
        set_deployState(DeployState::EXTENDED);
        if (useAnimation()) {
            AnimationState& s = AnimState();
            s.set_normalizedTime(1.0f);
            s.set_normalizedSpeed(0.0f);
            s.enabled = true;
            s.speed = 0.0f;
            s.weight = 1.0f;
            anim->Play(Str("animationName"));
        }
    } else if (name == "RETRACTED") {
        set_deployState(DeployState::RETRACTED);
        if (useAnimation()) {
            AnimationState& s = AnimState();
            s.set_normalizedTime(0.0f);
            s.set_normalizedSpeed(0.0f);
            s.enabled = true;
            s.weight = 1.0f;
            anim->Play(Str("animationName"));
        }
    }
}

// ModuleDeployablePart.cs:1335
void ModuleDeployablePart::SetDragCubes(float retracted, float angle) {
    if (part == nullptr) return;
    DragCubeList& cubes = part->DragCubes;
    cubes.SetCubeWeight("RETRACTED", retracted);
    if (Get<bool>("isTracking")) {
        cubes.SetCubeWeight("EXTENDED_A", f32store((1.0 - fx(retracted)) * fx(angle)));
        cubes.SetCubeWeight("EXTENDED_B", f32store((1.0 - fx(retracted)) * (1.0 - fx(angle))));
    } else {
        cubes.SetCubeWeight("EXTENDED_A", f32store(1.0 - fx(retracted)));
    }
}

// ModuleDeployablePart.cs:1372
bool ModuleDeployablePart::IsDeployablePartStuck() const {
    for (const auto& a : stuckAdjusters)
        if (a()) return true;
    return false;
}

// ModuleDeployablePart.cs:1384
std::string ModuleDeployablePart::GetModuleDisplayName() { return Localizer::Format("#autoLoc_6003036"); }

// ModuleDeployablePart.cs:1404
bool ModuleDeployablePart::AllowConstructionDeployState() const {
    return deployState() != DeployState::BROKEN && deployState() != DeployState::EXTENDING && deployState() != DeployState::RETRACTING;
}

// ================================================ ModuleDeployableAntenna ==========================================================

// ModuleDeployableAntenna.cs:11
bool ModuleDeployableAntenna::CanMove() const {
    if (!ModuleDeployablePart::CanMove()) return false;
    if (DeployablesEnvironment::SceneIsFlight()) return !const_cast<ModuleDeployableAntenna*>(this)->ShouldBreakFromPressure();
    return true;
}

// ModuleDeployableAntenna.cs:23
void ModuleDeployableAntenna::OnLoad(const ConfigNode& node) {
    ModuleDeployablePart::OnLoad(node);
    Set<std::string>("subPartName", Localizer::Format("#autoLOC_234195"));
    Set<std::string>("partType", Localizer::Format("#autoLOC_234196"));
}

// ModuleDeployableAntenna.cs:30
void ModuleDeployableAntenna::Extend() {
    if (DeployablesEnvironment::SceneIsFlight() && ShouldBreakFromPressure()) {
        DeployablesEnvironment::screenMessages.push_back(Localizer::Format("#autoLOC_234215", std::vector<std::string>{Get<std::string>("partType")}));
    } else {
        ModuleDeployablePart::Extend();
    }
}

// ModuleDeployableAntenna.cs:56
std::string ModuleDeployableAntenna::GetModuleDisplayName() { return Localizer::Format("#autoLoc_6003035"); }

// ================================================ ModuleDeployableRadiator =========================================================

// ModuleDeployableRadiator.cs:9
void ModuleDeployableRadiator::OnLoad(const ConfigNode& node) {
    ModuleDeployablePart::OnLoad(node);
    Set<std::string>("subPartName", Localizer::Format("#autoLOC_234369"));
    Set<std::string>("partType", Localizer::Format("#autoLOC_232117"));
}

// ModuleDeployableRadiator.cs:27 (cacheAutoLOC_7000030)
std::string ModuleDeployableRadiator::GetModuleDisplayName() { return Localizer::Format("#autoLOC_7000030"); }

// ============================================== ModuleDeployableSolarPanel =========================================================

ModuleDeployableSolarPanel::PanelType ModuleDeployableSolarPanel::panelType() const { return static_cast<PanelType>(Get<EnumValue>("panelType").value); }

// ModuleDeployableSolarPanel.cs:97 (the layer masks are not KSPFields)
void ModuleDeployableSolarPanel::OnAwake() {
    ModuleDeployablePart::OnAwake();
    resHandler.moduleResourceBasedPrimaryIsInput = false;
    if (IsNull("powerCurve")) Set<FloatCurve>("powerCurve", FloatCurve{});
    if (IsNull("temperatureEfficCurve")) {
        FloatCurve c;
        c.Add(4.0f, 1.2f, 0.0f, -0.0006f);
        c.Add(300.0f, 1.0f, -0.0008f, -0.0008f);
        c.Add(1200.0f, 0.134f, -0.00035f, -0.00035f);
        c.Add(1900.0f, 0.02f, -3.72E-05f, -3.72E-05f);
        c.Add(2500.0f, 0.01f, 0.0f, 0.0f);
        Set<FloatCurve>("temperatureEfficCurve", std::move(c));
    }
    if (IsNull("timeEfficCurve")) {
        FloatCurve c;
        c.Add(0.0f, 1.0f);
        Set<FloatCurve>("timeEfficCurve", std::move(c));
    }
    originalEfficiencyMultiplier = Get<float>("efficiencyMult");
}

// ModuleDeployableSolarPanel.cs:129
void ModuleDeployableSolarPanel::OnStart(StartState state) {
    if (!DeployablesEnvironment::SceneIsEditor() && !DeployablesEnvironment::SceneIsFlight()) return;
    if (!repairedSubscribed_) {
        GE::onPartRepaired.Add(this, &ModuleDeployableSolarPanel::OnPartRepaired);
        repairedSubscribed_ = true;
    }
    Fields()["flowRate"]->guiFormat = Get<std::string>("flowFormat");
    Fields()["flowRate"]->guiUnits = (Get<bool>("flowUnitsUseSpace") ? " " : "") + Get<std::string>("flowUnits");
    ModuleDeployablePart::OnStart(state);
    if (!hasSecondaryTransform) log::Error("Couldn't access secondaryTransform for raycasts");
    hasTrackingDotTransform = Get<bool>("useRaycastForTrackingDot") ? hasSecondaryTransform : hasPanelRotationTransform;
    if (DeployablesEnvironment::SceneIsFlight() && anim) {
        const float normalizedTime = deployState() == DeployState::EXTENDED ? 1.0f : 0.0f;
        AnimationState& s = AnimState();
        s.set_normalizedTime(normalizedTime);
        s.enabled = true;
        s.weight = 1.0f;
        if (deployState() == DeployState::RETRACTED) anim->Stop(Str("animationName"));
    }
}

ModuleDeployableSolarPanel::~ModuleDeployableSolarPanel() {
    // ModuleDeployableSolarPanel.cs:123 OnDestroy
    GE::onPartRepaired.Remove(this, &ModuleDeployableSolarPanel::OnPartRepaired);
}

// ModuleDeployableSolarPanel.cs:164 (GetInfo: UI text, deferred)
std::string ModuleDeployableSolarPanel::GetInfo() { return Get<bool>("showInfo") ? ModuleDeployablePart::GetInfo() : std::string(); }

// ModuleDeployableSolarPanel.cs:173
void ModuleDeployableSolarPanel::OnLoad(const ConfigNode& node) {
    if (Str("secondaryTransformName").empty()) Set<std::string>("secondaryTransformName", Str("raycastTransformName"));
    ModuleDeployablePart::OnLoad(node);
    Set<std::string>("subPartName", Localizer::Format("#autoLOC_235328"));
    Set<std::string>("partType", Localizer::Format("#autoLOC_235329"));
    if (node.HasValue("type")) SetEnumByName("panelType", *node.GetValue("type"));
    if (resHandler.outputResources.empty() && node.HasValue("chargeRate")) {
        auto moduleResource = std::make_shared<ModuleResource>();
        moduleResource->name = Get<std::string>("resourceName");
        moduleResource->title = KSPUtil::PrintModuleName(moduleResource->name);
        moduleResource->id = MonoStringHash(moduleResource->name);
        moduleResource->rate = static_cast<double>(Get<float>("chargeRate"));
        resHandler.outputResources.push_back(moduleResource);
    }
}

// ModuleDeployableSolarPanel.cs:197
void ModuleDeployableSolarPanel::FixedUpdate() {
    if (!DeployablesEnvironment::SceneIsMissionBuilder()) {
        const ProcessingVessel* v = VesselRecord();
        if ((DeployablesEnvironment::SceneIsEditor() || DeployablesEnvironment::SceneIsFlight()) && v != nullptr &&
            (Get<double>("launchUT") < 0.0 || v->situation == VesselSituations::PRELAUNCH)) {
            Set<double>("launchUT", ResourceProcessingEnvironment::UniversalTime());
        }
        Set<float>("efficiencyMult", ApplyEfficiencyAdjustments(originalEfficiencyMultiplier));
        ModuleDeployablePart::FixedUpdate();
    }
}

// ModuleDeployableSolarPanel.cs:210
void ModuleDeployableSolarPanel::PostFSMUpdate() {
    if (deployState() != DeployState::EXTENDED) {
        Set<float>("sunAOA", 0.0f);
        Set<float>("flowRate", 0.0f);
    }
}

// ModuleDeployableSolarPanel.cs:222 (both Physics.Raycast calls and the blocker naming are `trackingRaycast`)
bool ModuleDeployableSolarPanel::CalculateTrackingLOS(const Vector3f& trackingDirection, std::string& blocker) {
    bool result = ModuleDeployablePart::CalculateTrackingLOS(trackingDirection, blocker);
    if (result) {
        if (DeployablesEnvironment::trackingRaycast) result = DeployablesEnvironment::trackingRaycast(*this, trackingDirection, blocker);
    }
    return result;
}

// ModuleDeployableSolarPanel.cs:267
void ModuleDeployableSolarPanel::PostCalculateTracking(bool trackingLOS, const Vector3f& trackingDirection) {
    const ProcessingVessel* v = VesselRecord();
    Part* p = PartPtr();
    if (v == nullptr || p == nullptr) throw NullReferenceException(std::string("Object reference not set to an instance of an object (") + __func__ + ")");
    if (!lastFrame) throw NullReferenceException(std::string("Object reference not set to an instance of an object (") + __func__ + ")");
    const DeployablesEnvironment::TrackingFrame& frame = *lastFrame;
    float sunAOA;
    if (trackingLOS) {
        Set<std::string>("status", cache("#autoLOC_235418"));
        if (panelType() == PanelType::FLAT && hasTrackingDotTransform) {
            sunAOA = Mathf::Clamp(Vector3f::Dot(frame.trackingDotForward, trackingDirection), 0.0f, 1.0f);
        } else if (panelType() == PanelType::CYLINDRICAL) {
            const Vector3f lhs = alignType() == PanelAlignType::PIVOT ? frame.trackingDotForward
                                 : alignType() == PanelAlignType::X ? frame.partRight
                                 : alignType() != PanelAlignType::Y ? frame.partForward
                                                                    : frame.partUp;
            sunAOA = f32store((1.0 - fx(Mathf::Abs(Vector3f::Dot(lhs, trackingDirection)))) * fx(kInvPi));
        } else {
            sunAOA = 0.25f;
        }
    } else {
        sunAOA = 0.0f;
    }
    Set<float>("sunAOA", sunAOA);
    _distMult = 1.0;
    if (Get<bool>("useCurve")) _distMult = static_cast<double>(Get<FloatCurve>("powerCurve").Evaluate(frame.trackingDistance));
    else _distMult = v->solarFlux / PhysicsGlobals::SolarLuminosityAtHome();
    const float tempEffic = Get<FloatCurve>("temperatureEfficCurve").Evaluate(static_cast<float>(p->fields.skinTemperature));
    const float timeEffic = Get<FloatCurve>("timeEfficCurve").Evaluate(
        static_cast<float>((ResourceProcessingEnvironment::UniversalTime() - Get<double>("launchUT")) * 1.1574074074074073E-05));
    _efficMult = static_cast<double>(tempEffic) * static_cast<double>(timeEffic) * static_cast<double>(Get<float>("efficiencyMult"));
    _flowRate = static_cast<double>(sunAOA) * _efficMult * _distMult;
    if (p->fields.submergedPortion > 0.0) {
        double num = 0.0 - (DeployablesEnvironment::secondaryTransformAltitude ? DeployablesEnvironment::secondaryTransformAltitude(*this) : 0.0);
        num = (num * 3.0 + p->fields.maxDepth) * 0.25;
        if (num < 0.5) num = 0.5;
        const double num2 = 1.0 / (1.0 + num * v->mainBody.oceanDensity);
        if (p->fields.submergedPortion < 1.0) _flowRate *= UtilMath::LerpUnclamped(1.0, num2, p->fields.submergedPortion);
        else _flowRate *= num2;
        Set<std::string>("status", Get<std::string>("status") + cache("#autoLOC_235468"));
    }
    Set<float>("flowRate", f32store(resHandler.UpdateModuleResourceOutputs(_flowRate) * static_cast<double>(Get<float>("flowMult"))));
}

// ModuleDeployableSolarPanel.cs:323 (the snapshot write is the host's)
void ModuleDeployableSolarPanel::OnInventoryModeDisable() { ModuleDeployablePart::OnInventoryModeDisable(); }

// ModuleDeployableSolarPanel.cs:361
std::string ModuleDeployableSolarPanel::GetModuleDisplayName() { return Localizer::Format("#autoLoc_6003038"); }

// ModuleDeployableSolarPanel.cs:346 (OnPartRepaired: persistentId compare, then the trackingDotTransform choice)
void ModuleDeployableSolarPanel::OnPartRepaired(Part* p) {
    if (p != nullptr && part != nullptr && p->fields.persistentId == part->fields.persistentId)
        hasTrackingDotTransform = Get<bool>("useRaycastForTrackingDot") ? hasSecondaryTransform : hasPanelRotationTransform;
}

// ModuleDeployableSolarPanel.cs:382
float ModuleDeployableSolarPanel::ApplyEfficiencyAdjustments(float efficiency) {
    for (const auto& a : efficiencyAdjusters) efficiency = a(efficiency);
    return efficiency;
}

namespace detail {
void RegisterDeployableModules() {
    ModuleRegistrar<ModuleDeployablePart>("ModuleDeployablePart");
    ModuleRegistrar<ModuleDeployableAntenna>("ModuleDeployableAntenna");
    ModuleRegistrar<ModuleDeployableRadiator>("ModuleDeployableRadiator");
    ModuleRegistrar<ModuleDeployableSolarPanel>("ModuleDeployableSolarPanel");
}
}  // namespace detail

}  // namespace ksp
