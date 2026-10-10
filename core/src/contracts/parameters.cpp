// WP-37: the stock contract parameters (Assembly-CSharp/Contracts/Parameters/*.cs).
#include "ksp/contracts/parameters.hpp"

#include <algorithm>

#include "ksp/career/format.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/contracts/contract_system.hpp"
#include "ksp/contracts/fineprint_lite.hpp"
#include "ksp/contracts/module_test_subject.hpp"
#include "ksp/contracts/templates.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/parts/available_part.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/save/proto_vessel.hpp"
#include "ksp/science/science.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp::Contracts::Parameters {

using GE = events::GameEvents;
using KSP::Localization::Localizer;

namespace {
CelestialBody* BodyAt(int index) { return FlightRegistry::Bodies().at(static_cast<size_t>(index)); }
int BodyIndexOf(CelestialBody* b) {
    const auto& bodies = FlightRegistry::Bodies();
    for (size_t i = 0; i < bodies.size(); ++i)
        if (bodies[i] == b) return static_cast<int>(i);
    return -1;
}
bool ParseBool(const std::string& s) {
    bool b = false;
    if (!dotnet::TryParseBool(s, b)) throw dotnet::FormatException("String was not recognized as a valid Boolean.");
    return b;
}
std::string N0(double d) { return career::FormatNumber(d, "N0"); }
std::string N1(double d) { return career::FormatNumber(d, "N1"); }
template <class T>
bool Contains(const std::vector<T>& v, const T& x) {
    return std::find(v.begin(), v.end(), x) != v.end();
}
template <class T>
void RemoveFirst(std::vector<T>& v, const T& x) {
    auto it = std::find(v.begin(), v.end(), x);
    if (it != v.end()) v.erase(it);
}
Part* PartOf(ModuleTestSubject* m) { return dynamic_cast<Part*>(m->part); }
std::string Fmt1(const char* tag, const std::string& a) { return Localizer::Format(tag, {a}); }
}  // namespace

// ---- body parameters ---------------------------------------------------------------------------------------------------

// EnterOrbit.cs:21 (the hash / load / save are shared by the four classes with the identical C#)
std::string BodyParameterBase::GetHashString() { return targetBody->name(); }
// EnterOrbit.cs:31
void BodyParameterBase::OnLoad(ConfigNode& node) {
    if (node.HasValue("body")) targetBody = BodyAt(dotnet::ParseInt32(*node.GetValue("body")));
}
// EnterOrbit.cs:39
void BodyParameterBase::OnSave(ConfigNode& node) { node.AddValue("body", static_cast<int32_t>(targetBody->flightGlobalsIndex)); }

// EnterOrbit.cs:27
std::string EnterOrbit::GetTitle() { return Fmt1("#autoLOC_269557", targetBody->displayName()); }
// EnterOrbit.cs:45
void EnterOrbit::OnRegister() { GE::VesselSituation::onOrbit.Add(this, &EnterOrbit::OnEnterOrbit); }
// EnterOrbit.cs:50
void EnterOrbit::OnUnregister() { GE::VesselSituation::onOrbit.Remove(this, &EnterOrbit::OnEnterOrbit); }
// EnterOrbit.cs:55
void EnterOrbit::OnEnterOrbit(Vessel*, CelestialBody* body) {
    if (body == targetBody) SetComplete();
}

// EnterSOI.cs:27
std::string EnterSOI::GetTitle() { return Fmt1("#autoLOC_269618", targetBody->displayName()); }
// EnterSOI.cs:45
void EnterSOI::OnRegister() { GE::VesselSituation::onFlyBy.Add(this, &EnterSOI::OnEnterSOI); }
// EnterSOI.cs:50
void EnterSOI::OnUnregister() { GE::VesselSituation::onFlyBy.Remove(this, &EnterSOI::OnEnterSOI); }
// EnterSOI.cs:55
void EnterSOI::OnEnterSOI(Vessel*, CelestialBody* body) {
    if (body == targetBody) SetComplete();
}

// LandOnBody.cs:27
std::string LandOnBody::GetTitle() { return Fmt1("#autoLOC_269752", targetBody->displayName()); }
// LandOnBody.cs:45
void LandOnBody::OnRegister() { GE::VesselSituation::onLand.Add(this, &LandOnBody::OnVesselLand); }
// LandOnBody.cs:50
void LandOnBody::OnUnregister() { GE::VesselSituation::onLand.Remove(this, &LandOnBody::OnVesselLand); }
// LandOnBody.cs:55
void LandOnBody::OnVesselLand(Vessel*, CelestialBody* body) {
    if (body == targetBody) SetComplete();
}

// Parameters/PlantFlag.cs:27
std::string PlantFlag::GetTitle() { return Fmt1("#autoLOC_270135", targetBody->displayName()); }
// Parameters/PlantFlag.cs:45
void PlantFlag::OnRegister() { GE::onFlagPlant.Add(this, &PlantFlag::OnPlantFlag); }
// Parameters/PlantFlag.cs:50
void PlantFlag::OnUnregister() { GE::onFlagPlant.Remove(this, &PlantFlag::OnPlantFlag); }
// Parameters/PlantFlag.cs:55
void PlantFlag::OnPlantFlag(Vessel* vessel) {
    if (vessel->orbit().referenceBody == targetBody) SetComplete();
}

// ---- OR / XOR -----------------------------------------------------------------------------------------------------------

// OR.cs:13
void OR::OnParameterStateChange(ContractParameter*) {
    int num = 0;
    while (true) {
        if (num < ParameterCount()) {
            if (GetParameter(num)->State() == ParameterState::Complete) break;
            ++num;
            continue;
        }
        return;
    }
    SetComplete();
}

// XOR.cs:13
void XOR::OnParameterStateChange(ContractParameter*) {
    int num = 0;
    int num2 = 0;
    while (true) {
        if (num2 < ParameterCount()) {
            if (GetParameter(num2)->State() == ParameterState::Complete) {
                if (num > 0) break;
                ++num;
            }
            ++num2;
            continue;
        }
        if (num == 1) SetComplete();
        return;
    }
    SetIncomplete();
}

// ---- KerbalDeaths -------------------------------------------------------------------------------------------------------

// KerbalDeaths.cs:23
KerbalDeaths::KerbalDeaths(int countMax_) {
    state = ParameterState::Complete;
    countMax = countMax_;
    countCurrent = 0;
}

// KerbalDeaths.cs:35
std::string KerbalDeaths::GetTitle() {
    if (countMax > 1) return Fmt1("#autoLOC_269684", dotnet::FormatInt64(countMax));
    return Localizer::Format("#autoLOC_269686");
}

// KerbalDeaths.cs:42
void KerbalDeaths::OnLoad(ConfigNode& node) {
    if (node.HasValue("count")) countCurrent = dotnet::ParseInt32(*node.GetValue("count"));
    if (node.HasValue("countMax")) countMax = dotnet::ParseInt32(*node.GetValue("countMax"));
}

// KerbalDeaths.cs:54
void KerbalDeaths::OnSave(ConfigNode& node) {
    node.AddValue("count", static_cast<int32_t>(countCurrent));
    node.AddValue("countMax", static_cast<int32_t>(countMax));
}

// KerbalDeaths.cs:60
void KerbalDeaths::OnRegister() { GE::onCrewKilled.Add(this, &KerbalDeaths::OnCrewKilled); }
// KerbalDeaths.cs:65
void KerbalDeaths::OnUnregister() { GE::onCrewKilled.Remove(this, &KerbalDeaths::OnCrewKilled); }

// KerbalDeaths.cs:70
void KerbalDeaths::OnCrewKilled(EventReport* report) {
    if (report->eventType == FlightEvents::CREW_KILLED) {
        ++countCurrent;
        if (countCurrent == countMax) SetFailed();
    }
}

