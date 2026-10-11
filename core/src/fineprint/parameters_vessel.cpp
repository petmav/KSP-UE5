// WP-38 slice 2: FinePrint/Contracts/Parameters/{StabilityParameter,CrewCapacityParameter,CrewTraitParameter,MobileBaseParameter,
// LocationAndSituationParameter,VesselSystemsParameter,PartRequestParameter,ResourcePossessionParameter,ResourceExtractionParameter}.cs: the parameters
// that watch the active vessel (parts, crew, resources, situation).
#include <algorithm>
#include <cctype>
#include <cmath>

#include "ksp/contracts/agents.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/fineprint/parameters.hpp"
#include "ksp/kerbals/experience.hpp"
#include "ksp/save/crew.hpp"
#include "ksp/save/game.hpp"
#include "param_common.hpp"

namespace ksp::fpcommon {
std::string RoundedThousands(double x) {
    const double r = dotnet::Math::Round(x);
    long long v = static_cast<long long>(r < 0 ? -r : r);
    if (v == 0) return "";
    const std::string digits = std::to_string(v);
    std::string out;
    const int n = static_cast<int>(digits.size());
    for (int i = 0; i < n; ++i) {
        out += digits[static_cast<size_t>(i)];
        if ((n - 1 - i) % 3 == 0 && i != n - 1) out += ',';
    }
    return (r < 0 ? "-" : "") + out;
}
}  // namespace ksp::fpcommon

namespace ksp::Contracts::Parameters {

using namespace ksp::fpcommon;
using GE = events::GameEvents;
namespace L = Lite;

// ---- StabilityParameter ---------------------------------------------------------------------------------------------------------------

// StabilityParameter.cs:15
StabilityParameter::StabilityParameter() : holdSeconds(10.0f), holdTimer(10.0f) {}
// StabilityParameter.cs:21
StabilityParameter::StabilityParameter(float holdSeconds_) : holdSeconds(holdSeconds_), holdTimer(holdSeconds_) {}
// StabilityParameter.cs:27
std::string StabilityParameter::GetHashString() { return I(L::SuperSeed(*Root())) + ID(); }
// StabilityParameter.cs:32
std::string StabilityParameter::GetTitle() { return Fmt("#autoLOC_285413", {I(static_cast<int>(static_cast<double>(holdSeconds) + 0.5))}); }
// StabilityParameter.cs:37
void StabilityParameter::OnRegister() { SetDisableOnStateChange(false); }
// StabilityParameter.cs:42
void StabilityParameter::OnReset() { SetIncomplete(); }
// StabilityParameter.cs:47
void StabilityParameter::OnSave(ConfigNode& node) { node.AddValue("holdSeconds", holdSeconds); }
// StabilityParameter.cs:52
void StabilityParameter::OnLoad(ConfigNode& node) { L::LoadNode(node, "StabilityParameter", "holdSeconds", holdSeconds, 10.0f); }
// StabilityParameter.cs:57
void StabilityParameter::OnUpdate() {
    if (!L::FlightIsReady(Root()->ContractState(), Contract::State::Active, true)) return;
    Vessel* activeVessel = FlightRegistry::ActiveVessel;
    bool flag = true;
    if (activeVessel->geeForce > 0.1 && !activeVessel->LandedOrSplashed()) flag = false;
    if (flag && activeVessel->LandedOrSplashed() && activeVessel->srf_velocity.sqrMagnitude() > 0.09000000357627869) flag = false;
    if (flag && activeVessel->situation == Vessel::Situations::SUB_ORBITAL && activeVessel->heightFromTerrain < 500.0f && activeVessel->heightFromTerrain != -1.0f)
        flag = false;
    if (State() == ParameterState::Incomplete) {
        if (!flag)
            holdTimer = holdSeconds;
        else
            holdTimer = f32store(fx(holdTimer) - fx(L::FinePrintEnvironment::deltaTime ? L::FinePrintEnvironment::deltaTime() : Vessel::env.deltaTime));
        if (holdTimer <= 0.0f) SetComplete();
    }
    if (State() == ParameterState::Complete && !flag) SetIncomplete();
}

// ---- CrewCapacityParameter ------------------------------------------------------------------------------------------------------------

// CrewCapacityParameter.cs:24 SpecificVesselSibling
SpecificVesselParameter* CrewCapacityParameter::SpecificVesselSibling() {
    if (specificVesselSibling == nullptr) specificVesselSibling = Root()->GetParameterT<SpecificVesselParameter>();
    return specificVesselSibling;
}
// CrewCapacityParameter.cs:26
CrewCapacityParameter::CrewCapacityParameter() : targetCapacity(8), successCounter(0) {}
// CrewCapacityParameter.cs:32
CrewCapacityParameter::CrewCapacityParameter(int targetCapacity_) : targetCapacity(targetCapacity_), successCounter(0) {}
// CrewCapacityParameter.cs:38
std::string CrewCapacityParameter::GetHashString() { return I(L::SuperSeed(*Root())) + ID(); }
// CrewCapacityParameter.cs:43
std::string CrewCapacityParameter::GetTitle() { return Fmt("#autoLOC_282944", {I(targetCapacity)}); }
// CrewCapacityParameter.cs:48
std::string CrewCapacityParameter::GetNotes() {
    if (!Root()->IsFinished() && SpecificVesselSibling() != nullptr && SpecificVesselSibling()->targetVessel != nullptr) {
        const int num = L::ActualCrewCapacity(SpecificVesselSibling()->targetVessel);
        if (num < 0) return "";
        if (num == 0) return Fmt("#autoLOC_282958", {SpecificVesselSibling()->targetVesselName});
        return Fmt("#autoLOC_282960", {SpecificVesselSibling()->targetVesselName, I(num)});
    }
    return "";
}
// CrewCapacityParameter.cs:66
void CrewCapacityParameter::OnRegister() {
    SetDisableOnStateChange(false);
    eventsAdded = false;
    if (Root()->ContractState() == Contract::State::Active) {
        GE::onPartCouple.Add(this, &CrewCapacityParameter::OnDock);
        GE::onVesselWasModified.Add(this, &CrewCapacityParameter::VesselModified);
        GE::onPartDie.Add(this, &CrewCapacityParameter::PartModified);
        eventsAdded = true;
    }
}
// CrewCapacityParameter.cs:79
void CrewCapacityParameter::OnUnregister() {
    if (eventsAdded) {
        GE::onPartCouple.Remove(this, &CrewCapacityParameter::OnDock);
        GE::onVesselWasModified.Remove(this, &CrewCapacityParameter::VesselModified);
        GE::onPartDie.Remove(this, &CrewCapacityParameter::PartModified);
    }
}
// CrewCapacityParameter.cs:89
void CrewCapacityParameter::OnReset() {
    SetIncomplete();
    dirtyVessel = true;
}
// CrewCapacityParameter.cs:95
void CrewCapacityParameter::VesselModified(Vessel* v) {
    if (v == FlightRegistry::ActiveVessel) dirtyVessel = true;
}
// CrewCapacityParameter.cs:103
void CrewCapacityParameter::PartModified(Part* p) {
    if (p->vessel == FlightRegistry::ActiveVessel) dirtyVessel = true;
}
// CrewCapacityParameter.cs:111
void CrewCapacityParameter::OnDock(GE::FromToAction<Part*, Part*> action) {
    if (action.from->vessel == FlightRegistry::ActiveVessel || action.to->vessel == FlightRegistry::ActiveVessel) dirtyVessel = true;
}
// CrewCapacityParameter.cs:119
void CrewCapacityParameter::OnSave(ConfigNode& node) { node.AddValue("targetCapacity", static_cast<int32_t>(targetCapacity)); }
// CrewCapacityParameter.cs:124
void CrewCapacityParameter::OnLoad(ConfigNode& node) { L::LoadNode(node, "CrewCapacityParameter", "targetCapacity", targetCapacity, 8); }
// CrewCapacityParameter.cs:129
void CrewCapacityParameter::OnUpdate() {
    if (!L::FlightIsReady(Root()->ContractState(), Contract::State::Active, true)) return;
    Vessel* av = FlightRegistry::ActiveVessel;
    const int num = av->loaded ? static_cast<int>(av->Parts().size()) : static_cast<int>(av->protoVessel->protoPartSnapshots.size());
    if (activePartCount != num) dirtyVessel = true;
    if (dirtyVessel) {
        dirtyVessel = false;
        activePartCount = av->loaded ? static_cast<int>(av->Parts().size()) : static_cast<int>(av->protoVessel->protoPartSnapshots.size());
        validVessel = av->GetCrewCapacity() >= targetCapacity;
    }
    if (State() == ParameterState::Incomplete) {
        if (validVessel)
            successCounter++;
        else
            successCounter = 0;
        if (successCounter >= 5) SetComplete();
    }
    if (State() == ParameterState::Complete && !validVessel) SetIncomplete();
}

// ---- CrewTraitParameter ---------------------------------------------------------------------------------------------------------------

// CrewTraitParameter.cs:28 targetTraitDisplayName
std::string CrewTraitParameter::targetTraitDisplayName() {
    Experience::ExperienceTraitConfig* traitConfig = nullptr;
    if (KerbalRoster::TryGetExperienceTraitConfig(targetTrait, traitConfig)) return traitConfig->Title().value_or("");
    return targetTrait;
}
// CrewTraitParameter.cs:40 SpecificVesselSibling
SpecificVesselParameter* CrewTraitParameter::SpecificVesselSibling() {
    if (specificVesselSibling == nullptr) specificVesselSibling = Root()->GetParameterT<SpecificVesselParameter>();
    return specificVesselSibling;
}
// CrewTraitParameter.cs:48
CrewTraitParameter::CrewTraitParameter() : targetTrait("Scientist"), targetCount(1) {}
// CrewTraitParameter.cs:48
CrewTraitParameter::CrewTraitParameter(const std::string& targetTrait_, int targetCount_, const std::string& vesselDescription_)
    : vesselDescription(vesselDescription_), targetTrait(targetTrait_), targetCount(targetCount_) {}
// CrewTraitParameter.cs:55
std::string CrewTraitParameter::GetHashString() { return I(L::SuperSeed(*Root())) + ID(); }

namespace {
// string.ToLower() (culture sensitive; the trait titles are plain English)
std::string Lower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}
}  // namespace

