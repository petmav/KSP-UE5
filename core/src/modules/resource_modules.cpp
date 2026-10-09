// WP-15 resource part modules: see ksp/modules/resource_modules.hpp.
#include "ksp/modules/resource_modules.hpp"
#include "ksp/dotnet/mono_float.hpp"

#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/parts/part_module_data.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/resources/resource_flow_host.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/save_environment.hpp"

namespace ksp {

using GE = events::GameEvents;

namespace {
NullReferenceException Nre() { return NullReferenceException("Object reference not set to an instance of an object"); }
Part* PartOf(PartModule& m) { return dynamic_cast<Part*>(m.part); }
bool IsNullOrEmpty(const std::string& s) { return s.empty(); }
}  // namespace

// ---- ModuleResourceAutoShiftState -----------------------------------------------------------------------------------------

ModuleResourceAutoShiftState::ModuleResourceAutoShiftState() : PartModule(*GetPartModuleClass("ModuleResourceAutoShiftState")) {}

// ModuleResourceAutoShiftState.cs:56 (UI_FloatRange limits, guiNames and scene visibility are PAW state: not modelled)
void ModuleResourceAutoShiftState::OnStart(StartState state) {
    PartModule::OnStart(state);
    const std::string affectedResourceName = Get<std::string>("affectedResourceName");
    if (IsNullOrEmpty(affectedResourceName)) {
        destroyed = true;  // Object.Destroy(this)
        return;
    }
    Fields()["resourceShutOffStartUpUsePercent"]->OnValueModified.push_back([this](const ModuleValue&) { ModifyAmountPercent(); });
    Fields()["resourceShutOffAmount"]->OnValueModified.push_back([this](const ModuleValue&) { ModifyShutOffAmount(); });
    Fields()["resourceShutOffPercent"]->OnValueModified.push_back([this](const ModuleValue&) { ModifyShutOffPercent(); });
    Fields()["resourceStartUpAmount"]->OnValueModified.push_back([this](const ModuleValue&) { ModifyStartUpAmount(); });
    Fields()["resourceStartUpPercent"]->OnValueModified.push_back([this](const ModuleValue&) { ModifyStartUpPercent(); });
    const PartResourceLibrary* lib = PartResourceLibrary::Instance();
    const PartResourceDefinition* definition = lib ? lib->GetDefinition(affectedResourceName) : nullptr;
    (void)definition;
    Part* myPart = PartOf(*this);
    if (ResourceFlowEnvironment::LoadedSceneIsFlight()) {
        if (myPart == nullptr || myPart->vessel == nullptr) throw Nre();
        IResourceVessel* host = ResourceFlowEnvironment::Host(myPart->vessel);
        if (host == nullptr) throw Nre();
        const std::vector<Part*>& parts = host->ResourceParts();
        for (size_t i = 0; i < parts.size(); i++) {
            Part* p = parts[i];
            for (int j = 0; j < p->Resources.Count(); j++) {
                if (p->Resources.At(j).info->name() == affectedResourceName) {
                    partResource = &p->Resources.At(j);
                    PartModule* m = myPart->Modules().GetModule(Get<std::string>("affectedModuleName"));
                    if (m == nullptr) throw Nre();
                    resource = nullptr;
                    for (auto& r : m->resHandler.inputResources)
                        if (r->name == affectedResourceName) {
                            resource = r.get();
                            break;
                        }
                    break;
                }
            }
            if (partResource == nullptr) throw Nre();  // partResource.resourceName on null
            if (!IsNullOrEmpty(partResource->resourceName)) break;
        }
        if (resource != nullptr && !IsNullOrEmpty(partResource->resourceName)) {
            resource->autoStateShifter = this;
            if (Get<bool>("resourceShutOffHandler")) {
                resource->shutOffHandler = true;
                resource->shutOffStartUpUsePercent = Get<bool>("resourceShutOffStartUpUsePercent");
                resource->shutOffAmount = Get<float>("resourceShutOffAmount");
                resource->shutOffPercent = Get<float>("resourceShutOffPercent");
                Set<std::string>("moduleAutoShiftName", Localizer::Format("#autoLOC_6005054", {partResource->info->displayName()}));
            }
            if (Get<bool>("resourceStartUpHandler")) {
                resource->startUpHandler = true;
                resource->shutOffStartUpUsePercent = Get<bool>("resourceShutOffStartUpUsePercent");
                resource->startUpAmount = Get<float>("resourceStartUpAmount");
                resource->startUpPercent = Get<float>("resourceStartUpPercent");
                Set<std::string>("moduleAutoShiftName", Localizer::Format("#autoLOC_6005054", {partResource->info->displayName()}));
            }
            ModifyAmountPercent();
        }
    } else if (ResourceFlowEnvironment::LoadedSceneIsEditor()) {
        ModifyAmountPercent();
    }
}

// ModuleResourceAutoShiftState.cs:163 (Mono float rule: `resourceShutOffPercent + 1f` is evaluated in double and stored as float)
void ModuleResourceAutoShiftState::ModifyStartUpPercent() {
    if (Get<float>("resourceStartUpPercent") <= Get<float>("resourceShutOffPercent"))
        Set<float>("resourceStartUpPercent", f32store(fx(Get<float>("resourceShutOffPercent")) + fx(1.0f)));
    if (resource != nullptr) resource->startUpPercent = Get<float>("resourceStartUpPercent");
}

// ModuleResourceAutoShiftState.cs:175
void ModuleResourceAutoShiftState::ModifyShutOffAmount() {
    if (Get<float>("resourceShutOffAmount") >= Get<float>("resourceStartUpAmount"))
        Set<float>("resourceShutOffAmount", f32store(fx(Get<float>("resourceStartUpAmount")) - fx(1.0f)));
    if (resource != nullptr) resource->shutOffAmount = Get<float>("resourceShutOffAmount");
}

// ModuleResourceAutoShiftState.cs:187
void ModuleResourceAutoShiftState::ModifyStartUpAmount() {
    if (Get<float>("resourceStartUpAmount") <= Get<float>("resourceShutOffAmount"))
        Set<float>("resourceStartUpAmount", f32store(fx(Get<float>("resourceShutOffAmount")) + fx(1.0f)));
    if (resource != nullptr) resource->startUpAmount = Get<float>("resourceStartUpAmount");
}

// ModuleResourceAutoShiftState.cs:199
void ModuleResourceAutoShiftState::ModifyShutOffPercent() {
    if (Get<float>("resourceShutOffPercent") >= Get<float>("resourceStartUpPercent"))
        Set<float>("resourceShutOffPercent", f32store(fx(Get<float>("resourceStartUpPercent")) - fx(1.0f)));
    if (resource != nullptr) resource->shutOffPercent = Get<float>("resourceShutOffPercent");
}

// ModuleResourceAutoShiftState.cs:211 (the scene-dependent field visibility is UI)
void ModuleResourceAutoShiftState::ModifyAmountPercent() {
    if (resource != nullptr) resource->shutOffStartUpUsePercent = Get<bool>("resourceShutOffStartUpUsePercent");
}

// ModuleResourceAutoShiftState.cs:233 (moduleAutoShiftNameField.SetSceneVisibility(Flight, state))
void ModuleResourceAutoShiftState::ToggleWarningVisibility(bool state) { warningVisible = state; }

// ---- ModuleToggleCrossfeed ------------------------------------------------------------------------------------------------

// ModuleToggleCrossfeed.cs:43
void ModuleToggleCrossfeed::ToggleEvent() {
    Set<bool>("crossfeedStatus", !Get<bool>("crossfeedStatus"));
    Part* p = PartOf(*this);
    if ((ResourceFlowEnvironment::LoadedSceneIsEditor() && Get<bool>("eventPropagatesInEditor")) ||
        (ResourceFlowEnvironment::LoadedSceneIsFlight() && Get<bool>("eventPropagatesInFlight"))) {
        if (p == nullptr) throw Nre();
        int count = static_cast<int>(p->symmetryCounterparts.size());
        while (count-- > 0) {
            Part* part = p->symmetryCounterparts[static_cast<size_t>(count)];
            if (part != p) {
                PartModule* m = part->Modules().GetModule([](PartModule& pm) { return dynamic_cast<ModuleToggleCrossfeed*>(&pm) != nullptr; });
                ModuleToggleCrossfeed* module = dynamic_cast<ModuleToggleCrossfeed*>(m);
                if (module != nullptr) {
                    module->Set<bool>("crossfeedStatus", Get<bool>("crossfeedStatus"));
                    module->UpdateCrossfeed(false);
                }
            }
        }
    }
    UpdateCrossfeed(true);
}

// ModuleToggleCrossfeed.cs:67
void ModuleToggleCrossfeed::ToggleAction(KSPActionParam& /*param*/) {
    bool flag = Get<bool>("eventPropagatesInEditor");
    bool flag2 = Get<bool>("eventPropagatesInFlight");
    Set<bool>("eventPropagatesInEditor", false);
    Set<bool>("eventPropagatesInFlight", false);
    ToggleEvent();
    Set<bool>("eventPropagatesInEditor", flag);
    Set<bool>("eventPropagatesInFlight", flag2);
}

// ModuleToggleCrossfeed.cs:78
void ModuleToggleCrossfeed::ToggleAction(int actionType) {
    if (actionType == 1) Deactivate();
    else Activate();
}

// ModuleToggleCrossfeed.cs:91
void ModuleToggleCrossfeed::EnableAction(KSPActionParam&) { Activate(); }

// ModuleToggleCrossfeed.cs:96
void ModuleToggleCrossfeed::Activate() {
    Set<bool>("crossfeedStatus", true);
    UpdateCrossfeed(true);
}

// ModuleToggleCrossfeed.cs:103
void ModuleToggleCrossfeed::DisableAction(KSPActionParam&) { Deactivate(); }

// ModuleToggleCrossfeed.cs:108
void ModuleToggleCrossfeed::Deactivate() {
    Set<bool>("crossfeedStatus", false);
    UpdateCrossfeed(true);
}

// ModuleToggleCrossfeed.cs:114
void ModuleToggleCrossfeed::Start() { UpdateCrossfeed(false); }

// ModuleToggleCrossfeed.cs:119 (while the prefab compiles part.partInfo is null; a Part without partInfo.partPrefab behaves the same)
void ModuleToggleCrossfeed::OnAwake() {
    PartModule::OnAwake();
    toggleE = Events()["ToggleEvent"];
    toggleA = Actions()["ToggleAction"];
    enableA = Actions()["EnableAction"];
    disableA = Actions()["DisableAction"];
    Events().Bind("ToggleEvent", BaseEvent::Handler([this] { ToggleEvent(); }));
    Actions().Bind("ToggleAction", [this](KSPActionParam& p) { ToggleAction(p); });
    Actions().Bind("EnableAction", [this](KSPActionParam& p) { EnableAction(p); });
    Actions().Bind("DisableAction", [this](KSPActionParam& p) { DisableAction(p); });
    Part* p = PartOf(*this);
    if (part != nullptr && (p == nullptr || p->partInfo == nullptr || p->partInfo->partPrefab == nullptr)) defaultCrossfeedStatus = part->fields.fuelCrossFeed;
}

// ModuleToggleCrossfeed.cs:132
void ModuleToggleCrossfeed::OnLoad(const ConfigNode& node) {
    PartModule::OnLoad(node);
    UpdateCrossfeed(false);
}

// ModuleToggleCrossfeed.cs:138
void ModuleToggleCrossfeed::OnStart(StartState state) {
    PartModule::OnStart(state);
    bool flag = true;
    if (CrossfeedRequiresTech()) flag = CrossfeedHasTech();
    const bool moduleIsEnabled = Get<bool>("moduleIsEnabled");
    if (!flag || !moduleIsEnabled) Set<bool>("crossfeedStatus", defaultCrossfeedStatus);
    UpdateCrossfeed(true);
    toggleE->guiActiveEditor = flag && Get<bool>("toggleEditor") && moduleIsEnabled;
    toggleE->guiActive = toggleA->active = enableA->active = disableA->active = flag && Get<bool>("toggleFlight") && moduleIsEnabled;
}

// ModuleToggleCrossfeed.cs:155 (`fireEvent` is not read by the C#: the state-change event fires whenever the value changed)
void ModuleToggleCrossfeed::UpdateCrossfeed(bool /*fireEvent*/) {
    if (part == nullptr) throw Nre();
    const bool crossfeedStatus = Get<bool>("crossfeedStatus");
    const bool fuelCrossFeed = part->fields.fuelCrossFeed;
    part->fields.fuelCrossFeed = crossfeedStatus;
    if (fuelCrossFeed != crossfeedStatus) {
        if (Part* p = PartOf(*this)) GE::onPartCrossfeedStateChange.Fire(p);
    }
    if (toggleE == nullptr) toggleE = Events()["ToggleEvent"];
    if (toggleA == nullptr) toggleA = Actions()["ToggleAction"];
    if (enableA == nullptr) enableA = Actions()["EnableAction"];
    if (disableA == nullptr) disableA = Actions()["DisableAction"];
    toggleE->SetGuiName(Get<std::string>(crossfeedStatus ? "disableText" : "enableText"));
    toggleA->SetGuiName(Get<std::string>("toggleText"));
    enableA->SetGuiName(Get<std::string>("enableText"));
    disableA->SetGuiName(Get<std::string>("disableText"));
}

bool ModuleToggleCrossfeed::CrossfeedToggleableEditor() { return Get<bool>("toggleEditor"); }  // :176
bool ModuleToggleCrossfeed::CrossfeedToggleableFlight() { return Get<bool>("toggleFlight"); }  // :181
bool ModuleToggleCrossfeed::CrossfeedRequiresTech() { return !Get<std::string>("techRequired").empty(); }  // :186
std::string ModuleToggleCrossfeed::CrossfeedTech() { return Get<std::string>("techRequired"); }  // :191

// ModuleToggleCrossfeed.cs:196
bool ModuleToggleCrossfeed::CrossfeedHasTech() {
    Game* g = SaveEnvironment::Current().currentGame;
    if (g != nullptr && (g->Mode == Game_Modes::CAREER || g->Mode == Game_Modes::SCIENCE_SANDBOX))
        return technologyAvailable ? technologyAvailable(Get<std::string>("techRequired")) : true;
    return true;
}

// ModuleToggleCrossfeed.cs:205
std::string ModuleToggleCrossfeed::GetModuleDisplayName() { return Localizer::Format("#autoLOC_236032"); }

// Object.Instantiate copies the public field defaultCrossfeedStatus with the KSPFields
void ModuleToggleCrossfeed::CopySerializedFrom(const PartModule& from) {
    PartModule::CopySerializedFrom(from);
    if (const auto* o = dynamic_cast<const ModuleToggleCrossfeed*>(&from)) defaultCrossfeedStatus = o->defaultCrossfeedStatus;
}

// ---- CModuleFuelLine ------------------------------------------------------------------------------------------------------

// CModuleFuelLine.cs:12 (target.fuelLookupTargets.Add(part); the stack icon colour is UI)
void CModuleFuelLine::OnTargetSet(Part* target) {
    Part* p = PartOf(*this);
    target->fuelLookupTargets.push_back(p);
    GE::onPartFuelLookupStateChange.Fire(GE::HostedFromToAction<bool, Part*>(false, target, p));
}

// CModuleFuelLine.cs:19
void CModuleFuelLine::OnTargetLost() { CloseFuelLine(); }

// CModuleFuelLine.cs:24 (base.target != null)
void CModuleFuelLine::CloseFuelLine() {
    Part* t = compoundTarget ? target() : nullptr;
    if (t != nullptr) {
        Part* p = PartOf(*this);
        auto& v = t->fuelLookupTargets;
        auto it = std::find(v.begin(), v.end(), p);  // List.Remove: the first occurrence
        if (it != v.end()) v.erase(it);
        GE::onPartFuelLookupStateChange.Fire(GE::HostedFromToAction<bool, Part*>(false, t, p));
    }
}

// CModuleFuelLine.cs:43 (compoundPart.maxLength.ToString("0.0"))
std::string CModuleFuelLine::GetInfo() {
    if (compoundPart == nullptr) throw Nre();
    return Localizer::Format("#autoLOC_216749", {dotnet::FormatSingleF(compoundPart->compoundFields.maxLength, 1)}) + cacheAutoLOC_216750;
}

// CModuleFuelLine.cs:63
std::string CModuleFuelLine::GetModuleDisplayName() { return cacheAutoLOC_216757; }

// CModuleFuelLine.cs:68
void CModuleFuelLine::CacheLocalStrings() {
    cacheAutoLOC_216750 = Localizer::Format("#autoLOC_216750");
    cacheAutoLOC_216757 = Localizer::Format("#autoLOC_216757");
}

// ---- EVAResourceContainer -------------------------------------------------------------------------------------------------

// EVAResourceContainer.cs:28
void EVAResourceContainer::OnStart(StartState) {
    Part* p = PartOf(*this);
    if (p == nullptr) throw Nre();
    resource = p->Resources.Get(MonoStringHash(Get<std::string>("resourceName")));
    updateModuleUI();
}

// EVAResourceContainer.cs:35
void EVAResourceContainer::OnDestroy() { transferring = false; }

// EVAResourceContainer.cs:49
void EVAResourceContainer::updateModuleUI() {
    if (resource == nullptr) throw Nre();  // resource.amount.ToString("0")
    const float interactionRange = Get<float>("interactionRange");
    BaseEvent* get = Events()["GetResourceExternalEvent"];
    get->active = resource != nullptr && resource->amount > 0.0;
    get->SetGuiName(Get<std::string>("resourcePullActionName") + " (" + dotnet::FormatDoubleF(resource->amount, 0) + ")");
    get->unfocusedRange = interactionRange;
    BaseEvent* put = Events()["PutResourceExternalEvent"];
    put->active = resource != nullptr && resource->amount < resource->maxAmount;
    put->SetGuiName(Get<std::string>("resourcePushActionName") + " (" + dotnet::FormatDoubleF(resource->amount, 0) + ")");
    put->unfocusedRange = interactionRange;
    BaseEvent* stop = Events()["StopTransfer"];
    stop->active = false;
    stop->SetGuiName(Get<std::string>("resourceStopActionName"));
    stop->unfocusedRange = interactionRange;
}

// EVAResourceContainer.cs:63 (starts the transfer coroutine: the host calls TransferStep every frame while `transferring`)
void EVAResourceContainer::GetResourceExternalEvent(Part* activeVesselRoot) {
    if (resource == nullptr) throw Nre();
    PartResource* partResource = activeVesselRoot->Resources.Get(resource->info->id());
    if (partResource != nullptr) {
        Events()["GetResourceExternalEvent"]->active = false;
        Events()["PutResourceExternalEvent"]->active = false;
        Events()["StopTransfer"]->active = true;
        transferring = true;
        transferSrc = resource;
        transferDest = partResource;
    }
}

// EVAResourceContainer.cs:78
void EVAResourceContainer::PutResourceExternalEvent(Part* activeVesselRoot) {
    if (resource == nullptr) throw Nre();
    PartResource* partResource = activeVesselRoot->Resources.Get(resource->info->id());
    if (partResource != nullptr) {
        Events()["GetResourceExternalEvent"]->active = false;
        Events()["PutResourceExternalEvent"]->active = false;
        Events()["StopTransfer"]->active = true;
        transferring = true;
        transferSrc = partResource;
        transferDest = resource;
    }
}

// EVAResourceContainer.cs:93
void EVAResourceContainer::StopTransfer() {
    transferring = false;
    updateModuleUI();
}

// EVAResourceContainer.cs:99 transferResources: one loop iteration. `sqrDistance` = (src.part.position - dest.part.position).sqrMagnitude
bool EVAResourceContainer::TransferStep(PartResource& src, PartResource& dest, double& transferRate, float maxDist, double sqrDistance) {
    if (!(dest.amount < dest.maxAmount && src.amount > 0.0 && !(sqrDistance >= static_cast<double>(maxDist * maxDist)))) return false;
    transferRate = dotnet::Math::Min(transferRate, src.amount);
    transferRate = dotnet::Math::Min(transferRate, dest.maxAmount - dest.amount);
    src.part->TransferResource(src, 0.0 - transferRate, dest.part);
    dest.part->TransferResource(dest, transferRate, src.part);
    return true;
}

// ---- registration -----------------------------------------------------------------------------------------------------------

namespace detail {
void RegisterResourceModules() {
    ModuleRegistrar<ModuleToggleCrossfeed>("ModuleToggleCrossfeed");
    ModuleRegistrar<ModuleResourceAutoShiftState>("ModuleResourceAutoShiftState");
    ModuleRegistrar<CModuleFuelLine>("CModuleFuelLine");
    ModuleRegistrar<EVAResourceContainer>("EVAResourceContainer");
}
}  // namespace detail

}  // namespace ksp
