// WP-54 slice 2: Assembly-CSharp/Expansions/Missions/Tests/*.cs (the tests WP-55 deferred), part 2: the roster-aware tests (Kerbal*, TakeKerbal,
// CrewAssignment), TestGrapple, TestPartDocking and the registration. See mission_tests2.hpp / mission_tests2.cpp. Cite placeholders
// `File.cs:@Name[#k]` are resolved by tools/wp55/check_cites.py --resolve.
#include <algorithm>
#include <cctype>

#include "../save/save_util.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/missions/mission.hpp"
#include "ksp/missions/mission_actions.hpp"
#include "ksp/missions/mission_tests2.hpp"
#include "ksp/missions/mission_validator.hpp"
#include "ksp/modules/misc_modules.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/proto_vessel.hpp"
#include "ksp/science/science.hpp"
#include "ksp/space_center/high_logic.hpp"
#include "ksp/vessel/discovery_info.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp::Expansions::Missions::Tests {
namespace {

using Env = MissionTestsEnvironment;
using KSP::Localization::Localizer;
using PCM = std::shared_ptr<ProtoCrewMember>;

std::string Tag(const char* t) { return Localizer::Format(t); }
bool EqualsIgnoreCase(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i]))) return false;
    return true;
}
Vessel* Persistent(uint32_t id) { return AwardEnvironment::VesselByPersistentId(id); }
CelestialBody* Home() { return NodeLibraryEnvironment::HomeBody(); }
std::string ExperimentBiome(CelestialBody* body, double lat, double lon) {
    if (Env::experimentBiome) return Env::experimentBiome(body, lat, lon);
    return ScienceUtil::GetExperimentBiome(*body, lat, lon);
}
void InitSurfaceChoices(ParamChoices_CelestialBodySurface& p, CelestialBody* bodyDataBody) {
    p.bodyData = std::make_unique<MissionCelestialBody>(bodyDataBody);
    p.biomeData = std::make_unique<Editor::MissionBiome>(Home(), "");
    p.areaData = std::make_unique<SurfaceArea>(Home(), 0.0, 0.0, 150000.0f);
    p.launchSiteName = "";
}
FinePrint::Waypoint* NamedWaypoint(const ParamChoices_CelestialBodySurface& p, std::unique_ptr<FinePrint::Waypoint>& store, const std::string& title) {
    store = p.GetWaypoint();
    if (store == nullptr) return nullptr;
    store->name = title;
    return store.get();
}
KerbalRoster& Roster() { return *HighLogic::CurrentGame()->CrewRoster; }

// the `field.name == "vesselID"` text of the Kerbal tests (TestKerbalEVA.cs:@GetNodeBodyParameterString and copies)
std::string VesselIdText(Mission* mission, uint32_t vesselID) {
    VesselSituation* vesselSituation = mission->GetVesselSituationByVesselID(vesselID);
    std::string vesselName;
    bool flag = false;
    if (vesselSituation != nullptr) {
        vesselName = vesselSituation->vesselName;
    } else if (mission->UpdateFromMappedVesselIDs(vesselID, vesselName, vesselSituation)) {
        flag = true;
    }
    if (vesselSituation == nullptr && !flag) return Localizer::Format("#autoLOC_8000069", {Tag("#autoLOC_8001004")});
    std::string text = Localizer::Format("#autoLOC_8000069", {vesselName});
    if (flag && vesselSituation != nullptr) text = text + Localizer::Format("#autoLOC_6002491", {vesselSituation->vesselName}) + "\n";
    return text;
}
// the RunValidation of the Kerbal tests
void ValidateKerbal(Editor::MissionEditorValidator* validator, MENode* node, MissionKerbal& missionKerbal) {
    if (missionKerbal.Kerbal() != nullptr) {
        PCM p = (*node->mission->situation->crewRoster)[missionKerbal.Kerbal()->name().value_or("")];
        if (p == nullptr) throw NullReferenceException();
        if (p->rosterStatus() != ProtoCrewMember::RosterStatus::Assigned) validator->AddNodeWarn(node, Localizer::Format("#autoLOC_8002033", {missionKerbal.Name()}));
    }
}
// the location-choice test of TestKerbalEVA.OnEVA / TestKerbalBoarded.CheckSituation
void CheckLocation(const ParamChoices_CelestialBodySurface& situation, Vessel* vessel, bool& eventFound) {
    CelestialBody* current = Env::currentMainBody ? Env::currentMainBody() : nullptr;
    switch (situation.locationChoice) {
        default: eventFound = true; break;
        case ParamChoices_CelestialBodySurface::Choices::bodyData:
            if (situation.bodyData->IsValid(current)) eventFound = true;
            break;
        case ParamChoices_CelestialBodySurface::Choices::biomeData:
            if (!(current != situation.biomeData->body)) {
                const std::string experimentBiome = ExperimentBiome(current, vessel->latitude, vessel->longitude);
                if (situation.biomeData->biomeName.empty() || EqualsIgnoreCase(experimentBiome, situation.biomeData->biomeName)) eventFound = true;
            }
            break;
        case ParamChoices_CelestialBodySurface::Choices::areaData:
            if (!(current != situation.areaData->body)) eventFound = situation.areaData->IsPointInCircle(vessel->latitude, vessel->longitude);
            break;
    }
}
std::unique_ptr<MissionKerbal> NewMissionKerbal(TestModule* self, MENode* node, ProtoCrewMember::KerbalType type = ProtoCrewMember::KerbalType::Crew) {
    return std::make_unique<MissionKerbal>(nullptr, node, [self] { self->UpdateNodeBodyUI(); }, type);
}
}  // namespace

// ---- TestKerbalKilled ------------------------------------------------------------------------------------------------------------

// TestKerbalKilled.cs:@Awake
TestKerbalKilled::TestKerbalKilled() : TestModule(kTypeName) {
    title = Tag("#autoLOC_8007400");
    missionKerbal = NewMissionKerbal(this, node());
}
TestKerbalKilled::~TestKerbalKilled() {
    if (subscribed) GameEvents::onCrewKilled.Remove(this, &TestKerbalKilled::OnKilled);
}
// TestKerbalKilled.cs:@Initialize
void TestKerbalKilled::Initialize(TestGroup* testGroup, const std::string& newTitle) {
    TestModule::Initialize(testGroup, newTitle);
    missionKerbal->Initialize(nullptr, testGroup->node());
}
// TestKerbalKilled.cs:@Initialized
void TestKerbalKilled::Initialized() {
    eventFound = false;
    GameEvents::onCrewKilled.Add(this, &TestKerbalKilled::OnKilled);
    subscribed = true;
    if (missionKerbal->Kerbal() != nullptr) {
        PCM protoCrewMember = Roster()[missionKerbal->Kerbal()->name().value_or("")];
        if (protoCrewMember == nullptr ||
            (missionKerbal->TypeToShow() == protoCrewMember->type() && (protoCrewMember->rosterStatus() == ProtoCrewMember::RosterStatus::Missing ||
                                                                       protoCrewMember->rosterStatus() == ProtoCrewMember::RosterStatus::Dead))) {
            eventFound = true;
        }
    }
}
// TestKerbalKilled.cs:@Cleared
void TestKerbalKilled::Cleared() {
    GameEvents::onCrewKilled.Remove(this, &TestKerbalKilled::OnKilled);
    subscribed = false;
}
// TestKerbalKilled.cs:@OnKilled
void TestKerbalKilled::OnKilled(EventReport* report) {
    const std::string sender = Env::eventReportSender ? Env::eventReportSender(report) : std::string();
    if (missionKerbal->Kerbal() == nullptr) {
        PCM p = Roster()[sender];
        if (p == nullptr) throw NullReferenceException();
        if (p->type() == missionKerbal->TypeToShow()) eventFound = true;
    } else if (sender == missionKerbal->Kerbal()->name().value_or("")) {
        eventFound = true;
    }
}
// TestKerbalKilled.cs:@Test
bool TestKerbalKilled::Test() { return eventFound; }
// TestKerbalKilled.cs:@GetInfo
std::string TestKerbalKilled::GetInfo() { return Tag("#autoLOC_8007401"); }
// TestKerbalKilled.cs:@GetNodeBodyParameterString
std::string TestKerbalKilled::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.name == "missionKerbal") return missionKerbal->GetNodeBodyParameterString();
    return TestModule::GetNodeBodyParameterString(field);
}
// TestKerbalKilled.cs:@RunValidation
void TestKerbalKilled::RunValidation(Editor::MissionEditorValidator* validator) { ValidateKerbal(validator, node(), *missionKerbal); }
// TestKerbalKilled.cs:@Save
void TestKerbalKilled::Save(ConfigNode& node) const {
    TestModule::Save(node);
    missionKerbal->Save(node);
}
// TestKerbalKilled.cs:@Load
void TestKerbalKilled::Load(const ConfigNode& node) {
    TestModule::Load(node);
    if (missionKerbal == nullptr) missionKerbal = NewMissionKerbal(this, this->node());
    missionKerbal->Load(node);
}

