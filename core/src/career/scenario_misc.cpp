// WP-36: ScenarioDestructibles.cs, ScenarioAchievements.cs, VesselRecovery.cs.
#include "ksp/career/scenario_misc.hpp"

#include <algorithm>

#include "ksp/career/career_environment.hpp"
#include "ksp/career/format.hpp"
#include "ksp/career/funding.hpp"
#include "ksp/career/upgradeables.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/modules/scenario_runner.hpp"
#include "ksp/save/crew.hpp"
#include "ksp/save/proto_vessel.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/util/ksp_util.hpp"
#include "ksp/vessel/module_trip_logger.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

using GE = events::GameEvents;
using Localizer = KSP::Localization::Localizer;

namespace {
Game* CurrentGame() { return SaveEnvironment::Current().currentGame; }
}  // namespace

// ---- ScenarioDestructibles ----------------------------------------------------------------------------------------------------

dotnet::Dictionary<std::string, std::shared_ptr<ScenarioDestructibles::ProtoDestructible>>& ScenarioDestructibles::protoDestructibles() {
    static dotnet::Dictionary<std::string, std::shared_ptr<ProtoDestructible>> d;
    return d;
}
dotnet::Dictionary<std::string, std::vector<std::shared_ptr<ScenarioDestructibles::ProtoDestructible>>>& ScenarioDestructibles::facilityToDestructibles() {
    static dotnet::Dictionary<std::string, std::vector<std::shared_ptr<ProtoDestructible>>> d;
    return d;
}
void ScenarioDestructibles::ClearStatics() {
    protoDestructibles().Clear();
    facilityToDestructibles().Clear();
}

// ScenarioDestructibles.cs:23
ScenarioDestructibles::ProtoDestructible::ProtoDestructible(IDestructibleBuilding* dBuilding) {
    dBuildingRefs.push_back(dBuilding);
    configNode = ConfigNode::Create(dBuilding->BuildingId());
    dBuilding->Save(*configNode);
    FacilityDamageFraction_ = dBuilding->FacilityDamageFraction();
}

// ScenarioDestructibles.cs:32
ScenarioDestructibles::ProtoDestructible::ProtoDestructible(ConfigNode::NodePtr node) : configNode(std::move(node)) {}

// ScenarioDestructibles.cs:38
void ScenarioDestructibles::ProtoDestructible::AddRef(IDestructibleBuilding* dBuilding) {
    if (dBuildingRefs.empty()) FacilityDamageFraction_ = dBuilding->FacilityDamageFraction();
    dBuildingRefs.push_back(dBuilding);
}

// ScenarioDestructibles.cs:47
void ScenarioDestructibles::ProtoDestructible::Load(ConfigNode::NodePtr node) {
    configNode = std::move(node);
    int count = static_cast<int>(dBuildingRefs.size());
    while (count-- > 0) dBuildingRefs[static_cast<size_t>(count)]->Load(*configNode);
    GetDamage();
}

// ScenarioDestructibles.cs:58 (configNode keeps a copy of the saved node)
void ScenarioDestructibles::ProtoDestructible::Save(ConfigNode& node) {
    if (!dBuildingRefs.empty()) {
        dBuildingRefs[0]->Save(node);
        configNode = node.CreateCopy();
        isIntact_ = dBuildingRefs[0]->IsIntact();
    } else {
        configNode->CopyTo(node);
    }
}

// ScenarioDestructibles.cs:72
float ScenarioDestructibles::ProtoDestructible::GetDamage() {
    if (!dBuildingRefs.empty()) {
        isIntact_ = dBuildingRefs[0]->IsIntact();
    } else {
        if (configNode == nullptr) return 0.0f;
        if (configNode->HasValue("intact")) {
            bool b = false;
            if (!dotnet::TryParseBool(*configNode->GetValue("intact"), b)) throw dotnet::FormatException("String was not recognized as a valid Boolean.");
            isIntact_ = b;
        }
    }
    if (!isIntact_) return FacilityDamageFraction_;
    return 0.0f;
}

ScenarioDestructibles::ScenarioDestructibles() : ScenarioModule("ScenarioDestructibles") {}
ScenarioDestructibles::~ScenarioDestructibles() { OnDestroy(); }