// CrewTraitParameter.cs:60
std::string CrewTraitParameter::GetTitle() {
    if (targetCount <= 1) return Fmt("#autoLOC_6001095", {Lower(targetTraitDisplayName()), vesselDescription});
    return Fmt("#autoLOC_283108", {I(targetCount), Lower(targetTraitDisplayName()), vesselDescription});
}
// CrewTraitParameter.cs:69
std::string CrewTraitParameter::GetNotes() {
    if (!Root()->IsFinished() && SpecificVesselSibling() != nullptr && SpecificVesselSibling()->targetVessel != nullptr) {
        const int num = L::VesselCrewWithTraitCount(targetTrait, SpecificVesselSibling()->targetVessel);
        if (num < 0) return "";
        if (num == 0) return Fmt("#autoLOC_283122", {SpecificVesselSibling()->targetVesselName, Lower(targetTraitDisplayName())});
        return Fmt("#autoLOC_283124", {SpecificVesselSibling()->targetVesselName, I(num), Lower(targetTraitDisplayName())});
    }
    return "";
}
// CrewTraitParameter.cs:87
void CrewTraitParameter::OnRegister() {
    SetDisableOnStateChange(false);
    eventsAdded = false;
    if (Root()->ContractState() == Contract::State::Active) {
        GE::onPartCouple.Add(this, &CrewTraitParameter::OnDock);
        GE::onVesselWasModified.Add(this, &CrewTraitParameter::VesselModified);
        GE::onVesselCrewWasModified.Add(this, &CrewTraitParameter::CrewModified);
        GE::onPartDie.Add(this, &CrewTraitParameter::PartModified);
        eventsAdded = true;
    }
}
// CrewTraitParameter.cs:101
void CrewTraitParameter::OnUnregister() {
    if (eventsAdded) {
        GE::onPartCouple.Remove(this, &CrewTraitParameter::OnDock);
        GE::onVesselWasModified.Remove(this, &CrewTraitParameter::VesselModified);
        GE::onVesselCrewWasModified.Remove(this, &CrewTraitParameter::CrewModified);
        GE::onPartDie.Remove(this, &CrewTraitParameter::PartModified);
    }
}
// CrewTraitParameter.cs:112
void CrewTraitParameter::OnReset() {
    SetIncomplete();
    dirtyVessel = true;
}
// CrewTraitParameter.cs:118
void CrewTraitParameter::VesselModified(Vessel* v) {
    if (v == FlightRegistry::ActiveVessel) dirtyVessel = true;
}
// CrewTraitParameter.cs:126
void CrewTraitParameter::PartModified(Part* p) {
    if (p->vessel == FlightRegistry::ActiveVessel) dirtyVessel = true;
}
// CrewTraitParameter.cs:134
void CrewTraitParameter::OnDock(GE::FromToAction<Part*, Part*> action) {
    if (action.from->vessel == FlightRegistry::ActiveVessel || action.to->vessel == FlightRegistry::ActiveVessel) dirtyVessel = true;
}
// CrewTraitParameter.cs:142
void CrewTraitParameter::CrewModified(Vessel* v) {
    if (v != nullptr && v == FlightRegistry::ActiveVessel) dirtyVessel = true;
}
// CrewTraitParameter.cs:150
void CrewTraitParameter::OnSave(ConfigNode& node) {
    node.AddValue("targetTrait", targetTrait);
    node.AddValue("targetCount", static_cast<int32_t>(targetCount));
    node.AddValue("vesselDescription", vesselDescription);
}
// CrewTraitParameter.cs:157
void CrewTraitParameter::OnLoad(ConfigNode& node) {
    L::LoadNode(node, "CrewTraitParameter", "targetTrait", targetTrait, std::string("Scientist"));
    L::LoadNode(node, "CrewTraitParameter", "targetCount", targetCount, 1);
    L::LoadNode(node, "CrewTraitParameter", "vesselDescription", vesselDescription, std::string("vessel"));
}
// CrewTraitParameter.cs:164
void CrewTraitParameter::OnUpdate() {
    if (!L::FlightIsReady(Root()->ContractState(), Contract::State::Active, true)) return;
    Vessel* av = FlightRegistry::ActiveVessel;
    const int num = av->loaded ? static_cast<int>(av->Parts().size()) : static_cast<int>(av->protoVessel->protoPartSnapshots.size());
    if (activePartCount != num) dirtyVessel = true;
    if (dirtyVessel) {
        dirtyVessel = false;
        activePartCount = av->loaded ? static_cast<int>(av->Parts().size()) : static_cast<int>(av->protoVessel->protoPartSnapshots.size());
        validVessel = L::VesselCrewWithTraitCount(targetTrait, av) >= targetCount;
    }
    if (State() == ParameterState::Incomplete) {
        if (validVessel)
            successCounter++;
        else
            successCounter = 0;
        if (successCounter >= 5) SetComplete();
    }
    if (State() == ParameterState::Complete && !validVessel) SetIncomplete();
}