// ---- TestKerbalRecovery ----------------------------------------------------------------------------------------------------------

// TestKerbalRecovery.cs:@Awake
TestKerbalRecovery::TestKerbalRecovery() : TestModule(kTypeName) {
    title = Tag("#autoLOC_8000150");
    missionKerbal = NewMissionKerbal(this, node());
}
TestKerbalRecovery::~TestKerbalRecovery() {
    if (subscribed) GameEvents::onVesselRecovered.Remove(this, &TestKerbalRecovery::OnRecovered);
}
// TestKerbalRecovery.cs:@Initialize
void TestKerbalRecovery::Initialize(TestGroup* testGroup, const std::string& newTitle) {
    TestModule::Initialize(testGroup, newTitle);
    missionKerbal->Initialize(nullptr, testGroup->node());
}
// TestKerbalRecovery.cs:@checkAlreadyRecovered
void TestKerbalRecovery::checkAlreadyRecovered() {
    if (HighLogic::CurrentGame() != nullptr) {
        PCM protoCrewMember = Roster()[missionKerbal->Kerbal()->name().value_or("")];
        if (protoCrewMember != nullptr && protoCrewMember->rosterStatus() == ProtoCrewMember::RosterStatus::Available) eventFound = true;
    }
}
// TestKerbalRecovery.cs:@Initialized
void TestKerbalRecovery::Initialized() {
    eventFound = false;
    if (missionKerbal != nullptr && missionKerbal->Kerbal() != nullptr) checkAlreadyRecovered();
    if (!eventFound) {
        GameEvents::onVesselRecovered.Add(this, &TestKerbalRecovery::OnRecovered);
        subscribed = true;
    }
}
// TestKerbalRecovery.cs:@Cleared
void TestKerbalRecovery::Cleared() {
    GameEvents::onVesselRecovered.Remove(this, &TestKerbalRecovery::OnRecovered);
    subscribed = false;
}
// TestKerbalRecovery.cs:@OnRecovered
void TestKerbalRecovery::OnRecovered(ProtoVessel* pv, bool) {
    const std::vector<PCM> pcm = pv->GetVesselCrew();
    if (pcm.empty()) return;
    if (missionKerbal != nullptr && missionKerbal->Kerbal() != nullptr) {
        int count = static_cast<int>(pcm.size());
        while (count-- > 0) {
            if (missionKerbal->IsValid(*pcm[static_cast<size_t>(count)])) eventFound = true;
        }
    } else {
        eventFound = true;
    }
}
// TestKerbalRecovery.cs:@Test
bool TestKerbalRecovery::Test() { return eventFound; }
// TestKerbalRecovery.cs:@GetInfo
std::string TestKerbalRecovery::GetInfo() { return Tag("#autoLOC_8004018"); }
// TestKerbalRecovery.cs:@GetNodeBodyParameterString
std::string TestKerbalRecovery::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.name == "missionKerbal") return missionKerbal->GetNodeBodyParameterString();
    return TestModule::GetNodeBodyParameterString(field);
}
// TestKerbalRecovery.cs:@RunValidation
void TestKerbalRecovery::RunValidation(Editor::MissionEditorValidator* validator) { ValidateKerbal(validator, node(), *missionKerbal); }
// TestKerbalRecovery.cs:@Save
void TestKerbalRecovery::Save(ConfigNode& node) const {
    TestModule::Save(node);
    missionKerbal->Save(node);
}
// TestKerbalRecovery.cs:@Load
void TestKerbalRecovery::Load(const ConfigNode& node) {
    TestModule::Load(node);
    if (missionKerbal == nullptr) missionKerbal = NewMissionKerbal(this, this->node());
    missionKerbal->Load(node);
}
// TestKerbalRecovery.cs:@GetScoreModifier
ScoreModifier TestKerbalRecovery::GetScoreModifier(const std::string& scoreModule) {
    ScoreModifier r;
    if (scoreModule == "ScoreModule_Resource") r.vessel = AwardEnvironment::ActiveVessel();
    return r;
}

// ---- TestKerbalRescue ------------------------------------------------------------------------------------------------------------

// TestKerbalRescue.cs:@Awake
TestKerbalRescue::TestKerbalRescue() : TestModule(kTypeName) {
    title = Tag("#autoLOC_8000154");
    missionKerbal = NewMissionKerbal(this, node());
}
TestKerbalRescue::~TestKerbalRescue() { Cleared(); }
// TestKerbalRescue.cs:@Initialize
void TestKerbalRescue::Initialize(TestGroup* testGroup, const std::string& newTitle) {
    TestModule::Initialize(testGroup, newTitle);
    missionKerbal->Initialize(nullptr, testGroup->node());
    mustBoardVessel = false;
}
// TestKerbalRescue.cs:@Initialized
void TestKerbalRescue::Initialized() {
    eventFound = false;
    const std::vector<Vessel*> vessels = AwardEnvironment::Vessels();
    int count = static_cast<int>(vessels.size());
    while (count-- > 0) {
        Vessel* vessel = vessels[static_cast<size_t>(count)];
        const std::vector<PCM>& vesselCrew = vessel->GetVesselCrew();
        for (size_t i = 0; i < vesselCrew.size(); ++i) {
            if (missionKerbal->IsValid(*vesselCrew[i]) && vessel->GetDiscoveryInfo().Level() == DiscoveryLevels::Owned) {
                if (!mustBoardVessel || (mustBoardVessel && !vessel->isEVA())) eventFound = true;
                break;
            }
        }
    }
    if (!eventFound) {
        if (mustBoardVessel) {
            GameEvents::onCrewBoardVessel.Add(this, &TestKerbalRescue::OnCrewBoardVessel);
            GameEvents::onCrewTransferred.Add(this, &TestKerbalRescue::OnCrewTransferred);
            subscribedBoard = true;
        } else {
            GameEvents::onKnowledgeChanged.Add(this, &TestKerbalRescue::onKnowledgeChanged);
            subscribedKnowledge = true;
        }
    }
}
// TestKerbalRescue.cs:@Cleared
void TestKerbalRescue::Cleared() {
    GameEvents::onCrewBoardVessel.Remove(this, &TestKerbalRescue::OnCrewBoardVessel);
    GameEvents::onCrewTransferred.Remove(this, &TestKerbalRescue::OnCrewTransferred);
    GameEvents::onKnowledgeChanged.Remove(this, &TestKerbalRescue::onKnowledgeChanged);
    subscribedBoard = subscribedKnowledge = false;
}
// TestKerbalRescue.cs:@OnCrewBoardVessel
void TestKerbalRescue::OnCrewBoardVessel(GameEvents::FromToAction<Part*, Part*> action) { CheckNewKerbal(action.from->vessel->vesselName); }
// TestKerbalRescue.cs:@OnCrewTransferred
void TestKerbalRescue::OnCrewTransferred(GameEvents::HostedFromToAction<ProtoCrewMember*, Part*> action) { CheckNewKerbal(action.host->name().value_or("")); }
// TestKerbalRescue.cs:@CheckNewKerbal
void TestKerbalRescue::CheckNewKerbal(const std::string& rescuedName) {
    if (missionKerbal == nullptr || missionKerbal->Kerbal() == nullptr || missionKerbal->Kerbal()->name().value_or("") == rescuedName) eventFound = true;
}
// TestKerbalRescue.cs:@onKnowledgeChanged
void TestKerbalRescue::onKnowledgeChanged(GameEvents::HostedFromToAction<IDiscoverable*, DiscoveryLevels> kChg) {
    Vessel* vessel = dynamic_cast<Vessel*>(kChg.host);
    if (!(vessel != nullptr)) return;
    const std::vector<PCM>& vesselCrew = vessel->GetVesselCrew();
    for (size_t i = 0; i < vesselCrew.size(); ++i) {
        if (missionKerbal->Kerbal() == nullptr) throw NullReferenceException();
        if (vesselCrew[i]->name() == missionKerbal->Kerbal()->name() && kChg.to == DiscoveryLevels::Owned) eventFound = true;
    }
}
// TestKerbalRescue.cs:@Test
bool TestKerbalRescue::Test() { return eventFound; }
// TestKerbalRescue.cs:@GetInfo
std::string TestKerbalRescue::GetInfo() { return Tag("#autoLOC_8004019"); }
// TestKerbalRescue.cs:@GetNodeBodyParameterString
std::string TestKerbalRescue::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.name == "missionKerbal") return missionKerbal->GetNodeBodyParameterString();
    return TestModule::GetNodeBodyParameterString(field);
}
// TestKerbalRescue.cs:@RunValidation
void TestKerbalRescue::RunValidation(Editor::MissionEditorValidator* validator) { ValidateKerbal(validator, node(), *missionKerbal); }
// TestKerbalRescue.cs:@Save
void TestKerbalRescue::Save(ConfigNode& node) const {
    TestModule::Save(node);
    missionKerbal->Save(node);
    node.AddValue("mustBoardVessel", mustBoardVessel);
}
// TestKerbalRescue.cs:@Load
void TestKerbalRescue::Load(const ConfigNode& node) {
    TestModule::Load(node);
    if (missionKerbal == nullptr) missionKerbal = NewMissionKerbal(this, this->node());
    missionKerbal->Load(node);
    node.TryGetValue("mustBoardVessel", mustBoardVessel);
}
// TestKerbalRescue.cs:@GetScoreModifier
ScoreModifier TestKerbalRescue::GetScoreModifier(const std::string& scoreModule) {
    ScoreModifier r;
    if (scoreModule == "ScoreModule_Resource") r.vessel = AwardEnvironment::ActiveVessel();
    return r;
}

