// WP-53: Expansions/Serenity/Contracts/*.cs (see include/ksp/contracts/serenity_contracts.hpp).
#include "ksp/contracts/serenity_contracts.hpp"

#include <algorithm>

#include "ksp/career/career.hpp"
#include "ksp/career/career_environment.hpp"
#include "ksp/career/format.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/contracts/agents.hpp"
#include "ksp/contracts/contract_environment.hpp"
#include "ksp/contracts/contract_system.hpp"
#include "ksp/contracts/fineprint_lite.hpp"
#include "ksp/contracts/text_gen.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/random.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/modules/science_modules.hpp"
#include "ksp/parts/available_part.hpp"
#include "ksp/parts/part_loader.hpp"
#include "ksp/serenity/ground_modules.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/save/proto_vessel.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/science/science.hpp"
#include "ksp/serenity/roc.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp::Expansions::Serenity::Contracts {
namespace {

using ksp::Contracts::ContractDefsLite;
using ksp::Contracts::ContractSystem;
using ksp::Contracts::TextGen;
using KSP::Localization::Localizer;
using GE = events::GameEvents;
namespace PU = ksp::Contracts::Lite::ProgressUtilities;

bool SerenityInstalled() { return SaveEnvironment::Current().IsExpansionInstalled("Serenity"); }
CelestialBody* HomeBody() { return Planetarium::fetch->Home; }
CelestialBody* Sun() { return Planetarium::fetch->Sun; }
std::string HomeBodyName() { return CareerEnvironment::Current().HomeBody()->name(); }
std::string Fmt(const char* tag, std::vector<std::string> args) { return Localizer::Format(tag, args); }
std::string N0(float v) { return career::FormatNumber(v, "N0"); }

// SystemUtilities.LoadNode over a Serenity section (ContractDefs.cs:2307, 2663): the section is read only with the expansion installed
float F(const char* section, const char* sub, const char* key, float def) {
    if (!SerenityInstalled() || !ContractDefsLite::Loaded() || !ContractDefsLite::Has(section, sub, key)) return def;
    return ContractDefsLite::F(section, sub, key);
}
int I(const char* section, const char* sub, const char* key, int def) {
    if (!SerenityInstalled() || !ContractDefsLite::Loaded() || !ContractDefsLite::Has(section, sub, key)) return def;
    return ContractDefsLite::I(section, sub, key);
}

ROCDefinition* FindRocDef(const std::string& type) {
    if (ROCManager::Instance == nullptr) throw NullReferenceException();
    const std::vector<ROCDefinition*> defs = ROCManager::Instance->rocDefinitions();
    for (size_t i = 0; i < defs.size(); i++)
        if (type == defs[i]->type) return defs[i];
    return nullptr;
}

CelestialBody* BodyByName(const std::string& name) {  // FlightGlobals.GetBodyByName
    const std::vector<CelestialBody*>& bodies = FlightRegistry::Bodies();
    for (size_t i = bodies.size(); i-- > 0;)
        if (bodies[i]->name() == name) return bodies[i];
    return nullptr;
}

std::string BiomeSuffix(const std::vector<std::string>& biomes) {
    std::string text;
    if (biomes.size() == 1)
        text = Fmt("#autoLOC_8004435", {biomes[0]});
    else if (biomes.size() == 2)
        text = Fmt("#autoLOC_8004436", {biomes[0], biomes[1]});
    else if (biomes.size() >= 3)
        text = Fmt("#autoLOC_8004437", {biomes[0], biomes[1], biomes[2]});
    return text;
}

bool StateIsActiveOrFinished(::ksp::Contracts::Contract::State s) {
    using S = ::ksp::Contracts::Contract::State;
    return s == S::Active || s == S::Cancelled || s == S::Completed || s == S::DeadlineExpired || s == S::Failed;
}

}  // namespace

void SerenityContractEnvironment::Reset() {
    availableDeployedScienceExpParts = nullptr;
    availableRobotArmScannerParts = nullptr;
}

// ---- ContractDefs ---------------------------------------------------------------------------------------------------------------

// ContractDefs.cs:817 / :2663
SerenityContractDefs::DeployedScienceDefs SerenityContractDefs::DeployedScience() {
    DeployedScienceDefs d;
    d.MaximumExistent = I("DeployedScience", "", "MaximumExistent", d.MaximumExistent);
    d.SciencePercentage = F("DeployedScience", "", "SciencePercentage", d.SciencePercentage);
    d.expire.MinimumExpireDays = I("DeployedScience", "Expiration", "MinimumExpireDays", d.expire.MinimumExpireDays);
    d.expire.MaximumExpireDays = I("DeployedScience", "Expiration", "MaximumExpireDays", d.expire.MaximumExpireDays);
    d.expire.DeadlineDays = I("DeployedScience", "Expiration", "DeadlineDays", d.expire.DeadlineDays);
    d.FundsBaseAdvance = F("DeployedScience", "Funds", "BaseAdvance", d.FundsBaseAdvance);
    d.FundsBaseReward = F("DeployedScience", "Funds", "BaseReward", d.FundsBaseReward);
    d.FundsBaseFailure = F("DeployedScience", "Funds", "BaseFailure", d.FundsBaseFailure);
    d.ScienceBaseReward = F("DeployedScience", "Science", "BaseReward", d.ScienceBaseReward);
    d.ReputationBaseReward = F("DeployedScience", "Reputation", "BaseReward", d.ReputationBaseReward);
    d.ReputationBaseFailure = F("DeployedScience", "Reputation", "BaseFailure", d.ReputationBaseFailure);
    return d;
}

