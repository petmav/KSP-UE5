// WP-32: KerbalEVA.cs, part 1 (properties, lifecycle, persistence, EVA propellant, pack models, animation manager, boarding, flag, seat, grapple,
// helmet / visor, parachute states, science props). Every function cites KerbalEVA.cs:<line> of decompiled/KSP-Decompiled/Assembly-CSharp/.
// The state machine is in kerbal_eva_fsm.cpp, state callbacks in kerbal_eva_states.cpp, movement / jetpack / ragdoll / ladders in kerbal_eva_motion.cpp.
#include "ksp/eva/kerbal_eva.hpp"

#include <algorithm>

#include "ksp/career/game_variables.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/flight_globals.hpp"
#include "ksp/config/config_node.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/inventory/inventory.hpp"
#include "ksp/input/game_settings.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/modules/parachutes.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/resources/resource_flow_host.hpp"
#include "ksp/save/crew.hpp"
#include "ksp/science/science.hpp"
#include "ksp/inventory/construction.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/unity/random.hpp"
#include "ksp/util/misc_util.hpp"

namespace ksp {

using GE = events::GameEvents;
using KSP::Localization::Localizer;

namespace {
NullReferenceException Nre() { return NullReferenceException("Object reference not set to an instance of an object"); }

// bool.Parse
bool ParseBool(const std::string& s) {
    bool b = false;
    if (!dotnet::TryParseBool(s, b)) throw Exception("FormatException: String was not recognized as a valid Boolean.");
    return b;
}

void PostScreenMessage(const std::string& text, float duration, int style = 1 /*UPPER_CENTER*/) {
    EvaEnvironment::postedMessages.push_back(text);
    if (EvaEnvironment::postScreenMessage) EvaEnvironment::postScreenMessage(text, duration, style);
}

std::string KeyName(const std::shared_ptr<KeyBinding>& k) { return k ? k->name() : std::string(); }
}  // namespace

// ============================================================================================ KerbalAnimationState / Manager / small types

// KerbalAnimationState.cs:52
EvaAnimState* KerbalAnimationState::State() const {
    if (manager == nullptr || manager->evaController == nullptr) throw Nre();
    return manager->evaController->Engine().AnimState(GetAnimationString());
}

// KerbalAnimationState.cs:66
std::string KerbalAnimationState::GetAnimationString() const {
    if (manager == nullptr || manager->evaController == nullptr) throw Nre();
    if (manager->evaController->JetpackDeployed() && hasJpVariant) return "jp_" + animationName;
    return animationName;
}

// KerbalAnimationManager.cs:199
std::vector<KerbalAnimationState*> KerbalAnimationManager::GetAllAnimations() {
    std::vector<KerbalAnimationState*> list;
#define KSP_KERBAL_ANIM(n) list.push_back(&n);
#include "ksp/eva/kerbal_animations.inc"
#undef KSP_KERBAL_ANIM
    return list;
}

// KerbalAnimationManager.cs:150
void KerbalAnimationManager::Start(KerbalEVA* eva) {
    evaController = eva;
    // evaController.GetComponent<Animation>().wrapMode = WrapMode.Loop: the engine's default wrap mode (named effect)
    eva->Engine().Named("animation:wrapMode=Loop");
    std::vector<KerbalAnimationState*> all = GetAllAnimations();
    for (KerbalAnimationState* kas : all) {
        // setupAnimation (:184): layer / wrapMode / blendMode / weight go to the AnimationState; the mixing transforms are rig data (the shell)
        for (const std::string& nm : {kas->animationName}) {
            EvaAnimState* st = eva->Engine().AnimState(nm);
            if (st != nullptr) {
                st->layer = kas->layer;
                st->weight = kas->weight;
            }
        }
        if (kas->hasJpVariant) {
            EvaAnimState* st = eva->Engine().AnimState("jp_" + kas->animationName);
            if (st != nullptr) {
                st->layer = kas->layer;
                st->weight = kas->weight;
            }
        }
        kas->SetManager(this);
    }
    SyncAnimationLayers();
    RandomIdleAnims.push_back(&idle_a);
    RandomIdleAnims.push_back(&idle_b);
    RandomIdleAnims.push_back(&idle_c);
    RandomIdleAnims.push_back(&idle_d);
    RandomIdleAnims.push_back(&idle_f);
    RandomIdleAnims.push_back(&idle_g);
    RandomIdleAnims.push_back(&idle_i);
    controlPanelAnims.push_back(&controlpanel_a);
    controlPanelAnims.push_back(&controlpanel_b);
    controlPanelAnims.push_back(&controlpanel_c);
}

// KerbalAnimationManager.cs:189
void KerbalAnimationManager::SyncAnimationLayers() {
    size_t num = syncLayers.size();
    while (num-- > 0) evaController->Engine().AnimSyncLayer(syncLayers[num]);
}

// KerbalEVA.cs:47
ClamberPath::ClamberPath(const Vector3f& edgeFaceNormal_, const Vector3f& srfNormal_, const Vector3f& p1_, const Vector3f& p2_,
                         const Vector3f& p3_, float height, float standOff)
    : edgeFaceNormal(edgeFaceNormal_), srfNormal(srfNormal_) {
    clamberHeight = height;
    standOffDistance = standOff;
    p1 = p1_ + edgeFaceNormal_ * standOff;
    p2 = p2_ + (edgeFaceNormal_ + srfNormal_) * standOff;
    p3 = p3_ + srfNormal_ * standOff;
}

// KerbalRagdollNode.cs:18
void KerbalRagdollNode::updateVelocity(const Vector3f& rootPos, const Vector3d& rootVel, float fdtRecip) {
    Vector3f vector = goPosition - rootPos;
    // `velocity = (vector - lastPos) * fdtRecip + rootVel`: Vector3 * float (float per component), then Vector3d + Vector3d (the Vector3 widens
    // implicitly), the sum narrowed back to Vector3
    const Vector3f diff = (vector - lastPos) * fdtRecip;
    const Vector3d sum(static_cast<double>(diff.x) + rootVel.x, static_cast<double>(diff.y) + rootVel.y, static_cast<double>(diff.z) + rootVel.z);
    velocity = sum.ToVector3f();
    lastPos = vector;
}

// ============================================================================================ construction / access

KerbalEVA::KerbalEVA(const ModuleClassInfo& cls) : PartModule(cls) { live_.push_back(this); }

KerbalEVA::~KerbalEVA() {
    OnDestroy();
    live_.erase(std::remove(live_.begin(), live_.end(), this), live_.end());
}

IEvaEngine& KerbalEVA::Engine() {
    if (engine == nullptr) {
        defaultEngine_ = std::make_unique<HeadlessEvaEngine>();
        engine = defaultEngine_.get();
    }
    return *engine;
}

Part* KerbalEVA::PartPtr() const { return dynamic_cast<Part*>(part); }
Vessel* KerbalEVA::VesselPtr() const {
    Part* p = PartPtr();
    return p != nullptr ? p->vessel : nullptr;
}
PartTransform& KerbalEVA::transform() {
    Part* p = PartPtr();
    if (p == nullptr) throw Nre();
    return p->partTransform;
}

std::shared_ptr<Coroutine> KerbalEVA::StartCoroutine(Enumerator e) {
    return ScienceEnvironment::Scheduler().StartCoroutine(std::move(e), CoroutineOwner());
}
void KerbalEVA::StopCoroutine(std::shared_ptr<Coroutine>& c) {
    if (c && ScienceEnvironment::scheduler != nullptr) ScienceEnvironment::scheduler->StopCoroutine(c);
    c.reset();
}

bool KerbalEVA::Key(EvaKey k, KeyQuery q) {
    if (EvaEnvironment::key) return EvaEnvironment::key(k, q);
    std::shared_ptr<KeyBinding> b;
    switch (k) {
        case EvaKey::Forward: b = GameSettings::EVA_forward; break;
        case EvaKey::Back: b = GameSettings::EVA_back; break;
        case EvaKey::Left: b = GameSettings::EVA_left; break;
        case EvaKey::Right: b = GameSettings::EVA_right; break;
        case EvaKey::YawLeft: b = GameSettings::EVA_yaw_left; break;
        case EvaKey::YawRight: b = GameSettings::EVA_yaw_right; break;
        case EvaKey::PackForward: b = GameSettings::EVA_Pack_forward; break;
        case EvaKey::PackBack: b = GameSettings::EVA_Pack_back; break;
        case EvaKey::PackLeft: b = GameSettings::EVA_Pack_left; break;
        case EvaKey::PackRight: b = GameSettings::EVA_Pack_right; break;
        case EvaKey::PackUp: b = GameSettings::EVA_Pack_up; break;
        case EvaKey::PackDown: b = GameSettings::EVA_Pack_down; break;
        case EvaKey::Jump: b = GameSettings::EVA_Jump; break;
        case EvaKey::Run: b = GameSettings::EVA_Run; break;
        case EvaKey::TogglePack: b = GameSettings::EVA_TogglePack; break;
        case EvaKey::Use: b = GameSettings::EVA_Use; break;
        case EvaKey::Board: b = GameSettings::EVA_Board; break;
        case EvaKey::Orient: b = GameSettings::EVA_Orient; break;
        case EvaKey::Lights: b = GameSettings::EVA_Lights; break;
        case EvaKey::Helmet: b = GameSettings::EVA_Helmet; break;
        case EvaKey::ChuteDeploy: b = GameSettings::EVA_ChuteDeploy; break;
        case EvaKey::ToggleMovementMode: b = GameSettings::EVA_ToggleMovementMode; break;
    }
    if (!b) return false;
    switch (q) {
        case KeyQuery::Held: return b->GetKey();
        case KeyQuery::Down: return b->GetKeyDown();
        case KeyQuery::Up: return b->GetKeyUp();
    }
    return false;
}

float KerbalEVA::Axis(EvaAxis a) {
    if (EvaEnvironment::axis) return EvaEnvironment::axis(a);
    return 0.0f;
}

// ============================================================================================ properties

// KerbalEVA.cs:1130
bool KerbalEVA::IsChuteState() const {
    if (fsm->CurrentState() != st_semi_deployed_parachute) return fsm->CurrentState() == st_fully_deployed_parachute;
    return true;
}

// KerbalEVA.cs:1152 `base.vessel.state == Vessel.State.ACTIVE && !base.vessel.packed`
bool KerbalEVA::VesselUnderControl() const {
    Vessel* v = VesselPtr();
    if (v == nullptr) throw Nre();
    if (v->state == Vessel::State::ACTIVE) return !v->packed;
    return false;
}

// KerbalEVA.cs:1164 setter
void KerbalEVA::set_flagItems(int value) {
    set__flags(value);
    Events()["PlantFlag"]->SetGuiName(Localizer::Format("#autoLOC_6003096", std::vector<std::string>{std::to_string(value)}));
}

// KerbalEVA.cs:1179 / :1181
double KerbalEVA::Fuel() const {
    if (propellantResource == nullptr) throw Nre();
    return propellantResource->amount;
}
double KerbalEVA::FuelCapacity() const {
    if (propellantResource == nullptr) throw Nre();
    return propellantResource->maxAmount;
}

// KerbalEVA.cs:1183 setter
void KerbalEVA::set_currentLadderPart(Part* value) {
    if (_currentLadderPart != value) {
        if (_currentLadderPart != nullptr) GE::onPartLadderExit.Fire(this, _currentLadderPart);
        GE::onPartLadderEnter.Fire(this, value);
        _currentLadderPart = value;
    }
}

// KerbalEVA.cs:1205
bool KerbalEVA::OnALadder_() const {
    if (!fsm || !fsm->Started()) return false;
    return onLadder;
}

// ============================================================================================ PartModule lifecycle

// KerbalEVA.cs:6906
void KerbalEVA::OnAwake() {
    PartModule::OnAwake();
    PIDBoost = 1.0f;
    CacheLocalStrings();
    if (!localStringsEvent_) {
        GE::onLanguageSwitched.Add(this, &KerbalEVA::CacheLocalStrings);
        localStringsEvent_ = true;
    }
    inventoryPropellantResources.clear();
    // the [KSPEvent] methods (reflection binds them by name)
    Events().Bind("PickUpROC", BaseEvent::Handler([this] { PickUpROC(); }));
    Events().Bind("LowerVisor", BaseEvent::Handler([this] { LowerVisor(); }));
    Events().Bind("RaiseVisor", BaseEvent::Handler([this] { RaiseVisor(); }));
    Events().Bind("PlantFlag", BaseEvent::Handler([this] { PlantFlag(); }));
    Events().Bind("MakeReference", BaseEvent::Handler([this] { MakeReference(); }));
    Events().Bind("OnDeboardSeat", BaseEvent::Handler([this] { OnDeboardSeat(); }));
    Events().Bind("RenameVessel", BaseEvent::Handler([this] { RenameVessel(); }));
    Events().Bind("ChangeHelmet", BaseEvent::Handler([this] { ChangeHelmet(); }));
    Events().Bind("ChangeNeckRing", BaseEvent::Handler([this] { ChangeNeckRing(); }));
}

// KerbalEVA.cs:6979
void KerbalEVA::CacheLocalStrings() {
    auto fmt = [](const char* tag, std::vector<std::string> args) { return Localizer::Format(tag, args); };
    cacheAutoLOC_114130 = fmt("#autoLOC_114130", {KeyName(GameSettings::EVA_Use)});
    cacheAutoLOC_114293 = fmt("#autoLOC_114293", {KeyName(GameSettings::EVA_Use)});
    cacheAutoLOC_114297 = fmt("#autoLOC_114297", {KeyName(GameSettings::EVA_Use)});
    cacheAutoLOC_114358 = fmt("#autoLOC_114358", {KeyName(GameSettings::EVA_Board)});
    cacheAutoLOC_115662 = fmt("#autoLOC_115662", {KeyName(GameSettings::EVA_Jump)});
    cacheAutoLOC_115694 = fmt("#autoLOC_115694", {KeyName(GameSettings::EVA_Jump)});
    cacheAutoLOC_6010008 = Localizer::Format("#autoLOC_6010008");
    cacheAutoLOC_6010009 = Localizer::Format("#autoLOC_6010009");
    cacheAutoLOC_6010010 = Localizer::Format("#autoLOC_6010010");
    cacheAutoLOC_6010011 = Localizer::Format("#autoLOC_6010011");
    cacheAutoLOC_8003204 = Localizer::Format("#autoLOC_8003204");
    cacheAutoLOC_6010015 = Localizer::Format("#autoLOC_6010015");
    cacheAutoLOC_8002357 = fmt("#autoLOC_8002357", {KeyName(GameSettings::EVA_Jump)});
    cacheAutoLOC_8002358 = fmt("#autoLOC_8002358", {KeyName(GameSettings::PAUSE)});
    cacheAutoLOC_8002359 = fmt("#autoLOC_8002359", {KeyName(GameSettings::TRANSLATE_LEFT), KeyName(GameSettings::TRANSLATE_RIGHT)});
    cacheAutoLOC_8002360 = fmt("#autoLOC_8002360", {KeyName(GameSettings::TRANSLATE_LEFT), KeyName(GameSettings::TRANSLATE_RIGHT),
                                                    KeyName(GameSettings::TRANSLATE_FWD), KeyName(GameSettings::TRANSLATE_BACK),
                                                    KeyName(GameSettings::TRANSLATE_DOWN), KeyName(GameSettings::TRANSLATE_UP)});
    cacheAutoLOC_6012032 = fmt("#autoLOC_6012032", {KeyName(GameSettings::EVA_Use)});
    cacheAutoLOC_6006049 = Localizer::Format("#autoLOC_6006049");
}

// KerbalEVA.cs:1426
void KerbalEVA::OnLoad(const ConfigNode& node) {
    std::optional<std::string> value = node.GetValue("isCfg");
    if (value && !value->empty() && ParseBool(*value)) {
        loadedFromSFS = false;
    } else {
        loadedFromSFS = true;
        log::Info("[KerbalEVA]: Loaded, added rigidbody");
        if (node.HasValue("state")) loadedStateName = *node.GetValue("state");
        if (node.HasValue("step")) set_lastBoundStep(dotnet::ParseSingle(*node.GetValue("step")));
    }
    if (node.HasValue("packExt")) set_JetpackDeployed(ParseBool(*node.GetValue("packExt")));
    if (node.HasValue("lightOn")) set_lampOn(ParseBool(*node.GetValue("lightOn")));
    if (node.HasValue("flags")) set_flagItems(dotnet::ParseInt32(*node.GetValue("flags")));
    else set_flagItems(_flags());
    if (node.HasNode("vInfo")) {
        if (!kerbalVesselInfo) kerbalVesselInfo = std::make_unique<DockedVesselInfo>();
        kerbalVesselInfo->Load(*node.GetNode("vInfo"));
    }
    if (node.HasValue("useGlobalPhysicMaterial")) set_useGlobalPhysicMaterial(ParseBool(*node.GetValue("useGlobalPhysicMaterial")));
    else set_useGlobalPhysicMaterial(true);
    if (!useGlobalPhysicMaterial()) {
        PhysicMaterialData m;
        m.dynamicFriction = PhysicsGlobals::KerbalEVADynamicFriction();
        m.staticFriction = PhysicsGlobals::KerbalEVAStaticFriction();
        m.bounciness = PhysicsGlobals::KerbalEVABounciness();
        m.frictionCombine = static_cast<int>(PhysicsGlobals::KerbalEVAFrictionCombine());
        m.bounceCombine = static_cast<int>(PhysicsGlobals::KerbalEVABounceCombine());
        float v = m.dynamicFriction;
        if (node.HasValue("kerbalEVADynamicFriction") && dotnet::TryParseSingle(*node.GetValue("kerbalEVADynamicFriction"), v)) m.dynamicFriction = v;
        v = m.staticFriction;
        if (node.HasValue("kerbalEVAStaticFriction") && dotnet::TryParseSingle(*node.GetValue("kerbalEVAStaticFriction"), v)) m.staticFriction = v;
        v = m.bounciness;
        if (node.HasValue("kerbalEVABounciness") && dotnet::TryParseSingle(*node.GetValue("kerbalEVABounciness"), v)) m.bounciness = v;
        int e = m.frictionCombine;
        if (node.HasValue("kerbalEVAFrictionCombine") && dotnet::TryParseInt32(*node.GetValue("kerbalEVAFrictionCombine"), e)) m.frictionCombine = e;
        e = m.bounceCombine;
        if (node.HasValue("kerbalEVABounceCombine") && dotnet::TryParseInt32(*node.GetValue("kerbalEVABounceCombine"), e)) m.bounceCombine = e;
        physicMaterial = m;
        SetPhysicMaterial(m);
    }
}

// KerbalEVA.cs:1846
void KerbalEVA::OnSave(ConfigNode& node) {
    Part* p = PartPtr();
    if (fsm && fsm->Started()) {
        node.AddValue("state", fsm->currentStateName);
        kerbalVesselInfo->Save(*node.AddNode("vInfo"));
    }
    if (evaChute != nullptr && p != nullptr && !p->protoModuleCrew.empty()) {
        if (EvaEnvironment::named) EvaEnvironment::Effect("SaveEVAChute");
        else EvaEnvironment::namedLog.push_back("SaveEVAChute");
    }
    if (moduleInventoryPartReference != nullptr && p != nullptr && !p->protoModuleCrew.empty()) {
        p->protoModuleCrew[0]->SaveInventory(moduleInventoryPartReference);
        if (KerbalInventoryScenario::Instance != nullptr) KerbalInventoryScenario::Instance->RemoveKerbalInventoryInstance(p->protoModuleCrew[0]->name().value_or(""));
    }
    if (!useGlobalPhysicMaterial()) {
        node.AddValue("kerbalEVADynamicFriction", physicMaterial->dynamicFriction);
        node.AddValue("kerbalEVAStaticFriction", physicMaterial->staticFriction);
        node.AddValue("kerbalEVABounciness", physicMaterial->bounciness);
        node.AddValue("kerbalEVAFrictionCombine", physicMaterial->frictionCombine);
        node.AddValue("kerbalEVABounceCombine", physicMaterial->bounceCombine);
    }
}

// KerbalEVA.cs:1505
void KerbalEVA::OnStart(StartState state) {
    isEnabled = true;
    Part* p = PartPtr();
    if (p == nullptr) throw Nre();
    if (!eventsSubscribed_) {
        GE::onVesselGoOnRails.Add(this, &KerbalEVA::OnVesselGoOnRails);
        GE::onVesselGoOffRails.Add(this, &KerbalEVA::OnVesselGoOffRails);
        GE::onKrakensbaneEngage.Add(this, &KerbalEVA::onFrameVelocityChange);
        GE::onKrakensbaneDisengage.Add(this, &KerbalEVA::onFrameVelocityChange);
        GE::onRotatingFrameTransition.Add(this, &KerbalEVA::onRotatingFrameChanged);
        GE::onDominantBodyChange.Add(this, &KerbalEVA::onReferencebodyChanged);
        GE::onVesselSituationChange.Add(this, &KerbalEVA::OnVesselSituationChange);
        GE::OnHelmetChanged.Add(this, &KerbalEVA::OnHelmetChanged);
        GE::onVesselChange.Add(this, &KerbalEVA::OnVesselChange);
        GE::OnROCExperimentStored.Add(this, &KerbalEVA::OnROCExperimentFinished);
        GE::OnROCExperimentReset.Add(this, &KerbalEVA::OnROCExperimentReset);
        GE::onPartDie.Add(this, &KerbalEVA::OnPartEvent);
        GE::OnVisorRaised.Add(this, &KerbalEVA::OnVisorRaised);
        GE::OnEVAConstructionWeldStart.Add(this, &KerbalEVA::OnWeldStart);
        GE::OnEVAConstructionWeldFinish.Add(this, &KerbalEVA::OnWeldFinish);
        eventsSubscribed_ = true;
    }
    if (!p->Modules().Contains("ModuleTripLogger")) p->AddModule("ModuleTripLogger");
    p->fields.waterAngularDragMultiplier = waterAngularDragMultiplier();
    if (PhysicsGlobals::Instance() != nullptr) p->fields.buoyancy = PhysicsGlobals::BuoyancyKerbals();
    moduleInventoryPartReference = dynamic_cast<ModuleInventoryPart*>(p->FindModuleOfClass("ModuleInventoryPart"));
    evaChute = dynamic_cast<ModuleEvaChute*>(p->FindModuleOfClass("ModuleEvaChute"));
    GE::onModuleInventoryChanged.Add(this, &KerbalEVA::OnModuleInventoryChanged);
    // ModuleLiftingSurface[] components = GetComponents<ModuleLiftingSurface>(): the chute canopy lift is the aero package's (chuteLiftingSurfaces
    // are enabled / disabled through the engine effect "chuteLift:<on|off>" by OnParachuteFullyDeployed / Cut)
    startEvaCoroutine_ = StartCoroutine(StartEVA(state));
    // myExpressionSystem / suit textures / helmet setup (the crew member and the renderers)
    if (!p->protoModuleCrew.empty() && p->protoModuleCrew[0] != nullptr) {
        InitHelmetSetup();
    } else {
        log::Error("Unable to get the Proto Crew Member on Kerbal Eva (InitHelmetSetup)");
    }
    if (!p->protoModuleCrew.empty() && p->protoModuleCrew[0] != nullptr) {
        UnityRandom::Global().InitState(StringExtensions::GetHashCode_Net35(p->protoModuleCrew[0]->name().value_or("")));
        KerbalAvatarUpdateInterval = UnityRandom::Global().Range(0.1f, 0.15f);
    }
    Vessel* v = p->vessel;
    if (FlightRegistry::ActiveVessel != nullptr && v != nullptr && FlightRegistry::ActiveVessel->id == v->id) isActiveVessel = true;
    if (GameSettings::EVA_SHOW_PORTRAIT) {
        EvaEnvironment::Effect("portrait:setup");
        updateAvatarCoroutine_ = nullptr;  // kerbalAvatarUpdateCycle is a render loop: the shell's (portrait:setup)
    }
    // moduleScienceExperiments / moduleScienceExperimentROC / suitColorChanger (ModuleScienceExperiment / ModuleColorChanger lookups)
    if (EvaEnvironment::IsInstalled("Serenity")) {
        std::shared_ptr<ProtoCrewMember> pcm = p->protoModuleCrew.empty() ? nullptr : p->protoModuleCrew[0];
        if (pcm != nullptr) {
            Set<float>("lightR", pcm->lightR);
            Set<float>("lightG", pcm->lightG);
            Set<float>("lightB", pcm->lightB);
        }
    } else {
        Fields()["lightR"]->guiActive = false;
        Fields()["lightG"]->guiActive = false;
        Fields()["lightB"]->guiActive = false;
    }
    SetupEVAScienceSoundFX();
    EvaEnvironment::Effect("SetupSoundFX");
}

// KerbalEVA.cs:1673
void KerbalEVA::OnStartFinished(StartState state) {
    PartModule::OnStartFinished(state);
    UpdatePackModels();
}

// KerbalEVA.cs:1753
Enumerator KerbalEVA::StartEVA(StartState state) {
    (void)state;
    Part* p = PartPtr();
    if (p == nullptr) throw Nre();
    // advRagdoll = GetComponent<AdvancedRagdoll>() ?? AddComponent: the ragdoll rig (shell)
    EvaEnvironment::Effect("advRagdoll:attach");
    if (EvaEnvironment::loadedSceneIsFlight) {
        // part.partInfo = new AvailablePart(part.partInfo): the per-kerbal part info (name / title) is the roster's; recorded
        if (!p->protoModuleCrew.empty() && p->protoModuleCrew[0] != nullptr) EvaEnvironment::Effect("partInfo:" + p->protoModuleCrew[0]->GetKerbalEVAPartName());
        else EvaEnvironment::Effect("partInfo:kerbalEVA");
    }
    if (!kerbalVesselInfo) {
        kerbalVesselInfo = std::make_unique<DockedVesselInfo>();
        kerbalVesselInfo->name = p->vessel->vesselName;
        kerbalVesselInfo->vesselType = VesselType::EVA;
        kerbalVesselInfo->rootPartUId = p->fields.flightID;
    }
    SetupAnimations();
    SetupRagdoll(*p);
    set_ladderPushoffForce(f32store(fx(ladderPushoffForce()) * fx(massMultiplier())));
    set_maxJumpForce(f32store(fx(maxJumpForce()) * fx(massMultiplier())));
    set_boundForce(f32store(fx(boundForce()) * fx(massMultiplier())));
    set_linPower(f32store(fx(linPower()) * fx(massMultiplier())));
    ResetRagdollLinks();
    SetRagdoll(false);
    Events()["PlantFlag"]->active = false;
    Events()["OnDeboardSeat"]->active = false;
    Events()["MakeReference"]->active = false;
    Events()["RenameVessel"]->active = false;
    SetupFSM();
    SetupJetpackEffects();
    co_yield YieldInstruction::Null();
    if (loadedFromSFS && !loadedStateName.empty()) {
        if (loadedStateName.find("Ladder") != std::string::npos) {
            if (!currentLadderTriggers.empty()) loadedStateName = st_ladder_acquire->name;
            else loadedStateName = st_idle_fl->name;
        }
        try {
            fsm->StartFSM(loadedStateName);
            Ready = true;
        } catch (const std::exception& ex) {
            log::Info(ex.what());
            Ready = false;
        }
    }
    if (!Ready) {
        if (!currentLadderTriggers.empty() && autoGrabLadderOnStart) fsm->StartFSM(st_ladder_acquire);
        else if (SurfaceContact()) fsm->StartFSM(st_idle_gr);
        else if (p->vessel->Splashed) fsm->StartFSM(st_swim_idle);
        else fsm->StartFSM(st_idle_fl);
        Ready = true;
    }
    fsm->OnStateChange = fsm->OnStateChange + Callback<KFSMState*, KFSMState*, KFSMEvent*>::Bind(this, &KerbalEVA::UpdateInventoryPaw);
    ToggleJetpack(JetpackDeployed());
    EvaEnvironment::Effect(std::string("headLamp:") + (lampOn() ? "on" : "off"));
    {
        const auto& crew = p->vessel->GetVesselCrew();
        const bool future = !crew.empty() && crew[0] != nullptr && crew[0]->suit() == ProtoCrewMember::KerbalSuit::Future;
        Fields()["colorChanger"]->guiActive = future;
    }
    ResetOrientationPID();
    // while (!part.started) yield return null: Part.started is the host's Start() flag; recorded by a named effect poll
    EvaEnvironment::Effect("part:started");
    EvaEnvironment::Effect("collisionEnhancer:TRANSLATE_BACK");
    EvaEnvironment::Effect("part:SetReferenceTransform");
    co_return;
}

// KerbalEVA.cs:6915
void KerbalEVA::OnDestroy() {
    static thread_local bool inDestroy = false;
    (void)inDestroy;
    if (fsm && fsm->CurrentState() != nullptr && fsm->CurrentState() == st_flagPlant) {
        Vessel* v = VesselPtr();
        if (v != nullptr) {
            UnlockInput("FlagDeployLock_" + v->id.ToString());
        }
    }
    if (eventsSubscribed_) {
        GE::OnHelmetChanged.Remove(this, &KerbalEVA::OnHelmetChanged);
        GE::onVesselGoOnRails.Remove(this, &KerbalEVA::OnVesselGoOnRails);
        GE::onVesselGoOffRails.Remove(this, &KerbalEVA::OnVesselGoOffRails);
        GE::onKrakensbaneEngage.Remove(this, &KerbalEVA::onFrameVelocityChange);
        GE::onKrakensbaneDisengage.Remove(this, &KerbalEVA::onFrameVelocityChange);
        GE::onRotatingFrameTransition.Remove(this, &KerbalEVA::onRotatingFrameChanged);
        GE::onDominantBodyChange.Remove(this, &KerbalEVA::onReferencebodyChanged);
        GE::onVesselSituationChange.Remove(this, &KerbalEVA::OnVesselSituationChange);
        GE::onVesselChange.Remove(this, &KerbalEVA::OnVesselChange);
        GE::onPartDie.Remove(this, &KerbalEVA::OnPartEvent);
        GE::onModuleInventoryChanged.Remove(this, &KerbalEVA::OnModuleInventoryChanged);
        GE::OnVisorRaised.Remove(this, &KerbalEVA::OnVisorRaised);
        GE::OnEVAConstructionWeldStart.Remove(this, &KerbalEVA::OnWeldStart);
        GE::OnEVAConstructionWeldFinish.Remove(this, &KerbalEVA::OnWeldFinish);
        GE::OnROCExperimentStored.Remove(this, &KerbalEVA::OnROCExperimentFinished);
        GE::OnROCExperimentReset.Remove(this, &KerbalEVA::OnROCExperimentReset);
        eventsSubscribed_ = false;
    }
    if (localStringsEvent_) {
        GE::onLanguageSwitched.Remove(this, &KerbalEVA::CacheLocalStrings);
        localStringsEvent_ = false;
    }
    if (fsm) fsm->OnStateChange = fsm->OnStateChange - Callback<KFSMState*, KFSMState*, KFSMEvent*>::Bind(this, &KerbalEVA::UpdateInventoryPaw);
    StopCoroutine(startEvaCoroutine_);
    StopCoroutine(returnIDLE_);
    StopCoroutine(startExpression_);
    StopCoroutine(updateAvatarCoroutine_);
    if (ScienceEnvironment::scheduler != nullptr) ScienceEnvironment::scheduler->StopAllCoroutines(CoroutineOwner());
    if (_currentLadderPart != nullptr) _currentLadderPart->fields.hasKerbalOnLadder = false;
}

// KerbalEVA.cs:6964
void KerbalEVA::OnInactive() {
    Part* p = PartPtr();
    if (evaChute == nullptr) throw Nre();
    EvaEnvironment::Effect("evaChute:OnInactive");  // CutParachute / Repack / SaveEVAChute: ModuleEvaChute + roster (state read through its field)
    if (moduleInventoryPartReference != nullptr && p != nullptr && !p->protoModuleCrew.empty()) p->protoModuleCrew[0]->SaveInventory(moduleInventoryPartReference);
    PartModule::OnInactive();
}

// KerbalEVA.cs:1875
void KerbalEVA::OnVesselChange(Vessel* vsl) {
    Vessel* v = VesselPtr();
    isActiveVessel = (vsl->id == v->id);
    if (isActiveVessel) {
        if (!updateAvatarCoroutine_ && GameSettings::EVA_SHOW_PORTRAIT) EvaEnvironment::Effect("portrait:start");
    } else {
        EvaEnvironment::Effect("portrait:stop");
    }
}

// KerbalEVA.cs:1892
void KerbalEVA::OnPartEvent(Part* p) {
    if (currentAirlockPart != nullptr && p->fields.persistentId == currentAirlockPart->fields.persistentId) currentAirlockPart = nullptr;
}

// KerbalEVA.cs:1900
void KerbalEVA::UpdateInventoryPaw(KFSMState*, KFSMState*, KFSMEvent*) {
    if (moduleInventoryPartReference != nullptr) moduleInventoryPartReference->KerbalStateChanged();
}

// KerbalEVA.cs:1908
void KerbalEVA::SetPortraitCameraDistance(float newDistance) {
    if (GameSettings::EVA_SHOW_PORTRAIT) EvaEnvironment::Effect("portraitCamera.z=" + dotnet::FormatSingleG(newDistance, 9));
}

// KerbalEVA.cs:1948
void KerbalEVA::SetVisibleInPortrait(bool visible) {
    visibleInPortrait = visible;
    EvaEnvironment::Effect(visible ? "portrait:start" : "portrait:stop");
}

// KerbalEVA.cs:1968
void KerbalEVA::SetupAnimations() { Animations().Start(this); }

// KerbalEVA.cs:4354 / :4362
void KerbalEVA::OnFSMStateChange(KFSMState* oldState, KFSMState* newState, KFSMEvent* fsmEvent) {
    if (DebugFSMState) {
        Part* p = PartPtr();
        log::Info("[KerbalEVA]: Part:" + (p && p->partInfo ? p->partInfo->title : std::string()) + "-" + std::to_string(p ? p->fields.persistentId : 0) +
                  " FSM State Changed, Old State:" + oldState->name + " New State:" + newState->name + " Event:" + fsmEvent->name);
    }
}
void KerbalEVA::OnFSMEventCalled(KFSMEvent* fsmEvent) {
    if (DebugFSMState) {
        Part* p = PartPtr();
        log::Info("[KerbalEVA]: Part:" + (p && p->partInfo ? p->partInfo->title : std::string()) + "-" + std::to_string(p ? p->fields.persistentId : 0) +
                  " FSM Event Called, Event:" + fsmEvent->name);
    }
}

// ============================================================================================ main loops

// KerbalEVA.cs:4370
void KerbalEVA::FixedUpdate() {
    Vessel* v = VesselPtr();
    if (!v->packed) {
        getCoordinateFrame();
        lastDeltaHdg = deltaHdg;
        deltaHdg = 0.0f;
        cmdRot = Vector3f::zero();
        packLinear = Vector3f::zero();
        fuelFlowRate = 0.0f;
        tgtRpos = Vector3f::zero();
        ladderTgtRPos = Vector3f::zero();
        packTgtRPos = Vector3f::zero();
        packRRot = Vector3f::zero();
        HandleMovementInput();
        fsm->FixedUpdateFSM();
        JetpackIsThrusting = fx(fuelFlowRate) > fx(PropellantConsumption()) / 2.0 * (fx(thrustPercentage()) * fx(0.01f));
        UpdatePackFuel();
        UpdateHelmetOffChecks();
        UpdateVisorPosition();
    }
}

// KerbalEVA.cs:4422
void KerbalEVA::Update() {
    Vessel* v = VesselPtr();
    Part* p = PartPtr();
    if (v != nullptr && v->vesselType == VesselType::Debris && p != nullptr && p->parent == nullptr) {
        v->vesselType = VesselType::EVA;
        if (!p->protoModuleCrew.empty()) v->vesselName = p->protoModuleCrew[0]->name().value_or("");
        log::Warning("Kerbal " + v->vesselName + " was marked as debris, fixed.");
    }
    fsm->UpdateFSM();
    if (PartPlacementMode() && moduleInventoryPartReference != nullptr && moduleInventoryPartReference->SelectedPart() != nullptr) {
        PostInteractionScreenMessage(cacheAutoLOC_8002357);
        PostInteractionScreenMessage(cacheAutoLOC_8002358);
        if (!moduleInventoryPartReference->placementAllowXRotation && !moduleInventoryPartReference->placementAllowZRotation)
            PostInteractionScreenMessage(cacheAutoLOC_8002359);
        else
            PostInteractionScreenMessage(cacheAutoLOC_8002360);
    }
    if (v->packed) return;
    if (Key(EvaKey::Lights, KeyQuery::Down)) ToggleLamp();
    if (Key(EvaKey::Helmet, KeyQuery::Down)) ChangeHelmet();
    if (startTimer) {
        newidleCounter = f32store(fx(newidleCounter) - fx(Time::deltaTime));
        if (newidleCounter <= 0.0f) {
            newidleCounter = newidleTime;
            startTimer = false;
            fsm->RunEvent(On_idle_b_gr);
        }
    }
    updateJetpackEffects();
    Animations().SyncAnimationLayers();
    // kerbalCamAtmos.transform.rotation = kerbalPortraitCamera.transform.rotation: portrait cameras (shell)
    if (onLadder) {
        // topLadderEnd.Reached / bottomLadderEnd.Reached (LadderEndCheck): the end-check triggers are the shell's, reported through canClimb /
        // canDescend inputs
        canClimb = !topLadderEndReached;
        canDescend = !bottomLadderEndReached;
        if (!canClimb && !canDescend) {
            canDescend = true;
            canClimb = true;
        }
    } else {
        canClimb = false;
        canDescend = false;
    }
}

// KerbalEVA.cs:4493
void KerbalEVA::LateUpdate() {
    if (!VesselPtr()->packed) {
        fsm->LateUpdateFSM();
        AnchorUpdate();
    }
}

}  // namespace ksp