// ---- TestKerbalEVA ---------------------------------------------------------------------------------------------------------------

// TestKerbalEVA.cs:@Awake
TestKerbalEVA::TestKerbalEVA() : TestModule(kTypeName) {
    title = Tag("#autoLOC_8000185");
    missionKerbal = NewMissionKerbal(this, node());
    situation = std::make_unique<ParamChoices_CelestialBodySurface>();
    InitSurfaceChoices(*situation, Home());
    if (node() != nullptr) node()->UpdateInfoOnRefresh = true;
}
TestKerbalEVA::~TestKerbalEVA() {
    if (subscribed) GameEvents::onCrewOnEva.Remove(this, &TestKerbalEVA::OnEVA);
}
// TestKerbalEVA.cs:@Initialize
void TestKerbalEVA::Initialize(TestGroup* testGroup, const std::string& newTitle) {
    TestModule::Initialize(testGroup, newTitle);
    missionKerbal->Initialize(nullptr, testGroup->node());
}
// TestKerbalEVA.cs:@Initialized
void TestKerbalEVA::Initialized() {
    eventFound = false;
    GameEvents::onCrewOnEva.Add(this, &TestKerbalEVA::OnEVA);
    subscribed = true;
}
// TestKerbalEVA.cs:@Cleared
void TestKerbalEVA::Cleared() {
    GameEvents::onCrewOnEva.Remove(this, &TestKerbalEVA::OnEVA);
    subscribed = false;
}
// TestKerbalEVA.cs:@OnEVA
void TestKerbalEVA::OnEVA(GameEvents::FromToAction<Part*, Part*> fromTo) {
    if (!fromTo.to->vessel->isEVA() || !missionKerbal->IsValid(*fromTo.to->protoModuleCrew.at(0))) return;
    vessel = fromTo.from->vessel;
    if (vesselID != 0) {
        const uint32_t num = node()->mission->CurrentVesselID(node(), vesselID);
        if (vessel->persistentId != num) return;
    }
    CheckLocation(*situation, vessel, eventFound);
}
// TestKerbalEVA.cs:@Test
bool TestKerbalEVA::Test() { return eventFound; }
// TestKerbalEVA.cs:@OnVesselPersistentIdChanged
void TestKerbalEVA::OnVesselPersistentIdChanged(uint32_t oldId, uint32_t newId) {
    if (vesselID == oldId) vesselID = newId;
}
// TestKerbalEVA.cs:@HasNodeWaypoint
bool TestKerbalEVA::HasNodeWaypoint() { return situation->HasWaypoint(); }
// TestKerbalEVA.cs:@GetNodeWaypoint
FinePrint::Waypoint* TestKerbalEVA::GetNodeWaypoint() { return NamedWaypoint(*situation, nodeWaypoint_, title); }
// TestKerbalEVA.cs:@HasNodeLabel
bool TestKerbalEVA::HasNodeLabel() { return situation->HasNodeLabel(); }
// TestKerbalEVA.cs:@HasWorldPosition
bool TestKerbalEVA::HasWorldPosition() { return situation->HasWorldPosition(); }
// TestKerbalEVA.cs:@GetWorldPosition
Vector3f TestKerbalEVA::GetWorldPosition() { return situation->GetWorldPosition(); }
// TestKerbalEVA.cs:@GetExtraText
std::string TestKerbalEVA::GetExtraText() { return situation->GetExtraText(); }
// TestKerbalEVA.cs:@GetInfo
std::string TestKerbalEVA::GetInfo() { return Tag("#autoLOC_8004014"); }
// TestKerbalEVA.cs:@GetNodeBodyParameterString
std::string TestKerbalEVA::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (node() != nullptr) node()->UpdateInfoOnRefresh = true;
    if (field.name == "missionKerbal" && node()->mission != nullptr) return missionKerbal->GetNodeBodyParameterString();
    if (field.name == "vesselID") return VesselIdText(node()->mission, vesselID);
    return TestModule::GetNodeBodyParameterString(field);
}
// TestKerbalEVA.cs:@RunValidation
void TestKerbalEVA::RunValidation(Editor::MissionEditorValidator* validator) { ValidateKerbal(validator, node(), *missionKerbal); }
// TestKerbalEVA.cs:@Save
void TestKerbalEVA::Save(ConfigNode& node) const {
    TestModule::Save(node);
    missionKerbal->Save(node);
    situation->Save(node);
    node.AddValue("vesselID", vesselID);
}
// TestKerbalEVA.cs:@Load
void TestKerbalEVA::Load(const ConfigNode& node) {
    TestModule::Load(node);
    if (missionKerbal == nullptr) missionKerbal = NewMissionKerbal(this, this->node());
    missionKerbal->Load(node);
    situation->Load(node);
    node.TryGetValue("vesselID", vesselID);
}
// TestKerbalEVA.cs:@GetScoreModifier
ScoreModifier TestKerbalEVA::GetScoreModifier(const std::string& scoreModule) {
    ScoreModifier r;
    if (scoreModule == "ScoreModule_Resource") r.vessel = vessel != nullptr ? vessel : AwardEnvironment::ActiveVessel();
    return r;
}

// ---- TestKerbalBoarded -----------------------------------------------------------------------------------------------------------

// TestKerbalBoarded.cs:@Awake
TestKerbalBoarded::TestKerbalBoarded() : TestModule(kTypeName) {
    title = Tag("#autoLOC_8000148");
    missionKerbal = NewMissionKerbal(this, node());
    situation = std::make_unique<ParamChoices_CelestialBodySurface>();
    InitSurfaceChoices(*situation, Home());
}
TestKerbalBoarded::~TestKerbalBoarded() {
    if (subscribed) {
        GameEvents::onCrewTransferred.Remove(this, &TestKerbalBoarded::OnBoarded);
        GameEvents::onCommandSeatInteraction.Remove(this, &TestKerbalBoarded::OnSeatedEvent);
    }
}
// TestKerbalBoarded.cs:@Initialize
void TestKerbalBoarded::Initialize(TestGroup* testGroup, const std::string& newTitle) {
    TestModule::Initialize(testGroup, newTitle);
    missionKerbal->Initialize(nullptr, testGroup->node());
}
// TestKerbalBoarded.cs:@Initialized
void TestKerbalBoarded::Initialized() {
    eventFound = false;
    GameEvents::onCrewTransferred.Add(this, &TestKerbalBoarded::OnBoarded);
    GameEvents::onCommandSeatInteraction.Add(this, &TestKerbalBoarded::OnSeatedEvent);
    subscribed = true;
}
// TestKerbalBoarded.cs:@Cleared
void TestKerbalBoarded::Cleared() {
    GameEvents::onCrewTransferred.Remove(this, &TestKerbalBoarded::OnBoarded);
    GameEvents::onCommandSeatInteraction.Remove(this, &TestKerbalBoarded::OnSeatedEvent);
    subscribed = false;
}
// TestKerbalBoarded.cs:@OnBoarded
void TestKerbalBoarded::OnBoarded(GameEvents::HostedFromToAction<ProtoCrewMember*, Part*> hostedFromTo) {
    if (hostedFromTo.from->vessel->isEVA() && missionKerbal->IsValid(*hostedFromTo.host)) {
        vessel = hostedFromTo.to->vessel;
        CheckSituation();
    }
}
// TestKerbalBoarded.cs:@OnSeated
void TestKerbalBoarded::OnSeatedEvent(KerbalEVA* eva, bool) {
    std::pair<std::vector<PCM>, Vessel*> info;
    if (Env::kerbalEvaCrew) info = Env::kerbalEvaCrew(eva);
    OnSeated(info.first, info.second);
}
void TestKerbalBoarded::OnSeated(const std::vector<PCM>& evaPartCrew, Vessel* evaVessel) {
    size_t num = 0;
    while (true) {
        if (num < evaPartCrew.size()) {
            if (missionKerbal->IsValid(*evaPartCrew[num])) break;
            ++num;
            continue;
        }
        return;
    }
    vessel = evaVessel;
    CheckSituation();
}
// TestKerbalBoarded.cs:@CheckSituation
void TestKerbalBoarded::CheckSituation() {
    if (vesselID != 0) {
        const uint32_t num = node()->mission->CurrentVesselID(node(), vesselID);
        if (vessel->persistentId != num) return;
    }
    CheckLocation(*situation, vessel, eventFound);
}
// TestKerbalBoarded.cs:@Test
bool TestKerbalBoarded::Test() { return eventFound; }
// TestKerbalBoarded.cs:@OnVesselPersistentIdChanged
void TestKerbalBoarded::OnVesselPersistentIdChanged(uint32_t oldId, uint32_t newId) {
    if (vesselID == oldId) vesselID = newId;
}
// TestKerbalBoarded.cs:@HasNodeWaypoint
bool TestKerbalBoarded::HasNodeWaypoint() { return situation->HasWaypoint(); }
// TestKerbalBoarded.cs:@GetNodeWaypoint
FinePrint::Waypoint* TestKerbalBoarded::GetNodeWaypoint() { return NamedWaypoint(*situation, nodeWaypoint_, title); }
// TestKerbalBoarded.cs:@HasNodeLabel
bool TestKerbalBoarded::HasNodeLabel() { return situation->HasNodeLabel(); }
// TestKerbalBoarded.cs:@HasWorldPosition
bool TestKerbalBoarded::HasWorldPosition() { return situation->HasWorldPosition(); }
// TestKerbalBoarded.cs:@GetWorldPosition
Vector3f TestKerbalBoarded::GetWorldPosition() { return situation->GetWorldPosition(); }
// TestKerbalBoarded.cs:@GetExtraText
std::string TestKerbalBoarded::GetExtraText() { return situation->GetExtraText(); }
// TestKerbalBoarded.cs:@GetInfo
std::string TestKerbalBoarded::GetInfo() { return Tag("#autoLOC_8004017"); }
// TestKerbalBoarded.cs:@GetNodeBodyParameterString
std::string TestKerbalBoarded::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (node() != nullptr) node()->UpdateInfoOnRefresh = true;
    if (field.name == "missionKerbal") return missionKerbal->GetNodeBodyParameterString();
    if (field.name == "vesselID" && node()->mission != nullptr) return VesselIdText(node()->mission, vesselID);
    return TestModule::GetNodeBodyParameterString(field);
}
// TestKerbalBoarded.cs:@RunValidation
void TestKerbalBoarded::RunValidation(Editor::MissionEditorValidator* validator) { ValidateKerbal(validator, node(), *missionKerbal); }
// TestKerbalBoarded.cs:@Save
void TestKerbalBoarded::Save(ConfigNode& node) const {
    TestModule::Save(node);
    missionKerbal->Save(node);
    situation->Save(node);
    node.AddValue("vesselID", vesselID);
}
// TestKerbalBoarded.cs:@Load
void TestKerbalBoarded::Load(const ConfigNode& node) {
    TestModule::Load(node);
    if (missionKerbal == nullptr) missionKerbal = NewMissionKerbal(this, this->node());
    missionKerbal->Load(node);
    situation->Load(node);
    node.TryGetValue("vesselID", vesselID);
}
// TestKerbalBoarded.cs:@GetScoreModifier
ScoreModifier TestKerbalBoarded::GetScoreModifier(const std::string& scoreModule) {
    ScoreModifier r;
    if (scoreModule == "ScoreModule_Resource") r.vessel = vessel != nullptr ? vessel : AwardEnvironment::ActiveVessel();
    return r;
}

