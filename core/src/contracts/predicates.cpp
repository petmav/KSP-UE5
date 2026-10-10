// WP-37: the stock contract predicates (Assembly-CSharp/Contracts/Predicates/*.cs).
#include "ksp/contracts/predicates.hpp"

#include "ksp/contracts/contract_environment.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/parts/available_part.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/proto_vessel.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/contracts/contract_system.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp::Contracts::Predicates {

using KSP::Localization::Localizer;

// ---- AgentFlag -------------------------------------------------------------------------------------------------------------

// AgentFlag.cs:26
bool AgentFlag::Test(Vessel* vessel) {
    size_t num = 0;
    while (true) {
        if (num < vessel->parts.size()) {
            if (vessel->parts[num]->fields.flagURL.value_or(std::string()) == agent->LogoURL()) break;
            ++num;
            continue;
        }
        return false;
    }
    return true;
}

// AgentFlag.cs:45
bool AgentFlag::Test(ProtoVessel* vessel) {
    size_t num = 0;
    while (true) {
        if (num < vessel->protoPartSnapshots.size()) {
            if (vessel->protoPartSnapshots[num]->flagURL == agent->LogoURL()) break;
            ++num;
            continue;
        }
        return false;
    }
    return true;
}

// AgentFlag.cs:64
std::string AgentFlag::GetDescription() { return Localizer::Format("#autoLOC_270961", {agent->Title()}); }

// AgentFlag.cs:69
void AgentFlag::OnLoad(ConfigNode& node) {
    if (!node.HasValue("agent")) {
        log::Error("Contract Predicate " + TypeName() + " has invalid config.");
        return;
    }
    agent = Agents::AgentList::Instance->GetAgent(*node.GetValue("agent"));
    if (agent == nullptr) log::Error("Contract Predicate " + TypeName() + " has an invalid agent name.");
}

// AgentFlag.cs:83
void AgentFlag::OnSave(ConfigNode& node) { node.AddValue("agent", agent->Name()); }

// ---- AgentParts ------------------------------------------------------------------------------------------------------------

// AgentParts.cs:19
AgentParts::AgentParts(IContractParameterHost* parent, Agents::Agent* agent_, int partCount_) : ContractPredicate(parent) {
    if (partCount_ < 0) log::Error("Contract predicate: partCount must be >= 0");
    agent = agent_;
    partCount = partCount_;
}

// AgentParts.cs:35
bool AgentParts::Test(Vessel* vessel) {
    size_t num = 0;
    while (true) {
        if (num < vessel->parts.size()) {
            if (vessel->parts[num]->partInfo->manufacturer == agent->Name()) break;
            ++num;
            continue;
        }
        return false;
    }
    return true;
}

// AgentParts.cs:54
bool AgentParts::Test(ProtoVessel* vessel) {
    size_t num = 0;
    while (true) {
        if (num < vessel->protoPartSnapshots.size()) {
            if (vessel->protoPartSnapshots[num]->partInfo->manufacturer == agent->Name()) break;
            ++num;
            continue;
        }
        return false;
    }
    return true;
}

// AgentParts.cs:73
std::string AgentParts::GetDescription() {
    if (partCount == 0) return Localizer::Format("#autoLOC_271045", {agent->Title()});
    if (partCount == 1) return Localizer::Format("#autoLOC_271049", {agent->Title()});
    return Localizer::Format("Vessel must contain at least <<1>> parts made by <<2>>", {dotnet::FormatInt64(partCount), agent->Title()});
}

// AgentParts.cs:86
void AgentParts::OnLoad(ConfigNode& node) {
    if (node.HasValue("agent") && node.HasValue("count")) {
        agent = Agents::AgentList::Instance->GetAgent(*node.GetValue("agent"));
        if (agent == nullptr)
            log::Error("Contract Predicate " + TypeName() + " has an invalid agent name.");
        else
            partCount = dotnet::ParseInt32(*node.GetValue("count"));
    } else {
        log::Error("Contract Predicate " + TypeName() + " has invalid config.");
    }
}

// AgentParts.cs:106
void AgentParts::OnSave(ConfigNode& node) {
    node.AddValue("agent", agent->Name());
    node.AddValue("count", static_cast<int32_t>(partCount));
}

// ---- IsActiveVessel / IsControllable ----------------------------------------------------------------------------------------

// IsActiveVessel.cs:13
bool IsActiveVessel::Test(Vessel* vessel) { return vessel == FlightRegistry::ActiveVessel; }

// IsControllable.cs:13
bool IsControllable::Test(Vessel* vessel) { return vessel->IsControllable(); }
// IsControllable.cs:18
bool IsControllable::Test(ProtoVessel* protoVessel) { return protoVessel->wasControllable; }

// ---- LaunchID --------------------------------------------------------------------------------------------------------------

// LaunchID.cs:17
LaunchID::LaunchID(IContractParameterHost* parent) : ContractPredicate(parent) { parent->Root()->OnStateChange.Add(this, &LaunchID::OnContractStateChange); }

// C++ helper (see LaunchID.cs:19)
LaunchID::~LaunchID() {
    if (Root() != nullptr) Root()->OnStateChange.Remove(this, &LaunchID::OnContractStateChange);
}

// LaunchID.cs:13 (the C# property returns itself: StackOverflowException)
uint32_t LaunchID::ID() const { throw Exception("LaunchID.ID recurses forever (StackOverflowException) in the C#"); }

// LaunchID.cs:31
void LaunchID::OnContractStateChange(Contract::State state) {
    if (state == Contract::State::Active) {
        id = SaveEnvironment::Current().currentGame->launchID;
        date = SaveEnvironment::Current().currentGame->UniversalTime();
    }
}

// LaunchID.cs:40
bool LaunchID::Test(Vessel* vessel) { return vessel->launchTime >= date; }
// LaunchID.cs:40
bool LaunchID::Test(ProtoVessel* vessel) { return vessel->launchTime >= date; }

// LaunchID.cs:50
bool LaunchID::ContainsLaunchID_GreaterThanOrEqualTo(Vessel* vessel, uint32_t launchID) {
    size_t num = 0;
    while (true) {
        if (num < vessel->parts.size()) {
            if (vessel->parts[num]->fields.launchID >= launchID) break;
            ++num;
            continue;
        }
        return false;
    }
    return true;
}

// LaunchID.cs:50
bool LaunchID::ContainsLaunchID_GreaterThanOrEqualTo(ProtoVessel* vessel, uint32_t launchID) {
    size_t num = 0;
    while (true) {
        if (num < vessel->protoPartSnapshots.size()) {
            if (vessel->protoPartSnapshots[num]->launchID >= launchID) break;
            ++num;
            continue;
        }
        return false;
    }
    return true;
}

// LaunchID.cs:88
std::string LaunchID::GetDescription() { return Localizer::Format("#autoLOC_271214"); }

// LaunchID.cs:93
void LaunchID::OnLoad(ConfigNode& node) {
    if (node.HasValue("launchID")) {
        uint32_t u = 0;
        if (!dotnet::TryParseUInt32(*node.GetValue("launchID"), u)) throw dotnet::FormatException("Input string was not in a correct format.");
        id = u;
    }
    if (node.HasValue("date")) date = dotnet::ParseDouble(*node.GetValue("date"));
}

// LaunchID.cs:105
void LaunchID::OnSave(ConfigNode& node) {
    node.AddValue("launchID", id);
    node.AddValue("date", date);
}

}  // namespace ksp::Contracts::Predicates
