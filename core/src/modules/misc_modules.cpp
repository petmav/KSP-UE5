// WP-30: ModuleGPS.cs, FlagSite.cs, ModulePartFirework.cs (see include/ksp/modules/misc_modules.hpp for the seams).
#include "ksp/modules/misc_modules.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/modules/base_event.hpp"
#include "ksp/modules/resource_processing.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/resources/planetary_resources.hpp"
#include "ksp/resources/resource_flow_host.hpp"
#include "ksp/unity/random.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {
namespace {
using GE = events::GameEvents;

Part* PartOfModule(const PartModule& m) { return dynamic_cast<Part*>(m.part); }

BaseField& FieldOrThrow(PartModule& m, const char* name) {  // base.Fields["name"] (null -> NullReferenceException on use)
    BaseField* f = m.Fields()[name];
    if (f == nullptr) throw NullReferenceException();
    return *f;
}
BaseEvent& EventOrThrow(PartModule& m, const char* name) {  // base.Events["name"]
    BaseEvent* e = m.Events()[name];
    if (e == nullptr) throw NullReferenceException();
    return *e;
}
}  // namespace

void ModuleGPSEnvironment::Reset() {
    itemListContains = nullptr;
    printed.clear();
}
void FlagSiteEnvironment::Reset() {
    hasAnimationRoot = true;
    flagDeployAnimationLength = 3.0;
    playDeployAnimation = nullptr;
    showDeployedPose = nullptr;
    setCollidersEnabled = nullptr;
    setVisible = nullptr;
    createJoint = nullptr;
    destroyJoint = nullptr;
    setJointBreakForce = nullptr;
    evaAddFlag = nullptr;
    spawnSiteRenameDialog = nullptr;
    dismissSiteRenameDialog = nullptr;
    spawnPlaqueDialog = nullptr;
    terminatePlaqueDialog = nullptr;
    controlLocks.clear();
}
void FireworkEnvironment::Reset() {
    shellPrefabExists = nullptr;
    fxDefinitions.clear();
    cannon = nullptr;
    activeVesselInAtmosphere = nullptr;
    configureSoundFX = nullptr;
    krakensbaneFrameSpeed = nullptr;
    vesselRbVelocityD = nullptr;
    launchShell = nullptr;
    setPickerColor = nullptr;
    setTertiaryColorVisible = nullptr;
    presetColors.clear();
}

// ================================================================ ModuleGPS

namespace {
const ProcessingVessel* GpsVessel(PartModule& m) { return ResourceProcessingEnvironment::VesselOf(PartOfModule(m)); }
}  // namespace

// ModuleGPS.cs:168 (the cacheAutoLOC_* statics are Localizer.Format values taken at use)
void ModuleGPS::CacheLocalStrings() {}

// ModuleGPS.cs:37
bool ModuleGPS::CheckForAsteroidConnect() {
    const ProcessingVessel* v = GpsVessel(*this);
    if (v == nullptr) throw NullReferenceException();
    const size_t count = v->parts.size();
    for (size_t num = 0; num < count; num++) {
        if (v->parts[num]->HasModuleImplementing([](PartModule& m) { return m.IsA("ModuleAsteroid"); })) {
            Set<std::string>("body", Localizer::Format("#autoLOC_258910"));
            Set<std::string>("bioName", Localizer::Format("#autoLOC_258911"));
            Set<std::string>("lat", Localizer::Format("#autoLOC_258912"));
            Set<std::string>("lon", Localizer::Format("#autoLOC_258913"));
            return true;
        }
    }
    return false;
}

// ModuleGPS.cs:61
bool ModuleGPS::CheckForCometConnect() {
    const ProcessingVessel* v = GpsVessel(*this);
    if (v == nullptr) throw NullReferenceException();
    const size_t count = v->parts.size();
    for (size_t num = 0; num < count; num++) {
        if (v->parts[num]->HasModuleImplementing([](PartModule& m) { return m.IsA("ModuleComet"); })) {
            Set<std::string>("body", Localizer::Format("#autoLOC_6005065"));
            Set<std::string>("bioName", Localizer::Format("#autoLOC_258911"));
            Set<std::string>("lat", Localizer::Format("#autoLOC_258912"));
            Set<std::string>("lon", Localizer::Format("#autoLOC_258913"));
            return true;
        }
    }
    return false;
}

