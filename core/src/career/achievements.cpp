// WP-36: the KSPAchievements/ subtree (every progress node, VesselRef, CrewRef) and ProgressTracking.generateAchievementsTree.
#include <algorithm>

#include "ksp/career/progress.hpp"
#include "ksp/career/upgradeables.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/random.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/save/proto_vessel.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/vessel/module_trip_logger.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

using GE = events::GameEvents;
using Localizer = KSP::Localization::Localizer;
using FinePrint::Utilities::ProgressType;
using HostedSit = events::GameEvents::HostedFromToAction<Vessel*, Vessel_Situations>;

namespace KSPAchievements {

namespace {
Game* CurrentGame() { return SaveEnvironment::Current().currentGame; }
CelestialBody* HomeBody() { return CareerEnvironment::Current().HomeBody(); }
std::string Fmt(const char* tag, const std::string& a) { return Localizer::Format(tag, {a}); }
bool Owned(Vessel* v) { return v->GetDiscoveryInfo().Level() == DiscoveryLevels::Owned; }
// the shared "body == FlightGlobals.GetHomeBody() ? tagHome : tagOther(body)" award text
std::string HomeOr(CelestialBody* body, const char* homeTag, const char* otherTag) {
    return body == HomeBody() ? Localizer::Format(homeTag) : Fmt(otherTag, body->displayName());
}
}  // namespace

// ---- VesselRef ------------------------------------------------------------------------------------------------------------

// VesselRef.cs:39
VesselRef VesselRef::FromVessel(Vessel* v) {
    VesselRef vesselRef;
    if (v == nullptr) return vesselRef;
    if (v->loaded) {
        vesselRef.Name = v->vesselName;
        if (v->rootPart != nullptr) vesselRef.FlagURL = v->rootPart->fields.flagURL.value_or(std::string());
    } else if (v->protoVessel != nullptr) {
        vesselRef.Name = v->protoVessel->vesselName;
        if (!v->protoVessel->protoPartSnapshots.empty() && v->protoVessel->protoPartSnapshots[0] != nullptr)
            vesselRef.FlagURL = v->protoVessel->protoPartSnapshots[0]->flagURL;
    }
    return vesselRef;
}

// VesselRef.cs:65
void VesselRef::Load(const ConfigNode& node) {
    if (node.HasValue("name")) Name = *node.GetValue("name");
    if (node.HasValue("flag")) FlagURL = *node.GetValue("flag");
}

// VesselRef.cs:77
void VesselRef::Save(ConfigNode& node) const {
    node.AddValue("name", Name);
    node.AddValue("flag", FlagURL);
}

// ---- CrewRef ---------------------------------------------------------------------------------------------------------------

// CrewRef.cs:28
CrewRef CrewRef::FromVessel(Vessel* v) {
    CrewRef crewRef;
    if (v == nullptr) return crewRef;
    if (v->loaded)
        crewRef.Crews = v->GetVesselCrew();
    else if (v->protoVessel != nullptr)
        crewRef.Crews = v->protoVessel->GetVesselCrew();
    return crewRef;
}

// CrewRef.cs:46
CrewRef CrewRef::FromProtoVessel(ProtoVessel* pv) {
    CrewRef crewRef;
    if (pv == nullptr) return crewRef;
    crewRef.Crews = pv->GetVesselCrew();
    return crewRef;
}

// CrewRef.cs:57
void CrewRef::Load(const ConfigNode& node) {
    if (!node.HasValue("crews")) return;
    const std::vector<std::string> array = dotnet::Split(*node.GetValue("crews"), ",", false);
    for (size_t i = 0; i < array.size(); ++i) {
        const std::string text = dotnet::Trim(array[i]);
        int32_t result = -1;
        if (dotnet::TryParseInt32(text, result)) {
            auto pcm = (*CurrentGame()->CrewRoster)[result];
            if (pcm != nullptr) Crews.push_back(pcm);
        } else {
            auto pcm2 = (*CurrentGame()->CrewRoster)[text];
            if (pcm2 != nullptr) Crews.push_back(pcm2);
        }
    }
}

// CrewRef.cs:88
void CrewRef::Save(ConfigNode& node) const {
    std::string text;
    const int count = static_cast<int>(Crews.size());
    for (int i = 0; i < count; ++i) {
        const auto& c = Crews[static_cast<size_t>(i)];
        if (c != nullptr) {
            text += c->name().value_or(std::string());
            if (i < count - 1) text += ", ";
        }
    }
    node.AddValue("crews", text);
}

// ---- FirstLaunch ------------------------------------------------------------------------------------------------------------

// FirstLaunch.cs:9
FirstLaunch::FirstLaunch() : ProgressNode("FirstLaunch", false) {
    OnDeploy = [this] { GE::onVesselSituationChange.Add(this, &FirstLaunch::OnVesselSituationChange); };
    OnStow = [this] { GE::onVesselSituationChange.Remove(this, &FirstLaunch::OnVesselSituationChange); };
}

// FirstLaunch.cs:22
void FirstLaunch::OnVesselSituationChange(HostedSit vs) {
    Vessel* host = vs.host;
    if (!host->isEVA() && host->IsControllable() && vs.to == Vessel_Situations::FLYING &&
        (vs.from == Vessel_Situations::PRELAUNCH ||
         (vs.from == Vessel_Situations::LANDED &&
          (host->landedAtLast.find("Runway") != std::string::npos || host->landedAtLast.find("LaunchPad") != std::string::npos))))
        CareerEnvironment::Current().Scheduler().StartCoroutine(TestFlight(host));
}

// FirstLaunch.cs:30
Enumerator FirstLaunch::TestFlight(Vessel* v) {
    const double endTime = Planetarium::fetch->time + 1.0;
    while (true) {
        if (!(Planetarium::fetch->time < endTime)) {
            CrewSensitiveComplete(v);
            if (!IsComplete()) {
                Complete();
                AwardProgressStandard(Localizer::Format("#autoLOC_296348"), ProgressType::FIRSTLAUNCH);
            }
            Achieve();
            GE::VesselSituation::onLaunch.Fire(v);
            break;
        }
        if (v->situation == Vessel_Situations::FLYING && v->verticalSpeed >= 0.0) {
            co_yield YieldInstruction::Null();
            continue;
        }
        break;
    }
}

// ---- BaseConstruction -----------------------------------------------------------------------------------------------------

// BaseConstruction.cs:12
BaseConstruction::BaseConstruction(CelestialBody* cb) : ProgressNode("BaseConstruction", false), body_(cb) {
    OnDeploy = [this] { GE::onPartCouple.Add(this, &BaseConstruction::onPartsCoupled); };
    OnStow = [this] { GE::onPartCouple.Remove(this, &BaseConstruction::onPartsCoupled); };
}

// BaseConstruction.cs:27
void BaseConstruction::onPartsCoupled(GE::FromToAction<Part*, Part*> pa) {
    if (!pa.from->vessel->isEVA() && !pa.to->vessel->isEVA() && pa.from->fields.missionID != pa.to->fields.missionID &&
        pa.to->vessel->LandedOrSplashed() && pa.to->vessel->mainBody() == body_) {
        CrewSensitiveComplete(pa.from->vessel);
        if (!IsComplete()) {
            firstBase_ = VesselRef::FromVessel(pa.to->vessel);
            Complete();
            AwardProgressStandard(Fmt("#autoLOC_295188", body_->displayName()), ProgressType::BASECONSTRUCTION, body_);
        }
        Achieve();
    }
}

// BaseConstruction.cs:42
void BaseConstruction::OnLoad(const ConfigNode& node) {
    if (node.GetNode("base") != nullptr) firstBase_.Load(*node.GetNode("base"));
}

// BaseConstruction.cs:50
void BaseConstruction::OnSave(ConfigNode& node) { firstBase_.Save(*node.AddNode("base")); }

// ---- CelestialBodyEscape ----------------------------------------------------------------------------------------------------

// CelestialBodyEscape.cs:10
CelestialBodyEscape::CelestialBodyEscape(CelestialBody* cb) : ProgressNode("Escape", false), body_(cb) {
    OnDeploy = [this] { GE::onVesselSOIChanged.Add(this, &CelestialBodyEscape::OnSOIEscaped); };
    OnStow = [this] { GE::onVesselSOIChanged.Remove(this, &CelestialBodyEscape::OnSOIEscaped); };
}

// CelestialBodyEscape.cs:24
void CelestialBodyEscape::OnSOIEscaped(GE::HostedFromToAction<Vessel*, CelestialBody*> vcs) {
    if (Owned(vcs.host) && vcs.from == body_) {
        CrewSensitiveComplete(vcs.host);
        if (!IsComplete()) {
            Complete();
            AwardProgressStandard(Fmt("#autoLOC_295242", body_->displayName()), ProgressType::ESCAPE, body_);
        }
        Achieve();
        GE::VesselSituation::onEscape.Fire(vcs.host, body_);
    }
}

// ---- CelestialBodyFlight -----------------------------------------------------------------------------------------------------

// CelestialBodyFlight.cs:14
CelestialBodyFlight::CelestialBodyFlight(CelestialBody* cb) : ProgressNode("Flight", false), body(cb) {
    OnDeploy = [this] { GE::onVesselSituationChange.Add(this, &CelestialBodyFlight::onVesselSitChange); };
    OnStow = [this] { GE::onVesselSituationChange.Remove(this, &CelestialBodyFlight::onVesselSitChange); };
}

// CelestialBodyFlight.cs:30
void CelestialBodyFlight::onVesselSitChange(HostedSit vs) {
    if (Owned(vs.host) && vs.host->mainBody() == body &&
        (vs.from == Vessel_Situations::SPLASHED || vs.from == Vessel_Situations::LANDED || vs.from == Vessel_Situations::SUB_ORBITAL) &&
        vs.to == Vessel_Situations::FLYING) {
        CrewSensitiveComplete(vs.host);
        if (!IsComplete()) {
            firstVessel = VesselRef::FromVessel(vs.host);
            firstCrew = CrewRef::FromVessel(vs.host);
            Complete();
            AwardProgressStandard(HomeOr(body, "#autoLOC_6001937", "#autoLOC_6001938"), ProgressType::FLIGHT, body);
        }
        Achieve();
    }
}

// CelestialBodyFlight.cs:47
void CelestialBodyFlight::OnSave(ConfigNode& node) {
    firstVessel.Save(*node.AddNode("vessel"));
    if (firstCrew.HasAny()) firstCrew.Save(*node.AddNode("crew"));
}

// CelestialBodyFlight.cs:56
void CelestialBodyFlight::OnLoad(const ConfigNode& node) {
    if (node.GetNode("vessel") != nullptr) firstVessel.Load(*node.GetNode("vessel"));
    if (node.GetNode("crew") != nullptr) firstCrew.Load(*node.GetNode("crew"));
}

// ---- CelestialBodyFlyby ------------------------------------------------------------------------------------------------------

// CelestialBodyFlyby.cs:10
CelestialBodyFlyby::CelestialBodyFlyby(CelestialBody* cb) : ProgressNode("Flyby", false), body_(cb) {
    OnDeploy = [this] { GE::onVesselSOIChanged.Add(this, &CelestialBodyFlyby::OnVesselSOIChange); };
    OnStow = [this] { GE::onVesselSOIChanged.Remove(this, &CelestialBodyFlyby::OnVesselSOIChange); };
}

// CelestialBodyFlyby.cs:24
void CelestialBodyFlyby::OnVesselSOIChange(GE::HostedFromToAction<Vessel*, CelestialBody*> vcb) {
    if (Owned(vcb.host) && vcb.from != nullptr && vcb.to == body_) {
        CrewSensitiveComplete(vcb.host);
        if (!IsComplete()) {
            Complete();
            AwardProgressStandard(Fmt("#autoLOC_295360", body_->displayName()), ProgressType::FLYBY, body_);
        }
        Achieve();
        GE::VesselSituation::onFlyBy.Fire(vcb.host, body_);
    }
}

// ---- CelestialBodyLanding ------------------------------------------------------------------------------------------------------

// CelestialBodyLanding.cs:14
CelestialBodyLanding::CelestialBodyLanding(CelestialBody* cb) : ProgressNode("Landing", false), body_(cb) {
    OnDeploy = [this] { GE::onVesselSituationChange.Add(this, &CelestialBodyLanding::OnVesselSituationChange); };
    OnStow = [this] { GE::onVesselSituationChange.Remove(this, &CelestialBodyLanding::OnVesselSituationChange); };
}

// CelestialBodyLanding.cs:51
bool CelestialBodyLanding::IsValidVessel(Vessel* v) {
    if (v != nullptr && v->mainBody() != nullptr) {
        if (v->GetDiscoveryInfo().Level() != DiscoveryLevels::Owned) return false;
        return true;
    }
    return false;
}

// CelestialBodyLanding.cs:30
void CelestialBodyLanding::OnVesselSituationChange(HostedSit vs) {
    Vessel* host = vs.host;
    if (IsValidVessel(host) && host->mainBody() == body_ &&
        (vs.from == Vessel_Situations::FLYING || vs.from == Vessel_Situations::SUB_ORBITAL || vs.from == Vessel_Situations::SPLASHED) &&
        vs.to == Vessel_Situations::LANDED &&
        (!(body_ == HomeBody()) || host->landedAt.empty() ||
         (host->landedAt.find("Runway") == std::string::npos && host->landedAt.find("LaunchPad") == std::string::npos))) {
        CrewSensitiveComplete(host);
        if (!IsComplete()) {
            firstVessel_ = VesselRef::FromVessel(host);
            firstCrew_ = CrewRef::FromVessel(host);
            Complete();
            AwardProgressStandard(HomeOr(body_, "#autoLOC_6001943", "#autoLOC_6001944"), ProgressType::LANDING, body_);
        }
        Achieve();
        GE::VesselSituation::onLand.Fire(host, host->mainBody());
    }
}

// CelestialBodyLanding.cs:64
void CelestialBodyLanding::OnLoad(const ConfigNode& node) {
    if (node.GetNode("vessel") != nullptr) firstVessel_.Load(*node.GetNode("vessel"));
    if (node.GetNode("crew") != nullptr) firstCrew_.Load(*node.GetNode("crew"));
}

// CelestialBodyLanding.cs:76
void CelestialBodyLanding::OnSave(ConfigNode& node) {
    firstVessel_.Save(*node.AddNode("vessel"));
    if (firstCrew_.HasAny()) firstCrew_.Save(*node.AddNode("crew"));
}

// ---- CelestialBodyOrbit ---------------------------------------------------------------------------------------------------------

// CelestialBodyOrbit.cs:14
CelestialBodyOrbit::CelestialBodyOrbit(CelestialBody* cb) : ProgressNode("Orbit", false), body(cb) {
    OnDeploy = [this] { GE::onVesselSituationChange.Add(this, &CelestialBodyOrbit::onVesselSitChange); };
    OnStow = [this] { GE::onVesselSituationChange.Remove(this, &CelestialBodyOrbit::onVesselSitChange); };
}

// CelestialBodyOrbit.cs:30
void CelestialBodyOrbit::onVesselSitChange(HostedSit vs) {
    if (Owned(vs.host) && vs.host->mainBody() == body && vs.to == Vessel_Situations::ORBITING) {
        CrewSensitiveComplete(vs.host);
        if (!IsComplete()) {
            firstVessel = VesselRef::FromVessel(vs.host);
            firstCrew = CrewRef::FromVessel(vs.host);
            Complete();
            AwardProgressStandard(HomeOr(body, "#autoLOC_6001939", "#autoLOC_6001940"), ProgressType::ORBIT, body);
        }
        Achieve();
        GE::VesselSituation::onOrbit.Fire(vs.host, vs.host->mainBody());
    }
}

// CelestialBodyOrbit.cs:48
void CelestialBodyOrbit::OnSave(ConfigNode& node) {
    firstVessel.Save(*node.AddNode("vessel"));
    if (firstCrew.HasAny()) firstCrew.Save(*node.AddNode("crew"));
}

// CelestialBodyOrbit.cs:57
void CelestialBodyOrbit::OnLoad(const ConfigNode& node) {
    if (node.GetNode("vessel") != nullptr) firstVessel.Load(*node.GetNode("vessel"));
    if (node.GetNode("crew") != nullptr) firstCrew.Load(*node.GetNode("crew"));
}

// ---- CelestialBodyReturn --------------------------------------------------------------------------------------------------------

namespace {
const char* ReturnFromName(ReturnFrom l) {
    switch (l) {
        case ReturnFrom::FlyBy: return "FlyBy";
        case ReturnFrom::Orbit: return "Orbit";
        case ReturnFrom::SubOrbit: return "SubOrbit";
        case ReturnFrom::Flight: return "Flight";
        case ReturnFrom::Surface: return "Surface";
    }
    return "";
}
}  // namespace

// CelestialBodyReturn.cs:16
CelestialBodyReturn::CelestialBodyReturn(CelestialBody* cb, ReturnFrom level)
    : ProgressNode(std::string("ReturnFrom") + ReturnFromName(level), false), Level(level), body_(cb) {
    OnDeploy = [this] { GE::onVesselSituationChange.Add(this, &CelestialBodyReturn::OnVesselSituationChange); };
    OnStow = [this] { GE::onVesselSituationChange.Remove(this, &CelestialBodyReturn::OnVesselSituationChange); };
}

// CelestialBodyReturn.cs:33
void CelestialBodyReturn::OnVesselSituationChange(HostedSit vs) {
    if ((vs.to != Vessel_Situations::LANDED && vs.to != Vessel_Situations::SPLASHED) || vs.from != Vessel_Situations::FLYING ||
        !vs.host->mainBody()->isHomeWorld)
        return;
    VesselTripLog vesselTripLog = VesselTripLog::FromVessel(*vs.host);
    if (!IsComplete()) {
        firstVessel = VesselRef::FromVessel(vs.host);
        firstCrew = CrewRef::FromVessel(vs.host);
    }
    CrewSensitiveComplete(vs.host);
    switch (Level) {
        case ReturnFrom::FlyBy:
            if (vesselTripLog.Log.HasEntry(FlightLog::EntryType::Flyby, body_->name())) {
                if (!IsComplete()) {
                    Complete();
                    AwardProgressStandard(Fmt("#autoLOC_295665", body_->displayName()), ProgressType::FLYBYRETURN, body_);
                }
                Achieve();
                GE::VesselSituation::onReturnFromOrbit.Fire(vs.host, body_);
            }
            break;
        case ReturnFrom::Orbit:
            if (vesselTripLog.Log.HasEntry(FlightLog::EntryType::Orbit, body_->name())) {
                if (!IsComplete()) {
                    Complete();
                    AwardProgressStandard(HomeOr(body_, "#autoLOC_6001947", "#autoLOC_6001948"), ProgressType::ORBITRETURN, body_);
                }
                Achieve();
                GE::VesselSituation::onReturnFromOrbit.Fire(vs.host, body_);
            }
            break;
        case ReturnFrom::Surface:
            if (vesselTripLog.Log.HasEntry(FlightLog::EntryType::Land, body_->name())) {
                if (!IsComplete()) {
                    Complete();
                    AwardProgressStandard(Fmt("#autoLOC_295693", body_->displayName()), ProgressType::LANDINGRETURN, body_);
                }
                Achieve();
                GE::VesselSituation::onReturnFromSurface.Fire(vs.host, body_);
            }
            break;
        case ReturnFrom::SubOrbit:
        case ReturnFrom::Flight: break;
    }
}

// CelestialBodyReturn.cs:93
void CelestialBodyReturn::OnLoad(const ConfigNode& node) {
    if (node.GetNode("vessel") != nullptr) firstVessel.Load(*node.GetNode("vessel"));
    if (node.GetNode("crew") != nullptr) firstCrew.Load(*node.GetNode("crew"));
}

// CelestialBodyReturn.cs:105
void CelestialBodyReturn::OnSave(ConfigNode& node) {
    firstVessel.Save(*node.AddNode("vessel"));
    if (firstCrew.HasAny()) firstCrew.Save(*node.AddNode("crew"));
}

// ---- CelestialBodyScience -------------------------------------------------------------------------------------------------------

// CelestialBodyScience.cs:14
CelestialBodyScience::CelestialBodyScience(CelestialBody* cb) : ProgressNode("Science", false), body_(cb) {
    OnDeploy = [this] {
        GE::OnScienceRecieved.Add(this, &CelestialBodyScience::OnScience);
        GE::OnTriggeredDataTransmission.Add(this, &CelestialBodyScience::OnTriggeredScience);
    };
    OnStow = [this] {
        GE::OnScienceRecieved.Remove(this, &CelestialBodyScience::OnScience);
        GE::OnTriggeredDataTransmission.Remove(this, &CelestialBodyScience::OnTriggeredScience);
    };
}

// CelestialBodyScience.cs:32
void CelestialBodyScience::OnScience(float science, ScienceSubject* subject, ProtoVessel* pv, bool reverseEngineered) {
    if (pv == nullptr || reverseEngineered) return;
    DiscoveryLevels discoveryLevels = DiscoveryLevels::Unowned;
    if (pv->discoveryInfo != nullptr && pv->discoveryInfo->HasValue("state"))
        discoveryLevels = static_cast<DiscoveryLevels>(dotnet::ParseInt32(*pv->discoveryInfo->GetValue("state")));
    const bool fromBody = subjectIsFromBody && subjectIsFromBody(subject, body_);
    if (discoveryLevels == DiscoveryLevels::Owned && fromBody && science > 0.0f) {
        CrewSensitiveComplete(pv);
        if (!IsComplete()) {
            firstVessel_.Name = pv->vesselName;
            firstVessel_.FlagURL = pv->protoPartSnapshots[0]->flagURL;
            firstCrew_ = CrewRef::FromProtoVessel(pv);
            Complete();
            AwardProgressStandard(HomeOr(body_, "#autoLOC_6001949", "#autoLOC_6001950"), ProgressType::SCIENCE, body_);
        }
        Achieve();
    }
}

// CelestialBodyScience.cs:62
void CelestialBodyScience::OnTriggeredScience(ScienceData* data, Vessel* origin, bool xmitAborted) {
    const float amount = (data != nullptr && dataAmount) ? dataAmount(data) : 0.0f;
    if (!(data == nullptr || origin == nullptr || origin->mainBody() == nullptr || xmitAborted) && Owned(origin) && !(origin->mainBody() != body_) &&
        amount > 0.0f) {
        CrewSensitiveComplete(origin);
        if (!IsComplete()) {
            firstVessel_ = VesselRef::FromVessel(origin);
            firstCrew_ = CrewRef::FromVessel(origin);
            Complete();
            AwardProgressStandard(HomeOr(body_, "#autoLOC_6001951", "#autoLOC_6001952"), ProgressType::SCIENCE, body_);
        }
        Achieve();
    }
}

// CelestialBodyScience.cs:78
void CelestialBodyScience::OnLoad(const ConfigNode& node) {
    if (node.GetNode("vessel") != nullptr) firstVessel_.Load(*node.GetNode("vessel"));
    if (node.GetNode("crew") != nullptr) firstCrew_.Load(*node.GetNode("crew"));
}

// CelestialBodyScience.cs:90
void CelestialBodyScience::OnSave(ConfigNode& node) {
    firstVessel_.Save(*node.AddNode("vessel"));
    if (firstCrew_.HasAny()) firstCrew_.Save(*node.AddNode("crew"));
}

// ---- CelestialBodySplashdown ------------------------------------------------------------------------------------------------------

// CelestialBodySplashdown.cs:10
CelestialBodySplashdown::CelestialBodySplashdown(CelestialBody* cb) : ProgressNode("Splashdown", false), body_(cb) {
    OnDeploy = [this] { GE::onVesselSituationChange.Add(this, &CelestialBodySplashdown::OnVesselSituationChange); };
    OnStow = [this] { GE::onVesselSituationChange.Remove(this, &CelestialBodySplashdown::OnVesselSituationChange); };
}

// CelestialBodySplashdown.cs:42
bool CelestialBodySplashdown::IsValidVessel(Vessel* v) {
    if (v != nullptr && v->mainBody() != nullptr) {
        if (v->GetDiscoveryInfo().Level() != DiscoveryLevels::Owned) return false;
        return true;
    }
    return false;
}

// CelestialBodySplashdown.cs:24
void CelestialBodySplashdown::OnVesselSituationChange(HostedSit vs) {
    Vessel* host = vs.host;
    if (IsValidVessel(host) && host->mainBody() == body_ &&
        (vs.from == Vessel_Situations::FLYING || vs.from == Vessel_Situations::SUB_ORBITAL || vs.from == Vessel_Situations::LANDED) &&
        vs.to == Vessel_Situations::SPLASHED) {
        CrewSensitiveComplete(host);
        if (!IsComplete()) {
            Complete();
            AwardProgressStandard(HomeOr(body_, "#autoLOC_6001945", "#autoLOC_6001946"), ProgressType::SPLASHDOWN, body_);
        }
        Achieve();
        GE::VesselSituation::onLand.Fire(host, host->mainBody());
    }
}

// ---- CelestialBodySuborbit ---------------------------------------------------------------------------------------------------------

// CelestialBodySuborbit.cs:14
CelestialBodySuborbit::CelestialBodySuborbit(CelestialBody* cb) : ProgressNode("Suborbit", false), body(cb) {
    OnDeploy = [this] { GE::onVesselSituationChange.Add(this, &CelestialBodySuborbit::onVesselSitChange); };
    OnStow = [this] { GE::onVesselSituationChange.Remove(this, &CelestialBodySuborbit::onVesselSitChange); };
}

// CelestialBodySuborbit.cs:30
void CelestialBodySuborbit::onVesselSitChange(HostedSit vs) {
    if (Owned(vs.host) && vs.host->mainBody() == body &&
        (vs.from == Vessel_Situations::SPLASHED || vs.from == Vessel_Situations::LANDED || vs.from == Vessel_Situations::FLYING ||
         vs.from == Vessel_Situations::ORBITING) &&
        vs.to == Vessel_Situations::SUB_ORBITAL) {
        CrewSensitiveComplete(vs.host);
        if (!IsComplete()) {
            firstVessel = VesselRef::FromVessel(vs.host);
            firstCrew = CrewRef::FromVessel(vs.host);
            Complete();
            AwardProgressStandard(HomeOr(body, "#autoLOC_6001941", "#autoLOC_6001942"), ProgressType::SUBORBIT, body);
        }
        Achieve();
    }
}

// CelestialBodySuborbit.cs:46
void CelestialBodySuborbit::OnSave(ConfigNode& node) {
    firstVessel.Save(*node.AddNode("vessel"));
    if (firstCrew.HasAny()) firstCrew.Save(*node.AddNode("crew"));
}

// CelestialBodySuborbit.cs:55
void CelestialBodySuborbit::OnLoad(const ConfigNode& node) {
    if (node.GetNode("vessel") != nullptr) firstVessel.Load(*node.GetNode("vessel"));
    if (node.GetNode("crew") != nullptr) firstCrew.Load(*node.GetNode("crew"));
}

// ---- CelestialBodyTransfer ----------------------------------------------------------------------------------------------------------

// CelestialBodyTransfer.cs:10
CelestialBodyTransfer::CelestialBodyTransfer(CelestialBody* cb) : ProgressNode("CelestialBodyTransfer", false), body_(cb) {
    OnDeploy = [this] { GE::onCrewTransferred.Add(this, &CelestialBodyTransfer::OnCrewTransfer); };
    OnStow = [this] { GE::onCrewTransferred.Remove(this, &CelestialBodyTransfer::OnCrewTransfer); };
}

// CelestialBodyTransfer.cs:24
void CelestialBodyTransfer::OnCrewTransfer(GE::HostedFromToAction<ProtoCrewMember*, Part*> fp) {
    // HostedFromToAction<ProtoCrewMember, Part>: host = the crew member, from / to = the two Parts
    if (!(fp.from == nullptr) && !(fp.to == nullptr) && fp.from->fields.missionID != fp.to->fields.missionID && fp.to->vessel->mainBody() == body_) {
        if (!IsComplete()) {
            Complete();
            AwardProgressStandard(Fmt("#autoLOC_296191", body_->displayName()), ProgressType::CREWTRANSFER, body_);
        }
        Achieve();
    }
}

// CelestialBodyTransfer.cs:37
void CelestialBodyTransfer::OnLoad(const ConfigNode&) {}

// CelestialBodyTransfer.cs:41
void CelestialBodyTransfer::OnSave(ConfigNode&) {}

// ---- CrewRecovery ---------------------------------------------------------------------------------------------------------------------

// CrewRecovery.cs:10
CrewRecovery::CrewRecovery() : ProgressNode("FirstCrewToSurvive", false) {
    OnDeploy = [this] { GE::onVesselRecovered.Add(this, &CrewRecovery::onVesselRecovered); };
    OnStow = [this] { GE::onVesselRecovered.Remove(this, &CrewRecovery::onVesselRecovered); };
}

// CrewRecovery.cs:24
void CrewRecovery::onVesselRecovered(ProtoVessel* pv, bool quick) {
    (void)quick;
    if (pv->situation != Vessel_Situations::PRELAUNCH && !pv->GetVesselCrew().empty()) {
        if (!IsComplete()) {
            firstCrew_ = CrewRef::FromProtoVessel(pv);
            Complete();
            AwardProgressStandard(Localizer::Format("#autoLOC_296238"), ProgressType::CREWRECOVERY);
        }
        Achieve();
    }
}

// CrewRecovery.cs:38
void CrewRecovery::OnLoad(const ConfigNode& node) {
    if (node.GetNode("crew") != nullptr) firstCrew_.Load(*node.GetNode("crew"));
}

// CrewRecovery.cs:46
void CrewRecovery::OnSave(ConfigNode& node) { firstCrew_.Save(*node.AddNode("crew")); }

// ---- Docking -----------------------------------------------------------------------------------------------------------------------------

// Docking.cs:10
Docking::Docking(CelestialBody* cb) : ProgressNode("Docking", false), body_(cb) {
    OnDeploy = [this] { GE::onPartCouple.Add(this, &Docking::OnPartsCoupled); };
    OnStow = [this] { GE::onPartCouple.Remove(this, &Docking::OnPartsCoupled); };
}

// Docking.cs:24
void Docking::OnPartsCoupled(GE::FromToAction<Part*, Part*> pa) {
    if (!pa.from->vessel->isEVA() && !pa.to->vessel->isEVA() && pa.from->fields.missionID != pa.to->fields.missionID &&
        pa.from->vessel->mainBody() == body_) {
        CrewSensitiveComplete(pa.from->vessel);
        if (!IsComplete()) {
            Complete();
            AwardProgressStandard(Fmt("#autoLOC_296294", body_->displayName()), ProgressType::DOCKING, body_);
        }
        Achieve();
    }
}

// ---- FlagPlant -----------------------------------------------------------------------------------------------------------------------------

// FlagPlant.cs:12
FlagPlant::FlagPlant(CelestialBody* cb) : ProgressNode("FlagPlant", false), body_(cb) {
    OnDeploy = [this] { GE::onFlagPlant.Add(this, &FlagPlant::OnFlagPlant); };
    OnStow = [this] { GE::onFlagPlant.Remove(this, &FlagPlant::OnFlagPlant); };
}

// FlagPlant.cs:27 (v.orbit.referenceBody == Vessel.mainBody)
void FlagPlant::OnFlagPlant(Vessel* v) {
    if (v->mainBody() == body_) {
        if (!IsComplete()) {
            firstKerbal_ = CrewRef::FromVessel(v);
            Complete();
            AwardProgressStandard(HomeOr(body_, "#autoLOC_6001953", "#autoLOC_6001954"), ProgressType::FLAGPLANT, body_);
        }
        Achieve();
    }
}

// FlagPlant.cs:41
void FlagPlant::OnLoad(const ConfigNode& node) {
    if (node.GetNode("crew") != nullptr) firstKerbal_.Load(*node.GetNode("crew"));
}

// FlagPlant.cs:49
void FlagPlant::OnSave(ConfigNode& node) {
    if (firstKerbal_.HasAny()) firstKerbal_.Save(*node.AddNode("crew"));
}

// ---- PointOfInterest ---------------------------------------------------------------------------------------------------------------------------

// PointOfInterest.cs:23
PointOfInterest::PointOfInterest(std::string body, std::string name, std::string text, bool uplifting)
    : ProgressNode("POI" + body + name, false), body_(body), name_(name), text_(std::move(text)), uplifting_(uplifting), launchSite_(false) {
    OnDeploy = [this] { GE::OnPOIRangeEntered.Add(this, &PointOfInterest::OnAnomalyLoaded); };
    OnStow = [this] { GE::OnPOIRangeEntered.Remove(this, &PointOfInterest::OnAnomalyLoaded); };
}

// PointOfInterest.cs:43
PointOfInterest::PointOfInterest(std::string body, std::string name, std::string text, bool uplifting, bool launchSite)
    : PointOfInterest(std::move(body), name, std::move(text), uplifting) {
    name_ = name;
    launchSite_ = launchSite;
}

// PointOfInterest.cs:50
void PointOfInterest::OnAnomalyLoaded(CelestialBody* anomalyBody, std::string anomalyName) {
    if (anomalyBody->name() != body_ || anomalyName != name_ || !FlightRegistry::ready) return;
    Vessel* activeVessel = FlightRegistry::ActiveVessel;
    if (activeVessel == nullptr || !activeVessel->isCommandable() || !Owned(activeVessel)) return;
    if (!IsComplete()) {
        firstVessel_ = VesselRef::FromVessel(activeVessel);
        firstCrew_ = CrewRef::FromVessel(activeVessel);
        Complete();
        CelestialBody* celestialBody = nullptr;
        const auto& bodies = FlightRegistry::Bodies();
        int count = static_cast<int>(bodies.size());
        while (count-- > 0) {
            if (bodies[static_cast<size_t>(count)]->name() == body_) {
                celestialBody = bodies[static_cast<size_t>(count)];
                break;
            }
        }
        CareerEnvironment& env = CareerEnvironment::Current();
        auto message = [&](const std::string& title, const std::string& msg) {
            if (env.postMessage) env.postMessage(title, msg);
        };
        if (launchSite_) {
            bool flag = false;
            std::string text, text2;
            std::string launchSiteFoundName;
            if (env.launchSites) {
                const auto sites = env.launchSites();
                for (size_t i = 0; i < sites.size(); ++i) {
                    if (sites[i].requiresPOIVisit && sites[i].name == name_) {
                        text = Localizer::Format(sites[i].launchSiteName);
                        flag = true;
                        text2 = Localizer::Format(sites[i].editorFacilityDisplay);
                        launchSiteFoundName = sites[i].name;
                        break;
                    }
                }
            }
            if (flag) {
                // ScreenMessages.PostScreenMessage(...) is GUI (dropped)
                message(Localizer::Format("#autoLOC_6006122"), Localizer::Format("#autoLOC_6006121", {text, text2}));
                if (env.launchSiteFound && !launchSiteFoundName.empty()) env.launchSiteFound(launchSiteFoundName);
            } else {
                message(Localizer::Format("#autoLOC_8003616"), Localizer::Format("#autoLOC_296488", {text_, celestialBody->displayName()}));
            }
        } else {
            if (uplifting_) {
                const int pQSCitySeed = pqsCitySeed ? pqsCitySeed(anomalyName, anomalyBody->bodyName) : 0;
                std::string text3 = Localizer::Format("#autoLOC_296488", {text_, celestialBody->displayName()});
                text3 += GetUpliftText(pQSCitySeed);
                AwardProgressRandomTech(text3, pQSCitySeed);
            } else {
                AwardProgressStandard(Localizer::Format("#autoLOC_296494", {text_, celestialBody->displayName()}), ProgressType::POINTOFINTEREST,
                                      celestialBody);
            }
            if (CurrentGame()->Mode != Game::Modes::CAREER)
                message(Localizer::Format("#autoLOC_8003616"), Localizer::Format("#autoLOC_296488", {text_, celestialBody->displayName()}));
        }
    }
    Achieve();
}

// PointOfInterest.cs:136
void PointOfInterest::OnLoad(const ConfigNode& node) {
    if (node.GetNode("vessel") != nullptr) firstVessel_.Load(*node.GetNode("vessel"));
    if (node.GetNode("crew") != nullptr) firstCrew_.Load(*node.GetNode("crew"));
}

// PointOfInterest.cs:148
void PointOfInterest::OnSave(ConfigNode& node) {
    firstVessel_.Save(*node.AddNode("vessel"));
    if (firstCrew_.HasAny()) firstCrew_.Save(*node.AddNode("crew"));
}

// PointOfInterest.cs:157
std::string PointOfInterest::GetUpliftText(int seed) {
    static const char* tags[] = {"#autoLOC_296522", "#autoLOC_296524", "#autoLOC_296526", "#autoLOC_296528", "#autoLOC_296530",
                                 "#autoLOC_296532", "#autoLOC_296534", "#autoLOC_296536", "#autoLOC_296538"};
    const int n = KSPRandom(seed).Next(10);
    if (n >= 0 && n <= 8) return Localizer::Format(tags[n]);
    return Localizer::Format("#autoLOC_296540");
}

// ---- ReachSpace --------------------------------------------------------------------------------------------------------------------------------

// ReachSpace.cs:12
ReachSpace::ReachSpace() : ProgressNode("ReachedSpace", false) {
    OnDeploy = [this] { GE::onVesselSituationChange.Add(this, &ReachSpace::OnVesselSituationChange); };
    OnStow = [this] { GE::onVesselSituationChange.Remove(this, &ReachSpace::OnVesselSituationChange); };
}

// ReachSpace.cs:27
void ReachSpace::OnVesselSituationChange(HostedSit vs) {
    if (Owned(vs.host) && vs.from == Vessel_Situations::FLYING && vs.to == Vessel_Situations::SUB_ORBITAL) {
        CrewSensitiveComplete(vs.host);
        if (!IsComplete()) {
            firstVessel = VesselRef::FromVessel(vs.host);
            firstCrew = CrewRef::FromVessel(vs.host);
            Complete();
            AwardProgressStandard(Localizer::Format("#autoLOC_297725"), ProgressType::REACHSPACE);
        }
        Achieve();
        GE::VesselSituation::onReachSpace.Fire(vs.host);
    }
}

// ReachSpace.cs:44
void ReachSpace::OnSave(ConfigNode& node) {
    firstVessel.Save(*node.AddNode("vessel"));
    if (firstCrew.HasAny()) firstCrew.Save(*node.AddNode("crew"));
}

// ReachSpace.cs:53
void ReachSpace::OnLoad(const ConfigNode& node) {
    if (node.GetNode("vessel") != nullptr) {
        firstVessel = VesselRef();
        firstVessel.Load(*node.GetNode("vessel"));
    }
    if (node.GetNode("crew") != nullptr) {
        firstCrew = CrewRef();
        firstCrew.Load(*node.GetNode("crew"));
    }
}

// ---- Records* --------------------------------------------------------------------------------------------------------------------------------------

namespace {
double FindNextRecord(double cur, double max, double round, int& interval) {
    CareerEnvironment& env = CareerEnvironment::Current();
    return env.findNextRecord ? env.findNextRecord(cur, max, round, interval) : max;
}
int RecordSplit() {
    CareerEnvironment& env = CareerEnvironment::Current();
    return env.progressionRecordSplit ? env.progressionRecordSplit() : 1;
}
bool ContractDefsPresent() {
    CareerEnvironment& env = CareerEnvironment::Current();
    return env.contractDefsPresent && env.contractDefsPresent();
}
std::string ValueAndUnits(int type, double v) {
    CareerEnvironment& env = CareerEnvironment::Current();
    return env.valueAndUnits ? env.valueAndUnits(type, v) : dotnet::FormatDoubleG(v);
}
}  // namespace

// RecordsAltitude.cs:19
double RecordsAltitude::maxAltitude() {
    if (atmosphereDepth_ >= 0.0) return atmosphereDepth_;
    CelestialBody* homeBody = HomeBody();
    if (homeBody == nullptr) return 70000.0;
    atmosphereDepth_ = homeBody->atmosphereDepth;
    return atmosphereDepth_;
}

// RecordsAltitude.cs:37
RecordsAltitude::RecordsAltitude() : ProgressNode("RecordsAltitude", false) { OnIterateVessels = [this](Vessel* v) { iterateVessels(v); }; }

// RecordsAltitude.cs:43
void RecordsAltitude::iterateVessels(Vessel* v) {
    if (v == nullptr || v != FlightRegistry::ActiveVessel || !v->isCommandable() || v->situation == Vessel_Situations::PRELAUNCH || !Owned(v) ||
        !(v->mainBody()->name() == Planetarium::fetch->Home->name()))
        return;
    if (v->altitude > record) {
        if (v->altitude > maxAltitude())
            record = maxAltitude();
        else
            record = v->altitude;
        if (recordHolder_ != v) {
            Reach();
            recordHolder_ = v;
        } else {
            AchieveDate = SaveUniversalTime();
            Achieve();
        }
    }
    if (!IsComplete()) {
        if (rewardThreshold_ <= 0.0) rewardThreshold_ = FindNextRecord(0.0, maxAltitude(), 500.0, rewardInterval_);
        while (record >= rewardThreshold_) {
            AwardProgressInterval(Fmt("#autoLOC_297833", ValueAndUnits(1, rewardThreshold_)), rewardInterval_, RecordSplit(), ProgressType::ALTITUDERECORD);
            if (rewardInterval_ >= RecordSplit() || rewardThreshold_ >= maxAltitude()) break;
            rewardThreshold_ = FindNextRecord(rewardThreshold_, maxAltitude(), 500.0, rewardInterval_);
        }
    }
    if (v->altitude >= maxAltitude()) {
        Complete();
        OnIterateVessels = nullptr;
    }
}

// RecordsAltitude.cs:93
void RecordsAltitude::OnLoad(const ConfigNode& node) {
    if (node.HasValue("record")) record = dotnet::ParseDouble(*node.GetValue("record"));
    if (IsComplete())
        OnIterateVessels = nullptr;
    else
        rewardThreshold_ = FindNextRecord(record, maxAltitude(), 500.0, rewardInterval_);
}

// RecordsAltitude.cs:109
void RecordsAltitude::OnSave(ConfigNode& node) { node.AddValue("record", record); }

// RecordsDepth.cs:20
double RecordsDepth::maxDepth() {
    if (depthCeiling_ >= 0.0) return depthCeiling_;
    CareerEnvironment& env = CareerEnvironment::Current();
    if (!ContractDefsPresent()) return 750.0;
    depthCeiling_ = env.progressionMaxDepthRecord ? env.progressionMaxDepthRecord() : 750.0;
    return depthCeiling_;
}

// RecordsDepth.cs:37
RecordsDepth::RecordsDepth() : ProgressNode("RecordsDepth", false) { OnIterateVessels = [this](Vessel* v) { iterateVessels(v); }; }

// RecordsDepth.cs:43
void RecordsDepth::iterateVessels(Vessel* v) {
    if (v == nullptr || v != FlightRegistry::ActiveVessel || !v->isCommandable() || !Owned(v) ||
        v->mainBody()->name() != Planetarium::fetch->Home->name() || v->situation != Vessel_Situations::SPLASHED || !(v->altitude <= 0.0))
        return;
    const double num = std::fabs(v->altitude);
    if (num > record) {
        if (num > maxDepth())
            record = maxDepth();
        else
            record = num;
        if (recordHolder_ != v) {
            Reach();
            recordHolder_ = v;
        } else {
            AchieveDate = SaveUniversalTime();
            Achieve();
        }
    }
    if (!IsComplete()) {
        if (rewardThreshold_ <= 0.0) rewardThreshold_ = FindNextRecord(0.0, maxDepth(), 10.0, rewardInterval_);
        while (record >= rewardThreshold_) {
            AwardProgressInterval(Fmt("#autoLOC_297943", ValueAndUnits(2, rewardThreshold_)), rewardInterval_, RecordSplit(), ProgressType::DEPTHRECORD);
            if (rewardInterval_ >= RecordSplit() || rewardThreshold_ >= maxDepth()) break;
            rewardThreshold_ = FindNextRecord(rewardThreshold_, maxDepth(), 10.0, rewardInterval_);
        }
    }
    if (num >= maxDepth()) {
        Complete();
        OnIterateVessels = nullptr;
    }
}

// RecordsDepth.cs:94
void RecordsDepth::OnLoad(const ConfigNode& node) {
    if (node.HasValue("record")) record = dotnet::ParseDouble(*node.GetValue("record"));
    if (IsComplete())
        OnIterateVessels = nullptr;
    else
        rewardThreshold_ = FindNextRecord(record, maxDepth(), 10.0, rewardInterval_);
}

// RecordsDepth.cs:110
void RecordsDepth::OnSave(ConfigNode& node) { node.AddValue("record", record); }

// RecordsDistance.cs:20
double RecordsDistance::maxDistance() {
    if (distanceCeiling_ >= 0.0) return distanceCeiling_;
    CareerEnvironment& env = CareerEnvironment::Current();
    if (!ContractDefsPresent()) return 100000.0;
    distanceCeiling_ = env.progressionMaxDistanceRecord ? env.progressionMaxDistanceRecord() : 100000.0;
    return distanceCeiling_;
}

// RecordsDistance.cs:37
RecordsDistance::RecordsDistance() : ProgressNode("RecordsDistance", false) { OnIterateVessels = [this](Vessel* v) { iterateVessels(v); }; }

// RecordsDistance.cs:43
void RecordsDistance::iterateVessels(Vessel* v) {
    CareerEnvironment& env = CareerEnvironment::Current();
    CelestialBody* kscBody = env.spaceCenterBody ? env.spaceCenterBody() : nullptr;
    if (v == nullptr || v != FlightRegistry::ActiveVessel || !v->isCommandable() || v->situation == Vessel_Situations::PRELAUNCH || !Owned(v) ||
        !env.spaceCenterBody || kscBody == nullptr)
        return;
    bool flag = false;
    double num = 0.0;
    if ((v->launchedFrom == "" || v->launchedFrom == "Runway" || v->launchedFrom == "LaunchPad") && v->mainBody()->name() == kscBody->name()) {
        num = env.spaceCenterGreatCircleDistance ? env.spaceCenterGreatCircleDistance(v->latitude, v->longitude) : 0.0;
        flag = true;
    } else if (v->launchedFrom != "" && SaveEnvironment::Current().IsExpansionInstalled("MakingHistory") && env.isStockLaunchSite &&
               env.isStockLaunchSite(v->launchedFrom)) {
        CelestialBody* siteBody = nullptr;
        double distance = 0.0;
        if (env.launchSiteDistance && env.launchSiteDistance(v->launchedFrom, siteBody, v->latitude, v->longitude, distance) && siteBody != nullptr &&
            v->mainBody()->name() == siteBody->name()) {
            num = distance;
            flag = true;
        }
    }
    if (!flag) return;
    if (num > record) {
        if (num > maxDistance())
            record = maxDistance();
        else
            record = num;
        if (recordHolder_ != v) {
            Reach();
            recordHolder_ = v;
        } else {
            AchieveDate = SaveUniversalTime();
            Achieve();
        }
    }
    if (!IsComplete()) {
        if (rewardThreshold_ <= 0.0) rewardThreshold_ = FindNextRecord(0.0, maxDistance(), 1000.0, rewardInterval_);
        while (record >= rewardThreshold_) {
            AwardProgressInterval(Fmt("#autoLOC_298052", ValueAndUnits(3, rewardThreshold_)), rewardInterval_, RecordSplit(), ProgressType::DISTANCERECORD);
            if (rewardInterval_ >= RecordSplit() || rewardThreshold_ >= maxDistance()) break;
            rewardThreshold_ = FindNextRecord(rewardThreshold_, maxDistance(), 1000.0, rewardInterval_);
        }
    }
    if (num >= maxDistance()) {
        Complete();
        OnIterateVessels = nullptr;
    }
}

// RecordsDistance.cs:113
void RecordsDistance::OnLoad(const ConfigNode& node) {
    if (node.HasValue("record")) record = dotnet::ParseDouble(*node.GetValue("record"));
    if (IsComplete())
        OnIterateVessels = nullptr;
    else
        rewardThreshold_ = FindNextRecord(record, maxDistance(), 1000.0, rewardInterval_);
}

// RecordsDistance.cs:129
void RecordsDistance::OnSave(ConfigNode& node) { node.AddValue("record", record); }

// RecordsSpeed.cs:19
double RecordsSpeed::maxSpeed() {
    if (speedCeiling_ >= 0.0) return speedCeiling_;
    CareerEnvironment& env = CareerEnvironment::Current();
    if (!ContractDefsPresent()) return 2500.0;
    speedCeiling_ = env.progressionMaxSpeedRecord ? env.progressionMaxSpeedRecord() : 2500.0;
    return speedCeiling_;
}

// RecordsSpeed.cs:36
RecordsSpeed::RecordsSpeed() : ProgressNode("RecordsSpeed", false) { OnIterateVessels = [this](Vessel* v) { iterateVessels(v); }; }

// RecordsSpeed.cs:42
void RecordsSpeed::iterateVessels(Vessel* v) {
    if (v == nullptr || v != FlightRegistry::ActiveVessel || !v->isCommandable() || !Owned(v) ||
        !(v->mainBody()->name() == Planetarium::fetch->Home->name()))
        return;
    double num = 0.0;
    switch (v->situation) {
        default: num = v->obt_speed; break;
        case Vessel_Situations::PRELAUNCH: return;
        case Vessel_Situations::LANDED:
        case Vessel_Situations::SPLASHED:
        case Vessel_Situations::FLYING: num = v->srfSpeed; break;
    }
    if (num > record) {
        if (num > maxSpeed())
            record = maxSpeed();
        else
            record = num;
        if (recordHolder_ != v) {
            Reach();
            recordHolder_ = v;
        } else {
            AchieveDate = SaveUniversalTime();
            Achieve();
        }
    }
    if (!IsComplete()) {
        if (rewardThreshold_ <= 0.0) rewardThreshold_ = FindNextRecord(0.0, maxSpeed(), 5.0, rewardInterval_);
        while (record >= rewardThreshold_) {
            AwardProgressInterval(Fmt("#autoLOC_298174", ValueAndUnits(4, rewardThreshold_)), rewardInterval_, RecordSplit(), ProgressType::SPEEDRECORD);
            if (rewardInterval_ >= RecordSplit() || rewardThreshold_ >= maxSpeed()) break;
            rewardThreshold_ = FindNextRecord(rewardThreshold_, maxSpeed(), 5.0, rewardInterval_);
        }
    }
    if (num >= maxSpeed()) {
        Complete();
        OnIterateVessels = nullptr;
    }
}

// RecordsSpeed.cs:106
void RecordsSpeed::OnLoad(const ConfigNode& node) {
    if (node.HasValue("record")) record = dotnet::ParseDouble(*node.GetValue("record"));
    if (IsComplete())
        OnIterateVessels = nullptr;
    else
        rewardThreshold_ = FindNextRecord(record, maxSpeed(), 5.0, rewardInterval_);
}

// RecordsSpeed.cs:122
void RecordsSpeed::OnSave(ConfigNode& node) { node.AddValue("record", record); }

// ---- Rendezvous --------------------------------------------------------------------------------------------------------------------------------------

// Rendezvous.cs:14
Rendezvous::Rendezvous(CelestialBody* cb) : ProgressNode("Rendezvous", false), body_(cb) {
    OnDeploy = [this] { GE::onVesselLoaded.Add(this, &Rendezvous::OnVesselLoaded); };
    OnStow = [this] { GE::onVesselLoaded.Remove(this, &Rendezvous::OnVesselLoaded); };
}

// Rendezvous.cs:28
void Rendezvous::OnVesselLoaded(Vessel* v) {
    Vessel* active = FlightRegistry::ActiveVessel;
    if (!(v == active) && !(v->mainBody() != body_) && !v->isEVA() && !active->isEVA() &&
        (v->situation == Vessel_Situations::ORBITING || v->situation == Vessel_Situations::ESCAPING) &&
        v->rootPart->fields.missionID != active->rootPart->fields.missionID)
        CareerEnvironment::Current().Scheduler().StartCoroutine(TrackCandidateVessel(v, active));
}

// Rendezvous.cs:36
Enumerator Rendezvous::TrackCandidateVessel(Vessel* candidate, Vessel* reference) {
    log::Info("[Rendezvous]: Now tracking " + candidate->GetDisplayName() + " vs. " + reference->GetDisplayName());
    // (candidate != null / reference != null: the C# objects may be destroyed while tracked; here a destroyed Vessel is a dangling
    // pointer, so the host must stop the coroutine when it destroys one)
    while (!IsComplete() && candidate != nullptr && candidate->loaded && reference != nullptr && reference->loaded) {
        if ((candidate->obt_velocity - reference->obt_velocity).sqrMagnitude() < maxClosingSpeed_ * maxClosingSpeed_) {
            CrewSensitiveComplete(reference);
            if (!IsComplete()) {
                Complete();
                AwardProgressStandard(Fmt("#autoLOC_298266", body_->displayName()), ProgressType::RENDEZVOUS, body_);
            }
            Achieve();
        }
        co_yield YieldInstruction::Null();
    }
    log::Info("[Rendezvous]: Tracking " + candidate->GetDisplayName() + " vs. " + reference->GetDisplayName() + " ended.");
}

// ---- Spacewalk / StationConstruction / SurfaceEVA ----------------------------------------------------------------------------------------------------

// Spacewalk.cs:12
Spacewalk::Spacewalk(CelestialBody* cb) : ProgressNode("Spacewalk", false), body_(cb) {
    OnDeploy = [this] { GE::onCrewOnEva.Add(this, &Spacewalk::OnGoEVA); };
    OnStow = [this] { GE::onCrewOnEva.Remove(this, &Spacewalk::OnGoEVA); };
}

// Spacewalk.cs:27
void Spacewalk::OnGoEVA(GE::FromToAction<Part*, Part*> fv) {
    if ((fv.from->vessel->situation == Vessel_Situations::ORBITING || fv.from->vessel->situation == Vessel_Situations::SUB_ORBITAL) &&
        fv.from->vessel->mainBody() == body_) {
        if (!IsComplete()) {
            firstKerbal_ = CrewRef::FromVessel(fv.to->vessel);
            Complete();
            AwardProgressStandard(Fmt("#autoLOC_298311", body_->displayName()), ProgressType::SPACEWALK, body_);
        }
        Achieve();
    }
}

// Spacewalk.cs:41
void Spacewalk::OnLoad(const ConfigNode& node) {
    if (node.GetNode("crew") != nullptr) firstKerbal_.Load(*node.GetNode("crew"));
}

// Spacewalk.cs:49
void Spacewalk::OnSave(ConfigNode& node) {
    if (firstKerbal_.HasAny()) firstKerbal_.Save(*node.AddNode("crew"));
}

// StationConstruction.cs:12
StationConstruction::StationConstruction(CelestialBody* cb) : ProgressNode("StationConstruction", false), body_(cb) {
    OnDeploy = [this] { GE::onPartCouple.Add(this, &StationConstruction::onPartsCoupled); };
    OnStow = [this] { GE::onPartCouple.Remove(this, &StationConstruction::onPartsCoupled); };
}

// StationConstruction.cs:27
void StationConstruction::onPartsCoupled(GE::FromToAction<Part*, Part*> pa) {
    if (!pa.from->vessel->isEVA() && !pa.to->vessel->isEVA() && pa.from->fields.missionID != pa.to->fields.missionID &&
        pa.to->vessel->situation == Vessel_Situations::ORBITING && pa.to->vessel->mainBody() == body_) {
        CrewSensitiveComplete(pa.from->vessel);
        if (!IsComplete()) {
            firstStation_ = VesselRef::FromVessel(pa.to->vessel);
            Complete();
            AwardProgressStandard(Fmt("#autoLOC_298375", body_->displayName()), ProgressType::STATIONCONSTRUCTION, body_);
        }
        Achieve();
    }
}

// StationConstruction.cs:42
void StationConstruction::OnLoad(const ConfigNode& node) {
    if (node.GetNode("station") != nullptr) firstStation_.Load(*node.GetNode("station"));
}

// StationConstruction.cs:50
void StationConstruction::OnSave(ConfigNode& node) { firstStation_.Save(*node.AddNode("station")); }

// SurfaceEVA.cs:12
SurfaceEVA::SurfaceEVA(CelestialBody* cb) : ProgressNode("SurfaceEVA", false), body_(cb) {
    OnDeploy = [this] { GE::onCrewOnEva.Add(this, &SurfaceEVA::OnGoEVA); };
    OnStow = [this] { GE::onCrewOnEva.Remove(this, &SurfaceEVA::OnGoEVA); };
}

// SurfaceEVA.cs:27
void SurfaceEVA::OnGoEVA(GE::FromToAction<Part*, Part*> fv) {
    if (fv.from->vessel->LandedOrSplashed() && fv.from->vessel->mainBody() == body_) {
        if (!IsComplete()) {
            firstKerbal_ = CrewRef::FromVessel(fv.to->vessel);
            Complete();
            AwardProgressStandard(HomeOr(body_, "#autoLOC_6001955", "#autoLOC_6001956"), ProgressType::SURFACEEVA, body_);
        }
        Achieve();
    }
}

// SurfaceEVA.cs:41
void SurfaceEVA::OnLoad(const ConfigNode& node) {
    if (node.GetNode("crew") != nullptr) firstKerbal_.Load(*node.GetNode("crew"));
}

// SurfaceEVA.cs:49
void SurfaceEVA::OnSave(ConfigNode& node) {
    if (firstKerbal_.HasAny()) firstKerbal_.Save(*node.AddNode("crew"));
}

// ---- TargetedLanding ------------------------------------------------------------------------------------------------------------------------------------

// TargetedLanding.cs:12
TargetedLanding::TargetedLanding(std::string targetTag, ReturnFrom level) : ProgressNode(targetTag + "Landing", false), targetTag_(targetTag), Level_(level) {
    OnDeploy = [this] { GE::onVesselSituationChange.Add(this, &TargetedLanding::OnVesselSituationChange); };
    OnStow = [this] { GE::onVesselSituationChange.Remove(this, &TargetedLanding::OnVesselSituationChange); };
}

// TargetedLanding.cs:27
void TargetedLanding::OnVesselSituationChange(HostedSit vs) {
    if (Owned(vs.host) &&
        (vs.from == Vessel_Situations::FLYING || vs.from == Vessel_Situations::SUB_ORBITAL || vs.from == Vessel_Situations::SPLASHED) &&
        vs.to == Vessel_Situations::LANDED)
        CareerEnvironment::Current().Scheduler().StartCoroutine(WaitAndCheckLandedTag(vs.host, targetTag_));
}

// TargetedLanding.cs:35
Enumerator TargetedLanding::WaitAndCheckLandedTag(Vessel* host, std::string tgtTag) {
    co_yield YieldInstruction::WaitForEndOfFrame();
    if (host->landedAt.empty() || host->landedAt.find(tgtTag) == std::string::npos) co_return;
    VesselTripLog vesselTripLog = VesselTripLog::FromVessel(*host);
    const std::string homeName = HomeBody() != nullptr ? HomeBody()->name() : std::string();  // FlightGlobals.GetHomeBodyName()
    FlightLog::EntryType entry;
    switch (Level_) {
        case ReturnFrom::FlyBy: entry = FlightLog::EntryType::Flyby; break;
        case ReturnFrom::Orbit: entry = FlightLog::EntryType::Orbit; break;
        case ReturnFrom::SubOrbit: entry = FlightLog::EntryType::Suborbit; break;
        case ReturnFrom::Flight: entry = FlightLog::EntryType::Flight; break;
        default: co_return;
    }
    if (vesselTripLog.Log.HasEntry(entry, homeName)) {
        if (!IsComplete()) Complete();
        Achieve();
        GE::VesselSituation::onTargetedLanding.Fire(host, tgtTag, Level_);
    }
}

// ---- TowerBuzz ----------------------------------------------------------------------------------------------------------------------------------------------

// TowerBuzz.cs:20
TowerBuzz::TowerBuzz() : ProgressNode("TowerBuzz", false) {
    OnDeploy = [this] {
        GE::onFlightReady.Add(this, &TowerBuzz::OnFlightReady);
        GE::OnFlightGlobalsReady.Add(this, &TowerBuzz::OnFlightGlobalsReady);
        GE::OnKSCFacilityUpgraded.Add(this, &TowerBuzz::OnFacilityUpgrade);
    };
    OnStow = [this] {
        GE::onFlightReady.Remove(this, &TowerBuzz::OnFlightReady);
        GE::OnFlightGlobalsReady.Remove(this, &TowerBuzz::OnFlightGlobalsReady);
        GE::OnKSCFacilityUpgraded.Remove(this, &TowerBuzz::OnFacilityUpgrade);
    };
}

// TowerBuzz.cs:39
void TowerBuzz::OnFlightReady() { Initialize(); }

// TowerBuzz.cs:44
void TowerBuzz::OnFlightGlobalsReady(bool ready) {
    if (ready) Initialize();
}

// TowerBuzz.cs:52
void TowerBuzz::OnFacilityUpgrade(Upgradeables::UpgradeableFacility* facility, int level) {
    (void)level;
    if (facility->name == "SpaceplaneHangar") Initialize();
}

// TowerBuzz.cs:60
void TowerBuzz::iterateVessels(Vessel* v) {
    if (active_ && !(v == nullptr) && !(v != FlightRegistry::ActiveVessel) && v->situation == Vessel_Situations::FLYING && v->isCommandable() &&
        Owned(v) && !(v->altitude > 200.0) && v->srfSpeed >= 200.0) {
        CelestialBody* homeBody = HomeBody();
        CareerEnvironment& env = CareerEnvironment::Current();
        const double dist = env.greatCircleDistance ? env.greatCircleDistance(homeBody, latitude_, longitude_, v->latitude, v->longitude) : 0.0;
        if ((!(v->mainBody() != nullptr) || !(v->mainBody() != homeBody)) && !(dist > 50.0) && !IsComplete()) {
            firstVessel_ = VesselRef::FromVessel(v);
            firstCrew_ = CrewRef::FromVessel(v);
            Complete();
            AwardProgressStandard(Localizer::Format("#autoLOC_298640"), ProgressType::STUNT, homeBody);
            Terminate();
        }
    }
}

// TowerBuzz.cs:76
void TowerBuzz::OnLoad(const ConfigNode& node) {
    if (node.GetNode("vessel") != nullptr) firstVessel_.Load(*node.GetNode("vessel"));
    if (node.GetNode("crew") != nullptr) firstCrew_.Load(*node.GetNode("crew"));
}

// TowerBuzz.cs:88
void TowerBuzz::OnSave(ConfigNode& node) {
    firstVessel_.Save(*node.AddNode("vessel"));
    if (firstCrew_.HasAny()) firstCrew_.Save(*node.AddNode("crew"));
}

// TowerBuzz.cs:97
void TowerBuzz::SetActive(bool active) {
    if (active && !OnIterateVessels)
        OnIterateVessels = [this](Vessel* v) { iterateVessels(v); };
    else if (!active && OnIterateVessels)
        OnIterateVessels = nullptr;
    active_ = active;
}

// TowerBuzz.cs:110
void TowerBuzz::Initialize() {
    if (IsComplete()) {
        latitude_ = 0.0;
        longitude_ = 0.0;
        Terminate();
        return;
    }
    switch (Mathf::RoundToInt(f32store(fx(ScenarioUpgradeableFacilities::GetFacilityLevel(SpaceCenterFacility::SpaceplaneHangar)) *
                                       fx(static_cast<float>(ScenarioUpgradeableFacilities::GetFacilityLevelCount(SpaceCenterFacility::SpaceplaneHangar)))))) {
        case 0:
            latitude_ = 0.0;
            longitude_ = 0.0;
            SetActive(false);
            break;
        case 1:
            latitude_ = -0.0660831849602887;
            longitude_ = 285.366569132555;
            SetActive(true);
            break;
        case 2:
            latitude_ = -0.0627680294327377;
            longitude_ = 285.36674754833;
            SetActive(true);
            break;
    }
}

// TowerBuzz.cs:139
void TowerBuzz::Terminate() {
    active_ = false;
    if (OnStow) OnStow();
    OnIterateVessels = nullptr;
    OnDeploy = nullptr;
    OnStow = nullptr;
}

// ---- CelestialBodySubtree ----------------------------------------------------------------------------------------------------------------------------------------

// CelestialBodySubtree.cs:56
CelestialBodySubtree::CelestialBodySubtree(CelestialBody* body) : ProgressNode(body->name(), false), Body(body) {
    body->progressTree = this;
    ProgressTree& st = Subtree();
    orbit = st.AddNode(std::make_unique<CelestialBodyOrbit>(body));
    escape = st.AddNode(std::make_unique<CelestialBodyEscape>(body));
    landing = st.AddNode(std::make_unique<CelestialBodyLanding>(body));
    splashdown = st.AddNode(std::make_unique<CelestialBodySplashdown>(body));
    science = st.AddNode(std::make_unique<CelestialBodyScience>(body));
    rendezvous = st.AddNode(std::make_unique<Rendezvous>(body));
    docking = st.AddNode(std::make_unique<Docking>(body));
    spacewalk = st.AddNode(std::make_unique<Spacewalk>(body));
    surfaceEVA = st.AddNode(std::make_unique<SurfaceEVA>(body));
    flagPlant = st.AddNode(std::make_unique<FlagPlant>(body));
    baseConstruction = st.AddNode(std::make_unique<BaseConstruction>(body));
    stationConstruction = st.AddNode(std::make_unique<StationConstruction>(body));
    crewTransfer = st.AddNode(std::make_unique<CelestialBodyTransfer>(body));
    returnFromOrbit = st.AddNode(std::make_unique<CelestialBodyReturn>(body, ReturnFrom::Orbit));
    if (!body->isHomeWorld) {
        flight = st.AddNode(std::make_unique<CelestialBodyFlight>(body));
        suborbit = st.AddNode(std::make_unique<CelestialBodySuborbit>(body));
        flyBy = st.AddNode(std::make_unique<CelestialBodyFlyby>(body));
        returnFromFlyby = st.AddNode(std::make_unique<CelestialBodyReturn>(body, ReturnFrom::FlyBy));
        returnFromSurface = st.AddNode(std::make_unique<CelestialBodyReturn>(body, ReturnFrom::Surface));
    }
    OnDeploy = [this] {
        GE::OnProgressReached.Add(this, &CelestialBodySubtree::OnAchievementAchieve);
        Subtree().Deploy();
    };
    OnIterateVessels = [this](Vessel* v) { Subtree().IterateVessels(v); };
    OnStow = [this] {
        GE::OnProgressReached.Remove(this, &CelestialBodySubtree::OnAchievementAchieve);
        Subtree().Stow();
    };
    OnGenerateSummary = [this](const std::string& baseID) { return generateSubtreeSummary(baseID); };
}

CelestialBodySubtree::~CelestialBodySubtree() {
    if (Body != nullptr && Body->progressTree == this) Body->progressTree = nullptr;
}

// CelestialBodySubtree.cs:119
void CelestialBodySubtree::OnAchievementAchieve(ProgressNode* node) {
    if (!IsReached() && Subtree().Contains(node)) Reach();
    if (Subtree().AllComplete()) {
        if (!IsComplete()) Complete();
        Achieve();
    }
}

// CelestialBodySubtree.cs:135
void CelestialBodySubtree::OnSave(ConfigNode& node) { Subtree().Save(node); }

// CelestialBodySubtree.cs:140
void CelestialBodySubtree::OnLoad(const ConfigNode& node) { Subtree().Load(node); }

// CelestialBodySubtree.cs:145
std::string CelestialBodySubtree::generateSubtreeSummary(const std::string& baseID) {
    (void)baseID;
    return Subtree().GetTreeSummary(Body->name());
}

// CelestialBodySubtree.cs:150
void CelestialBodySubtree::LinkBodyHome(const std::vector<CelestialBodySubtree*>& trees) {
    int num = static_cast<int>(Body->orbitingBodies.size());
    if (Body->isHomeWorld) num++;
    if (num == 0) {
        childTrees.clear();
        return;
    }
    std::vector<CelestialBodySubtree*> list;
    for (size_t i = 0; i < Body->orbitingBodies.size(); ++i) {
        CelestialBody* celestialBody = Body->orbitingBodies[i];
        if (celestialBody == Body || celestialBody->isHomeWorld) continue;
        for (size_t j = 0, n = trees.size(); j < n; ++j) {
            if (trees[j]->Body == celestialBody) {
                list.push_back(trees[j]);
                trees[j]->parentTree = this;
                trees[j]->LinkBodyHome(trees);
                break;
            }
        }
    }
    if (Body->isHomeWorld) {
        for (size_t k = 0, n = trees.size(); k < n; ++k) {
            if (trees[k]->Body == Body->referenceBody()) {
                list.push_back(trees[k]);
                trees[k]->parentTree = this;
                trees[k]->LinkBodyHome(trees);
                break;
            }
        }
    }
    childTrees = list;
    // Array.Sort(childTrees, (a, b) => a.Body.scienceValues.RecoveryValue.CompareTo(b.Body.scienceValues.RecoveryValue)): the unstable
    // introsort of List.Sort is not reproduced for equal keys; with <= 16 elements Array.Sort is an insertion sort (stable)
    std::stable_sort(childTrees.begin(), childTrees.end(), [](CelestialBodySubtree* a, CelestialBodySubtree* b) {
        return a->Body->scienceValues.RecoveryValue < b->Body->scienceValues.RecoveryValue;
    });
}

// CelestialBodySubtree.cs:201
void CelestialBodySubtree::DebugBodyTree() {
    log::Info(Body->bodyName + " " + (IsReached() ? "True" : "False"));
    for (size_t i = 0; i < childTrees.size(); ++i) childTrees[i]->DebugBodyTree();
}

}  // namespace KSPAchievements