// ContractDefs.cs:854 / :2674
SerenityContractDefs::ROCScienceArmDefs SerenityContractDefs::ROCScienceArm() {
    ROCScienceArmDefs d;
    const char* s = "ROCScienceArm";
    d.MaximumExistent = I(s, "", "MaximumExistent", d.MaximumExistent);
    d.SimpleArmPercentage = F(s, "", "SimpleArmPercentage", d.SimpleArmPercentage);
    d.ComplexArmPercentage = F(s, "", "ComplexArmPercentage", d.ComplexArmPercentage);
    d.expire.MinimumExpireDays = I(s, "Expiration", "MinimumExpireDays", d.expire.MinimumExpireDays);
    d.expire.MaximumExpireDays = I(s, "Expiration", "MaximumExpireDays", d.expire.MaximumExpireDays);
    d.expire.DeadlineDays = I(s, "Expiration", "DeadlineDays", d.expire.DeadlineDays);
    d.SimpleArmAdvance = F(s, "Funds", "SimpleArmAdvance", d.SimpleArmAdvance);
    d.ComplexArmAdvance = F(s, "Funds", "ComplexArmAdvance", d.ComplexArmAdvance);
    d.AdvancedArmAdvance = F(s, "Funds", "AdvancedArmAdvance", d.AdvancedArmAdvance);
    d.SimpleArmReward = F(s, "Funds", "SimpleArmReward", d.SimpleArmReward);
    d.ComplexArmReward = F(s, "Funds", "ComplexArmReward", d.ComplexArmReward);
    d.AdvancedArmReward = F(s, "Funds", "AdvancedArmReward", d.AdvancedArmReward);
    d.SimpleArmFailure = F(s, "Funds", "SimpleArmFailure", d.SimpleArmFailure);
    d.ComplexArmFailure = F(s, "Funds", "ComplexArmFailure", d.ComplexArmFailure);
    d.AdvancedArmFailure = F(s, "Funds", "AdvancedArmFailure", d.AdvancedArmFailure);
    d.ScienceSimpleArmReward = F(s, "Science", "SimpleArmReward", d.ScienceSimpleArmReward);
    d.ScienceComplexArmReward = F(s, "Science", "ComplexArmReward", d.ScienceComplexArmReward);
    d.ScienceAdvancedArmReward = F(s, "Science", "AdvancedArmReward", d.ScienceAdvancedArmReward);
    d.ReputationSimpleArmReward = F(s, "Reputation", "SimpleArmReward", d.ReputationSimpleArmReward);
    d.ReputationComplexArmReward = F(s, "Reputation", "ComplexArmReward", d.ReputationComplexArmReward);
    d.ReputationAdvancedArmReward = F(s, "Reputation", "AdvancedArmReward", d.ReputationAdvancedArmReward);
    d.ReputationSimpleArmFailure = F(s, "Reputation", "SimpleArmFailure", d.ReputationSimpleArmFailure);
    d.ReputationComplexArmFailure = F(s, "Reputation", "ComplexArmFailure", d.ReputationComplexArmFailure);
    d.ReputationAdvancedArmFailure = F(s, "Reputation", "AdvancedArmFailure", d.ReputationAdvancedArmFailure);
    return d;
}

// ContractDefs.cs:917 / :2697
SerenityContractDefs::ROCScienceRetrievalDefs SerenityContractDefs::ROCScienceRetrieval() {
    ROCScienceRetrievalDefs d;
    const char* s = "ROCScienceRetrieval";
    d.MaximumExistent = I(s, "", "MaximumExistent", d.MaximumExistent);
    d.expire.MinimumExpireDays = I(s, "Expiration", "MinimumExpireDays", d.expire.MinimumExpireDays);
    d.expire.MaximumExpireDays = I(s, "Expiration", "MaximumExpireDays", d.expire.MaximumExpireDays);
    d.expire.DeadlineDays = I(s, "Expiration", "DeadlineDays", d.expire.DeadlineDays);
    d.FundsAdvance = F(s, "Funds", "Advance", d.FundsAdvance);
    d.FundsReward = F(s, "Funds", "Reward", d.FundsReward);
    d.FundsFailure = F(s, "Funds", "Failure", d.FundsFailure);
    d.ScienceReward = F(s, "Science", "Reward", d.ScienceReward);
    d.ReputationReward = F(s, "Reputation", "Reward", d.ReputationReward);
    d.ReputationFailure = F(s, "Reputation", "Failure", d.ReputationFailure);
    return d;
}

// ---- PartLoader lists --------------------------------------------------------------------------------------------------------------

namespace {
// PartLoader.cs:2133 GetUnlockedCargoParts(list, includeNotPurchased: false)
std::vector<const AvailablePart*> UnlockedCargoParts(const std::vector<const AvailablePart*>& input) {
    std::vector<const AvailablePart*> list;
    Game* g = SaveEnvironment::Current().currentGame;
    const bool flag = g != nullptr && g->Mode == Game_Modes::MISSION;
    for (size_t i = 0; i < input.size(); i++)
        if (flag || ResearchAndDevelopment::PartModelPurchased(input[i])) list.push_back(input[i]);
    return list;
}
template <class Pred>
std::vector<const AvailablePart*> PartsWith(Pred hasModule) {
    std::vector<const AvailablePart*> all = ScienceEnvironment::loadedParts ? ScienceEnvironment::loadedParts() : std::vector<const AvailablePart*>();
    std::vector<const AvailablePart*> out;
    for (const AvailablePart* ap : all)
        if (ap != nullptr && ap->partPrefab && hasModule(*ap->partPrefab)) out.push_back(ap);
    return out;
}
}  // namespace