// ---- MobileBaseParameter --------------------------------------------------------------------------------------------------------------

// MobileBaseParameter.cs:20
MobileBaseParameter::MobileBaseParameter() : successCounter(0) {}
// MobileBaseParameter.cs:25
std::string MobileBaseParameter::GetHashString() { return I(L::SuperSeed(*Root())) + ID(); }
// MobileBaseParameter.cs:30
std::string MobileBaseParameter::GetTitle() { return Fmt("#autoLOC_7001018"); }
// MobileBaseParameter.cs:35
void MobileBaseParameter::OnRegister() {
    SetDisableOnStateChange(false);
    if (Root()->ContractState() == Contract::State::Active) {
        GE::onPartCouple.Add(this, &MobileBaseParameter::OnDock);
        GE::onVesselWasModified.Add(this, &MobileBaseParameter::VesselModified);
        GE::onPartDie.Add(this, &MobileBaseParameter::PartModified);
        GE::onVesselSituationChange.Add(this, &MobileBaseParameter::ChangeSituation);
        eventsAdded = true;
    }
}
// MobileBaseParameter.cs:48
void MobileBaseParameter::OnUnregister() {
    if (eventsAdded) {
        GE::onPartCouple.Remove(this, &MobileBaseParameter::OnDock);
        GE::onVesselWasModified.Remove(this, &MobileBaseParameter::VesselModified);
        GE::onPartDie.Remove(this, &MobileBaseParameter::PartModified);
        GE::onVesselSituationChange.Remove(this, &MobileBaseParameter::ChangeSituation);
    }
}
// MobileBaseParameter.cs:59
void MobileBaseParameter::OnReset() {
    SetIncomplete();
    dirtyVessel = true;
}
// MobileBaseParameter.cs:65
void MobileBaseParameter::VesselModified(Vessel* v) {
    if (v == FlightRegistry::ActiveVessel) dirtyVessel = true;
}
// MobileBaseParameter.cs:73
void MobileBaseParameter::PartModified(Part* p) {
    if (p->vessel == FlightRegistry::ActiveVessel) dirtyVessel = true;
}
// MobileBaseParameter.cs:81
void MobileBaseParameter::OnDock(GE::FromToAction<Part*, Part*> action) {
    if (action.from->vessel == FlightRegistry::ActiveVessel || action.to->vessel == FlightRegistry::ActiveVessel) dirtyVessel = true;
}
// MobileBaseParameter.cs:89
void MobileBaseParameter::ChangeSituation(GE::HostedFromToAction<Vessel*, Vessel::Situations> action) {
    if (action.host == FlightRegistry::ActiveVessel) dirtyVessel = true;
}
// MobileBaseParameter.cs:97
void MobileBaseParameter::OnUpdate() {
    if (!L::FlightIsReady(Root()->ContractState(), Contract::State::Active, true)) return;
    Vessel* av = FlightRegistry::ActiveVessel;
    const int num = av->loaded ? static_cast<int>(av->Parts().size()) : static_cast<int>(av->protoVessel->protoPartSnapshots.size());
    if (activePartCount != num) dirtyVessel = true;
    if (dirtyVessel) {
        dirtyVessel = false;
        activePartCount = av->loaded ? static_cast<int>(av->Parts().size()) : static_cast<int>(av->protoVessel->protoPartSnapshots.size());
        validVessel = L::VesselHasWheelsOnGround(av, {L::WheelType::MOTORIZED});
    }
    if (State() == ParameterState::Incomplete) {
        if (validVessel)
            successCounter++;
        else
            successCounter = 0;
        if (successCounter >= 5) SetComplete();
    }
    if (State() == ParameterState::Complete && !validVessel) SetIncomplete();
}

// ---- LocationAndSituationParameter ----------------------------------------------------------------------------------------------------