// ---- ProgressTracking.generateAchievementsTree -----------------------------------------------------------------------------------------------------------------

namespace achievements_detail {

// ProgressTracking.cs:157
void BuildTree(ProgressTracking& pt, ProgressTree& tree) {
    using namespace KSPAchievements;
    pt.firstLaunch = tree.AddNode(std::make_unique<FirstLaunch>());
    pt.firstCrewToSurvive = tree.AddNode(std::make_unique<CrewRecovery>());
    pt.towerBuzz = tree.AddNode(std::make_unique<TowerBuzz>());
    pt.altitudeRecords = tree.AddNode(std::make_unique<RecordsAltitude>());
    pt.depthRecords = tree.AddNode(std::make_unique<RecordsDepth>());
    pt.speedRecords = tree.AddNode(std::make_unique<RecordsSpeed>());
    pt.distanceRecords = tree.AddNode(std::make_unique<RecordsDistance>());
    pt.reachSpace = tree.AddNode(std::make_unique<ReachSpace>());
    pt.KSCLanding = tree.AddNode(std::make_unique<TargetedLanding>("KSC", ReturnFrom::SubOrbit));
    pt.runwayLanding = tree.AddNode(std::make_unique<TargetedLanding>("Runway", ReturnFrom::SubOrbit));
    pt.launchpadLanding = tree.AddNode(std::make_unique<TargetedLanding>("LaunchPad", ReturnFrom::SubOrbit));
// the 50 PointOfInterest nodes are constructed first (ProgressTracking.cs:182-231), then added in a different order (:233-:282)
    pt.POIKerbinKSC2 = new PointOfInterest("Kerbin", "KSC2", Localizer::Format("#autoLOC_297133"), false);
    pt.POIKerbinIslandAirfield = new PointOfInterest("Kerbin", "IslandAirfield", Localizer::Format("#autoLOC_297134"), false, true);
    pt.POIKerbinDiscoverableLaunchsite01 = new PointOfInterest("Kerbin", "Crater_Launch_Site", Localizer::Format("#autoLOC_6006117"), false, true);
    pt.POIKerbinDiscoverableLaunchsite02 = new PointOfInterest("Kerbin", "MahiMahi_Launch_Site", Localizer::Format("#autoLOC_6006118"), false, true);
    pt.POIKerbinDiscoverableLaunchsite03 = new PointOfInterest("Kerbin", "Cove_Launch_Site", Localizer::Format("#autoLOC_6006119"), false, true);
    pt.POIKerbinDiscoverableLaunchsite04 = new PointOfInterest("Kerbin", "GlacierLake_Launch_Site", Localizer::Format("#autoLOC_6006120"), false, true);
    pt.POIKerbinUFO = new PointOfInterest("Kerbin", "UFO", Localizer::Format("#autoLOC_297135"), false);
    pt.POIKerbinPyramids = new PointOfInterest("Kerbin", "Pyramids", Localizer::Format("#autoLOC_297136"), false);
    pt.POIKerbinMonolith00 = new PointOfInterest("Kerbin", "Monolith00", Localizer::Format("#autoLOC_297137"), false);
    pt.POIKerbinMonolith01 = new PointOfInterest("Kerbin", "Monolith01", Localizer::Format("#autoLOC_297138"), false);
    pt.POIKerbinMonolith02 = new PointOfInterest("Kerbin", "Monolith02", Localizer::Format("#autoLOC_297139"), false);
    pt.POIKerbinDessertAirfield = new PointOfInterest("Kerbin", "Desert_Airfield", Localizer::Format("#autoLOC_8003184"), false, true);
    pt.POIKerbinWoomerang = new PointOfInterest("Kerbin", "Woomerang_Launch_Site", Localizer::Format("#autoLOC_8003185"), false, true);
    pt.POIMunArmstrongMemorial = new PointOfInterest("Mun", "ArmstrongMemorial", Localizer::Format("#autoLOC_297140"), false);
    pt.POIMunUFO = new PointOfInterest("Mun", "UFO", Localizer::Format("#autoLOC_297141"), false);
    pt.POIMunRockArch00 = new PointOfInterest("Mun", "RockArch00", Localizer::Format("#autoLOC_297142"), false);
    pt.POIMunRockArch01 = new PointOfInterest("Mun", "RockArch01", Localizer::Format("#autoLOC_297143"), false);
    pt.POIMunRockArch02 = new PointOfInterest("Mun", "RockArch02", Localizer::Format("#autoLOC_297144"), false);
    pt.POIMunMonolith00 = new PointOfInterest("Mun", "Monolith00", Localizer::Format("#autoLOC_297145"), false);
    pt.POIMunMonolith01 = new PointOfInterest("Mun", "Monolith01", Localizer::Format("#autoLOC_297146"), false);
    pt.POIMunMonolith02 = new PointOfInterest("Mun", "Monolith02", Localizer::Format("#autoLOC_297147"), false);
    pt.POIDunaPyramid = new PointOfInterest("Duna", "Pyramid", Localizer::Format("#autoLOC_297148"), false);
    pt.POIDunaMSL = new PointOfInterest("Duna", "MSL", Localizer::Format("#autoLOC_297149"), false);
    pt.POIDunaFace = new PointOfInterest("Duna", "Face", Localizer::Format("#autoLOC_297150"), false);
    pt.POIMinmusMonolith00 = new PointOfInterest("Minmus", "Monolith00", Localizer::Format("#autoLOC_297151"), false);
    pt.POITyloCave = new PointOfInterest("Tylo", "Cave", Localizer::Format("#autoLOC_297152"), false);
    pt.POIVallIcehenge = new PointOfInterest("Vall", "Icehenge", Localizer::Format("#autoLOC_297153"), false);
    pt.POIBopDeadKraken = new PointOfInterest("Bop", "DeadKraken", Localizer::Format("#autoLOC_297154"), false);
    pt.POIMohoRandolith = new PointOfInterest("Moho", "Randolith", Localizer::Format("#autoLOC_297156"), true);
    pt.POIEveRandolith = new PointOfInterest("Eve", "Randolith", Localizer::Format("#autoLOC_297157"), true);
    pt.POIGillyRandolith = new PointOfInterest("Gilly", "Randolith", Localizer::Format("#autoLOC_297158"), true);
    pt.POIKerbinRandolith = new PointOfInterest("Kerbin", "Randolith", Localizer::Format("#autoLOC_297159"), true);
    pt.POIMunRandolith = new PointOfInterest("Mun", "Randolith", Localizer::Format("#autoLOC_297160"), true);
    pt.POIMinmusRandolith = new PointOfInterest("Minmus", "Randolith", Localizer::Format("#autoLOC_297161"), true);
    pt.POIDunaRandolith = new PointOfInterest("Duna", "Randolith", Localizer::Format("#autoLOC_297162"), true);
    pt.POIIkeRandolith = new PointOfInterest("Ike", "Randolith", Localizer::Format("#autoLOC_297163"), true);
    pt.POIDresRandolith = new PointOfInterest("Dres", "Randolith", Localizer::Format("#autoLOC_297164"), true);
    pt.POILaytheRandolith = new PointOfInterest("Laythe", "Randolith", Localizer::Format("#autoLOC_297165"), true);
    pt.POIVallRandolith = new PointOfInterest("Vall", "Randolith", Localizer::Format("#autoLOC_297166"), true);
    pt.POITyloRandolith = new PointOfInterest("Tylo", "Randolith", Localizer::Format("#autoLOC_297167"), true);
    pt.POIBopRandolith = new PointOfInterest("Bop", "Randolith", Localizer::Format("#autoLOC_297168"), true);
    pt.POIPolRandolith = new PointOfInterest("Pol", "Randolith", Localizer::Format("#autoLOC_297169"), true);
    pt.POIEelooRandolith = new PointOfInterest("Eeloo", "Randolith", Localizer::Format("#autoLOC_297170"), true);
    pt.POIAnniversary1 = new PointOfInterest("Kerbin", "Anniversary1", Localizer::Format("#autoLOC_8003541"), false);
    pt.POIAnniversary2 = new PointOfInterest("Kerbin", "Anniversary2", Localizer::Format("#autoLOC_8003542"), false);
    pt.POIAnniversary3 = new PointOfInterest("Eeloo", "Anniversary3", Localizer::Format("#autoLOC_8003543"), false);
    pt.POIAnniversary4 = new PointOfInterest("Duna", "Anniversary4", Localizer::Format("#autoLOC_8003544"), false);
    pt.POIAnniversary5 = new PointOfInterest("Ike", "Anniversary5", Localizer::Format("#autoLOC_8003545"), false);
    pt.POIAnniversary6 = new PointOfInterest("Dres", "Anniversary6", Localizer::Format("#autoLOC_8003546"), false);
    pt.POIAnniversary7 = new PointOfInterest("Laythe", "Anniversary7", Localizer::Format("#autoLOC_8003547"), false);
    pt.SetNonCheatablePOIs({pt.POIKerbinDiscoverableLaunchsite01, pt.POIKerbinDiscoverableLaunchsite02, pt.POIKerbinDiscoverableLaunchsite03,
                             pt.POIKerbinDiscoverableLaunchsite04});
    pt.pointsOfInterest.push_back(tree.AddNode(std::unique_ptr<PointOfInterest>(pt.POIKerbinKSC2)));
    pt.pointsOfInterest.push_back(tree.AddNode(std::unique_ptr<PointOfInterest>(pt.POIKerbinIslandAirfield)));
    pt.pointsOfInterest.push_back(tree.AddNode(std::unique_ptr<PointOfInterest>(pt.POIKerbinDiscoverableLaunchsite01)));
    pt.pointsOfInterest.push_back(tree.AddNode(std::unique_ptr<PointOfInterest>(pt.POIKerbinDiscoverableLaunchsite02)));
    pt.pointsOfInterest.push_back(tree.AddNode(std::unique_ptr<PointOfInterest>(pt.POIKerbinDiscoverableLaunchsite03)));
    pt.pointsOfInterest.push_back(tree.AddNode(std::unique_ptr<PointOfInterest>(pt.POIKerbinDiscoverableLaunchsite04)));
    pt.pointsOfInterest.push_back(tree.AddNode(std::unique_ptr<PointOfInterest>(pt.POIKerbinUFO)));
    pt.pointsOfInterest.push_back(tree.AddNode(std::unique_ptr<PointOfInterest>(pt.POIKerbinPyramids)));
    pt.pointsOfInterest.push_back(tree.AddNode(std::unique_ptr<PointOfInterest>(pt.POIKerbinMonolith00)));
    pt.pointsOfInterest.push_back(tree.AddNode(std::unique_ptr<PointOfInterest>(pt.POIKerbinMonolith01)));
    pt.pointsOfInterest.push_back(tree.AddNode(std::unique_ptr<PointOfInterest>(pt.POIKerbinMonolith02)));
    pt.pointsOfInterest.push_back(tree.AddNode(std::unique_ptr<PointOfInterest>(pt.POIKerbinDessertAirfield)));
    pt.pointsOfInterest.push_back(tree.AddNode(std::unique_ptr<PointOfInterest>(pt.POIKerbinWoomerang)));
    pt.pointsOfInterest.push_back(tree.AddNode(std::unique_ptr<PointOfInterest>(pt.POIMunArmstrongMemorial)));
    pt.pointsOfInterest.push_back(tree.AddNode(std::unique_ptr<PointOfInterest>(pt.POIMunUFO)));
    pt.pointsOfInterest.push_back(tree.AddNode(std::unique_ptr<PointOfInterest>(pt.POIMunRockArch00)));
    pt.pointsOfInterest.push_back(tree.AddNode(std::unique_ptr<PointOfInterest>(pt.POIMunRockArch01)));
    pt.pointsOfInterest.push_back(tree.AddNode(std::unique_ptr<PointOfInterest>(pt.POIMunRockArch02)));
    pt.pointsOfInterest.push_back(tree.AddNode(std::unique_ptr<PointOfInterest>(pt.POIMunMonolith00)));
    pt.pointsOfInterest.push_back(tree.AddNode(std::unique_ptr<PointOfInterest>(pt.POIMunMonolith01)));
    pt.pointsOfInterest.push_back(tree.AddNode(std::unique_ptr<PointOfInterest>(pt.POIMunMonolith02)));
    pt.pointsOfInterest.push_back(tree.AddNode(std::unique_ptr<PointOfInterest>(pt.POIDunaPyramid)));
    pt.pointsOfInterest.push_back(tree.AddNode(std::unique_ptr<PointOfInterest>(pt.POIDunaMSL)));
    pt.pointsOfInterest.push_back(tree.AddNode(std::unique_ptr<PointOfInterest>(pt.POIDunaFace)));
    pt.pointsOfInterest.push_back(tree.AddNode(std::unique_ptr<PointOfInterest>(pt.POIMinmusMonolith00)));
    pt.pointsOfInterest.push_back(tree.AddNode(std::unique_ptr<PointOfInterest>(pt.POITyloCave)));
    pt.pointsOfInterest.push_back(tree.AddNode(std::unique_ptr<PointOfInterest>(pt.POIVallIcehenge)));
    pt.pointsOfInterest.push_back(tree.AddNode(std::unique_ptr<PointOfInterest>(pt.POIBopDeadKraken)));
    pt.pointsOfInterest.push_back(tree.AddNode(std::unique_ptr<PointOfInterest>(pt.POIMohoRandolith)));
    pt.pointsOfInterest.push_back(tree.AddNode(std::unique_ptr<PointOfInterest>(pt.POIEveRandolith)));
    pt.pointsOfInterest.push_back(tree.AddNode(std::unique_ptr<PointOfInterest>(pt.POIGillyRandolith)));
    pt.pointsOfInterest.push_back(tree.AddNode(std::unique_ptr<PointOfInterest>(pt.POIKerbinRandolith)));
    pt.pointsOfInterest.push_back(tree.AddNode(std::unique_ptr<PointOfInterest>(pt.POIMunRandolith)));
    pt.pointsOfInterest.push_back(tree.AddNode(std::unique_ptr<PointOfInterest>(pt.POIMinmusRandolith)));
    pt.pointsOfInterest.push_back(tree.AddNode(std::unique_ptr<PointOfInterest>(pt.POIDunaRandolith)));
    pt.pointsOfInterest.push_back(tree.AddNode(std::unique_ptr<PointOfInterest>(pt.POIIkeRandolith)));
    pt.pointsOfInterest.push_back(tree.AddNode(std::unique_ptr<PointOfInterest>(pt.POIDresRandolith)));
    pt.pointsOfInterest.push_back(tree.AddNode(std::unique_ptr<PointOfInterest>(pt.POILaytheRandolith)));
    pt.pointsOfInterest.push_back(tree.AddNode(std::unique_ptr<PointOfInterest>(pt.POIVallRandolith)));
    pt.pointsOfInterest.push_back(tree.AddNode(std::unique_ptr<PointOfInterest>(pt.POITyloRandolith)));
    pt.pointsOfInterest.push_back(tree.AddNode(std::unique_ptr<PointOfInterest>(pt.POIBopRandolith)));
    pt.pointsOfInterest.push_back(tree.AddNode(std::unique_ptr<PointOfInterest>(pt.POIPolRandolith)));
    pt.pointsOfInterest.push_back(tree.AddNode(std::unique_ptr<PointOfInterest>(pt.POIEelooRandolith)));
    pt.pointsOfInterest.push_back(tree.AddNode(std::unique_ptr<PointOfInterest>(pt.POIAnniversary1)));
    pt.pointsOfInterest.push_back(tree.AddNode(std::unique_ptr<PointOfInterest>(pt.POIAnniversary2)));
    pt.pointsOfInterest.push_back(tree.AddNode(std::unique_ptr<PointOfInterest>(pt.POIAnniversary3)));
    pt.pointsOfInterest.push_back(tree.AddNode(std::unique_ptr<PointOfInterest>(pt.POIAnniversary4)));
    pt.pointsOfInterest.push_back(tree.AddNode(std::unique_ptr<PointOfInterest>(pt.POIAnniversary5)));
    pt.pointsOfInterest.push_back(tree.AddNode(std::unique_ptr<PointOfInterest>(pt.POIAnniversary6)));
    pt.pointsOfInterest.push_back(tree.AddNode(std::unique_ptr<PointOfInterest>(pt.POIAnniversary7)));
    const auto& bodies = FlightRegistry::Bodies();
    pt.celestialBodyNodes.assign(bodies.size(), nullptr);
    for (size_t i = 0; i < bodies.size(); ++i) pt.celestialBodyNodes[i] = tree.AddNode(std::make_unique<CelestialBodySubtree>(bodies[i]));
    for (size_t j = 0; j < pt.celestialBodyNodes.size(); ++j) {
        if (pt.celestialBodyNodes[j]->Body->isHomeWorld) {
            pt.celestialBodyHome = pt.celestialBodyNodes[j];
            pt.celestialBodyNodes[j]->LinkBodyHome(pt.celestialBodyNodes);
        }
    }
}

}  // namespace achievements_detail

}  // namespace ksp
