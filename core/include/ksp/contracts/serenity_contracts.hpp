// WP-53: the Breaking Ground contracts (Expansions/Serenity/Contracts/*.cs) on the WP-37 contract system: the contracts DeployedScienceContract,
// ROCScienceArmContract, ROCScienceRetrievalContract and their parameters CollectDeployedScience, CollectROCScienceArm, CollectROCScienceRetrieval.
//
// ContractDefs: the DeployedScience / ROCScienceArm / ROCScienceRetrieval sections of Contracts.cfg are only read when Serenity is installed
// (ContractDefs.cs:2307); the shipped Contracts.cfg of the base game has none of them, so the C# static initialisers are the values
// (ContractDefs.cs:817-950, `SerenityContractDefs`); a loaded ContractDefsLite node of the same name overrides them like SystemUtilities.LoadNode.
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "ksp/contracts/contract.hpp"
#include "ksp/contracts/parameters.hpp"

namespace ksp {
class CelestialBody;
class ProtoVessel;
class ScienceSubject;
struct AvailablePart;
namespace KSP::UI::Screens {
class MissionRecoveryDialog;
}
}  // namespace ksp

namespace ksp::Expansions::Serenity::Contracts {

// ContractDefs.DeployedScience / ROCScienceArm / ROCScienceRetrieval
struct SerenityContractDefs {
    struct Expire { int MinimumExpireDays = 1, MaximumExpireDays = 7, DeadlineDays = 2130; };
    // ContractDefs.cs:817
    struct DeployedScienceDefs {
        Expire expire;
        float FundsBaseAdvance = 10000.0f, FundsBaseReward = 21000.0f, FundsBaseFailure = 12000.0f;
        float ScienceBaseReward = 0.0f;
        float ReputationBaseReward = 6.0f, ReputationBaseFailure = 3.0f;
        int MaximumExistent = 5;
        float SciencePercentage = 50.0f;
    };
    // ContractDefs.cs:854
    struct ROCScienceArmDefs {
        Expire expire;
        float SimpleArmAdvance = 8000.0f, SimpleArmReward = 17000.0f, SimpleArmFailure = 10000.0f;
        float ComplexArmAdvance = 10000.0f, ComplexArmReward = 21000.0f, ComplexArmFailure = 12000.0f;
        float AdvancedArmAdvance = 12000.0f, AdvancedArmReward = 25000.0f, AdvancedArmFailure = 17000.0f;
        float ScienceSimpleArmReward = 0.0f, ScienceComplexArmReward = 0.0f, ScienceAdvancedArmReward = 0.0f;
        float ReputationSimpleArmReward = 2.0f, ReputationSimpleArmFailure = 2.0f, ReputationComplexArmReward = 2.0f, ReputationComplexArmFailure = 2.0f,
              ReputationAdvancedArmReward = 2.0f, ReputationAdvancedArmFailure = 2.0f;
        int MaximumExistent = 3;
        float SimpleArmPercentage = 40.0f, ComplexArmPercentage = 70.0f;
    };
    // ContractDefs.cs:917
    struct ROCScienceRetrievalDefs {
        Expire expire;
        float FundsAdvance = 22000.0f, FundsReward = 35000.0f, FundsFailure = 27000.0f;
        float ScienceReward = 0.0f;
        float ReputationReward = 2.0f, ReputationFailure = 2.0f;
        int MaximumExistent = 2;
    };
    // The values (C# initialisers, overridden by the loaded Contracts.cfg nodes when Serenity is installed). Reloaded on every call.
    static DeployedScienceDefs DeployedScience();
    static ROCScienceArmDefs ROCScienceArm();
    static ROCScienceRetrievalDefs ROCScienceRetrieval();
};

// What the contracts read from systems that are other work packages
struct SerenityContractEnvironment {
    // PartLoader.Instance.GetAvailableDeployedScienceExpParts() / GetAvailableRobotArmScannerParts(): derived from ScienceEnvironment::loadedParts()
    // (the parts carrying ModuleGroundExperiment / ModuleRobotArmScanner, unlocked by GetUnlockedCargoParts); these overrides replace the derivation.
    static inline std::function<std::vector<const AvailablePart*>()> availableDeployedScienceExpParts;
    static inline std::function<std::vector<const AvailablePart*>()> availableRobotArmScannerParts;
    static void Reset();
};
std::vector<const AvailablePart*> GetAvailableDeployedScienceExpParts();  // PartLoader.cs:2147
std::vector<const AvailablePart*> GetAvailableRobotArmScannerParts();     // PartLoader.cs:2177

// CollectDeployedScience.cs:9
class CollectDeployedScience : public ::ksp::Contracts::ContractParameter {
public:
    CollectDeployedScience() = default;                                                                              // :30
    CollectDeployedScience(CelestialBody* targetBody, std::string subjectId, std::string scienceTitle, float sciencePercentage);  // :30
    ~CollectDeployedScience() override;
    std::string TypeName() const override { return "CollectDeployedScience"; }
    CelestialBody* TargetBody() const { return targetBody; }
    const std::string& SubjectId() const { return subjectId; }
    float SciencePercentage = 0.0f;  // :16 public (separate from the protected field)
    float ScienceCollected = 0.0f;   // :20
    std::string ScienceTitle;        // :24
    std::string GetHashString() override;     // :43
    std::string GetTitle() override;          // :48
    void OnLoad(ConfigNode& node) override;   // :57
    void OnSave(ConfigNode& node) override;   // :70
    void OnRegister() override;               // :82
    void OnUnregister() override;             // :87
    void OnScience(float science, ScienceSubject* subject, ProtoVessel* pv, bool reverseEngineered);  // :92
    // protected state (:14-:22)
    CelestialBody* targetBody = nullptr;
    std::string subjectId;
    float sciencePercentage = 0.0f;
    float scienceCollected = 0.0f;
    std::string scienceTitle;
};

// CollectROCScienceArm.cs:9
class CollectROCScienceArm : public ::ksp::Contracts::ContractParameter {
public:
    CollectROCScienceArm() = default;                                                                                    // :32
    CollectROCScienceArm(CelestialBody* targetBody, std::string subjectId, std::string rocType, std::string scienceTitle, float sciencePercentage);  // :36
    ~CollectROCScienceArm() override;
    std::string TypeName() const override { return "CollectROCScienceArm"; }
    CelestialBody* TargetBody() const { return targetBody; }
    const std::string& SubjectId() const { return subjectId; }
    const std::string& ScienceTitle() const { return scienceTitle; }
    float SciencePercentage = 0.0f, ScienceCollected = 0.0f;  // :16 / :20 public
    std::string GetHashString() override;     // :46
    std::string GetTitle() override;          // :51
    void OnLoad(ConfigNode& node) override;   // :60
    void OnSave(ConfigNode& node) override;   // :91
    void OnRegister() override;               // :104
    void OnUnregister() override;             // :109
    void OnScience(float science, ScienceSubject* subject, ProtoVessel* pv, bool reverseEngineered);  // :114
    CelestialBody* targetBody = nullptr;
    std::string subjectId;
    float sciencePercentage = 0.0f, scienceCollected = 0.0f;
    std::string rocType, scienceTitle;
};

// CollectROCScienceRetrieval.cs:10
class CollectROCScienceRetrieval : public ::ksp::Contracts::ContractParameter {
public:
    CollectROCScienceRetrieval() = default;                                                                       // :26
    CollectROCScienceRetrieval(CelestialBody* targetBody, std::string subjectId, std::string rocType, std::string scienceTitle);  // :26
    ~CollectROCScienceRetrieval() override;
    std::string TypeName() const override { return "CollectROCScienceRetrieval"; }
    CelestialBody* TargetBody() const { return targetBody; }
    const std::string& SubjectId() const { return subjectId; }
    const std::string& ScienceTitle() const { return scienceTitle; }
    std::string GetHashString() override;     // :38
    std::string GetTitle() override;          // :43
    void OnLoad(ConfigNode& node) override;   // :56
    void OnSave(ConfigNode& node) override;   // :85
    void OnRegister() override;               // :96
    void OnUnregister() override;             // :101
    void OnVesselRecoveryProcessing(ProtoVessel* pv, KSP::UI::Screens::MissionRecoveryDialog* mrDialog, float recoveryScore);  // :106
    CelestialBody* targetBody = nullptr;
    std::string subjectId, rocType, scienceTitle;
};

// DeployedScienceContract.cs:13
class DeployedScienceContract : public ::ksp::Contracts::Contract {
public:
    std::string TypeName() const override { return "DeployedScienceContract"; }
    CelestialBody* TargetBody() const { return targetBody; }
    const std::string& SubjectId() const { return subjectId; }
    float SciencePercentage = 0.0f;  // :36 public
    bool MeetRequirements() override;                                        // :44
    std::vector<CelestialBody*> WeightBodies() override { return {targetBody}; }
    bool HasWeightBodies() override { return true; }
    std::string GetDescription() override;                                   // :66
    std::string GetSynopsys() override;                                      // :71
    std::string GetTitle() override;                                         // :76
    std::string MessageCompleted() override;                                 // :81
    void OnLoad(ConfigNode& node) override;                                  // :86
    void OnSave(ConfigNode& node) override;                                  // :105
    bool Generate() override;                                                // :115
    CelestialBody* targetBody = nullptr;
    std::string subjectId;
    float sciencePercentage = 0.0f;
    std::string scienceTitle;

private:
    struct PotentialScienceCombo {
        std::string experimentId;
        CelestialBody* body;
    };
    bool GenerateContract_Surface();                                         // :124
    static bool PotentialsContainBody(CelestialBody* body, const std::vector<PotentialScienceCombo>& potentialCombos, const std::string& id);  // :203
};

// ROCScienceArmContract.cs:13
class ROCScienceArmContract : public ::ksp::Contracts::Contract {
public:
    std::string TypeName() const override { return "ROCScienceArmContract"; }
    CelestialBody* TargetBody() const { return targetBody; }
    const std::string& SubjectId() const { return subjectId; }
    bool MeetRequirements() override;                                        // :53
    std::vector<CelestialBody*> WeightBodies() override { return {targetBody}; }
    bool HasWeightBodies() override { return true; }
    std::string GetDescription() override;                                   // :71
    std::string GetSynopsys() override;                                      // :85
    std::string GetTitle() override;                                         // :103
    std::string MessageCompleted() override;                                 // :116
    void OnLoad(ConfigNode& node) override;                                  // :121
    void OnSave(ConfigNode& node) override;                                  // :155
    bool Generate() override;                                                // :173
    CelestialBody* targetBody = nullptr;
    std::string subjectId;
    float sciencePercentage = 0.0f;
    std::string rocType, scienceTitle, scannerArmName, scannerArmTitle;
    std::vector<std::string> biomes;
};

// ROCScienceRetrievalContract.cs:13
class ROCScienceRetrievalContract : public ::ksp::Contracts::Contract {
public:
    std::string TypeName() const override { return "ROCScienceRetrievalContract"; }
    CelestialBody* TargetBody() const { return targetBody; }
    const std::string& SubjectId() const { return subjectId; }
    bool MeetRequirements() override;                                        // :46
    std::vector<CelestialBody*> WeightBodies() override { return {targetBody}; }
    bool HasWeightBodies() override { return true; }
    std::string GetDescription() override;                                   // :68
    std::string GetSynopsys() override;                                      // :73
    std::string GetTitle() override;                                         // :91
    std::string MessageCompleted() override;                                 // :96
    void OnLoad(ConfigNode& node) override;                                  // :101
    void OnSave(ConfigNode& node) override;                                  // :127
    bool Generate() override;                                                // :142
    CelestialBody* targetBody = nullptr;
    std::string subjectId;
    std::string rocType, scienceTitle;
    std::vector<std::string> biomes;
};

// registration of the six types with the contract system (ContractSystem::RegisterContractType / RegisterParameterType)
void Install();

}  // namespace ksp::Expansions::Serenity::Contracts
