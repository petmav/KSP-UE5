// WP-35 science part modules (see ksp/modules/science_modules.hpp for the scope and the seams).
#include "ksp/modules/science_modules.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

#include "ksp/celestial/flight_globals.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/kerbals/experience.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/modules/base_event.hpp"
#include "ksp/modules/base_field.hpp"
#include "ksp/modules/module_resource.hpp"
#include "ksp/orbit/orbit.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/parts/available_part.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/resources/resource_flow_host.hpp"
#include "ksp/save/proto_vessel.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/util/ksp_util.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

using GE = events::GameEvents;
using Loc = KSP::Localization::Localizer;

namespace {

std::string Fmt(const char* tag, std::vector<std::string> args) { return Loc::Format(tag, args); }

void PostMsg(const std::string& text) { ScienceEnvironment::PostScreenMessage(text); }

// a string KSPField that is null after AddComponent reads as ""
std::string StrField(PartModule& m, const char* name) { return m.IsNull(name) ? std::string() : m.Get<std::string>(name); }

const PartResourceDefinition* LookupDefinition(const std::string& name) {
    const PartResourceLibrary* lib = PartResourceLibrary::Instance();
    return lib ? lib->GetDefinition(std::string_view(name)) : nullptr;
}

bool PartActionUIContains(const Part* p) {
    if (ScienceModuleEnvironment::partActionUIContains) return ScienceModuleEnvironment::partActionUIContains(p);
    if (ResourceFlowEnvironment::partActionUIContains) return ResourceFlowEnvironment::partActionUIContains(p);
    return false;
}

std::string F32(float v) { return dotnet::FormatSingleG(v); }

template <class T>
T* FindModule(Part& p, const char* cls) {
    return dynamic_cast<T*>(p.FindModuleOfClass(cls));
}

}  // namespace

void ScienceModuleEnvironment::Reset() {
    evaAction = nullptr;
    kerbalEvaHasMethod = nullptr;
    inventoryContainsPart = nullptr;
    partActionUIContains = nullptr;
    geeForceAtPosition = nullptr;
    experimentTransfers.clear();
}

// ----------------------------------------------------------------------------------------------------------------------------------
// ScienceLabSearch

// ScienceLabSearch.cs:42
std::string ScienceLabSearch::ErrorString() const {
    switch (Error) {
    default: return Loc::Format("#autoLOC_239591");
    case SearchError::MIXED: return Loc::Format("#autoLOC_239579");
    case SearchError::NO_LABS: return Loc::Format("#autoLOC_239581");
    case SearchError::NO_MANNED: return !HasMultipleLabs() ? Loc::Format("#autoLOC_6001034") : Loc::Format("#autoLOC_6001033");
    case SearchError::NO_SPACE: return !HasMultipleLabs() ? Loc::Format("#autoLOC_6001036") : Loc::Format("#autoLOC_6001035");
    case SearchError::ALL_RESEARCHED: return !HasMultipleLabs() ? Loc::Format("#autoLOC_6001038") : Loc::Format("#autoLOC_6001037");
    case SearchError::ALL_PROCESSING: return !HasMultipleLabs() ? Loc::Format("#autoLOC_6001032") : Loc::Format("#autoLOC_6001031");
    }
}

// ScienceLabSearch.cs:82
double ScienceLabSearch::ScienceExpectation() const {
    if (NextLabForData != nullptr && NextLabForData->Converter() != nullptr) return NextLabForData->Converter()->CalculateScienceAmount(Data->labValue);
    return 0.0;
}

// ScienceLabSearch.cs:94
double ScienceLabSearch::TimeExpectation() const {
    if (NextLabForData != nullptr && NextLabForData->Converter() != nullptr) return NextLabForData->Converter()->CalculateResearchTime(Data->labValue);
    return 0.0;
}

// ScienceLabSearch.cs:106
std::string ScienceLabSearch::DataExpectationSummary() const {
    if (NextLabForData != nullptr && NextLabForData->Converter() != nullptr) return NextLabForData->Converter()->DataExpectationSummary(Data->labValue);
    return std::string();
}

// ScienceLabSearch.cs:118
ScienceLabSearch::ScienceLabSearch(Vessel* vessel_, const std::shared_ptr<ScienceData>& data) {
    vessel = vessel_;
    Data = data;
    if (vessel_ != nullptr && data != nullptr && vessel_->loaded) {
        ModuleScienceLab* moduleScienceLab = nullptr;
        int count = static_cast<int>(vessel_->Parts().size());
        while (count-- > 0) {
            Part* part = vessel_->Parts()[static_cast<size_t>(count)];
            int count2 = static_cast<int>(part->Modules().Count());
            while (count2-- > 0) {
                auto* lab = dynamic_cast<ModuleScienceLab*>(part->Modules()[static_cast<size_t>(count2)]);
                if (lab != nullptr) {
                    LabsTotal++;
                    const bool num = lab->IsOperational();
                    const bool flag = fx(data->labValue) + fx(lab->Get<float>("dataStored")) <= fx(lab->Get<float>("dataStorage"));
                    const bool flag2 = std::find(lab->ExperimentData.begin(), lab->ExperimentData.end(), data->subjectID) == lab->ExperimentData.end();
                    const bool flag3 = !lab->processingData;
                    if (num) LabsManned++;
                    if (flag) LabsWithSpace++;
                    if (flag2) LabsNotResearched++;
                    if (flag3) LabsNotProcessing++;
                    if (num && flag && flag2 && flag3) moduleScienceLab = lab;
                }
            }
        }
        if (moduleScienceLab != nullptr) NextLabForData = moduleScienceLab;
        else if (LabsTotal <= 0) Error = SearchError::NO_LABS;
        else if (LabsNotProcessing <= 0) Error = SearchError::ALL_PROCESSING;
        else if (LabsManned <= 0) Error = SearchError::NO_MANNED;
        else if (LabsNotResearched <= 0) Error = SearchError::ALL_RESEARCHED;
        else if (LabsWithSpace <= 0) Error = SearchError::NO_SPACE;
        else Error = SearchError::MIXED;
    } else {
        Error = SearchError::BUG;
        log::Error("A science lab data search has failed due to invalid arguments.");
    }
}

