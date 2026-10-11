// WP-38 slice 2: FinePrint/Contracts/Parameters/{VesselDestinationParameter,KerbalDestinationParameter,KerbalGeeAdventureParameter,
// KerbalTourParameter}.cs: the parameters driven by the flight logs and the kerbal events.
#include "ksp/contracts/agents.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/fineprint/parameters.hpp"
#include "ksp/save/crew.hpp"
#include "ksp/save/game.hpp"
#include "ksp/vessel/module_trip_logger.hpp"
#include "param_common.hpp"

namespace ksp::Contracts::Parameters {

using namespace ksp::fpcommon;
using GE = events::GameEvents;
namespace L = Lite;

// ---- VesselDestinationParameter -------------------------------------------------------------------------------------------------------

// VesselDestinationParameter.cs:19
VesselDestinationParameter::VesselDestinationParameter() : targetBody(Planetarium::fetch->Home), targetType(FlightLog_EntryType::Suborbit) {}
// VesselDestinationParameter.cs:25
VesselDestinationParameter::VesselDestinationParameter(CelestialBody* targetBody_, FlightLog_EntryType targetType_)
    : targetBody(targetBody_), targetType(targetType_) {}
// VesselDestinationParameter.cs:31
std::string VesselDestinationParameter::GetHashString() { return I(L::SuperSeed(*Root())) + ID(); }
// VesselDestinationParameter.cs:36
std::string VesselDestinationParameter::GetTitle() {
    switch (targetType) {
        default: return Fmt("#autoLOC_286272");
        case FlightLog_EntryType::Land: return Fmt("#autoLOC_286262", {targetBody->displayName()});
        case FlightLog_EntryType::Flight: return Fmt("#autoLOC_286266", {targetBody->displayName()});
        case FlightLog_EntryType::Flyby: return Fmt("#autoLOC_286268", {targetBody->displayName()});
        case FlightLog_EntryType::Orbit: return Fmt("#autoLOC_286270", {targetBody->displayName()});
        case FlightLog_EntryType::Suborbit: return Fmt("#autoLOC_286264", {targetBody->displayName()});
    }
}
// VesselDestinationParameter.cs:56
void VesselDestinationParameter::OnRegister() {
    SetDisableOnStateChange(false);
    if (Root()->ContractState() == Contract::State::Active) {
        GE::OnFlightLogRecorded.Add(this, &VesselDestinationParameter::CheckFlightLog);
        GE::onVesselWasModified.Add(this, &VesselDestinationParameter::CheckFlightLog);
        eventsAdded = true;
    }
}
// VesselDestinationParameter.cs:68
void VesselDestinationParameter::OnUnregister() {
    if (eventsAdded) {
        GE::OnFlightLogRecorded.Remove(this, &VesselDestinationParameter::CheckFlightLog);
        GE::onVesselWasModified.Remove(this, &VesselDestinationParameter::CheckFlightLog);
    }
}
// VesselDestinationParameter.cs:79
void VesselDestinationParameter::OnReset() {
    if (L::FlightIsReady(true)) CheckFlightLog(FlightRegistry::ActiveVessel);
}
// VesselDestinationParameter.cs:87
void VesselDestinationParameter::OnSave(ConfigNode& node) {
    node.AddValue("targetBody", static_cast<int32_t>(targetBody->flightGlobalsIndex));
    node.AddValue("targetType", dotnet::EnumToString(targetType));
}
// VesselDestinationParameter.cs:93
void VesselDestinationParameter::OnLoad(ConfigNode& node) {
    L::LoadNode(node, "VesselDestinationParameter", "targetBody", targetBody, Planetarium::fetch->Home);
    L::LoadNode(node, "VesselDestinationParameter", "targetType", targetType, FlightLog_EntryType::Suborbit);
}
// VesselDestinationParameter.cs:99
void VesselDestinationParameter::CheckFlightLog(Vessel* v) {
    if (static_cast<int>(v->vesselType) <= static_cast<int>(VesselType::SpaceObject)) return;
    VesselTripLog trip = VesselTripLog::FromVessel(*v);
    std::vector<FlightLog::Entry>& entries = trip.Log.Entries();
    int count = static_cast<int>(entries.size());
    for (;;) {
        if (count-- > 0) {
            FlightLog::Entry& entry = entries[static_cast<size_t>(count)];
            if (entry.type != dotnet::EnumToString(targetType) || entry.target != targetBody->GetName()) continue;
            SetComplete();
            return;
        }
        SetIncomplete();
        return;
    }
}

// ---- KerbalDestinationParameter -------------------------------------------------------------------------------------------------------

// KerbalDestinationParameter.cs:21
KerbalDestinationParameter::KerbalDestinationParameter()
    : targetBody(Planetarium::fetch->Home), targetType(FlightLog_EntryType::Suborbit), kerbalName("Jebediah Kerman") {}
// KerbalDestinationParameter.cs:29
KerbalDestinationParameter::KerbalDestinationParameter(CelestialBody* targetBody_, FlightLog_EntryType targetType_, const std::string& kerbalName_)
    : targetBody(targetBody_), targetType(targetType_), kerbalName(kerbalName_) {}
// KerbalDestinationParameter.cs:37
std::string KerbalDestinationParameter::GetHashString() { return I(L::SuperSeed(*Root())) + ID(); }
// KerbalDestinationParameter.cs:42
std::string KerbalDestinationParameter::GetTitle() {
    switch (targetType) {
        default: return Fmt("#autoLOC_283280");
        case FlightLog_EntryType::Land: return Fmt("#autoLOC_283270", {targetBody->displayName()});
        case FlightLog_EntryType::Flight: return Fmt("#autoLOC_283274", {targetBody->displayName()});
        case FlightLog_EntryType::Flyby: return Fmt("#autoLOC_283276", {targetBody->displayName()});
        case FlightLog_EntryType::Orbit: return Fmt("#autoLOC_283278", {targetBody->displayName()});
        case FlightLog_EntryType::Suborbit: return Fmt("#autoLOC_283272", {targetBody->displayName()});
    }
}
// KerbalDestinationParameter.cs:62
void KerbalDestinationParameter::OnRegister() {
    SetDisableOnStateChange(true);
    if (Root()->ContractState() == Contract::State::Active && !CheckFlightLogs()) {
        GE::OnFlightLogRecorded.Add(this, &KerbalDestinationParameter::CheckFlightLog);
        eventsAdded = true;
    }
}
// KerbalDestinationParameter.cs:74
void KerbalDestinationParameter::OnUnregister() {
    if (eventsAdded) GE::OnFlightLogRecorded.Remove(this, &KerbalDestinationParameter::CheckFlightLog);
}
// KerbalDestinationParameter.cs:82
void KerbalDestinationParameter::OnSave(ConfigNode& node) {
    node.AddValue("targetBody", static_cast<int32_t>(targetBody->flightGlobalsIndex));
    node.AddValue("targetType", dotnet::EnumToString(targetType));
    node.AddValue("kerbalName", kerbalName);
}
// KerbalDestinationParameter.cs:90
void KerbalDestinationParameter::OnLoad(ConfigNode& node) {
    L::LoadNode(node, "KerbalDestinationParameter", "targetBody", targetBody, Planetarium::fetch->Home);
    L::LoadNode(node, "KerbalDestinationParameter", "targetType", targetType, FlightLog_EntryType::Suborbit);
    L::LoadNode(node, "KerbalDestinationParameter", "kerbalName", kerbalName, std::string("Jebediah Kerman"));
}
// KerbalDestinationParameter.cs:98
void KerbalDestinationParameter::CheckFlightLog(Vessel* v) {
    const std::vector<std::shared_ptr<ProtoCrewMember>> vesselCrew = v->GetVesselCrew();
    int count = static_cast<int>(vesselCrew.size());
    while (count-- > 0) {
        const std::shared_ptr<ProtoCrewMember>& protoCrewMember = vesselCrew[static_cast<size_t>(count)];
        if (protoCrewMember->name().value_or("") != kerbalName) continue;
        std::vector<FlightLog::Entry> list(protoCrewMember->flightLog.Entries());
        list.insert(list.end(), protoCrewMember->careerLog.Entries().begin(), protoCrewMember->careerLog.Entries().end());
        for (int num = static_cast<int>(list.size()) - 1; num >= 0; num--) {
            const FlightLog::Entry& entry = list[static_cast<size_t>(num)];
            if (entry.type == dotnet::EnumToString(targetType) && entry.target == targetBody->GetName()) SetComplete();
        }
    }
}
// KerbalDestinationParameter.cs:122
bool KerbalDestinationParameter::CheckFlightLogs() {
    std::shared_ptr<ProtoCrewMember> protoCrewMember;
    Game* game = SaveEnvironment::Current().currentGame;
    if (game != nullptr) {
        if (game->CrewRoster->Exists(kerbalName)) protoCrewMember = (*game->CrewRoster)[kerbalName];
        if (protoCrewMember != nullptr) {
            std::vector<FlightLog::Entry> list(protoCrewMember->flightLog.Entries());
            list.insert(list.end(), protoCrewMember->careerLog.Entries().begin(), protoCrewMember->careerLog.Entries().end());
            int num = static_cast<int>(list.size()) - 1;
            while (num >= 0) {
                const FlightLog::Entry& entry = list[static_cast<size_t>(num)];
                if (!(entry.type == dotnet::EnumToString(targetType)) || !(entry.target == targetBody->GetName())) {
                    num--;
                    continue;
                }
                SetComplete();
                return true;
            }
        }
    }
    return false;
}

// ---- KerbalGeeAdventureParameter ------------------------------------------------------------------------------------------------------

// KerbalGeeAdventureParameter.cs:15
KerbalGeeAdventureParameter::KerbalGeeAdventureParameter() : kerbalName("Jebediah Kerman") {}
// KerbalGeeAdventureParameter.cs:20
KerbalGeeAdventureParameter::KerbalGeeAdventureParameter(const std::string& kerbalName_) : kerbalName(kerbalName_) { allowPartialFailure = true; }
// KerbalGeeAdventureParameter.cs:26
std::string KerbalGeeAdventureParameter::GetHashString() { return I(L::SuperSeed(*Root())) + ID(); }
// KerbalGeeAdventureParameter.cs:31
std::string KerbalGeeAdventureParameter::GetTitle() { return Fmt("#autoLOC_283371", {L::StringUtilities::ShortKerbalName(kerbalName)}); }
// KerbalGeeAdventureParameter.cs:36
void KerbalGeeAdventureParameter::OnRegister() {
    SetDisableOnStateChange(false);
    if (Root()->ContractState() == Contract::State::Active) {
        GE::onCrewKilled.Add(this, &KerbalGeeAdventureParameter::OnCrewKilled);
        GE::onKerbalPassedOutFromGeeForce.Add(this, &KerbalGeeAdventureParameter::OnKerbalPassedOut);
        eventsAdded = true;
    }
}
// KerbalGeeAdventureParameter.cs:48
void KerbalGeeAdventureParameter::OnUnregister() {
    if (eventsAdded) {
        GE::onCrewKilled.Remove(this, &KerbalGeeAdventureParameter::OnCrewKilled);
        GE::onKerbalPassedOutFromGeeForce.Remove(this, &KerbalGeeAdventureParameter::OnKerbalPassedOut);
    }
}
// KerbalGeeAdventureParameter.cs:58
void KerbalGeeAdventureParameter::OnSave(ConfigNode& node) { node.AddValue("kerbalName", kerbalName); }
// KerbalGeeAdventureParameter.cs:63
void KerbalGeeAdventureParameter::OnLoad(ConfigNode& node) { L::LoadNode(node, "KerbalGeeAdventureParameter", "kerbalName", kerbalName, std::string("Jebediah Kerman")); }
// KerbalGeeAdventureParameter.cs:68
void KerbalGeeAdventureParameter::OnCrewKilled(EventReport* evt) {
    if (evt->sender == kerbalName) {
        if (state != ParameterState::Failed) {
            allowPartialFailure = false;
            SetFailed();
        } else {
            Root()->Fail();
        }
    }
}
// KerbalGeeAdventureParameter.cs:81
void KerbalGeeAdventureParameter::OnKerbalPassedOut(ProtoCrewMember* pcm) {
    if (pcm->name().value_or("") == kerbalName && AllChildParametersComplete()) {
        if (pcm != nullptr && pcm->type() == ProtoCrewMember_KerbalType::Tourist) pcm->hasToured = true;
        SetComplete();
    }
}

// ---- KerbalTourParameter --------------------------------------------------------------------------------------------------------------

// KerbalTourParameter.cs:19
KerbalTourParameter::KerbalTourParameter() : kerbalName("Jebediah Kerman"), kerbalGender(ProtoCrewMember_Gender::Male) {}
// KerbalTourParameter.cs:25
KerbalTourParameter::KerbalTourParameter(const std::string& kerbalName_, ProtoCrewMember_Gender kerbalGender_)
    : kerbalName(kerbalName_), kerbalGender(kerbalGender_) {
    allowPartialFailure = true;
}
// KerbalTourParameter.cs:32
std::string KerbalTourParameter::GetHashString() { return I(L::SuperSeed(*Root())) + ID(); }
// KerbalTourParameter.cs:37
std::string KerbalTourParameter::GetTitle() {
    return Fmt("#autoLOC_283467", {L::LingoonaGrammar::LocalizeName(L::StringUtilities::ShortKerbalName(kerbalName), kerbalGender)});
}
// KerbalTourParameter.cs:42
void KerbalTourParameter::OnRegister() {
    SetDisableOnStateChange(false);
    if (Root()->ContractState() == Contract::State::Active) {
        GE::onCrewKilled.Add(this, &KerbalTourParameter::OnCrewKilled);
        GE::onVesselRecoveryProcessing.Add(this, &KerbalTourParameter::OnVesselRecovered);
        GE::onKerbalInactiveChange.Add(this, &KerbalTourParameter::OnKerbalInactiveChange);
        eventsAdded = true;
    }
}
// KerbalTourParameter.cs:56
void KerbalTourParameter::OnUnregister() {
    if (eventsAdded) {
        GE::onCrewKilled.Remove(this, &KerbalTourParameter::OnCrewKilled);
        GE::onVesselRecoveryProcessing.Remove(this, &KerbalTourParameter::OnVesselRecovered);
        GE::onKerbalInactiveChange.Remove(this, &KerbalTourParameter::OnKerbalInactiveChange);
    }
}
// KerbalTourParameter.cs:69
void KerbalTourParameter::OnSave(ConfigNode& node) {
    node.AddValue("kerbalName", kerbalName);
    node.AddValue("kerbalGender", dotnet::EnumToString(kerbalGender));
}
// KerbalTourParameter.cs:75
void KerbalTourParameter::OnLoad(ConfigNode& node) {
    L::LoadNode(node, "KerbalTourParameter", "kerbalName", kerbalName, std::string("Jebediah Kerman"));
    L::LoadNode(node, "KerbalTourParameter", "kerbalGender", kerbalGender, ProtoCrewMember_Gender::Male);
}
// KerbalTourParameter.cs:81
void KerbalTourParameter::OnCrewKilled(EventReport* evt) {
    if (evt->sender == kerbalName) {
        if (state != ParameterState::Failed) {
            allowPartialFailure = false;
            SetFailed();
        } else {
            Root()->Fail();
        }
    }
}
// KerbalTourParameter.cs:94
void KerbalTourParameter::OnParameterStateChange(ContractParameter* p) {
    (void)p;
    if (AllChildParametersComplete()) {
        std::shared_ptr<ProtoCrewMember> protoCrewMember = (*SaveEnvironment::Current().currentGame->CrewRoster)[kerbalName];
        if (protoCrewMember != nullptr && protoCrewMember->type() == ProtoCrewMember_KerbalType::Tourist) protoCrewMember->hasToured = true;
    }
}
// KerbalTourParameter.cs:107
void KerbalTourParameter::OnVesselRecovered(ProtoVessel* v, KSP::UI::Screens::MissionRecoveryDialog* mrd, float x) {
    (void)mrd;
    (void)x;
    const auto& crew = v->GetVesselCrew();
    const std::shared_ptr<ProtoCrewMember> member = (*SaveEnvironment::Current().currentGame->CrewRoster)[kerbalName];
    bool contains = false;
    for (const auto& c : crew)
        if (c == member) contains = true;
    if (contains && AllChildParametersComplete()) SetComplete();
}
// KerbalTourParameter.cs:114
void KerbalTourParameter::OnKerbalInactiveChange(ProtoCrewMember* pcm, bool oldValue, bool newValue) {
    (void)oldValue;
    if (ContractDefsLite::B("Tour", "", "FailOnInactive") && newValue && pcm->name().value_or("") == kerbalName) {
        kerbalInactive = true;
        if (pcm != nullptr && pcm->type() == ProtoCrewMember_KerbalType::Tourist) pcm->hasToured = true;
        SetFailed();
    }
}
// KerbalTourParameter.cs:127
std::string KerbalTourParameter::GetMessageFailed() {
    if (kerbalInactive) return Fmt("#autoLOC_283554", {Title(), kerbalName});
    return ContractParameter::GetMessageFailed();
}

}  // namespace ksp::Contracts::Parameters