// ModuleGPS.cs:85
void ModuleGPS::Update() {
    secondsSinceLastDisplayUpdate = f32store(fx(secondsSinceLastDisplayUpdate) + fx(Time::deltaTime));
    if (secondsSinceLastDisplayUpdate < secondsUpdateRate) return;
    secondsSinceLastDisplayUpdate = 0.0f;
    if (!ResourceFlowEnvironment::LoadedSceneIsFlight()) return;
    const ProcessingVessel* v = GpsVessel(*this);
    if (v != nullptr && !v->parts.empty()) isConnectedToAsteroid = CheckForAsteroidConnect();
    if (isConnectedToAsteroid) return;
    if (v != nullptr && !v->parts.empty()) isConnectedToComet = CheckForCometConnect();
    if (isConnectedToComet || !ModuleGPSEnvironment::itemListContains || !ModuleGPSEnvironment::itemListContains(*PartOfModule(*this))) return;
    try {
        if (v == nullptr) throw NullReferenceException();
        // FlightGlobals.currentMainBody: the main body of the vessel's record
        const double num = ResourceUtilities::clampLat(v->latitude);
        const double num2 = ResourceUtilities::clampLon(v->longitude);
        const double num3 = ResourceUtilities::Deg2Rad(num);
        const double num4 = ResourceUtilities::Deg2Rad(num2);
        std::optional<ResourceProcessingEnvironment::Biome> biome;
        if (ResourceProcessingEnvironment::biomeAt) biome = ResourceProcessingEnvironment::biomeAt(num3, num4, *PartOfModule(*this));
        // Math.Abs(x).ToString("0.000") is the fixed three-decimal text of a finite double (FormatDoubleF)
        Set<std::string>("body", Localizer::Format("#autoLOC_7001301", {v->mainBody.displayName}));
        Set<std::string>("lat", Localizer::Format("#autoLOC_6001052", {dotnet::FormatDoubleF(std::fabs(num), 3), dotnet::FormatInt64(num >= 0.0 ? 1 : 0),
                                                                        dotnet::FormatDoubleF(num3, 3)}));
        Set<std::string>("lon", Localizer::Format("#autoLOC_6001053", {dotnet::FormatDoubleF(std::fabs(num2), 3), dotnet::FormatInt64(num2 >= 0.0 ? 1 : 0),
                                                                        dotnet::FormatDoubleF(num4, 3)}));
        if (biome) Set<std::string>("bioName", Localizer::Format("#autoLOC_7001301", {biome->displayname}));
        else Set<std::string>("bioName", v->SituationString);
    } catch (const std::exception&) {
        ModuleGPSEnvironment::printed.push_back("[RESOURCES] Error updating ModuleGPS");
    }
}

// ModuleGPS.cs:139
void ModuleGPS::EnableModule() { isEnabled = true; }
// ModuleGPS.cs:144
void ModuleGPS::DisableModule() { isEnabled = false; }
// ModuleGPS.cs:149
bool ModuleGPS::ModuleIsActive() { return isEnabled; }
// ModuleGPS.cs:154
bool ModuleGPS::IsSituationValid() { return !isConnectedToAsteroid && !isConnectedToComet; }
// ModuleGPS.cs:163
std::string ModuleGPS::GetModuleDisplayName() { return Localizer::Format("#autoLOC_8004214"); }

// ================================================================ FlagSite

Part* FlagSite::PartOf() const { return PartOfModule(*this); }

FlagSite::~FlagSite() {
    if (subscribed_) events::GameEvents::onGameSceneLoadRequested.Remove(this, &FlagSite::onGameSceneLoadRequested);
}

void FlagSite::OnAwake() {
    PartModule::OnAwake();
    if (bound_) return;
    bound_ = true;
    Events().Bind("TakeDown", BaseEvent::Handler([this] { TakeDown(); }));
    Events().Bind("PickUp", BaseEvent::Handler([this] { PickUp(); }));
    Events().Bind("ReadPlaque", BaseEvent::Handler([this] { ReadPlaque(); }));
}

// FlagSite.cs:67
void FlagSite::Start() {
    isEnabled = true;
    EventOrThrow(*this, "TakeDown").active = false;
    EventOrThrow(*this, "PickUp").active = false;
    if (FlagSiteEnvironment::hasAnimationRoot) SetupFSM();
    events::GameEvents::onGameSceneLoadRequested.Add(this, &FlagSite::onGameSceneLoadRequested);
    subscribed_ = true;
}

// FlagSite.cs:79
void FlagSite::OnDestroy() {
    events::GameEvents::onGameSceneLoadRequested.Remove(this, &FlagSite::onGameSceneLoadRequested);
    subscribed_ = false;
}

// FlagSite.cs:84
void FlagSite::onGameSceneLoadRequested(GameScenes) {
    if (siteRenameDialogOpen) DismissSiteRename();
    if (flagPlaqueDialogOpen) {
        if (FlagSiteEnvironment::terminatePlaqueDialog) FlagSiteEnvironment::terminatePlaqueDialog(*this);
        onFlagPlaqueDialogDismiss();
    }
}

// FlagSite.cs:97
void FlagSite::OnLoad(const ConfigNode& node) {
    if (node.HasValue("state")) stateName = node.GetValue("state");
}

// FlagSite.cs:105
void FlagSite::Update() {
    if (!fsm) throw NullReferenceException();
    if (fsm->Started()) fsm->UpdateFSM();
}

// FlagSite.cs:113
void FlagSite::FixedUpdate() {
    if (!fsm) throw NullReferenceException();
    if (fsm->Started()) fsm->FixedUpdateFSM();
}

// FlagSite.cs:121
void FlagSite::LateUpdate() {
    if (!fsm) throw NullReferenceException();
    if (fsm->Started()) fsm->LateUpdateFSM();
}

// FlagSite.cs:129
void FlagSite::OnSave(ConfigNode& node) {
    if (fsm) node.AddValue("state", fsm->currentStateName);
}