// ---- TestTakeKerbal --------------------------------------------------------------------------------------------------------------

// TestTakeKerbal.cs:@Awake
TestTakeKerbal::TestTakeKerbal() : TestModule(kTypeName) {
    title = Tag("#autoLOC_8000174");
    missionKerbal = NewMissionKerbal(this, node(), ProtoCrewMember::KerbalType::Tourist);
    locationSituation = std::make_unique<ParamChoices_CelestialBodySurface>();
    InitSurfaceChoices(*locationSituation, Home());
    targetVessel = nullptr;
}
TestTakeKerbal::~TestTakeKerbal() { Cleared(); }
// TestTakeKerbal.cs:@Initialize
void TestTakeKerbal::Initialize(TestGroup* testGroup, const std::string& newTitle) {
    TestModule::Initialize(testGroup, newTitle);
    missionKerbal->Initialize(nullptr, testGroup->node());
}
// TestTakeKerbal.cs:@Initialized
void TestTakeKerbal::Initialized() {
    testPassed = false;
    if (vesselID != 0) {
        if (!missionKerbal->AnyValid()) {
            GameEvents::onVesselCrewWasModified.Add(this, &TestTakeKerbal::onVesselCrewWasModified);
            subscribedCrew = true;
        }
        const uint32_t key = node()->mission->CurrentVesselID(node(), vesselID);
        if (Vessel* v = Persistent(key); v != nullptr) CheckCrew(v);
        return;
    }
    if (!missionKerbal->AnyValid()) {
        GameEvents::onVesselCrewWasModified.Add(this, &TestTakeKerbal::onVesselCrewWasModified);
        GameEvents::onVesselChange.Add(this, &TestTakeKerbal::onVesselChange);
        subscribedCrew = subscribedChange = true;
        scanVessels();
        return;
    }
    GameEvents::onVesselChange.Add(this, &TestTakeKerbal::onVesselChange);
    subscribedChange = true;
    const std::vector<Vessel*> vessels = AwardEnvironment::Vessels();
    for (size_t i = 0; i < vessels.size(); ++i) {
        testLocation(vessels[i]);
        if (testPassed) {
            CheckCrew(vessels[i]);
            if (targetVessel != nullptr && targetVessel == vessels[i]) {
                testPassed = true;
            } else {
                testPassed = false;
            }
        }
    }
    targetVessel = AwardEnvironment::ActiveVessel();
}
// TestTakeKerbal.cs:@Cleared
void TestTakeKerbal::Cleared() {
    GameEvents::onVesselCrewWasModified.Remove(this, &TestTakeKerbal::onVesselCrewWasModified);
    GameEvents::onVesselChange.Remove(this, &TestTakeKerbal::onVesselChange);
    subscribedCrew = subscribedChange = false;
}
// TestTakeKerbal.cs:@onVesselChange
void TestTakeKerbal::onVesselChange(Vessel*) { targetVessel = AwardEnvironment::ActiveVessel(); }
// TestTakeKerbal.cs:@scanVessels
void TestTakeKerbal::scanVessels() {
    targetVessel = nullptr;
    const std::vector<Vessel*> vessels = AwardEnvironment::Vessels();
    for (size_t i = 0; i < vessels.size(); ++i) CheckCrew(vessels[i]);
}
// TestTakeKerbal.cs:@onVesselCrewWasModified
void TestTakeKerbal::onVesselCrewWasModified(Vessel* v) { CheckCrew(v); }
// TestTakeKerbal.cs:@CheckCrew
bool TestTakeKerbal::CheckCrew(Vessel* v) {
    if (v != nullptr) {
        const std::vector<PCM>& pcm = v->GetVesselCrew();
        int count = static_cast<int>(pcm.size());
        while (count-- > 0) {
            if (missionKerbal->IsValid(*pcm[static_cast<size_t>(count)])) {
                targetVessel = v;
                return true;
            }
        }
    }
    return false;
}
// TestTakeKerbal.cs:@targetVesselStillValid
bool TestTakeKerbal::targetVesselStillValid(Vessel* vessel) { return CheckCrew(vessel); }
// TestTakeKerbal.cs:@testLocation
void TestTakeKerbal::testLocation(Vessel* vessel) {
    switch (locationSituation->locationChoice) {
        case ParamChoices_CelestialBodySurface::Choices::bodyData:
            if (locationSituation->bodyData->IsValid(vessel->mainBody()) && targetVesselStillValid(vessel)) testPassed = true;
            break;
        case ParamChoices_CelestialBodySurface::Choices::biomeData:
            currentBiome = ExperimentBiome(vessel->mainBody(), vessel->latitude, vessel->longitude);
            if ((vessel->Landed || vessel->Splashed) &&
                (locationSituation->biomeData->biomeName.empty() || EqualsIgnoreCase(currentBiome, locationSituation->biomeData->biomeName)) &&
                targetVesselStillValid(vessel))
                testPassed = true;
            break;
        case ParamChoices_CelestialBodySurface::Choices::areaData:
            if (locationSituation->areaData->body == vessel->mainBody() && (vessel->Landed || vessel->Splashed) && targetVesselStillValid(vessel))
                testPassed = locationSituation->areaData->IsPointInCircle(vessel->latitude, vessel->longitude);
            break;
        case ParamChoices_CelestialBodySurface::Choices::launchSiteName: testPassed = locationSituation->launchSiteName == vessel->landedAt; break;
        default: break;
    }
}
// TestTakeKerbal.cs:@Test
bool TestTakeKerbal::Test() {
    if (targetVessel != nullptr) {
        if (vesselID != 0 && targetVessel->persistentId != vesselID) return testPassed;
        testLocation(targetVessel);
    }
    return testPassed;
}
// TestTakeKerbal.cs:@GetInfo
std::string TestTakeKerbal::GetInfo() { return Tag("#autoLOC_8004033"); }
// TestTakeKerbal.cs:@GetNodeBodyParameterString
std::string TestTakeKerbal::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (node() != nullptr) node()->UpdateInfoOnRefresh = true;
    if (field.name == "missionKerbal") return missionKerbal->GetNodeBodyParameterString();
    if (field.name == "vesselID" && node()->mission != nullptr) return VesselIdText(node()->mission, vesselID);
    return TestModule::GetNodeBodyParameterString(field);
}
// TestTakeKerbal.cs:@RunValidation
void TestTakeKerbal::RunValidation(Editor::MissionEditorValidator* validator) { ValidateKerbal(validator, node(), *missionKerbal); }
// TestTakeKerbal.cs:@HasNodeWaypoint
bool TestTakeKerbal::HasNodeWaypoint() { return locationSituation->HasWaypoint(); }
// TestTakeKerbal.cs:@GetNodeWaypoint
FinePrint::Waypoint* TestTakeKerbal::GetNodeWaypoint() { return NamedWaypoint(*locationSituation, nodeWaypoint_, title); }
// TestTakeKerbal.cs:@HasNodeLabel
bool TestTakeKerbal::HasNodeLabel() { return locationSituation->HasNodeLabel(); }
// TestTakeKerbal.cs:@HasWorldPosition
bool TestTakeKerbal::HasWorldPosition() { return locationSituation->HasWorldPosition(); }
// TestTakeKerbal.cs:@GetWorldPosition
Vector3f TestTakeKerbal::GetWorldPosition() { return locationSituation->GetWorldPosition(); }
// TestTakeKerbal.cs:@GetExtraText
std::string TestTakeKerbal::GetExtraText() { return locationSituation->GetExtraText(); }
// TestTakeKerbal.cs:@Save
void TestTakeKerbal::Save(ConfigNode& node) const {
    TestModule::Save(node);
    missionKerbal->Save(node);
    locationSituation->Save(node);
    node.AddValue("vesselID", vesselID);
}
// TestTakeKerbal.cs:@Load
void TestTakeKerbal::Load(const ConfigNode& node) {
    TestModule::Load(node);
    if (missionKerbal == nullptr) missionKerbal = NewMissionKerbal(this, this->node());
    missionKerbal->Load(node);
    locationSituation->Load(node);
    node.TryGetValue("vesselID", vesselID);
}
// TestTakeKerbal.cs:@GetScoreModifier
ScoreModifier TestTakeKerbal::GetScoreModifier(const std::string& scoreModule) {
    ScoreModifier r;
    if (scoreModule == "ScoreModule_Resource") r.vessel = targetVessel != nullptr ? targetVessel : AwardEnvironment::ActiveVessel();
    return r;
}

