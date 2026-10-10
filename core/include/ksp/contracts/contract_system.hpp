// WP-37: Contracts.ContractSystem (Assembly-CSharp/Contracts/ContractSystem.cs): the ScenarioModule that owns the offered / active / finished
// contracts, generates them from the seed, keeps the per-type / per-body generation weights, and serialises the CONTRACTS / WEIGHTS nodes.
//
// Seams: the contract / parameter / predicate type tables (AssemblyLoader.loadedAssemblies.TypeOperation) are registration tables
// (RegisterContractType / RegisterParameterType / RegisterPredicateType; RegisterStockTypes() adds the WP-37 ones, WP-38 adds the FinePrint
// ones in the TypeDef order of the shipped assembly: that order decides WeightedContractChoice, see STATUS.md); the pre-built craft
// spawning (ConstructShip) is ContractsEnvironment::spawnPreBuiltShip; the UpdateDaemon / OnLoadRoutine coroutines run on the
// CareerEnvironment scheduler (the caller advances frames); MissionControl GUI is WP-46.
#pragma once

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ksp/contracts/contract.hpp"
#include "ksp/core/coroutine.hpp"
#include "ksp/save/game.hpp"

namespace ksp::Contracts {

class ContractSystem : public ScenarioModule {
public:
    using ContractCreator = std::function<std::unique_ptr<Contract>()>;
    using ParameterCreator = std::function<std::unique_ptr<ContractParameter>()>;
    using PredicateCreator = std::function<std::unique_ptr<ContractPredicate>(IContractParameterHost*)>;
    struct ContractTypeInfo {
        std::string name;
        ContractCreator creator;
    };

    ContractSystem();
    ~ContractSystem() override;

    static inline bool loaded = false;                                           // :21
    static inline std::vector<ContractTypeInfo> ContractTypes;                   // :23 (registration order = ContractTypes order)
    static inline std::vector<std::string> MandatoryTypes;                       // :25 (type names)
    static inline std::vector<std::pair<std::string, ParameterCreator>> ParameterTypes;  // :27
    static inline std::vector<std::pair<std::string, PredicateCreator>> PredicateTypes;  // :29
    static inline int generateContractIterations = 50;                           // :43
    static inline std::optional<std::map<std::string, int>> ContractWeights;     // :55 (nullopt = null)
    static inline ContractSystem* Instance = nullptr;                                             // :72

    // registration (the TypeOperation scan of the C#)
    static void RegisterContractType(const std::string& name, ContractCreator creator);
    static void RegisterParameterType(const std::string& name, ParameterCreator creator);
    static void RegisterPredicateType(const std::string& name, PredicateCreator creator);
    static void RegisterStockTypes();   // WP-37 templates, parameters, predicates
    static void ClearTypes();           // tests
    static void Register();             // ScenarioModuleFactory registration ("ContractSystem")
    static std::unique_ptr<Contract> CreateContract(const std::string& typeName);        // Activator.CreateInstance(GetContractType(name))
    static std::unique_ptr<ContractParameter> CreateParameter(const std::string& typeName);  // GetParameterType + Activator.CreateInstance
    static std::unique_ptr<ContractPredicate> CreatePredicate(const std::string& typeName, IContractParameterHost* parent);
    static bool HasContractType(const std::string& typeName);                             // GetContractType(...) != null
    static bool HasParameterType(const std::string& typeName);                            // :107
    static bool HasPredicateType(const std::string& typeName);                            // :149

    std::vector<std::unique_ptr<Contract>>& Contracts() { return contracts; }            // :74
    std::vector<std::unique_ptr<Contract>>& ContractsFinished() { return contractsFinished; }  // :76
    double LastUpdate() const { return lastUpdate; }
    void SetLastUpdate(double v) { lastUpdate = v; }
    const std::string& Version() const { return version; }
    bool RequireRefresh() const { return requireRefresh; }

    void OnAwake() override;                       // :183
    void OnLoad(ConfigNode& gameNode) override;    // :213
    void OnSave(ConfigNode& gameNode) override;    // :315
    // the body of OnLoadRoutine (:218) after its first `yield return null`
    void LoadNow(ConfigNode& gameNode);
    void OnDestroy();                              // :196