// PartLoader.cs:2147 (deployedScienceExperimentParts: parts with ModuleGroundExperiment, PartLoader.cs:969)
std::vector<const AvailablePart*> GetAvailableDeployedScienceExpParts() {
    if (SerenityContractEnvironment::availableDeployedScienceExpParts) return SerenityContractEnvironment::availableDeployedScienceExpParts();
    return UnlockedCargoParts(PartsWith([](Part& p) { return p.FindModuleOfClass("ModuleGroundExperiment") != nullptr; }));
}

// PartLoader.cs:2177 (robotArmScannerParts: parts with ModuleRobotArmScanner, PartLoader.cs:973)
std::vector<const AvailablePart*> GetAvailableRobotArmScannerParts() {
    if (SerenityContractEnvironment::availableRobotArmScannerParts) return SerenityContractEnvironment::availableRobotArmScannerParts();
    return UnlockedCargoParts(PartsWith([](Part& p) { return p.FindModuleOfClass("ModuleRobotArmScanner") != nullptr; }));
}

// =============================================================================================================================
// CollectDeployedScience

CollectDeployedScience::CollectDeployedScience(CelestialBody* targetBody_, std::string subjectId_, std::string scienceTitle_, float sciencePercentage_)
    : targetBody(targetBody_), subjectId(std::move(subjectId_)), sciencePercentage(sciencePercentage_), scienceCollected(0.0f), scienceTitle(std::move(scienceTitle_)) {}

CollectDeployedScience::~CollectDeployedScience() { GE::OnScienceRecieved.Remove(this, &CollectDeployedScience::OnScience); }

// :44
std::string CollectDeployedScience::GetHashString() { return SubjectId() + " " + targetBody->name(); }

// :49
std::string CollectDeployedScience::GetTitle() {
    if (!StateIsActiveOrFinished(Root()->ContractState()))
        return Fmt("#autoLOC_8002261", {N0(sciencePercentage), scienceTitle, targetBody->displayName()});
    return Fmt("#autoLOC_8002260", {N0(scienceCollected), N0(sciencePercentage), scienceTitle, targetBody->displayName()});
}

// :58
void CollectDeployedScience::OnLoad(ConfigNode& node) {
    std::string value;
    if (node.TryGetValue("body", value)) targetBody = BodyByName(value);
    node.TryGetValue("subjectId", subjectId);
    node.TryGetValue("scienceTitle", scienceTitle);
    node.TryGetValue("sciencePercentage", sciencePercentage);
    node.TryGetValue("scienceCollected", scienceCollected);
}

// :72
void CollectDeployedScience::OnSave(ConfigNode& node) {
    if (targetBody != nullptr) node.AddValue("body", targetBody->name());
    node.AddValue("subjectId", subjectId);
    node.AddValue("scienceTitle", scienceTitle);
    node.AddValue("sciencePercentage", sciencePercentage);
    node.AddValue("scienceCollected", scienceCollected);
}

// :82 / :87
void CollectDeployedScience::OnRegister() { GE::OnScienceRecieved.Add(this, &CollectDeployedScience::OnScience); }
void CollectDeployedScience::OnUnregister() { GE::OnScienceRecieved.Remove(this, &CollectDeployedScience::OnScience); }

// :92
void CollectDeployedScience::OnScience(float, ScienceSubject* subject, ProtoVessel*, bool reverseEngineered) {
    if (!SerenityInstalled() || reverseEngineered) return;
    const size_t at = subject->id.find('@');
    if (at == std::string::npos) throw ArgumentException("Length cannot be less than zero. (Parameter 'length')");  // Substring(0, -1)
    if (!(subject->id.substr(0, at) == subjectId && subject->IsFromBody(*targetBody) &&
          (subject->IsFromSituation(ExperimentSituations::SrfLanded) || subject->IsFromSituation(ExperimentSituations::SrfSplashed))))
        return;
    scienceCollected = f32store(fx(subject->science) / fx(subject->scienceCap) * 100.0);
    if (scienceCollected >= sciencePercentage)
        SetComplete();
    else
        GE::Contract::onParameterChange.Fire(Root(), this);
}

// =============================================================================================================================
// CollectROCScienceArm

CollectROCScienceArm::CollectROCScienceArm(CelestialBody* targetBody_, std::string subjectId_, std::string rocType_, std::string scienceTitle_, float sciencePercentage_)
    : targetBody(targetBody_), subjectId(std::move(subjectId_)), sciencePercentage(sciencePercentage_), scienceCollected(0.0f), rocType(std::move(rocType_)),
      scienceTitle(std::move(scienceTitle_)) {}

CollectROCScienceArm::~CollectROCScienceArm() { GE::OnScienceRecieved.Remove(this, &CollectROCScienceArm::OnScience); }

// :48
std::string CollectROCScienceArm::GetHashString() { return SubjectId() + " " + targetBody->name(); }

// :53
std::string CollectROCScienceArm::GetTitle() {
    if (!StateIsActiveOrFinished(Root()->ContractState()))
        return Fmt("#autoLOC_8004394", {N0(sciencePercentage), scienceTitle, targetBody->displayName()});
    return Fmt("#autoLOC_8004395", {N0(scienceCollected), N0(sciencePercentage), scienceTitle, targetBody->displayName()});
}

// :62
void CollectROCScienceArm::OnLoad(ConfigNode& node) {
    std::string value;
    if (node.TryGetValue("body", value)) targetBody = BodyByName(value);
    node.TryGetValue("subjectId", subjectId);
    node.TryGetValue("sciencePercentage", sciencePercentage);
    node.TryGetValue("scienceCollected", scienceCollected);
    node.TryGetValue("scienceTitle", scienceTitle);
    node.TryGetValue("rocType", rocType);
    ROCDefinition* def = FindRocDef(rocType);
    if (def != nullptr) scienceTitle = def->displayName;
}