// ScenarioDestructibles.cs:103
void ScenarioDestructibles::OnAwake() {
    if (Instance != nullptr) return;  // Object.Destroy(this)
    Instance = this;
    protoDestructibles().Clear();
    facilityToDestructibles().Clear();
}

// ScenarioDestructibles.cs:115
void ScenarioDestructibles::OnDestroy() {
    if (Instance != nullptr && Instance == this) Instance = nullptr;
}

// ScenarioDestructibles.cs:123
void ScenarioDestructibles::OnSave(ConfigNode& node) {
    for (const std::string& current : protoDestructibles().Keys()) protoDestructibles().At(current)->Save(*node.AddNode(current));
}

// ScenarioDestructibles.cs:133
std::string ScenarioDestructibles::GetFacility(const std::string& dName) {
    int num = 0;
    int num2 = 0;
    const int length = static_cast<int>(dName.size());
    for (int i = 0; i < length; ++i) {
        if (dName[static_cast<size_t>(i)] == '/') {
            num2++;
            if (num2 == 2) {
                num = i;
                break;
            }
        }
    }
    if (num > 0) return dName.substr(0, static_cast<size_t>(num));
    return dName;
}

// ScenarioDestructibles.cs:157 (List.AddUnique)
bool ScenarioDestructibles::AddTo(dotnet::Dictionary<std::string, std::vector<std::shared_ptr<ProtoDestructible>>>& dict, const std::string& key,
                                  const std::shared_ptr<ProtoDestructible>& pD) {
    auto* value = dict.Find(key);
    if (value != nullptr) {
        if (std::find(value->begin(), value->end(), pD) == value->end()) value->push_back(pD);
        return false;
    }
    dict.Set(key, std::vector<std::shared_ptr<ProtoDestructible>>{pD});
    return true;
}

// ScenarioDestructibles.cs:170
void ScenarioDestructibles::RemoveFrom(dotnet::Dictionary<std::string, std::vector<std::shared_ptr<ProtoDestructible>>>& dict, const std::string& key,
                                       const std::shared_ptr<ProtoDestructible>& pD) {
    auto* value = dict.Find(key);
    if (value != nullptr) {
        auto it = std::find(value->begin(), value->end(), pD);
        if (it != value->end()) value->erase(it);
        if (value->empty()) dict.Remove(key);
    }
}

// ScenarioDestructibles.cs:182
void ScenarioDestructibles::OnLoad(ConfigNode& node) {
    log::Info("[ScenarioDestructibles]: Loading... " + std::to_string(protoDestructibles().Count()) + " objects registered");
    const int count = node.CountNodes();
    for (int i = 0; i < count; ++i) {
        ConfigNode::NodePtr configNode = node.nodes().Shared(i);
        auto* existing = protoDestructibles().Find(configNode->name);
        if (existing != nullptr) {
            (*existing)->Load(configNode);
            continue;
        }
        auto protoDestructible = std::make_shared<ProtoDestructible>(configNode);
        protoDestructibles().Add(configNode->name, protoDestructible);
        AddTo(facilityToDestructibles(), GetFacility(configNode->name), protoDestructible);
    }
}

// ScenarioDestructibles.cs:200
bool ScenarioDestructibles::RegisterDestructible(IDestructibleBuilding* dBuilding, const std::string& id) {
    auto* existing = protoDestructibles().Find(id);
    if (existing == nullptr) {
        auto protoDestructible = std::make_shared<ProtoDestructible>(dBuilding);
        protoDestructibles().Add(id, protoDestructible);
        AddTo(facilityToDestructibles(), GetFacility(id), protoDestructible);
    } else {
        ProtoDestructible& p = **existing;
        if (std::find(p.dBuildingRefs.begin(), p.dBuildingRefs.end(), dBuilding) == p.dBuildingRefs.end()) p.AddRef(dBuilding);
        if (Instance != nullptr && p.configNode != nullptr) {
            dBuilding->Load(*p.configNode);
            return true;
        }
    }
    return false;
}