// ---- CollectScience -----------------------------------------------------------------------------------------------------

// Parameters/CollectScience.cs:23
CollectScience::CollectScience(CelestialBody* body, BodyLocation location) : targetBody(body), targetLocation(location) {}

// Parameters/CollectScience.cs:27
std::string CollectScience::GetHashString() { return dotnet::EnumToString(targetLocation) + " " + targetBody->name(); }

// Parameters/CollectScience.cs:32
std::string CollectScience::GetTitle() {
    if (targetLocation == BodyLocation::Space) return Fmt1("#autoLOC_269461", targetBody->displayName());
    return Fmt1("#autoLOC_269463", targetBody->displayName());
}

// Parameters/CollectScience.cs:41
void CollectScience::OnLoad(ConfigNode& node) {
    if (node.HasValue("body")) targetBody = BodyAt(dotnet::ParseInt32(*node.GetValue("body")));
    if (node.HasValue("location")) targetLocation = dotnet::EnumParseFull<BodyLocation>(*node.GetValue("location"));
}

// Parameters/CollectScience.cs:53
void CollectScience::OnSave(ConfigNode& node) {
    node.AddValue("body", static_cast<int32_t>(targetBody->flightGlobalsIndex));
    node.AddValue("location", dotnet::EnumToString(targetLocation));
}

// Parameters/CollectScience.cs:59
void CollectScience::OnRegister() {
    GE::OnScienceRecieved.Add(this, &CollectScience::OnScience);
    GE::OnTriggeredDataTransmission.Add(this, &CollectScience::OnTriggeredScience);
}

// Parameters/CollectScience.cs:65
void CollectScience::OnUnregister() {
    GE::OnScienceRecieved.Remove(this, &CollectScience::OnScience);
    GE::OnTriggeredDataTransmission.Remove(this, &CollectScience::OnTriggeredScience);
}

// Parameters/CollectScience.cs:71
void CollectScience::OnScience(float, ScienceSubject* subject, ProtoVessel*, bool reverseEngineered) {
    if (reverseEngineered) return;
    if (targetLocation == BodyLocation::Space) {
        if (subject->IsFromBody(*targetBody) && (subject->IsFromSituation(ExperimentSituations::InSpaceHigh) || subject->IsFromSituation(ExperimentSituations::InSpaceLow)))
            SetComplete();
    } else if (subject->IsFromBody(*targetBody) && (subject->IsFromSituation(ExperimentSituations::SrfLanded) || subject->IsFromSituation(ExperimentSituations::SrfSplashed))) {
        SetComplete();
    }
}

// Parameters/CollectScience.cs:90
void CollectScience::OnTriggeredScience(ScienceData* data, Vessel* origin, bool xmitAborted) {
    if (data == nullptr || origin == nullptr || origin->mainBody() == nullptr || xmitAborted || data->dataAmount <= 0.0f || origin->mainBody() != targetBody) return;
    if (targetLocation == BodyLocation::Space) {
        if (origin->situation == Vessel::Situations::ORBITING || origin->situation == Vessel::Situations::SUB_ORBITAL) SetComplete();
    } else if (origin->situation == Vessel::Situations::LANDED || origin->situation == Vessel::Situations::SPLASHED) {
        SetComplete();
    }
}

// ---- ReachSituation -----------------------------------------------------------------------------------------------------

// ReachSituation.cs:18
ReachSituation::ReachSituation(Vessel::Situations sit, std::string title_) : Situation(sit), title(std::move(title_)) {}

// ReachSituation.cs:24
std::string ReachSituation::GetTitle() { return title + GetTitleStringShort(Situation); }

// ReachSituation.cs:29
std::string ReachSituation::GetTitleStringShort(Vessel::Situations sit) {
    switch (sit) {
        case Vessel::Situations::FLYING: return Localizer::Format("#autoLOC_268856");
        case Vessel::Situations::LANDED: return Localizer::Format("#autoLOC_268855");
        case Vessel::Situations::SPLASHED: return Localizer::Format("#autoLOC_268858");
        case Vessel::Situations::PRELAUNCH: return Localizer::Format("#autoLOC_268854");
        default: return Localizer::Format("#autoLOC_268861");
        case Vessel::Situations::ESCAPING: return Localizer::Format("#autoLOC_268860");
        case Vessel::Situations::ORBITING: return Localizer::Format("#autoLOC_268857");
        case Vessel::Situations::SUB_ORBITAL: return Localizer::Format("#autoLOC_268859");
    }
}

// ReachSituation.cs:52
void ReachSituation::OnLoad(ConfigNode& node) {
    if (node.HasValue("sit")) Situation = dotnet::EnumParseFull<Vessel::Situations>(*node.GetValue("sit"));
    if (node.HasValue("title")) title = *node.GetValue("title");
}

// ReachSituation.cs:64
void ReachSituation::OnSave(ConfigNode& node) {
    node.AddValue("sit", dotnet::EnumToString(Situation));
    node.AddValue("title", title);
}

// ReachSituation.cs:70
void ReachSituation::OnRegister() { GE::onVesselSituationChange.Add(this, &ReachSituation::OnVesselSituationChange); }
// ReachSituation.cs:75
void ReachSituation::OnUnregister() { GE::onVesselSituationChange.Remove(this, &ReachSituation::OnVesselSituationChange); }

// ReachSituation.cs:80
void ReachSituation::OnReset() {
    if (Lite::FlightIsReady(true))
        TrackVessel(FlightRegistry::ActiveVessel);
    else
        SetIncomplete();
}

// ReachSituation.cs:92
void ReachSituation::OnVesselSituationChange(GE::HostedFromToAction<Vessel*, Vessel::Situations> v) {
    if (v.host == FlightRegistry::ActiveVessel) TrackVessel(v.host);
}

// ReachSituation.cs:100
void ReachSituation::TrackVessel(Vessel* v) {
    if (State() == ParameterState::Incomplete && checkVesselSituation(v))
        SetComplete();
    else if (State() == ParameterState::Complete && !checkVesselSituation(v))
        SetIncomplete();
}

// ReachSituation.cs:112
bool ReachSituation::checkVesselSituation(Vessel* v) {
    if (Situation == Vessel::Situations::LANDED) {
        if (v->situation != Vessel::Situations::LANDED) return v->situation == Vessel::Situations::PRELAUNCH;
        return true;
    }
    if (Situation == Vessel::Situations::PRELAUNCH) {
        if (v->situation != Vessel::Situations::PRELAUNCH) {
            if (v->situation == Vessel::Situations::LANDED) {
                if (v->landedAt.find("Runway") == std::string::npos) return v->landedAt.find("LaunchPad") != std::string::npos;
                return true;
            }
            return false;
        }
        return true;
    }
    return v->situation == Situation;
}

// ---- ReachDestination ---------------------------------------------------------------------------------------------------

// ReachDestination.cs:19
ReachDestination::ReachDestination(CelestialBody* destination, std::string title_) : Destination(destination), title(std::move(title_)) {}
// ReachDestination.cs:26
std::string ReachDestination::GetTitle() { return title + GetTitleStringShort(Destination); }
// ReachDestination.cs:31
std::string ReachDestination::GetTitleStringShort(CelestialBody* dest) { return Fmt1("#autoLOC_7001301", dest->displayName()); }

// ReachDestination.cs:36
void ReachDestination::OnLoad(ConfigNode& node) {
    if (node.HasValue("dest")) Destination = BodyAt(dotnet::ParseInt32(*node.GetValue("dest")));
    if (node.HasValue("title")) title = *node.GetValue("title");
}

// ReachDestination.cs:47
void ReachDestination::OnSave(ConfigNode& node) {
    node.AddValue("dest", static_cast<int32_t>(BodyIndexOf(Destination)));
    node.AddValue("title", title);
}