// LocationAndSituationParameter.cs:27
LocationAndSituationParameter::LocationAndSituationParameter()
    : targetBody(nullptr), targetSituation(Vessel::Situations::ESCAPING), noun("potato"), finalObjective(false), successCounter(0) {}
// LocationAndSituationParameter.cs:36
LocationAndSituationParameter::LocationAndSituationParameter(CelestialBody* targetBody_, Vessel::Situations targetSituation_, const std::string& noun_,
                                                             bool finalObjective_)
    : targetBody(targetBody_), targetSituation(targetSituation_), noun(noun_), finalObjective(finalObjective_), successCounter(0) {}
// LocationAndSituationParameter.cs:45
void LocationAndSituationParameter::OnRegister() {
    SetDisableOnStateChange(false);
    if (Root()->ContractState() == Contract::State::Active) {
        GE::onVesselSOIChanged.Add(this, &LocationAndSituationParameter::ChangeBody);
        GE::onVesselSituationChange.Add(this, &LocationAndSituationParameter::ChangeSituation);
        eventsAdded = true;
    }
}
// LocationAndSituationParameter.cs:56
void LocationAndSituationParameter::OnUnregister() {
    if (eventsAdded) {
        GE::onVesselSOIChanged.Remove(this, &LocationAndSituationParameter::ChangeBody);
        GE::onVesselSituationChange.Remove(this, &LocationAndSituationParameter::ChangeSituation);
    }
}
// LocationAndSituationParameter.cs:65
void LocationAndSituationParameter::OnReset() {
    SetIncomplete();
    dirtyVessel = true;
}
// LocationAndSituationParameter.cs:71
void LocationAndSituationParameter::ChangeBody(GE::HostedFromToAction<Vessel*, CelestialBody*> action) {
    if (action.host == FlightRegistry::ActiveVessel) dirtyVessel = true;
}
// LocationAndSituationParameter.cs:79
void LocationAndSituationParameter::ChangeSituation(GE::HostedFromToAction<Vessel*, Vessel::Situations> action) {
    if (action.host == FlightRegistry::ActiveVessel) dirtyVessel = true;
}
// LocationAndSituationParameter.cs:87
std::string LocationAndSituationParameter::GetHashString() { return I(L::SuperSeed(*Root())) + ID(); }
// LocationAndSituationParameter.cs:92
std::string LocationAndSituationParameter::GetTitle() {
    if (targetBody == nullptr) return Fmt("#autoLOC_283645", {Planetarium::fetch->Sun->displayName()});
    const std::string flag = I(!finalObjective ? 1 : 0);
    const std::string bn = targetBody->displayName();
    switch (targetSituation) {
        case Vessel::Situations::SUB_ORBITAL: return Fmt("#autoLOC_7000017", {flag, noun, bn});
        case Vessel::Situations::FLYING: return Fmt("#autoLOC_7000012", {flag, noun, bn});
        case Vessel::Situations::LANDED: return Fmt("#autoLOC_7000013", {flag, noun, bn});
        case Vessel::Situations::SPLASHED: return Fmt("#autoLOC_7000016", {flag, noun, bn});
        case Vessel::Situations::PRELAUNCH: return Fmt("#autoLOC_7000015", {flag, noun, bn});
        case Vessel::Situations::DOCKED: return Fmt("#autoLOC_7000010", {flag, noun, bn});
        default: return Fmt("#autoLOC_7000018", {flag, noun, bn});
        case Vessel::Situations::ESCAPING: return Fmt("#autoLOC_7000011", {flag, noun, bn});
        case Vessel::Situations::ORBITING: return Fmt("#autoLOC_7000014", {flag, noun, bn});
    }
}
// LocationAndSituationParameter.cs:121
void LocationAndSituationParameter::OnSave(ConfigNode& node) {
    node.AddValue("targetBody", static_cast<int32_t>(targetBody->flightGlobalsIndex));
    node.AddValue("targetSituation", dotnet::EnumToString(targetSituation));
    node.AddValue("noun", noun);
    node.AddValue("finalObjective", finalObjective);
}
// LocationAndSituationParameter.cs:129
void LocationAndSituationParameter::OnLoad(ConfigNode& node) {
    L::LoadNode(node, "LocationAndSituationParameter", "targetBody", targetBody, Planetarium::fetch->Home);
    L::LoadNode(node, "LocationAndSituationParameter", "targetSituation", targetSituation, Vessel::Situations::ORBITING);
    L::LoadNode(node, "LocationAndSituationParameter", "noun", noun, std::string("potato"));
    L::LoadNode(node, "LocationAndSituationParameter", "finalObjective", finalObjective, false);
}
// LocationAndSituationParameter.cs:137
void LocationAndSituationParameter::OnUpdate() {
    if (!L::FlightIsReady(Root()->ContractState(), Contract::State::Active, true)) return;
    if (dirtyVessel) {
        dirtyVessel = false;
        Vessel* av = FlightRegistry::ActiveVessel;
        validVessel = av->situation == targetSituation && av->mainBody() == targetBody && av->vesselType > VesselType::Unknown &&
                      av->vesselType != VesselType::EVA && av->vesselType != VesselType::Flag && av->vesselType != VesselType::DeployedSciencePart &&
                      av->vesselType != VesselType::DeployedScienceController;
    }
    if (State() == ParameterState::Incomplete) {
        if (validVessel)
            successCounter++;
        else
            successCounter = 0;
        if (successCounter >= 5) SetComplete();
    }
    if (State() == ParameterState::Complete && !validVessel) SetIncomplete();
}

// ---- VesselSystemsParameter -----------------------------------------------------------------------------------------------------------

// VesselSystemsParameter.cs:33 mannedString
std::string VesselSystemsParameter::mannedString() {
    switch (mannedStatus) {
        default: return "";
        case MannedStatus::UNMANNED: return Fmt("#autoLOC_286382");
        case MannedStatus::MANNED: return Fmt("#autoLOC_286380");
    }
}
// VesselSystemsParameter.cs:49
VesselSystemsParameter::VesselSystemsParameter() : requireNew(false), vesselDescription("potato") {}
// VesselSystemsParameter.cs:57
VesselSystemsParameter::VesselSystemsParameter(const std::vector<std::string>& checkModuleTypes_, const std::vector<std::string>& checkModuleDescriptions_,
                                               const std::string& vesselDescription_, MannedStatus mannedStatus_, bool requireNew_)
    : requireNew(requireNew_), checkModuleTypes(checkModuleTypes_), checkModuleDescriptions(checkModuleDescriptions_), mannedStatus(mannedStatus_),
      vesselDescription(vesselDescription_) {}