// :95
void CollectROCScienceArm::OnSave(ConfigNode& node) {
    if (targetBody != nullptr) node.AddValue("body", targetBody->name());
    node.AddValue("subjectId", subjectId);
    node.AddValue("sciencePercentage", sciencePercentage);
    node.AddValue("scienceCollected", scienceCollected);
    node.AddValue("rocType", rocType);
    node.AddValue("scienceTitle", scienceTitle);
}

void CollectROCScienceArm::OnRegister() { GE::OnScienceRecieved.Add(this, &CollectROCScienceArm::OnScience); }      // :107
void CollectROCScienceArm::OnUnregister() { GE::OnScienceRecieved.Remove(this, &CollectROCScienceArm::OnScience); }  // :112

// :117
void CollectROCScienceArm::OnScience(float, ScienceSubject* subject, ProtoVessel*, bool) {
    if (subject->id == subjectId) {
        scienceCollected = f32store(fx(subject->science) / fx(subject->scienceCap) * 100.0);
        if (subject->IsFromBody(*targetBody) && subject->IsFromSituation(ExperimentSituations::SrfLanded) &&
            fx(subject->science) / fx(subject->scienceCap) >= fx(sciencePercentage) * 0.95 / 100.0)
            SetComplete();
        else
            GE::Contract::onParameterChange.Fire(Root(), this);
    }
}

// =============================================================================================================================
// CollectROCScienceRetrieval

CollectROCScienceRetrieval::CollectROCScienceRetrieval(CelestialBody* targetBody_, std::string subjectId_, std::string rocType_, std::string scienceTitle_)
    : targetBody(targetBody_), subjectId(std::move(subjectId_)), rocType(std::move(rocType_)), scienceTitle(std::move(scienceTitle_)) {}

CollectROCScienceRetrieval::~CollectROCScienceRetrieval() { GE::onVesselRecoveryProcessing.Remove(this, &CollectROCScienceRetrieval::OnVesselRecoveryProcessing); }

// :36
std::string CollectROCScienceRetrieval::GetHashString() { return SubjectId() + " " + targetBody->name(); }

// :41
std::string CollectROCScienceRetrieval::GetTitle() {
    using S = ::ksp::Contracts::Contract::State;
    if (Root()->ContractState() == S::Completed) return Fmt("#autoLOC_8004398", {scienceTitle});
    const S s = Root()->ContractState();
    if (s != S::Cancelled && s != S::DeadlineExpired && s != S::Failed) return Fmt("#autoLOC_8004397", {scienceTitle, targetBody->displayName()});
    return Fmt("#autoLOC_8004399", {scienceTitle});
}

// :56
void CollectROCScienceRetrieval::OnLoad(ConfigNode& node) {
    std::string value;
    if (node.TryGetValue("body", value)) targetBody = BodyByName(value);
    node.TryGetValue("subjectId", subjectId);
    node.TryGetValue("scienceTitle", scienceTitle);
    node.TryGetValue("rocType", rocType);
    ROCDefinition* def = FindRocDef(rocType);
    if (def != nullptr) scienceTitle = def->displayName;
}

// :85
void CollectROCScienceRetrieval::OnSave(ConfigNode& node) {
    if (targetBody != nullptr) node.AddValue("body", targetBody->name());
    node.AddValue("subjectId", subjectId);
    node.AddValue("scienceTitle", scienceTitle);
    node.AddValue("rocType", rocType);
}

void CollectROCScienceRetrieval::OnRegister() { GE::onVesselRecoveryProcessing.Add(this, &CollectROCScienceRetrieval::OnVesselRecoveryProcessing); }      // :95
void CollectROCScienceRetrieval::OnUnregister() { GE::onVesselRecoveryProcessing.Remove(this, &CollectROCScienceRetrieval::OnVesselRecoveryProcessing); }  // :100

// :105
void CollectROCScienceRetrieval::OnVesselRecoveryProcessing(ProtoVessel* pv, KSP::UI::Screens::MissionRecoveryDialog*, float) {
    if (pv == nullptr) return;
    std::vector<ProtoPartSnapshot*> allProtoPartsIncludingCargo = pv->GetAllProtoPartsIncludingCargo();
    for (size_t i = 0; i < allProtoPartsIncludingCargo.size(); i++) {
        ProtoPartSnapshot* pps = allProtoPartsIncludingCargo[i];
        for (size_t j = 0; j < pps->modules.size(); j++) {
            ProtoPartModuleSnapshot* mod = pps->modules[j].get();
            ConfigNode* moduleValues = mod->moduleValues.get();
            const std::string moduleName = mod->moduleName.value_or("");
            const std::string partName = pps->partName.value_or("");
            if ((!(moduleName == "ModuleScienceContainer") && (!(partName == "kerbalEVA") || !(moduleName == "ModuleScienceExperiment"))) || !moduleValues->HasNode("ScienceData"))
                continue;
            std::vector<ConfigNode*> nodes = moduleValues->GetNodes("ScienceData");
            for (size_t k = 0; k < nodes.size(); k++) {
                const std::optional<std::string> value = nodes[k]->GetValue("subjectID");
                if (value && subjectId == *value) SetComplete();
            }
        }
    }
}

// =============================================================================================================================
// DeployedScienceContract

// :46
bool DeployedScienceContract::MeetRequirements() {
    if (!SerenityInstalled()) return false;
    if (!GameVariables::Instance->UnlockedEVA(ScenarioUpgradeableFacilities::GetFacilityLevel(SpaceCenterFacility::AstronautComplex))) return false;
    if (!ProgressTracking::Instance()->NodeComplete({HomeBodyName(), "Orbit"})) return false;
    return true;
}

// :64
std::string DeployedScienceContract::GetDescription() {
    return TextGen::GenerateBackStories("DeployedScience" + subjectId, Agent()->Title(), targetBody->displayName(), targetBody->displayName() + "Srf", MissionSeed(), false,
                                        false, false);
}