// FlagSite.cs:137
void FlagSite::SetupFSM() {
    using FE = FlagSiteEnvironment;
    ownedStates_.clear();
    ownedEvents_.clear();
    fsm = std::make_unique<KerbalFSM>();
    auto newState = [this](const char* n) {
        ownedStates_.push_back(std::make_unique<KFSMState>(n));
        return ownedStates_.back().get();
    };
    auto setColliders = [this](bool v) {
        if (FE::setCollidersEnabled) FE::setCollidersEnabled(*this, v);
    };
    st_placing = newState("Placing");
    st_placing->OnEnter = KFSMStateChange::Lambda(this, [this, setColliders](KFSMState*) {
        if (FE::playDeployAnimation) FE::playDeployAnimation(*this);
        setColliders(false);
        MakeInvisible();
        pendingInvokes.emplace_back("MakeVisible", Get<float>("deployVisibilityDelay"));  // Invoke("MakeVisible", deployVisibilityDelay)
        Part* p = PartOf();
        if (p == nullptr) throw NullReferenceException();
        if (!p->fields.packed) SetJoint();
        FE::controlLocks["Flag_NoInterruptWhileDeploying"] = "PAUSE|TIMEWARP|QUICKSAVE|QUICKLOAD|VESSEL_SWITCHING";
    });
    st_placing->OnLeave = KFSMStateChange::Lambda(this, [this](KFSMState*) {
        MakeVisible();
        if (fsm->LastEvent() != onPlaceComplete) UnsetJoint();
        FE::controlLocks.erase("Flag_NoInterruptWhileDeploying");
    });
    fsm->AddState(st_placing);
    st_placed = newState("Placed");
    st_placed->OnEnter = KFSMStateChange::Lambda(this, [this, setColliders](KFSMState*) {
        if (FE::showDeployedPose) FE::showDeployedPose(*this);
        setColliders(true);
        EventOrThrow(*this, "TakeDown").active = true;
        Part* p = PartOf();
        if (p == nullptr) throw NullReferenceException();
        if (!joint && !p->fields.packed) SetJoint();
        p->fields.PermanentGroundContact = true;
        if (p->vessel != nullptr) p->vessel->permanentGroundContact = true;
    });
    st_placed->OnLeave = KFSMStateChange::Lambda(this, [this](KFSMState*) {
        EventOrThrow(*this, "TakeDown").active = false;
        if (joint) UnsetJoint();
        Part* p = PartOf();
        if (p == nullptr) throw NullReferenceException();
        p->fields.PermanentGroundContact = false;
        if (p->vessel != nullptr) p->vessel->permanentGroundContact = false;
    });
    fsm->AddState(st_placed);
    st_retracting = newState("Retracting");
    fsm->AddState(st_retracting);
    st_toppled = newState("Toppled");
    st_toppled->OnEnter = KFSMStateChange::Lambda(this, [this, setColliders](KFSMState*) {
        if (FE::showDeployedPose) FE::showDeployedPose(*this);
        EventOrThrow(*this, "PickUp").active = true;
        setColliders(true);
    });
    st_toppled->OnLeave = KFSMStateChange::Lambda(this, [this](KFSMState*) { EventOrThrow(*this, "PickUp").active = false; });
    fsm->AddState(st_toppled);
    auto timed = std::make_unique<KFSMTimedEvent>("Place Complete", FE::flagDeployAnimationLength);
    onPlaceComplete = timed.get();
    ownedEvents_.push_back(std::move(timed));
    onPlaceComplete->GoToStateOnEvent = st_placed;
    fsm->AddEvent(onPlaceComplete, {st_placing});
    ownedEvents_.push_back(std::make_unique<KFSMEvent>("Place Failed"));
    onPlacementFailed = ownedEvents_.back().get();
    onPlacementFailed->GoToStateOnEvent = st_toppled;
    onPlacementFailed->OnEvent = KFSMCallback::Lambda(this, [this] {
        if (fsm->TimeAtCurrentState() < static_cast<double>(Get<float>("deployFailRevertThreshold"))) PickUp();
    });
    fsm->AddEvent(onPlacementFailed, {st_placing});
    ownedEvents_.push_back(std::make_unique<KFSMEvent>("Toppled"));
    onTopple = ownedEvents_.back().get();
    onTopple->updateMode = KFSMUpdateMode::MANUAL_TRIGGER;
    onTopple->GoToStateOnEvent = st_toppled;
    fsm->AddEventExcluding(onTopple, {st_toppled});
    if (stateName) fsm->StartFSM(*stateName);
    else fsm->StartFSM(st_placing);
}

// FlagSite.cs:252
void FlagSite::MakeVisible() {
    if (FlagSiteEnvironment::setVisible) FlagSiteEnvironment::setVisible(*this, true);
}

// FlagSite.cs:261
void FlagSite::MakeInvisible() {
    if (FlagSiteEnvironment::setVisible) FlagSiteEnvironment::setVisible(*this, false);
}

// FlagSite.cs:271
void FlagSite::TakeDown() { PickUp(); }

// FlagSite.cs:277
void FlagSite::PickUp() {
    if (FlagSiteEnvironment::evaAddFlag && FlagSiteEnvironment::evaAddFlag()) {
        Part* p = PartOf();
        if (p == nullptr || p->vessel == nullptr) throw NullReferenceException();
        p->vessel->Die();
    }
}

// FlagSite.cs:287
void FlagSite::SetJoint() {
    joint = FlagSiteEnvironment::createJoint ? FlagSiteEnvironment::createJoint(*this) : true;
    jointBreakPending = true;  // StartCoroutine(CallbackUtil.DelayedCallback(unbreakablePeriodLength, ...))
}

// FlagSite.cs:287 SetJoint, the delayed callback body (:312-:319)
void FlagSite::ApplyJointBreakForce() {
    jointBreakPending = false;
    if (joint && FlagSiteEnvironment::setJointBreakForce) {
        FlagSiteEnvironment::setJointBreakForce(*this, Get<float>("breakForce"), Get<float>("breakForce"));
    }
}

// FlagSite.cs:357
void FlagSite::UnsetJoint() {
    if (FlagSiteEnvironment::destroyJoint) FlagSiteEnvironment::destroyJoint(*this);
    joint = false;
}