// ScienceLabSearch.cs:199
void ScienceLabSearch::PostErrorToScreen() const {
    std::string e = ErrorString();
    std::transform(e.begin(), e.end(), e.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    PostMsg(Fmt("#autoLOC_239631", {e}));
}

// ----------------------------------------------------------------------------------------------------------------------------------
// ExperimentTransfer

// ExperimentTransfer.cs:94
bool ExperimentTransfer::Create(Part& srcPart, IScienceDataContainer& cont) {
    if (ScienceEnvironment::resultDialogOpen && ScienceEnvironment::resultDialogOpen()) {
        PostMsg(Loc::Format("#autoLOC_133305"));
        return false;
    }
    ScienceModuleEnvironment::experimentTransfers.emplace_back(&srcPart, &cont);
    return true;
}

// ExperimentTransfer.cs:107
bool ExperimentTransfer::IsSemiValidPart(Part&) { return false; }

// ExperimentTransfer.cs:112
bool ExperimentTransfer::IsValidPart(Part& p) {
    auto* moduleScienceContainer = FindModule<ModuleScienceContainer>(p, "ModuleScienceContainer");
    if (moduleScienceContainer != nullptr) return moduleScienceContainer->Get<bool>("canBeTransferredToInVessel");
    return false;
}

// ----------------------------------------------------------------------------------------------------------------------------------
// ModuleScienceExperiment

Part* ModuleScienceExperiment::PartOf() const { return dynamic_cast<Part*>(part); }
Vessel* ModuleScienceExperiment::VesselOf() const {
    Part* p = PartOf();
    return p ? p->vessel : nullptr;
}

ModuleScienceExperiment::~ModuleScienceExperiment() {
    for (auto& c : running) ScienceEnvironment::Scheduler().StopCoroutine(c);
    // OnDestroy (:293)
    if (subscribed_) {
        GE::onVesselSituationChange.Remove(this, &ModuleScienceExperiment::OnVesselSituationChange);
        GE::onVesselStandardModification.Remove(this, &ModuleScienceExperiment::OnVesselModified);
        GE::onModuleInventoryChanged.Remove(this, &ModuleScienceExperiment::OnModuleInventoryChanged);
    }
}

// ModuleScienceExperiment.cs:35
void ModuleScienceExperiment::EVASituation::Load(const ConfigNode& node) {
    ResultString.clear();
    node.TryGetValue("priority", priority);
    node.TryGetValue("situationMask", situationMask);
    node.TryGetValue("requiresAtmosphere", RequiresAtmosphere);
    node.TryGetValue("requiresNoAtmosphere", RequiresNoAtmosphere);
    node.TryGetValue("minTemp", MinTemp);
    node.TryGetValue("maxTemp", MaxTemp);
    node.TryGetValue("kerbalAction", KerbalAction);
    ResultString = node.values().GetValues("resultString");
    node.TryGetValue("dialogDelay", dialogDelay);
}

void ModuleScienceExperiment::Start_(Enumerator e) { running.push_back(ScienceEnvironment::Scheduler().StartCoroutine(std::move(e))); }

// ModuleScienceExperiment.cs:204
void ModuleScienceExperiment::OnAwake() {
    PartModule::OnAwake();
    Events().Bind("DeployExperiment", BaseEvent::Handler([this] { DeployExperiment(); }));
    Events().Bind("CollectDataExternalEvent", BaseEvent::Handler([this] { CollectDataExternalEvent(); }));
    Events().Bind("ReviewDataEvent", BaseEvent::Handler([this] { ReviewDataEvent(); }));
    Events().Bind("ResetExperiment", BaseEvent::Handler([this] { ResetExperiment(); }));
    Events().Bind("DeployExperimentExternal", BaseEvent::Handler([this] { DeployExperimentExternal(); }));
    Events().Bind("ResetExperimentExternal", BaseEvent::Handler([this] { ResetExperimentExternal(); }));
    Events().Bind("CleanUpExperimentExternal", BaseEvent::Handler([this] { CleanUpExperimentExternal(); }));
    Events().Bind("TransferDataEvent", BaseEvent::Handler([this] { TransferDataEvent(); }));
    Actions().Bind("DeployAction", [this](KSPActionParam& p) { DeployAction(p); });
    Actions().Bind("ResetAction", [this](KSPActionParam& p) { ResetAction(p); });
    if (!subscribed_) {
        GE::onVesselSituationChange.Add(this, &ModuleScienceExperiment::OnVesselSituationChange);
        GE::onVesselStandardModification.Add(this, &ModuleScienceExperiment::OnVesselModified);
        subscribed_ = true;
    }
    // onGamePause / onGameUnpause: the dialogs' visibility (UI); onCommandSeatInteraction: OnCommandSeatInteraction is called by the EVA host
}

// ModuleScienceExperiment.cs:213
void ModuleScienceExperiment::OnLoad(const ConfigNode& node) {
    if (node.HasValue("FxModules")) {
        fxModuleIndices = KSPUtil::ParseArray<int>(*node.GetValue("FxModules"), [](const std::string& s) { return dotnet::ParseInt32(s); });
    }
    if (node.HasNode("ScienceData")) experimentData = std::make_shared<ScienceData>(*node.GetNode("ScienceData"));
    if (!node.HasNode("EVASITUATION")) return;
    evaSituations.emplace();
    for (ConfigNode* n : node.GetNodes("EVASITUATION")) {
        auto eVASituation = std::make_shared<EVASituation>();
        eVASituation->Load(*n);
        const bool known = ScienceModuleEnvironment::kerbalEvaHasMethod
                               ? ScienceModuleEnvironment::kerbalEvaHasMethod(eVASituation->KerbalAction)
                               : (eVASituation->KerbalAction == "PlayGolf" || eVASituation->KerbalAction == "Banana" ||
                                  eVASituation->KerbalAction == "Dzhanibekov");
        if (!eVASituation->KerbalAction.empty() && known) evaSituations->push_back(eVASituation);
    }
}

// ModuleScienceExperiment.cs:240
void ModuleScienceExperiment::OnSave(ConfigNode& node) {
    if (experimentData != nullptr) experimentData->Save(*node.AddNode("ScienceData"));
}

// ModuleScienceExperiment.cs:248
void ModuleScienceExperiment::OnStart(StartState state) {
    Part* p = PartOf();
    if (Get<bool>("requiresInventoryPart")) {
        inventory = p ? p->FindModuleOfClass("ModuleInventoryPart") : nullptr;
        GE::onModuleInventoryChanged.Add(this, &ModuleScienceExperiment::OnModuleInventoryChanged);
        subscribed_ = true;
    }
    useCooldown = Get<double>("cooldownTimer") > 0.0;
    Fields()["cooldownString"]->guiActive = useCooldown;
    if (!DeployEventDisabled) experiment = ResearchAndDevelopment::GetExperiment(StrField(*this, "experimentID"));
    FindContainer();
    updateModuleUI();
    if (p != nullptr && Get<bool>("useStaging") && p->fields.stagingIcon && p->fields.stagingIcon->empty() && Get<bool>("overrideStagingIconIfBlank"))
        p->fields.stagingIcon = std::string("SCIENCE_GENERIC");
    if (!fxModuleIndices.has_value()) return;
    fxModules.clear();
    fxModulesCreated = true;
    for (int num2 : *fxModuleIndices) {
        IScalarModule* scalar = p ? p->Modules().GetModule(static_cast<size_t>(num2))->AsScalarModule() : nullptr;
        if (scalar != nullptr) {
            if (Get<bool>("hideFxModuleUI")) {
                scalar->SetUIWrite(false);
                scalar->SetUIRead(false);
            }
            fxModules.push_back(scalar);
        } else {
            log::Error("[ModuleScienceExperiment]: Part Module " + std::to_string(num2) + " doesn't implement IScalarModule");
        }
    }
    (void)state;
}

// ModuleScienceExperiment.cs:303
void ModuleScienceExperiment::Update() {
    if (useCooldown && ScienceEnvironment::loadedSceneIsFlight) {
        double num = static_cast<double>(TimeWarp::deltaTime());
        if (Get<double>("cooldownToGo") > num) {
            Set<double>("cooldownToGo", Get<double>("cooldownToGo") - num);
            Set<std::string>("cooldownString", KSPUtil::PrintTimeCompact(Get<double>("cooldownToGo"), false));
        } else {
            Set<double>("cooldownToGo", 0.0);
            Set<std::string>("cooldownString", "Ready");
        }
    }
    if (containersDirty) {
        FindContainer();
        updateModuleUI();
    }
}

// ModuleScienceExperiment.cs:326
void ModuleScienceExperiment::OnInventoryModeDisable() { GE::onModuleInventoryChanged.Remove(this, &ModuleScienceExperiment::OnModuleInventoryChanged); }

// ModuleScienceExperiment.cs:331
void ModuleScienceExperiment::OnInventoryModeEnable() { containersDirty = true; }

// ModuleScienceExperiment.cs:336
void ModuleScienceExperiment::OnVesselSituationChange(GE::HostedFromToAction<Vessel*, Vessel_Situations> vcs) {
    if (vcs.host == VesselOf()) {
        if (Get<bool>("requiresInventoryPart") && inventory == nullptr) {
            Part* p = PartOf();
            inventory = p ? p->FindModuleOfClass("ModuleInventoryPart") : nullptr;
        }
        updateModuleUI();
    }
}

// ModuleScienceExperiment.cs:348
void ModuleScienceExperiment::OnModuleInventoryChanged(ModuleInventoryPart* inventoryPart) {
    if (Get<bool>("requiresInventoryPart") && inventory == nullptr) {
        Part* p = PartOf();
        inventory = p ? p->FindModuleOfClass("ModuleInventoryPart") : nullptr;
    }
    // sic approximation: the C# compares inventory.part.persistentId with inventoryPart.part.persistentId; the inventory module class belongs
    // to the inventory package, so any change of an inventory refreshes the UI here
    if (inventory != nullptr && inventoryPart != nullptr) updateModuleUI();
}

// ModuleScienceExperiment.cs:360
void ModuleScienceExperiment::OnCommandSeatInteraction(Part* evaPart, bool entered) {
    if (!(evaPart == nullptr) && !(evaPart != PartOf())) {
        kerbalInSeat = entered;
        updateModuleUI();
    }
}

// ModuleScienceExperiment.cs:393
void ModuleScienceExperiment::updateModuleUI() {
    Vessel* vessel = VesselOf();
    auto ev = [this](const char* n) { return Events()[n]; };
    auto act = [this](const char* n) { return Actions()[n]; };
    const bool useActionGroups = Get<bool>("useActionGroups");
    const std::string experimentActionName = StrField(*this, "experimentActionName");
    const std::string resetActionName = StrField(*this, "resetActionName");
    const bool Deployed = Get<bool>("Deployed"), Inoperable = Get<bool>("Inoperable"), rerunnable = Get<bool>("rerunnable");
    if (vessel != nullptr) {
        bool avail = experiment != nullptr && experiment->IsUnlocked();
        if (avail && Get<bool>("hideUIwhenUnavailable")) {
            Vessel* ladder = vessel->EVALadderVessel();
            avail = experiment->IsAvailableWhile(ScienceUtil::GetExperimentSituation(*ladder), *vessel->mainBody());
        }
        if (avail && Get<bool>("requiresInventoryPart")) {
            avail = inventory != nullptr && ScienceModuleEnvironment::inventoryContainsPart &&
                    ScienceModuleEnvironment::inventoryContainsPart(*inventory, StrField(*this, "requiredInventoryPart"));
        }
        avail = avail && (Get<bool>("deployableSeated") || !kerbalInSeat) && resultsDialog == nullptr;
        if (avail) {
            ev("DeployExperiment")->active = (!Deployed || rerunnable) && !DeployEventDisabled && Get<int>("usageReqMaskInternal") != -1;
            ev("DeployExperiment")->SetGuiName(experimentActionName);
            act("DeployAction")->active = useActionGroups && !DeployEventDisabled;
            act("DeployAction")->SetGuiName(experimentActionName);
            ev("DeployExperimentExternal")->active = (!Deployed || rerunnable) && !DeployEventDisabled && Get<int>("usageReqMaskExternal") != -1;
            ev("DeployExperimentExternal")->SetGuiName(experimentActionName);
            ev("DeployExperimentExternal")->unfocusedRange = Get<float>("interactionRange");
        } else {
            ev("DeployExperiment")->active = false;
            act("DeployAction")->active = false;
            act("DeployAction")->SetGuiName(experimentActionName);
            ev("DeployExperimentExternal")->active = false;
        }
        ev("ResetExperiment")->active = Deployed && !Inoperable && Get<bool>("resettable") && !rerunnable && resultsDialog == nullptr;
        ev("ResetExperiment")->SetGuiName(resetActionName);
        ev("ResetExperimentExternal")->active = Deployed && !Inoperable && Get<bool>("resettableOnEVA") && !rerunnable && resultsDialog == nullptr;
        ev("ResetExperimentExternal")->SetGuiName(resetActionName);
        ev("ResetExperimentExternal")->unfocusedRange = Get<float>("interactionRange");
        act("ResetAction")->active = useActionGroups && !Inoperable && resultsDialog == nullptr;
        act("ResetAction")->SetGuiName(resetActionName);
        ev("ReviewDataEvent")->active = Deployed && experimentData != nullptr && resultsDialog == nullptr;
        ev("ReviewDataEvent")->SetGuiName(StrField(*this, "reviewActionName"));
        ev("CollectDataExternalEvent")->SetGuiName(Loc::Format(StrField(*this, "collectActionName")));
        const bool flag = Get<bool>("dataIsCollectable") && experimentData != nullptr;
        ev("CollectDataExternalEvent")->active = flag;
        ev("TransferDataEvent")->active = hasContainer && flag;
        ev("CollectDataExternalEvent")->unfocusedRange = Get<float>("interactionRange");
        ev("CleanUpExperimentExternal")->active = Deployed && Inoperable && Get<bool>("resettableOnEVA");
        ev("CleanUpExperimentExternal")->unfocusedRange = Get<float>("interactionRange");
    } else {
        act("DeployAction")->active = useActionGroups && !DeployEventDisabled;
        act("DeployAction")->SetGuiName(experimentActionName);
        act("ResetAction")->active = useActionGroups;
        act("ResetAction")->SetGuiName(resetActionName);
    }
}

// ModuleScienceExperiment.cs:440
bool ModuleScienceExperiment::ValidEVASituation() {
    validEvaSituation = nullptr;
    Vessel* vessel = VesselOf();
    Vessel* ladder = vessel->EVALadderVessel();
    ExperimentSituations experimentSituation = ScienceUtil::GetExperimentSituation(*ladder);
    if (!evaSituations.has_value()) throw NullReferenceException("ModuleScienceExperiment.evaSituations");
    for (auto& eVASituation : *evaSituations) {
        bool flag = true;
        if ((eVASituation->situationMask & static_cast<uint32_t>(experimentSituation)) == 0) flag = false;
        if (eVASituation->RequiresAtmosphere && ladder->atmDensity <= 0.0) flag = false;
        if (eVASituation->RequiresNoAtmosphere && ladder->atmDensity > 0.0) flag = false;
        if (eVASituation->MinTemp > -std::numeric_limits<float>::max() && vessel != nullptr && vessel->mainBody() != nullptr &&
            static_cast<float>(vessel->mainBody()->bodyTemperature) < eVASituation->MinTemp)
            flag = false;
        if (eVASituation->MaxTemp < std::numeric_limits<float>::max() && vessel != nullptr && vessel->mainBody() != nullptr &&
            static_cast<float>(vessel->mainBody()->bodyTemperature) > eVASituation->MaxTemp)
            flag = false;
        if (vessel != nullptr && vessel->isEVA() && vessel->evaController != nullptr && vessel->evaController->OnALadder()) flag = false;
        // vessel.evaController.InConstructionMode: KerbalEVA construction mode belongs to WP-32 (never set here)
        if (flag && (validEvaSituation == nullptr || (validEvaSituation != nullptr && eVASituation->priority < validEvaSituation->priority)))
            validEvaSituation = eVASituation;
    }
    return validEvaSituation != nullptr;
}

// ModuleScienceExperiment.cs:489
void ModuleScienceExperiment::DeployExperiment() {
    Part* p = PartOf();
    if (p != nullptr && p->vessel != nullptr) {
        if (experiment == nullptr) {
            log::Error("[ModuleScienceExperiment]: Cannot deploy experiment '" + StrField(*this, "experimentID") + "'. ScienceExperiment reference is Null.");
        } else if (!Get<bool>("availableShielded") && p->ShieldedFromAirstream()) {
            PostMsg(Loc::Format("#autoLOC_238290"));
        } else {
            std::string msg = StrField(*this, "usageReqMessage");
            const bool ok = ScienceUtil::RequiredUsageInternalAvailable(*p->vessel, *p, static_cast<ExperimentUsageReqs>(Get<int>("usageReqMaskInternal")), *experiment, msg);
            Set<std::string>("usageReqMessage", msg);
            if (ok) {
                if (useCooldown && Get<double>("cooldownTimer") > 0.0) {
                    PostMsg(Fmt("#autoLOC_238298", {KSPUtil::PrintTimeCompact(Get<double>("cooldownToGo"), false)}));
                } else {
                    Events()["DeployExperiment"]->active = false;
                    Start_(gatherData(true));
                }
            } else {
                PostMsg("<b><color=orange>" + msg + "</color></b>");
            }
        }
    } else {
        log::Error("[ModuleScienceExperiment]: Cannot deploy experiment '" + StrField(*this, "experimentID") + "'. Part/Vessel reference is Null.");
    }
}

// ModuleScienceExperiment.cs:525
void ModuleScienceExperiment::DeployAction(KSPActionParam&) {
    Part* p = PartOf();
    if (p != nullptr && p->vessel != nullptr) {
        if (experiment == nullptr) {
            log::Error("[ModuleScienceExperiment]: Cannot deploy experiment '" + StrField(*this, "experimentID") + "'. ScienceExperiment reference is Null.");
        } else if (!Get<bool>("availableShielded") && p->ShieldedFromAirstream()) {
            PostMsg(Loc::Format("#autoLOC_238318"));
        } else {
            std::string msg = StrField(*this, "usageReqMessage");
            const bool ok = ScienceUtil::RequiredUsageInternalAvailable(*p->vessel, *p, static_cast<ExperimentUsageReqs>(Get<int>("usageReqMaskInternal")), *experiment, msg);
            Set<std::string>("usageReqMessage", msg);
            if (ok) {
                if (!Get<bool>("Deployed")) {
                    if (useCooldown && Get<double>("cooldownTimer") > 0.0)
                        PostMsg(Fmt("#autoLOC_238298", {KSPUtil::PrintTimeCompact(Get<double>("cooldownToGo"), false)}));
                    else
                        Start_(gatherData(true));
                }
            } else {
                PostMsg("<b><color=orange>" + msg + "</color></b>");
            }
        }
    } else {
        log::Error("[ModuleScienceExperiment]: Cannot deploy experiment '" + StrField(*this, "experimentID") + "'. Part/Vessel reference is Null.");
    }
}

// ModuleScienceExperiment.cs:562
void ModuleScienceExperiment::OnActive() {
    if (!Get<bool>("useStaging") || !Get<bool>("stagingEnabled")) return;
    Part* p = PartOf();
    if (experiment == nullptr) {
        log::Error("[ModuleScienceExperiment]: Cannot deploy experiment '" + StrField(*this, "experimentID") + "'. ScienceExperiment reference is Null.");
    } else {
        std::string msg = StrField(*this, "usageReqMessage");
        const bool ok = ScienceUtil::RequiredUsageInternalAvailable(*p->vessel, *p, static_cast<ExperimentUsageReqs>(Get<int>("usageReqMaskInternal")), *experiment, msg);
        Set<std::string>("usageReqMessage", msg);
        if (ok) {
            if (!Get<bool>("Deployed")) Start_(gatherData(false));
            else lastIconColor = "cyan";  // part.stackIcon.SetIconColor(XKCDColors.Cyan)
        } else {
            PostMsg("<b><color=orange>" + msg + "</color></b>");
        }
    }
}

// ModuleScienceExperiment.cs:589
Enumerator ModuleScienceExperiment::gatherData(bool showDialog) {
    Part* p = PartOf();
    Vessel* vessel = p->vessel;
    if (Get<bool>("requiresInventoryPart")) {
        if (!(inventory != nullptr && ScienceModuleEnvironment::inventoryContainsPart &&
              ScienceModuleEnvironment::inventoryContainsPart(*inventory, StrField(*this, "requiredInventoryPart")))) {
            updateModuleUI();
            co_return;
        }
        if (!ValidEVASituation()) {
            PostMsg(Fmt("#autoLOC_238424", {experiment->experimentTitle}));
            updateModuleUI();
            co_return;
        }
    }
    if (Get<bool>("requiresEVASituation") && validEvaSituation == nullptr) {
        log::Error("[ModuleScienceExperiment]: Cannot deploy experiment '" + StrField(*this, "experimentID") +
                   "'. requires EVA situation and no valid Situation defined to Module.");
        updateModuleUI();
        co_return;
    }
    if (HasExperimentData()) {
        const std::string eid = experimentData->subjectID.substr(0, experimentData->subjectID.find('@'));
        ScienceExperiment* scienceExperiment = ResearchAndDevelopment::GetExperiment(eid);
        const std::string title = scienceExperiment ? scienceExperiment->experimentTitle : std::string();
        overwriteDialogOpen = true;
        if (ScienceEnvironment::overwriteDialog) {
            // OK: dumpData(); StartCoroutine(gatherData(showDialog)); cancel: updateModuleUI() (both dismiss the popup)
            ScienceEnvironment::overwriteDialog(
                title,
                [this, showDialog] {
                    overwriteDialogOpen = false;
                    dumpData();
                    Start_(gatherData(showDialog));
                },
                [this] {
                    overwriteDialogOpen = false;
                    updateModuleUI();
                });
        }
        co_return;
    }
    if (experiment->requireAtmosphere && p->ShieldedFromAirstream()) {
        PostMsg(Fmt("#autoLOC_238379", {p->partInfo->title}));
    } else {
        situation = ScienceUtil::GetExperimentSituation(*vessel);
        if (experiment->IsAvailableWhile(situation, *vessel->mainBody())) {
            co_yield YieldInstruction::Wait(ScienceEnvironment::Scheduler().StartCoroutine(SetFXModules(1.0f)));
            if (Get<bool>("requiresEVASituation")) {
                const std::string& ka = validEvaSituation->KerbalAction;
                if ((ka == "PlayGolf" || ka == "Banana" || ka == "Dzhanibekov") && vessel->evaController != nullptr) {
                    showDialogAfter = showDialog;
                    auto done = [this] { OnScienceComplete(); };
                    if (ScienceModuleEnvironment::evaAction) ScienceModuleEnvironment::evaAction(ka, *p, done);
                    else done();
                    co_return;
                }
            }
            showDialogAfter = showDialog;
            OnScienceComplete();
        } else {
            PostMsg(Fmt("#autoLOC_238424", {experiment->experimentTitle}));
        }
    }
    updateModuleUI();
}

// ModuleScienceExperiment.cs:664
void ModuleScienceExperiment::OnScienceComplete() { Start_(OnScienceCompleteDelay()); }

// ModuleScienceExperiment.cs:669
Enumerator ModuleScienceExperiment::OnScienceCompleteDelay() {
    Part* p = PartOf();
    Vessel* vessel = p->vessel;
    Set<bool>("Deployed", true);
    if (Get<bool>("useStaging")) lastIconColor = "cyan";
    std::string text = "";
    std::string text2;
    if (experiment->BiomeIsRelevantWhile(situation)) {
        Vessel* eVALadderVessel = vessel->EVALadderVessel();
        if (!eVALadderVessel->landedAt.empty()) {
            text = Vessel::GetLandedAtString(eVALadderVessel->landedAt);
            text2 = Loc::Format(eVALadderVessel->displaylandedAt);
        } else {
            text = ScienceUtil::GetExperimentBiome(*eVALadderVessel->mainBody(), eVALadderVessel->latitude, eVALadderVessel->longitude);
            text2 = ScienceUtil::GetBiomedisplayName(eVALadderVessel->mainBody(), text);
        }
        if (text2.empty()) text2 = text;
    }
    subject = ResearchAndDevelopment::GetExperimentSubject(*experiment, situation, *vessel->mainBody(), text, text2);
    if (Get<bool>("requiresEVASituation") && !validEvaSituation->ResultString.empty()) {
        if (validEvaSituation->ResultString.size() > 1) {
            const int hi = static_cast<int>(validEvaSituation->ResultString.size()) - 1;
            const int index = ScienceEnvironment::systemRandomNext ? ScienceEnvironment::systemRandomNext(0, hi) : 0;
            Set<std::string>("extraResultString", validEvaSituation->ResultString[static_cast<size_t>(index)]);
        } else {
            Set<std::string>("extraResultString", validEvaSituation->ResultString[0]);
        }
    }
    experimentData = std::make_shared<ScienceData>(f32store(fx(experiment->baseValue) * fx(experiment->dataScale)), Get<float>("scienceValueRatio"),
                                                   Get<float>("xmitDataScalar"), 0.0f, subject->id, subject->title, false, p->fields.flightID,
                                                   StrField(*this, "extraResultString"));
    GE::OnExperimentDeployed.Fire(experimentData.get());
    if (showDialogAfter) {
        if (Get<bool>("requiresEVASituation") && validEvaSituation != nullptr) Set<float>("dialogDelay", validEvaSituation->dialogDelay);
        dialogTimer = ScienceEnvironment::UniversalTime() + static_cast<double>(Get<float>("dialogDelay"));
        while (ScienceEnvironment::UniversalTime() <= dialogTimer) co_yield YieldInstruction::Null();
        reviewData();
    } else {
        PostMsg(Fmt("#autoLOC_238419", {p->partInfo->title, F32(experimentData->dataAmount), subject->title}));
    }
    co_yield YieldInstruction::Null();
    updateModuleUI();
}

// ModuleScienceExperiment.cs:732
Enumerator ModuleScienceExperiment::SetFXModules(float tgt) {
    log::Info("[Experiments]: Setting FX Modules to " + F32(tgt) + "...");
    if (fxModulesCreated) {
        bool allFXDone = false;
        while (!allFXDone) {
            allFXDone = true;
            for (size_t i = 0; i < fxModules.size(); i++) {
                if (!(Mathf::Abs(f32store(fx(fxModules[i]->GetScalar()) - fx(tgt))) < 0.01f)) {
                    allFXDone = false;
                    fxModules[i]->SetScalar(tgt);
                }
            }
            if (!allFXDone) co_yield YieldInstruction::Null();
        }
    }
    log::Info("[Experiments]: FX Modules set: " + F32(tgt));
}

// ModuleScienceExperiment.cs:759
void ModuleScienceExperiment::endExperiment(const std::shared_ptr<ScienceData>&) {
    if (Get<bool>("resettable") && !Get<bool>("Inoperable")) {
        Start_(resetExperiment());
        return;
    }
    dumpData();
    updateModuleUI();
}

// ModuleScienceExperiment.cs:770
void ModuleScienceExperiment::dumpData() {
    subject = nullptr;
    experimentData = nullptr;
    resultsDialog = nullptr;
}

// ModuleScienceExperiment.cs:777
void ModuleScienceExperiment::sendDataToComms(const std::shared_ptr<ScienceData>& pageData) {
    resultsDialog = nullptr;
    IScienceDataTransmitter* bestTransmitter = ScienceUtil::GetBestTransmitter(*VesselOf());
    if (bestTransmitter != nullptr) {
        std::vector<std::shared_ptr<ScienceData>> list;
        list.push_back(pageData);
        bestTransmitter->TransmitData(list);
        if (!Get<bool>("rerunnable")) SetInoperable();
        endExperiment(pageData);
        if (useCooldown) Set<double>("cooldownToGo", Get<double>("cooldownTimer"));
    } else {
        if (ScienceEnvironment::commNetEnabled && ScienceEnvironment::commNetEnabled()) PostMsg(Loc::Format("#autoLOC_238505"));
        else PostMsg(Loc::Format("#autoLOC_238507"));
        updateModuleUI();
    }
}

// ModuleScienceExperiment.cs:810
void ModuleScienceExperiment::sendDataToLab(const std::shared_ptr<ScienceData>& pageData) {
    resultsDialog = nullptr;
    ScienceLabSearch scienceLabSearch(VesselOf(), experimentData);
    if (scienceLabSearch.NextLabForDataFound()) {
        Start_(scienceLabSearch.NextLabForData->ProcessData(experimentData));
        if (!Get<bool>("rerunnable")) SetInoperable();
        endExperiment(pageData);
        if (useCooldown) Set<double>("cooldownToGo", Get<double>("cooldownTimer"));
    } else {
        scienceLabSearch.PostErrorToScreen();
        updateModuleUI();
    }
}

// ModuleScienceExperiment.cs:835
void ModuleScienceExperiment::CollectDataExternalEvent() {
    Vessel* active = dynamic_cast<Vessel*>(FlightGlobals::ActiveVessel);
    if (active == nullptr) throw NullReferenceException("FlightGlobals.ActiveVessel");
    TransferToContainer(dynamic_cast<ModuleScienceContainer*>(active->FindPartModuleImplementingClass("ModuleScienceContainer")), active->GetDisplayName());
    updateModuleUI();
}

// ModuleScienceExperiment.cs:841
void ModuleScienceExperiment::TransferToContainer(ModuleScienceContainer* container, const std::string& destName) {
    Part* p = PartOf();
    if (container != nullptr) {
        if (experimentData != nullptr) {
            if (!Get<bool>("rerunnable") && ScienceEnvironment::showTransferWarning && ScienceEnvironment::confirmDialog) {
                ScienceEnvironment::confirmDialog(StrField(*this, "collectWarningText"), [this, container](bool) { onCollectData(container); });
            } else {
                onCollectData(container);
            }
        } else {
            PostMsg(Fmt("#autoLOC_238567", {p->partInfo->title}));
        }
    } else {
        PostMsg(Fmt("#autoLOC_238572", {p->partInfo->title, destName}));
    }
}

// ModuleScienceExperiment.cs:882
void ModuleScienceExperiment::onCollectData(ModuleScienceContainer* vesselContainer) {
    Part* p = PartOf();
    if (vesselContainer != nullptr) {
        std::vector<IScienceDataContainer*> one{this};
        if (vesselContainer->StoreData(one, false)) {
            PostMsg(Fmt("#autoLOC_238582", {dynamic_cast<Part*>(vesselContainer->part)->partInfo->title}));
            if (useCooldown) Set<double>("cooldownToGo", Get<double>("cooldownTimer"));
        } else {
            PostMsg(Fmt("#autoLOC_238589", {dynamic_cast<Part*>(vesselContainer->part)->partInfo->title}));
        }
    } else {
        log::Warning("[ScienceExperiment]: This container module reference was non-null just a few seconds ago. This is why we can't have nice things.");
        PostMsg(Fmt("#autoLOC_238595", {p->partInfo->title}));
    }
}

// ModuleScienceExperiment.cs:907
void ModuleScienceExperiment::ReviewDataEvent() {
    reviewData();
    updateModuleUI();
}

// ModuleScienceExperiment.cs:913
void ModuleScienceExperiment::reviewData() {
    auto labSearch = std::make_shared<ScienceLabSearch>(VesselOf(), experimentData);
    auto page = std::make_shared<ExperimentResultDialogPage>(
        PartOf(), experimentData, experimentData->baseTransmitValue, experimentData->transmitBonus, !Get<bool>("rerunnable"),
        StrField(*this, "transmitWarningText"), true, labSearch, [this](const std::shared_ptr<ScienceData>& d) { endExperiment(d); },
        [this](const std::shared_ptr<ScienceData>& d) { keepData(d); }, [this](const std::shared_ptr<ScienceData>& d) { sendDataToComms(d); },
        [this](const std::shared_ptr<ScienceData>& d) { sendDataToLab(d); });
    ScienceEnvironment::shownPages.push_back(page);
    if (ScienceEnvironment::displayResult) ScienceEnvironment::displayResult(page);
    resultsDialog = page;
    // PartItemTransfer.DismissActive(): UI
}

// ModuleScienceExperiment.cs:919
void ModuleScienceExperiment::keepData(const std::shared_ptr<ScienceData>&) {
    resultsDialog = nullptr;
    updateModuleUI();
}

// ModuleScienceExperiment.cs:926
void ModuleScienceExperiment::ResetExperiment() {
    Events()["ResetExperiment"]->active = false;
    Start_(resetExperiment());
}

// ModuleScienceExperiment.cs:933
void ModuleScienceExperiment::ResetAction(KSPActionParam&) { Start_(resetExperiment()); }

// ModuleScienceExperiment.cs:939
void ModuleScienceExperiment::DeployExperimentExternal() {
    Part* p = PartOf();
    if (p != nullptr && p->vessel != nullptr) {
        if (experiment == nullptr) {
            log::Error("[ModuleScienceExperiment]: Cannot deploy experiment '" + StrField(*this, "experimentID") + "'. ScienceExperiment reference is Null.");
        } else if (!Get<bool>("availableShielded") && p->ShieldedFromAirstream()) {
            PostMsg(Loc::Format("#autoLOC_238643"));
        } else {
            Vessel* active = dynamic_cast<Vessel*>(FlightGlobals::ActiveVessel);
            std::string msg = StrField(*this, "usageReqMessage");
            const bool ok = ScienceUtil::RequiredUsageExternalAvailable(*p->vessel, *active, static_cast<ExperimentUsageReqs>(Get<int>("usageReqMaskExternal")), *experiment, msg);
            Set<std::string>("usageReqMessage", msg);
            if (ok) {
                if (useCooldown && Get<double>("cooldownTimer") > 0.0) {
                    PostMsg(Fmt("#autoLOC_238298", {KSPUtil::PrintTimeCompact(Get<double>("cooldownToGo"), false)}));
                } else {
                    Events()["DeployExperimentExternal"]->active = false;
                    Start_(gatherData(true));
                }
            } else {
                PostMsg("<b><color=orange>" + msg + "</color></b>");
            }
        }
    } else {
        log::Error("[ModuleScienceExperiment]: Cannot deploy experiment '" + StrField(*this, "experimentID") + "'. Part/Vessel reference is Null.");
    }
}

// ModuleScienceExperiment.cs:975
void ModuleScienceExperiment::ResetExperimentExternal() {
    Events()["ResetExperimentExternal"]->active = false;
    Start_(resetExperiment());
}

// ModuleScienceExperiment.cs:981
Enumerator ModuleScienceExperiment::resetExperiment() {
    Events()["ReviewDataEvent"]->active = false;
    resettingExperiment = true;
    co_yield YieldInstruction::Wait(ScienceEnvironment::Scheduler().StartCoroutine(SetFXModules(0.0f)));
    if (Get<bool>("useStaging")) lastIconColor = "white";
    if (experimentData != nullptr) dumpData();
    Set<bool>("Deployed", false);
    Set<bool>("Inoperable", false);
    updateModuleUI();
    if (OnExperimentReset) {
        auto cb = std::move(OnExperimentReset);
        OnExperimentReset = nullptr;
        cb();
    }
    resettingExperiment = false;
}

// ModuleScienceExperiment.cs:1005
std::vector<std::shared_ptr<ScienceData>> ModuleScienceExperiment::GetData() {
    if (experimentData != nullptr) return {experimentData};
    return {};
}

// ModuleScienceExperiment.cs:1014
void ModuleScienceExperiment::ReturnData(const std::shared_ptr<ScienceData>& data) {
    if (experimentData == nullptr) {
        experimentData = data;
        Set<bool>("Deployed", true);
        Set<bool>("Inoperable", false);
        updateModuleUI();
    }
}

// ModuleScienceExperiment.cs:1025
void ModuleScienceExperiment::DumpData(const std::shared_ptr<ScienceData>& data) {
    if (!Get<bool>("rerunnable")) SetInoperable();
    if (experimentData != nullptr && data != nullptr && experimentData->subjectID == data->subjectID) endExperiment(experimentData);
}

// ModuleScienceExperiment.cs:1037
void ModuleScienceExperiment::ReviewData() {
    if (Get<bool>("Deployed") && experimentData != nullptr && resultsDialog == nullptr) {
        reviewData();
        updateModuleUI();
    }
}

// ModuleScienceExperiment.cs:1046
int ModuleScienceExperiment::GetScienceCount() { return experimentData == nullptr ? 0 : 1; }

// ModuleScienceExperiment.cs:1055
bool ModuleScienceExperiment::IsRerunnable() { return Get<bool>("rerunnable"); }

// ModuleScienceExperiment.cs:1060
void ModuleScienceExperiment::SetInoperable() {
    PostMsg(Fmt("#autoLOC_238754", {PartOf()->partInfo->title}));
    Set<bool>("Inoperable", true);
    updateModuleUI();
}

// ModuleScienceExperiment.cs:1068
void ModuleScienceExperiment::CleanUpExperimentExternal() {
    Vessel* active = dynamic_cast<Vessel*>(FlightGlobals::ActiveVessel);
    if (active != nullptr && active->isEVA()) {
        if (active->parts[0]->protoModuleCrew[0]->HasEffect<Experience::Effects::ScienceResetSkill>()) {
            Set<bool>("Inoperable", false);
            ResetExperiment();
            PostMsg(Fmt("#autoLOC_238770", {PartOf()->partInfo->title}));
        } else {
            PostMsg(Loc::Format("#autoLOC_238776"));
        }
    } else {
        log::Error("[ModuleScienceExperiment]: The CleanUp Event should only be availabel for EVAs!");
    }
}

// ModuleScienceExperiment.cs:1089
void ModuleScienceExperiment::ReviewDataItem(const std::shared_ptr<ScienceData>& data) {
    if (data == experimentData) reviewData();
}

// ModuleScienceExperiment.cs:1097
std::string ModuleScienceExperiment::GetInfo() {
    // XKCDColors.HexFormat.Cyan, RUIutils.GetYesNoUIString: UI texts; the yes/no tags are #autoLOC_ yes / no strings
    auto yn = [](bool b) { return Loc::Format(b ? "#autoLOC_6001078" : "#autoLOC_6001079"); };
    std::string text = "<color=#00ffff>" + StrField(*this, "experimentActionName") + "</color>\n";
    text += Fmt("#autoLOC_238797", {yn(Get<bool>("dataIsCollectable"))});
    text += Fmt("#autoLOC_238798", {yn(Get<bool>("rerunnable"))});
    text += Fmt("#autoLOC_238799", {yn(Get<bool>("resettable"))});
    if (!Get<bool>("rerunnable")) {
        const std::string resourceToReset = StrField(*this, "resourceToReset");
        const PartResourceDefinition* definition = LookupDefinition(resourceToReset);
        text += Fmt("#autoLOC_238802", {definition == nullptr ? resourceToReset : definition->displayName()});
        text += Fmt("#autoLOC_238803", {F32(Get<float>("resourceResetCost"))});
    }
    if (Get<bool>("showScienceValueRatio")) text += Fmt("#autoLOC_8004390", {dotnet::FormatSingleF(f32store(fx(Get<float>("scienceValueRatio")) * 100.0), 0)});
    return text;
}

// ModuleScienceExperiment.cs:1119
std::string ModuleScienceExperiment::GetModuleDisplayName() { return Loc::Format("#autoLoc_6003059"); }

// ModuleScienceExperiment.cs:1125
void ModuleScienceExperiment::TransferDataEvent() {
    if (ScienceEnvironment::partItemTransferActive && ScienceEnvironment::partItemTransferActive()) PostMsg(Loc::Format("#autoLOC_238816"));
    else ExperimentTransfer::Create(*PartOf(), *this);
}

// ModuleScienceExperiment.cs:1137
void ModuleScienceExperiment::FindContainer() {
    containersDirty = false;
    hasContainer = false;
    Vessel* vessel = VesselOf();
    if (!(vessel != nullptr)) return;
    int count = static_cast<int>(vessel->Parts().size());
    while (true) {
        if (count-- <= 0) return;
        Part* part = vessel->Parts()[static_cast<size_t>(count)];
        if (part->State() != PartStates::DEAD) {
            auto* c = FindModule<ModuleScienceContainer>(*part, "ModuleScienceContainer");
            if (c != nullptr && c->Get<bool>("canBeTransferredToInVessel")) break;
        }
    }
    hasContainer = true;
}

// ModuleScienceExperiment.cs:1166
void ModuleScienceExperiment::OnDataTransfer(bool itemMoved, Part* containerPart) {
    if (itemMoved && containerPart != nullptr) {
        TransferToContainer(FindModule<ModuleScienceContainer>(*containerPart, "ModuleScienceContainer"), containerPart->partInfo->title);
        updateModuleUI();
    }
}

// ModuleScienceExperiment.cs:1175
void ModuleScienceExperiment::OnVesselModified(Vessel* v) {
    if (v == VesselOf() && ScienceEnvironment::loadedSceneIsFlight) containersDirty = true;
}

// ModuleScienceExperiment.cs:1207
std::string ModuleScienceExperiment::GetStagingEnableText() {
    if (!IsNull("stagingEnableText") && !Get<std::string>("stagingEnableText").empty()) return Get<std::string>("stagingEnableText");
    return Loc::Format("#autoLOC_238885");
}

// ModuleScienceExperiment.cs:1216
std::string ModuleScienceExperiment::GetStagingDisableText() {
    if (!IsNull("stagingDisableText") && !Get<std::string>("stagingDisableText").empty()) return Get<std::string>("stagingDisableText");
    return Loc::Format("#autoLOC_238886");
}

// ----------------------------------------------------------------------------------------------------------------------------------
// ModuleScienceContainer

Part* ModuleScienceContainer::PartOf() const { return dynamic_cast<Part*>(part); }
Vessel* ModuleScienceContainer::VesselOf() const {
    Part* p = PartOf();
    return p ? p->vessel : nullptr;
}

ModuleScienceContainer::~ModuleScienceContainer() {
    if (subscribed_) GE::onPartActionUICreate.Remove(this, &ModuleScienceContainer::onPartActionUIOpened);
}

void ModuleScienceContainer::OnAwake() {
    PartModule::OnAwake();
    Events().Bind("StoreDataExternalEvent", BaseEvent::Handler([this] { StoreDataExternalEvent(); }));
    Events().Bind("CollectDataExternalEvent", BaseEvent::Handler([this] { CollectDataExternalEvent(); }));
    Events().Bind("TransferDataEvent", BaseEvent::Handler([this] { TransferDataEvent(); }));
    Events().Bind("CollectAllEvent", BaseEvent::Handler([this] { CollectAllEvent(); }));
    Events().Bind("ReviewDataEvent", BaseEvent::Handler([this] { ReviewDataEvent(); }));
    Actions().Bind("CollectAllAction", [this](KSPActionParam& p) { CollectAllAction(p); });
}

// ModuleScienceContainer.cs:57
void ModuleScienceContainer::OnLoad(const ConfigNode& node) {
    dataStore.clear();
    for (ConfigNode* n : node.GetNodes("ScienceData")) dataStore.push_back(std::make_shared<ScienceData>(*n));
    for (ConfigNode* n : node.GetNodes("CommsData")) dataStore.push_back(std::make_shared<ScienceData>(*n));
}

// ModuleScienceContainer.cs:76
void ModuleScienceContainer::OnSave(ConfigNode& node) {
    for (size_t i = 0; i < dataStore.size(); i++) dataStore[i]->Save(*node.AddNode(Get<bool>("dataIsRecoverable") ? "ScienceData" : "CommsData"));
}

// ModuleScienceContainer.cs:85
void ModuleScienceContainer::OnStart(StartState) {
    pagesWithDialogs.clear();
    updateModuleUI();
    GE::onPartActionUICreate.Add(this, &ModuleScienceContainer::onPartActionUIOpened);
    subscribed_ = true;
}

// ModuleScienceContainer.cs:97
void ModuleScienceContainer::onPartActionUIOpened(Part* p) {
    if (p == PartOf()) updateModuleUI();
}

// ModuleScienceContainer.cs:105
void ModuleScienceContainer::updateModuleUI() {
    const int count = static_cast<int>(dataStore.size());
    const int capacity = Get<int>("capacity");
    Events()["ReviewDataEvent"]->active = count > 0 && static_cast<int>(pagesWithDialogs.size()) != count;
    Events()["ReviewDataEvent"]->SetGuiName(StrField(*this, "reviewActionName") + " (" + std::to_string(count) + ")");
    Fields()["status"]->guiActive = (Get<bool>("showStatus") && count == 0) || capacity > 0;
    if (capacity > 0) Set<std::string>("status", Fmt("#autoLOC_237286", {std::to_string(count), std::to_string(capacity)}));
    else Set<std::string>("status", Fmt("#autoLOC_237288", {std::to_string(count)}));
    const int activeVesselDataCount = GetActiveVesselDataCount();
    const int storedDataCount = GetStoredDataCount();
    BaseEvent* store = Events()["StoreDataExternalEvent"];
    store->SetGuiName(StrField(*this, "storeActionName") + " (" + std::to_string(activeVesselDataCount) + ")");
    store->active = activeVesselDataCount > 0 && Get<bool>("dataIsStorable");
    store->externalToEVAOnly = Get<bool>("evaOnlyStorage");
    store->unfocusedRange = Get<float>("storageRange");
    BaseEvent* collect = Events()["CollectDataExternalEvent"];
    collect->SetGuiName(StrField(*this, "collectActionName") + " (" + std::to_string(storedDataCount) + ")");
    const bool flag = storedDataCount > 0 && Get<bool>("dataIsCollectable");
    collect->active = flag;
    Events()["TransferDataEvent"]->active = flag && Get<bool>("canTransferInVessel");
    Events()["CollectAllEvent"]->active = Get<bool>("canTransferInVessel") && activeVesselDataCount > 0 && activeVesselDataCount > static_cast<int>(dataStore.size());
    collect->externalToEVAOnly = Get<bool>("evaOnlyStorage");
    collect->unfocusedRange = Get<float>("storageRange");
}

// ModuleScienceContainer.cs:134
int ModuleScienceContainer::GetActiveVesselDataCount() {
    Vessel* activeVessel = dynamic_cast<Vessel*>(FlightGlobals::ActiveVessel);
    if (activeVessel != nullptr) {
        int num = 0;
        int count = static_cast<int>(activeVessel->parts.size());
        while (count-- > 0) {
            Part* p = activeVessel->parts[static_cast<size_t>(count)];
            int count2 = static_cast<int>(p->Modules().Count());
            while (count2-- > 0) {
                if (auto* c = dynamic_cast<IScienceDataContainer*>(p->Modules()[static_cast<size_t>(count2)])) num += c->GetScienceCount();
            }
        }
        return num;
    }
    return 0;
}

// ModuleScienceContainer.cs:158
int ModuleScienceContainer::GetStoredDataCount() {
    if (Get<bool>("CollectOnlyOwnData")) return static_cast<int>(dataStore.size());
    int num = 0;
    Part* p = PartOf();
    int count = static_cast<int>(p->Modules().Count());
    while (count-- > 0) {
        if (auto* c = dynamic_cast<IScienceDataContainer*>(p->Modules()[static_cast<size_t>(count)])) num += c->GetScienceCount();
    }
    return num;
}

// ModuleScienceContainer.cs:177
void ModuleScienceContainer::StoreDataExternalEvent() {
    Vessel* active = dynamic_cast<Vessel*>(FlightGlobals::ActiveVessel);
    std::vector<IScienceDataContainer*> list;
    for (Part* p : active->Parts())
        for (size_t i = 0; i < p->Modules().Count(); i++)
            if (auto* c = dynamic_cast<IScienceDataContainer*>(p->Modules()[i])) list.push_back(c);
    bool flag = false;
    int count = static_cast<int>(list.size());
    while (count-- > 0) {
        if (list[static_cast<size_t>(count)]->GetScienceCount() > 0) {
            flag = true;
            break;
        }
    }
    const std::string title = PartOf()->partInfo->title;
    if (flag) {
        if (StoreData(list, false)) PostMsg(Fmt("#autoLOC_237370", {title}));
        else PostMsg(Fmt("#autoLOC_237374", {title}));
    } else {
        PostMsg(Fmt("#autoLOC_237379", {title}));
    }
    updateModuleUI();
}

// ModuleScienceContainer.cs:209
void ModuleScienceContainer::CollectDataExternalEvent() {
    Vessel* active = dynamic_cast<Vessel*>(FlightGlobals::ActiveVessel);
    TransferToContainer(dynamic_cast<ModuleScienceContainer*>(active->FindPartModuleImplementingClass("ModuleScienceContainer")), active->GetDisplayName());
    updateModuleUI();
}

// ModuleScienceContainer.cs:215
void ModuleScienceContainer::TransferToContainer(ModuleScienceContainer* activeVesselContainer, const std::string& targetName) {
    std::vector<IScienceDataContainer*> list;
    if (!Get<bool>("CollectOnlyOwnData")) {
        Part* p = PartOf();
        for (size_t i = 0; i < p->Modules().Count(); i++)
            if (auto* c = dynamic_cast<IScienceDataContainer*>(p->Modules()[i])) list.push_back(c);
    } else {
        list.push_back(this);
    }
    if (activeVesselContainer != nullptr) {
        bool flag = false;
        int count = static_cast<int>(list.size());
        while (count-- > 0) {
            if (list[static_cast<size_t>(count)]->GetScienceCount() > 0) {
                flag = true;
                break;
            }
        }
        const std::string title = dynamic_cast<Part*>(activeVesselContainer->part)->partInfo->title;
        if (flag) {
            if (activeVesselContainer->StoreData(list, false)) PostMsg(Fmt("#autoLOC_237418", {title}));
            else PostMsg(Fmt("#autoLOC_237422", {title}));
        } else {
            PostMsg(Fmt("#autoLOC_237427", {title}));
        }
    } else {
        PostMsg(Fmt("#autoLOC_237432", {PartOf()->partInfo->title, targetName}));
    }
}

// ModuleScienceContainer.cs:253
void ModuleScienceContainer::TransferDataEvent() {
    if (ScienceEnvironment::partItemTransferActive && ScienceEnvironment::partItemTransferActive()) PostMsg(Loc::Format("#autoLOC_237449"));
    else ExperimentTransfer::Create(*PartOf(), *this);
}

// ModuleScienceContainer.cs:266
void ModuleScienceContainer::CollectAllAction(KSPActionParam&) {
    updateModuleUI();
    if (Events()["CollectAllEvent"]->active) CollectAllEvent();
}

// ModuleScienceContainer.cs:276
void ModuleScienceContainer::CollectAllEvent() {
    std::vector<IScienceDataContainer*> list;
    Vessel* vessel = VesselOf();
    int count = static_cast<int>(vessel->Parts().size());
    while (count-- > 0) {
        Part* p = vessel->Parts()[static_cast<size_t>(count)];
        int count2 = static_cast<int>(p->Modules().Count());
        while (count2-- > 0) {
            auto* c = dynamic_cast<IScienceDataContainer*>(p->Modules()[static_cast<size_t>(count2)]);
            if (c != nullptr && c != this && c->GetScienceCount() > 0) list.push_back(c);
        }
    }
    const std::string title = PartOf()->partInfo->title;
    if (!list.empty()) {
        if (StoreData(list, false)) PostMsg(Fmt("#autoLOC_237485", {title}));
        else PostMsg(Fmt("#autoLOC_237489", {title}));
    } else {
        PostMsg(Fmt("#autoLOC_237494", {title}));
    }
    updateModuleUI();
}

// ModuleScienceContainer.cs:310
void ModuleScienceContainer::OnDataTransfer(bool itemMoved, Part* containerPart) {
    if (itemMoved && containerPart != nullptr) {
        TransferToContainer(FindModule<ModuleScienceContainer>(*containerPart, "ModuleScienceContainer"), containerPart->partInfo->title);
        updateModuleUI();
    }
}

// ModuleScienceContainer.cs:319
bool ModuleScienceContainer::StoreData(std::vector<IScienceDataContainer*>& containers, bool dumpRepeats, bool excludeScienceContainers,
                                       bool scienceContainerPriority) {
    if (excludeScienceContainers) {
        // sic: the C# collects the ModuleScienceContainers' ... `containers[count] as ModuleScienceContainer != null` and removes them
        std::vector<IScienceDataContainer*> list;
        int count = static_cast<int>(containers.size());
        while (count-- > 0) {
            if (dynamic_cast<ModuleScienceContainer*>(containers[static_cast<size_t>(count)]) != nullptr) list.push_back(containers[static_cast<size_t>(count)]);
        }
        int count2 = static_cast<int>(list.size());
        while (count2-- > 0) {
            auto it = std::find(containers.begin(), containers.end(), list[static_cast<size_t>(count2)]);
            if (it != containers.end()) containers.erase(it);
        }
    }
    sortContainerList(containers, scienceContainerPriority);
    const int capacity = Get<int>("capacity");
    int num = 0;
    const int count3 = static_cast<int>(containers.size());
    for (int i = 0; i < count3; i++) {
        IScienceDataContainer* scienceDataContainer = containers[static_cast<size_t>(i)];
        const int scienceCount = scienceDataContainer->GetScienceCount();
        if (scienceCount == 0) continue;
        if (scienceCount + static_cast<int>(dataStore.size()) > capacity && static_cast<float>(capacity) != 0.0f) {
            num++;
            continue;
        }
        std::vector<std::shared_ptr<ScienceData>> data = scienceDataContainer->GetData();
        for (size_t j = 0; j < data.size(); j++) {
            if (data[j] == nullptr) continue;
            bool flag = !Get<bool>("allowRepeatedSubjects");
            if (flag) {
                flag = false;
                int count4 = static_cast<int>(dataStore.size());
                while (count4-- > 0) {
                    if (dataStore[static_cast<size_t>(count4)]->subjectID == data[j]->subjectID) {
                        flag = true;
                        break;
                    }
                }
            }
            if (flag) {
                PostMsg(Fmt("#autoLOC_6001022", {PartOf()->partInfo->title, data[j]->title}));
                if (dumpRepeats) scienceDataContainer->DumpData(data[j]);
                else num++;
            } else {
                scienceDataContainer->DumpData(data[j]);
                AddData(data[j]);
            }
        }
    }
    return num == 0;
}

// ModuleScienceContainer.cs:398
void ModuleScienceContainer::sortContainerList(std::vector<IScienceDataContainer*>& containers, bool scienceContainerPriority) {
    int count = static_cast<int>(containers.size());
    while (count-- > 0) {
        int num = std::numeric_limits<int>::max();
        int index = 0;
        for (int num2 = count; num2 >= 0; num2--) {
            const int scienceCount = containers[static_cast<size_t>(num2)]->GetScienceCount();
            if (scienceCount < num) {
                num = scienceCount;
                index = num2;
            }
        }
        std::swap(containers[static_cast<size_t>(count)], containers[static_cast<size_t>(index)]);
    }
    if (scienceContainerPriority) return;
    std::vector<IScienceDataContainer*> list, list2;
    int count2 = static_cast<int>(containers.size());
    while (count2-- > 0) {
        if (dynamic_cast<ModuleScienceContainer*>(containers[static_cast<size_t>(count2)]) != nullptr) list2.push_back(containers[static_cast<size_t>(count2)]);
        else list.push_back(containers[static_cast<size_t>(count2)]);
    }
    size_t num3 = 0;
    for (size_t i = 0; i < list.size(); i++) containers[num3++] = list[i];
    for (size_t j = 0; j < list2.size(); j++) containers[num3++] = list2[j];
}

// ModuleScienceContainer.cs:449
bool ModuleScienceContainer::AddData(const std::shared_ptr<ScienceData>& data) {
    const int capacity = Get<int>("capacity");
    const std::string title = PartOf()->partInfo->title;
    if (static_cast<int>(dataStore.size()) >= capacity && capacity != 0) {
        PostMsg(Fmt("#autoLOC_237617", {title, data->title, std::to_string(dataStore.size()), std::to_string(capacity)}));
        return false;
    }
    data->container = PartOf()->fields.flightID;
    dataStore.push_back(data);
    updateModuleUI();
    if (capacity != 0) PostMsg(Fmt("#autoLOC_237607", {title, data->title, std::to_string(dataStore.size()), std::to_string(capacity)}));
    else PostMsg(Fmt("#autoLOC_237611", {title, data->title}));
    return true;
}

// ModuleScienceContainer.cs:470
bool ModuleScienceContainer::HasData(const ScienceData& data) {
    int count = static_cast<int>(dataStore.size());
    do {
        if (count-- <= 0) return false;
    } while (!(dataStore[static_cast<size_t>(count)]->subjectID == data.subjectID));
    return true;
}

// ModuleScienceContainer.cs:484
void ModuleScienceContainer::RemoveData(const std::shared_ptr<ScienceData>& data) {
    auto it = std::find(dataStore.begin(), dataStore.end(), data);
    if (it != dataStore.end()) dataStore.erase(it);
    updateModuleUI();
}

// ModuleScienceContainer.cs:490
std::vector<std::shared_ptr<ScienceData>> ModuleScienceContainer::GetData() { return dataStore; }

// ModuleScienceContainer.cs:495
void ModuleScienceContainer::ReturnData(const std::shared_ptr<ScienceData>& data) {
    const int count = static_cast<int>(dataStore.size());
    const int capacity = Get<int>("capacity");
    if (count >= capacity && capacity != 0) return;
    if (!Get<bool>("allowRepeatedSubjects")) {
        int index = count;
        while (index-- > 0) {
            if (dataStore[static_cast<size_t>(index)]->subjectID == data->subjectID) return;
        }
    }
    dataStore.push_back(data);
    updateModuleUI();
}

// ModuleScienceContainer.cs:517
void ModuleScienceContainer::DumpData(const std::shared_ptr<ScienceData>& data) {
    PostMsg(Fmt("#autoLOC_237662", {PartOf()->partInfo->title, data->title}));
    auto it = std::find(dataStore.begin(), dataStore.end(), data);
    if (it != dataStore.end()) dataStore.erase(it);
    updateModuleUI();
}

// ModuleScienceContainer.cs:525
void ModuleScienceContainer::ReviewDataEvent() {
    reviewData();
    updateModuleUI();
}

// ModuleScienceContainer.cs:531
void ModuleScienceContainer::ReviewData() {
    reviewData();
    updateModuleUI();
}

// ModuleScienceContainer.cs:537
void ModuleScienceContainer::reviewData() {
    const int count = static_cast<int>(dataStore.size());
    if (count > 0) {
        for (int i = 0; i < count; i++) ReviewDataItem(dataStore[static_cast<size_t>(i)]);
    }
}

// ModuleScienceContainer.cs:549
void ModuleScienceContainer::ReviewDataItem(const std::shared_ptr<ScienceData>& data) {
    if (std::find(pagesWithDialogs.begin(), pagesWithDialogs.end(), data) == pagesWithDialogs.end()) {
        pagesWithDialogs.push_back(data);
        auto labSearch = std::make_shared<ScienceLabSearch>(VesselOf(), data);
        auto page = std::make_shared<ExperimentResultDialogPage>(
            PartOf(), data, data->baseTransmitValue, data->transmitBonus, false, std::string(), false, labSearch,
            [this](const std::shared_ptr<ScienceData>& d) { discardData(d); }, [this](const std::shared_ptr<ScienceData>& d) { keepData(d); },
            [this](const std::shared_ptr<ScienceData>& d) { sendDataToComms(d); }, [this](const std::shared_ptr<ScienceData>& d) { sendDataToLab(d); });
        ScienceEnvironment::shownPages.push_back(page);
        if (ScienceEnvironment::displayResult) ScienceEnvironment::displayResult(page);
    }
}

// ModuleScienceContainer.cs:558
void ModuleScienceContainer::discardData(const std::shared_ptr<ScienceData>& pageData) {
    auto it = std::find(pagesWithDialogs.begin(), pagesWithDialogs.end(), pageData);
    if (it != pagesWithDialogs.end()) pagesWithDialogs.erase(it);
    RemoveData(pageData);
    updateModuleUI();
}

// ModuleScienceContainer.cs:565
void ModuleScienceContainer::keepData(const std::shared_ptr<ScienceData>& pageData) {
    auto it = std::find(pagesWithDialogs.begin(), pagesWithDialogs.end(), pageData);
    if (it != pagesWithDialogs.end()) pagesWithDialogs.erase(it);
    updateModuleUI();
}

// ModuleScienceContainer.cs:571
void ModuleScienceContainer::sendDataToComms(const std::shared_ptr<ScienceData>& pageData) {
    auto it = std::find(pagesWithDialogs.begin(), pagesWithDialogs.end(), pageData);
    if (it != pagesWithDialogs.end()) pagesWithDialogs.erase(it);
    IScienceDataTransmitter* bestTransmitter = ScienceUtil::GetBestTransmitter(*VesselOf());
    if (bestTransmitter != nullptr) {
        std::vector<std::shared_ptr<ScienceData>> list;
        list.push_back(pageData);
        bestTransmitter->TransmitData(list);
        RemoveData(pageData);
    } else if (ScienceEnvironment::commNetEnabled && ScienceEnvironment::commNetEnabled()) {
        PostMsg(Loc::Format("#autoLOC_237738"));
    } else {
        PostMsg(Loc::Format("#autoLOC_237740"));
    }
    updateModuleUI();
}

// ModuleScienceContainer.cs:593
void ModuleScienceContainer::sendDataToLab(const std::shared_ptr<ScienceData>& pageData) {
    auto it = std::find(pagesWithDialogs.begin(), pagesWithDialogs.end(), pageData);
    if (it != pagesWithDialogs.end()) pagesWithDialogs.erase(it);
    ScienceLabSearch scienceLabSearch(VesselOf(), pageData);
    if (scienceLabSearch.NextLabForDataFound()) {
        ScienceEnvironment::Scheduler().StartCoroutine(scienceLabSearch.NextLabForData->ProcessData(pageData));
        RemoveData(pageData);
    } else {
        scienceLabSearch.PostErrorToScreen();
        updateModuleUI();
    }
}

// ModuleScienceContainer.cs:609
int ModuleScienceContainer::GetScienceCount() { return static_cast<int>(dataStore.size()); }

// ModuleScienceContainer.cs:614
bool ModuleScienceContainer::IsRerunnable() { return true; }

// ModuleScienceContainer.cs:619
std::string ModuleScienceContainer::GetModuleDisplayName() { return Loc::Format("#autoLoc_6003058"); }

// ----------------------------------------------------------------------------------------------------------------------------------
// ModuleScienceLab

Part* ModuleScienceLab::PartOf() const { return dynamic_cast<Part*>(part); }
Vessel* ModuleScienceLab::VesselOf() const {
    Part* p = PartOf();
    return p ? p->vessel : nullptr;
}

ModuleScienceLab::~ModuleScienceLab() {
    for (auto& c : running) ScienceEnvironment::Scheduler().StopCoroutine(c);
    if (subscribed_) {
        GE::onPartActionUICreate.Remove(this, &ModuleScienceLab::updateModuleUI_);
        GE::OnTriggeredDataTransmission.Remove(this, &ModuleScienceLab::OnTransmissionComplete);
    }
}

// ModuleScienceLab.cs:73
ModuleScienceConverter* ModuleScienceLab::Converter() {
    if (converter_ == nullptr) converter_ = FindModule<ModuleScienceConverter>(*PartOf(), "ModuleScienceConverter");
    return converter_;
}

// ModuleScienceLab.cs:81
void ModuleScienceLab::TransmitScience() { sendDataToComms(); }

// ModuleScienceLab.cs:86
void ModuleScienceLab::OnTransmissionComplete(ScienceData* data, Vessel*, bool xmitAborted) {
    const std::string prefix = "sciencelab@";
    if (data->container != PartOf()->fields.flightID || data->subjectID.compare(0, prefix.size(), prefix) != 0) return;
    transmittingData = false;
    updateModuleUI();
    if (!xmitAborted) {
        const float dataAmount = data->dataAmount;
        progressMessage = Fmt("#autoLOC_238985", {VesselOf()->GetDisplayName(), ResearchAndDevelopment::ScienceTransmissionRewardString(dataAmount)});
        PostMsg(progressMessage);
        if (ResearchAndDevelopment::Instance != nullptr) ResearchAndDevelopment::Instance->AddScience(Get<float>("storedScience"), science_reason::ScienceTransmission());
        Set<float>("storedScience", f32store(fx(Get<float>("storedScience")) - fx(dataAmount)));
    }
}

// ModuleScienceLab.cs:107
void ModuleScienceLab::OnAwake() {
    PartModule::OnAwake();
    Events().Bind("TransmitScience", BaseEvent::Handler([this] { TransmitScience(); }));
    Events().Bind("CleanModulesEvent", BaseEvent::Handler([this] { CleanModulesEvent(); }));
    consumedResources.clear();
    for (size_t i = 0; i < processResources.size(); i++) consumedResources.push_back(LookupDefinition(processResources[i]->name));
    if (!subscribed_) {
        GE::onPartActionUICreate.Add(this, &ModuleScienceLab::updateModuleUI_);
        GE::OnTriggeredDataTransmission.Add(this, &ModuleScienceLab::OnTransmissionComplete);
        subscribed_ = true;
    }
}

void ModuleScienceLab::updateModuleUI_(Part* p) {
    if (p == PartOf()) updateModuleUI();
}

// ModuleScienceLab.cs:136
void ModuleScienceLab::OnLoad(const ConfigNode& node) {
    if (node.HasNode("RESOURCE_PROCESS")) {
        processResources.clear();
        for (ConfigNode* n : node.GetNodes("RESOURCE_PROCESS")) {
            auto moduleResource = std::make_shared<ModuleResource>();
            moduleResource->Load(*n);
            processResources.push_back(moduleResource);
        }
    }
    if (node.HasNode("EXPERIMENT_DATA")) {
        ExperimentData.clear();
        ConfigNode* node3 = node.GetNode("EXPERIMENT_DATA");
        for (int j = 0; j < node3->values().Count(); j++) {
            const std::string value = node3->values()[j].value;
            log::Info("Loading: " + value);
            ExperimentData.push_back(value);
        }
    }
}

// ModuleScienceLab.cs:165
void ModuleScienceLab::OnSave(ConfigNode& node) {
    PartModule::OnSave(node);
    auto configNode = ConfigNode::Create("EXPERIMENT_DATA");
    for (size_t i = 0; i < ExperimentData.size(); i++) configNode->AddValue("experiment", ExperimentData[i]);
    node.AddNode(configNode);
}

// ModuleScienceLab.cs:178
void ModuleScienceLab::OnStart(StartState) {
    Part* p = PartOf();
    container_ = p ? dynamic_cast<ModuleScienceContainer*>(p->Modules().GetModule(static_cast<size_t>(Get<int>("containerModuleIndex")))) : nullptr;
    if (container_ == nullptr) {
        log::Error("[ModuleScienceLab]: No Container Module found at index " + std::to_string(Get<int>("containerModuleIndex")));
        return;
    }
    updateModuleUI();
}

// ModuleScienceLab.cs:200
void ModuleScienceLab::updateModuleUI() {
    BaseEvent* transmit = Events()["TransmitScience"];
    BaseEvent* clean = Events()["CleanModulesEvent"];
    transmit->active = !transmittingData;
    Part* p = PartOf();
    int crewCountOfExperienceEffect = 0;
    for (auto& pcm : p->protoModuleCrew)
        if (pcm->HasEffect<Experience::Effects::ScienceSkill>()) crewCountOfExperienceEffect++;
    if (static_cast<float>(crewCountOfExperienceEffect) < Get<float>("crewsRequired")) {
        Set<std::string>("statusText", Fmt("#autoLOC_6001028", {std::to_string(crewCountOfExperienceEffect), F32(Get<float>("crewsRequired"))}));
        clean->active = false;
        return;
    }
    Set<std::string>("statusText", Loc::Format("#autoLOC_239109"));
    if (!Get<bool>("canResetConnectedModules") && !Get<bool>("canResetNearbyModules")) {
        clean->active = false;
        return;
    }
    int num = 0;
    Vessel* active = dynamic_cast<Vessel*>(FlightGlobals::ActiveVessel);
    int count = static_cast<int>(active->Parts().size());
    while (count-- > 0) {
        Part* pt = active->Parts()[static_cast<size_t>(count)];
        int count2 = static_cast<int>(pt->Modules().Count());
        while (count2-- > 0) {
            auto* exp = dynamic_cast<ModuleScienceExperiment*>(pt->Modules()[static_cast<size_t>(count2)]);
            if (exp != nullptr && exp->Get<bool>("Inoperable")) num++;
        }
    }
    clean->active = num > 0;
    clean->guiActiveUnfocused = Get<bool>("canResetNearbyModules");
    clean->unfocusedRange = Get<float>("interactionRange");
    clean->SetGuiName(Fmt("#autoLOC_6001863", {std::to_string(num)}));
}

// ModuleScienceLab.cs:237
bool ModuleScienceLab::IsStorable(const ScienceData& item) {
    if (fx(item.labValue) + fx(Get<float>("dataStored")) > fx(Get<float>("dataStorage"))) {
        progressMessage = Fmt("#autoLOC_239144", {PartOf()->partInfo->title, item.title});
        PostMsg(progressMessage);
        return false;
    }
    if (std::find(ExperimentData.begin(), ExperimentData.end(), item.subjectID) != ExperimentData.end()) {
        progressMessage = Fmt("#autoLOC_239150", {PartOf()->partInfo->title, item.title});
        PostMsg(progressMessage);
        return false;
    }
    return true;
}

// ModuleScienceLab.cs:254
Enumerator ModuleScienceLab::ProcessData(std::shared_ptr<ScienceData> item, std::function<void(const std::shared_ptr<ScienceData>&)> onComplete) {
    if (IsStorable(*item)) {
        PostMsg(Fmt("#autoLOC_239163", {item->title, F32(item->labValue)}));
        StoreData(item);
        if (onComplete) onComplete(item);
    }
    co_return;
}

// ModuleScienceLab.cs:265
void ModuleScienceLab::StoreData(const std::shared_ptr<ScienceData>& item) {
    if (IsStorable(*item)) {
        ExperimentData.push_back(item->subjectID);
        Set<float>("dataStored", f32store(fx(Get<float>("dataStored")) + fx(item->labValue)));
    }
}

// ModuleScienceLab.cs:275
void ModuleScienceLab::CleanModulesEvent() {
    running.push_back(ScienceEnvironment::Scheduler().StartCoroutine(CleanUpVesselExperiments(dynamic_cast<Vessel*>(FlightGlobals::ActiveVessel))));
}

// ModuleScienceLab.cs:280
Enumerator ModuleScienceLab::CleanUpVesselExperiments(Vessel* v) {
    int i = static_cast<int>(v->Parts().size());
    while (i-- > 0) {
        Part* p = v->Parts()[static_cast<size_t>(i)];
        int j = static_cast<int>(p->Modules().Count());
        while (j-- > 0) {
            auto* exp = dynamic_cast<ModuleScienceExperiment*>(p->Modules()[static_cast<size_t>(j)]);
            if (exp != nullptr && exp->Get<bool>("Inoperable")) {
                co_yield YieldInstruction::Wait(ScienceEnvironment::Scheduler().StartCoroutine(CleanUpExperiment(exp)));
                co_yield YieldInstruction::WaitForSeconds(1.0f);
            }
        }
    }
}

// ModuleScienceLab.cs:303
Enumerator ModuleScienceLab::CleanUpExperiment(ModuleScienceExperiment* exp) {
    if (!exp->Get<bool>("Inoperable")) {
        log::Error("[Lab Module]: " + dynamic_cast<Part*>(exp->part)->partInfo->title + " does not require cleanup");
        co_return;
    }
    if (exp->experiment == nullptr) {
        log::Error("[ModuleScienceExperiment]: Cannot deploy experiment '" + StrField(*exp, "experimentID") + "'. ScienceExperiment reference is Null.");
        co_return;
    }
    co_yield YieldInstruction::Wait(ScienceEnvironment::Scheduler().StartCoroutine(
        gatherProcessResources(f32store(fx(exp->Get<float>("resourceResetCost")) * fx(exp->experiment->baseValue)), dynamic_cast<Part*>(exp->part)->partInfo->title,
                               "#autoLOC_6001029", "#autoLOC_6001030")));
    if (allResourcesAvailable) {
        for (size_t i = 0; i < processResources.size(); i++) {
            auto& moduleResource = processResources[i];
            moduleResource->currentAmount -= moduleResource->amount * static_cast<double>(exp->Get<float>("resourceResetCost"));
        }
        exp->Set<bool>("Inoperable", false);
        exp->ResetExperiment();
        progressMessage = Fmt("#autoLOC_239242", {PartOf()->partInfo->title, dynamic_cast<Part*>(exp->part)->partInfo->title});
        PostMsg(progressMessage);
    }
}

// ModuleScienceLab.cs:336
Enumerator ModuleScienceLab::gatherProcessResources(float amount, std::string expParttitle, std::string progressMessageText, std::string shortageMessageText) {
    allResourcesAvailable = true;
    size_t i = 0;
    const size_t iC = processResources.size();
    while (i < iC) {
        std::shared_ptr<ModuleResource> mr = processResources[i];
        const PartResourceDefinition* definition = LookupDefinition(mr->name);
        const std::string resourceName = (definition == nullptr) ? mr->title : definition->displayName();
        double required = mr->amount * static_cast<double>(amount);
        mr->rate = mr->amount * static_cast<double>(ScienceEnvironment::deltaTime);
        while (dotnet::Math::Ceiling(mr->currentAmount) < dotnet::Math::Floor(required)) {
            mr->currentRequest = std::min(mr->rate, required - mr->currentAmount);
            double num = PartOf()->RequestResource(mr->id, mr->currentRequest);
            mr->currentAmount += num;
            std::string text;
            if (num <= 0.0) {
                text = "(" + dotnet::FormatDoubleF(mr->currentAmount, 0) + "/" + dotnet::FormatDoubleF(mr->amount * static_cast<double>(amount), 0) + ")";
                progressMessage = "<color=orange>" + Fmt(shortageMessageText.c_str(), {PartOf()->partInfo->title, resourceName, expParttitle, text}) + "</color>";
                PostMsg(progressMessage);
                allResourcesAvailable = false;
                co_return;
            }
            text = "[" + dotnet::FormatDoubleF(mr->currentAmount / required * 100.0, 0) + "%]";
            progressMessage = "<color=#99ff0099>" + Fmt(progressMessageText.c_str(), {PartOf()->partInfo->title, expParttitle, text}) + "</color>";
            PostMsg(progressMessage);
            co_yield YieldInstruction::Null();
        }
        i++;
    }
}

// ModuleScienceLab.cs:372
bool ModuleScienceLab::IsOperational() {
    int n = 0;
    for (auto& pcm : PartOf()->protoModuleCrew)
        if (pcm->HasEffect<Experience::Effects::ScienceSkill>()) n++;
    if (static_cast<float>(n) < Get<float>("crewsRequired")) return false;
    return true;
}

// ModuleScienceLab.cs:381
std::string ModuleScienceLab::GetInfo() {
    return Fmt("#autoLOC_239317", {dotnet::FormatSingleF(Get<float>("crewsRequired"), 1)}) +
           Fmt("#autoLOC_239318", {dotnet::FormatSingleF(Get<float>("dataStorage"), 1)});
}

// ModuleScienceLab.cs:386
void ModuleScienceLab::TransmissionErrorScreenMessage(const std::string& reason) { PostMsg(Fmt("#autoLOC_7003223", {reason})); }

// ModuleScienceLab.cs:391
void ModuleScienceLab::sendDataToComms() {
    if (transmittingData) {
        TransmissionErrorScreenMessage(Loc::Format("#autoLOC_239331"));
        return;
    }
    if (Get<float>("storedScience") < 1.0f) {
        TransmissionErrorScreenMessage(Loc::Format("#autoLOC_239337"));
        return;
    }
    IScienceDataTransmitter* bestTransmitter = ScienceUtil::GetBestTransmitter(*VesselOf());
    if (bestTransmitter != nullptr) {
        auto item = std::make_shared<ScienceData>(static_cast<float>(dotnet::Math::Floor(static_cast<double>(Get<float>("storedScience")))), 1.0f, 0.0f,
                                                  "sciencelab@" + VesselOf()->mainBody()->bodyName, Loc::Format("#autoLOC_239346"), true,
                                                  PartOf()->fields.flightID);
        std::vector<std::shared_ptr<ScienceData>> list;
        list.push_back(item);
        bestTransmitter->TransmitData(list);
        transmittingData = true;
    } else {
        if (ScienceEnvironment::commNetEnabled && ScienceEnvironment::commNetEnabled()) TransmissionErrorScreenMessage(Loc::Format("#autoLOC_239357"));
        else TransmissionErrorScreenMessage(Loc::Format("#autoLOC_239359"));
        updateModuleUI();
    }
}

// ModuleScienceLab.cs:426
void ModuleScienceLab::RecoverScienceLabs(ProtoVessel& protoVessel, KSP::UI::Screens::MissionRecoveryDialog* dialog) {
    if (ResearchAndDevelopment::Instance == nullptr) return;
    float num = 0.0f;
    int count = static_cast<int>(protoVessel.protoPartSnapshots.size());
    while (count-- > 0) {
        float num2 = 0.0f;
        ProtoPartSnapshot& protoPartSnapshot = *protoVessel.protoPartSnapshots[static_cast<size_t>(count)];
        int count2 = static_cast<int>(protoPartSnapshot.modules.size());
        while (count2-- > 0) {
            ProtoPartModuleSnapshot& m = *protoPartSnapshot.modules[static_cast<size_t>(count2)];
            if (!m.moduleName || *m.moduleName != "ModuleScienceLab") continue;
            std::vector<std::string> values = m.moduleValues->GetValues("storedScience");
            int num3 = static_cast<int>(values.size());
            while (num3-- > 0) {
                float result = 0.0f;
                if (dotnet::TryParseSingle(values[static_cast<size_t>(num3)], result) && result > Mathf::Epsilon) num2 = f32store(fx(num2) + fx(result));
            }
        }
        if (!(num2 <= 0.0f)) {
            num = f32store(fx(num) + fx(num2));
            if (!(dialog == nullptr)) {
                // ScienceSubjectWidget.Create(new ScienceSubject("", partInfo.title, 0, 0, 0), num2, num2, dialog); dialog.scienceEarned += num2: UI
                ResearchAndDevelopment::Instance->recoveredDataWidgets.push_back(
                    {std::make_shared<ScienceSubject>("", protoPartSnapshot.partInfo ? protoPartSnapshot.partInfo->title : std::string(), 0.0f, 0.0f, 0.0f), num2, num2});
            }
        }
    }
    if (num > 0.0f) ResearchAndDevelopment::Instance->AddScience(num, science_reason::VesselRecovery());
}

// ModuleScienceLab.cs:505
std::string ModuleScienceLab::GetModuleDisplayName() { return Loc::Format("#autoLoc_6003060"); }

// ----------------------------------------------------------------------------------------------------------------------------------
// ModuleScienceConverter

// ModuleScienceConverter.cs:43
ModuleScienceLab* ModuleScienceConverter::Lab() {
    if (lab_ == nullptr) lab_ = dynamic_cast<ModuleScienceLab*>(GetPart()->FindModuleOfClass("ModuleScienceLab"));
    return lab_;
}

// ModuleScienceConverter.cs:45
void ModuleScienceConverter::OnStart(StartState state) {
    Fields()["status"]->guiName = "#autoLOC_6001435";
    BaseConverter::OnStart(state);
}

// ModuleScienceConverter.cs:51
std::string ModuleScienceConverter::DataExpectationSummary(float dataAmount) {
    std::string text = Fmt("#autoLOC_6001023", {dotnet::FormatSingleF(dataAmount, 0), dotnet::FormatDoubleF(CalculateScienceAmount(dataAmount), 0)});
    std::string text2 = "n/a";
    const float stored = Lab()->Get<float>("dataStored");
    double time = 0.0 - dotnet::Math::Round(CalculateResearchTime(f32store(fx(dataAmount) + fx(stored))));
    if (fx(dataAmount) + fx(stored) > 0.0) {
        // CultureInfo.InvariantCulture.TextInfo.ToTitleCase(KSPUtil.PrintDateDelta(time, false, false, true).ToLowerInvariant())
        std::string s = KSPUtil::dateTimeFormatter().PrintDateDelta(time, false, false, true);
        bool startOfWord = true;
        for (char& c : s) {
            const unsigned char u = static_cast<unsigned char>(c);
            if (std::isalpha(u)) {
                c = static_cast<char>(startOfWord ? std::toupper(u) : std::tolower(u));
                startOfWord = false;
            } else {
                startOfWord = !std::isdigit(u) && u != '\'';
            }
        }
        text2 = s;
    }
    return Fmt("#autoLOC_6001024", {text, F32(f32store(fx(stored) + fx(dataAmount))), text2});
}

// ModuleScienceConverter.cs:63 (`return dataAmount * (float)scienceMultiplier;` in a double method: the float product is converted, not stored as float)
double ModuleScienceConverter::CalculateScienceAmount(float dataAmount) {
    return fx(dataAmount) * fx(static_cast<float>(Get<int>("scienceMultiplier")));
}

// ModuleScienceConverter.cs:68 (`(double)(day * scientists * ...)`: a cast to double is no float store)
double ModuleScienceConverter::CalculateScienceRate(float dataAmount) {
    float scientists = GetScientists();
    const float day = static_cast<float>(KSPUtil::dateTimeFormatter().Day());
    const double num = fx(day) * fx(scientists) * fx(dataAmount) * fx(Get<float>("dataProcessingMultiplier")) * fx(static_cast<float>(Get<int>("scienceMultiplier")));
    return num / dotnet::Math::Pow(10.0, static_cast<double>(Get<int>("researchTime")));
}

// ModuleScienceConverter.cs:74
double ModuleScienceConverter::CalculateResearchTime(float dataAmount) {
    double num = CalculateScienceRate(f32store(fx(Lab()->Get<float>("dataStored")) + fx(dataAmount)));
    return static_cast<double>(KSPUtil::dateTimeFormatter().Day()) * (static_cast<double>(dataAmount) / num);
}

// ModuleScienceConverter.cs:80
void ModuleScienceConverter::UpdateDisplayStrings() {
    ModuleScienceLab* lab = Lab();
    Set<std::string>("sciString", dotnet::FormatSingleF(lab->Get<float>("storedScience"), 3) + "/" + std::to_string(Get<int>("scienceCap")));
    Set<std::string>("datString", dotnet::FormatSingleF(lab->Get<float>("dataStored"), 3) + "/" + dotnet::FormatSingleF(lab->Get<float>("dataStorage"), 0));
    Set<std::string>("rateString", Fmt("#autoLOC_6001025", {dotnet::FormatDoubleF(CalculateScienceRate(lab->Get<float>("dataStored")), 4)}));
}

// ModuleScienceConverter.cs:92
ConversionRecipe* ModuleScienceConverter::PrepareRecipe(double) {
    recipe_ = ConversionRecipe();
    ResourceRatio r;
    r.FlowMode = ResourceFlowMode::ALL_VESSEL;
    r.Ratio = static_cast<double>(Get<float>("powerRequirement"));
    r.ResourceName = "ElectricCharge";
    r.DumpExcess = true;
    recipe_.Inputs().push_back(r);
    return &recipe_;
}

// ModuleScienceConverter.cs:109
void ModuleScienceConverter::PreProcessing() {
    if (GetPart() != nullptr && PartActionUIContains(GetPart())) UpdateDisplayStrings();
    BaseConverter::PostUpdateCleanup();
}

// ModuleScienceConverter.cs:118
void ModuleScienceConverter::PostProcess(const ConverterResults& result, double deltaTime) {
    ModuleScienceLab* lab = Lab();
    if (static_cast<double>(lab->Get<float>("dataStored")) < 1E-09) lab->Set<float>("dataStored", 0.0f);
    if (lab->Get<float>("dataStored") > lab->Get<float>("dataStorage")) lab->Set<float>("dataStored", lab->Get<float>("dataStorage"));
    if (lab->Get<float>("storedScience") > static_cast<float>(Get<int>("scienceCap"))) lab->Set<float>("storedScience", static_cast<float>(Get<int>("scienceCap")));
    float scientists = GetScientists();
    if (static_cast<double>(scientists) < 1E-09) {
        if (Get<bool>("IsActivated")) {
            StopResourceConverter();
            UpdateConverterStatus();
            const ProcessingVessel* v = Vessel();
            if (v != nullptr && v->isActiveVessel) ResourceProcessingEnvironment::screenMessages.push_back(Fmt("#autoLOC_237902", {GetPart()->partInfo->title}) + "</color>");
        }
        return;
    }
    double num = result.TimeFactor / deltaTime;
    if (num < 1E-09) {
        Set<std::string>("status", Loc::Format("#autoLOC_237911"));
        return;
    }
    double num2 = deltaTime * num * static_cast<double>(Get<float>("dataProcessingMultiplier")) * static_cast<double>(scientists) *
                  static_cast<double>(lab->Get<float>("dataStored")) / dotnet::Math::Pow(10.0, static_cast<double>(Get<int>("researchTime")));
    double num3 = static_cast<double>(Get<int>("scienceMultiplier")) * num2;
    if (num3 + static_cast<double>(lab->Get<float>("storedScience")) > static_cast<double>(Get<int>("scienceCap"))) {
        Set<std::string>("status", Loc::Format("#autoLOC_237928"));
        return;
    }
    Set<std::string>("status", Loc::Format("#autoLOC_237931"));
    lab->Set<float>("dataStored", f32store(fx(lab->Get<float>("dataStored")) - fx(static_cast<float>(num2))));
    lab->Set<float>("storedScience", f32store(fx(lab->Get<float>("storedScience")) + fx(static_cast<float>(num3))));
}

// ModuleScienceConverter.cs:164
float ModuleScienceConverter::GetScientists() {
    float num = 0.0f;
    Part* p = GetPart();
    int count = static_cast<int>(p->protoModuleCrew.size());
    while (count-- > 0) {
        auto& pcm = p->protoModuleCrew[static_cast<size_t>(count)];
        if (pcm->HasEffect<Experience::Effects::ScienceSkill>())
            num = f32store(fx(num) + (1.0 + fx(Get<float>("scientistBonus")) * fx(static_cast<float>(pcm->experienceLevel))));
    }
    return num;
}

// ModuleScienceConverter.cs:179
void ModuleScienceConverter::CacheLocalStrings() {}

// ----------------------------------------------------------------------------------------------------------------------------------
// ModuleEnviroSensor

// ModuleEnviroSensor.cs:45
void ModuleEnviroSensor::ToggleAction(KSPActionParam& param) {
    const bool sensorActive = Get<bool>("sensorActive");
    if (param.type != 0 && (param.type != static_cast<int>(KSPActionType::Toggle) || sensorActive)) Set<bool>("sensorActive", false);
    else Set<bool>("sensorActive", true);
}

// ModuleEnviroSensor.cs:58
void ModuleEnviroSensor::Toggle() { Set<bool>("sensorActive", !Get<bool>("sensorActive")); }

// ModuleEnviroSensor.cs:63
void ModuleEnviroSensor::OnAwake() {
    PartModule::OnAwake();
    Events().Bind("Toggle", BaseEvent::Handler([this] { Toggle(); }));
    Actions().Bind("ToggleAction", [this](KSPActionParam& p) { ToggleAction(p); });
    consumedResources.clear();
    for (size_t i = 0; i < resHandler.inputResources.size(); i++) consumedResources.push_back(LookupDefinition(resHandler.inputResources[i]->name));
}

// ModuleEnviroSensor.cs:80
void ModuleEnviroSensor::OnLoad(const ConfigNode& node) {
    Part* p = dynamic_cast<Part*>(part);
    const bool noPrefab = p == nullptr || p->partInfo == nullptr || p->partInfo->partPrefab == nullptr;
    if (resHandler.inputResources.empty() && (node.HasValue("resourceName") || node.HasValue("powerConsumption") || noPrefab)) {
        std::string value = "ElectricCharge";
        float value2 = 0.0075f;
        node.TryGetValue("resourceName", value);
        node.TryGetValue("powerConsumption", value2);
        auto moduleResource = std::make_shared<ModuleResource>();
        moduleResource->name = value;
        moduleResource->title = KSPUtil::PrintModuleName(value);
        moduleResource->id = MonoStringHash(value);
        moduleResource->rate = static_cast<double>(value2);
        resHandler.inputResources.push_back(moduleResource);
    }
}

// ModuleEnviroSensor.cs:97
void ModuleEnviroSensor::FixedUpdate() {
    Part* p = dynamic_cast<Part*>(part);
    const std::string off = Loc::Format("#autoLOC_237153");
    if (Get<bool>("sensorActive") && p != nullptr && p->fields.started) {
        std::string readout = StrField(*this, "readoutInfo");
        if (resHandler.UpdateModuleResourceInputs(readout, 1.0, 0.9, true)) {
            Set<std::string>("readoutInfo", readout);
            if (!PartActionUIContains(p)) return;
            Vessel* vessel = p->vessel;
            const std::string sensorTypeName = EnumName("sensorType");
            const SensorType sensorType = sensorTypeName == "TEMP" ? SensorType::TEMP : sensorTypeName == "GRAV" ? SensorType::GRAV : sensorTypeName == "ACC" ? SensorType::ACC : SensorType::PRES;
            switch (sensorType) {
            case SensorType::TEMP:
                Set<std::string>("readoutInfo", dotnet::FormatDoubleF(p->fields.temperature, 2) + " " + Loc::Format("#autoLOC_7001406"));
                break;
            case SensorType::GRAV:
                if (vessel->orbit().altitude <= vessel->orbit().referenceBody->Radius * 3.0) {
                    const double g = ScienceModuleEnvironment::geeForceAtPosition ? ScienceModuleEnvironment::geeForceAtPosition(*p) : vessel->geeForce;
                    Set<std::string>("readoutInfo", Fmt("#autoLOC_237120", {dotnet::FormatDoubleF(g, 2)}));
                } else {
                    Set<std::string>("readoutInfo", Loc::Format("#autoLOC_6004058"));
                }
                break;
            case SensorType::ACC:
                Set<std::string>("readoutInfo", dotnet::FormatDoubleF(vessel->geeForce, 3) + Loc::Format("#autoLOC_7001413"));
                break;
            case SensorType::PRES: {
                double staticPressurekPa = vessel->staticPressurekPa;
                if (staticPressurekPa > 0.0001) Set<std::string>("readoutInfo", dotnet::FormatDoubleF(staticPressurekPa, 4) + " " + Loc::Format("#autoLOC_7001410"));
                else if (staticPressurekPa > 0.0) Set<std::string>("readoutInfo", Loc::Format("#autoLOC_6004059"));
                else Set<std::string>("readoutInfo", Loc::Format("#autoLOC_6004060"));
                break;
            }
            }
        } else {
            Set<std::string>("readoutInfo", readout);
            if (Get<std::string>("readoutInfo") != off) Set<std::string>("readoutInfo", off);
        }
    } else {
        Set<std::string>("readoutInfo", off);
    }
}

// ModuleEnviroSensor.cs:156
// (deferred: PartModuleUtil.PrintResourceRequirements text, see the ModuleResourceHandler comment in module_resource.hpp)
std::string ModuleEnviroSensor::GetInfo() { return std::string(); }

// ModuleEnviroSensor.cs:161
std::string ModuleEnviroSensor::GetModuleDisplayName() { return Loc::Format("#autoLoc_6003040"); }

// ModuleEnviroSensor.cs:166
void ModuleEnviroSensor::CacheLocalStrings() {}

// ----------------------------------------------------------------------------------------------------------------------------------
// ModuleSampleContainer

void ModuleSampleContainer::OnAwake() {
    PartModule::OnAwake();
    Events().Bind("Discard", BaseEvent::Handler([this] { Discard(); }));
}

// ModuleSampleContainer.cs:14
void ModuleSampleContainer::OnStart(StartState state) {
    if (state != StartState::Editor && state != StartState::None) Events()["Discard"]->active = Get<bool>("sampleHeld");
}

// ModuleSampleContainer.cs:22
void ModuleSampleContainer::TransferSample(const std::shared_ptr<PlanetarySample>& transferee) {
    Set<std::string>("heldSampleName", transferee->sampleName);
    Set<float>("heldSampleMass", transferee->sampleMass);
    sample = transferee;
    Set<bool>("sampleHeld", true);
    Events()["Discard"]->active = true;
}

// ModuleSampleContainer.cs:32
void ModuleSampleContainer::Discard() {
    sample = nullptr;
    Set<float>("heldSampleMass", 0.0f);
    Set<std::string>("heldSampleName", "N/A");
    Set<bool>("sampleHeld", false);
    Events()["Discard"]->active = false;
}

// ----------------------------------------------------------------------------------------------------------------------------------
namespace detail {
void RegisterScienceModules() {
    ModuleFactory::Register("ModuleScienceExperiment", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleScienceExperiment>(c)); });
    ModuleFactory::Register("ModuleScienceContainer", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleScienceContainer>(c)); });
    ModuleFactory::Register("ModuleScienceLab", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleScienceLab>(c)); });
    ModuleFactory::Register("ModuleScienceConverter", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleScienceConverter>(c)); });
    ModuleFactory::Register("ModuleEnviroSensor", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleEnviroSensor>(c)); });
    ModuleFactory::Register("ModuleSampleContainer", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleSampleContainer>(c)); });
}
}  // namespace detail

}  // namespace ksp