// ScenarioDestructibles.cs:223
void ScenarioDestructibles::UnregisterDestructible(IDestructibleBuilding* dBuilding, const std::string& id, bool saveState) {
    auto* existing = protoDestructibles().Find(id);
    if (existing != nullptr && std::find((*existing)->dBuildingRefs.begin(), (*existing)->dBuildingRefs.end(), dBuilding) != (*existing)->dBuildingRefs.end()) {
        if (saveState) {
            ConfigNode scratch(id);
            (*existing)->Save(scratch);
        }
        auto& refs = (*existing)->dBuildingRefs;
        refs.erase(std::find(refs.begin(), refs.end(), dBuilding));
    }
}

// ScenarioDestructibles.cs:235
float ScenarioDestructibles::GetFacilityDamage(std::string facilityName) {
    if (Instance != nullptr) {
        float num = 0.0f;
        facilityName = ScenarioUpgradeableFacilities::SlashSanitize(facilityName);
        auto* value = facilityToDestructibles().Find(facilityName);
        if (value != nullptr) {
            int count = static_cast<int>(value->size());
            while (count-- > 0) num = f32store(fx(num) + fx((*value)[static_cast<size_t>(count)]->GetDamage()));
        }
        return num;
    }
    return 0.0f;
}

void ScenarioDestructibles::Register() {
    ScenarioModuleFactory::Register("ScenarioDestructibles", [](const std::string&) { return std::unique_ptr<ScenarioModule>(new ScenarioDestructibles()); });
}

// ---- ScenarioAchievements -------------------------------------------------------------------------------------------------------

// ScenarioAchievements.cs:29
void ScenarioAchievements::DebugAchievementProvider::SetAchievementComplete(const std::string& id) {
    achievements_[id] = true;
    log::Info("[DebugAchievementProvider]: Completing achievement \"" + id + "\"!");
}

// ScenarioAchievements.cs:35
bool ScenarioAchievements::DebugAchievementProvider::GetAchievementComplete(const std::string& id) {
    auto it = achievements_.find(id);
    const bool flag = it != achievements_.end() && it->second;
    log::Info("[DebugAchievementProvider]: Achievement \"" + id + "\" is " + (flag ? "" : "not") + " complete...");
    return flag;
}

ScenarioAchievements::ScenarioAchievements() : ScenarioModule("ScenarioAchievements") {}
ScenarioAchievements::~ScenarioAchievements() { OnDestroy(); }

// ScenarioAchievements.cs:57 (Object.Destroy(this) in the scenario modes does not stop the rest of OnAwake: the module is destroyed
// at the end of the frame, the host checks destroyRequested)
void ScenarioAchievements::OnAwake() {
    const Game::Modes m = CurrentGame()->Mode;
    if (m == Game::Modes::SCENARIO || m == Game::Modes::SCENARIO_NON_RESUMABLE || m == Game::Modes::MISSION || m == Game::Modes::MISSION_BUILDER)
        destroyRequested_ = true;
    achievements_ = {{"FirstContract", false}, {"FirstCrewFromSpace", false}, {"FirstEelooFlyby", false}, {"FirstEVA", false},
                     {"FirstLaunchPadLaunch", false}, {"FirstMohoLanding", false}, {"FirstMunWalk", false}, {"FirstOrbitalDock", false},
                     {"FirstRover", false}, {"FirstRunwayLaunch", false}, {"FirstSatelliteLaunch", false}, {"MunFlagPlant", false},
                     {"TargetedLaunchPadLanding", false}, {"FlybyBop", false}, {"FlybyDres", false}, {"FlybyDuna", false}, {"FlybyEve", false},
                     {"FlybyGilly", false}, {"FlybyIke", false}, {"FlybyJool", false}, {"FlybyKerbin", false}, {"FlybyLaythe", false},
                     {"FlybyMinmus", false}, {"FlybyMoho", false}, {"FlybyMun", false}, {"FlybyPol", false}, {"FlybyTylo", false},
                     {"FlybyVall", false}, {"ReturnFromBop", false}, {"ReturnFromDres", false}, {"ReturnFromDuna", false},
                     {"ReturnFromEeloo", false}, {"ReturnFromEve", false}, {"ReturnFromGilly", false}, {"ReturnFromIke", false},
                     {"ReturnFromKerbin", false}, {"ReturnFromLaythe", false}, {"ReturnFromMinmus", false}, {"ReturnFromMoho", false},
                     {"ReturnFromMun", false}, {"ReturnFromPol", false}, {"ReturnFromTylo", false}, {"ReturnFromVall", false}, {"ReturnFromJool", false}};
    GE::VesselSituation::onLaunch.Add(this, &ScenarioAchievements::OnLaunch);
    GE::VesselSituation::onFlyBy.Add(this, &ScenarioAchievements::OnFlyby);
    GE::VesselSituation::onOrbit.Add(this, &ScenarioAchievements::OnOrbit);
    GE::VesselSituation::onLand.Add(this, &ScenarioAchievements::OnLanding);
    GE::VesselSituation::onReturnFromOrbit.Add(this, &ScenarioAchievements::OnReturnFromOrbit);
    GE::VesselSituation::onTargetedLanding.Add(this, &ScenarioAchievements::OnTargetedLanding);
    GE::Contract::onCompleted.Add(this, &ScenarioAchievements::OnContractCompleted);
    GE::onVesselRecoveryProcessing.Add(this, &ScenarioAchievements::OnVesselRecovered);
    GE::onCrewOnEva.Add(this, &ScenarioAchievements::OnCrewEVA);
    GE::onFlagPlant.Add(this, &ScenarioAchievements::OnFlagPlant);
    GE::onPartCouple.Add(this, &ScenarioAchievements::OnPartsCoupled);
    eventsAdded_ = true;
}