// VesselSystemsParameter.cs:66
std::string VesselSystemsParameter::GetHashString() { return I(L::SuperSeed(*Root())) + ID(); }
// VesselSystemsParameter.cs:71
std::string VesselSystemsParameter::GetTitle() {
    if (!checkModuleDescriptions.empty()) {
        if (requireNew)
            return Fmt("#autoLOC_7001085", {mannedString() == "" ? std::string() : mannedString(), vesselDescription, L::StringUtilities::ThisThisAndThat(checkModuleDescriptions)}) +
                   "\n" + Fmt("#autoLOC_6002645", {vesselDescription});
        return Fmt("#autoLOC_7001086", {vesselDescription, L::StringUtilities::ThisThisAndThat(checkModuleDescriptions)});
    }
    if (requireNew) return Fmt("#autoLOC_286401", {mannedString(), vesselDescription}) + "\n" + Fmt("#autoLOC_6002645", {vesselDescription});
    if (mannedStatus != MannedStatus::ANY) return Fmt("#autoLOC_286404", {vesselDescription, mannedString()});
    return Fmt("#autoLOC_286407", {vesselDescription});
}
// VesselSystemsParameter.cs:92
std::string VesselSystemsParameter::GetNotes() {
    if (requireNew) return Fmt("#autoLOC_7000009", {mannedString(), vesselDescription, Root()->Agent()->Title()});
    return "";
}
// VesselSystemsParameter.cs:101
void VesselSystemsParameter::OnRegister() {
    SetDisableOnStateChange(false);
    eventsAdded = false;
    if (Root()->ContractState() == Contract::State::Active) {
        GE::onPartCouple.Add(this, &VesselSystemsParameter::OnDock);
        GE::onVesselWasModified.Add(this, &VesselSystemsParameter::VesselModified);
        GE::onPartDie.Add(this, &VesselSystemsParameter::PartModified);
        eventsAdded = true;
    }
}
// VesselSystemsParameter.cs:114
void VesselSystemsParameter::OnUnregister() {
    if (eventsAdded) {
        GE::onPartCouple.Remove(this, &VesselSystemsParameter::OnDock);
        GE::onVesselWasModified.Remove(this, &VesselSystemsParameter::VesselModified);
        GE::onPartDie.Remove(this, &VesselSystemsParameter::PartModified);
    }
}
// VesselSystemsParameter.cs:124
void VesselSystemsParameter::OnReset() {
    SetIncomplete();
    dirtyVessel = true;
}
// VesselSystemsParameter.cs:130
void VesselSystemsParameter::VesselModified(Vessel* v) {
    if (v == FlightRegistry::ActiveVessel) dirtyVessel = true;
}
// VesselSystemsParameter.cs:138
void VesselSystemsParameter::PartModified(Part* p) {
    if (p->vessel == FlightRegistry::ActiveVessel) dirtyVessel = true;
}
// VesselSystemsParameter.cs:146
void VesselSystemsParameter::OnDock(GE::FromToAction<Part*, Part*> action) {
    if (action.from->vessel == FlightRegistry::ActiveVessel || action.to->vessel == FlightRegistry::ActiveVessel) dirtyVessel = true;
}
// VesselSystemsParameter.cs:154
void VesselSystemsParameter::OnSave(ConfigNode& node) {
    node.AddValue("typeString", vesselDescription);
    node.AddValue("mannedStatus", static_cast<int32_t>(mannedStatus));
    node.AddValue("requireNew", requireNew);
    L::SystemUtilities::SaveNodeList(&node, "checkModuleTypes", checkModuleTypes);
    L::SystemUtilities::SaveNodeList(&node, "checkModuleDescriptions", checkModuleDescriptions);
    if (requireNew) node.AddValue("launchID", launchID);
}
// VesselSystemsParameter.cs:167
void VesselSystemsParameter::OnLoad(ConfigNode& node) {
    L::LoadNode(node, "VesselSystemsParameter", "typeString", vesselDescription, std::string("potato"));
    L::LoadNode(node, "VesselSystemsParameter", "mannedStatus", mannedStatus, MannedStatus::ANY);
    L::LoadNode(node, "VesselSystemsParameter", "requireNew", requireNew, false);
    L::SystemUtilities::LoadNodeList(&node, "VesselSystemsParameter", "checkModuleTypes", checkModuleTypes);
    L::SystemUtilities::LoadNodeList(&node, "VesselSystemsParameter", "checkModuleDescriptions", checkModuleDescriptions);
    if (requireNew) L::LoadNode(node, "VesselSystemsParameter", "launchID", launchID, 0u);
}
// VesselSystemsParameter.cs:180
void VesselSystemsParameter::OnUpdate() {
    if (!L::FlightIsReady(Root()->ContractState(), Contract::State::Active, true)) return;
    Vessel* av = FlightRegistry::ActiveVessel;
    const int num = av->loaded ? static_cast<int>(av->Parts().size()) : static_cast<int>(av->protoVessel->protoPartSnapshots.size());
    if (activePartCount != num) dirtyVessel = true;
    if (dirtyVessel) {
        dirtyVessel = false;
        activePartCount = av->loaded ? static_cast<int>(av->Parts().size()) : static_cast<int>(av->protoVessel->protoPartSnapshots.size());
        validVessel = isVesselValid(av);
    }
    if (State() == ParameterState::Incomplete) {
        if (validVessel)
            successCounter++;
        else
            successCounter = 0;
        if (successCounter >= 5) SetComplete();
    }
    if (State() == ParameterState::Complete && !validVessel) SetIncomplete();
}
// VesselSystemsParameter.cs:218
bool VesselSystemsParameter::isVesselValid(Vessel* v) {
    if (v == nullptr) return false;
    if (!v->IsControllable()) return false;
    const int num = v->loaded ? v->GetCrewCount() : static_cast<int>(v->protoVessel->GetVesselCrew().size());
    if (mannedStatus == MannedStatus::UNMANNED && num > 0) return false;
    if (mannedStatus == MannedStatus::MANNED && num < 1) return false;
    if (requireNew && !L::VesselLaunchedAfterID(launchID, v, {"PotatoRoid"})) return false;
    if (!checkModuleTypes.empty() && !L::HasValidContractObjectives(*v, checkModuleTypes)) return false;
    return true;
}

