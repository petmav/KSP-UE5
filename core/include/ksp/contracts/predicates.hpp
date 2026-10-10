// WP-37: the stock contract predicates (Assembly-CSharp/Contracts/Predicates/*.cs): AgentFlag, AgentParts, IsActiveVessel, IsControllable,
// LaunchID.
#pragma once

#include <string>

#include "ksp/contracts/agents.hpp"
#include "ksp/contracts/contract.hpp"

namespace ksp::Contracts::Predicates {

// AgentFlag.cs:8
class AgentFlag : public ContractPredicate {
public:
    explicit AgentFlag(IContractParameterHost* parent) : ContractPredicate(parent) {}         // :14
    AgentFlag(IContractParameterHost* contract, Agents::Agent* agent_) : ContractPredicate(contract), agent(agent_) {}  // :19
    std::string TypeName() const override { return "AgentFlag"; }
    Agents::Agent* GetAgent() const { return agent; }                                         // :12
    bool Test(Vessel* vessel) override;                                                       // :25
    bool Test(ProtoVessel* vessel) override;                                                  // :40
    std::string GetDescription() override;                                                    // :55
    void OnLoad(ConfigNode& node) override;                                                   // :60
    void OnSave(ConfigNode& node) override;                                                   // :75
    Agents::Agent* agent = nullptr;
};

// AgentParts.cs:8
class AgentParts : public ContractPredicate {
public:
    explicit AgentParts(IContractParameterHost* parent) : ContractPredicate(parent) {}         // :16
    AgentParts(IContractParameterHost* parent, Agents::Agent* agent_, int partCount_);        // :21
    std::string TypeName() const override { return "AgentParts"; }
    Agents::Agent* GetAgent() const { return agent; }                                         // :12
    int PartCount() const { return partCount; }                                               // :14
    bool Test(Vessel* vessel) override;                                                       // :35
    bool Test(ProtoVessel* vessel) override;                                                  // :50
    std::string GetDescription() override;                                                    // :65
    void OnLoad(ConfigNode& node) override;                                                   // :81
    void OnSave(ConfigNode& node) override;                                                   // :101
    Agents::Agent* agent = nullptr;
    int partCount = 0;
};

// IsActiveVessel.cs:5
class IsActiveVessel : public ContractPredicate {
public:
    explicit IsActiveVessel(IContractParameterHost* parent) : ContractPredicate(parent) {}     // :7
    std::string TypeName() const override { return "IsActiveVessel"; }
    bool Test(Vessel* vessel) override;                                                       // :11
    bool Test(ProtoVessel*) override { return false; }                                        // :16
};

// IsControllable.cs:5
class IsControllable : public ContractPredicate {
public:
    explicit IsControllable(IContractParameterHost* parent) : ContractPredicate(parent) {}     // :7
    std::string TypeName() const override { return "IsControllable"; }
    bool Test(Vessel* vessel) override;                                                       // :11
    bool Test(ProtoVessel* protoVessel) override;                                             // :16
    std::string GetDescription() override { return std::string(); }                           // :21 (null)
};

// LaunchID.cs:6
class LaunchID : public ContractPredicate {
public:
    explicit LaunchID(IContractParameterHost* parent);                                         // :14
    ~LaunchID() override;                                                                      // :19
    std::string TypeName() const override { return "LaunchID"; }
    // :12 `public uint ID => ID;` recurses forever in the C# (StackOverflowException): calling it throws
    uint32_t ID() const;
    double Date() const { return date; }                                                       // :14
    void OnContractStateChange(Contract::State state);                                         // :27
    bool Test(Vessel* vessel) override;                                                        // :36
    bool Test(ProtoVessel* vessel) override;                                                   // :41
    std::string GetDescription() override;                                                     // :64
    void OnLoad(ConfigNode& node) override;                                                    // :69
    void OnSave(ConfigNode& node) override;                                                    // :81

private:
    bool ContainsLaunchID_GreaterThanOrEqualTo(Vessel* vessel, uint32_t launchID);             // :46
    bool ContainsLaunchID_GreaterThanOrEqualTo(ProtoVessel* vessel, uint32_t launchID);        // :55
    uint32_t id = 0;
    double date = -1.0;
};

}  // namespace ksp::Contracts::Predicates