// FlagSite.cs:362
void FlagSite::OnVesselPack() {
    Part* p = PartOf();
    if (p == nullptr) throw NullReferenceException();
    if (p->vessel != nullptr) p->vessel->BackupVessel();
    if (joint) UnsetJoint();
}

// FlagSite.cs:374
void FlagSite::OnVesselUnpack() {
    if (!fsm) throw NullReferenceException();
    if (!joint && fsm->CurrentState() == st_placed) SetJoint();
}

// FlagSite.cs:382
void FlagSite::OnPlacementFail() {
    if (!fsm) throw NullReferenceException();
    fsm->RunEvent(onPlacementFailed);
}

// FlagSite.cs:387
void FlagSite::OnPlacementComplete() {
    if (FlagSiteEnvironment::setCollidersEnabled) FlagSiteEnvironment::setCollidersEnabled(*this, true);
    RenameSite([this] { events::GameEvents::afterFlagPlanted.Fire(this); });
}

// FlagSite.cs:400
void FlagSite::OnJointBreak(float breakForce) {
    Part* p = PartOf();
    if (p == nullptr) throw NullReferenceException();
    log::Info("[FlagSite]: Flag " + (p->vessel != nullptr ? p->vessel->vesselName : dotnet::FormatUInt64(p->fields.persistentId)) +
              " broke from ground. Force: " + dotnet::FormatSingleG(breakForce));
    if (!fsm) throw NullReferenceException();
    fsm->RunEvent(onTopple);
}

// FlagSite.cs:457
void FlagSite::RenameSite() { RenameSite(nullptr); }

// FlagSite.cs:462
void FlagSite::RenameSite(std::function<void()> afterDialog) {
    FlagSiteEnvironment::controlLocks["siteRenameDialog"] = "All";  // InputLockManager.SetControlLock(lockID) = ControlTypes.All
    siteName = "";
    newPlaqueText = PlaqueText();
    siteRenameDialogOpen = true;  // SiteRenameDialog = PopupDialog.SpawnPopupDialog(...)
    afterDialog_ = std::move(afterDialog);
    if (FlagSiteEnvironment::spawnSiteRenameDialog) FlagSiteEnvironment::spawnSiteRenameDialog(*this);
}

// FlagSite.cs:462 RenameSite, the accept button handler (:475: AcceptSiteRename(); afterDialog?.Invoke())
void FlagSite::SiteRenameAcceptPressed() {
    auto after = afterDialog_;
    AcceptSiteRename();
    if (after) after();
}

// FlagSite.cs:462 RenameSite, the cancel button handler (:482)
void FlagSite::SiteRenameDismissPressed() {
    auto after = afterDialog_;
    DismissSiteRename();
    if (after) after();
}

// FlagSite.cs:462 RenameSite, the accept button enabled-condition delegate (:475)
bool FlagSite::SiteRenameAcceptEnabled() const { return Vessel::IsValidVesselName(siteName); }

// FlagSite.cs:493
void FlagSite::AcceptSiteRename() {
    if (Vessel::IsValidVesselName(siteName)) {
        Part* p = PartOf();
        if (p == nullptr || p->vessel == nullptr) throw NullReferenceException();
        p->vessel->vesselName = siteName;
        Set<std::string>("PlaqueText", newPlaqueText);
        DismissSiteRename();
    }
}

// FlagSite.cs:503
void FlagSite::CancelSiteRename() {
    Part* p = PartOf();
    if (p == nullptr || p->vessel == nullptr) throw NullReferenceException();
    p->vessel->vesselName = Localizer::Format("#autoLoc_6002179");
    Set<std::string>("PlaqueText", "");
    DismissSiteRename();
}

// FlagSite.cs:510
void FlagSite::DismissSiteRename() {
    if (!siteRenameDialogOpen) throw NullReferenceException();  // SiteRenameDialog.Dismiss() on a null dialog
    if (FlagSiteEnvironment::dismissSiteRenameDialog) FlagSiteEnvironment::dismissSiteRenameDialog(*this);
    siteRenameDialogOpen = false;
    FlagSiteEnvironment::controlLocks.erase("siteRenameDialog");
}

// FlagSite.cs:518
void FlagSite::ReadPlaque() {
    FlagSiteEnvironment::controlLocks["FlagPlaqueDialog"] = "~(UI|CAMERACONTROLS)";
    Part* p = PartOf();
    if (p == nullptr || p->vessel == nullptr) throw NullReferenceException();
    flagPlaqueDialogOpen = true;
    if (FlagSiteEnvironment::spawnPlaqueDialog) FlagSiteEnvironment::spawnPlaqueDialog(*this, p->vessel->vesselName, PlaqueText());
}

// FlagSite.cs:524
void FlagSite::onFlagPlaqueDialogDismiss() {
    flagPlaqueDialogOpen = false;
    FlagSiteEnvironment::controlLocks.erase("FlagPlaqueDialog");
}

// ================================================================ ModulePartFirework

Part* ModulePartFirework::PartOf() const { return PartOfModule(*this); }