// ---- TestCrewAssignment ----------------------------------------------------------------------------------------------------------

// TestCrewAssignment.cs:@Awake
TestCrewAssignment::TestCrewAssignment() : TestModule(kTypeName) {
    title = Tag("#autoLOC_8000070");
    vesselPartIDs = std::make_unique<VesselPartIDPair>();
    vesselPartIDs->partID = 0;
    missionKerbal = NewMissionKerbal(this, node());
}
// TestCrewAssignment.cs:@Initialize
void TestCrewAssignment::Initialize(TestGroup* testGroup, const std::string& newTitle) {
    TestModule::Initialize(testGroup, newTitle);
    missionKerbal->Initialize(nullptr, testGroup->node());
}
// TestCrewAssignment.cs:@partContainsKerbal
bool TestCrewAssignment::partContainsKerbal(const std::vector<PCM>& crew) {
    size_t num = 0;
    while (true) {
        if (num < crew.size()) {
            if (crew[num] != nullptr && missionKerbal->IsValid(*crew[num])) break;
            ++num;
            continue;
        }
        return false;
    }
    return true;
}
// TestCrewAssignment.cs:@Test
bool TestCrewAssignment::Test() {
    if (vesselPartIDs->partID == 0) {
        const uint32_t key = node()->mission->CurrentVesselID(node(), vesselPartIDs->VesselID);
        if (Vessel* v = Persistent(key); v != nullptr) vessel = v;
        if (vessel == nullptr) {
            log::Warning("[TestCrewAssignment]: Vessel Not found for Test. Skipping..");
            return true;
        }
        size_t num = 0;
        while (true) {
            if (num < vessel->parts.size()) {
                if (vessel->parts[num]->fields.CrewCapacity > 0 && partContainsKerbal(vessel->parts[num]->protoModuleCrew)) break;
                ++num;
                continue;
            }
            return false;
        }
        return true;
    }
    if (Env::persistentLoadedPart) {
        if (Part* part = Env::persistentLoadedPart(vesselPartIDs->partID); part != nullptr) return partContainsKerbal(part->protoModuleCrew);
    }
    if (Env::persistentUnloadedPart) {
        if (std::optional<Env::UnloadedPart> snap = Env::persistentUnloadedPart(vesselPartIDs->partID); snap.has_value()) return partContainsKerbal(snap->protoModuleCrew);
    }
    log::Warning("[TestCrewAssignment]: Unable to find Vessel PartID (" + std::to_string(vesselPartIDs->partID) + "). Skipping..");
    return true;
}
// TestCrewAssignment.cs:@OnPartPersistentIdChanged
void TestCrewAssignment::OnPartPersistentIdChanged(uint32_t vesselID, uint32_t oldId, uint32_t newId) {
    if (vesselPartIDs->VesselID == vesselID && vesselPartIDs->partID == oldId) {
        vesselPartIDs->partID = newId;
        log::Info("[TestCrewAssignment]: Node (" + node()->id().ToString() + ") PartId changed from " + std::to_string(oldId) + " to " + std::to_string(newId));
    }
}
// TestCrewAssignment.cs:@OnVesselPersistentIdChanged
void TestCrewAssignment::OnVesselPersistentIdChanged(uint32_t oldId, uint32_t newId) {
    if (vesselPartIDs->VesselID == oldId) {
        vesselPartIDs->VesselID = newId;
        log::Info("[TestCrewAssignment]: Node (" + node()->id().ToString() + ") VesselId changed from " + std::to_string(oldId) + " to " + std::to_string(newId));
    }
}
// TestCrewAssignment.cs:@OnVesselsUndocking
void TestCrewAssignment::OnVesselsUndocking(Vessel* oldVessel, Vessel* newVessel) {
    if (vesselPartIDs->VesselID != oldVessel->persistentId && vesselPartIDs->VesselID != newVessel->persistentId) return;
    size_t num = 0;
    while (true) {
        if (num < newVessel->parts.size()) {
            if (vesselPartIDs->partID == newVessel->parts[num]->fields.persistentId) break;
            ++num;
            continue;
        }
        return;
    }
    vesselPartIDs->VesselID = newVessel->persistentId;
    log::Info("[TestCrewAssignment]: Node (" + node()->id().ToString() + ") VesselId changed on undocking from " + std::to_string(oldVessel->persistentId) + " to " +
              std::to_string(newVessel->persistentId));
}
// TestCrewAssignment.cs:@GetInfo
std::string TestCrewAssignment::GetInfo() { return Tag("#autoLOC_8004011"); }
// TestCrewAssignment.cs:@GetNodeBodyParameterString
std::string TestCrewAssignment::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (node() != nullptr) node()->UpdateInfoOnRefresh = true;
    if (field.name == "missionKerbal") return missionKerbal->GetNodeBodyParameterString();
    if (field.name == "vesselID" && node()->mission != nullptr) {
        VesselSituation* vesselSituation = nullptr;  // `new VesselSituation()` is replaced just below (vesselPartIDs != null)
        std::string vesselName;
        bool flag = false;
        if (vesselPartIDs != nullptr) vesselSituation = node()->mission->GetVesselSituationByVesselID(vesselPartIDs->VesselID);
        if (vesselSituation != nullptr) {
            vesselName = vesselSituation->vesselName;
        } else if (node()->mission->UpdateFromMappedVesselIDs(vesselPartIDs->VesselID, vesselName, vesselSituation)) {
            flag = true;
        }
        if ((vesselSituation == nullptr || vesselSituation->persistentId == 0) && !flag) return Tag("#autoLOC_8000068");
        std::string text = Localizer::Format("#autoLOC_8000069", {Localizer::Format(vesselName)});
        if (flag && vesselSituation != nullptr) text = text + Localizer::Format("#autoLOC_6002491", {vesselSituation->vesselName}) + "\n";
        return text;
    }
    if (field.name == "vesselPartIDs") return Localizer::Format("#autoLOC_8006061", {Localizer::Format(vesselPartIDs->partName)});
    return TestModule::GetNodeBodyParameterString(field);
}
// TestCrewAssignment.cs:@RunValidation
void TestCrewAssignment::RunValidation(Editor::MissionEditorValidator* validator) {
    ValidateKerbal(validator, node(), *missionKerbal);
    if (vesselPartIDs != nullptr) vesselPartIDs->ValidatePartAgainstCraft(node(), validator);
}
// TestCrewAssignment.cs:@Save
void TestCrewAssignment::Save(ConfigNode& node) const {
    TestModule::Save(node);
    vesselPartIDs->Save(node);
    missionKerbal->Save(node);
}
// TestCrewAssignment.cs:@Load
void TestCrewAssignment::Load(const ConfigNode& node) {
    TestModule::Load(node);
    vesselPartIDs->Load(node);
    if (missionKerbal == nullptr) missionKerbal = NewMissionKerbal(this, this->node());
    missionKerbal->Load(node);
}
// TestCrewAssignment.cs:@GetScoreModifier
ScoreModifier TestCrewAssignment::GetScoreModifier(const std::string& scoreModule) {
    ScoreModifier r;
    if (scoreModule == "ScoreModule_Resource") {
        const uint32_t key = node()->mission->CurrentVesselID(node(), vesselPartIDs->VesselID);
        if (Vessel* v = Persistent(key); v != nullptr) {
            r.vessel = v;
        } else {
            r.vessel = AwardEnvironment::ActiveVessel();
        }
    }
    return r;
}