// :69
std::string DeployedScienceContract::GetSynopsys() { return Fmt("#autoLOC_272436", {targetBody->displayName()}); }

// :74
std::string DeployedScienceContract::GetTitle() { return Fmt("#autoLOC_8002258", {scienceTitle, targetBody->displayName()}); }

// :79
std::string DeployedScienceContract::MessageCompleted() {
    return Fmt("#autoLOC_8002259", {dotnet::FormatSingleF(sciencePercentage, 2), scienceTitle, targetBody->displayName()});
}

// :84
void DeployedScienceContract::OnLoad(ConfigNode& node) {
    std::string value;
    if (node.TryGetValue("body", value)) targetBody = BodyByName(value);
    node.TryGetValue("subjectId", subjectId);
    if (!subjectId.empty()) {
        ScienceExperiment* experiment = ResearchAndDevelopment::GetExperiment(subjectId);
        if (experiment != nullptr) scienceTitle = experiment->experimentTitle;
    }
    node.TryGetValue("sciencePercentage", sciencePercentage);
}

// :105
void DeployedScienceContract::OnSave(ConfigNode& node) {
    if (targetBody != nullptr) node.AddValue("body", targetBody->name());
    node.AddValue("subjectId", subjectId);
    node.AddValue("sciencePercentage", sciencePercentage);
}

// :115
bool DeployedScienceContract::Generate() {
    const auto defs = SerenityContractDefs::DeployedScience();
    if (static_cast<int>(ContractSystem::Instance->GetCurrentContracts<DeployedScienceContract>().size()) >= defs.MaximumExistent) return false;
    return GenerateContract_Surface();
}

// :124
bool DeployedScienceContract::GenerateContract_Surface() {
    const auto defs = SerenityContractDefs::DeployedScience();
    if (Prestige() == ContractPrestige::Trivial) return false;
    std::vector<CelestialBody*> bodiesProgress =
        PU::GetBodiesReached(true, [](CelestialBody* cb) { return cb != HomeBody() && cb != Sun() && cb->hasSolidSurface; });
    if (bodiesProgress.empty()) return false;
    std::vector<const AvailablePart*> availableDeployedScienceExpParts = GetAvailableDeployedScienceExpParts();
    if (availableDeployedScienceExpParts.empty()) return false;
    std::vector<ModuleGroundExperiment*> list;
    for (size_t i = 0; i < availableDeployedScienceExpParts.size(); i++) {
        PartModule* m = availableDeployedScienceExpParts[i]->partPrefab->FindModuleImplementing(
            [](PartModule& pm) { return dynamic_cast<ModuleGroundExperiment*>(&pm) != nullptr; });
        if (m != nullptr) list.push_back(static_cast<ModuleGroundExperiment*>(m));
    }
    if (list.empty()) return false;
    sciencePercentage = defs.SciencePercentage;
    std::vector<PotentialScienceCombo> list2;
    for (size_t j = 0; j < list.size(); j++) {
        for (size_t k = 0; k < bodiesProgress.size(); k++) {
            std::shared_ptr<ScienceSubject> subjectByID =
                ResearchAndDevelopment::GetSubjectByID(list[j]->experimentId() + "@" + bodiesProgress[k]->name() + "SrfLanded");
            ScienceExperiment* experiment = ResearchAndDevelopment::GetExperiment(list[j]->experimentId());
            if (experiment == nullptr) throw NullReferenceException();
            if ((!experiment->requireAtmosphere || bodiesProgress[k]->atmosphere) && (!experiment->requireNoAtmosphere || !bodiesProgress[k]->atmosphere)) {
                if (subjectByID == nullptr)
                    list2.push_back({list[j]->experimentId(), bodiesProgress[k]});
                else if (fx(subjectByID->science) / fx(subjectByID->scienceCap) * 100.0 < fx(sciencePercentage))
                    list2.push_back({list[j]->experimentId(), bodiesProgress[k]});
            }
        }
    }
    if (list2.empty()) return false;
    const int index = KSPRandom(MissionSeed()).Next(0, static_cast<int>(list2.size()) - 1);
    subjectId = list2[static_cast<size_t>(index)].experimentId;
    for (int num = static_cast<int>(bodiesProgress.size()) - 1; num >= 0; num--)
        if (!PotentialsContainBody(bodiesProgress[static_cast<size_t>(num)], list2, subjectId)) bodiesProgress.erase(bodiesProgress.begin() + num);
    targetBody = WeightedBodyChoice(bodiesProgress);
    ScienceExperiment* experiment2 = ResearchAndDevelopment::GetExperiment(subjectId);
    if (experiment2 != nullptr) scienceTitle = experiment2->experimentTitle;
    AddParameter(std::make_unique<CollectDeployedScience>(targetBody, subjectId, scienceTitle, sciencePercentage));
    AddKeywords({"Scientific", "Commercial"});
    SetExpiry(defs.expire.MinimumExpireDays, defs.expire.MaximumExpireDays);
    SetDeadlineDays(static_cast<float>(defs.expire.DeadlineDays), targetBody);
    SetFunds(defs.FundsBaseAdvance, defs.FundsBaseReward, defs.FundsBaseFailure, targetBody);
    SetReputation(defs.ReputationBaseReward, defs.ReputationBaseFailure);
    SetScience(defs.ScienceBaseReward);
    return true;
}

// :228
bool DeployedScienceContract::PotentialsContainBody(CelestialBody* body, const std::vector<PotentialScienceCombo>& potentialCombos, const std::string& id) {
    size_t num = 0;
    while (true) {
        if (num < potentialCombos.size()) {
            if (potentialCombos[num].experimentId == id && potentialCombos[num].body->name() == body->name()) break;
            num++;
            continue;
        }
        return false;
    }
    return true;
}