// ---- PartRequestParameter -------------------------------------------------------------------------------------------------------------

// PartRequestParameter.cs:33
PartRequestParameter::PartRequestParameter() : successCounter(0), article("a"), partDescription("part"), vesselDescription("vessel") {
    partNames.push_back("part");
    moduleNames.push_back("module");
}
// PartRequestParameter.cs:46
PartRequestParameter::PartRequestParameter(ConfigNode& requestNode, Vessel* v) : successCounter(0), article("a"), partDescription("part"), vesselDescription("vessel") {
    L::LoadNode(requestNode, "PartRequestParameter", "Article", article, std::string("a"), false);
    L::LoadNode(requestNode, "PartRequestParameter", "PartDescription", partDescription, std::string("part"), false);
    L::LoadNode(requestNode, "PartRequestParameter", "VesselDescription", vesselDescription, std::string("vessel"), false);
    partNames = requestNode.GetValuesList("Part");
    moduleNames = requestNode.GetValuesList("Module");
    if (v != nullptr) ScanVesselForExistingParts(v);
    CheckLists();
}
// PartRequestParameter.cs:67
std::string PartRequestParameter::GetHashString() { return I(L::SuperSeed(*Root())) + ID(); }
// PartRequestParameter.cs:72
std::string PartRequestParameter::GetTitle() {
    if (!existingParts.empty()) return Fmt("#autoLOC_7001087", {partDescription, vesselDescription});
    return Fmt("#autoLOC_7001088", {partDescription, vesselDescription});
}
// PartRequestParameter.cs:81
void PartRequestParameter::OnRegister() {
    SetDisableOnStateChange(false);
    if (Root()->ContractState() == Contract::State::Active) {
        GE::onPartCouple.Add(this, &PartRequestParameter::OnDock);
        GE::onVesselWasModified.Add(this, &PartRequestParameter::VesselModified);
        GE::onPartDie.Add(this, &PartRequestParameter::PartModified);
        eventsAdded = true;
    }
}
// PartRequestParameter.cs:93
void PartRequestParameter::OnUnregister() {
    if (eventsAdded) {
        GE::onPartCouple.Remove(this, &PartRequestParameter::OnDock);
        GE::onVesselWasModified.Remove(this, &PartRequestParameter::VesselModified);
        GE::onPartDie.Remove(this, &PartRequestParameter::PartModified);
    }
}
// PartRequestParameter.cs:103
void PartRequestParameter::OnReset() {
    SetIncomplete();
    dirtyVessel = true;
}
// PartRequestParameter.cs:109
void PartRequestParameter::VesselModified(Vessel* v) {
    if (v == FlightRegistry::ActiveVessel) dirtyVessel = true;
}
// PartRequestParameter.cs:117
void PartRequestParameter::PartModified(Part* p) {
    if (p->vessel == FlightRegistry::ActiveVessel) dirtyVessel = true;
}
// PartRequestParameter.cs:125
void PartRequestParameter::OnDock(GE::FromToAction<Part*, Part*> action) {
    if (action.from->vessel == FlightRegistry::ActiveVessel || action.to->vessel == FlightRegistry::ActiveVessel) dirtyVessel = true;
}
// PartRequestParameter.cs:133
void PartRequestParameter::OnSave(ConfigNode& node) {
    node.AddValue("article", article);
    node.AddValue("partDescription", partDescription);
    node.AddValue("vesselDescription", vesselDescription);
    L::SystemUtilities::SaveNodeList(&node, "partNames", partNames);
    L::SystemUtilities::SaveNodeList(&node, "moduleNames", moduleNames);
    L::SystemUtilities::SaveNodeList(&node, "existingParts", existingParts);
}
// PartRequestParameter.cs:143
void PartRequestParameter::OnLoad(ConfigNode& node) {
    L::LoadNode(node, "PartRequestParameter", "article", article, std::string("a"));
    L::LoadNode(node, "PartRequestParameter", "partDescription", partDescription, std::string("part"));
    L::LoadNode(node, "PartRequestParameter", "vesselDescription", vesselDescription, std::string("vessel"));
    L::SystemUtilities::LoadNodeList(&node, "PartRequestParameter", "partNames", partNames);
    L::SystemUtilities::LoadNodeList(&node, "PartRequestParameter", "moduleNames", moduleNames);
    L::SystemUtilities::LoadNodeList(&node, "PartRequestParameter", "existingParts", existingParts);
    CheckLists();
}
// PartRequestParameter.cs:154
void PartRequestParameter::OnUpdate() {
    if (!L::FlightIsReady(Root()->ContractState(), Contract::State::Active, true)) return;
    Vessel* av = FlightRegistry::ActiveVessel;
    const int num = av->loaded ? static_cast<int>(av->Parts().size()) : static_cast<int>(av->protoVessel->protoPartSnapshots.size());
    if (activePartCount != num) dirtyVessel = true;
    if (dirtyVessel) {
        dirtyVessel = false;
        activePartCount = num;
        validVessel = VesselHasPartRequest();
    }
    if (State() == ParameterState::Incomplete) {
        if (validVessel)
            successCounter++;
        else
            successCounter = 0;
        if (successCounter >= 5) SetComplete();
    }
    if (State() == ParameterState::Complete && !validVessel) SetIncomplete();
}
// PartRequestParameter.cs:192
void PartRequestParameter::CheckLists() {
    if (partNames.empty() && moduleNames.empty()) {
        Root()->RemoveParameter(this);
        Unregister();
    }
}

namespace {
std::string DotName(std::string s) {
    for (char& c : s)
        if (c == '_') c = '.';
    return s;
}
bool ListContains(const std::vector<uint32_t>& l, uint32_t v) { return std::find(l.begin(), l.end(), v) != l.end(); }
}  // namespace