// ---- TestGrapple -----------------------------------------------------------------------------------------------------------------

// TestGrapple.cs:@Awake
TestGrapple::TestGrapple() : TestModule(kTypeName) {
    title = "#autoLOC_8002013";
    firstRunTest = true;
    testSuccess = false;
}
TestGrapple::~TestGrapple() {
    if (subscribed) Cleared();
}
// TestGrapple.cs:@Initialized
void TestGrapple::Initialized() {
    testSuccess = false;
    if (dockedUndocked == "Docked") {
        GameEvents::onDockingComplete.Add(this, &TestGrapple::onPartCouple);
        GameEvents::onPartCoupleComplete.Add(this, &TestGrapple::onPartCouple);
    } else {
        GameEvents::onPartUndockComplete.Add(this, &TestGrapple::onPartUndock);
        GameEvents::onPartDeCoupleComplete.Add(this, &TestGrapple::onPartUndock);
    }
    subscribed = true;
}
// TestGrapple.cs:@Cleared
void TestGrapple::Cleared() {
    GameEvents::onDockingComplete.Remove(this, &TestGrapple::onPartCouple);
    GameEvents::onPartCoupleComplete.Remove(this, &TestGrapple::onPartCouple);
    GameEvents::onPartUndockComplete.Remove(this, &TestGrapple::onPartUndock);
    GameEvents::onPartDeCoupleComplete.Remove(this, &TestGrapple::onPartUndock);
    subscribed = false;
}
// TestGrapple.cs:@onPartCouple
void TestGrapple::onPartCouple(GameEvents::FromToAction<Part*, Part*>) { testPartVessels(); }
// TestGrapple.cs:@onPartUndock
void TestGrapple::onPartUndock(Part*) { testPartVessels(); }
// TestGrapple.cs:@testPartVessels
void TestGrapple::testPartVessels() {
    uint32_t num = 0;
    uint32_t num2 = 0;
    switch (grappledSpaceObject) {
        default: num2 = grappleAsteroidID; break;
        case SpaceObjectChoices::Comet: num2 = grappleCometID; break;
        case SpaceObjectChoices::Vessel: num2 = node()->mission->CurrentVesselID(node(), grappleVesselID); break;
    }
    if (grapplingVesselID == 0) {
        if (!(AwardEnvironment::ActiveVessel() != nullptr)) return;
        num = AwardEnvironment::ActiveVessel()->persistentId;
    } else {
        num = node()->mission->CurrentVesselID(node(), grapplingVesselID);
    }
    const bool hasNum = Persistent(num) != nullptr;
    const bool hasNum2 = Persistent(num2) != nullptr;
    if (hasNum & hasNum2) {
        if (dockedUndocked == "Docked") {
            if (num == num2) testSuccess = true;
        } else if (num != num2) {
            testSuccess = true;
        }
    } else {
        if ((grappledSpaceObject != SpaceObjectChoices::Asteroid && grappledSpaceObject != SpaceObjectChoices::Comet) || !(AwardEnvironment::ActiveVessel() != nullptr)) return;
        Vessel* active = AwardEnvironment::ActiveVessel();
        int num3 = static_cast<int>(active->parts.size()) - 1;
        while (num3 >= 0) {
            Part* part = active->parts[static_cast<size_t>(num3)];
            if (!(part != nullptr) || !(Env::partIsAsteroidOrComet && Env::partIsAsteroidOrComet(part))) {
                --num3;
                continue;
            }
            grappleSOPartID = part->fields.persistentId;
            break;
        }
        Part* grabbingUnit = nullptr;  // `grabbingUnit` is cached for good once found (private field)
        std::optional<std::string> state;
        if (grabbingPart_ == nullptr) {
            for (size_t i = 0; i < active->parts.size(); ++i) {
                Part* part2 = active->parts[i];
                if (Env::grappleNodeState && Env::grappleNodeState(part2).has_value()) {
                    grabbingPart_ = part2;
                    break;
                }
            }
        }
        grabbingUnit = grabbingPart_;
        if (grabbingUnit != nullptr) state = Env::grappleNodeState(grabbingUnit);
        if (grappleSOPartID != 0 && state.has_value() && (state->find("Grappled") != std::string::npos || state->find("Ready") != std::string::npos)) testSuccess = true;
    }
}
// TestGrapple.cs:@Test
bool TestGrapple::Test() {
    if (firstRunTest) {
        testPartVessels();
        firstRunTest = false;
    }
    return testSuccess;
}
// TestGrapple.cs:@OnVesselPersistentIdChanged
void TestGrapple::OnVesselPersistentIdChanged(uint32_t oldId, uint32_t newId) {
    if (grapplingVesselID == oldId) {
        grapplingVesselID = newId;
        log::Info("[TestPartDocking]: Node (" + node()->id().ToString() + ") Grappling VesselId changed from " + std::to_string(oldId) + " to " + std::to_string(newId));
    }
    if (grappleAsteroidID == oldId) {
        grappleAsteroidID = newId;
        log::Info("[TestPartDocking]: Node (" + node()->id().ToString() + ") Grappled Asteroid VesselId changed from " + std::to_string(oldId) + " to " + std::to_string(newId));
    }
    if (grappleCometID == oldId) {
        grappleCometID = newId;
        log::Info("[TestPartDocking]: Node (" + node()->id().ToString() + ") Grappled Comet VesselId changed from " + std::to_string(oldId) + " to " + std::to_string(newId));
    }
    if (grappleVesselID == oldId) {
        grappleVesselID = newId;
        log::Info("[TestPartDocking]: Node (" + node()->id().ToString() + ") Grappled Vessel VesselId changed from " + std::to_string(oldId) + " to " + std::to_string(newId));
    }
}
// TestGrapple.cs:@OnVesselDocking
void TestGrapple::OnVesselDocking(uint32_t oldId, uint32_t newId) {
    if (grapplingVesselID == oldId) {
        grapplingVesselID = newId;
        log::Info("[TestPartDocking]: Node (" + node()->id().ToString() + ") Grappling VesselId changed from " + std::to_string(oldId) + " to " + std::to_string(newId));
    }
}
// TestGrapple.cs:@GetInfo
std::string TestGrapple::GetInfo() { return Tag("#autoLOC_8005462"); }
// TestGrapple.cs:@GetNodeBodyParameterString
std::string TestGrapple::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (node() != nullptr) node()->UpdateInfoOnRefresh = true;
    if (field.name == "grapplingVesselID" && field.hasValue) {
        std::string vesselName;
        bool flag = false;
        VesselSituation* vesselSituation = node()->mission->GetVesselSituationByVesselID(grapplingVesselID);
        if (vesselSituation != nullptr) {
            vesselName = vesselSituation->vesselName;
        } else if (node()->mission->UpdateFromMappedVesselIDs(grapplingVesselID, vesselName, vesselSituation)) {
            flag = true;
        }
        std::string text = (vesselSituation != nullptr || flag) ? vesselName : (grapplingVesselID != 0 ? Tag("#autoLOC_8100159") : Tag("#autoLOC_8004217"));
        std::string text2 = Localizer::Format("#autoLOC_8004190", {Tag("#autoLOC_8000001"), text});
        if (flag && vesselSituation != nullptr) text2 = text2 + "\n" + Localizer::Format("#autoLOC_6002491", {vesselSituation->vesselName}) + "\n";
        return text2;
    }
    if (field.name == "grappledSpaceObject") {
        std::string text3 = TestModule::GetNodeBodyParameterString(field) + "\n";
        switch (grappledSpaceObject) {
            case SpaceObjectChoices::Asteroid: {
                std::string text6;
                if (grappleAsteroidID != 0 && Persistent(grappleAsteroidID) != nullptr) {
                    text6 = text3 + Localizer::Format("#autoLOC_8004190", {"#autoLOC_8000046", Persistent(grappleAsteroidID)->GetDisplayName()});
                }
                return text3 + text6;
            }
            case SpaceObjectChoices::Comet: {
                std::string text5;
                Comet* cometByPersistentID = node()->mission->GetCometByPersistentID(grappleCometID);
                text5 = cometByPersistentID != nullptr ? Localizer::Format(cometByPersistentID->name) : Tag("#autoLOC_6003000");
                return text3 + text5;
            }
            case SpaceObjectChoices::Vessel: {
                std::string text4;
                const uint32_t num = node()->mission->CurrentVesselID(node(), grapplingVesselID);
                if (num != 0 && Persistent(num) != nullptr) text4 = text3 + Localizer::Format("#autoLOC_8000069", {Persistent(num)->GetDisplayName()});
                return text3 + text4;
            }
        }
    }
    if (field.name == "dockedUndocked" && field.hasValue) {
        if (field.value == "Docked") return Localizer::Format("#autoLOC_8004190", {field.guiName, Tag("#autoLOC_8002014")});
        return Localizer::Format("#autoLOC_8004190", {field.guiName, Tag("#autoLOC_8002015")});
    }
    return TestModule::GetNodeBodyParameterString(field);
}
// TestGrapple.cs:@RunValidation
void TestGrapple::RunValidation(Editor::MissionEditorValidator* validator) {
    VesselSituation* vesselSituationByVesselID = node()->mission->GetVesselSituationByVesselID(grapplingVesselID);
    if (vesselSituationByVesselID != nullptr) {
        if (grappledSpaceObject == SpaceObjectChoices::Vessel && vesselSituationByVesselID->persistentId == grappleVesselID) {
            validator->AddNodeFail(node(), Tag("#autoLOC_6002526"));
        } else if (grappledSpaceObject == SpaceObjectChoices::Asteroid && vesselSituationByVesselID->persistentId == grappleAsteroidID) {
            validator->AddNodeFail(node(), Tag("#autoLOC_6002526"));
        } else if (grappledSpaceObject == SpaceObjectChoices::Comet && vesselSituationByVesselID->persistentId == grappleAsteroidID) {
            validator->AddNodeFail(node(), Tag("#autoLOC_6002526"));
        }
    } else if (grappledSpaceObject == SpaceObjectChoices::Vessel && grapplingVesselID == 0 && grappleVesselID == 0) {
        validator->AddNodeFail(node(), Tag("#autoLOC_6002526"));
    }
}
// TestGrapple.cs:@Save
void TestGrapple::Save(ConfigNode& node) const {
    TestModule::Save(node);
    node.AddValue("grapplingVesselID", grapplingVesselID);
    node.AddValue("grappleAsteroidID", grappleAsteroidID);
    node.AddValue("grappleCometID", grappleCometID);
    node.AddValue("grappleVesselID", grappleVesselID);
    node.AddValue("grappledSpaceObject", dotnet::EnumToString(grappledSpaceObject));
    node.AddValue("dockedUndocked", dockedUndocked);
}
// TestGrapple.cs:@Load
void TestGrapple::Load(const ConfigNode& node) {
    TestModule::Load(node);
    ConfigNode* configNode = nullptr;
    if (node.TryGetNode("PARTONE", configNode)) {
        VesselPartIDPair vesselPartIDPair;
        vesselPartIDPair.Load(*configNode);
        grapplingVesselID = vesselPartIDPair.VesselID;
    }
    dockedUndocked = "Docked";
    node.TryGetValue("dockedUndocked", dockedUndocked);
    node.TryGetValue("grapplingVesselID", grapplingVesselID);
    node.TryGetValue("grappleAsteroidID", grappleAsteroidID);
    node.TryGetValue("grappleCometID", grappleCometID);
    node.TryGetValue("grappleVesselID", grappleVesselID);
    save_detail::TryGetEnum(node, "grappledSpaceObject", grappledSpaceObject, SpaceObjectChoices::Asteroid);
}