// ReachDestination.cs:53
void ReachDestination::OnRegister() { GE::onVesselSOIChanged.Add(this, &ReachDestination::OnVesselSOIChange); }
// ReachDestination.cs:58
void ReachDestination::OnUnregister() { GE::onVesselSOIChanged.Remove(this, &ReachDestination::OnVesselSOIChange); }

// ReachDestination.cs:63
void ReachDestination::OnReset() {
    if (Lite::FlightIsReady(true))
        TrackVessel(FlightRegistry::ActiveVessel);
    else
        SetIncomplete();
}

// ReachDestination.cs:74
void ReachDestination::OnVesselSOIChange(GE::HostedFromToAction<Vessel*, CelestialBody*> v) {
    if (v.host == FlightRegistry::ActiveVessel) TrackVessel(v.host);
}

// ReachDestination.cs:81
void ReachDestination::TrackVessel(Vessel* v) {
    if (State() == ParameterState::Incomplete && checkVesselDestination(v))
        SetComplete();
    else if (State() == ParameterState::Complete && !checkVesselDestination(v))
        SetIncomplete();
}

// ReachDestination.cs:88
bool ReachDestination::checkVesselDestination(Vessel* v) { return v->mainBody() == Destination; }

// ---- ReachBiome ----------------------------------------------------------------------------------------------------------

// ReachBiome.cs:22
ReachBiome::ReachBiome(const std::string* biomeName, std::string title_) : BiomeName(biomeName != nullptr ? *biomeName : std::string()), title(std::move(title_)) {}
// ReachBiome.cs:26
std::string ReachBiome::GetTitle() { return title + BiomeName; }
// ReachBiome.cs:31
std::string ReachBiome::GetTitleStringShort(const std::string& biomeName) { return biomeName; }
// ReachBiome.cs:36
void ReachBiome::OnReset() { SetIncomplete(); }

// ReachBiome.cs:41
void ReachBiome::OnLoad(ConfigNode& node) {
    if (node.HasValue("biome")) BiomeName = *node.GetValue("biome");
    if (node.HasValue("title")) title = *node.GetValue("title");
}

// ReachBiome.cs:53
void ReachBiome::OnSave(ConfigNode& node) {
    node.AddValue("biome", BiomeName);
    node.AddValue("title", title);
}

// ReachBiome.cs:59
void ReachBiome::OnUpdate() {
    if (Root()->ContractState() == Contract::State::Active && Lite::FlightIsReady()) {
        IsWithinBiome = false;
        if (bodyHasBiome(FlightRegistry::ActiveVessel->mainBody())) IsWithinBiome = checkVesselWithinBiome(FlightRegistry::ActiveVessel);
        if (IsWithinBiome && State() == ParameterState::Incomplete) SetComplete();
        if (!IsWithinBiome && State() == ParameterState::Complete) SetIncomplete();
    }
}

// ReachBiome.cs:79
bool ReachBiome::checkVesselWithinBiome(Vessel* v) {
    if (!BiomeName.empty()) {
        if (v->mainBody()->BiomeMap.has_value()) {
            ContractsEnvironment& env = ContractsEnvironment::Current();
            return env.biomeNameAt && env.biomeNameAt(v->mainBody(), v->latitude, v->longitude) == BiomeName;
        }
        return false;
    }
    return true;
}

// ReachBiome.cs:92
bool ReachBiome::bodyHasBiome(CelestialBody* body) {
    if (!BiomeName.empty() && body->BiomeMap.has_value()) {
        int num = static_cast<int>(body->BiomeMap->Attributes.size());
        do {
            if (num-- <= 0) return false;
        } while (!(body->BiomeMap->Attributes[static_cast<size_t>(num)].name == BiomeName));
        return true;
    }
    return false;
}

// ---- ReachAltitudeEnvelope / ReachSpeedEnvelope ----------------------------------------------------------------------------

// ReachAltitudeEnvelope.cs:15
ReachAltitudeEnvelope::ReachAltitudeEnvelope() : minAltitude(0.0), maxAltitude(1.7976931348623157e308) {}
// ReachAltitudeEnvelope.cs:21
ReachAltitudeEnvelope::ReachAltitudeEnvelope(float minAlt, float maxAlt)
    : minAltitude(dotnet::Math::Min(static_cast<double>(minAlt), static_cast<double>(maxAlt))),
      maxAltitude(dotnet::Math::Max(static_cast<double>(minAlt), static_cast<double>(maxAlt))) {}
// ReachAltitudeEnvelope.cs:28
std::string ReachAltitudeEnvelope::GetTitle() { return Localizer::Format("#autoLOC_268606", {N0(minAltitude), N0(maxAltitude)}); }
// ReachAltitudeEnvelope.cs:33
void ReachAltitudeEnvelope::OnReset() { SetIncomplete(); }
// ReachAltitudeEnvelope.cs:38
void ReachAltitudeEnvelope::OnLoad(ConfigNode& node) {
    Lite::LoadNode(node, "ReachAltitudeEnvelope", "minAlt", minAltitude, 0.0);
    Lite::LoadNode(node, "ReachAltitudeEnvelope", "maxAlt", maxAltitude, 1.7976931348623157e308);
}
// ReachAltitudeEnvelope.cs:44
void ReachAltitudeEnvelope::OnSave(ConfigNode& node) {
    node.AddValue("minAlt", minAltitude);
    node.AddValue("maxAlt", maxAltitude);
}
// ReachAltitudeEnvelope.cs:50
void ReachAltitudeEnvelope::OnUpdate() {
    if (Root()->ContractState() != Contract::State::Active || !Lite::FlightIsReady()) return;
    Vessel* av = FlightRegistry::ActiveVessel;
    if (av->altitude < maxAltitude) {
        const bool num = av->altitude >= minAltitude;
        if (num && State() == ParameterState::Incomplete) SetComplete();
        if (num) return;
    }
    if (State() == ParameterState::Complete) SetIncomplete();
}

// ReachSpeedEnvelope.cs:15
ReachSpeedEnvelope::ReachSpeedEnvelope() : minSpeed(0.0), maxSpeed(1.7976931348623157e308) {}
// ReachSpeedEnvelope.cs:21
ReachSpeedEnvelope::ReachSpeedEnvelope(float minSpd, float maxSpd)
    : minSpeed(dotnet::Math::Min(static_cast<double>(minSpd), static_cast<double>(maxSpd))),
      maxSpeed(dotnet::Math::Max(static_cast<double>(minSpd), static_cast<double>(maxSpd))) {}
// ReachSpeedEnvelope.cs:28
std::string ReachSpeedEnvelope::GetTitle() { return Localizer::Format("#autoLOC_268955", {N1(minSpeed), N1(maxSpeed)}); }
// ReachSpeedEnvelope.cs:33
void ReachSpeedEnvelope::OnReset() { SetIncomplete(); }
// ReachSpeedEnvelope.cs:38
void ReachSpeedEnvelope::OnLoad(ConfigNode& node) {
    Lite::LoadNode(node, "ReachSpeedEnvelope", "minSpd", minSpeed, 0.0);
    Lite::LoadNode(node, "ReachSpeedEnvelope", "maxSpd", maxSpeed, 1.7976931348623157e308);
}
// ReachSpeedEnvelope.cs:44
void ReachSpeedEnvelope::OnSave(ConfigNode& node) {
    node.AddValue("minSpd", minSpeed);
    node.AddValue("maxSpd", maxSpeed);
}
// ReachSpeedEnvelope.cs:50
void ReachSpeedEnvelope::OnUpdate() {
    if (Root()->ContractState() == Contract::State::Active && Lite::FlightIsReady()) {
        const bool num = checkVesselWithinEnvelope(FlightRegistry::ActiveVessel);
        if (num && State() == ParameterState::Incomplete) SetComplete();
        if (!num && State() == ParameterState::Complete) SetIncomplete();
    }
}
// ReachSpeedEnvelope.cs:62
bool ReachSpeedEnvelope::checkVesselWithinEnvelope(Vessel* v) {
    switch (v->situation) {
        case Vessel::Situations::LANDED:
        case Vessel::Situations::SPLASHED:
        case Vessel::Situations::PRELAUNCH:
        case Vessel::Situations::FLYING:
            if (v->srfSpeed < maxSpeed) return v->srfSpeed >= minSpeed;
            return false;
        default: return false;
        case Vessel::Situations::SUB_ORBITAL:
        case Vessel::Situations::ORBITING:
        case Vessel::Situations::ESCAPING:
            if (v->obt_speed < maxSpeed) return v->srfSpeed >= minSpeed;  // sic: srfSpeed in the C#
            return false;
    }
}