// PartRequestParameter.cs:201
bool PartRequestParameter::VesselHasPartRequest(Vessel* v) {
    if (!L::ActiveVesselFallback(v, false)) return false;
    if (v->loaded) {
        int count = static_cast<int>(v->Parts().size());
        while (count-- > 0) {
            Part* part = v->Parts()[static_cast<size_t>(count)];
            if (ListContains(existingParts, part->fields.flightID)) continue;
            int count2 = static_cast<int>(partNames.size());
            while (count2-- > 0)
                if (L::GetPartName(part) == DotName(partNames[static_cast<size_t>(count2)])) return true;
            int count3 = static_cast<int>(part->Modules().Count());
            while (count3-- > 0) {
                PartModule* partModule = part->Modules()[static_cast<size_t>(count3)];
                int count4 = static_cast<int>(moduleNames.size());
                while (count4-- > 0)
                    if (partModule->moduleName == moduleNames[static_cast<size_t>(count4)]) return true;
            }
        }
    } else {
        int count5 = static_cast<int>(v->protoVessel->protoPartSnapshots.size());
        while (count5-- > 0) {
            ProtoPartSnapshot* pps = v->protoVessel->protoPartSnapshots[static_cast<size_t>(count5)].get();
            if (ListContains(existingParts, pps->flightID)) continue;
            int count6 = static_cast<int>(partNames.size());
            while (count6-- > 0)
                if (pps->partName.value_or("") == DotName(partNames[static_cast<size_t>(count6)])) return true;
            int count7 = static_cast<int>(pps->modules.size());
            while (count7-- > 0) {
                ProtoPartModuleSnapshot* pm = pps->modules[static_cast<size_t>(count7)].get();
                int count8 = static_cast<int>(moduleNames.size());
                while (count8-- > 0)
                    if (pm->moduleName.value_or("") == moduleNames[static_cast<size_t>(count8)]) return true;
            }
        }
    }
    return false;
}
// PartRequestParameter.cs:276
void PartRequestParameter::ScanVesselForExistingParts(Vessel* v) {
    if (v->loaded) {
        int count = static_cast<int>(v->Parts().size());
        while (count-- > 0) {
            Part* part = v->Parts()[static_cast<size_t>(count)];
            int count2 = static_cast<int>(partNames.size());
            while (count2-- > 0) {
                if (L::GetPartName(part) == DotName(partNames[static_cast<size_t>(count2)]) && !ListContains(existingParts, part->fields.flightID)) {
                    existingParts.push_back(part->fields.flightID);
                    break;
                }
            }
            int count3 = static_cast<int>(part->Modules().Count());
            while (count3-- > 0 && !ListContains(existingParts, part->fields.flightID)) {
                PartModule* partModule = part->Modules()[static_cast<size_t>(count3)];
                int count4 = static_cast<int>(moduleNames.size());
                while (count4-- > 0) {
                    if (partModule->moduleName == moduleNames[static_cast<size_t>(count4)]) {
                        existingParts.push_back(part->fields.flightID);
                        break;
                    }
                }
            }
        }
        return;
    }
    int count5 = static_cast<int>(v->protoVessel->protoPartSnapshots.size());
    while (count5-- > 0) {
        ProtoPartSnapshot* pps = v->protoVessel->protoPartSnapshots[static_cast<size_t>(count5)].get();
        int count6 = static_cast<int>(partNames.size());
        while (count6-- > 0) {
            if (pps->partName.value_or("") == DotName(partNames[static_cast<size_t>(count6)]) && !ListContains(existingParts, pps->flightID)) {
                existingParts.push_back(pps->flightID);
                break;
            }
        }
        int count7 = static_cast<int>(pps->modules.size());
        while (count7-- > 0 && !ListContains(existingParts, pps->flightID)) {
            ProtoPartModuleSnapshot* pm = pps->modules[static_cast<size_t>(count7)].get();
            int count8 = static_cast<int>(moduleNames.size());
            while (count8-- > 0) {
                if (pm->moduleName.value_or("") == moduleNames[static_cast<size_t>(count8)]) {
                    existingParts.push_back(pps->flightID);
                    break;
                }
            }
        }
    }
}

// ---- ResourcePossessionParameter ------------------------------------------------------------------------------------------------------

// ResourcePossessionParameter.cs:25 SpecificVesselSibling
SpecificVesselParameter* ResourcePossessionParameter::SpecificVesselSibling() {
    if (specificVesselSibling == nullptr) specificVesselSibling = Root()->GetParameterT<SpecificVesselParameter>();
    return specificVesselSibling;
}
// ResourcePossessionParameter.cs:27
ResourcePossessionParameter::ResourcePossessionParameter()
    : successCounter(0), currentResource(0.0), goalResource(1000.0), resourceName("Matter"), resourceTitle("matter"), vesselName("vessel") {}
// ResourcePossessionParameter.cs:37
ResourcePossessionParameter::ResourcePossessionParameter(const std::string& resourceName_, const std::string& resourceTitle_, const std::string& vesselName_,
                                                         double goalResource_)
    : successCounter(0), currentResource(0.0), goalResource(goalResource_), resourceName(resourceName_), resourceTitle(resourceTitle_), vesselName(vesselName_) {}
// ResourcePossessionParameter.cs:47
std::string ResourcePossessionParameter::GetHashString() { return I(L::SuperSeed(*Root())) + ID(); }
// ResourcePossessionParameter.cs:52
std::string ResourcePossessionParameter::GetTitle() { return Fmt("#autoLOC_284677", {RoundedThousands(goalResource), resourceTitle, vesselName}); }
// ResourcePossessionParameter.cs:57
std::string ResourcePossessionParameter::GetNotes() {
    if (!Root()->IsFinished() && SpecificVesselSibling() != nullptr && SpecificVesselSibling()->targetVessel != nullptr) {
        const double num = L::VesselResourceAmount(resourceName, SpecificVesselSibling()->targetVessel);
        if (num < 0.0) return "";
        if (num < 1.0) return Fmt("#autoLOC_284691", {SpecificVesselSibling()->targetVesselName, resourceTitle});
        return Fmt("#autoLOC_284693", {SpecificVesselSibling()->targetVesselName, RoundedThousands(num), resourceTitle});
    }
    return "";
}
// ResourcePossessionParameter.cs:75
void ResourcePossessionParameter::OnRegister() { SetDisableOnStateChange(false); }
// ResourcePossessionParameter.cs:80
void ResourcePossessionParameter::OnReset() { SetIncomplete(); }
// ResourcePossessionParameter.cs:85
void ResourcePossessionParameter::OnSave(ConfigNode& node) {
    node.AddValue("goalResource", goalResource);
    node.AddValue("resourceName", resourceName);
    node.AddValue("resourceTitle", resourceTitle);
    node.AddValue("vesselName", vesselName);
}
// ResourcePossessionParameter.cs:93
void ResourcePossessionParameter::OnLoad(ConfigNode& node) {
    L::LoadNode(node, "ResourcePossessionParameter", "goalResource", goalResource, 1000.0);
    L::LoadNode(node, "ResourcePossessionParameter", "resourceName", resourceName, std::string("Matter"));
    L::LoadNode(node, "ResourcePossessionParameter", "resourceTitle", resourceTitle, std::string("matter"));
    L::LoadNode(node, "ResourcePossessionParameter", "vesselName", vesselName, std::string("vessel"));
}
// ResourcePossessionParameter.cs:101
void ResourcePossessionParameter::OnUpdate() {
    if (!L::FlightIsReady(Root()->ContractState(), Contract::State::Active, true)) return;
    currentResource = L::VesselResourceAmount(resourceName);
    const bool flag = dotnet::Math::Ceiling(currentResource) >= goalResource;
    if (State() == ParameterState::Incomplete) {
        if (flag)
            successCounter++;
        else
            successCounter = 0;
        if (successCounter >= 5) SetComplete();
    }
    if (State() == ParameterState::Complete && !flag) SetIncomplete();
}