// ScenarioAchievements.cs:124 (StartCoroutine(EndInTutorials()): the host calls EndInTutorials after the first frame)
void ScenarioAchievements::Start() {}

// ScenarioAchievements.cs:129
void ScenarioAchievements::OnDestroy() {
    if (eventsAdded_) {
        GE::VesselSituation::onLaunch.Remove(this, &ScenarioAchievements::OnLaunch);
        GE::VesselSituation::onFlyBy.Remove(this, &ScenarioAchievements::OnFlyby);
        GE::VesselSituation::onOrbit.Remove(this, &ScenarioAchievements::OnOrbit);
        GE::VesselSituation::onLand.Remove(this, &ScenarioAchievements::OnLanding);
        GE::VesselSituation::onReturnFromOrbit.Remove(this, &ScenarioAchievements::OnReturnFromOrbit);
        GE::VesselSituation::onTargetedLanding.Remove(this, &ScenarioAchievements::OnTargetedLanding);
        GE::Contract::onCompleted.Remove(this, &ScenarioAchievements::OnContractCompleted);
        GE::onVesselRecoveryProcessing.Remove(this, &ScenarioAchievements::OnVesselRecovered);
        GE::onCrewOnEva.Remove(this, &ScenarioAchievements::OnCrewEVA);
        GE::onFlagPlant.Remove(this, &ScenarioAchievements::OnFlagPlant);
        GE::onPartCouple.Remove(this, &ScenarioAchievements::OnPartsCoupled);
        eventsAdded_ = false;
    }
}

// ScenarioAchievements.cs:147
void ScenarioAchievements::SetProvider(IAchievementProvider* ap) { provider_ = ap; }

// ScenarioAchievements.cs:152
void ScenarioAchievements::CheckAchievement(const std::string& id) {
    auto it = achievements_.find(id);
    if (!(provider_ == nullptr || it == achievements_.end() || it->second)) {
        if (!provider_->GetAchievementComplete(id)) provider_->SetAchievementComplete(id);
        achievements_[id] = true;
    }
}

// ScenarioAchievements.cs:165
void ScenarioAchievements::EndInTutorials(bool exclusiveTutorialPresent) {
    if (CurrentGame() == nullptr) return;
    if (exclusiveTutorialPresent) destroyRequested_ = true;  // Object.Destroy(this)
}