// =============================================================================================================================
// ROCScienceArmContract

// :51
bool ROCScienceArmContract::MeetRequirements() {
    if (!SerenityInstalled()) return false;
    if (ROCManager::Instance != nullptr && ROCManager::Instance->RocsEnabledInCurrentGame()) return true;
    return false;
}

// :65
std::string ROCScienceArmContract::GetDescription() {
    const auto defs = SerenityContractDefs::ROCScienceArm();
    std::string contractType = "ROCAdvancedArmScience";
    if (sciencePercentage < defs.SimpleArmPercentage)
        contractType = "ROCSimpleArmScience";
    else if (sciencePercentage < defs.ComplexArmPercentage)
        contractType = "ROCComplexArmScience";
    return TextGen::GenerateBackStories(contractType, Agent()->Title(), scienceTitle, scienceTitle, MissionSeed(), false, false, false);
}

// :80
std::string ROCScienceArmContract::GetSynopsys() { return Fmt("#autoLOC_8004414", {scienceTitle, scannerArmTitle}) + BiomeSuffix(biomes); }

// :100
std::string ROCScienceArmContract::GetTitle() {
    const auto defs = SerenityContractDefs::ROCScienceArm();
    if (sciencePercentage < defs.SimpleArmPercentage) return Fmt("#autoLOC_8004391", {scienceTitle});
    if (sciencePercentage < defs.ComplexArmPercentage) return Fmt("#autoLOC_8004392", {scienceTitle});
    return Fmt("#autoLOC_8004393", {scienceTitle});
}

// :114
std::string ROCScienceArmContract::MessageCompleted() {
    return Fmt("#autoLOC_8002259", {dotnet::FormatSingleF(sciencePercentage, 2), scienceTitle, targetBody->displayName()});
}

// :119
void ROCScienceArmContract::OnLoad(ConfigNode& node) {
    std::string value;
    if (node.TryGetValue("body", value)) targetBody = BodyByName(value);
    node.TryGetValue("subjectId", subjectId);
    node.TryGetValue("sciencePercentage", sciencePercentage);
    node.TryGetValue("scannerArmName", scannerArmName);
    node.TryGetValue("scannerArmTitle", scannerArmTitle);
    const AvailablePart* partInfoByName = PartLoader::Instance() ? PartLoader::Instance()->getPartInfoByName(scannerArmName) : nullptr;
    if (partInfoByName != nullptr) scannerArmTitle = partInfoByName->title;
    node.TryGetValue("rocType", rocType);
    node.TryGetValue("scienceTitle", scienceTitle);
    if (ROCManager::Instance == nullptr) throw NullReferenceException();
    const std::vector<ROCDefinition*> defs = ROCManager::Instance->rocDefinitions();
    for (size_t i = 0; i < defs.size(); i++) {
        if (rocType == defs[i]->type) {
            scienceTitle = defs[i]->displayName;
            break;
        }
    }
    biomes = node.GetValuesList("biomes");
}

// :152
void ROCScienceArmContract::OnSave(ConfigNode& node) {
    if (targetBody != nullptr) node.AddValue("body", targetBody->name());
    node.AddValue("subjectId", subjectId);
    node.AddValue("scienceTitle", scienceTitle);
    node.AddValue("rocType", rocType);
    node.AddValue("scannerArmName", scannerArmName);
    node.AddValue("scannerArmTitle", scannerArmTitle);
    node.AddValue("sciencePercentage", sciencePercentage);
    for (size_t i = 0; i < biomes.size(); i++) node.AddValue("biomes", biomes[i]);
}

