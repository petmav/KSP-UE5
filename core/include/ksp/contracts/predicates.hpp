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
    explicit AgentFlag(IContractParameterHost* parent) : ContractPredicate(parent) {}         // :15
    AgentFlag(IContractParameterHost* contract, Agents::Agent* agent_) : ContractPredicate(contract), agent(agent_) {}  // :20
    std::string TypeName() const override { return "AgentFlag"; }
    Agents::Agent* GetAgent() const { return agent; }                                         // (C++ addition)
    bool Test(Vessel* vessel) override;                                                       // :26
    bool Test(ProtoVessel* vessel) override;                                                  // :45
    std::string GetDescription() override;                                                    // :64
    void OnLoad(ConfigNode& node) override;                                                   // :69
    void OnSave(ConfigNode& node) override;                                                   // :83
    Agents::Agent* agent = nullptr;
};

// AgentParts.cs:8
class AgentParts : public ContractPredicate {
public:
    explicit AgentParts(IContractParameterHost* parent) : ContractPredicate(parent) {}         // :19
    AgentParts(IContractParameterHost* parent, Agents::Agent* agent_, int partCount_);        // :19
    std::string TypeName() const override { return "AgentParts"; }
    Agents::Agent* GetAgent() const { return agent; }                                         // (C++ addition)
    int PartCount() const { return partCount; }                                               // :17
    bool Test(Vessel* vessel) override;                                                       // :35
    bool Test(ProtoVessel* vessel) override;                                                  // :54
    std::string GetDescription() override;                                                    // :73
    void OnLoad(ConfigNode& node) override;                                                   // :86
    void OnSave(ConfigNode& node) override;                                                   // :106
    Agents::Agent* agent = nullptr;
    int partCount = 0;
};

// IsActiveVessel.cs:5
class IsActiveVessel : public ContractPredicate {
public:
    explicit IsActiveVessel(IContractParameterHost* parent) : ContractPredicate(parent) {}     // :8
    std::string TypeName() const override { return "IsActiveVessel"; }
    bool Test(Vessel* vessel) override;                                                       // :13
    bool Test(ProtoVessel*) override { return false; }                                        // :18
};

// IsControllable.cs:5
class IsControllable : public ContractPredicate {
public:
    explicit IsControllable(IContractParameterHost* parent) : ContractPredicate(parent) {}     // :8
    std::string TypeName() const override { return "IsControllable"; }
    bool Test(Vessel* vessel) override;                                                       // :13
    bool Test(ProtoVessel* protoVessel) override;                                             // :18
    std::string GetDescription() override { return std::string(); }                           // :23 (null)
};

// LaunchID.cs:6
class LaunchID : public ContractPredicate {
public:
    explicit LaunchID(IContractParameterHost* parent);                                         // :17
    ~LaunchID() override;                                                                      // (C++ addition)
    std::string TypeName() const override { return "LaunchID"; }
    // :12 `public uint ID => ID;` recurses forever in the C# (StackOverflowException): calling it throws
    uint32_t ID() const;
    double Date() const { return date; }                                                       // :15
    void OnContractStateChange(Contract::State state);                                         // :31
    bool Test(Vessel* vessel) override;                                                        // :40
    bool Test(ProtoVessel* vessel) override;                                                   // :40
    std::string GetDescription() override;                                                     // :88
    void OnLoad(ConfigNode& node) override;                                                    // :93
    void OnSave(ConfigNode& node) override;                                                    // :105

private:
    bool ContainsLaunchID_GreaterThanOrEqualTo(Vessel* vessel, uint32_t launchID);             // :50
    bool ContainsLaunchID_GreaterThanOrEqualTo(ProtoVessel* vessel, uint32_t launchID);        // :50
    uint32_t id = 0;
    double date = -1.0;
};

}  // namespace ksp::Contracts::Predicates
