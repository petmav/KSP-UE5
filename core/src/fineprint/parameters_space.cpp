// WP-38 slice 2: FinePrint/Contracts/Parameters/{AsteroidParameter,CometParameter,CometDetectionParameter,CometScienceParameter,
// ConstructionParameter,RepairPartParameter}.cs.
#include <algorithm>

#include "ksp/contracts/agents.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/fineprint/parameters.hpp"
#include "ksp/parts/part_loader.hpp"
#include "ksp/science/science.hpp"
#include "param_common.hpp"

namespace ksp::Contracts::Parameters {

using namespace ksp::fpcommon;
using GE = events::GameEvents;
namespace L = Lite;

namespace {
// FlightGlobals.FindVessel(uint persistentId, out Vessel): the vessel registered under that persistentId
Vessel* FindVesselByPersistentId(uint32_t id) {
    for (Vessel* v : FlightRegistry::Vessels)
        if (v->persistentId == id) return v;
    return nullptr;
}
// FlightGlobals.GetBodyByName
CelestialBody* GetBodyByName(const std::string& name) {
    for (CelestialBody* b : FlightRegistry::Bodies())
        if (b->bodyName == name) return b;
    return nullptr;
}
// vessel.DiscoveryInfo.size.Value (KnowledgeItem<UntrackedObjectClass>.Value: the class letter once the presence is known)
std::string DiscoverySizeValue(Vessel& v) {
    DiscoveryInfo& di = v.GetDiscoveryInfo();
    if (di.HaveKnowledgeAbout(DiscoveryLevels::Presence)) return std::string(1, static_cast<char>('A' + static_cast<int>(di.objectSize())));  // UntrackedObjectClass A..I
    return Localizer::Format("#autoLOC_6002223");
}
bool ListHas(const std::vector<int32_t>& l, int32_t v) { return std::find(l.begin(), l.end(), v) != l.end(); }
std::string Upper(std::string s) {
    for (char& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return s;
}
int ModuleSeed(PartModule* m) { return m->Get<int32_t>("seed"); }
}  // namespace

// ---- AsteroidParameter ----------------------------------------------------------------------------------------------------------------

// AsteroidParameter.cs:29
AsteroidParameter::AsteroidParameter() : forStation(false), asteroidClass("A") {}
// AsteroidParameter.cs:35
AsteroidParameter::AsteroidParameter(const std::string& size, bool forStation_) : forStation(forStation_), asteroidClass(Upper(size)) {}
// AsteroidParameter.cs:43
std::string AsteroidParameter::GetHashString() { return I(L::SuperSeed(*Root())) + ID(); }
// AsteroidParameter.cs:48
std::string AsteroidParameter::GetTitle() {
    if (forStation) return Fmt("#autoLOC_282737", {asteroidClass});
    return Fmt("#autoLOC_282739", {asteroidClass});
}
// AsteroidParameter.cs:57
std::string AsteroidParameter::GetNotes() { return Fmt("#autoLOC_282744", {Root()->Agent()->Title(), asteroidClass}); }
// AsteroidParameter.cs:62
void AsteroidParameter::OnRegister() {
    SetDisableOnStateChange(false);
    eventsAdded = false;
    if (Root()->ContractState() == Contract::State::Active) {
        GE::onPartCouple.Add(this, &AsteroidParameter::OnDock);
        GE::onVesselWasModified.Add(this, &AsteroidParameter::VesselModified);
        GE::onPartDie.Add(this, &AsteroidParameter::PartModified);
        eventsAdded = true;
    }
}
// AsteroidParameter.cs:77
void AsteroidParameter::OnUnregister() {
    if (eventsAdded) {
        GE::onPartCouple.Remove(this, &AsteroidParameter::OnDock);
        GE::onVesselWasModified.Remove(this, &AsteroidParameter::VesselModified);
        GE::onPartDie.Remove(this, &AsteroidParameter::PartModified);
    }
}
// AsteroidParameter.cs:91
void AsteroidParameter::OnReset() {
    SetIncomplete();
    dirtyVessel = true;
}
// AsteroidParameter.cs:97
void AsteroidParameter::VesselModified(Vessel* v) {
    if (v == FlightRegistry::ActiveVessel) dirtyVessel = true;
}
// AsteroidParameter.cs:105
void AsteroidParameter::PartModified(Part* p) {
    if (p->vessel == FlightRegistry::ActiveVessel) dirtyVessel = true;
}
// AsteroidParameter.cs:113
void AsteroidParameter::OnDock(GE::FromToAction<Part*, Part*> action) {
    if (!L::FlightIsReady(Root()->ContractState(), Contract::State::Active, true)) return;
    std::vector<PartModule*> list = action.from->vessel->FindPartModulesImplementing([](PartModule& m) { return m.IsA("ModuleAsteroid"); });
    int count = static_cast<int>(list.size());
    while (count-- > 0) {
        PartModule* moduleAsteroid = list[static_cast<size_t>(count)];
        Vessel* mv = static_cast<Part*>(moduleAsteroid->part)->vessel;
        if (mv->GetDiscoveryInfo().Level() != DiscoveryLevels::Owned) {
            if (DiscoverySizeValue(*mv) == asteroidClass) {
                asteroidSeeds.push_back(ModuleSeed(moduleAsteroid));
                L::FinePrintEnvironment::Post(Fmt("#autoLOC_282809", {Root()->Agent()->Title()}), 5.0f, L::ScreenMessageStyle::UPPER_LEFT);
            } else {
                L::FinePrintEnvironment::Post(Fmt("#autoLOC_282812", {Root()->Agent()->Title()}), 5.0f, L::ScreenMessageStyle::UPPER_LEFT);
            }
            continue;
        }
        bool flag = false;
        int count2 = static_cast<int>(asteroidSeeds.size());
        while (count2-- > 0)
            if (ModuleSeed(moduleAsteroid) == asteroidSeeds[static_cast<size_t>(count2)]) flag = true;
        if (!flag)
            L::FinePrintEnvironment::Post(Fmt("#autoLOC_282825", {Root()->Agent()->Title()}), 5.0f, L::ScreenMessageStyle::UPPER_LEFT);
        else
            L::FinePrintEnvironment::Post(Fmt("#autoLOC_282827", {Root()->Agent()->Title()}), 5.0f, L::ScreenMessageStyle::UPPER_LEFT);
    }
    if (action.from->vessel == FlightRegistry::ActiveVessel || action.to->vessel == FlightRegistry::ActiveVessel) dirtyVessel = true;
}
// AsteroidParameter.cs:171
void AsteroidParameter::OnSave(ConfigNode& node) {
    node.AddValue("forStation", forStation);
    node.AddValue("asteroidClass", asteroidClass);
    L::SystemUtilities::SaveNodeList(&node, "seedList", asteroidSeeds);
}
// AsteroidParameter.cs:178
void AsteroidParameter::OnLoad(ConfigNode& node) {
    L::LoadNode(node, "AsteroidParameter", "forStation", forStation, false);
    L::LoadNode(node, "AsteroidParameter", "asteroidClass", asteroidClass, std::string("A"));
    L::SystemUtilities::LoadNodeList(&node, "AsteroidParameter", "seedList", asteroidSeeds);
}
// AsteroidParameter.cs:185
void AsteroidParameter::OnUpdate() {
    if (!L::FlightIsReady(Root()->ContractState(), Contract::State::Active, true)) return;
    Vessel* av = FlightRegistry::ActiveVessel;
    const int num = av->loaded ? static_cast<int>(av->Parts().size()) : static_cast<int>(av->protoVessel->protoPartSnapshots.size());
    if (activePartCount != num) dirtyVessel = true;
    if (dirtyVessel) validVessel = VesselTowingAsteroid(av);
    if (State() == ParameterState::Incomplete) {
        if (validVessel)
            successCounter++;
        else
            successCounter = 0;
        if (successCounter >= 5) SetComplete();
    }
    if (State() == ParameterState::Complete && !validVessel) SetIncomplete();
}
// AsteroidParameter.cs:213
bool AsteroidParameter::VesselTowingAsteroid(Vessel* v) {
    dirtyVessel = false;
    activePartCount = v->loaded ? static_cast<int>(v->Parts().size()) : static_cast<int>(v->protoVessel->protoPartSnapshots.size());
    std::vector<PartModule*> list = v->FindPartModulesImplementing([](PartModule& m) { return m.IsA("ModuleAsteroid"); });
    int count = static_cast<int>(list.size());
    while (count-- > 0) {
        int count2 = static_cast<int>(asteroidSeeds.size());
        while (count2-- > 0)
            if (ModuleSeed(list[static_cast<size_t>(count)]) == asteroidSeeds[static_cast<size_t>(count2)]) return true;
    }
    return false;
}

// ---- CometParameter -------------------------------------------------------------------------------------------------------------------

// CometParameter.cs:29
CometParameter::CometParameter() : forStation(false), cometClass("A") {}
// CometParameter.cs:35
CometParameter::CometParameter(const std::string& size, bool forStation_) : forStation(forStation_), cometClass(Upper(size)) {}
// CometParameter.cs:43
std::string CometParameter::GetHashString() { return I(L::SuperSeed(*Root())) + ID(); }
// CometParameter.cs:48
std::string CometParameter::GetTitle() {
    if (forStation) return Fmt("#autoLOC_282737", {cometClass});
    return Fmt("#autoLOC_282739", {cometClass});
}
// CometParameter.cs:57
std::string CometParameter::GetNotes() { return Fmt("#autoLOC_282744", {Root()->Agent()->Title(), cometClass}); }
// CometParameter.cs:62
void CometParameter::OnRegister() {
    SetDisableOnStateChange(false);
    eventsAdded = false;
    if (Root()->ContractState() == Contract::State::Active) {
        GE::onPartCouple.Add(this, &CometParameter::OnDock);
        GE::onVesselWasModified.Add(this, &CometParameter::VesselModified);
        GE::onPartDie.Add(this, &CometParameter::PartModified);
        eventsAdded = true;
    }
}
// CometParameter.cs:77
void CometParameter::OnUnregister() {
    if (eventsAdded) {
        GE::onPartCouple.Remove(this, &CometParameter::OnDock);
        GE::onVesselWasModified.Remove(this, &CometParameter::VesselModified);
        GE::onPartDie.Remove(this, &CometParameter::PartModified);
    }
}
// CometParameter.cs:91
void CometParameter::OnReset() {
    SetIncomplete();
    dirtyVessel = true;
}
// CometParameter.cs:97
void CometParameter::VesselModified(Vessel* v) {
    if (v == FlightRegistry::ActiveVessel) dirtyVessel = true;
}
// CometParameter.cs:105
void CometParameter::PartModified(Part* p) {
    if (p->vessel == FlightRegistry::ActiveVessel) dirtyVessel = true;
}
// CometParameter.cs:113
void CometParameter::OnDock(GE::FromToAction<Part*, Part*> action) {
    if (!L::FlightIsReady(Root()->ContractState(), Contract::State::Active, true)) return;
    std::vector<PartModule*> list = action.from->vessel->FindPartModulesImplementing([](PartModule& m) { return m.IsA("ModuleComet"); });
    int count = static_cast<int>(list.size());
    while (count-- > 0) {
        PartModule* moduleComet = list[static_cast<size_t>(count)];
        Vessel* mv = static_cast<Part*>(moduleComet->part)->vessel;
        if (mv->GetDiscoveryInfo().Level() != DiscoveryLevels::Owned) {
            if (DiscoverySizeValue(*mv) == cometClass) {
                cometSeeds.push_back(ModuleSeed(moduleComet));
                L::FinePrintEnvironment::Post(Fmt("#autoLOC_282809", {Root()->Agent()->Title()}), 5.0f, L::ScreenMessageStyle::UPPER_LEFT);
            } else {
                L::FinePrintEnvironment::Post(Fmt("#autoLOC_282812", {Root()->Agent()->Title()}), 5.0f, L::ScreenMessageStyle::UPPER_LEFT);
            }
            continue;
        }
        bool flag = false;
        int count2 = static_cast<int>(cometSeeds.size());
        while (count2-- > 0)
            if (ModuleSeed(moduleComet) == cometSeeds[static_cast<size_t>(count2)]) flag = true;
        if (!flag)
            L::FinePrintEnvironment::Post(Fmt("#autoLOC_282825", {Root()->Agent()->Title()}), 5.0f, L::ScreenMessageStyle::UPPER_LEFT);
        else
            L::FinePrintEnvironment::Post(Fmt("#autoLOC_282827", {Root()->Agent()->Title()}), 5.0f, L::ScreenMessageStyle::UPPER_LEFT);
    }
    if (action.from->vessel == FlightRegistry::ActiveVessel || action.to->vessel == FlightRegistry::ActiveVessel) dirtyVessel = true;
}
// CometParameter.cs:171
void CometParameter::OnSave(ConfigNode& node) {
    node.AddValue("forStation", forStation);
    node.AddValue("cometClass", cometClass);
    L::SystemUtilities::SaveNodeList(&node, "seedList", cometSeeds);
}
// CometParameter.cs:178
void CometParameter::OnLoad(ConfigNode& node) {
    L::LoadNode(node, "CometParameter", "forStation", forStation, false);
    L::LoadNode(node, "CometParameter", "cometClass", cometClass, std::string("A"));
    L::SystemUtilities::LoadNodeList(&node, "CometParameter", "seedList", cometSeeds);
}
// CometParameter.cs:185
void CometParameter::OnUpdate() {
    if (!L::FlightIsReady(Root()->ContractState(), Contract::State::Active, true)) return;
    Vessel* av = FlightRegistry::ActiveVessel;
    const int num = av->loaded ? static_cast<int>(av->Parts().size()) : static_cast<int>(av->protoVessel->protoPartSnapshots.size());
    if (activePartCount != num) dirtyVessel = true;
    if (dirtyVessel) validVessel = VesselTowingComet(av);
    if (State() == ParameterState::Incomplete) {
        if (validVessel)
            successCounter++;
        else
            successCounter = 0;
        if (successCounter >= 5) SetComplete();
    }
    if (State() == ParameterState::Complete && !validVessel) SetIncomplete();
}
// CometParameter.cs:213
bool CometParameter::VesselTowingComet(Vessel* v) {
    dirtyVessel = false;
    activePartCount = v->loaded ? static_cast<int>(v->Parts().size()) : static_cast<int>(v->protoVessel->protoPartSnapshots.size());
    std::vector<PartModule*> list = v->FindPartModulesImplementing([](PartModule& m) { return m.IsA("ModuleComet"); });
    int count = static_cast<int>(list.size());
    while (count-- > 0) {
        int count2 = static_cast<int>(cometSeeds.size());
        while (count2-- > 0)
            if (ModuleSeed(list[static_cast<size_t>(count)]) == cometSeeds[static_cast<size_t>(count2)]) return true;
    }
    return false;
}

// ---- CometDetectionParameter ----------------------------------------------------------------------------------------------------------

// CometDetectionParameter.cs:20
CometDetectionParameter::CometDetectionParameter() : cometCountDetected(0) {}
// CometDetectionParameter.cs:25
CometDetectionParameter::CometDetectionParameter(int cometCountRequired_, const std::string& sentinelPartName_)
    : cometCountRequired(cometCountRequired_), cometCountDetected(0), sentinelPartName(sentinelPartName_) {}
// CometDetectionParameter.cs:33
std::string CometDetectionParameter::GetHashString() { return I(L::SuperSeed(*Root())) + ID(); }
// CometDetectionParameter.cs:38
std::string CometDetectionParameter::GetTitle() { return Fmt("#autoLOC_6011147", {I(cometCountRequired)}); }
// CometDetectionParameter.cs:43
std::string CometDetectionParameter::GetNotes() { return std::string(Scene() == GameScenes::SPACECENTER ? "\n" : "") + Fmt("#autoLOC_8003406"); }
// CometDetectionParameter.cs:48
void CometDetectionParameter::OnRegister() {
    SetDisableOnStateChange(false);
    GE::onSentinelCometDetected.Add(this, &CometDetectionParameter::CometDetected);
}
// CometDetectionParameter.cs:54
void CometDetectionParameter::OnUnregister() { GE::onSentinelCometDetected.Remove(this, &CometDetectionParameter::CometDetected); }
// CometDetectionParameter.cs:59
void CometDetectionParameter::CometDetected(Vessel* data) {
    (void)data;
    cometCountDetected++;
    RemainingDiscoveries = cometCountRequired - cometCountDetected;
    if (RemainingDiscoveries > 0) {
        const std::string text = Fmt("#autoLOC_8003397", {I(cometCountDetected), I(cometCountRequired), Root()->Agent()->Title()});
        L::FinePrintEnvironment::Post(text, L::StringUtilities::CalculateReadDuration(text), L::ScreenMessageStyle::UPPER_CENTER);
    } else {
        const std::string text2 = Fmt("#autoLOC_8003398", {Root()->Agent()->Title()});
        L::FinePrintEnvironment::Post(text2, L::StringUtilities::CalculateReadDuration(text2), L::ScreenMessageStyle::UPPER_CENTER);
        SetComplete();
        Root()->Complete();
    }
}
// CometDetectionParameter.cs:75
void CometDetectionParameter::OnReset() { SetIncomplete(); }
// CometDetectionParameter.cs:80
void CometDetectionParameter::OnSave(ConfigNode& node) {
    node.AddValue("cometCount", static_cast<int32_t>(cometCountDetected));
    node.AddValue("cometCountRequired", static_cast<int32_t>(cometCountRequired));
}
// CometDetectionParameter.cs:86
void CometDetectionParameter::OnLoad(ConfigNode& node) {
    L::LoadNode(node, "CometDetectionParameter", "cometCount", cometCountDetected, 1);
    L::LoadNode(node, "CometDetectionParameter", "cometCountRequired", cometCountRequired, 1);
}

// ---- CometScienceParameter ------------------------------------------------------------------------------------------------------------

// CometScienceParameter.cs:32
CometScienceParameter::CometScienceParameter(uint32_t cometId, const std::string& cometName_, const std::string& scienceSubjectId_)
    : cometPersistentId(cometId), cometName(cometName_), scienceSubjectId(scienceSubjectId_), scienceDataCollected(false) {}
// CometScienceParameter.cs:40
std::string CometScienceParameter::GetHashString() { return cometName + dotnet::FormatUInt64(cometPersistentId); }
// CometScienceParameter.cs:45
std::string CometScienceParameter::GetTitle() { return Fmt("#autoLOC_8012045", {cometName}); }
// CometScienceParameter.cs:50
void CometScienceParameter::OnLoad(ConfigNode& node) {
    node.TryGetValue("cometName", cometName);
    node.TryGetValue("cometPersistentId", cometPersistentId);
    node.TryGetValue("scienceSubjectId", scienceSubjectId);
    node.TryGetValue("scienceDataCollected", scienceDataCollected);
}
// CometScienceParameter.cs:58
void CometScienceParameter::OnSave(ConfigNode& node) {
    node.AddValue("cometName", cometName);
    node.AddValue("cometPersistentId", cometPersistentId);
    node.AddValue("scienceSubjectId", scienceSubjectId);
    node.AddValue("scienceDataCollected", scienceDataCollected);
}
// CometScienceParameter.cs:66
void CometScienceParameter::OnRegister() {
    GE::onVesselPersistentIdChanged.Add(this, &CometScienceParameter::VesselIdChanged);
    GE::OnScienceRecieved.Add(this, &CometScienceParameter::OnScience);
    GE::OnTriggeredDataTransmission.Add(this, &CometScienceParameter::OnTriggeredScience);
    GE::onVesselRename.Add(this, &CometScienceParameter::OnVesselRename);
    GE::OnExperimentDeployed.Add(this, &CometScienceParameter::OnExperimentDeployed);
}
// CometScienceParameter.cs:75
void CometScienceParameter::OnUnregister() {
    GE::onVesselPersistentIdChanged.Remove(this, &CometScienceParameter::VesselIdChanged);
    GE::OnScienceRecieved.Remove(this, &CometScienceParameter::OnScience);
    GE::OnTriggeredDataTransmission.Remove(this, &CometScienceParameter::OnTriggeredScience);
    GE::onVesselRename.Remove(this, &CometScienceParameter::OnVesselRename);
    GE::OnExperimentDeployed.Remove(this, &CometScienceParameter::OnExperimentDeployed);
}
// CometScienceParameter.cs:84
void CometScienceParameter::OnExperimentDeployed(ScienceData* experimentData) {
    if (experimentData->subjectID == scienceSubjectId) scienceDataCollected = true;
}
// CometScienceParameter.cs:92
void CometScienceParameter::OnVesselRename(GE::HostedFromToAction<Vessel*, std::string> hostedFromTo) {
    if (hostedFromTo.host != nullptr && !scienceDataCollected && cometPersistentId == hostedFromTo.host->persistentId) {
        cometName = hostedFromTo.to;
        const std::string typeName = L::FinePrintEnvironment::cometTypeName ? L::FinePrintEnvironment::cometTypeName(*hostedFromTo.host) : std::string();
        scienceSubjectId = "cometSample_" + typeName + "@SunInSpaceHigh_" + cometName;
    }
}
// CometScienceParameter.cs:104
void CometScienceParameter::VesselIdChanged(uint32_t oldId, uint32_t newId) {
    if (cometPersistentId == oldId) cometPersistentId = newId;
}
// CometScienceParameter.cs:112
void CometScienceParameter::OnScience(float science, ScienceSubject* subject, ProtoVessel* pv, bool reverseEngineered) {
    (void)science;
    (void)pv;
    if (!reverseEngineered && subject->id == scienceSubjectId) {
        SetComplete();
        Root()->Complete();
    }
}
// CometScienceParameter.cs:120
void CometScienceParameter::OnTriggeredScience(ScienceData* data, Vessel* origin, bool xmitAborted) {
    (void)origin;
    if (!(data == nullptr || xmitAborted) && !(data->dataAmount <= 0.0f) && data->subjectID == scienceSubjectId) {
        SetComplete();
        Root()->Complete();
    }
}

// ---- ConstructionParameter ------------------------------------------------------------------------------------------------------------

// ConstructionParameter.cs:36
ConstructionParameter::ConstructionParameter(const std::string& partName_, uint32_t vesselId, const std::string& vesselName_, CelestialBody* targetBody_) {
    UpdatePartInfo(partName_, vesselId, vesselName_, targetBody_);
}
// ConstructionParameter.cs:41
void ConstructionParameter::UpdatePartInfo(const std::string& partName_, uint32_t vesselId, const std::string& vesselName_, CelestialBody* targetBody_) {
    if (!partName_.empty()) {
        partName = partName_;
        ap = SaveEnvironment::Current().getPartInfoByName ? SaveEnvironment::Current().getPartInfoByName(partName_) : nullptr;
    }
    vesselPersistentId = vesselId;
    if (!vesselName_.empty()) vesselName = vesselName_;
    if (targetBody_ != nullptr) targetBody = targetBody_;
    vsl = FindVesselByPersistentId(vesselId);
}
// ConstructionParameter.cs:60
std::string ConstructionParameter::GetHashString() { return partName + dotnet::FormatUInt64(vesselPersistentId); }
// ConstructionParameter.cs:65
std::string ConstructionParameter::GetTitle() {
    if (vsl == nullptr) vsl = FindVesselByPersistentId(vesselPersistentId);
    return Fmt("#autoLOC_6002602", {ap != nullptr ? ap->title : partName, vsl != nullptr ? vsl->vesselName : vesselName});
}
// ConstructionParameter.cs:74
void ConstructionParameter::OnLoad(ConfigNode& node) {
    std::string value;
    if (node.TryGetValue("bodyName", value)) targetBody = GetBodyByName(value);
    node.TryGetValue("partName", partName);
    node.TryGetValue("vesselName", vesselName);
    node.TryGetValue("vesselPersistentId", vesselPersistentId);
    ap = SaveEnvironment::Current().getPartInfoByName ? SaveEnvironment::Current().getPartInfoByName(partName) : nullptr;
    vsl = FindVesselByPersistentId(vesselPersistentId);
}
// ConstructionParameter.cs:90
void ConstructionParameter::OnSave(ConfigNode& node) {
    node.AddValue("bodyName", targetBody->bodyName);
    node.AddValue("partName", partName);
    node.AddValue("vesselName", vesselName);
    node.AddValue("vesselPersistentId", vesselPersistentId);
}
// ConstructionParameter.cs:98
void ConstructionParameter::OnRegister() {
    GE::onVesselPersistentIdChanged.Add(this, &ConstructionParameter::VesselIdChanged);
    GE::OnEVAConstructionModePartAttached.Add(this, &ConstructionParameter::OnConstructionPartAttached);
}
// ConstructionParameter.cs:104
void ConstructionParameter::OnUnregister() {
    GE::onVesselPersistentIdChanged.Remove(this, &ConstructionParameter::VesselIdChanged);
    GE::OnEVAConstructionModePartAttached.Remove(this, &ConstructionParameter::OnConstructionPartAttached);
}
// ConstructionParameter.cs:110
std::string ConstructionParameter::GetMessageComplete() { return Fmt("#autoLOC_6002604"); }
// ConstructionParameter.cs:115
void ConstructionParameter::VesselIdChanged(uint32_t oldId, uint32_t newId) {
    if (vesselPersistentId == oldId) vesselPersistentId = newId;
}
// ConstructionParameter.cs:123
void ConstructionParameter::OnConstructionPartAttached(Vessel* vsl_, Part* p) {
    if (vsl_->persistentId == vesselPersistentId && p->partInfo->name == partName) {
        SetComplete();
        Root()->Complete();
    }
}

// ---- RepairPartParameter --------------------------------------------------------------------------------------------------------------

// RepairPartParameter.cs:40
RepairPartParameter::RepairPartParameter(uint32_t partId, const std::string& partName_, uint32_t vesselId, const std::string& vesselName_, CelestialBody* targetBody_) {
    UpdatePartInfo(partId, partName_, vesselId, vesselName_, targetBody_);
}
// RepairPartParameter.cs:45
void RepairPartParameter::UpdatePartInfo(uint32_t partId, const std::string& partName_, uint32_t vesselId, const std::string& vesselName_, CelestialBody* targetBody_) {
    partPersistentId = partId;
    if (!partName_.empty()) {
        partName = partName_;
        ap = SaveEnvironment::Current().getPartInfoByName ? SaveEnvironment::Current().getPartInfoByName(partName_) : nullptr;
    }
    vesselPersistentId = vesselId;
    if (!vesselName_.empty()) vesselName = vesselName_;
    if (targetBody_ != nullptr) targetBody = targetBody_;
    vsl = FindVesselByPersistentId(vesselId);
}
// RepairPartParameter.cs:66
std::string RepairPartParameter::GetHashString() { return partName + dotnet::FormatUInt64(partPersistentId); }
// RepairPartParameter.cs:71
std::string RepairPartParameter::GetTitle() {
    if (vsl == nullptr) vsl = FindVesselByPersistentId(vesselPersistentId);
    return Fmt("#autoLOC_6002578", {vsl != nullptr ? vsl->vesselName : vesselName, ap != nullptr ? ap->title : partName, targetBody != nullptr ? targetBody->bodyDisplayName : std::string()});
}
// RepairPartParameter.cs:80
void RepairPartParameter::OnLoad(ConfigNode& node) {
    std::string value;
    if (node.TryGetValue("bodyName", value)) targetBody = GetBodyByName(value);
    node.TryGetValue("partName", partName);
    node.TryGetValue("partPersistentId", partPersistentId);
    node.TryGetValue("vesselPersistentId", vesselPersistentId);
    node.TryGetValue("vesselName", vesselName);
    ap = SaveEnvironment::Current().getPartInfoByName ? SaveEnvironment::Current().getPartInfoByName(partName) : nullptr;
    vsl = FindVesselByPersistentId(vesselPersistentId);
}
// RepairPartParameter.cs:96
void RepairPartParameter::OnSave(ConfigNode& node) {
    node.AddValue("bodyName", targetBody->bodyName);
    node.AddValue("partName", partName);
    node.AddValue("partPersistentId", partPersistentId);
    node.AddValue("vesselPersistentId", vesselPersistentId);
    if (!vesselName.empty()) node.AddValue("vesselName", vesselName);
}
// RepairPartParameter.cs:108
void RepairPartParameter::OnRegister() {
    GE::onVesselPersistentIdChanged.Add(this, &RepairPartParameter::VesselIdChanged);
    GE::onPartPersistentIdChanged.Add(this, &RepairPartParameter::PartIdChanged);
    GE::onPartRepaired.Add(this, &RepairPartParameter::OnPartRepaired);
}
// RepairPartParameter.cs:115
void RepairPartParameter::OnUnregister() {
    GE::onVesselPersistentIdChanged.Remove(this, &RepairPartParameter::VesselIdChanged);
    GE::onPartPersistentIdChanged.Remove(this, &RepairPartParameter::PartIdChanged);
    GE::onPartRepaired.Remove(this, &RepairPartParameter::OnPartRepaired);
}
// RepairPartParameter.cs:122
std::string RepairPartParameter::GetMessageComplete() { return Fmt("#autoLOC_6002580"); }
// RepairPartParameter.cs:127
void RepairPartParameter::PartIdChanged(uint32_t vslId, uint32_t oldId, uint32_t newId) {
    if (vslId == vesselPersistentId && oldId == partPersistentId) partPersistentId = newId;
}
// RepairPartParameter.cs:135
void RepairPartParameter::VesselIdChanged(uint32_t oldId, uint32_t newId) {
    if (vesselPersistentId == oldId) vesselPersistentId = newId;
}
// RepairPartParameter.cs:143
void RepairPartParameter::OnPartRepaired(Part* p) {
    if ((partPersistentId == 0 || p->fields.persistentId == partPersistentId) && p->vessel->persistentId == vesselPersistentId) {
        SetComplete();
        Root()->Complete();
    }
}

}  // namespace ksp::Contracts::Parameters