// ---- ReachFlightEnvelope -------------------------------------------------------------------------------------------------

// ReachFlightEnvelope.cs:39
ReachFlightEnvelope::ReachFlightEnvelope(CelestialBody* dest, Vessel::Situations sit, const std::string* biomeName, float maxAlt, float minAlt,
                                         float maxSpd, float minSpd, std::string title_)
    : Destination(dest), Situation(sit), BiomeName(biomeName != nullptr ? *biomeName : std::string()), title(std::move(title_)) {
    maxAltitude = static_cast<double>(maxAlt);
    minAltitude = static_cast<double>(minAlt);
    useAltLimits = maxAltitude != 3.4028234663852886E+38 && minAltitude != 0.0;
    maxSpeed = static_cast<double>(maxSpd);
    minSpeed = static_cast<double>(minSpd);
    useSpdLimits = maxSpeed != 3.4028234663852886E+38 && minSpeed != 0.0;
}

// ReachFlightEnvelope.cs:54
std::string ReachFlightEnvelope::GetTitle() {
    title += Localizer::Format("#autoLOC_270222", {title, GetSituationStringShort(Situation, Destination, BiomeName)});
    if (useAltLimits) title += Localizer::Format("#autoLOC_270224", {N0(minAltitude), N0(maxAltitude)});
    if (useSpdLimits) title += Localizer::Format("#autoLOC_270225", {N1(minAltitude), N1(maxAltitude)});  // sic: altitudes in the C#
    return title;
}

// ReachFlightEnvelope.cs:69
std::string ReachFlightEnvelope::GetSituationStringShort(Vessel::Situations sit, CelestialBody* body, const std::string& biomeName) {
    if (biomeName.empty()) {
        switch (sit) {
            case Vessel::Situations::FLYING: return Fmt1("#autoLOC_270235", body->displayName());
            case Vessel::Situations::LANDED: return Fmt1("#autoLOC_270236", body->displayName());
            case Vessel::Situations::SPLASHED: return Fmt1("#autoLOC_270238", body->displayName());
            case Vessel::Situations::PRELAUNCH: return Fmt1("#autoLOC_270234", body->displayName());
            default: return Localizer::Format("#autoLOC_270241");
            case Vessel::Situations::ESCAPING: return Fmt1("#autoLOC_270240", body->displayName());
            case Vessel::Situations::ORBITING: return Fmt1("#autoLOC_270237", body->displayName());
            case Vessel::Situations::SUB_ORBITAL: return Fmt1("#autoLOC_270239", body->displayName());
        }
    }
    switch (sit) {
        case Vessel::Situations::FLYING: return Localizer::Format("#autoLOC_270249", {body->name(), biomeName});
        case Vessel::Situations::LANDED: return Localizer::Format("#autoLOC_270250", {body->name(), biomeName});
        case Vessel::Situations::SPLASHED: return Localizer::Format("#autoLOC_270252", {body->name(), biomeName});
        case Vessel::Situations::PRELAUNCH: return Localizer::Format("#autoLOC_270248", {body->name(), biomeName});
        default: return Localizer::Format("#autoLOC_270255");
        case Vessel::Situations::ESCAPING: return Localizer::Format("#autoLOC_270254", {body->name(), biomeName});
        case Vessel::Situations::ORBITING: return Localizer::Format("#autoLOC_270251", {body->name(), biomeName});
        case Vessel::Situations::SUB_ORBITAL: return Localizer::Format("#autoLOC_270253", {body->name(), biomeName});
    }
}

// ReachFlightEnvelope.cs:112
void ReachFlightEnvelope::OnLoad(ConfigNode& node) {
    if (node.HasValue("dest")) Destination = BodyAt(dotnet::ParseInt32(*node.GetValue("dest")));
    if (node.HasValue("sit")) Situation = dotnet::EnumParseFull<Vessel::Situations>(*node.GetValue("sit"));
    if (node.HasValue("biome")) BiomeName = *node.GetValue("biome");
    if (node.HasValue("maxAlt")) maxAltitude = dotnet::ParseDouble(*node.GetValue("maxAlt"));
    if (node.HasValue("minAlt")) minAltitude = dotnet::ParseDouble(*node.GetValue("minAlt"));
    if (node.HasValue("maxSpd")) maxSpeed = dotnet::ParseDouble(*node.GetValue("maxSpd"));
    if (node.HasValue("minSpd")) minSpeed = dotnet::ParseDouble(*node.GetValue("minSpd"));
    if (node.HasValue("useAltLimit")) useAltLimits = ParseBool(*node.GetValue("useAltLimit"));
    if (node.HasValue("useSpdLimit")) useSpdLimits = ParseBool(*node.GetValue("useSpdLimit"));
    if (node.HasValue("title")) title = *node.GetValue("title");
}

// ReachFlightEnvelope.cs:156
void ReachFlightEnvelope::OnSave(ConfigNode& node) {
    node.AddValue("dest", static_cast<int32_t>(BodyIndexOf(Destination)));
    node.AddValue("sit", dotnet::EnumToString(Situation));
    node.AddValue("biome", BiomeName);
    node.AddValue("maxAlt", maxAltitude);
    node.AddValue("minAlt", minAltitude);
    node.AddValue("maxSpd", maxSpeed);
    node.AddValue("minSpd", minSpeed);
    node.AddValue("useAltLimit", useAltLimits);
    node.AddValue("useSpdLimit", useSpdLimits);
    node.AddValue("title", title);
}

// ReachFlightEnvelope.cs:170
void ReachFlightEnvelope::OnRegister() {
    if (SaveEnvironment::Current().loadedScene == GameScenes::FLIGHT) {
        GE::onFlightReady.Add(this, &ReachFlightEnvelope::OnFlightReady);
        GE::onVesselChange.Add(this, &ReachFlightEnvelope::OnVesselChange);
        GE::onVesselSituationChange.Add(this, &ReachFlightEnvelope::OnVesselSituationChange);
        trackerActive = true;
    } else if (State() == ParameterState::Complete && Root()->ContractState() != Contract::State::Completed) {
        SetIncomplete();
    }
}

// ReachFlightEnvelope.cs:185
void ReachFlightEnvelope::OnUnregister() {
    if (trackerActive) {
        trackerActive = false;
        GE::onFlightReady.Remove(this, &ReachFlightEnvelope::OnFlightReady);
        GE::onVesselChange.Remove(this, &ReachFlightEnvelope::OnVesselChange);
        GE::onVesselSituationChange.Remove(this, &ReachFlightEnvelope::OnVesselSituationChange);
    }
}

// ReachFlightEnvelope.cs:196
void ReachFlightEnvelope::OnFlightReady() {
    tgtVessel = FlightRegistry::ActiveVessel;
    CareerEnvironment::Current().Scheduler().StartCoroutine(TrackVessel(tgtVessel));
}

// ReachFlightEnvelope.cs:202
void ReachFlightEnvelope::OnVesselChange(Vessel* v) {
    tgtVessel = v;
    CareerEnvironment::Current().Scheduler().StartCoroutine(TrackVessel(tgtVessel));
}

