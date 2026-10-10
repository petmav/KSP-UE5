// WP-24: ModuleCommand.cs, ModuleSAS.cs, ModuleRemoteController.cs (see ksp/modules/module_command.hpp for the scope and the seams).
#include "ksp/modules/module_command.hpp"

#include <algorithm>

#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/kerbals/experience.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/modules/module_reaction_wheel.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/resources/resource_flow_host.hpp"
#include "ksp/save/crew.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/game_parameters.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/vessel/vessel.hpp"
#include "ksp/vessel/vessel_autopilot.hpp"

namespace ksp {

void CommandEnvironment::Reset() {
    commNetEnabled = nullptr;
    connection = nullptr;
    flightPaused = nullptr;
    partInPAW = nullptr;
    setPartReferenceTransform = nullptr;
    renameVessel = nullptr;
    controlPointVisualsEnabled = false;
    controlPointArrowLength = 2.5f;
    quaternionEuler = nullptr;
}

namespace {
const PartResourceDefinition* LookupDefinition(const std::string& name) {
    const PartResourceLibrary* lib = PartResourceLibrary::Instance();
    return lib ? lib->GetDefinition(std::string_view(name)) : nullptr;
}
Part* AsPart(PartPrefab* p) { return dynamic_cast<Part*>(p); }
std::string Str(int v) { return std::to_string(v); }
}  // namespace

Vessel* ModuleCommand::VesselPtr() const {
    Part* p = AsPart(part);
    return p ? p->vessel : nullptr;
}

// ModuleCommand.cs:130 Connection (vessel.connection; NullReferenceException without a vessel)
std::optional<CommNetLinkState> ModuleCommand::Connection() const {
    Vessel* v = VesselPtr();
    if (v == nullptr) throw NullReferenceException();
    return CommandEnvironment::connection ? CommandEnvironment::connection(*v) : std::nullopt;
}

// ModuleCommand.cs:142
bool ModuleCommand::SignalRequired() const {
    const bool enabled = CommandEnvironment::commNetEnabled ? CommandEnvironment::commNetEnabled() : false;
    if (enabled) {
        Game* g = SaveEnvironment::Current().currentGame;  // HighLogic.CurrentGame
        if (g == nullptr || !g->Parameters) throw NullReferenceException();
        return g->Parameters->CustomParams<CommNet::CommNetParams>().requireSignalForControl;
    }
    return false;
}

// ModuleCommand.cs:154
bool ModuleCommand::IsHibernating() const {
    if (!Get<bool>("hibernation")) {
        if (Get<bool>("hibernateOnWarp")) {
            if (TimeWarp::CurrentRate() > 1.0f && TimeWarp::WarpMode() == TimeWarp::Modes::HIGH) return true;
            Part* p = AsPart(part);
            return p != nullptr && p->fields.packed;
        }
        return false;
    }
    return true;
}

// ModuleCommand.cs:174
const std::string& ModuleCommand::ActiveControlPointName() {
    const ModuleValue* v = Value("activeControlPointName");
    static const std::string empty;
    const std::string* s = v ? std::get_if<std::string>(v) : nullptr;
    return s ? *s : empty;
}

// Unity Instantiate: the serialized members (controlPointsList and cpArrowLength; the colours and arrows are rendering)
void ModuleCommand::CopySerializedFrom(const PartModule& from) {
    PartModule::CopySerializedFrom(from);
    if (const auto* r = dynamic_cast<const ModuleCommand*>(&from)) {
        cpArrowLength = r->cpArrowLength;
        if (r->controlPointsList) {
            controlPointsList.emplace();
            for (const auto& cp : *r->controlPointsList) controlPointsList->push_back(std::make_shared<ControlPoint>(*cp));
        } else {
            controlPointsList.reset();
        }
    }
}

// ModuleCommand.cs:177
void ModuleCommand::ToggleControlPointVisual() {
    if (ControlPointsExist()) {
        showControlPointVisual = !showControlPointVisual;
        if (!showControlPointVisual) Events()["ToggleControlPointVisual"]->SetGuiName(Localizer::Format("#autoLOC_8003232"));
        else Events()["ToggleControlPointVisual"]->SetGuiName(Localizer::Format("#autoLOC_8003233"));
    }
}

// ModuleCommand.cs:258
void ModuleCommand::ChangeControlPoint() {
    if (!controlPoints) {
        Part* p = AsPart(part);
        log::Error("[ModuleCommand]: There are no alternate control points for " + (p && p->partInfo ? p->partInfo->title : std::string()));
        return;
    }
    const std::vector<std::string>& keys = controlPoints->KeysList();
    auto it = std::find(keys.begin(), keys.end(), ActiveControlPointName());
    int num = it == keys.end() ? -1 : static_cast<int>(it - keys.begin());
    num++;
    if (num >= controlPoints->Count()) num = 0;
    SetControlPoint(keys[static_cast<size_t>(num)]);
}

// ModuleCommand.cs:275
void ModuleCommand::SetControlPoint(std::string newControlPointName) {
    if (!controlPoints) {
        Part* p = AsPart(part);
        log::Error("[ModuleCommand]: There are no alternate control points for " + (p && p->partInfo ? p->partInfo->title : std::string()));
        return;
    }
    const auto& keys = controlPoints->KeysList();
    auto has = [&](const std::string& k) { return std::find(keys.begin(), keys.end(), k) != keys.end(); };
    if (!has(newControlPointName)) {
        if (!has("_default")) {
            log::Warning("[ModuleCommand]: Control Point name does not exist and cant find _default: " + newControlPointName);
            return;
        }
        log::Warning("[ModuleCommand]: Control Point name does not exist. using _default: " + newControlPointName);
        newControlPointName = "_default";
    }
    std::shared_ptr<ControlPoint> controlPoint = (*controlPoints)[newControlPointName];
    if (ResourceFlowEnvironment::LoadedSceneIsFlight() && CommandEnvironment::setPartReferenceTransform)
        CommandEnvironment::setPartReferenceTransform(*this, *controlPoint);
    controlPointDisplayName_ = Localizer::Format(controlPoint->displayName);
    Set<std::string>("activeControlPointName", newControlPointName);
    activeControlPoint_ = controlPoint;
    UpdateControlPointEvent();
    if (Part* p = AsPart(part)) GameEvents::OnControlPointChanged.Fire(p, controlPoint.get());
}

// ModuleCommand.cs:304
void ModuleCommand::UpdateControlPointEvent() {
    const bool flag = controlPoints.has_value() && controlPoints->Count() > 1;
    Events()["ChangeControlPoint"]->active = flag;
    if (flag)
        Events()["ChangeControlPoint"]->SetGuiName(Localizer::Format("#autoLOC_8004190", {Localizer::Format("#autoLOC_6011002"), controlPointDisplayName_}));
}

// ModuleCommand.cs:363 (the CommNet registration of Start / OnDestroy / OnNetworkInitialised is WP-39's)
// ModuleCommand.cs:363
void ModuleCommand::OnAwake() {
    PartModule::OnAwake();
    Events().Bind("ToggleControlPointVisual", BaseEvent::Handler([this] { ToggleControlPointVisual(); }));
    Events().Bind("ChangeControlPoint", BaseEvent::Handler([this] { ChangeControlPoint(); }));
    Events().Bind("MakeReference", BaseEvent::Handler([this] { MakeReference(); }));
    Events().Bind("RenameVessel", BaseEvent::Handler([this] { RenameVessel(); }));
    Actions().Bind("MakeReferenceToggle", [this](KSPActionParam& a) { MakeReferenceToggle(a); });
    Actions().Bind("HibernateToggle", [this](KSPActionParam& a) { HibernateToggle(a); });
    consumedResources.clear();
    for (size_t i = 0; i < resHandler.inputResources.size(); i++) consumedResources.push_back(LookupDefinition(resHandler.inputResources[i]->name));
    if (part == nullptr) throw NullReferenceException();
    part->fields.isControlSource = static_cast<int32_t>(ControlLevel::FULL);
    const bool hasHibernation = Get<bool>("hasHibernation");
    Actions()["HibernateToggle"]->active = hasHibernation;
    Fields()["hibernation"]->guiActive = hasHibernation;
    Fields()["hibernation"]->guiActiveEditor = hasHibernation;
    Fields()["hibernateOnWarp"]->guiActive = hasHibernation;
    Fields()["hibernateOnWarp"]->guiActiveEditor = hasHibernation;
    if (controlPointsList.has_value() && !controlPoints.has_value()) {
        controlPoints.emplace();
        auto controlPoint = std::make_shared<ControlPoint>("_default", Get<std::string>("defaultControlPointDisplayName"), nullptr, Vector3f::zero());
        controlPoint->hasTransform = true;
        controlPoint->isPartTransform = true;
        controlPointDisplayName_ = Localizer::Format(Get<std::string>("defaultControlPointDisplayName"));
        controlPoints->Add(controlPoint->name, controlPoint);
        for (size_t j = 0; j < controlPointsList->size(); j++) controlPoints->Add((*controlPointsList)[j]->name, (*controlPointsList)[j]);
    }
    cpArrowLength = CommandEnvironment::controlPointArrowLength;
    const bool vis = CommandEnvironment::controlPointVisualsEnabled && ControlPointsExist();
    BaseEvent* e = Events()["ToggleControlPointVisual"];
    e->active = e->guiActive = e->guiActiveEditor = vis;
}

// Unity Quaternion.Euler (ZXY order; native Unity, single precision, the trig through ksp::dotnet)
Quaternionf ModuleCommand::DefaultQuaternionEuler(const Vector3f& euler) {
    const float hx = euler.x * 0.0174532924f * 0.5f, hy = euler.y * 0.0174532924f * 0.5f, hz = euler.z * 0.0174532924f * 0.5f;
    const float cx = Mathf::Cos(hx), sx = Mathf::Sin(hx), cy = Mathf::Cos(hy), sy = Mathf::Sin(hy), cz = Mathf::Cos(hz), sz = Mathf::Sin(hz);
    return {cy * sx * cz + sy * cx * sz, sy * cx * cz - cy * sx * sz, cy * cx * sz - sy * sx * cz, cy * cx * cz + sy * sx * sz};
}

// ModuleCommand.cs:406
void ModuleCommand::OnStart(StartState) {
    const bool hasHibernation = Get<bool>("hasHibernation");
    Actions()["HibernateToggle"]->active = hasHibernation;
    Fields()["hibernation"]->guiActive = hasHibernation;
    Fields()["hibernation"]->guiActiveEditor = hasHibernation;
    Fields()["hibernateOnWarp"]->guiActive = hasHibernation;
    Fields()["hibernateOnWarp"]->guiActiveEditor = hasHibernation;
    if (controlPoints.has_value() && (ResourceFlowEnvironment::LoadedSceneIsEditor() || ResourceFlowEnvironment::LoadedSceneIsFlight())) {
        for (int i = 0; i < controlPoints->Count(); i++) {
            const std::string key = controlPoints->KeysList()[static_cast<size_t>(i)];
            ControlPoint& cp = *(*controlPoints)[key];
            cp.hasTransform = true;
            cp.isPartTransform = false;
            cp.localRotation = CommandEnvironment::quaternionEuler ? CommandEnvironment::quaternionEuler(cp.orientation) : DefaultQuaternionEuler(cp.orientation);
        }
    }
    SetControlPoint(ActiveControlPointName());
    UpdateControlPointEvent();
    const bool vis = CommandEnvironment::controlPointVisualsEnabled && ControlPointsExist();
    BaseEvent* e = Events()["ToggleControlPointVisual"];
    e->active = e->guiActive = e->guiActiveEditor = vis;
    if (Part* p = AsPart(part))
        for (size_t j = 0; j < p->protoModuleCrew.size(); j++)
            p->protoModuleCrew[j]->persistentID = SaveEnvironment::Current().persistentIds.GetUniquepersistentId();
}

// ModuleCommand.cs:438
void ModuleCommand::OnLoad(const ConfigNode& node) {
    if (controlPointsList.has_value()) return;
    std::vector<ConfigNode*> nodes = node.GetNodes("CONTROLPOINT");
    if (!nodes.empty()) {
        controlPointsList.emplace();
        for (size_t i = 0; i < nodes.size(); i++) {
            auto controlPoint = std::make_shared<ControlPoint>();
            controlPoint->Load(*nodes[i]);
            controlPointsList->push_back(controlPoint);
        }
    }
}

// ModuleCommand.cs:457
void ModuleCommand::OnSave(ConfigNode&) {}

// ModuleCommand.cs:461
void ModuleCommand::FixedUpdate() { UpdateControlState(); }

// ModuleCommand.cs:462
void ModuleCommand::UpdateControlState() {
    const bool paused = CommandEnvironment::flightPaused ? CommandEnvironment::flightPaused() : false;
    if (ResourceFlowEnvironment::LoadedSceneIsFlight() && !paused && VesselPtr() != nullptr) {
        localVesselControlState_ = UpdateControlSourceState();
        if ((localVesselControlState_ & VesselControlState::Full) > VesselControlState::None) {
            part->fields.isControlSource = static_cast<int32_t>(ControlLevel::FULL);
        } else if ((localVesselControlState_ & VesselControlState::Partial) > VesselControlState::None) {
            if ((localVesselControlState_ & VesselControlState::Kerbal) > VesselControlState::None)
                part->fields.isControlSource = static_cast<int32_t>(ControlLevel::PARTIAL_MANNED);
            else
                part->fields.isControlSource = static_cast<int32_t>(ControlLevel::PARTIAL_UNMANNED);
        } else {
            part->fields.isControlSource = static_cast<int32_t>(ControlLevel::NONE);
        }
    }
}

// ModuleCommand.cs:504
VesselControlState ModuleCommand::UpdateControlSourceState() {
    commCapable_ = false;
    const bool isHibernating = IsHibernating();
    const int minimumCrew = Get<int>("minimumCrew");
    std::string status = Get<std::string>("controlSrcStatusText");
    const bool resourcesOk = resHandler.UpdateModuleResourceInputs(status, isHibernating ? Get<double>("hibernationMultiplier") : 1.0, 0.9, true);
    Set<std::string>("controlSrcStatusText", status);
    if (!resourcesOk) {
        moduleState_ = ModuleControlState::NotEnoughResources;
        if (minimumCrew > 0) return VesselControlState::Kerbal;
        return VesselControlState::Probe;
    }
    totalCrewCount_ = 0;
    crewCount_ = 0;
    pilots_ = 0;
    Part* p = AsPart(part);
    if (p == nullptr) throw NullReferenceException();
    int count = static_cast<int>(p->protoModuleCrew.size());
    while (count-- > 0) {
        ProtoCrewMember& protoCrewMember = *p->protoModuleCrew[static_cast<size_t>(count)];
        if (protoCrewMember.HasEffect<Experience::Effects::FullVesselControlSkill>() && !protoCrewMember.inactive()) pilots_++;
        if (protoCrewMember.type() != ProtoCrewMember::KerbalType::Tourist) {
            totalCrewCount_++;
            if (!protoCrewMember.inactive()) crewCount_++;
        }
    }
    const bool flag = Get<bool>("requiresPilot") && pilots_ == 0 && p->fields.CrewCapacity > 0;
    const bool inPAW = CommandEnvironment::partInPAW ? CommandEnvironment::partInPAW(*this) : false;
    if (crewCount_ < minimumCrew) {
        if (totalCrewCount_ < static_cast<int>(p->protoModuleCrew.size())) {
            if (inPAW) Set<std::string>("controlSrcStatusText", Localizer::Format("#autoLOC_7000069", {Str(crewCount_), Str(minimumCrew)}));
            moduleState_ = ModuleControlState::TouristCrew;
            return VesselControlState::Kerbal;
        }
        if (inPAW) Set<std::string>("controlSrcStatusText", Localizer::Format("#autoLOC_7000068", {Str(crewCount_), Str(minimumCrew)}));
        moduleState_ = ModuleControlState::NotEnoughCrew;
        return VesselControlState::Kerbal;
    }
    commCapable_ = true;
    const bool commNet = CommandEnvironment::commNetEnabled ? CommandEnvironment::commNetEnabled() : false;
    const bool remoteControl = Get<bool>("remoteControl");
    if (commNet) {
        const std::optional<CommNetLinkState> link = Connection();
        const bool flag2 = link.has_value() && link->isConnected;
        if (!remoteControl && flag) {
            if (Get<std::string>("controlSrcStatusText") != "No Pilot") Set<std::string>("controlSrcStatusText", Localizer::Format("#autoLOC_217408"));
            moduleState_ = ModuleControlState::PartialManned;
            return VesselControlState::KerbalPartial;
        }
        if (!flag2) {
            if (flag) {
                if (Get<std::string>("controlSrcStatusText") != "No Pilot") Set<std::string>("controlSrcStatusText", Localizer::Format("#autoLOC_217417"));
                moduleState_ = ModuleControlState::PartialManned;
                return VesselControlState::KerbalPartial;
            }
            if (totalCrewCount_ == 0) {
                if (SignalRequired() || !remoteControl) {
                    if (Get<std::string>("controlSrcStatusText") != "No Telemetry") Set<std::string>("controlSrcStatusText", Localizer::Format("#autoLOC_217429"));
                    moduleState_ = ModuleControlState::NoControlPoint;
                    return VesselControlState::Probe;
                }
                if (Get<bool>("requiresTelemetry")) {
                    if (Get<std::string>("controlSrcStatusText") != "Partial Control") Set<std::string>("controlSrcStatusText", Localizer::Format("#autoLOC_217437"));
                    moduleState_ = ModuleControlState::PartialProbe;
                    return VesselControlState::ProbePartial;
                }
            }
        }
    }
    if (isHibernating) {
        Set<std::string>("controlSrcStatusText", Localizer::Format("#autoLOC_217448"));
        if (minimumCrew > 0) {
            moduleState_ = ModuleControlState::PartialManned;
            return VesselControlState::KerbalPartial;
        }
        moduleState_ = ModuleControlState::PartialProbe;
        return VesselControlState::ProbePartial;
    }
    Set<std::string>("controlSrcStatusText", Localizer::Format("#autoLOC_217464"));
    if (minimumCrew > 0) {
        moduleState_ = ModuleControlState::Nominal;
        return VesselControlState::KerbalFull;
    }
    moduleState_ = ModuleControlState::Nominal;
    return VesselControlState::ProbeFull;
}

// ModuleCommand.cs:625
void ModuleCommand::MakeReferenceToggle(KSPActionParam&) { MakeReference(); }

// ModuleCommand.cs:625
void ModuleCommand::MakeReference() {
    Vessel* v = VesselPtr();
    if (v == nullptr) throw NullReferenceException();
    v->SetReferenceTransform(AsPart(part));
}

// ModuleCommand.cs:637
void ModuleCommand::RenameVessel() {
    Vessel* v = VesselPtr();
    if (v == nullptr) throw NullReferenceException();
    if (CommandEnvironment::renameVessel) CommandEnvironment::renameVessel(*v);
}

// ModuleCommand.cs:643
void ModuleCommand::HibernateToggle(KSPActionParam&) { Set<bool>("hibernation", !Get<bool>("hibernation")); }

// ModuleCommand.cs:667
ControlPoint* ModuleCommand::GetControlPoint(const std::string& name) {
    if (controlPoints.has_value() && controlPoints->Count() > 0) {
        const auto& keys = controlPoints->KeysList();
        if (std::find(keys.begin(), keys.end(), name) != keys.end()) return (*controlPoints)[name].get();
        return nullptr;
    }
    return nullptr;
}

// ModuleCommand.cs:680
bool ModuleCommand::ControlPointsExist() const { return controlPoints.has_value() && controlPoints->Count() > 0; }

// ======================================================= ModuleSAS

ModuleSAS::~ModuleSAS() {
    // OnDestroy (ModuleSAS.cs:104): the providers leave the part values (the Part outlives its modules)
    if (registeredOn_ != nullptr) {
        registeredOn_->partValues.AutopilotSkill.Remove(this, &ModuleSAS::GetValue);
        registeredOn_->partValues.AutopilotSASSkill.Remove(this, &ModuleSAS::GetValue);
    }
}

// Unity Instantiate: the public / SerializeField members (ModuleSAS.cs:12, :25)
void ModuleSAS::CopySerializedFrom(const PartModule& from) {
    PartModule::CopySerializedFrom(from);
    if (const auto* r = dynamic_cast<const ModuleSAS*>(&from)) {
        targetSASServiceLevel = r->targetSASServiceLevel;
        standaloneOperational = r->standaloneOperational;
    }
}

// ModuleSAS.cs:47
void ModuleSAS::OnStart(StartState) {
    targetSASServiceLevel = Get<int>("SASServiceLevel");
    Part* p = AsPart(part);
    if (p != nullptr && p->vessel != nullptr) {
        p->partValues.AutopilotSkill.Add(this, &ModuleSAS::GetValue);
        p->partValues.AutopilotSASSkill.Add(this, &ModuleSAS::GetValue);
        registeredOn_ = p;
    }
    if (!Get<bool>("standalone")) {
        const int idx = Get<int>("CommandModuleIndex");
        if (p == nullptr) throw NullReferenceException();
        if (idx != -1) {
            cmdModule_ = idx >= 0 && static_cast<size_t>(idx) < p->Modules().Count() ? dynamic_cast<ModuleCommand*>(p->Modules()[static_cast<size_t>(idx)]) : nullptr;
        } else {
            cmdModule_ = nullptr;
            for (size_t i = 0; i < p->Modules().Count(); i++)
                if (auto* c = dynamic_cast<ModuleCommand*>(p->Modules()[i])) {
                    cmdModule_ = c;
                    break;
                }
        }
        if (cmdModule_ == nullptr) log::Warning("[ModuleSAS]: No ModuleCommand found in " + p->gameObjectName + ". SAS services won't be available.");
        Fields()["standaloneToggle"]->guiActive = false;
        Fields()["standaloneStateText"]->guiActive = false;
        Fields()["standaloneToggle"]->guiActiveEditor = false;
        Fields()["standaloneStateText"]->guiActiveEditor = false;
    } else {
        const bool moduleIsEnabled = Get<bool>("moduleIsEnabled");
        Fields()["standaloneToggle"]->guiActiveEditor = moduleIsEnabled;
        Fields()["standaloneToggle"]->guiActive = moduleIsEnabled;
        Fields()["standaloneStateText"]->guiActive = moduleIsEnabled;
    }
}

// ModuleSAS.cs:92
int ModuleSAS::GetValue() {
    const bool moduleIsEnabled = Get<bool>("moduleIsEnabled");
    if (cmdModule_ != nullptr && moduleIsEnabled) {
        const bool ok = Get<bool>("RequireCrew") ? (cmdModule_->ModuleState() == ModuleCommand::ModuleControlState::Nominal)
                                                 : (cmdModule_->ModuleState() != ModuleCommand::ModuleControlState::NotEnoughResources);
        if (ok) return Get<int>("SASServiceLevel");
        return -1;
    }
    if (Get<bool>("standalone")) {
        if (!standaloneOperational) return -1;
        return Get<int>("SASServiceLevel");
    }
    return -1;
}

// ModuleSAS.cs:113
void ModuleSAS::FixedUpdate() {
    Part* p = AsPart(part);
    const bool moduleIsEnabled = Get<bool>("moduleIsEnabled");
    const bool flight = ResourceFlowEnvironment::LoadedSceneIsFlight();
    const bool started = p != nullptr && p->fields.started;
    if (moduleIsEnabled && flight && started) Set<int>("SASServiceLevel", ApplySASServiceLevelAdjustments(targetSASServiceLevel));
    if (!moduleIsEnabled || !Get<bool>("standalone") || !flight || !started) return;
    if (Get<bool>("standaloneToggle")) {
        if (p->vessel == nullptr) throw NullReferenceException();
        if (p->vessel->Autopilot().Enabled()) {
            std::string text = Localizer::Format("#autoLOC_257237");
            standaloneOperational = resHandler.UpdateModuleResourceInputs(text, 1.0, 0.9, true);
            Set<std::string>("standaloneStateText", text);
        } else {
            standaloneOperational = true;
            Set<std::string>("standaloneStateText", Localizer::Format("#autoLOC_6001373"));
        }
    } else {
        standaloneOperational = false;
        Set<std::string>("standaloneStateText", Localizer::Format("#autoLOC_6001374"));
    }
}

// ModuleSAS.cs:186
int ModuleSAS::ApplySASServiceLevelAdjustments(int serviceLevel) {
    for (size_t i = 0; i < adjusters_.size(); i++) serviceLevel = adjusters_[i](serviceLevel);
    return serviceLevel;
}

// ======================================================= ModuleRemoteController

void ModuleRemoteController::OnAwake() {
    PartModule::OnAwake();
    Events().Bind("Toggle", BaseEvent::Handler([this] { Toggle(); }));
}

// ModuleRemoteController.cs:6
void ModuleRemoteController::Toggle() {
    const std::string v = Get<std::string>("controllerActive");
    if (v == "Inactive") Set<std::string>("controllerActive", "Active");
    else if (v == "Active") Set<std::string>("controllerActive", "Inactive");
}

namespace detail {
void RegisterControlModules() {
    ModuleFactory::Register("ModuleCommand", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleCommand>(c)); });
    ModuleFactory::Register("ModuleSAS", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleSAS>(c)); });
    ModuleFactory::Register("ModuleRemoteController",
                            [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleRemoteController>(c)); });
    ModuleFactory::Register("ModuleReactionWheel",
                            [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleReactionWheel>(c)); });
}
}  // namespace detail

}  // namespace ksp