// ---- TestPartDocking -------------------------------------------------------------------------------------------------------------

// TestPartDocking.cs:@Awake
TestPartDocking::TestPartDocking() : TestModule(kTypeName) {
    title = "#autoLOC_8002013";
    partOnevesselPartIDs = std::make_unique<VesselPartIDPair>();
    partTwovesselPartIDs = std::make_unique<VesselPartIDPair>();
    firstRunTest = true;
    testSuccess = false;
}
TestPartDocking::~TestPartDocking() {
    if (subscribed) Cleared();
}
// TestPartDocking.cs:@Initialized
void TestPartDocking::Initialized() {
    testSuccess = false;
    if (dockedUndocked == "Docked") {
        GameEvents::onDockingComplete.Add(this, &TestPartDocking::onPartCouple);
        GameEvents::onPartCoupleComplete.Add(this, &TestPartDocking::onPartCouple);
    } else {
        GameEvents::onPartUndockComplete.Add(this, &TestPartDocking::onPartUndock);
        GameEvents::onPartDeCoupleComplete.Add(this, &TestPartDocking::onPartUndock);
    }
    subscribed = true;
}
// TestPartDocking.cs:@setPlayerCreatedRootParts
void TestPartDocking::setPlayerCreatedRootParts() {
    if (partOnevesselPartIDs->partID == 0) {
        bool flag = false;
        const uint32_t key = node()->mission->CurrentVesselID(node(), partOnevesselPartIDs->VesselID);
        if (Vessel* vessel = Persistent(key); vessel != nullptr) {
            if (vessel->loaded) {
                partOnevesselPartIDs->partID = vessel->rootPart->fields.persistentId;
                flag = true;
            } else if (static_cast<int>(vessel->protoVessel->protoPartSnapshots.size()) > vessel->protoVessel->rootIndex) {
                partOnevesselPartIDs->partID = vessel->protoVessel->protoPartSnapshots[static_cast<size_t>(vessel->protoVessel->rootIndex)]->persistentId;
                flag = true;
            }
        }
        if (!flag) {
            log::Info("[TestPartDocking]: Node (" + node()->id().ToString() + ") Part One Player Created Vessel. Unable to find root part. Bypassing Test Node.");
            testSuccess = true;
        }
    }
    if (partTwovesselPartIDs->partID != 0) return;
    bool flag2 = false;
    const uint32_t key2 = node()->mission->CurrentVesselID(node(), partTwovesselPartIDs->VesselID);
    if (Vessel* vessel2 = Persistent(key2); vessel2 != nullptr) {
        if (vessel2->loaded) {
            partTwovesselPartIDs->partID = vessel2->rootPart->fields.persistentId;
            flag2 = true;
        } else if (static_cast<int>(vessel2->protoVessel->protoPartSnapshots.size()) > vessel2->protoVessel->rootIndex) {
            // sic: the C# writes partOnevesselPartIDs here
            partOnevesselPartIDs->partID = vessel2->protoVessel->protoPartSnapshots[static_cast<size_t>(vessel2->protoVessel->rootIndex)]->persistentId;
            flag2 = true;
        }
    }
    if (!flag2) {
        log::Info("[TestPartDocking]: Node (" + node()->id().ToString() + ") Part Two Player Created Vessel. Unable to find root part. Bypassing Test Node.");
        testSuccess = true;
    }
}
// TestPartDocking.cs:@Cleared
void TestPartDocking::Cleared() {
    GameEvents::onDockingComplete.Remove(this, &TestPartDocking::onPartCouple);
    GameEvents::onPartCoupleComplete.Remove(this, &TestPartDocking::onPartCouple);
    GameEvents::onPartUndockComplete.Remove(this, &TestPartDocking::onPartUndock);
    GameEvents::onPartDeCoupleComplete.Remove(this, &TestPartDocking::onPartUndock);
    subscribed = false;
}
// TestPartDocking.cs:@onPartCouple
void TestPartDocking::onPartCouple(GameEvents::FromToAction<Part*, Part*>) { testPartVessels(); }
// TestPartDocking.cs:@onPartUndock
void TestPartDocking::onPartUndock(Part*) { testPartVessels(); }
// TestPartDocking.cs:@testPartVessels
void TestPartDocking::testPartVessels() {
    uint32_t vesselId = 0;
    uint32_t vesselId2 = 0;
    if (partOnevesselPartIDs->partID == 0 || partTwovesselPartIDs->partID == 0) setPlayerCreatedRootParts();
    const bool a = findPartVesselId(partOnevesselPartIDs->partID, vesselId);
    const bool b = findPartVesselId(partTwovesselPartIDs->partID, vesselId2);
    if (!(a & b)) return;
    if (dockedUndocked == "Docked") {
        if (vesselId == vesselId2) testSuccess = true;
    } else if (vesselId != vesselId2) {
        testSuccess = true;
    }
}
// TestPartDocking.cs:@findPartVesselId
bool TestPartDocking::findPartVesselId(uint32_t partId, uint32_t& vesselId) {
    vesselId = 0;
    bool result = false;
    Part* partout = NodeLibraryEnvironment::findLoadedPart ? NodeLibraryEnvironment::findLoadedPart(partId) : nullptr;
    if (partout != nullptr) {
        result = true;
        vesselId = partout->vessel->persistentId;
    } else if (Env::findUnloadedPartVessel) {
        if (std::optional<uint32_t> v = Env::findUnloadedPartVessel(partId); v.has_value()) {
            result = true;
            vesselId = *v;
        }
    }
    return result;
}
// TestPartDocking.cs:@Test
bool TestPartDocking::Test() {
    if (firstRunTest) {
        testPartVessels();
        firstRunTest = false;
    }
    return testSuccess;
}
// TestPartDocking.cs:@OnPartPersistentIdChanged
void TestPartDocking::OnPartPersistentIdChanged(uint32_t vesselID, uint32_t oldId, uint32_t newId) {
    if (partOnevesselPartIDs->VesselID == vesselID && partOnevesselPartIDs->partID == oldId) {
        partOnevesselPartIDs->partID = newId;
        log::Info("[TestPartDocking]: Node (" + node()->id().ToString() + ") Part One PartId changed from " + std::to_string(oldId) + " to " + std::to_string(newId));
    }
    if (partTwovesselPartIDs->VesselID == vesselID && partTwovesselPartIDs->partID == oldId) {
        partTwovesselPartIDs->partID = newId;
        log::Info("[TestPartDocking]: Node (" + node()->id().ToString() + ") Part Two PartId changed from " + std::to_string(oldId) + " to " + std::to_string(newId));
    }
}
// TestPartDocking.cs:@OnVesselPersistentIdChanged
void TestPartDocking::OnVesselPersistentIdChanged(uint32_t oldId, uint32_t newId) {
    if (partOnevesselPartIDs->VesselID == oldId) {
        partOnevesselPartIDs->VesselID = newId;
        log::Info("[TestPartDocking]: Node (" + node()->id().ToString() + ") Part One VesselId changed from " + std::to_string(oldId) + " to " + std::to_string(newId));
    }
    if (partTwovesselPartIDs->VesselID == oldId) {
        partTwovesselPartIDs->VesselID = newId;
        log::Info("[TestPartDocking]: Node (" + node()->id().ToString() + ") Part Two VesselId changed from " + std::to_string(oldId) + " to " + std::to_string(newId));
    }
}
// TestPartDocking.cs:@OnVesselDocking
void TestPartDocking::OnVesselDocking(uint32_t oldId, uint32_t) {
    if ((partOnevesselPartIDs->VesselID == oldId && partOnevesselPartIDs->partID == 0) || (partTwovesselPartIDs->VesselID == oldId && partTwovesselPartIDs->partID == 0))
        setPlayerCreatedRootParts();
}
// TestPartDocking.cs:@GetInfo
std::string TestPartDocking::GetInfo() { return Tag("#autoLOC_8002016"); }
// TestPartDocking.cs:@GetNodeBodyParameterString
std::string TestPartDocking::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (node() != nullptr) node()->UpdateInfoOnRefresh = true;
    auto partText = [&](VesselPartIDPair& ids, const char* partTag, bool useOneName) {
        VesselSituation* vesselSituation = node()->mission->GetVesselSituationByVesselID(ids.VesselID);
        std::string text;
        std::string vesselName;
        bool flag = false;
        if (vesselSituation != nullptr) {
            vesselName = vesselSituation->vesselName;
        } else if (node()->mission->UpdateFromMappedVesselIDs(ids.VesselID, vesselName, vesselSituation)) {
            flag = true;
        }
        if (vesselSituation != nullptr || flag) {
            text = Tag("#autoLOC_8000001") + ": " + Localizer::Format(vesselName) + "\n";
            if (flag && vesselSituation != nullptr) text = text + Localizer::Format("#autoLOC_6002491", {vesselSituation->vesselName}) + "\n";
        }
        if (vesselSituation != nullptr && vesselSituation->playerCreated) return text + Localizer::Format(partTag, {Tag("#autoLOC_8001005")});
        // the C# of partTwo tests partOnevesselPartIDs.partName in the second operand (sic)
        const std::string& nameForAny = useOneName ? partOnevesselPartIDs->partName : ids.partName;
        const std::string text2 = (ids.partName == "Any" || nameForAny == "#autoLOC_8001004") ? Tag("#autoLOC_8001004") : Localizer::Format(ids.partName);
        return text + Localizer::Format(partTag, {text2});
    };
    if (field.name == "partOnevesselPartIDs" && node()->mission != nullptr) return partText(*partOnevesselPartIDs, "#autoLOC_8006059", false);
    if (field.name == "partTwovesselPartIDs" && node()->mission != nullptr) return partText(*partTwovesselPartIDs, "#autoLOC_8006060", true);
    if (field.name == "dockedUndocked" && field.hasValue) {
        if (field.value == "Docked") return Localizer::Format("#autoLOC_8004190", {field.guiName, Tag("#autoLOC_8002014")});
        return Localizer::Format("#autoLOC_8004190", {field.guiName, Tag("#autoLOC_8002015")});
    }
    return TestModule::GetNodeBodyParameterString(field);
}
// TestPartDocking.cs:@RunValidation
void TestPartDocking::RunValidation(Editor::MissionEditorValidator* validator) {
    if (partOnevesselPartIDs != nullptr && partTwovesselPartIDs != nullptr) {
        VesselSituation* vesselSituationByVesselID = node()->mission->GetVesselSituationByVesselID(partOnevesselPartIDs->VesselID);
        VesselSituation* vesselSituationByVesselID2 = node()->mission->GetVesselSituationByVesselID(partTwovesselPartIDs->VesselID);
        if (vesselSituationByVesselID == nullptr) validator->AddNodeFail(node(), Tag("#autoLOC_8002043"));
        if (vesselSituationByVesselID2 == nullptr) validator->AddNodeFail(node(), Tag("#autoLOC_8002044"));
        if (partOnevesselPartIDs->partID == 0 && (vesselSituationByVesselID == nullptr || !vesselSituationByVesselID->playerCreated))
            validator->AddNodeFail(node(), Tag("#autoLOC_8002047"));
        if (partTwovesselPartIDs->partID == 0 && (vesselSituationByVesselID2 == nullptr || !vesselSituationByVesselID2->playerCreated))
            validator->AddNodeFail(node(), Tag("#autoLOC_8002048"));
        if (partOnevesselPartIDs->VesselID == partTwovesselPartIDs->VesselID && partOnevesselPartIDs->partID == partTwovesselPartIDs->partID)
            validator->AddNodeFail(node(), Tag("#autoLOC_8002049"));
    }
    if (partOnevesselPartIDs != nullptr) partOnevesselPartIDs->ValidatePartAgainstCraft(node(), validator);
    if (partTwovesselPartIDs != nullptr) partTwovesselPartIDs->ValidatePartAgainstCraft(node(), validator);
}
// TestPartDocking.cs:@Save
void TestPartDocking::Save(ConfigNode& node) const {
    TestModule::Save(node);
    auto configNode = std::make_shared<ConfigNode>("PARTONE");
    partOnevesselPartIDs->Save(*configNode);
    node.AddNode(configNode);
    auto configNode2 = std::make_shared<ConfigNode>("PARTTWO");
    partTwovesselPartIDs->Save(*configNode2);
    node.AddNode(configNode2);
    node.AddValue("dockedUndocked", dockedUndocked);
}
// TestPartDocking.cs:@Load
void TestPartDocking::Load(const ConfigNode& node) {
    TestModule::Load(node);
    ConfigNode* configNode = nullptr;
    if (node.TryGetNode("PARTONE", configNode)) partOnevesselPartIDs->Load(*configNode);
    ConfigNode* configNode2 = nullptr;
    if (node.TryGetNode("PARTTWO", configNode2)) partTwovesselPartIDs->Load(*configNode2);
    dockedUndocked = "Docked";
    node.TryGetValue("dockedUndocked", dockedUndocked);
}