// ScenarioAchievements.cs:188
void ScenarioAchievements::OnLaunch(Vessel* vessel) {
    if (vessel != nullptr) {
        const std::string& landedAtLast = vessel->landedAtLast;
        if (landedAtLast.find("LaunchPad") != std::string::npos)
            CheckAchievement("FirstLaunchPadLaunch");
        else if (landedAtLast.find("Runway") != std::string::npos)
            CheckAchievement("FirstRunwayLaunch");
    }
}

// ScenarioAchievements.cs:204
void ScenarioAchievements::OnFlyby(Vessel* vessel, CelestialBody* body) {
    (void)vessel;
    if (!(body == Planetarium::fetch->Sun)) {
        if (body->name() == "Eeloo")
            CheckAchievement("FirstEelooFlyby");
        else
            CheckAchievement("Flyby" + body->name());
    }
}

// ScenarioAchievements.cs:219
void ScenarioAchievements::OnOrbit(Vessel* vessel, CelestialBody* body) {
    if (body->isHomeWorld) {
        CareerEnvironment& env = CareerEnvironment::Current();
        const int num = env.actualCrewCapacity ? env.actualCrewCapacity() : 0;
        const int num2 = vessel->loaded ? vessel->GetCrewCount() : static_cast<int>(vessel->protoVessel->GetVesselCrew().size());
        if ((vessel->loaded ? vessel->IsControllable() : vessel->protoVessel->wasControllable) && num + num2 == 0) CheckAchievement("FirstSatelliteLaunch");
    }
}

// ScenarioAchievements.cs:232
void ScenarioAchievements::OnLanding(Vessel* vessel, CelestialBody* body) {
    if (!body->hasSolidSurface || vessel == nullptr) return;
    if (body->name() == "Moho") CheckAchievement("FirstMohoLanding");
    if (body->isHomeWorld) {
        VesselTripLog vesselTripLog = VesselTripLog::FromVessel(*vessel);
        if (vesselTripLog.Log.HasEntry(FlightLog::EntryType::Suborbit, body->name()) || vesselTripLog.Log.HasEntry(FlightLog::EntryType::Orbit, body->name()) ||
            vesselTripLog.Log.HasEntry(FlightLog::EntryType::Escape, body->name()))
            CheckAchievement("ReturnFromKerbin");
    }
    if (!body->isHomeWorld && (vessel->loaded ? vessel->vesselType : vessel->protoVessel->vesselType) == VesselType::Rover) CheckAchievement("FirstRover");
}

// ScenarioAchievements.cs:256
void ScenarioAchievements::OnReturnFromOrbit(Vessel* vessel, CelestialBody* body) {
    (void)vessel;
    if (!body->isHomeWorld && !(body == Planetarium::fetch->Sun)) CheckAchievement("ReturnFrom" + body->name());
}

// ScenarioAchievements.cs:264
void ScenarioAchievements::OnTargetedLanding(Vessel* vessel, std::string to, KSPAchievements::ReturnFrom from) {
    (void)vessel;
    if (to.find("LaunchPad") != std::string::npos && (from == KSPAchievements::ReturnFrom::Orbit || from == KSPAchievements::ReturnFrom::SubOrbit))
        CheckAchievement("TargetedLaunchPadLanding");
}

// ScenarioAchievements.cs:272
void ScenarioAchievements::OnContractCompleted(Contracts::Contract* contract) {
    (void)contract;
    CheckAchievement("FirstContract");
}

// ScenarioAchievements.cs:277
void ScenarioAchievements::OnVesselRecovered(ProtoVessel* protoVessel, KSP::UI::Screens::MissionRecoveryDialog* dialog, float amount) {
    (void)dialog;
    (void)amount;
    const std::vector<std::shared_ptr<ProtoCrewMember>> vesselCrew = protoVessel->GetVesselCrew();
    int count = static_cast<int>(vesselCrew.size());
    while (count-- > 0) {
        FlightLog& flightLog = vesselCrew[static_cast<size_t>(count)]->flightLog;
        int count2 = flightLog.Count();
        while (count2-- > 0) {
            const std::string& type = flightLog[count2].type;
            if (type == "Orbit" || type == "Suborbit") {
                CheckAchievement("FirstCrewFromSpace");
                return;
            }
        }
    }
}