// ReachFlightEnvelope.cs:208
void ReachFlightEnvelope::OnVesselSituationChange(GE::HostedFromToAction<Vessel*, Vessel::Situations> v) {
    if (v.host == tgtVessel) CareerEnvironment::Current().Scheduler().StartCoroutine(TrackVessel(tgtVessel));
}

// ReachFlightEnvelope.cs:216
Enumerator ReachFlightEnvelope::TrackVessel(Vessel* v) {
    log::Info("[Parameter.ReachFlightEnvelope]: Tracking Started for " + v->GetDisplayName());
    while (v == tgtVessel && v->situation == Situation && Root()->ContractState() == Contract::State::Active) {
        if (State() == ParameterState::Incomplete && checkVesselWithinFlightEnvelope(tgtVessel)) {
            if (checkVesselWithinBiome(tgtVessel)) SetComplete();
        } else if ((State() == ParameterState::Complete && !checkVesselWithinFlightEnvelope(tgtVessel)) || !checkVesselWithinBiome(tgtVessel)) {
            SetIncomplete();
        }
        co_yield YieldInstruction::Null();
    }
    if (State() == ParameterState::Complete && Root()->ContractState() != Contract::State::Completed) SetIncomplete();
}

// ReachFlightEnvelope.cs:240
bool ReachFlightEnvelope::checkVesselWithinBiome(Vessel* v) {
    if (!BiomeName.empty()) {
        if (v->mainBody()->BiomeMap.has_value()) {
            ContractsEnvironment& env = ContractsEnvironment::Current();
            return env.biomeNameAt && env.biomeNameAt(v->mainBody(), v->latitude, v->longitude) == BiomeName;
        }
        return false;
    }
    return true;
}

// ReachFlightEnvelope.cs:253
bool ReachFlightEnvelope::checkVesselWithinFlightEnvelope(Vessel* v) {
    if (v->situation != Situation) return false;
    switch (Situation) {
        case Vessel::Situations::FLYING:
            if (v->srfSpeed < maxSpeed && v->srfSpeed >= minSpeed && v->altitude < maxAltitude && v->altitude >= minAltitude) return true;
            break;
        case Vessel::Situations::LANDED:
        case Vessel::Situations::SPLASHED:
        case Vessel::Situations::PRELAUNCH:
            if (v->srfSpeed < maxSpeed && v->srfSpeed >= minSpeed) return true;
            break;
        case Vessel::Situations::SUB_ORBITAL:
        case Vessel::Situations::ORBITING:
        case Vessel::Situations::ESCAPING:
            if (v->obt_speed < maxSpeed && v->obt_speed >= minSpeed && v->altitude < maxAltitude && v->altitude >= minAltitude) return true;
            break;
        default: break;
    }
    return false;
}

// ---- RecoverKerbal ---------------------------------------------------------------------------------------------------------

// RecoverKerbal.cs:35
RecoverKerbal::RecoverKerbal(std::string title_, CompleteCondition win, CompleteCondition fail)
    : title(std::move(title_)), failCondition(fail), winCondition(win) {}

// RecoverKerbal.cs:48
void RecoverKerbal::OnLoad(ConfigNode& node) {
    if (node.HasValue("title")) title = *node.GetValue("title");
    if (node.HasValue("failOn")) failCondition = dotnet::EnumParseFull<CompleteCondition>(*node.GetValue("failOn"));
    if (node.HasValue("winOn")) winCondition = dotnet::EnumParseFull<CompleteCondition>(*node.GetValue("winOn"));
    const std::vector<std::string> values = node.GetValues("kerbal");
    for (const auto& v : values) kerbalsToRecover.push_back(v);
}

// RecoverKerbal.cs:70
void RecoverKerbal::OnSave(ConfigNode& node) {
    node.AddValue("title", title);
    node.AddValue("failOn", dotnet::EnumToString(failCondition));
    node.AddValue("winOn", dotnet::EnumToString(winCondition));
    for (const auto& k : kerbalsToRecover) node.AddValue("kerbal", k);
}

// RecoverKerbal.cs:82
void RecoverKerbal::OnRegister() {
    if (Root()->ContractState() == Contract::State::Active) {
        GE::onVesselRecovered.Add(this, &RecoverKerbal::OnVesselRecovered);
        GE::onCrewKilled.Add(this, &RecoverKerbal::OnCrewKilled);
        GE::onPartDie.Add(this, &RecoverKerbal::OnPartDied);
        GE::onVesselLoaded.Add(this, &RecoverKerbal::OnVesselLoad);
        GE::onFlightReady.Add(this, &RecoverKerbal::OnFlightReady);
        eventsAdded = true;
    }
}

// RecoverKerbal.cs:95
void RecoverKerbal::OnUnregister() {
    if (eventsAdded) {
        GE::onVesselRecovered.Remove(this, &RecoverKerbal::OnVesselRecovered);
        GE::onCrewKilled.Remove(this, &RecoverKerbal::OnCrewKilled);
        GE::onPartDie.Remove(this, &RecoverKerbal::OnPartDied);
        GE::onVesselLoaded.Remove(this, &RecoverKerbal::OnVesselLoad);
        GE::onFlightReady.Remove(this, &RecoverKerbal::OnFlightReady);
    }
}

// RecoverKerbal.cs:107
void RecoverKerbal::OnVesselRecovered(ProtoVessel* v, bool) {
    switch (winCondition) {
        case CompleteCondition::All: {
            const auto& vesselCrew = v->GetVesselCrew();
            int count = static_cast<int>(vesselCrew.size());
            while (count-- > 0) {
                int count2 = static_cast<int>(kerbalsToRecover.size());
                while (count2-- > 0) {
                    if (vesselCrew[static_cast<size_t>(count)]->name().value_or(std::string()) == kerbalsToRecover[static_cast<size_t>(count2)])
                        kerbalsToRecover.erase(kerbalsToRecover.begin() + count2);
                }
                if (kerbalsToRecover.empty()) SetComplete();
            }
            return;
        }
        default: break;
    }
    const auto& vesselCrew = v->GetVesselCrew();
    int count3 = static_cast<int>(vesselCrew.size());
    while (count3-- > 0) {
        bool flag = false;
        int count4 = static_cast<int>(kerbalsToRecover.size());
        while (count4-- > 0) {
            if (vesselCrew[static_cast<size_t>(count3)]->name().value_or(std::string()) == kerbalsToRecover[static_cast<size_t>(count4)]) {
                flag = true;
                SetComplete();
                break;
            }
        }
        if (flag) break;
    }
}

// RecoverKerbal.cs:156
void RecoverKerbal::OnCrewKilled(EventReport* evt) {
    switch (failCondition) {
        case CompleteCondition::Any: {
            int count2 = static_cast<int>(kerbalsToRecover.size());
            do {
                if (count2-- <= 0) return;
            } while (!(evt->sender == kerbalsToRecover[static_cast<size_t>(count2)]));
            SetFailed();
            break;
        }
        case CompleteCondition::All: {
            int count = static_cast<int>(kerbalsToRecover.size());
            while (true) {
                if (count-- <= 0) return;
                if (evt->sender == kerbalsToRecover[static_cast<size_t>(count)]) {
                    kerbalsToRecover.erase(kerbalsToRecover.begin() + count);
                    if (kerbalsToRecover.empty()) break;
                }
            }
            SetFailed();
            break;
        }
    }
}

// RecoverKerbal.cs:198
void RecoverKerbal::OnPartDied(Part* p) {
    int count = static_cast<int>(p->protoModuleCrew.size());
    while (count-- > 0) {
        ProtoCrewMember& protoCrewMember = *p->protoModuleCrew[static_cast<size_t>(count)];
        while (Contains(kerbalsToRecover, protoCrewMember.name().value_or(std::string()))) {
            if (failCondition != CompleteCondition::Any) {
                RemoveFirst(kerbalsToRecover, protoCrewMember.name().value_or(std::string()));
                continue;
            }
            SetFailed();
            return;
        }
        if (failCondition == CompleteCondition::All && kerbalsToRecover.size() <= 0) SetFailed();
    }
}