// ModulePartFirework.cs:167
void ModulePartFirework::OnAwake() {
    PartModule::OnAwake();
    const bool editor = ResourceFlowEnvironment::LoadedSceneIsEditor();
    const bool flight = ResourceFlowEnvironment::LoadedSceneIsFlight();
    auto control = [&](const char* field) {
        BaseField& f = FieldOrThrow(*this, field);
        return editor ? f.uiControlEditor : f.uiControlFlight;
    };
    auto isType = [](const std::shared_ptr<UI_Control>& c, const char* t) { return c != nullptr && c->Is(t); };
    if (editor || flight) {
        trailChooseOptionPresent = isType(control("trailType"), "UI_ChooseOption");
        burstChooseOptionPresent = isType(control("burstType"), "UI_ChooseOption");
        fireworkShotsFloatRangePresent = isType(control("fireworkShots"), "UI_FloatRange");
        tertiaryFieldPresent = isType(control("tertiaryBurstColorChanger"), "UI_ColorPicker");
        if (editor) FieldOrThrow(*this, "fireworkShotsDisplay").guiActiveEditor = false;
    }
    fireworkColors.assign(5, Color());
    FieldOrThrow(*this, "fireworkShots").OnValueModified.push_back([this](const ModuleValue&) {
        fieldModifiedLog.push_back("fireworkShots");
        updateFireworkShotLabel();
    });
    struct Name {
        const char* field;
        const char* tag;
        const char* arg;
    };
    static const Name names[] = {
        {"trailColor1R", "#autoLOC_6005129", "1"}, {"trailColor1G", "#autoLOC_6005130", "1"}, {"trailColor1B", "#autoLOC_6005131", "1"},
        {"trailColor2R", "#autoLOC_6005129", "2"}, {"trailColor2G", "#autoLOC_6005130", "2"}, {"trailColor2B", "#autoLOC_6005131", "2"},
        {"burstColor1R", "#autoLOC_6005126", "1"}, {"burstColor1G", "#autoLOC_6005127", "1"}, {"burstColor1B", "#autoLOC_6005128", "1"},
        {"burstColor2R", "#autoLOC_6005126", "2"}, {"burstColor2G", "#autoLOC_6005127", "2"}, {"burstColor2B", "#autoLOC_6005128", "2"},
        {"burstColor3R", "#autoLOC_6005126", "3"}, {"burstColor3G", "#autoLOC_6005127", "3"}, {"burstColor3B", "#autoLOC_6005128", "3"}};
    for (const Name& n : names) FieldOrThrow(*this, n.field).guiName = Localizer::Format(n.tag, {n.arg});
    for (const Name& n : names) {
        const std::string fieldName = n.field;
        FieldOrThrow(*this, n.field).OnValueModified.push_back([this, fieldName](const ModuleValue&) {
            fieldModifiedLog.push_back(fieldName);
            UpdateFireworksColors();
        });
    }
    FieldOrThrow(*this, "trailType").guiName = Localizer::Format("#autoLOC_6005105", {"#autoLOC_6005108"});
    FieldOrThrow(*this, "burstType").guiName = Localizer::Format("#autoLOC_6005105", {"#autoLOC_6005107"});
    if (!bound_) {
        bound_ = true;
        Events().Bind("LaunchShellEvent", BaseEvent::Handler([this] { LaunchShellEvent(); }));
        Actions().Bind("LaunchShellAction", [this](KSPActionParam& p) { LaunchShellAction(p); });
    }
    shellPrefabPresent = FireworkEnvironment::shellPrefabExists ? FireworkEnvironment::shellPrefabExists(Get<std::string>("shellName")) : true;
    if (!shellPrefabPresent) {
        EventOrThrow(*this, "LaunchShell").guiActive = false;  // sic: the event is named LaunchShellEvent, the lookup is null
        return;
    }
    availableFX.clear();
    availableFXPresent = true;
}

// ModulePartFirework.cs:230
void ModulePartFirework::OnSave(ConfigNode& node) {
    PartModule::OnSave(node);
    if (fireworkColors.size() < 5) throw NullReferenceException();
    const char* names[5][3] = {{"burstColor1R", "burstColor1G", "burstColor1B"}, {"burstColor2R", "burstColor2G", "burstColor2B"},
                               {"burstColor3R", "burstColor3G", "burstColor3B"}, {"trailColor1R", "trailColor1G", "trailColor1B"},
                               {"trailColor2R", "trailColor2G", "trailColor2B"}};
    for (int i = 0; i < 5; i++) {
        node.SetValue(names[i][0], fireworkColors[static_cast<size_t>(i)].r, true);
        node.SetValue(names[i][1], fireworkColors[static_cast<size_t>(i)].g, true);
        node.SetValue(names[i][2], fireworkColors[static_cast<size_t>(i)].b, true);
    }
}

// ModulePartFirework.cs:250
void ModulePartFirework::OnLoad(const ConfigNode& node) {
    PartModule::OnLoad(node);
    fireworkColors.assign(5, Color());
    fireworkColors[0] = Color(Get<float>("burstColor1R"), Get<float>("burstColor1G"), Get<float>("burstColor1B"));
    fireworkColors[1] = Color(Get<float>("burstColor2R"), Get<float>("burstColor2G"), Get<float>("burstColor2B"));
    fireworkColors[2] = Color(Get<float>("burstColor3R"), Get<float>("burstColor3G"), Get<float>("burstColor3B"));
    fireworkColors[3] = Color(Get<float>("trailColor1R"), Get<float>("trailColor1G"), Get<float>("trailColor1B"));
    fireworkColors[4] = Color(Get<float>("trailColor2R"), Get<float>("trailColor2G"), Get<float>("trailColor2B"));
}