// ScenarioAchievements.cs:297
void ScenarioAchievements::OnCrewEVA(GE::FromToAction<Part*, Part*> action) {
    if (!(action.to == nullptr) && !(action.from == nullptr)) {
        CheckAchievement("FirstEVA");
        Vessel* vessel = action.from->vessel;
        CelestialBody* celestialBody = vessel->loaded ? vessel->mainBody() : FlightRegistry::Bodies()[static_cast<size_t>(vessel->protoVessel->orbitSnapShot->ReferenceBodyIndex)];
        if ((vessel->loaded ? vessel->situation : vessel->protoVessel->situation) == Vessel_Situations::LANDED && celestialBody->name() == "Mun")
            CheckAchievement("FirstMunWalk");
    }
}

// ScenarioAchievements.cs:311
void ScenarioAchievements::OnFlagPlant(Vessel* vessel) {
    if (!(vessel == nullptr) &&
        (vessel->loaded ? vessel->mainBody() : FlightRegistry::Bodies()[static_cast<size_t>(vessel->protoVessel->orbitSnapShot->ReferenceBodyIndex)])->name() == "Mun")
        CheckAchievement("MunFlagPlant");
}

// ScenarioAchievements.cs:319
void ScenarioAchievements::OnPartsCoupled(GE::FromToAction<Part*, Part*> action) {
    if (!(action.from == nullptr) && !(action.to == nullptr) && !action.from->vessel->isEVA() && !action.to->vessel->isEVA() &&
        action.from->fields.missionID != action.to->fields.missionID) {
        Vessel* vessel = action.to->vessel;
        if ((vessel->loaded ? vessel->situation : vessel->protoVessel->situation) == Vessel_Situations::ORBITING) CheckAchievement("FirstOrbitalDock");
    }
}

void ScenarioAchievements::Register() {
    ScenarioModuleFactory::Register("ScenarioAchievements", [](const std::string&) { return std::unique_ptr<ScenarioModule>(new ScenarioAchievements()); });
}

// ---- VesselRecovery ---------------------------------------------------------------------------------------------------------------

VesselRecovery::VesselRecovery() : ScenarioModule("VesselRecovery") {}
VesselRecovery::~VesselRecovery() { OnDestroy(); }

// VesselRecovery.cs:17
void VesselRecovery::OnAwake() {
    GE::onVesselRecovered.Add(this, &VesselRecovery::OnVesselRecovered);
    eventsAdded_ = true;
}

// VesselRecovery.cs:22
void VesselRecovery::OnDestroy() {
    if (eventsAdded_) {
        GE::onVesselRecovered.Remove(this, &VesselRecovery::OnVesselRecovered);
        eventsAdded_ = false;
    }
}

// (VesselRecovery.cs:37-:70 helper: the distance factor part of OnVesselRecovered; num = great-circle distance from the space centre,
// num2 = body radius * pi)
VesselRecovery::RecoveryFactor VesselRecovery::ComputeRecoveryFactor(double num, double num2, float minDelta, float maxDelta, const std::string& landedAt) {
    const float b = dotnet::Math::Max(0.0f, dotnet::Math::Min(1.0f, f32store(fx(0.1f) + fx(minDelta))));
    const float num3a = Mathf::Lerp(dotnet::Math::Max(0.0f, dotnet::Math::Min(1.0f, f32store(fx(0.98f) + fx(maxDelta)))), b, static_cast<float>(num / num2));
    const float num4a = Mathf::Lerp(0.98f, 0.1f, static_cast<float>(num / num2));
    RecoveryFactor r{num3a, num4a};
    if (landedAt.find("KSC") != std::string::npos) {
        r.num3 = 0.98f;
        r.num4 = 0.98f;
    }
    if (landedAt.find("Runway") != std::string::npos || landedAt.find("LaunchPad") != std::string::npos) {
        r.num3 = 1.0f;
        r.num4 = 1.0f;
    }
    return r;
}