// RecoverKerbal.cs:221
void RecoverKerbal::AddKerbal(const std::string& kerbalName) {
    if (!Contains(kerbalsToRecover, kerbalName)) kerbalsToRecover.push_back(kerbalName);
}
// RecoverKerbal.cs:229
void RecoverKerbal::RemoveKerbal(const std::string& kerbalName) {
    if (Contains(kerbalsToRecover, kerbalName)) RemoveFirst(kerbalsToRecover, kerbalName);
}
// RecoverKerbal.cs:242
void RecoverKerbal::OnVesselLoad(Vessel* v) {
    if (auto* recoverAsset = dynamic_cast<Templates::RecoverAsset*>(Root())) recoverAsset->DiscoverAsset(v);
}
// RecoverKerbal.cs:250
void RecoverKerbal::OnFlightReady() {
    if (auto* recoverAsset = dynamic_cast<Templates::RecoverAsset*>(Root())) {
        int count = static_cast<int>(FlightRegistry::VesselsLoaded.size());
        while (count-- > 0) recoverAsset->DiscoverAsset(FlightRegistry::VesselsLoaded[static_cast<size_t>(count)]);
    }
}

// ---- RecoverPart ------------------------------------------------------------------------------------------------------------

// RecoverPart.cs:34
RecoverPart::RecoverPart() : title("Recover a part."), failCondition(CompleteCondition::AnyCandidate), winCondition(CompleteCondition::AllCandidates) {}
// RecoverPart.cs:36
RecoverPart::RecoverPart(std::string title_, CompleteCondition win, CompleteCondition fail) : title(std::move(title_)), failCondition(fail), winCondition(win) {}

// RecoverPart.cs:49
void RecoverPart::OnLoad(ConfigNode& node) {
    Lite::LoadNode(node, "Parameter.RecoverPart", "title", title, std::string("Recover a part."));
    Lite::LoadNode(node, "Parameter.RecoverPart", "failOn", failCondition, CompleteCondition::AnyCandidate);
    Lite::LoadNode(node, "Parameter.RecoverPart", "winOn", winCondition, CompleteCondition::AnyCandidate);
    const std::vector<std::string> values = node.GetValues("part");
    for (const auto& v : values) {
        uint32_t u = 0;
        if (!dotnet::TryParseUInt32(v, u)) throw dotnet::FormatException("Input string was not in a correct format.");
        partsToRecover.push_back(u);
    }
}

// RecoverPart.cs:62
void RecoverPart::OnSave(ConfigNode& node) {
    node.AddValue("title", title);
    node.AddValue("failOn", dotnet::EnumToString(failCondition));
    node.AddValue("winOn", dotnet::EnumToString(winCondition));
    for (uint32_t p : partsToRecover) node.AddValue("part", p);
}

// RecoverPart.cs:74
void RecoverPart::OnRegister() {
    if (Root()->ContractState() == Contract::State::Active) {
        GE::onVesselRecovered.Add(this, &RecoverPart::OnVesselRecovered);
        GE::onPartDie.Add(this, &RecoverPart::OnPartDie);
        GE::onVesselLoaded.Add(this, &RecoverPart::OnVesselLoad);
        GE::onFlightReady.Add(this, &RecoverPart::OnFlightReady);
        eventsAdded = true;
    }
}

// RecoverPart.cs:86
void RecoverPart::OnUnregister() {
    if (eventsAdded) {
        GE::onVesselRecovered.Remove(this, &RecoverPart::OnVesselRecovered);
        GE::onPartDie.Remove(this, &RecoverPart::OnPartDie);
        GE::onVesselLoaded.Remove(this, &RecoverPart::OnVesselLoad);
        GE::onFlightReady.Remove(this, &RecoverPart::OnFlightReady);
    }
}

// RecoverPart.cs:97
void RecoverPart::OnVesselRecovered(ProtoVessel* v, bool) {
    if (winCondition == CompleteCondition::AnyCandidate) {
        int count = static_cast<int>(v->protoPartSnapshots.size());
        do {
            if (count-- <= 0) return;
        } while (!Contains(partsToRecover, v->protoPartSnapshots[static_cast<size_t>(count)]->flightID));
        SetComplete();
    } else {
        if (winCondition != CompleteCondition::AllCandidates) return;
        int count2 = static_cast<int>(v->protoPartSnapshots.size());
        while (count2-- > 0) {
            ProtoPartSnapshot& snap = *v->protoPartSnapshots[static_cast<size_t>(count2)];
            if (Contains(partsToRecover, snap.flightID)) {
                RemovePartToRecover(snap.flightID);
                if (partsToRecover.size() <= 0) SetComplete();
            }
        }
    }
}

// RecoverPart.cs:134
void RecoverPart::OnPartDie(Part* p) {
    if (failCondition == CompleteCondition::AnyCandidate) {
        if (Contains(partsToRecover, p->fields.flightID)) SetFailed();
    } else if (failCondition == CompleteCondition::AllCandidates && Contains(partsToRecover, p->fields.flightID)) {
        RemovePartToRecover(p->fields.flightID);
        if (partsToRecover.empty()) SetFailed();
    }
}

// RecoverPart.cs:153
void RecoverPart::AddPartToRecover(uint32_t partFlightId) {
    if (!Contains(partsToRecover, partFlightId)) partsToRecover.push_back(partFlightId);
}
// RecoverPart.cs:161
void RecoverPart::RemovePartToRecover(uint32_t partFlightId) {
    if (Contains(partsToRecover, partFlightId)) RemoveFirst(partsToRecover, partFlightId);
}
// RecoverPart.cs:169
void RecoverPart::OnVesselLoad(Vessel* v) {
    if (auto* recoverAsset = dynamic_cast<Templates::RecoverAsset*>(Root())) recoverAsset->DiscoverAsset(v);
}
// RecoverPart.cs:177
void RecoverPart::OnFlightReady() {
    if (auto* recoverAsset = dynamic_cast<Templates::RecoverAsset*>(Root())) {
        int count = static_cast<int>(FlightRegistry::VesselsLoaded.size());
        while (count-- > 0) recoverAsset->DiscoverAsset(FlightRegistry::VesselsLoaded[static_cast<size_t>(count)]);
    }
}

// ---- AcquireCrew -----------------------------------------------------------------------------------------------------------

// AcquireCrew.cs:31
AcquireCrew::AcquireCrew(std::string title_, CompleteCondition win) : title(std::move(title_)), winCondition(win) {}

// AcquireCrew.cs:48
void AcquireCrew::OnLoad(ConfigNode& node) {
    Lite::LoadNode(node, "Parameter.AcquireCrew", "title", title, std::string());
    Lite::LoadNode(node, "Parameter.AcquireCrew", "winOn", winCondition, CompleteCondition::Any);
    const std::vector<std::string> values = node.GetValues("kerbal");
    for (const auto& v : values) kerbalsToRecover.push_back(v);
}

// AcquireCrew.cs:60
void AcquireCrew::OnSave(ConfigNode& node) {
    node.AddValue("title", title);
    node.AddValue("winOn", static_cast<int32_t>(winCondition));
    for (const auto& k : kerbalsToRecover) node.AddValue("kerbal", k);
}