// ---- registration ----------------------------------------------------------------------------------------------------------------

namespace {
template <class T>
void RegisterTest() {
    TestModuleRegistry::Register({T::kTypeName, [] { return std::make_unique<T>(); }});
}
}  // namespace

void RegisterNodeLibrarySlice2Tests() {
    RegisterTest<TestVesselSituationFlying>();
    RegisterTest<TestVesselSituationLanded>();
    RegisterTest<TestVesselSituationSplashed>();
    RegisterTest<TestVesselStateCrashed>();
    RegisterTest<TestVesselVelocity>();
    RegisterTest<TestGoTo>();
    RegisterTest<TestFlyThrough>();
    RegisterTest<TestPlantFlag>();
    RegisterTest<TestKerbalKilled>();
    RegisterTest<TestKerbalRecovery>();
    RegisterTest<TestKerbalRescue>();
    RegisterTest<TestKerbalEVA>();
    RegisterTest<TestKerbalBoarded>();
    RegisterTest<TestTakeKerbal>();
    RegisterTest<TestCrewAssignment>();
    RegisterTest<TestOrbit>();
    RegisterTest<TestOrbitParams>();
    RegisterTest<TestScienceExperiment>();
    RegisterTest<TestGrapple>();
    RegisterTest<TestPartDocking>();
}

}  // namespace ksp::Expansions::Missions::Tests