// ModulePartFirework.cs:261
void ModulePartFirework::OnStart(StartState state) {
    std::vector<std::string> list, list2;
    PartModule::OnStart(state);
    const bool scene = ResourceFlowEnvironment::LoadedSceneIsEditor() || ResourceFlowEnvironment::LoadedSceneIsFlight();
    if (scene) {
        if (!availableFXPresent) throw NullReferenceException();  // availableFX.Add on a null list
        for (const FireworkFXDefinition& value : FireworkEnvironment::fxDefinitions) {
            availableFX.push_back(value);
            if (value.fwType == FireworkEffectType::TRAIL) list.push_back(value.displayName);
            else list2.push_back(value.displayName);
        }
    }
    if (!trailChooseOptionPresent || !burstChooseOptionPresent || !fireworkShotsFloatRangePresent) throw NullReferenceException();
    trailOptions = trailDisplay = list;
    burstOptions = burstDisplay = list2;
    fireworkShotsRangeMax = Get<float>("maxShots");
    if (Get<float>("fireworkShots") > Get<float>("maxShots")) Set<float>("fireworkShots", Get<float>("maxShots"));
    updateFireworkShotLabel();
    if (availableFXPresent && !availableFX.empty()) configureColorFields();
    if (scene) {
        if (fireworkColors.size() < 5) throw NullReferenceException();
        fireworkColors[0] = Color(Get<float>("burstColor1R"), Get<float>("burstColor1G"), Get<float>("burstColor1B"));
        fireworkColors[1] = Color(Get<float>("burstColor2R"), Get<float>("burstColor2G"), Get<float>("burstColor2B"));
        fireworkColors[2] = Color(Get<float>("burstColor3R"), Get<float>("burstColor3G"), Get<float>("burstColor3B"));
        fireworkColors[3] = Color(Get<float>("trailColor1R"), Get<float>("trailColor1G"), Get<float>("trailColor1B"));
        fireworkColors[4] = Color(Get<float>("trailColor2R"), Get<float>("trailColor2G"), Get<float>("trailColor2B"));
    }
    FieldOrThrow(*this, "burstType").OnValueModified.push_back([this](const ModuleValue&) {
        fieldModifiedLog.push_back("burstType");
        configureColorFields();
    });
    // GameEvents.onPartActionUIShown / onPartActionInitialized subscriptions: PAW-side (the host calls OnPAWInit)
}

// ModulePartFirework.cs:306
void ModulePartFirework::OnDestroy() {}

// ModulePartFirework.cs:312
std::vector<Color> ModulePartFirework::PresetColors() { return FireworkEnvironment::presetColors; }

// ModulePartFirework.cs:317
void ModulePartFirework::OnColorChanged(const Color& color, const std::string& pickerName) {
    if (pickerName.empty()) return;
    if (fireworkColors.size() < 5) throw NullReferenceException();
    auto apply = [&](size_t idx, const char* r, const char* g, const char* b) {
        fireworkColors[idx] = color;
        Set<float>(r, color.r);
        Set<float>(g, color.g);
        Set<float>(b, color.b);
        if (FireworkEnvironment::setPickerColor) FireworkEnvironment::setPickerColor(*this, pickerName, color);
    };
    if (pickerName == "secondaryTrailColorChanger") apply(4, "trailColor2R", "trailColor2G", "trailColor2B");
    else if (pickerName == "primaryTrailColorChanger") apply(3, "trailColor1R", "trailColor1G", "trailColor1B");
    else if (pickerName == "tertiaryBurstColorChanger") apply(2, "burstColor3R", "burstColor3G", "burstColor3B");
    else if (pickerName == "secondaryBurstColorChanger") apply(1, "burstColor2R", "burstColor2G", "burstColor2B");
    else if (pickerName == "primaryBurstColorChanger") apply(0, "burstColor1R", "burstColor1G", "burstColor1B");
}

// ModulePartFirework.cs:384
void ModulePartFirework::LaunchShellAction(KSPActionParam&) { LaunchShell(); }

// ModulePartFirework.cs:390
void ModulePartFirework::LaunchShellEvent() {
    LaunchShell();
    Part* p = PartOf();
    if (p == nullptr) throw NullReferenceException();
    int count = static_cast<int>(p->symmetryCounterparts.size());
    while (count-- > 0) {
        Part* other = p->symmetryCounterparts[static_cast<size_t>(count)];
        if (other != p) {
            PartModule* m = other->Modules().GetModule([](PartModule& pm) { return pm.IsA("ModulePartFirework"); });
            if (m == nullptr) throw NullReferenceException();
            static_cast<ModulePartFirework*>(m)->LaunchShell();
        }
    }
}

// ModulePartFirework.cs:411 (the persistentId comparison is the caller's)
void ModulePartFirework::OnPAWInit(Part*) {
    const Color y(1.0f, 0.6705883f, 1.0f / 17.0f, 1.0f);  // XKCDColors.YellowishOrange (XKCDColors.cs:2684)
    setAllColors(y, y, y, y, y);
}

// ModulePartFirework.cs:420
void ModulePartFirework::UpdateFireworksColors() {
    updatePickerColor(FireworkEffectType::TRAIL, 1);
    updatePickerColor(FireworkEffectType::TRAIL, 2);
    updatePickerColor(FireworkEffectType::BURST, 1);
    updatePickerColor(FireworkEffectType::BURST, 2);
    updatePickerColor(FireworkEffectType::BURST, 3);
}