// AcquireCrew.cs:72
void AcquireCrew::OnRegister() {
    eventsAdded = false;
    if (Root()->ContractState() == Contract::State::Active) {
        GE::onCrewBoardVessel.Add(this, &AcquireCrew::OnCrewBoardVessel);
        GE::onCrewTransferred.Add(this, &AcquireCrew::OnCrewTransferred);
        GE::onPartCouple.Add(this, &AcquireCrew::OnCouple);
        GE::onCrewKilled.Add(this, &AcquireCrew::OnCrewKilled);
        GE::onPartDie.Add(this, &AcquireCrew::OnPartDied);
        eventsAdded = true;
    }
}

// AcquireCrew.cs:86
void AcquireCrew::OnUnregister() {
    if (eventsAdded) {
        GE::onCrewBoardVessel.Remove(this, &AcquireCrew::OnCrewBoardVessel);
        GE::onCrewTransferred.Remove(this, &AcquireCrew::OnCrewTransferred);
        GE::onPartCouple.Remove(this, &AcquireCrew::OnCouple);
        GE::onCrewKilled.Remove(this, &AcquireCrew::OnCrewKilled);
        GE::onPartDie.Remove(this, &AcquireCrew::OnPartDied);
    }
}

// AcquireCrew.cs:98
void AcquireCrew::OnReset() {
    if (Lite::FlightIsReady(true)) ScanVessel(FlightRegistry::ActiveVessel);
}
// AcquireCrew.cs:106
void AcquireCrew::OnCrewBoardVessel(GE::FromToAction<Part*, Part*> action) { CheckNewKerbal(action.from->vessel->vesselName); }
// AcquireCrew.cs:111
void AcquireCrew::OnCrewTransferred(GE::HostedFromToAction<ProtoCrewMember*, Part*> action) { CheckNewKerbal(action.host->name().value_or(std::string())); }
// AcquireCrew.cs:116
void AcquireCrew::OnCouple(GE::FromToAction<Part*, Part*> action) { ScanVessel(action.from->vessel); }
// AcquireCrew.cs:121
void AcquireCrew::OnCrewKilled(EventReport* evt) { KillKerbal(evt->sender); }
// AcquireCrew.cs:126
void AcquireCrew::OnPartDied(Part* p) {
    int count = static_cast<int>(p->protoModuleCrew.size());
    while (count-- > 0) KillKerbal(p->protoModuleCrew[static_cast<size_t>(count)]->name().value_or(std::string()));
}
// AcquireCrew.cs:135
void AcquireCrew::AddKerbal(const std::string& kerbalName) {
    if (!Contains(kerbalsToRecover, kerbalName)) kerbalsToRecover.push_back(kerbalName);
}
// AcquireCrew.cs:143
void AcquireCrew::RemoveKerbal(const std::string& kerbalName) {
    if (Contains(kerbalsToRecover, kerbalName)) RemoveFirst(kerbalsToRecover, kerbalName);
}
// AcquireCrew.cs:151
void AcquireCrew::ScanVessel(Vessel* v) {
    int count = static_cast<int>(v->parts.size());
    while (count-- > 0) {
        Part* part = v->parts[static_cast<size_t>(count)];
        int count2 = static_cast<int>(part->protoModuleCrew.size());
        while (count2-- > 0) CheckNewKerbal(part->protoModuleCrew[static_cast<size_t>(count2)]->name().value_or(std::string()));
    }
}
// AcquireCrew.cs:165
void AcquireCrew::CheckNewKerbal(const std::string& kerbalName) {
    if (Contains(kerbalsToRecover, kerbalName)) {
        kerbalsToRecover.erase(std::remove(kerbalsToRecover.begin(), kerbalsToRecover.end(), kerbalName), kerbalsToRecover.end());
        if (winCondition == CompleteCondition::Any)
            SetComplete();
        else if (winCondition == CompleteCondition::All && kerbalsToRecover.size() <= 0)
            SetComplete();
    }
}
// AcquireCrew.cs:181
void AcquireCrew::KillKerbal(const std::string& kerbalName) {
    if (Contains(kerbalsToRecover, kerbalName)) {
        kerbalsToRecover.erase(std::remove(kerbalsToRecover.begin(), kerbalsToRecover.end(), kerbalName), kerbalsToRecover.end());
        if (winCondition == CompleteCondition::All)
            SetFailed();
        else if (winCondition == CompleteCondition::Any && kerbalsToRecover.size() <= 0)
            SetFailed();
    }
}

// ---- AcquirePart -----------------------------------------------------------------------------------------------------------

AcquirePart::AcquirePart(std::string title_, CompleteCondition win) : title(std::move(title_)), winCondition(win) {}

void AcquirePart::OnLoad(ConfigNode& node) {
    Lite::LoadNode(node, "Parameter.AcquirePart", "title", title, std::string());
    Lite::LoadNode(node, "Parameter.AcquirePart", "winOn", winCondition, CompleteCondition::Any);
    const std::vector<std::string> values = node.GetValues("part");
    for (const auto& v : values) {
        uint32_t u = 0;
        if (!dotnet::TryParseUInt32(v, u)) throw dotnet::FormatException("Input string was not in a correct format.");
        partsToRecover.push_back(u);
    }
}
void AcquirePart::OnSave(ConfigNode& node) {
    node.AddValue("title", title);
    node.AddValue("winOn", static_cast<int32_t>(winCondition));
    for (uint32_t p : partsToRecover) node.AddValue("part", dotnet::FormatUInt64(p));
}
void AcquirePart::OnRegister() {
    eventsAdded = false;
    if (Root()->ContractState() == Contract::State::Active) {
        GE::onPartCouple.Add(this, &AcquirePart::OnCouple);
        GE::onPartDie.Add(this, &AcquirePart::OnPartDied);
        eventsAdded = true;
    }
}
void AcquirePart::OnUnregister() {
    if (eventsAdded) {
        GE::onPartCouple.Remove(this, &AcquirePart::OnCouple);
        GE::onPartDie.Remove(this, &AcquirePart::OnPartDied);
    }
}
void AcquirePart::OnReset() {
    if (Lite::FlightIsReady(true)) ScanVessel(FlightRegistry::ActiveVessel);
}
void AcquirePart::OnCouple(GE::FromToAction<Part*, Part*> action) { ScanVessel(action.from->vessel); }
void AcquirePart::OnPartDied(Part* p) { DestroyPart(p->fields.flightID); }
void AcquirePart::AddPart(uint32_t id) {
    if (!Contains(partsToRecover, id)) partsToRecover.push_back(id);
}
void AcquirePart::RemovePart(uint32_t id) {
    if (Contains(partsToRecover, id)) RemoveFirst(partsToRecover, id);
}
void AcquirePart::ScanVessel(Vessel* v) {
    int count = static_cast<int>(v->parts.size());
    while (count-- > 0) CheckNewPart(v->parts[static_cast<size_t>(count)]->fields.flightID);
}
void AcquirePart::CheckNewPart(uint32_t id) {
    if (Contains(partsToRecover, id)) {
        partsToRecover.erase(std::remove(partsToRecover.begin(), partsToRecover.end(), id), partsToRecover.end());
        if (winCondition == CompleteCondition::Any)
            SetComplete();
        else if (winCondition == CompleteCondition::All && partsToRecover.size() <= 0)
            SetComplete();
    }
}
void AcquirePart::DestroyPart(uint32_t id) {
    if (Contains(partsToRecover, id)) {
        partsToRecover.erase(std::remove(partsToRecover.begin(), partsToRecover.end(), id), partsToRecover.end());
        if (winCondition == CompleteCondition::All)
            SetFailed();
        else if (winCondition == CompleteCondition::Any && partsToRecover.size() <= 0)
            SetFailed();
    }
}

// ---- PartTest (parameter) ---------------------------------------------------------------------------------------------------

// Parameters/PartTest.cs:42
PartTest::PartTest() = default;