// ---- ResourceExtractionParameter ------------------------------------------------------------------------------------------------------

// ResourceExtractionParameter.cs:30
ResourceExtractionParameter::ResourceExtractionParameter()
    : targetBody(Planetarium::fetch->Home), successCounter(0), totalHarvested(0.0), goalHarvested(1000.0), resourceName("Matter"), resourceTitle("matter"),
      eventsAdded(false), notifyLevel(0.25f) {}
// ResourceExtractionParameter.cs:43
ResourceExtractionParameter::ResourceExtractionParameter(const std::string& resourceName_, const std::string& resourceTitle_, double goalHarvested_,
                                                         CelestialBody* targetBody_, const std::vector<std::string>& modules_)
    : targetBody(targetBody_), successCounter(0), totalHarvested(0.0), goalHarvested(goalHarvested_), resourceName(resourceName_), resourceTitle(resourceTitle_),
      modules(modules_), eventsAdded(false), notifyLevel(0.25f) {}
// ResourceExtractionParameter.cs:56
std::string ResourceExtractionParameter::GetHashString() { return I(L::SuperSeed(*Root())) + ID(); }
// ResourceExtractionParameter.cs:61
std::string ResourceExtractionParameter::GetTitle() { return Fmt("#autoLOC_284531", {RoundedThousands(goalHarvested), resourceTitle, targetBody->displayName()}); }
// ResourceExtractionParameter.cs:66
void ResourceExtractionParameter::OnRegister() {
    eventsAdded = false;
    if (Root()->ContractState() == Contract::State::Active) {
        GE::OnResourceConverterOutput.Add(this, &ResourceExtractionParameter::ResourcesHarvested);
        eventsAdded = true;
    }
}
// ResourceExtractionParameter.cs:76
void ResourceExtractionParameter::OnUnregister() {
    if (eventsAdded) GE::OnResourceConverterOutput.Remove(this, &ResourceExtractionParameter::ResourcesHarvested);
}
// ResourceExtractionParameter.cs:84
void ResourceExtractionParameter::OnReset() { SetIncomplete(); }
// ResourceExtractionParameter.cs:89
void ResourceExtractionParameter::ResourcesHarvested(PartModule* module, const std::string& resource, double amount) {
    if (!(resource != resourceName) && amount > 0.0 && module != nullptr && module->part != nullptr) {
        Vessel* mv = static_cast<Part*>(module->part)->vessel;
        if (mv != nullptr && mv->mainBody() != nullptr &&
            std::find(modules.begin(), modules.end(), module->moduleName) != modules.end() && !(mv->mainBody() != targetBody)) {
            totalHarvested += amount;
            if (totalHarvested / goalHarvested >= static_cast<double>(notifyLevel)) {
                std::string name = Root()->Agent()->Name();
                name += "^M";
                const std::string text = Localizer::Format("<<g:2,1>>", {name, resourceTitle});
                L::FinePrintEnvironment::Post(
                    Fmt("#autoLOC_284574", {dotnet::FormatDoubleG(dotnet::Math::Round(f32store(fx(notifyLevel) * 100.0))), text}), 5.0f, L::ScreenMessageStyle::UPPER_LEFT);
                notifyLevel = f32store(fx(notifyLevel) + 0.25);
            }
        }
    }
}
// ResourceExtractionParameter.cs:105
void ResourceExtractionParameter::OnSave(ConfigNode& node) {
    node.AddValue("targetBody", static_cast<int32_t>(targetBody->flightGlobalsIndex));
    node.AddValue("totalHarvested", totalHarvested);
    node.AddValue("goalHarvested", goalHarvested);
    node.AddValue("resourceName", resourceName);
    node.AddValue("resourceTitle", resourceTitle);
    node.AddValue("notifyLevel", notifyLevel);
    L::SystemUtilities::SaveNodeList(&node, "modules", modules);
}
// ResourceExtractionParameter.cs:116
void ResourceExtractionParameter::OnLoad(ConfigNode& node) {
    L::LoadNode(node, "ResourceExtractionParameter", "targetBody", targetBody, Planetarium::fetch->Home);
    L::LoadNode(node, "ResourceExtractionParameter", "totalHarvested", totalHarvested, 0.0);
    L::LoadNode(node, "ResourceExtractionParameter", "goalHarvested", goalHarvested, 1000.0);
    L::LoadNode(node, "ResourceExtractionParameter", "resourceName", resourceName, std::string("Matter"));
    L::LoadNode(node, "ResourceExtractionParameter", "resourceTitle", resourceTitle, std::string("matter"));
    L::LoadNode(node, "ResourceExtractionParameter", "notifyLevel", notifyLevel, 0.25f);
    L::SystemUtilities::LoadNodeList(&node, "ResourceExtractionParameter", "modules", modules);
}
// ResourceExtractionParameter.cs:127
void ResourceExtractionParameter::OnUpdate() {
    if (!L::FlightIsReady(Root()->ContractState(), Contract::State::Active)) return;
    const bool flag = dotnet::Math::Ceiling(totalHarvested) >= goalHarvested;
    if (State() == ParameterState::Incomplete) {
        if (flag)
            successCounter++;
        else
            successCounter = 0;
        if (successCounter >= 5) SetComplete();
    }
    if (State() == ParameterState::Complete && !flag) SetIncomplete();
}

}  // namespace ksp::Contracts::Parameters