// ModulePartFirework.cs:430 (a picker is "open" when the host reported its id in pickerIds)
void ModulePartFirework::updatePickerColor(FireworkEffectType type, int number) {
    auto picker = [&](const char* field) -> const std::string* {
        auto it = pickerIds.find(field);
        return it == pickerIds.end() ? nullptr : &it->second;
    };
    Color white = Color::white();
    if (type != FireworkEffectType::TRAIL) {
        if (number == 1 && picker("primaryBurstColorChanger")) {
            white.r = Get<float>("burstColor1R"), white.g = Get<float>("burstColor1G"), white.b = Get<float>("burstColor1B");
            OnColorChanged(white, *picker("primaryBurstColorChanger"));
        } else if (number == 2 && picker("secondaryBurstColorChanger")) {
            white.r = Get<float>("burstColor2R"), white.g = Get<float>("burstColor2G"), white.b = Get<float>("burstColor2B");
            OnColorChanged(white, *picker("secondaryBurstColorChanger"));
        } else if (number == 3 && picker("tertiaryBurstColorChanger")) {
            white.r = Get<float>("burstColor3R"), white.g = Get<float>("burstColor3G"), white.b = Get<float>("burstColor3B");
            OnColorChanged(white, *picker("tertiaryBurstColorChanger"));
        }
    } else if (number == 1 && picker("primaryTrailColorChanger")) {
        white.r = Get<float>("trailColor1R"), white.g = Get<float>("trailColor1G"), white.b = Get<float>("trailColor1B");
        OnColorChanged(white, *picker("primaryTrailColorChanger"));
    } else if (number == 2 && picker("secondaryTrailColorChanger")) {
        white.r = Get<float>("trailColor2R"), white.g = Get<float>("trailColor2G"), white.b = Get<float>("trailColor2B");
        OnColorChanged(white, *picker("secondaryTrailColorChanger"));
    }
}

// ModulePartFirework.cs:473
void ModulePartFirework::LaunchShell() {
    if (!shellPrefabPresent) return;
    if (!ResourceFlowEnvironment::InfinitePropellant) {
        if (!(Get<float>("fireworkShots") > 0.0f)) return;
        Set<float>("fireworkShots", f32store(fx(Get<float>("fireworkShots")) - fx(1.0f)));
        updateFireworkShotLabel();
    }
    Part* p = PartOf();
    if (p == nullptr) throw NullReferenceException();
    FireworkShellLaunch L;
    // base.gameObject.GetChild(cannonName).transform: a missing child is a NullReferenceException
    const std::optional<FireworkEnvironment::Cannon> cannon =
        FireworkEnvironment::cannon ? FireworkEnvironment::cannon(*this, Get<std::string>("cannonName"))
                                    : std::optional<FireworkEnvironment::Cannon>(FireworkEnvironment::Cannon{Vector3f::zero(), Vector3f::up()});
    if (!cannon) throw NullReferenceException();
    L.position = cannon->position;
    L.soundFX = FireworkEnvironment::activeVesselInAtmosphere && FireworkEnvironment::activeVesselInAtmosphere();
    if (L.soundFX && FireworkEnvironment::configureSoundFX) FireworkEnvironment::configureSoundFX(*this);
    const float shellMass = Get<float>("shellMass"), shellDrag = Get<float>("shellDrag"), scale = Get<float>("shellRBMassScaleValue");
    const float shellVelocity = Get<float>("shellVelocity");
    L.maxDistance = 10000.0f;
    L.origDrag = shellDrag;
    L.rbMass = f32store(fx(shellMass) * fx(scale));
    L.rbDrag = shellDrag;
    shellForceDir = cannon->up;
    if (Get<bool>("variationOnShellDirection")) {
        shellForceDir = (shellForceDir + UnityRandom::Global().onUnitSphere() * Get<float>("variationOnShellDirMultiplier")).normalized();
    }
    L.shellForceDir = shellForceDir;
    const Vector3f force = shellForceDir * shellMass * (shellVelocity / Time::fixedDeltaTime) * scale;
    L.force = force;
    const double frameSpeed = FireworkEnvironment::krakensbaneFrameSpeed ? FireworkEnvironment::krakensbaneFrameSpeed() : 0.0;
    if (frameSpeed <= 0.0) {
        const Vector3d vel = FireworkEnvironment::vesselRbVelocityD ? FireworkEnvironment::vesselRbVelocityD(*this) : Vector3d::zero();
        L.vesselVelocityForce =
            (vel.normalized() * static_cast<double>(shellMass) * (vel.magnitude() / static_cast<double>(Time::fixedDeltaTime)) * static_cast<double>(scale))
                .ToVector3f();
    }
    L.recoil = force.normalized() * shellMass * shellVelocity * scale * -1.0f;
    L.shellDuration = Get<float>("shellDuration");
    L.shellVelocity = shellVelocity;
    L.burstSpread = Get<float>("burstSpread");
    L.burstDuration = Get<float>("burstDuration");
    L.burstFlareSize = Get<float>("burstFlareSize");
    L.trailPrefab = getCurrentPSByType(FireworkEffectType::TRAIL);
    L.burstPrefab = getCurrentPSByType(FireworkEffectType::BURST);
    if (fireworkColors.size() < 5) throw NullReferenceException();
    L.burstColor1 = fireworkColors[0];
    L.burstColor2 = fireworkColors[1];
    L.trailColor1 = fireworkColors[3];
    L.burstColor3 = fireworkColors[2];
    L.trailColor2 = fireworkColors[4];
    const FireworkFXDefinition* burst = getCurrentFXByType(FireworkEffectType::BURST);
    const FireworkFXDefinition* trail = getCurrentFXByType(FireworkEffectType::TRAIL);
    if (burst == nullptr || trail == nullptr) throw NullReferenceException();
    L.crackleSFX = burst->crackleSFX;
    L.randomizeBurstOrientation = burst->randomizeBurstOrientation;
    L.minTrailLifetime = trail->minTrailLifetime;
    L.maxTrailLifetime = trail->maxTrailLifetime;
    if (FireworkEnvironment::launchShell) FireworkEnvironment::launchShell(*this, L);
}