// :170
bool ROCScienceArmContract::Generate() {
    const auto defs = SerenityContractDefs::ROCScienceArm();
    if (static_cast<int>(ContractSystem::Instance->GetCurrentContracts<ROCScienceArmContract>().size()) >= defs.MaximumExistent) return false;
    std::vector<CelestialBody*> bodiesProgress = PU::GetBodiesReached(true, [](CelestialBody* cb) { return cb != Sun() && cb->hasSolidSurface; });
    if (bodiesProgress.empty()) return false;
    float num = 0.0f, num2 = 0.0f, num3 = 0.0f, num4 = 0.0f, num5 = 0.0f, num6 = 0.0f;
    std::vector<const AvailablePart*> availableRobotArmScannerParts = GetAvailableRobotArmScannerParts();
    const AvailablePart* availablePart = nullptr;
    sciencePercentage = -1.0f;
    for (size_t i = 0; i < availableRobotArmScannerParts.size(); i++) {
        PartModule* m = availableRobotArmScannerParts[i]->partPrefab->FindModuleImplementing(
            [](PartModule& pm) { return dynamic_cast<ModuleScienceExperiment*>(&pm) != nullptr; });
        if (m != nullptr && m->Get<float>("scienceValueRatio") > sciencePercentage) {
            availablePart = availableRobotArmScannerParts[i];
            sciencePercentage = m->Get<float>("scienceValueRatio");
        }
    }
    if (availablePart == nullptr) return false;
    const float num7 = f32store(fx(sciencePercentage) * 0.95);
    sciencePercentage = f32store(fx(sciencePercentage) * 100.0);
    scannerArmName = availablePart->name;
    scannerArmTitle = availablePart->title;
    if (sciencePercentage < defs.SimpleArmPercentage) {
        num = defs.SimpleArmAdvance;
        num2 = defs.SimpleArmReward;
        num3 = defs.SimpleArmFailure;
        num4 = defs.ReputationSimpleArmReward;
        num5 = defs.ReputationSimpleArmFailure;
        num6 = defs.ScienceSimpleArmReward;
    } else if (sciencePercentage < defs.ComplexArmPercentage) {
        num = defs.ComplexArmAdvance;
        num2 = defs.ComplexArmReward;
        num3 = defs.ComplexArmFailure;
        num4 = defs.ReputationComplexArmReward;
        num5 = defs.ReputationComplexArmFailure;
        num6 = defs.ScienceComplexArmReward;
    } else {
        num = defs.AdvancedArmAdvance;
        num2 = defs.AdvancedArmReward;
        num3 = defs.AdvancedArmFailure;
        num4 = defs.ReputationAdvancedArmReward;
        num5 = defs.ReputationAdvancedArmFailure;
        num6 = defs.ScienceAdvancedArmReward;
    }
    if (ROCManager::Instance == nullptr) throw NullReferenceException();
    struct PotentialScienceCombo {
        ROCDefinition* rocDef;
        std::string body, id;
    };
    std::vector<PotentialScienceCombo> list;
    const std::vector<ROCDefinition*> rocDefs = ROCManager::Instance->rocDefinitions();
    for (size_t j = 0; j < bodiesProgress.size(); j++) {
        for (size_t k = 0; k < rocDefs.size(); k++) {
            ROCDefinition* def = rocDefs[k];
            for (size_t l = 0; l < def->myCelestialBodies.size(); l++) {
                if (bodiesProgress[j]->name() == def->myCelestialBodies[l].name) {
                    const std::string text = "ROCScience_" + def->type + "@" + def->myCelestialBodies[l].name + "SrfLanded";
                    std::shared_ptr<ScienceSubject> subjectByID = ResearchAndDevelopment::GetSubjectByID(text);
                    if ((subjectByID != nullptr && fx(subjectByID->science) / fx(subjectByID->scienceCap) < fx(num7)) || subjectByID == nullptr)
                        list.push_back({def, bodiesProgress[j]->name(), text});
                }
            }
        }
    }
    if (list.empty()) return false;
    KSPRandom kSPRandom(MissionSeed());
    const int index = kSPRandom.Next(0, static_cast<int>(list.size()) - 1);
    PotentialScienceCombo& combo = list[static_cast<size_t>(index)];
    subjectId = combo.id;
    targetBody = BodyByName(combo.body);
    rocType = combo.rocDef->type;
    scienceTitle = combo.rocDef->displayName;
    RocCBDefinition* rocCBDefinition = nullptr;
    for (size_t m = 0; m < combo.rocDef->myCelestialBodies.size(); m++) {
        if (combo.rocDef->myCelestialBodies[m].name == combo.body) {
            rocCBDefinition = &combo.rocDef->myCelestialBodies[m];
            break;
        }
    }
    if (rocCBDefinition == nullptr) return false;
    int num8 = 1;
    if (rocCBDefinition->biomes.size() >= 5)
        num8 = 3;
    else if (rocCBDefinition->biomes.size() >= 3)
        num8 = 2;
    for (int n = 0; n < 10; n++) {
        if (static_cast<int>(biomes.size()) >= num8) break;
        const int index2 = kSPRandom.Next(0, static_cast<int>(rocCBDefinition->biomes.size()) - 1);
        if (std::find(biomes.begin(), biomes.end(), rocCBDefinition->biomes[static_cast<size_t>(index2)]) == biomes.end())
            biomes.push_back(rocCBDefinition->biomes[static_cast<size_t>(index2)]);
    }
    for (size_t num9 = 0; num9 < biomes.size(); num9++) biomes[num9] = ScienceUtil::GetBiomedisplayName(targetBody, biomes[num9], false);
    AddParameter(std::make_unique<CollectROCScienceArm>(targetBody, subjectId, rocType, scienceTitle, sciencePercentage));
    AddKeywords({"Scientific", "Commercial"});
    SetExpiry(defs.expire.MinimumExpireDays, defs.expire.MaximumExpireDays);
    SetDeadlineDays(static_cast<float>(defs.expire.DeadlineDays), targetBody);
    SetFunds(num, num2, num3, targetBody);
    SetReputation(num4, num5);
    SetScience(num6);
    return true;
}

// =============================================================================================================================
// ROCScienceRetrievalContract

// :45
bool ROCScienceRetrievalContract::MeetRequirements() {
    if (!SerenityInstalled()) return false;
    if (ROCManager::Instance != nullptr && ROCManager::Instance->RocsEnabledInCurrentGame()) {
        if (!GameVariables::Instance->UnlockedEVA(ScenarioUpgradeableFacilities::GetFacilityLevel(SpaceCenterFacility::AstronautComplex))) return false;
        return true;
    }
    return false;
}

// :64
std::string ROCScienceRetrievalContract::GetDescription() {
    return TextGen::GenerateBackStories("ROCScienceRetrieval", Agent()->Title(), scienceTitle, scienceTitle, MissionSeed(), false, false, false);
}

// :69
std::string ROCScienceRetrievalContract::GetSynopsys() { return Fmt("#autoLOC_8004421", {scienceTitle}) + BiomeSuffix(biomes); }

// :89
std::string ROCScienceRetrievalContract::GetTitle() { return Fmt("#autoLOC_8004396", {scienceTitle}); }

// :94
std::string ROCScienceRetrievalContract::MessageCompleted() { return Fmt("#autoLOC_8004424", {scienceTitle, targetBody->displayName()}); }

// :99
void ROCScienceRetrievalContract::OnLoad(ConfigNode& node) {
    std::string value;
    if (node.TryGetValue("body", value)) targetBody = BodyByName(value);
    node.TryGetValue("subjectId", subjectId);
    node.TryGetValue("scienceTitle", scienceTitle);
    node.TryGetValue("rocType", rocType);
    if (ROCManager::Instance == nullptr) throw NullReferenceException();
    const std::vector<ROCDefinition*> defs = ROCManager::Instance->rocDefinitions();
    for (size_t i = 0; i < defs.size(); i++) {
        if (rocType == defs[i]->type) {
            scienceTitle = defs[i]->displayName;
            break;
        }
    }
    biomes = node.GetValuesList("biomes");
}