// VesselRecovery.cs:140
bool VesselRecovery::checkLaunchSites(ProtoVessel* pv, float& recoveryFactor, float& vanillaFactor) {
    bool result = false;
    recoveryFactor = 0.0f;
    vanillaFactor = 0.0f;
    const std::string text = Vessel::GetLandedAtString(pv->landedAt);
    CareerEnvironment& env = CareerEnvironment::Current();
    if (!env.launchSites) return result;
    const auto sites = env.launchSites();
    for (size_t i = 0; i < sites.size(); ++i) {
        if (text.find(Vessel::GetLandedAtString(sites[i].name)) != std::string::npos) {
            recoveryFactor = 1.0f;
            vanillaFactor = 1.0f;
            result = true;
            break;
        }
    }
    return result;
}

// VesselRecovery.cs:160
void VesselRecovery::recoverVesselCrew(ProtoVessel* pv, KSP::UI::Screens::MissionRecoveryDialog* mrDialog) {
    const std::vector<std::shared_ptr<ProtoCrewMember>> vesselCrew = pv->GetVesselCrew();
    for (size_t i = 0, count = vesselCrew.size(); i < count; ++i) {
        ProtoCrewMember& protoCrewMember = *vesselCrew[i];
        protoCrewMember.set_rosterStatus(ProtoCrewMember::RosterStatus::Available);
        protoCrewMember.flightLog.AddEntryUnique(FlightLog::EntryType::Recover);
        if (protoCrewMember.ChuteNode != nullptr) protoCrewMember.ChuteNode->SetValue("persistentState", "STOWED");
        if (mrDialog != nullptr) {
            log::Info("Crewmember " + protoCrewMember.name().value_or(std::string()) + " is available again");
            mrDialog->AddCrewWidget({protoCrewMember.name().value_or(std::string())});
        }
        protoCrewMember.ArchiveFlightLog();
    }
}