// ModulePartFirework.cs:559
std::string ModulePartFirework::getCurrentPSByType(FireworkEffectType type) {
    const FireworkFXDefinition* d = getCurrentFXByType(type);
    return d != nullptr ? d->prefabName : std::string();
}

// ModulePartFirework.cs:589 (the last matching definition wins: the loop counts down)
const FireworkFXDefinition* ModulePartFirework::getCurrentFXByType(FireworkEffectType type) {
    if (!availableFXPresent) throw NullReferenceException();
    const bool burst = type == FireworkEffectType::BURST;
    const std::vector<std::string>& options = burst ? burstOptions : trailOptions;
    const int index = burst ? Get<int32_t>("burstType") : Get<int32_t>("trailType");
    int count = static_cast<int>(availableFX.size());
    while (count-- > 0) {
        const FireworkFXDefinition& d = availableFX[static_cast<size_t>(count)];
        if (d.fwType == (burst ? FireworkEffectType::BURST : FireworkEffectType::TRAIL)) {
            if (index < 0 || static_cast<size_t>(index) >= options.size()) throw std::out_of_range("Index was outside the bounds of the array.");
            if (d.displayName == options[static_cast<size_t>(index)]) return &d;
        }
    }
    return nullptr;
}

// ModulePartFirework.cs:624
void ModulePartFirework::updateFireworkShotLabel() { Set<std::string>("fireworkShotsDisplay", dotnet::FormatSingleF(Get<float>("fireworkShots"), 0)); }

// ModulePartFirework.cs:639
void ModulePartFirework::configureColorFields() {
    const FireworkFXDefinition* d = getCurrentFXByType(FireworkEffectType::BURST);
    if (d == nullptr) throw NullReferenceException();
    FieldOrThrow(*this, "primaryBurstColorChanger").guiName = d->color1Name;
    FieldOrThrow(*this, "secondaryBurstColorChanger").guiName = d->color2Name;
    if (d->color3Name != "none") {
        if (!tertiaryFieldPresent) throw NullReferenceException();
        if (FireworkEnvironment::setTertiaryColorVisible) FireworkEnvironment::setTertiaryColorVisible(*this, true);
        FieldOrThrow(*this, "tertiaryBurstColorChanger").guiName = d->color3Name;
    } else {
        if (!tertiaryFieldPresent) throw NullReferenceException();
        if (FireworkEnvironment::setTertiaryColorVisible) FireworkEnvironment::setTertiaryColorVisible(*this, false);
    }
    // StartCoroutine("waitAndRefreshColors") / displayDirty: PAW-side
}

// ModulePartFirework.cs:711
void ModulePartFirework::setAllColors(const Color& colB1, const Color& colB2, const Color& colB3, const Color& colT1, const Color& colT2) {
    auto set = [&](const char* picker, const Color& c, const char* r, const char* g, const char* b) {
        if (pickerIds.find(picker) == pickerIds.end()) return;
        if (FireworkEnvironment::setPickerColor) FireworkEnvironment::setPickerColor(*this, pickerIds[picker], c);
        Set<float>(r, c.r);
        Set<float>(g, c.g);
        Set<float>(b, c.b);
    };
    set("primaryBurstColorChanger", colB1, "burstColor1R", "burstColor1G", "burstColor1B");
    set("secondaryBurstColorChanger", colB2, "burstColor2R", "burstColor2G", "burstColor2B");
    set("tertiaryBurstColorChanger", colB3, "burstColor3R", "burstColor3G", "burstColor3B");
    set("primaryTrailColorChanger", colT1, "trailColor1R", "trailColor1G", "trailColor1B");
    set("secondaryTrailColorChanger", colT2, "trailColor2R", "trailColor2G", "trailColor2B");
}

// ModulePartFirework.cs:750
Color ModulePartFirework::GetCurrentColor(const std::string& fieldName) {
    if (fireworkColors.size() < 5) throw NullReferenceException();
    if (fieldName == "secondaryTrailColorChanger") return fireworkColors[4];
    if (fieldName == "primaryTrailColorChanger") return fireworkColors[3];
    if (fieldName == "tertiaryBurstColorChanger") return fireworkColors[2];
    if (fieldName == "secondaryBurstColorChanger") return fireworkColors[1];
    if (fieldName == "primaryBurstColorChanger") return fireworkColors[0];
    return Color::white();  // PartModule.GetCurrentColor(fieldName) (PartModule.cs: Color.white)
}

// ModulePartFirework.cs:769
float ModulePartFirework::GetModuleMass(float defaultMass, ModifierStagingSituation) {
    return f32store(fx(defaultMass) + fx(Get<float>("fireworkShots")) * fx(Get<float>("shellMass")) * fx(0.001f));
}

// ================================================================ registration

namespace detail {
void RegisterMiscModules() {
    ModuleFactory::Register("ModuleGPS", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleGPS>(c)); });
    ModuleFactory::Register("FlagSite", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<FlagSite>(c)); });
    ModuleFactory::Register("ModulePartFirework",
                            [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModulePartFirework>(c)); });
}
}  // namespace detail

}  // namespace ksp