// :127
void ROCScienceRetrievalContract::OnSave(ConfigNode& node) {
    if (targetBody != nullptr) node.AddValue("body", targetBody->name());
    node.AddValue("subjectId", subjectId);
    node.AddValue("scienceTitle", scienceTitle);
    node.AddValue("rocType", rocType);
    for (size_t i = 0; i < biomes.size(); i++) node.AddValue("biomes", biomes[i]);
}

// :146
bool ROCScienceRetrievalContract::Generate() {
    const auto defs = SerenityContractDefs::ROCScienceRetrieval();
    if (static_cast<int>(ContractSystem::Instance->GetCurrentContracts<ROCScienceRetrievalContract>().size()) >= defs.MaximumExistent) return false;
    std::vector<CelestialBody*> bodiesProgress = PU::GetBodiesReached(true, [](CelestialBody* cb) { return cb != Sun() && cb->hasSolidSurface; });
    if (bodiesProgress.empty()) return false;
    if (ROCManager::Instance == nullptr) throw NullReferenceException();
    struct PotentialScienceCombo {
        std::string body;
        ROCDefinition* rocDef;
        std::string id;
    };
    std::vector<PotentialScienceCombo> list;
    const std::vector<ROCDefinition*> rocDefs = ROCManager::Instance->rocDefinitions();
    for (size_t i = 0; i < bodiesProgress.size(); i++) {
        for (size_t j = 0; j < rocDefs.size(); j++) {
            ROCDefinition* def = rocDefs[j];
            for (size_t k = 0; k < def->myCelestialBodies.size(); k++) {
                if (bodiesProgress[i]->name() == def->myCelestialBodies[k].name && def->canBeTaken) {
                    const std::string text = "ROCScience_" + def->type + "@" + def->myCelestialBodies[k].name + "SrfLanded";
                    std::shared_ptr<ScienceSubject> subjectByID = ResearchAndDevelopment::GetSubjectByID(text);
                    if ((subjectByID != nullptr && fx(subjectByID->science) / fx(subjectByID->scienceCap) < 1.0) || subjectByID == nullptr)
                        list.push_back({bodiesProgress[i]->name(), def, text});
                }
            }
        }
    }
    if (list.empty()) return false;
    KSPRandom kSPRandom(MissionSeed());
    const int index = kSPRandom.Next(0, static_cast<int>(list.size()) - 1);
    PotentialScienceCombo& combo = list[static_cast<size_t>(index)];
    subjectId = combo.id;
    rocType = combo.rocDef->type;
    targetBody = BodyByName(combo.body);
    scienceTitle = combo.rocDef->displayName;
    RocCBDefinition* rocCBDefinition = nullptr;
    for (size_t l = 0; l < combo.rocDef->myCelestialBodies.size(); l++) {
        if (combo.rocDef->myCelestialBodies[l].name == combo.body) {
            rocCBDefinition = &combo.rocDef->myCelestialBodies[l];
            break;
        }
    }
    if (rocCBDefinition == nullptr) return false;
    int num = 1;
    if (rocCBDefinition->biomes.size() >= 5)
        num = 3;
    else if (rocCBDefinition->biomes.size() >= 3)
        num = 2;
    for (int m = 0; m < 10; m++) {
        if (static_cast<int>(biomes.size()) >= num) break;
        const int index2 = kSPRandom.Next(0, static_cast<int>(rocCBDefinition->biomes.size()) - 1);
        if (std::find(biomes.begin(), biomes.end(), rocCBDefinition->biomes[static_cast<size_t>(index2)]) == biomes.end())
            biomes.push_back(rocCBDefinition->biomes[static_cast<size_t>(index2)]);
    }
    for (size_t n = 0; n < biomes.size(); n++) biomes[n] = ScienceUtil::GetBiomedisplayName(targetBody, biomes[n], false);
    AddParameter(std::make_unique<CollectROCScienceRetrieval>(targetBody, subjectId, rocType, scienceTitle));
    AddKeywords({"Scientific", "Commercial"});
    SetExpiry(defs.expire.MinimumExpireDays, defs.expire.MaximumExpireDays);
    SetDeadlineDays(static_cast<float>(defs.expire.DeadlineDays), targetBody);
    SetFunds(defs.FundsAdvance, defs.FundsReward, defs.FundsFailure, targetBody);
    SetReputation(defs.ReputationReward, defs.ReputationFailure);
    SetScience(defs.ScienceReward);
    return true;
}

// =============================================================================================================================

void Install() {
    ContractSystem::RegisterContractType("DeployedScienceContract", [] { return std::make_unique<DeployedScienceContract>(); });
    ContractSystem::RegisterContractType("ROCScienceArmContract", [] { return std::make_unique<ROCScienceArmContract>(); });
    ContractSystem::RegisterContractType("ROCScienceRetrievalContract", [] { return std::make_unique<ROCScienceRetrievalContract>(); });
    ContractSystem::RegisterParameterType("CollectDeployedScience", [] { return std::make_unique<CollectDeployedScience>(); });
    ContractSystem::RegisterParameterType("CollectROCScienceArm", [] { return std::make_unique<CollectROCScienceArm>(); });
    ContractSystem::RegisterParameterType("CollectROCScienceRetrieval", [] { return std::make_unique<CollectROCScienceRetrieval>(); });
}

}  // namespace ksp::Expansions::Serenity::Contracts