// VesselRecovery.cs:31
void VesselRecovery::OnVesselRecovered(ProtoVessel* pv, bool quick) {
    if (pv == nullptr) return;
    CareerEnvironment& env = CareerEnvironment::Current();
    if (!env.spaceCenterBody) throw NullReferenceException("SpaceCenter.Instance is null");
    CelestialBody* cb = env.spaceCenterBody();
    double num = 0.0;
    double num2 = 1.0;
    if (cb != nullptr) {
        num = env.spaceCenterGreatCircleDistance ? env.spaceCenterGreatCircleDistance(pv->latitude, pv->longitude) : 0.0;
        num2 = cb->Radius * 3.141592653589793;
    } else if (!quick) {
        log::Warning("[VesselRecovery]: No 'home' found. This should never happen in a build!");
    }
    ValueModifierQuery valueModifierQuery = ValueModifierQuery::RunQuery("RecoveryMinimumDelta", 1.0f);
    ValueModifierQuery valueModifierQuery2 = ValueModifierQuery::RunQuery("RecoveryMaximumDelta", 1.0f);
    RecoveryFactor rf = ComputeRecoveryFactor(num, num2, valueModifierQuery.GetEffectDelta(), valueModifierQuery2.GetEffectDelta(), pv->landedAt);
    float num3 = rf.num3;
    float num4 = rf.num4;
    float recoveryFactor = 0.0f;
    float vanillaFactor = 0.0f;
    if (checkLaunchSites(pv, recoveryFactor, vanillaFactor)) {
        num3 = recoveryFactor;
        num4 = vanillaFactor;
    }
    const float num5 = f32store(fx(num3) / fx(num4) - 1.0);
    std::string text;
    if (num5 != 0.0f) text = std::string(num5 > 0.0f ? "<color=#caff00>(+" : "<color=#feb200>(") + career::FormatNumber(f32store(fx(num5) * 100.0), "N1") + "%)</color>";
    const bool haveRnd = env.rndPresent && env.rndPresent();
    if (Funding::Instance == nullptr && Reputation::Instance == nullptr && !haveRnd && pv->vesselRef != nullptr && pv->vesselRef->GetCrewCount() <= 0) {
        GE::onVesselRecoveryProcessingComplete.Fire(pv, nullptr, num3);
        return;
    }
    KSP::UI::Screens::MissionRecoveryDialog* missionRecoveryDialog = nullptr;
    if (!quick) {
        lastDialog_ = KSP::UI::Screens::MissionRecoveryDialog::CreateFullDialog(pv);
        missionRecoveryDialog = lastDialog_.get();
    }
    if (!quick) {
        if (Funding::Instance != nullptr) missionRecoveryDialog->beforeMissionFunds = Funding::Instance->Funds();
        if (Reputation::Instance != nullptr) missionRecoveryDialog->beforeMissionReputation = Reputation::Instance->reputation();
        if (haveRnd) missionRecoveryDialog->beforeMissionScience = env.rndScience ? env.rndScience() : 0.0f;
        if (!pv->landedAt.empty()) {
            std::string empty;
            if (pv->displaylandedAt != "")
                empty = Localizer::Format(pv->displaylandedAt);
            else
                empty = env.rndMiniBiomeDisplayName ? env.rndMiniBiomeDisplayName(Vessel::GetLandedAtString(pv->landedAt), true) : Vessel::GetLandedAtString(pv->landedAt);
            missionRecoveryDialog->recoveryLocation = Localizer::Format("#autoLOC_303271", {empty});
            missionRecoveryDialog->recoveryFactor = career::LocalizeNumber(f32store(fx(num3) * 100.0), "0.0") + "% " + text;
        } else {
            missionRecoveryDialog->recoveryLocation = Localizer::Format("#autoLOC_303276", {career::FormatNumber(num * 0.0010000000474974513, false, "0.0")});
            missionRecoveryDialog->recoveryFactor = career::LocalizeNumber(f32store(fx(num3) * 100.0), "0.0") + "% " + text;
        }
        log::Info("[VesselRecovery]: " + pv->GetDisplayName() + " recovered " + missionRecoveryDialog->recoveryLocation + ". Recovery Value: " +
                  missionRecoveryDialog->recoveryFactor);
    }
    recoverVesselCrew(pv, missionRecoveryDialog);
    GE::onVesselRecoveryProcessing.Fire(pv, missionRecoveryDialog, num3);
    if (quick) {
        GE::onVesselRecoveryProcessingComplete.Fire(pv, nullptr, num3);
        return;
    }
    if (Funding::Instance != nullptr) missionRecoveryDialog->totalFunds = Funding::Instance->Funds();
    if (Reputation::Instance != nullptr) missionRecoveryDialog->totalReputation = Reputation::CurrentRep();
    if (haveRnd) missionRecoveryDialog->totalScience = env.rndScience ? env.rndScience() : 0.0f;
    CurrencyModifierQuery currencyModifierQuery = CurrencyModifierQuery::RunQuery(
        TransactionReasons::VesselRecovery, static_cast<float>(missionRecoveryDialog->fundsEarned), missionRecoveryDialog->scienceEarned,
        missionRecoveryDialog->reputationEarned);
    missionRecoveryDialog->fundsEarned += static_cast<double>(currencyModifierQuery.GetEffectDelta(Currency::Funds));
    missionRecoveryDialog->FundsModifier = currencyModifierQuery.GetEffectDeltaText(Currency::Funds, "N1", CurrencyModifierQuery::TextStyling::OnGUI);
    missionRecoveryDialog->reputationEarned = f32store(fx(missionRecoveryDialog->reputationEarned) + fx(currencyModifierQuery.GetEffectDelta(Currency::Reputation)));
    missionRecoveryDialog->RepModifier = currencyModifierQuery.GetEffectDeltaText(Currency::Reputation, "N1", CurrencyModifierQuery::TextStyling::OnGUI);
    missionRecoveryDialog->scienceEarned = f32store(fx(missionRecoveryDialog->scienceEarned) + fx(currencyModifierQuery.GetEffectDelta(Currency::Science)));
    missionRecoveryDialog->ScienceModifier = currencyModifierQuery.GetEffectDeltaText(Currency::Science, "N1", CurrencyModifierQuery::TextStyling::OnGUI);
    GE::onVesselRecoveryProcessingComplete.Fire(pv, missionRecoveryDialog, num3);
}

void VesselRecovery::Register() {
    ScenarioModuleFactory::Register("VesselRecovery", [](const std::string&) { return std::unique_ptr<ScenarioModule>(new VesselRecovery()); });
}

}  // namespace ksp
