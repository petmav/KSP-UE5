// WP-37: Contracts.ContractSystem (Assembly-CSharp/Contracts/ContractSystem.cs).
#include "ksp/contracts/contract_system.hpp"

#include <algorithm>

#include "ksp/career/career.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/flight_globals.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/contracts/agents.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/random.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/modules/scenario_runner.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/unity/random.hpp"

namespace ksp::Contracts {

using GE = events::GameEvents;

namespace {
CareerEnvironment& Career() { return CareerEnvironment::Current(); }
int NextOwner() {
    static int next = 7000;
    return ++next;
}
bool LoadedSceneIsEditor() { return SaveEnvironment::Current().loadedScene == GameScenes::EDITOR; }
}  // namespace

ContractSystem::ContractSystem() : ScenarioModule("ContractSystem"), coroutineOwner_(NextOwner()) {}

ContractSystem::~ContractSystem() { OnDestroy(); }

// ---- type tables ------------------------------------------------------------------------------------------------------

// C++ helper (see ContractSystem.cs:80) GenerateContractTypes (the TypeOperation scan): registration
void ContractSystem::RegisterContractType(const std::string& name, ContractCreator creator) {
    for (auto& t : ContractTypes)
        if (t.name == name) {
            t.creator = std::move(creator);
            return;
        }
    ContractTypes.push_back({name, std::move(creator)});
}

// C++ helper (see ContractSystem.cs:107) GenerateParameterTypes
void ContractSystem::RegisterParameterType(const std::string& name, ParameterCreator creator) {
    for (auto& t : ParameterTypes)
        if (t.first == name) {
            t.second = std::move(creator);
            return;
        }
    ParameterTypes.emplace_back(name, std::move(creator));
}

// C++ helper (see ContractSystem.cs:149) GeneratePredicateTypes
void ContractSystem::RegisterPredicateType(const std::string& name, PredicateCreator creator) {
    for (auto& t : PredicateTypes)
        if (t.first == name) {
            t.second = std::move(creator);
            return;
        }
    PredicateTypes.emplace_back(name, std::move(creator));
}

void ContractSystem::ClearTypes() {
    ContractTypes.clear();
    ParameterTypes.clear();
    PredicateTypes.clear();
    MandatoryTypes.clear();
}

// C++ helper (see ContractSystem.cs:99) GetContractType + Activator.CreateInstance
std::unique_ptr<Contract> ContractSystem::CreateContract(const std::string& typeName) {
    for (auto& t : ContractTypes)
        if (t.name == typeName) return t.creator();
    throw ArgumentNullException("type");
}

bool ContractSystem::HasContractType(const std::string& typeName) {
    for (auto& t : ContractTypes)
        if (t.name == typeName) return true;
    return false;
}

// C++ helper (see ContractSystem.cs:128) GetParameterType + Activator.CreateInstance (nullptr when the type is unknown)
std::unique_ptr<ContractParameter> ContractSystem::CreateParameter(const std::string& typeName) {
    for (auto& t : ParameterTypes)
        if (t.first == typeName) return t.second();
    return nullptr;
}

bool ContractSystem::HasParameterType(const std::string& typeName) {
    for (auto& t : ParameterTypes)
        if (t.first == typeName) return true;
    return false;
}

// C++ helper (see ContractSystem.cs:170) GetPredicateType + Activator.CreateInstance
std::unique_ptr<ContractPredicate> ContractSystem::CreatePredicate(const std::string& typeName, IContractParameterHost* parent) {
    for (auto& t : PredicateTypes)
        if (t.first == typeName) return t.second(parent);
    return nullptr;
}

bool ContractSystem::HasPredicateType(const std::string& typeName) {
    for (auto& t : PredicateTypes)
        if (t.first == typeName) return true;
    return false;
}

// ContractSystem.cs:88 GenerateTypes
void ContractSystem::GenerateTypes() {
    if (ContractTypes.empty() && ParameterTypes.empty() && PredicateTypes.empty()) RegisterStockTypes();
    // GenerateMandatoryTypes (:183): the ExplorationContract type name (the type itself is WP-38's)
    if (MandatoryTypes.empty()) MandatoryTypes.push_back("ExplorationContract");
}

// ContractSystem.cs:345
void ContractSystem::Register() {
    ScenarioModuleFactory::Register("ContractSystem", [](const std::string&) { return std::unique_ptr<ScenarioModule>(new ContractSystem()); });
}

// ---- scenario ---------------------------------------------------------------------------------------------------------

// ContractSystem.cs:215
void ContractSystem::OnAwake() {
    loaded = false;
    Instance = this;
    if (ContractsEnvironment::Current().loadContractDefsData) ContractsEnvironment::Current().loadContractDefsData();
    GenerateTypes();
    if (!LoadedSceneIsEditor()) Career().Scheduler().StartCoroutine(UpdateDaemon(), coroutineOwner_);
}

// ContractSystem.cs:228
void ContractSystem::OnDestroy() {
    Career().Scheduler().StopAllCoroutines(coroutineOwner_);
    for (size_t i = 0; i < contracts.size(); ++i) contracts[i]->Unregister();
    contracts.clear();
    contractsFinished.clear();
    if (eventsAdded_) {
        GE::onGameSceneLoadRequested.Remove(this, &ContractSystem::OnSceneLoadRequested);
        GE::onFlightReady.Remove(this, &ContractSystem::OnFlightReady);
        GE::OnFlightGlobalsReady.Remove(this, &ContractSystem::OnFlightGlobalsReady);
        GE::onVesselChange.Remove(this, &ContractSystem::OnVesselChange);
        GE::OnProgressReached.Remove(this, &ContractSystem::OnNodeReached);
        GE::OnReputationChanged.Remove(this, &ContractSystem::OnReputationChanged);
        GE::onGUIMissionControlSpawn.Remove(this, &ContractSystem::OnMissionControlSpawned);
        eventsAdded_ = false;
    }
    if (Instance != nullptr && Instance == this) Instance = nullptr;
}

// ContractSystem.cs:256
void ContractSystem::OnLoad(ConfigNode& gameNode) {
    Career().Scheduler().StartCoroutine(OnLoadRoutine(gameNode.CreateCopy()), coroutineOwner_);
}

// ContractSystem.cs:261
Enumerator ContractSystem::OnLoadRoutine(ConfigNode::NodePtr gameNode) {
    co_yield YieldInstruction::Null();
    LoadNow(*gameNode);
}

// C++ helper (see ContractSystem.cs:261) (after the first yield)
void ContractSystem::LoadNow(ConfigNode& gameNode) {
    if (gameNode.HasValue("version")) version = *gameNode.GetValue("version");  // SystemUtilities.LoadNode(.., "version", ref version, "-1", false)
    else version = "-1";
    ContractsEnvironment& env = ContractsEnvironment::Current();
    const bool isCareer = SaveEnvironment::Current().currentGame != nullptr && SaveEnvironment::Current().currentGame->Mode == Game::Modes::CAREER;
    if (isCareer && env.oldCareerSave && env.oldCareerSave(version)) {
        if (env.updateCareerSave) env.updateCareerSave(gameNode);
        version = env.versionString;
    }
    LoadContractWeights(gameNode);
    contracts.clear();
    ConfigNode* node = gameNode.GetNode("CONTRACTS");
    if (node == nullptr) return;
    std::vector<ConfigNode*> nodes = node->GetNodes("CONTRACT");
    for (size_t i = 0; i < nodes.size(); ++i) {
        std::unique_ptr<Contract> contract;
        LoadContract(*nodes[i], contract);
        if (contract) contracts.push_back(std::move(contract));
    }
    nodes = node->GetNodes("CONTRACT_FINISHED");
    for (size_t j = 0; j < nodes.size(); ++j) {
        std::unique_ptr<Contract> contract2;
        LoadContract(*nodes[j], contract2);
        if (contract2)
            contractsFinished.push_back(std::move(contract2));
        else
            log::Error("[ContractSystem]: Contract " + std::to_string(j) + " is invalid");
    }
    if (node->HasValue("update")) lastUpdate = dotnet::ParseDouble(*node->GetValue("update"));
    RegisterContracts();
    GE::Contract::onContractsLoaded.Fire();
    ResetContracts();
    GE::onGameSceneLoadRequested.Add(this, &ContractSystem::OnSceneLoadRequested);
    GE::onFlightReady.Add(this, &ContractSystem::OnFlightReady);
    GE::OnFlightGlobalsReady.Add(this, &ContractSystem::OnFlightGlobalsReady);
    GE::onVesselChange.Add(this, &ContractSystem::OnVesselChange);
    GE::OnProgressReached.Add(this, &ContractSystem::OnNodeReached);
    GE::OnReputationChanged.Add(this, &ContractSystem::OnReputationChanged);
    GE::onGUIMissionControlSpawn.Add(this, &ContractSystem::OnMissionControlSpawned);
    eventsAdded_ = true;
    loaded = true;
}

// ContractSystem.cs:320
Contract* ContractSystem::LoadContract(ConfigNode& cNode, std::unique_ptr<Contract>& out) {
    const std::optional<std::string> value = cNode.GetValue("type");
    if (!value.has_value()) {
        log::Error("[ContractSystem]: Contract config is invalid");
        return nullptr;
    }
    cNode.RemoveValues("type");
    if (!HasContractType(*value)) {
        log::Error("[ContractSystem]: Contract type '" + *value + "' not found");
        return nullptr;
    }
    std::unique_ptr<Contract> contract = CreateContract(*value);
    if (!Contract::Load(*contract, cNode)) return nullptr;  // Contract.Load returned null
    out = std::move(contract);
    return out.get();
}

// ContractSystem.cs:338
void ContractSystem::RegisterContracts() {
    for (size_t i = 0; i < contracts.size(); ++i)
        if (contracts[i]->ContractState() == Contract::State::Active) contracts[i]->Register();
}

// ContractSystem.cs:350
void ContractSystem::OnSave(ConfigNode& gameNode) {
    gameNode.AddNode(SaveContractWeights());
    ConfigNode* configNode = gameNode.AddNode("CONTRACTS");
    for (size_t i = 0; i < contracts.size(); ++i) {
        ConfigNode* node = configNode->AddNode("CONTRACT");
        contracts[i]->Save(*node);
    }
    for (size_t j = 0; j < contractsFinished.size(); ++j) {
        ConfigNode* node = configNode->AddNode("CONTRACT_FINISHED");
        contractsFinished[j]->Save(*node);
    }
    gameNode.AddValue("update", dotnet::FormatDoubleG(lastUpdate, 17));
    gameNode.AddValue("version", version);
}

// ContractSystem.cs:372
void ContractSystem::OnNodeReached(ProgressNode*) { RefreshContracts(); }
// ContractSystem.cs:377
void ContractSystem::OnReputationChanged(float, TransactionReasons) { RefreshContracts(); }

// ContractSystem.cs:383
void ContractSystem::RefreshContracts() {
    const int64_t ticks = ContractsEnvironment::Current().NowTicks();
    int seed = static_cast<int>(ticks & 0x7FFFFFFFLL) ^ static_cast<int>(static_cast<int64_t>((static_cast<uint64_t>(ticks) & 0x7FFFFFFF00000000ULL) >> 8));
    int tier = 0, tier2 = 0, tier3 = 0;
    GetContractCounts(Reputation::UnitRep(), ContractDefsLite::I("", "", "AverageAvailableContracts"), tier, tier2, tier3);
    bool flag = false;
    if (WithdrawSurplusContracts(Contract::ContractPrestige::Trivial, tier)) flag = true;
    if (WithdrawSurplusContracts(Contract::ContractPrestige::Significant, tier2)) flag = true;
    if (WithdrawSurplusContracts(Contract::ContractPrestige::Exceptional, tier3)) flag = true;
    if (GenerateContracts(seed, Contract::ContractPrestige::Trivial, tier - CountContracts(Contract::ContractPrestige::Trivial))) flag = true;
    if (GenerateContracts(seed, Contract::ContractPrestige::Significant, tier2 - CountContracts(Contract::ContractPrestige::Significant))) flag = true;
    if (GenerateContracts(seed, Contract::ContractPrestige::Exceptional, tier3 - CountContracts(Contract::ContractPrestige::Exceptional))) flag = true;
    if (flag) GE::Contract::onContractsListChanged.Fire();
    requireRefresh = false;
    lastUpdate = Planetarium::fetch->time;
}

// ContractSystem.cs:421
void ContractSystem::GetContractCounts(float rep, int avgContracts, int& tier1, int& tier2, int& tier3) {
    const float num = f32store(fx(static_cast<float>(avgContracts)) * fx(Mathf::Lerp(0.5f, 1.5f, Mathf::InverseLerp(-1.0f, 1.0f, rep))));
    const float num2 = Mathf::InverseLerp(-1.0f, 1.0f, rep);
    const float num3 = Mathf::InverseLerp(-0.33f, 0.6f, rep);
    const float num4 = Mathf::InverseLerp(0.1f, 1.0f, rep);
    const float num5 = Mathf::Floor(Mathf::Lerp(num, 2.0f, f32store(fx(num2) - fx(num3) * 0.5)));
    const float num6 = Mathf::Floor(Mathf::Lerp(0.0f, num, f32store(fx(num3) - fx(num4) * 0.5)));
    const float num7 = Mathf::Floor(Mathf::Lerp(0.0f, num, num4));
    const float num8 = f32store(1.0 / ((fx(num5) + fx(num6) + fx(num7)) / fx(num)));
    tier1 = Mathf::RoundToInt(f32store(fx(num5) * fx(num8)));
    tier2 = Mathf::RoundToInt(f32store(fx(num6) * fx(num8)));
    tier3 = Mathf::RoundToInt(f32store(fx(num7) * fx(num8)));
}

// ContractSystem.cs:436
int ContractSystem::CountContracts(Contract::ContractPrestige difficulty) {
    int num = 0;
    for (size_t i = 0; i < contracts.size(); ++i)
        if (contracts[i]->Prestige() == difficulty) ++num;
    return num;
}

// ContractSystem.cs:450
bool ContractSystem::GenerateContracts(int& seed, Contract::ContractPrestige difficulty, int count) {
    bool result = false;
    int num = 0;
    while (true) {
        if (num < count) {
            std::unique_ptr<Contract> contract = GenerateContract(seed, difficulty);
            if (!contract) break;
            contract->Offer();
            Contract* raw = contract.get();
            contracts.push_back(std::move(contract));
            if (raw->AutoAccept) raw->Accept();
            result = true;
            ++num;
            continue;
        }
        return result;
    }
    return result;
}

// ContractSystem.cs:478
std::unique_ptr<Contract> ContractSystem::GenerateContract(int& seed, Contract::ContractPrestige difficulty) {
    std::unique_ptr<Contract> contract;
    int num = 0;
    while (!contract && ++num < generateContractIterations) {
        if (seed == INT32_MAX)
            seed = INT32_MIN;
        else
            seed++;
        contract = GenerateContract(seed, difficulty, std::string());
        if (!contract) continue;
        for (size_t i = 0; i < contracts.size(); ++i) {
            if (contract->ContractID() == contracts[i]->ContractID()) {
                contract->GenerateFailed();
                contract.reset();
                break;
            }
        }
        if (!contract) continue;
        for (int j = Mathf::Max(0, static_cast<int>(contractsFinished.size()) - finishedContractIDCheck); j < static_cast<int>(contractsFinished.size()); ++j) {
            if (contract->ContractID() == contractsFinished[static_cast<size_t>(j)]->ContractID()) {
                contract->GenerateFailed();
                contract.reset();
                break;
            }
        }
    }
    return contract;
}

// ContractSystem.cs:524
std::unique_ptr<Contract> ContractSystem::GenerateContract(int seed, Contract::ContractPrestige difficulty, const std::string& contractType) {
    UnityRandom::Global().InitState(seed);
    std::string type = contractType;
    if (type.empty()) type = WeightedContractChoice();
    return Contract::Generate(type, difficulty, seed, Contract::State::Generated);
}

// ContractSystem.cs:534
bool ContractSystem::WithdrawSurplusContracts(Contract::ContractPrestige level, int maxAllowed) {
    bool result = false;
    int num = 0;
    int count = static_cast<int>(contracts.size());
    while (count-- > 0) {
        Contract& contract = *contracts[static_cast<size_t>(count)];
        if (contract.Prestige() == level && contract.ContractState() == Contract::State::Offered) ++num;
    }
    int num2 = num - maxAllowed;
    int num3 = 0;
    while (num2 > 0 && num3 < static_cast<int>(contracts.size())) {
        Contract& contract2 = *contracts[static_cast<size_t>(num3)];
        if (contract2.CanBeCancelled() && contract2.Prestige() == level && contract2.ContractState() == Contract::State::Offered) {
            contract2.Withdraw();
            result = true;
            --num2;
            log::Info("[ContractSystem]: Contract " + contract2.Title() + " is no longer being offered by " + contract2.Agent()->Name());
        } else {
            ++num3;
        }
    }
    return result;
}

// ContractSystem.cs:567
Enumerator ContractSystem::UpdateDaemon() {
    co_yield YieldInstruction::Null();
    co_yield YieldInstruction::Null();
    if (updateDaemonRunning) co_return;
    updateDaemonRunning = true;
    while (true) {
        UpdateContracts();
        const float rate = ContractsEnvironment::Current().timeWarpRate ? ContractsEnvironment::Current().timeWarpRate() : 1.0f;
        if (rate < maxWarpFactorForUpdate && (Planetarium::fetch->time > lastUpdate + updateInterval || requireRefresh)) RefreshContracts();
        co_yield YieldInstruction::Null();
    }
}

// ContractSystem.cs:587
void ContractSystem::UpdateContracts() {
    int count = static_cast<int>(contracts.size());
    while (count-- > 0) {
        Contract& contract = *contracts[static_cast<size_t>(count)];
        if (contract.IsFinished()) {
            contract.Unregister();
            if (contract.ContractState() == Contract::State::Completed || contract.ContractState() == Contract::State::DeadlineExpired ||
                contract.ContractState() == Contract::State::Failed || contract.ContractState() == Contract::State::Cancelled)
                contractsFinished.push_back(std::move(contracts[static_cast<size_t>(count)]));
            contracts.erase(contracts.begin() + count);
            requireRefresh = true;
        } else {
            contract.Update();
        }
    }
    if (contracts.empty()) requireRefresh = true;
}

// ContractSystem.cs:614
void ContractSystem::OnSceneLoadRequested(GameScenes) { ResetContracts(); }
// ContractSystem.cs:619
void ContractSystem::OnFlightReady() { ResetContracts(); }
// ContractSystem.cs:624
void ContractSystem::OnFlightGlobalsReady(bool) { ResetContracts(); }
// ContractSystem.cs:629
void ContractSystem::OnVesselChange(Vessel*) { ResetContracts(); }

// ContractSystem.cs:634
void ContractSystem::ResetContracts() {
    int count = static_cast<int>(contracts.size());
    while (count-- > 0) {
        Contract& contract = *contracts[static_cast<size_t>(count)];
        if (contract.ContractState() == Contract::State::Active) contract.Reset();
    }
}

// ContractSystem.cs:647
Contract* ContractSystem::GetContractByGuid(const Guid& guid) {
    for (auto& c : contracts)
        if (c->ContractGuid() == guid) return c.get();
    return nullptr;
}

// ContractSystem.cs:723
bool ContractSystem::HasCompletedContract(const std::string& typeName) {
    for (auto& c : contractsFinished)
        if (c->TypeName() == typeName && c->ContractState() == Contract::State::Completed) return true;
    return false;
}

// ContractSystem.cs:772
int ContractSystem::GetActiveContractCount() {
    int num = 0;
    int count = static_cast<int>(contracts.size());
    while (count-- > 0) {
        Contract& contract = *contracts[static_cast<size_t>(count)];
        if (contract.ContractState() == Contract::State::Active && !contract.AutoAccept) ++num;
    }
    return num;
}

// ContractSystem.cs:787
void ContractSystem::RebuildContracts() {
    std::vector<std::string> list;
    std::vector<int> list2;
    std::vector<Contract::ContractPrestige> list3;
    for (size_t i = 0; i < contracts.size(); ++i) {
        list.push_back(contracts[i]->TypeName());
        list2.push_back(contracts[i]->MissionSeed());
        list3.push_back(contracts[i]->Prestige());
        if (contracts[i]->ContractState() == Contract::State::Active) contracts[i]->Cancel();
    }
    contracts.clear();
    for (size_t j = 0; j < list.size(); ++j) {
        std::unique_ptr<Contract> contract = GenerateContract(list2[j], list3[j], list[j]);
        if (contract) {
            Contract* raw = contract.get();
            contracts.push_back(std::move(contract));
            raw->Offer();
        }
    }
}

// ContractSystem.cs:816
void ContractSystem::ClearContractsCurrent() {
    for (size_t i = 0; i < contracts.size(); ++i) contracts[i]->Kill();
    contracts.clear();
}

// ContractSystem.cs:826
void ContractSystem::ClearContractsFinished() { contractsFinished.clear(); }

// ---- weights ----------------------------------------------------------------------------------------------------------

// ContractSystem.cs:831
int ContractSystem::ClampedWeight(int weight) { return dotnet::Math::Min(dotnet::Math::Max(weight, WeightMinimum), WeightMaximum); }

// ContractSystem.cs:846
int ContractSystem::TryGetWeight(const std::string& name) {
    int value = WeightDefault;
    if (ContractWeights.has_value()) {
        auto it = ContractWeights->find(name);
        if (it != ContractWeights->end()) value = it->second;
        else value = 0;  // Dictionary.TryGetValue leaves `default(int)` in the out parameter on a miss
    }
    return value;
}

// ContractSystem.cs:846
int ContractSystem::TryGetWeight(const std::string& name, bool& success) {
    int value = WeightDefault;
    success = false;
    if (ContractWeights.has_value()) {
        auto it = ContractWeights->find(name);
        if (it != ContractWeights->end()) {
            value = it->second;
            success = true;
        } else {
            value = 0;
        }
    }
    return value;
}

// ContractSystem.cs:853
void ContractSystem::LoadContractWeights(ConfigNode& gameNode) {
    WeightAcceptDelta = ContractDefsLite::I("", "", "WeightAcceptDelta");
    WeightDeclineDelta = ContractDefsLite::I("", "", "WeightDeclineDelta");
    WeightWithdrawReadDelta = ContractDefsLite::I("", "", "WeightWithdrawReadDelta");
    WeightWithdrawSeenDelta = ContractDefsLite::I("", "", "WeightWithdrawSeenDelta");
    WeightMinimum = ContractDefsLite::I("", "", "WeightMinimum");
    WeightMinimum = (WeightMinimum >= 0) ? WeightMinimum : 0;
    WeightMaximum = ContractDefsLite::I("", "", "WeightMaximum");
    WeightMaximum = (WeightMaximum >= 0) ? WeightMaximum : 0;
    WeightDefault = ContractDefsLite::I("", "", "WeightDefault");
    WeightDefault = (WeightDefault < WeightMinimum || WeightDefault > WeightMaximum) ? ((WeightMinimum + WeightMaximum) / 2) : WeightDefault;
    WeightDefault = ContractDefsLite::I("", "", "WeightDefault");
    WeightDefault = (WeightDefault < WeightMinimum || WeightDefault > WeightMaximum) ? ((WeightMinimum + WeightMaximum) / 2) : WeightDefault;
    if (WeightMinimum > WeightMaximum) {
        const int weightMinimum = WeightMinimum;
        WeightMinimum = WeightMaximum;
        WeightMaximum = weightMinimum;
    }
    if (WeightDefault > 0 && WeightMaximum > 0) {
        if (WeightMinimum <= 0 && (WeightAcceptDelta < 0 || WeightDeclineDelta < 0))
            log::Warning("[ContractSystem]: Contract weight configuration settings can render contract types permanently inaccessible");
    } else {
        log::Warning("[ContractSystem]: Contract weight configuration settings will not ever allow contracts to generate");
    }
    ContractWeights = std::map<std::string, int>();
    if (ContractTypes.empty()) return;  // `ContractTypes == null`: generated in OnAwake, never null afterwards
    ConfigNode* node = gameNode.GetNode("WEIGHTS");
    int count = static_cast<int>(ContractTypes.size());
    while (count-- > 0) {
        const std::string key = ContractTypes[static_cast<size_t>(count)].name;
        int result = WeightDefault;
        if (node != nullptr && node->HasValue(key)) dotnet::TryParseInt32(*node->GetValue(key), result);
        (*ContractWeights)[key] = result;
    }
    if (FlightGlobals::bodies == nullptr) return;
    int count2 = static_cast<int>(FlightGlobals::bodies->size());
    while (count2-- > 0) {
        const std::string key2 = (*FlightGlobals::bodies)[static_cast<size_t>(count2)]->name();
        int result2 = WeightDefault;
        if (node != nullptr && node->HasValue(key2)) dotnet::TryParseInt32(*node->GetValue(key2), result2);
        (*ContractWeights)[key2] = result2;
    }
}

// ContractSystem.cs:918
ConfigNode::NodePtr ContractSystem::SaveContractWeights() {
    ConfigNode::NodePtr configNode = ConfigNode::Create("WEIGHTS");
    if (ContractTypes.empty()) return configNode;
    int count = static_cast<int>(ContractTypes.size());
    while (count-- > 0) {
        const std::string text = ContractTypes[static_cast<size_t>(count)].name;
        const int value = TryGetWeight(text);
        configNode->AddValue(text, static_cast<int32_t>(value));
    }
    if (FlightGlobals::bodies == nullptr) return configNode;
    int count2 = static_cast<int>(FlightGlobals::bodies->size());
    while (count2-- > 0) {
        const std::string text2 = (*FlightGlobals::bodies)[static_cast<size_t>(count2)]->name();
        const int value2 = TryGetWeight(text2);
        configNode->AddValue(text2, static_cast<int32_t>(value2));
    }
    return configNode;
}

// ContractSystem.cs:946
void ContractSystem::ResetWeights() {
    if (ContractTypes.empty() || !ContractWeights.has_value()) return;
    log::Error("[ContractSystem]: Contract weights have been reset to default");
    int count = static_cast<int>(ContractTypes.size());
    while (count-- > 0) (*ContractWeights)[ContractTypes[static_cast<size_t>(count)].name] = WeightDefault;
    if (FlightGlobals::bodies != nullptr) {
        int count2 = static_cast<int>(FlightGlobals::bodies->size());
        while (count2-- > 0) (*ContractWeights)[(*FlightGlobals::bodies)[static_cast<size_t>(count2)]->name()] = WeightDefault;
    }
}

// ContractSystem.cs:968
std::string ContractSystem::WeightedContractChoice() {
    if (!ContractTypes.empty()) {
        int num = 0;
        int count = static_cast<int>(ContractTypes.size());
        while (count-- > 0) num += TryGetWeight(ContractTypes[static_cast<size_t>(count)].name);
        int num2 = UnityRandom::Global().Range(0, num);
        int count2 = static_cast<int>(ContractTypes.size());
        while (count2-- > 0) {
            const int num3 = TryGetWeight(ContractTypes[static_cast<size_t>(count2)].name);
            if (num2 >= num3) {
                num2 -= num3;
                continue;
            }
            num2 = count2;
            break;
        }
        return ContractTypes.at(static_cast<size_t>(num2)).name;
    }
    log::Error("[ContractSystem]: Attempted to generate contract with no contract types available");
    return std::string();
}

// ContractSystem.cs:997
CelestialBody* ContractSystem::WeightedBodyChoice(const std::vector<CelestialBody*>& bodies, KSPRandom* generator) {
    if (!bodies.empty()) {
        int num = 0;
        int count = static_cast<int>(bodies.size());
        while (count-- > 0) num += TryGetWeight(bodies[static_cast<size_t>(count)]->name());
        const int num2r = generator != nullptr ? generator->Next(0, num) : UnityRandom::Global().Range(0, num);
        int num2 = num2r;
        int count2 = static_cast<int>(bodies.size());
        while (count2-- > 0) {
            const int num3 = TryGetWeight(bodies[static_cast<size_t>(count2)]->name());
            if (num2 >= num3) {
                num2 -= num3;
                continue;
            }
            num2 = count2;
            break;
        }
        return bodies.at(static_cast<size_t>(num2));
    }
    log::Error("[ContractSystem]: Attempted to generate contract with no celestial bodies available");
    return nullptr;
}

// ContractSystem.cs:1031
void ContractSystem::WeightAssignment(const std::string& name, int amount, bool ignoreLimits) {
    if (ContractWeights.has_value() && ContractWeights->count(name) != 0) (*ContractWeights)[name] = ignoreLimits ? amount : ClampedWeight(amount);
}

// ContractSystem.cs:1039
void ContractSystem::WeightAdjustment(const std::string& name, int delta, bool ignoreLimits) {
    bool success = false;
    const int num = TryGetWeight(name, success);
    if (success) {
        const int num2 = num + delta;
        (*ContractWeights)[name] = ignoreLimits ? num2 : ClampedWeight(num2);
    }
}

// ContractSystem.cs:1050
void ContractSystem::AdjustWeight(const std::string& name, Contract* contract) {
    switch (contract->ContractState()) {
        case Contract::State::OfferExpired:
        case Contract::State::Withdrawn:
            if (contract->ContractViewed() == Contract::Viewed::Read)
                WeightAdjustment(name, WeightWithdrawReadDelta);
            else if (contract->ContractViewed() == Contract::Viewed::Seen)
                WeightAdjustment(name, WeightWithdrawSeenDelta);
            break;
        case Contract::State::Declined: WeightAdjustment(name, WeightDeclineDelta); break;
        case Contract::State::Active: WeightAdjustment(name, WeightAcceptDelta); break;
        default: break;
    }
}

// ContractSystem.cs:1074
void ContractSystem::OnMissionControlSpawned() { GenerateMandatoryContracts(); }

// ContractSystem.cs:1079
void ContractSystem::GenerateMandatoryContracts() {
    bool flag = false;
    for (int num = static_cast<int>(MandatoryTypes.size()) - 1; num >= 0; --num) {
        const std::string type = MandatoryTypes[static_cast<size_t>(num)];
        if (!HasContractType(type)) continue;  // the type is registered by its package (ExplorationContract: WP-38)
        bool flag2 = true;
        int num2 = static_cast<int>(contracts.size()) - 1;
        while (num2 >= 0) {
            if (!(contracts[static_cast<size_t>(num2)]->TypeName() == type)) {
                --num2;
                continue;
            }
            flag2 = false;
            break;
        }
        if (flag2) {
            // Enum.GetValues(typeof(ContractPrestige)) has 3 values
            const Contract::ContractPrestige difficulty = static_cast<Contract::ContractPrestige>(UnityRandom::Global().Range(0, 3));
            const int seedDraw = UnityRandom::Global().Range(INT32_MIN, INT32_MAX);
            std::unique_ptr<Contract> contract = GenerateContract(seedDraw, difficulty, type);
            if (contract) {
                Contract* raw = contract.get();
                raw->Offer();
                contracts.push_back(std::move(contract));
                if (raw->AutoAccept) raw->Accept();
                flag = true;
            }
        }
    }
    if (flag) GE::Contract::onContractsListChanged.Fire();
}

// ContractSystem.cs:1120
void ContractSystem::SpawnPreBuiltShip(const std::string& craftUrl) {
    if (ContractsEnvironment::Current().spawnPreBuiltShip) ContractsEnvironment::Current().spawnPreBuiltShip(craftUrl);
}

}  // namespace ksp::Contracts