    // :380 / :401 (static)
    static void GetContractCounts(float rep, int avgContracts, int& tier1, int& tier2, int& tier3);
    int CountContracts(Contract::ContractPrestige difficulty);                           // :435
    bool GenerateContracts(int& seed, Contract::ContractPrestige difficulty, int count); // :447
    std::unique_ptr<Contract> GenerateContract(int& seed, Contract::ContractPrestige difficulty);  // :472
    // :525 (contractType empty = WeightedContractChoice())
    std::unique_ptr<Contract> GenerateContract(int seed, Contract::ContractPrestige difficulty, const std::string& contractType);
    bool WithdrawSurplusContracts(Contract::ContractPrestige level, int maxAllowed);     // :536
    void UpdateContracts();                        // :591
    void RefreshContracts();                       // :359 (the [ContextMenu])
    void ResetContracts();                         // :659
    Contract* GetContractByGuid(const Guid& guid); // :679
    template <class T>
    std::vector<T*> GetCurrentContracts(const std::function<bool(T*)>& where = nullptr) {  // :694
        std::vector<T*> list;
        for (auto& c : contracts) {
            T* v = dynamic_cast<T*>(c.get());
            if (v != nullptr && (!where || where(v))) list.push_back(v);
        }
        return list;
    }
    template <class T>
    bool AnyCurrentContracts(const std::function<bool(T*)>& where = nullptr) {             // :706
        for (int i = static_cast<int>(contracts.size()) - 1; i >= 0; --i) {
            T* v = dynamic_cast<T*>(contracts[static_cast<size_t>(i)].get());
            if (v != nullptr && (!where || where(v))) return true;
        }
        return false;
    }
    template <class T>
    std::vector<T*> GetCurrentActiveContracts(const std::function<bool(T*)>& where = nullptr) {  // :719
        std::vector<T*> list;
        for (auto& c : contracts) {
            T* v = dynamic_cast<T*>(c.get());
            if (v != nullptr && v->ContractState() == Contract::State::Active && (!where || where(v))) list.push_back(v);
        }
        return list;
    }
    template <class T>
    bool AnyCurrentActiveContracts(const std::function<bool(T*)>& where = nullptr) {       // :731
        for (int i = static_cast<int>(contracts.size()) - 1; i >= 0; --i) {
            T* v = dynamic_cast<T*>(contracts[static_cast<size_t>(i)].get());
            if (v != nullptr && v->ContractState() == Contract::State::Active && (!where || where(v))) return true;
        }
        return false;
    }
    bool HasCompletedContract(const std::string& typeName);                                // :743
    template <class T>
    std::vector<T*> GetCompletedContracts(const std::function<bool(T*)>& where = nullptr) {  // :762
        std::vector<T*> list;
        for (auto& c : contractsFinished) {
            T* v = dynamic_cast<T*>(c.get());
            if (v != nullptr && v->ContractState() == Contract::State::Completed && (!where || where(v))) list.push_back(v);
        }
        return list;
    }
    template <class T>
    bool AnyCompletedContracts(const std::function<bool(T*)>& where = nullptr) {           // :773
        for (int i = static_cast<int>(contractsFinished.size()) - 1; i >= 0; --i) {
            T* v = dynamic_cast<T*>(contractsFinished[static_cast<size_t>(i)].get());
            if (v != nullptr && v->ContractState() == Contract::State::Completed && (!where || where(v))) return true;
        }
        return false;
    }
    int GetActiveContractCount();                  // :785
    void RebuildContracts();                       // :798
    void ClearContractsCurrent();                  // :829
    void ClearContractsFinished();                 // :838

    // weights
    static void ResetWeights();                                                            // :953
    static Contract* nullContract() { return nullptr; }
    static std::string WeightedContractChoice();   // :974 (the type name; empty = null)
    static CelestialBody* WeightedBodyChoice(const std::vector<CelestialBody*>& bodies, KSPRandom* generator = nullptr);  // :1008
    static void WeightAssignment(const std::string& name, int amount, bool ignoreLimits = false);  // :1049
    static void WeightAdjustment(const std::string& name, int delta, bool ignoreLimits = false);   // :1056
    static void AdjustWeight(const std::string& name, Contract* contract);                          // :1067
    static int WeightDefaultValue() { return WeightDefault; }
    static int WeightMinimumValue() { return WeightMinimum; }
    static int WeightMaximumValue() { return WeightMaximum; }
    static int TryGetWeightValue(const std::string& name) { return TryGetWeight(name); }
    void GenerateMandatoryContracts();             // :1100
    void SpawnPreBuiltShip(const std::string& craftUrl);    // :1148 (ConstructShip is the spawn seam)

    // the coroutines (kept for tests that drive them)
    Enumerator UpdateDaemon();                     // :555
    Enumerator OnLoadRoutine(ConfigNode::NodePtr gameNode);  // :218

    // event handlers (private in C#)
    void OnNodeReached(ProgressNode* node);                    // :351
    void OnReputationChanged(float newRep, TransactionReasons reason);  // :356
    void OnSceneLoadRequested(GameScenes scene);               // :637
    void OnFlightReady();                                      // :642
    void OnFlightGlobalsReady(bool ready);                     // :647
    void OnVesselChange(Vessel* vessel);                       // :652
    void OnMissionControlSpawned();                            // :1095

    std::vector<std::unique_ptr<Contract>> contracts;          // :35 (SerializeField list)
    std::vector<std::unique_ptr<Contract>> contractsFinished;  // :37

private:
    Contract* LoadContract(ConfigNode& cNode, std::unique_ptr<Contract>& out);   // :290
    void RegisterContracts();                                                     // :306
    static int ClampedWeight(int weight);                                         // :899
    static int TryGetWeight(const std::string& name);                             // :904
    static int TryGetWeight(const std::string& name, bool& success);              // :914
    void LoadContractWeights(ConfigNode& gameNode);                               // :921
    ConfigNode::NodePtr SaveContractWeights();                                    // :1008-ish
    static void GenerateTypes();                                                  // :80

    int finishedContractIDCheck = 5;               // :33
    double lastUpdate = -1000.0;                   // :38
    std::string version = "-1";                    // :40
    bool updateDaemonRunning = false;              // :50
    bool requireRefresh = false;                   // :52
    bool eventsAdded_ = false;
    int coroutineOwner_ = 0;
    static inline float maxWarpFactorForUpdate = 100.0f;   // :45
    static inline double updateInterval = 10.0;            // :47
    static inline int WeightDefault = 30;                  // :57
    static inline int WeightMinimum = 10;                  // :59
    static inline int WeightMaximum = 90;                  // :61
    static inline int WeightAcceptDelta = 12;              // :63
    static inline int WeightDeclineDelta = -8;             // :65
    static inline int WeightWithdrawReadDelta = -2;        // :67
    static inline int WeightWithdrawSeenDelta = -1;        // :69
};

}  // namespace ksp::Contracts