// Parameters/PartTest.cs:42
PartTest::PartTest(const AvailablePart* tgtPart, const std::optional<std::string>& testNotes_, int partRepeatability, CelestialBody* tgtBody, Vessel::Situations tgtSit,
                   const std::string& id_, bool hauled_) {
    tgtPartInfo = tgtPart;
    partName = tgtPartInfo->name;
    testNotes = testNotes_;
    repeatability = partRepeatability;
    body = tgtBody->bodyName;
    situation = dotnet::EnumToString(tgtSit);
    uniqueID = id_;
    hauled = hauled_;
}

// Parameters/PartTest.cs:60
std::string PartTest::GetHashString() {
    std::string text = partName;
    switch (repeatability) {
        case 1:  // BODYANDSITUATION
            text = text + body + situation;
            break;
        case 0:  // ALWAYS
            text += uniqueID;
            break;
        default: break;
    }
    return text;
}

// Parameters/PartTest.cs:75
std::string PartTest::GetTitle() {
    if (!title.empty()) return title;
    const std::string text = (tgtPartInfo != nullptr) ? tgtPartInfo->title : partName;
    if (hauled) return Fmt1("#autoLOC_6100000", text);
    return Fmt1("#autoLOC_6100001", text);
}

// Parameters/PartTest.cs:94
void PartTest::OnLoad(ConfigNode& node) {
    if (node.HasValue("part")) {
        partName = *node.GetValue("part");
        const auto& get = SaveEnvironment::Current().getPartInfoByName;
        tgtPartInfo = get ? get(partName) : nullptr;
    }
    if (node.HasValue("notes")) testNotes = *node.GetValue("notes");
    if (node.HasValue("body")) body = *node.GetValue("body");
    if (node.HasValue("situation")) situation = *node.GetValue("situation");
    if (node.HasValue("uniqueID")) uniqueID = *node.GetValue("uniqueID");
    if (node.HasValue("haul")) hauled = ParseBool(*node.GetValue("haul"));
    if (node.HasValue("title")) body = *node.GetValue("title");  // sic: the C# stores the title into `body`
    if (node.HasValue("repeatability")) {
        try {
            repeatability = static_cast<int>(dotnet::EnumParseFull<PartTestConstraint::TestRepeatability>(*node.GetValue("repeatability")));
        } catch (...) {
            log::Error("[PartTestConstraint]: Error parsing value of type repeatability, value =  " + *node.GetValue("repeatability") +
                       " in constraint for Part Test Contract.");
        }
    }
    const std::vector<std::string> values = node.GetValues("tested");
    for (const auto& v : values) {
        uint32_t u = 0;
        if (!dotnet::TryParseUInt32(v, u)) throw dotnet::FormatException("Input string was not in a correct format.");
        testedParts.push_back(u);
    }
    if (tgtPartInfo == nullptr || tgtPartInfo->TechHidden) {
        if (Root()->ContractState() == Contract::State::Active)
            Root()->Fail();
        else if (Root()->ContractState() == Contract::State::Offered)
            Root()->Withdraw();
    }
}

// Parameters/PartTest.cs:155
void PartTest::OnSave(ConfigNode& node) {
    node.AddValue("part", partName);
    node.AddValue("body", body);
    node.AddValue("situation", situation);
    node.AddValue("uniqueID", uniqueID);
    node.AddValue("haul", hauled);
    node.AddValue("repeatability", dotnet::EnumToString(static_cast<PartTestConstraint::TestRepeatability>(repeatability)));
    if (!title.empty()) node.AddValue("title", title);
    if (testNotes.has_value()) node.AddValue("notes", *testNotes);
    for (uint32_t t : testedParts) node.AddValue("tested", t);
}

// Parameters/PartTest.cs:178
void PartTest::OnRegister() {
    if (Root()->ContractState() == Contract::State::Active) {
        if (hauled) {
            SetDisableOnStateChange(false);
            GE::onPartCouple.Add(this, &PartTest::OnDock);
            GE::onVesselWasModified.Add(this, &PartTest::VesselModified);
            GE::onPartDie.Add(this, &PartTest::PartModified);
        } else {
            ModuleTestSubject::onTestRun.Add(this, &PartTest::OnPartRunTest);
            ModuleTestSubject::onTestSujectDestroyed.Add(this, &PartTest::OnTestPartDestroyed);
        }
        eventsAdded = true;
    }
}

// Parameters/PartTest.cs:198
void PartTest::OnUnregister() {
    if (eventsAdded) {
        if (hauled) {
            GE::onPartCouple.Remove(this, &PartTest::OnDock);
            GE::onVesselWasModified.Remove(this, &PartTest::VesselModified);
            GE::onPartDie.Remove(this, &PartTest::PartModified);
        } else {
            ModuleTestSubject::onTestRun.Remove(this, &PartTest::OnPartRunTest);
            ModuleTestSubject::onTestSujectDestroyed.Remove(this, &PartTest::OnTestPartDestroyed);
        }
    }
}

// Parameters/PartTest.cs:216
void PartTest::OnPartRunTest(ModuleTestSubject* p) {
    if (!hauled && !(PartOf(p)->partInfo->name != partName) && AllChildParametersComplete()) {
        if (!Contains(testedParts, PartOf(p)->fields.flightID)) testedParts.push_back(PartOf(p)->fields.flightID);
        SetComplete();
    }
}

// Parameters/PartTest.cs:228
void PartTest::OnTestPartDestroyed(ModuleTestSubject* p) {
    if (!hauled && !(PartOf(p)->partInfo->name != partName) && Contains(testedParts, PartOf(p)->fields.flightID)) RemoveFirst(testedParts, PartOf(p)->fields.flightID);
}

// Parameters/PartTest.cs:236
void PartTest::OnVesselChanged(Vessel* v) {
    if (!hauled) {
        if (state == ParameterState::Incomplete && hasTestedPart(v)) SetComplete();
        if (state == ParameterState::Complete && !hasTestedPart(v)) SetIncomplete();
    }
}

// Parameters/PartTest.cs:251
bool PartTest::hasTestedPart(Vessel* v) {
    int count = static_cast<int>(v->parts.size());
    do {
        if (count-- <= 0) return false;
    } while (!Contains(testedParts, v->parts[static_cast<size_t>(count)]->fields.flightID));
    return true;
}

// Parameters/PartTest.cs:265
void PartTest::OnReset() {
    if (hauled) {
        SetIncomplete();
        dirtyVessel = true;
    }
}

// Parameters/PartTest.cs:274
void PartTest::VesselModified(Vessel* v) {
    if (hauled && v == FlightRegistry::ActiveVessel) dirtyVessel = true;
}

// Parameters/PartTest.cs:282
void PartTest::PartModified(Part* p) {
    if (hauled && p->vessel == FlightRegistry::ActiveVessel) dirtyVessel = true;
}

// Parameters/PartTest.cs:290
void PartTest::OnDock(GE::FromToAction<Part*, Part*> action) {
    if (hauled && (action.from->vessel == FlightRegistry::ActiveVessel || action.to->vessel == FlightRegistry::ActiveVessel)) dirtyVessel = true;
}

// Parameters/PartTest.cs:298
void PartTest::OnUpdate() {
    if (!hauled || !Lite::FlightIsReady(Root()->ContractState(), Contract::State::Active, true)) return;
    Vessel* av = FlightRegistry::ActiveVessel;
    const int num = av->loaded ? static_cast<int>(av->Parts().size()) : static_cast<int>(av->protoVessel->protoPartSnapshots.size());
    if (activePartCount != num) dirtyVessel = true;
    if (dirtyVessel) {
        dirtyVessel = false;
        activePartCount = num;
        validVessel = Lite::VesselHasPartName(partName);
    }
    if (State() == ParameterState::Incomplete) {
        if (validVessel)
            ++successCounter;
        else
            successCounter = 0;
        if (successCounter >= 5) SetComplete();
    }
    if (State() == ParameterState::Complete && !validVessel) SetIncomplete();
}

}  // namespace ksp::Contracts::Parameters
