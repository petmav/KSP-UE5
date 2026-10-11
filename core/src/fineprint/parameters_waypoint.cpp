// WP-38 slice 2: FinePrint/Contracts/Parameters/{SpecificVesselParameter,SpecificOrbitParameter,StationaryPointParameter,RoverWayPointParameter,
// SurveyWaypointParameter}.cs: the parameters that own a Waypoint or an orbit. The waypoint manager, the nav waypoint and the orbit renderer
// are seams (FinePrintEnvironment, WaypointManagerSeam).
#include <algorithm>
#include <cmath>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/contracts/agents.hpp"
#include "ksp/contracts/contract_system.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/fineprint/contracts.hpp"
#include "ksp/fineprint/parameters.hpp"
#include "ksp/util/ksp_util.hpp"
#include "ksp/science/science.hpp"
#include "param_common.hpp"

namespace ksp::Contracts::Parameters {

using namespace ksp::fpcommon;
using GE = events::GameEvents;
namespace L = Lite;
namespace M = ksp::dotnet::Math;
using ksp::FinePrint::WaypointManager;
using ksp::FinePrint::WaypointManagerSeam;

namespace {
bool LoadedSceneHasPlanetarium() { return SaveEnvironment::Current().loadedSceneHasPlanetarium; }
bool ContractDefB(const char* key) { return ContractDefsLite::B("", "", key); }
// FlightGlobals.GetBodyByName
CelestialBody* GetBodyByName(const std::string& name) {
    for (CelestialBody* b : FlightRegistry::Bodies())
        if (b->bodyName == name) return b;
    return nullptr;
}
double DefD(const char* sec, const char* sub, const char* key) { return ContractDefsLite::D(sec, sub, key); }  // a C# double member
double DefFd(const char* sec, const char* sub, const char* key) { return static_cast<double>(ContractDefsLite::F(sec, sub, key)); }  // a C# float member read as double
float DefF(const char* sec, const char* sub, const char* key) { return ContractDefsLite::F(sec, sub, key); }
// the body a vessel orbits: loaded ? orbit.referenceBody : FlightGlobals.Bodies[protoVessel.orbitSnapShot.ReferenceBodyIndex]
CelestialBody* ReferenceBodyOf(Vessel* v) {
    return v->loaded ? v->orbit().referenceBody : FlightRegistry::Bodies().at(static_cast<size_t>(v->protoVessel->orbitSnapShot->ReferenceBodyIndex));
}
Guid VesselGuid(Vessel* v) { return v->loaded ? v->id : v->protoVessel->vesselID; }
void WpAdd(Waypoint* wp) {
    if (WaypointManagerSeam::addWaypoint) WaypointManagerSeam::addWaypoint(wp);
}
void WpRemove(Waypoint* wp) {
    if (WaypointManagerSeam::removeWaypoint) WaypointManagerSeam::removeWaypoint(wp);
}
bool WpManagerPresent() { return L::FinePrintEnvironment::waypointManagerPresent && L::FinePrintEnvironment::waypointManagerPresent(); }
float LateralDistance(Waypoint* wp, Vessel* v) { return L::FinePrintEnvironment::lateralDistanceToVessel(wp, v); }
}  // namespace

// ---- SpecificVesselParameter ----------------------------------------------------------------------------------------------------------

// SpecificVesselParameter.cs:34
SpecificVesselParameter::SpecificVesselParameter() : targetVesselID(Guid::Empty()) {
    TargetBody = Planetarium::fetch->Home;
    wp = std::make_unique<Waypoint>();
}
// SpecificVesselParameter.cs:46
SpecificVesselParameter::SpecificVesselParameter(Vessel* v) {
    partIDs = L::GetPartIDList(v);
    targetVessel = v;
    targetVesselID = VesselGuid(v);
    targetVesselPartCount = v->loaded ? static_cast<int>(v->parts.size()) : static_cast<int>(v->protoVessel->protoPartSnapshots.size());
    targetVesselName = targetVessel->loaded ? targetVessel->GetDisplayName() : targetVessel->protoVessel->GetDisplayName();
    targetVesselSituation = targetVessel->loaded ? targetVessel->situation : targetVessel->protoVessel->situation;
    TargetBody = ReferenceBodyOf(v);
    wp = std::make_unique<Waypoint>();
}
// SpecificVesselParameter.cs:58
std::string SpecificVesselParameter::GetHashString() { return I(L::SuperSeed(*Root())) + ID(); }
// SpecificVesselParameter.cs:63
std::string SpecificVesselParameter::GetTitle() { return Fmt("#autoLOC_285073", {targetVesselName}); }
// SpecificVesselParameter.cs:68
std::string SpecificVesselParameter::GetMessageComplete() { return Fmt("#autoLOC_285078", {targetVesselName}); }
// SpecificVesselParameter.cs:73
std::string SpecificVesselParameter::GetNotes() {
    if (!Root()->IsFinished() && targetVessel != nullptr) {
        const std::string bn = TargetBody->displayName();
        switch (targetVesselSituation) {
            case Vessel::Situations::SUB_ORBITAL: return Fmt("#autoLOC_285099", {targetVesselName, bn});
            case Vessel::Situations::FLYING: return Fmt("#autoLOC_285091", {targetVesselName, bn});
            case Vessel::Situations::LANDED: return Fmt("#autoLOC_285093", {targetVesselName, bn});
            case Vessel::Situations::SPLASHED: return Fmt("#autoLOC_285097", {targetVesselName, bn});
            case Vessel::Situations::PRELAUNCH: return Fmt("#autoLOC_285089", {targetVesselName, bn});
            default: return Fmt("#autoLOC_285105", {targetVesselName, bn});
            case Vessel::Situations::DOCKED: return Fmt("#autoLOC_285103", {targetVesselName, bn});
            case Vessel::Situations::ESCAPING: return Fmt("#autoLOC_285101", {targetVesselName, bn});
            case Vessel::Situations::ORBITING: return Fmt("#autoLOC_285095", {targetVesselName, bn});
        }
    }
    return "";
}
// SpecificVesselParameter.cs:102
void SpecificVesselParameter::OnRegister() { SetDisableOnStateChange(false); }
// SpecificVesselParameter.cs:107
void SpecificVesselParameter::OnUnregister() { CleanupWaypoints(); }
// SpecificVesselParameter.cs:112
void SpecificVesselParameter::OnReset() { SetIncomplete(); }
// SpecificVesselParameter.cs:117
void SpecificVesselParameter::OnSave(ConfigNode& node) {
    node.AddValue("TargetBody", static_cast<int32_t>(TargetBody->flightGlobalsIndex));
    node.AddValue("targetVesselID", targetVesselID.ToString('N'));
    node.AddValue("targetVesselPartCount", static_cast<int32_t>(targetVesselPartCount));
    node.AddValue("targetVesselName", targetVesselName);
    node.AddValue("targetVesselSituation", dotnet::EnumToString(targetVesselSituation));
    L::SystemUtilities::SaveNodeList(&node, "partIDs", partIDs);
}
// SpecificVesselParameter.cs:127
void SpecificVesselParameter::OnLoad(ConfigNode& node) {
    tracked = true;
    std::string value;
    L::LoadNode(node, "SpecificVesselParameter", "TargetBody", TargetBody, Planetarium::fetch->Home);
    L::LoadNode(node, "SpecificVesselParameter", "targetVesselID", value, std::string());
    L::LoadNode(node, "SpecificVesselParameter", "targetVesselPartCount", targetVesselPartCount, 1);
    L::LoadNode(node, "SpecificVesselParameter", "targetVesselName", targetVesselName, std::string("Vessel"));
    L::LoadNode(node, "SpecificVesselParameter", "targetVesselSituation", targetVesselSituation, Vessel::Situations::ORBITING);
    L::SystemUtilities::LoadNodeList(&node, "SpecificVesselParameter", "partIDs", partIDs);
    targetVesselID = Guid::Parse(value);
    if (LoadedSceneHasPlanetarium() && !Root()->IsFinished()) TargetVesselSearch(true);
    if (tracked) {
        double fadeRange = 1.7976931348623157e308;
        if (targetVessel != nullptr) {
            fadeRange = targetVessel->loaded ? targetVessel->orbit().ApR() : targetVessel->protoVessel->orbitSnapShot->Load(FlightRegistry::Bodies()).ApR();
        }
        auto setup = [&]() {
            wp->celestialName = TargetBody->GetName();
            wp->latitude = 0.0;
            wp->longitude = 0.0;
            wp->seed = L::SuperSeed(*Root());
            wp->index = 0;
            wp->landLocked = false;
            wp->name = targetVesselName;
            wp->id = "vessel";
            wp->iconSize = 32;
            wp->altitude = 0.0;
            wp->isOnSurface = true;
            wp->isNavigatable = false;
            wp->enableTooltip = false;
            wp->enableMarker = false;
            wp->blocksInput = false;
            wp->contractReference = Root();
            wp->SetFadeRange(fadeRange);
            WpAdd(wp.get());
            submittedWaypoint = true;
        };
        if (Scene() == GameScenes::FLIGHT && Root()->ContractState() == Contract::State::Active) setup();
        if (Scene() == GameScenes::TRACKSTATION &&
            (Root()->ContractState() == Contract::State::Active || (!Root()->IsFinished() && ContractDefB("DisplayOfferedWaypoints"))))
            setup();
    }
}
// SpecificVesselParameter.cs:196
void SpecificVesselParameter::OnUpdate() {
    if (!L::FlightIsReady(Root()->ContractState(), Contract::State::Active, true) || !tracked) return;
    if (State() == ParameterState::Incomplete) {
        if (FlightRegistry::ActiveVessel == targetVessel)
            successCounter++;
        else
            successCounter = 0;
        if (successCounter >= 5) SetComplete();
    }
    if (State() == ParameterState::Complete && FlightRegistry::ActiveVessel != targetVessel) SetIncomplete();
}
// SpecificVesselParameter.cs:223
void SpecificVesselParameter::UpdateWaypoints(bool focus) {
    (void)focus;
    if (targetVessel != nullptr && !Root()->IsFinished() && tracked) {
        if (targetVesselPartCount != (targetVessel->loaded ? static_cast<int>(targetVessel->parts.size()) : static_cast<int>(targetVessel->protoVessel->protoPartSnapshots.size())))
            TargetVesselSearch(false);
        const Vessel::Situations situations = targetVessel->loaded ? targetVessel->situation : targetVessel->protoVessel->situation;
        TargetBody = ReferenceBodyOf(targetVessel);
        wp->celestialName = TargetBody->GetName();
        wp->name = targetVessel->loaded ? targetVessel->GetDisplayName() : targetVessel->protoVessel->GetDisplayName();
        if (static_cast<unsigned>(static_cast<int>(situations) - 1) > 1u && situations != Vessel::Situations::PRELAUNCH) {
            Orbit orbit = targetVessel->loaded ? targetVessel->orbit() : targetVessel->protoVessel->orbitSnapShot->Load(FlightRegistry::Bodies());
            const Vector3d p = orbit.getPositionAtUT(Planetarium::GetUniversalTime());
            wp->orbitPosition = Vector3f(static_cast<float>(p.x), static_cast<float>(p.y), static_cast<float>(p.z));
            wp->isOnSurface = false;
        } else {
            wp->latitude = targetVessel->loaded ? targetVessel->latitude : targetVessel->protoVessel->latitude;
            wp->longitude = targetVessel->loaded ? targetVessel->longitude : targetVessel->protoVessel->longitude;
            wp->altitude = 0.0;
            wp->isOnSurface = true;
        }
    }
}
// SpecificVesselParameter.cs:251
void SpecificVesselParameter::SpecificVesselRename(const Guid& id, const std::string& name) {
    if (targetVessel != nullptr && !Root()->IsFinished() && tracked && refreshContractRequested && id == targetVesselID) {
        targetVesselName = name;
        refreshContractRequested(*Root(), false);
    }
}
// SpecificVesselParameter.cs:260
void SpecificVesselParameter::SpecificVesselScan(const std::vector<Guid>& ids) {
    if (!tracked) return;
    for (const Guid& current : ids) {
        if (!(current == Guid::Empty()) && !(current != targetVesselID)) {
            TargetVesselSearch(false);
            if (Root()->ContractState() == Contract::State::Active && targetVesselID != current) SetIncomplete();
        }
    }
}
// SpecificVesselParameter.cs:281
void SpecificVesselParameter::TargetVesselSearch(bool cached) {
    if (!tracked) return;
    targetVessel = nullptr;
    if (targetVesselID == Guid::Empty()) cached = false;
    if (cached) {
        int count = static_cast<int>(FlightRegistry::Vessels.size());
        while (count-- > 0) {
            Vessel* vessel = FlightRegistry::Vessels[static_cast<size_t>(count)];
            if (VesselGuid(vessel) == targetVesselID) {
                targetVessel = vessel;
                break;
            }
        }
    }
    if (targetVessel == nullptr) targetVessel = L::FindVesselWithPartIDs(&partIDs);
    if (targetVessel == nullptr) {
        L::FinePrintEnvironment::deferred.push_back([this] { KillContract(); });
        return;
    }
    targetVesselPartCount = targetVessel->loaded ? static_cast<int>(targetVessel->parts.size()) : static_cast<int>(targetVessel->protoVessel->protoPartSnapshots.size());
    if (targetVesselPartCount <= 0) {
        L::FinePrintEnvironment::deferred.push_back([this] { KillContract(); });
        return;
    }
    targetVesselID = VesselGuid(targetVessel);
    targetVesselName = targetVessel->loaded ? targetVessel->GetDisplayName() : targetVessel->protoVessel->GetDisplayName();
    targetVesselSituation = targetVessel->loaded ? targetVessel->situation : targetVessel->protoVessel->situation;
    TargetBody = ReferenceBodyOf(targetVessel);
}
// SpecificVesselParameter.cs:326
void SpecificVesselParameter::CleanupWaypoints() {
    if (submittedWaypoint) {
        WpRemove(wp.get());
        submittedWaypoint = false;
    }
}
// SpecificVesselParameter.cs:335 (the iterator: the first segment up to `yield return null` is run by the coroutine at once, the rest on the next frame)
void SpecificVesselParameter::KillContract() {
    if (Root() == nullptr || Root()->IsFinished()) return;
    tracked = false;
    SetDisableOnStateChange(true);
    SetIncomplete();
    L::FinePrintEnvironment::deferred.push_back([this] {
        switch (Root()->ContractState()) {
            case Contract::State::Active: SetFailed(); return;
            case Contract::State::Offered: Root()->Withdraw(); return;
            default: break;
        }
        Root()->Kill();
        if (ContractSystem::Instance != nullptr) {
            // ContractSystem.Instance.Contracts.Remove(contract): the C# list drops its reference (the owning pointer is parked until this lambda ends)
            auto& cs = ContractSystem::Instance->Contracts();
            for (auto it = cs.begin(); it != cs.end(); ++it) {
                if (it->get() == Root()) {
                    std::unique_ptr<Contract> dead = std::move(*it);
                    cs.erase(it);
                    break;
                }
            }
        }
    });
}

// ---- SpecificOrbitParameter -----------------------------------------------------------------------------------------------------------

// SpecificOrbitParameter.cs:39
SpecificOrbitParameter::SpecificOrbitParameter() {
    TargetBody = Planetarium::fetch->Home;
    orbit = nullptr;
}
// SpecificOrbitParameter.cs:55
SpecificOrbitParameter::SpecificOrbitParameter(OrbitType orbitType_, double inclination_, double eccentricity_, double sma_, double lan_,
                                               double argumentOfPeriapsis_, double meanAnomalyAtEpoch_, double epoch_, CelestialBody* targetBody_,
                                               double deviationWindow_)
    : deviationWindow(deviationWindow_), orbitType(orbitType_), inclination(inclination_), eccentricity(eccentricity_), sma(sma_), lan(lan_),
      argumentOfPeriapsis(argumentOfPeriapsis_), meanAnomalyAtEpoch(meanAnomalyAtEpoch_), epoch(epoch_) {
    TargetBody = targetBody_;
    orbit = std::make_unique<Orbit>(inclination, eccentricity, sma, lan, argumentOfPeriapsis, meanAnomalyAtEpoch, epoch, TargetBody);
}
// SpecificOrbitParameter.cs:71
std::string SpecificOrbitParameter::GetHashString() { return I(L::SuperSeed(*Root())) + ID(); }
// SpecificOrbitParameter.cs:76
std::string SpecificOrbitParameter::GetTitle() {
    std::string text = Fmt("#autoLOC_284812");
    switch (Root()->Prestige()) {
        case Contract::ContractPrestige::Trivial: text = Fmt("#autoLOC_284817"); break;
        case Contract::ContractPrestige::Significant: text = Fmt("#autoLOC_284820"); break;
        case Contract::ContractPrestige::Exceptional: text = Fmt("#autoLOC_284823"); break;
    }
    const std::string bn = TargetBody->displayName();
    switch (orbitType) {
        default: return Fmt("#autoLOC_7001091", {bn, text});
        case OrbitType::SYNCHRONOUS:
            if (TargetBody == Planetarium::fetch->Sun) return Fmt("#autoLOC_7001089", {Fmt("#autoLOC_7001208"), bn, text});
            if (TargetBody == Planetarium::fetch->Home) return Fmt("#autoLOC_7001089", {Fmt("#autoLOC_7001209"), bn, text});
            return Fmt("#autoLOC_7001089", {Fmt("#autoLOC_7001210"), bn, text});
        case OrbitType::STATIONARY:
            if (TargetBody == Planetarium::fetch->Sun) return Fmt("#autoLOC_7001090", {Fmt("#autoLOC_7001205"), bn, text});
            if (TargetBody == Planetarium::fetch->Home) return Fmt("#autoLOC_7001090", {Fmt("#autoLOC_7001206"), bn, text});
            return Fmt("#autoLOC_7001090", {Fmt("#autoLOC_7001207"), bn, text});
        case OrbitType::POLAR: return Fmt("#autoLOC_7001089", {Fmt("#autoLOC_7001202"), bn, text});
        case OrbitType::EQUATORIAL: return Fmt("#autoLOC_7001089", {Fmt("#autoLOC_7001201"), bn, text});
        case OrbitType::KOLNIYA: return Fmt("#autoLOC_7001089", {L::StringUtilities::TitleCase(Fmt("#autoLOC_7001203")), bn, text});
        case OrbitType::TUNDRA: return Fmt("#autoLOC_7001089", {Fmt("#autoLOC_7001204"), bn, text});
    }
}
// SpecificOrbitParameter.cs:126
std::string SpecificOrbitParameter::GetNotes() {
    const double a = sma * (1.0 - eccentricity) - TargetBody->Radius;
    const double a2 = sma * (1.0 + eccentricity) - TargetBody->Radius;
    std::string text;
    if (Scene() == GameScenes::SPACECENTER) text += "\n";
    text += Fmt("#autoLOC_7003228") + ":\n" + Fmt("#autoLOC_7003229", {RoundedThousands(a2)}) + "\n" + Fmt("#autoLOC_7003230", {RoundedThousands(a)}) + "\n" +
            Fmt("#autoLOC_7003231", {dotnet::FormatDoubleG(M::Round(inclination, 1, dotnet::MidpointRounding::ToEven))}) + "\n";
    if (std::fmod(M::Abs(inclination), 180.0) >= 1.0)
        text = text + "\n" + Fmt("#autoLOC_7003232", {dotnet::FormatDoubleG(M::Round(lan, 1, dotnet::MidpointRounding::ToEven))});
    if (eccentricity > 0.05) text = text + "\n" + Fmt("#autoLOC_7003233", {dotnet::FormatDoubleG(M::Round(argumentOfPeriapsis, 1, dotnet::MidpointRounding::ToEven))});
    return text;
}
// SpecificOrbitParameter.cs:147
void SpecificOrbitParameter::OnRegister() { SetDisableOnStateChange(false); }
// SpecificOrbitParameter.cs:152
void SpecificOrbitParameter::OnUnregister() { CleanupRenderer(); }
// SpecificOrbitParameter.cs:157
void SpecificOrbitParameter::OnReset() { SetIncomplete(); }
// SpecificOrbitParameter.cs:162
void SpecificOrbitParameter::OnSave(ConfigNode& node) {
    if (orbit == nullptr) orbit = std::make_unique<Orbit>(inclination, eccentricity, sma, lan, argumentOfPeriapsis, meanAnomalyAtEpoch, epoch, TargetBody);
    ValidateOrbit();
    node.AddValue("TargetBody", static_cast<int32_t>(TargetBody->flightGlobalsIndex));
    node.AddValue("deviationWindow", deviationWindow);
    node.AddValue("orbitType", static_cast<int32_t>(orbitType));
    node.AddValue("inclination", inclination);
    node.AddValue("eccentricity", eccentricity);
    node.AddValue("sma", sma);
    node.AddValue("lan", lan);
    node.AddValue("argumentOfPeriapsis", argumentOfPeriapsis);
    node.AddValue("meanAnomalyAtEpoch", meanAnomalyAtEpoch);
    node.AddValue("epoch", epoch);
}
// SpecificOrbitParameter.cs:181
void SpecificOrbitParameter::OnLoad(ConfigNode& node) {
    L::LoadNode(node, "SpecificOrbitParameter", "TargetBody", TargetBody, Planetarium::fetch->Home);
    L::LoadNode(node, "SpecificOrbitParameter", "deviationWindow", deviationWindow, 10.0);
    L::LoadNode(node, "SpecificOrbitParameter", "orbitType", orbitType, OrbitType::RANDOM);
    L::LoadNode(node, "SpecificOrbitParameter", "inclination", inclination, 0.0);
    L::LoadNode(node, "SpecificOrbitParameter", "eccentricity", eccentricity, 0.0);
    L::LoadNode(node, "SpecificOrbitParameter", "sma", sma, 1000000.0);
    L::LoadNode(node, "SpecificOrbitParameter", "lan", lan, 0.0);
    L::LoadNode(node, "SpecificOrbitParameter", "argumentOfPeriapsis", argumentOfPeriapsis, 0.0);
    L::LoadNode(node, "SpecificOrbitParameter", "meanAnomalyAtEpoch", meanAnomalyAtEpoch, 0.0);
    L::LoadNode(node, "SpecificOrbitParameter", "epoch", epoch, 0.0);
    if (orbit == nullptr) orbit = std::make_unique<Orbit>(inclination, eccentricity, sma, lan, argumentOfPeriapsis, meanAnomalyAtEpoch, epoch, TargetBody);
    ValidateOrbit();
    if (Scene() == GameScenes::FLIGHT && Root()->ContractState() == Contract::State::Active) SetupRenderer();
    if (Scene() == GameScenes::TRACKSTATION && State() == ParameterState::Incomplete &&
        (Root()->ContractState() == Contract::State::Active || (!Root()->IsFinished() && ContractDefB("DisplayOfferedOrbits"))))
        SetupRenderer();
}
// SpecificOrbitParameter.cs:208
void SpecificOrbitParameter::OnUpdate() {
    if (!L::FlightIsReady(Root()->ContractState(), Contract::State::Active, true)) return;
    Orbit* driverOrbit = L::FinePrintEnvironment::orbitRendererDriverOrbit ? L::FinePrintEnvironment::orbitRendererDriverOrbit() : nullptr;
    const bool flag = rendererSetUp && driverOrbit != nullptr && L::VesselAtOrbit(driverOrbit, deviationWindow);
    if (State() == ParameterState::Incomplete) {
        if (rendererSetUp) rendererActiveDraw = true;
        if (flag)
            successCounter++;
        else
            successCounter = 0;
        if (successCounter >= 5) SetComplete();
    }
    if (State() != ParameterState::Complete) return;
    if (flag) {
        if (rendererSetUp) rendererActiveDraw = false;
        return;
    }
    if (rendererSetUp) rendererActiveDraw = true;
    SetIncomplete();
}
// SpecificOrbitParameter.cs:253
void SpecificOrbitParameter::SetupRenderer() {
    if (!rendererSetUp && orbit != nullptr) {
        if (L::FinePrintEnvironment::setupOrbitRenderer) L::FinePrintEnvironment::setupOrbitRenderer(*Root(), *orbit);
        rendererSetUp = true;
    }
}
// SpecificOrbitParameter.cs:261
void SpecificOrbitParameter::CleanupRenderer() {
    if (rendererSetUp && L::FinePrintEnvironment::cleanupOrbitRenderer) L::FinePrintEnvironment::cleanupOrbitRenderer();
}
// SpecificOrbitParameter.cs:290
void SpecificOrbitParameter::ValidateOrbit() {
    if (Root() != nullptr && !Root()->IsFinished() && TargetBody != nullptr && orbit != nullptr) {
        double altitudeDifficulty;
        double inclinationDifficulty;
        switch (Root()->Prestige()) {
            default:
                altitudeDifficulty = DefFd("Satellite", "", "SignificantAltitudeDifficulty");
                inclinationDifficulty = DefFd("Satellite", "", "SignificantInclinationDifficulty");
                break;
            case Contract::ContractPrestige::Exceptional:
                altitudeDifficulty = DefFd("Satellite", "", "ExceptionalAltitudeDifficulty");
                inclinationDifficulty = DefFd("Satellite", "", "ExceptionalInclinationDifficulty");
                break;
            case Contract::ContractPrestige::Trivial:
                altitudeDifficulty = DefFd("Satellite", "", "TrivialAltitudeDifficulty");
                inclinationDifficulty = DefFd("Satellite", "", "TrivialInclinationDifficulty");
                break;
        }
        if (!FinePrint::Utilities::OrbitUtilities::ValidateOrbit(Root()->MissionSeed(), *orbit, orbitType, altitudeDifficulty, inclinationDifficulty,
                                                                 "contract parameter \"" + GetTitle() + "\"")) {
            inclination = orbit->inclination;
            eccentricity = orbit->eccentricity;
            sma = orbit->semiMajorAxis;
            lan = orbit->LAN;
            argumentOfPeriapsis = orbit->argumentOfPeriapsis;
            meanAnomalyAtEpoch = orbit->meanAnomalyAtEpoch;
            epoch = orbit->epoch;
        }
    }
}

// ---- StationaryPointParameter ---------------------------------------------------------------------------------------------------------

// StationaryPointParameter.cs:23
StationaryPointParameter::StationaryPointParameter()
    : targetBody(Planetarium::fetch->Home), submittedWaypoint(false), longitude(0.0), deviation(DefD("Satellite", "", "TrivialDeviation")), successCounter(0) {
    wp = std::make_unique<Waypoint>();
}
// StationaryPointParameter.cs:33
StationaryPointParameter::StationaryPointParameter(CelestialBody* targetBody_, double longitude_, double deviation_)
    : targetBody(targetBody_), submittedWaypoint(false), longitude(longitude_), deviation(deviation_), successCounter(0) {
    wp = std::make_unique<Waypoint>();
}
// StationaryPointParameter.cs:43
std::string StationaryPointParameter::GetHashString() { return I(L::SuperSeed(*Root())) + ID(); }
// StationaryPointParameter.cs:48
std::string StationaryPointParameter::GetTitle() {
    return Fmt("#autoLOC_285514", {L::StringUtilities::GenerateSiteName(L::SuperSeed(*Root()), targetBody, true)});
}
// StationaryPointParameter.cs:53
std::string StationaryPointParameter::GetMessageComplete() {
    return Fmt("#autoLOC_285519", {L::StringUtilities::GenerateSiteName(L::SuperSeed(*Root()), targetBody, true)});
}
// StationaryPointParameter.cs:58
void StationaryPointParameter::OnRegister() { SetDisableOnStateChange(false); }
// StationaryPointParameter.cs:63
void StationaryPointParameter::OnUnregister() {
    if (submittedWaypoint) WpRemove(wp.get());
}
// StationaryPointParameter.cs:71
void StationaryPointParameter::OnReset() { SetIncomplete(); }
// StationaryPointParameter.cs:76
void StationaryPointParameter::OnSave(ConfigNode& node) {
    node.AddValue("targetBody", static_cast<int32_t>(targetBody->flightGlobalsIndex));
    node.AddValue("longitude", longitude);
    node.AddValue("deviation", deviation);
}
// StationaryPointParameter.cs:83
void StationaryPointParameter::OnLoad(ConfigNode& node) {
    L::LoadNode(node, "StationaryPointParameter", "targetBody", targetBody, Planetarium::fetch->Home);
    L::LoadNode(node, "StationaryPointParameter", "longitude", longitude, 0.0);
    L::LoadNode(node, "StationaryPointParameter", "deviation", deviation, DefD("Satellite", "", "TrivialDeviation"));
    auto setup = [&]() {
        wp->celestialName = targetBody->GetName();
        wp->latitude = 0.0;
        wp->longitude = longitude;
        wp->seed = L::SuperSeed(*Root());
        wp->index = 0;
        wp->landLocked = true;
        wp->setName(false);
        wp->id = "dish";
        wp->altitude = 0.0;
        wp->isOnSurface = true;
        wp->isNavigatable = true;
        wp->contractReference = Root();
        wp->SetFadeRange();
        WpAdd(wp.get());
        submittedWaypoint = true;
    };
    if (Scene() == GameScenes::FLIGHT && Root()->ContractState() == Contract::State::Active) setup();
    if (Scene() == GameScenes::TRACKSTATION && State() == ParameterState::Incomplete &&
        (Root()->ContractState() == Contract::State::Active || (!Root()->IsFinished() && ContractDefB("DisplayOfferedOrbits"))))
        setup();
}
// StationaryPointParameter.cs:126
void StationaryPointParameter::OnUpdate() {
    if (!L::FlightIsReady(Root()->ContractState(), Contract::State::Active, true)) return;
    Vessel* activeVessel = FlightRegistry::ActiveVessel;
    if (!submittedWaypoint || activeVessel->mainBody() != targetBody) return;
    const bool flag = 180.0 - M::Abs(M::Abs(activeVessel->longitude - longitude) - 180.0) <= deviation / 100.0 * 360.0;
    if (State() == ParameterState::Incomplete) {
        if (flag)
            successCounter++;
        else
            successCounter = 0;
        if (successCounter >= 5) SetComplete();
    }
    if (State() == ParameterState::Complete && !flag) SetIncomplete();
}

// ---- RoverWayPointParameter -----------------------------------------------------------------------------------------------------------

// RoverWayPointParameter.cs:48
RoverWayPointParameter::RoverWayPointParameter() {
    startwp = std::make_unique<Waypoint>();
    endwp = std::make_unique<Waypoint>();
    TargetBody = nullptr;
}
// RoverWayPointParameter.cs:62
RoverWayPointParameter::RoverWayPointParameter(double CraftStartLatitude_, double CraftStartLongitude_, double CraftEndLatitude_, double CraftEndLongitude_,
                                               CelestialBody* targetBody) {
    startwp = std::make_unique<Waypoint>();
    endwp = std::make_unique<Waypoint>();
    endwp->setName();
    craftStartLongitude = CraftStartLongitude_;
    craftStartLatitude = CraftStartLatitude_;
    craftEndLatitude = CraftEndLatitude_;
    craftEndLongitude = CraftEndLongitude_;
    TargetBody = targetBody;
}
// RoverWayPointParameter.cs:76
std::string RoverWayPointParameter::GetHashString() { return I(L::SuperSeed(*Root())) + ID(); }
// RoverWayPointParameter.cs:81
std::string RoverWayPointParameter::GetTitle() { return Fmt("#autoLOC_6002560"); }
// RoverWayPointParameter.cs:86
std::string RoverWayPointParameter::GetMessageComplete() { return Fmt("#autoLOC_6002561"); }
// RoverWayPointParameter.cs:91
std::string RoverWayPointParameter::GetNotes() {
    if (!Root()->IsFinished() && roverVessel != nullptr && TargetBody != nullptr)
        return Fmt("#autoLOC_6002562", {roverVessel->vesselName, endwp->name, TargetBody->displayName()});
    return "";
}
// RoverWayPointParameter.cs:100
void RoverWayPointParameter::OnRegister() {
    SetDisableOnStateChange(false);
    GE::onVesselPersistentIdChanged.Add(this, &RoverWayPointParameter::VesselIdChanged);
}
// RoverWayPointParameter.cs:106
void RoverWayPointParameter::OnUnregister() {
    CleanupWaypoints();
    GE::onVesselPersistentIdChanged.Remove(this, &RoverWayPointParameter::VesselIdChanged);
}
// RoverWayPointParameter.cs:112
void RoverWayPointParameter::OnReset() { SetIncomplete(); }
// RoverWayPointParameter.cs:117
void RoverWayPointParameter::OnSave(ConfigNode& node) {
    node.AddValue("bodyName", TargetBody->bodyName);
    node.AddValue("roverVslId", roverVslId);
    node.AddValue("craftStartLat", craftStartLatitude);
    node.AddValue("craftStartLon", craftStartLongitude);
    node.AddValue("craftEndLat", craftEndLatitude);
    node.AddValue("craftEndLon", craftEndLongitude);
    if (endwp != nullptr) node.AddValue("endWPName", endwp->name);
}
// RoverWayPointParameter.cs:131
void RoverWayPointParameter::OnLoad(ConfigNode& node) {
    std::string value;
    if (node.TryGetValue("bodyName", value)) TargetBody = GetBodyByName(value);
    node.TryGetValue("roverVslId", roverVslId);
    roverVessel = nullptr;
    for (Vessel* v : FlightRegistry::Vessels)
        if (v->persistentId == roverVslId) roverVessel = v;
    node.TryGetValue("craftStartLat", craftStartLatitude);
    node.TryGetValue("craftStartLon", craftStartLongitude);
    node.TryGetValue("craftEndLat", craftEndLatitude);
    node.TryGetValue("craftEndLon", craftEndLongitude);
    if (endwp != nullptr) node.TryGetValue("endWPName", endwp->name);
    if (Scene() == GameScenes::FLIGHT && Root()->ContractState() == Contract::State::Active && endwp != nullptr) {
        endwp->celestialName = TargetBody->GetName();
        endwp->latitude = CraftEndLatitude();
        endwp->longitude = CraftEndLongitude();
        endwp->seed = L::SuperSeed(*Root());
        endwp->index = 1;
        endwp->landLocked = false;
        endwp->id = "vesselEndWP";
        endwp->iconSize = 32;
        endwp->altitude = 0.0;
        endwp->isOnSurface = true;
        endwp->isNavigatable = true;
        endwp->enableTooltip = true;
        endwp->enableMarker = true;
        endwp->blocksInput = false;
        endwp->contractReference = Root();
        endwp->SetFadeRange();
        WpAdd(endwp.get());
        submittedWaypoint = true;
    }
    if (Scene() == GameScenes::TRACKSTATION) {
        if (Root()->ContractState() == Contract::State::Offered) {
            startwp->celestialName = TargetBody->GetName();
            startwp->latitude = CraftStartLatitude();
            startwp->longitude = CraftStartLongitude();
            startwp->seed = L::SuperSeed(*Root());
            startwp->index = 0;
            startwp->landLocked = false;
            startwp->name = Fmt("#autoLOC_6002563");
            startwp->id = "vesselStartWP";
            startwp->iconSize = 32;
            startwp->altitude = 0.0;
            startwp->isOnSurface = true;
            startwp->isNavigatable = false;
            startwp->enableTooltip = true;
            startwp->enableMarker = true;
            startwp->blocksInput = false;
            startwp->contractReference = Root();
            startwp->SetFadeRange();
            WpAdd(startwp.get());
        }
        if (Root()->ContractState() == Contract::State::Active || (!Root()->IsFinished() && ContractDefB("DisplayOfferedWaypoints") && endwp != nullptr)) {
            endwp->celestialName = TargetBody->GetName();
            endwp->latitude = CraftEndLatitude();
            endwp->longitude = CraftEndLongitude();
            endwp->seed = L::SuperSeed(*Root());
            endwp->index = 1;
            endwp->landLocked = false;
            endwp->id = "vesselEndWP";
            endwp->iconSize = 32;
            endwp->altitude = 0.0;
            endwp->isOnSurface = true;
            endwp->isNavigatable = true;
            endwp->enableTooltip = true;
            endwp->enableMarker = true;
            endwp->blocksInput = false;
            endwp->contractReference = Root();
            endwp->SetFadeRange();
            WpAdd(endwp.get());
            submittedWaypoint = true;
        }
    }
}
// RoverWayPointParameter.cs:216
void RoverWayPointParameter::OnUpdate() {
    if (!L::FlightIsReady(Root()->ContractState(), Contract::State::Active, true) || roverVessel == nullptr || State() != ParameterState::Incomplete ||
        !submittedWaypoint || !(roverVessel->mainBody() == TargetBody) || !WpManagerPresent())
        return;
    const float num = LateralDistance(endwp.get(), roverVessel);
    const float range = DefF("RoverConstruction", "", "WayPointTriggerRange");
    if (num > range && outerWarning) {
        std::string text = endwp->name;
        if (endwp->isClustered) text = text + " " + L::StringUtilities::IntegerToGreek(endwp->index);
        outerWarning = false;
        L::FinePrintEnvironment::Post(Fmt("#autoLOC_285861", {text}), 5.0f, L::ScreenMessageStyle::UPPER_CENTER);
    }
    if (num <= range && !outerWarning) {
        std::string text2 = endwp->name;
        if (endwp->isClustered) text2 = text2 + " " + L::StringUtilities::IntegerToGreek(endwp->index);
        outerWarning = true;
        L::FinePrintEnvironment::Post(Fmt("#autoLOC_285872", {text2}), 5.0f, L::ScreenMessageStyle::UPPER_CENTER);
    }
    auto* fetch = L::FinePrintEnvironment::navWaypoint;
    if (fetch != nullptr && fetch->isActive && fetch->isActive() && fetch->isUsing && fetch->isUsing(endwp.get())) {
        if (num <= range) {
            if (!fetch->isBlinking()) fetch->setBlinking(true);
        } else if (fetch->isBlinking()) {
            fetch->setBlinking(false);
        }
    }
    if (num <= range) {
        successCounter++;
        if (successCounter >= 5) SetComplete();
    }
}
// RoverWayPointParameter.cs:268
void RoverWayPointParameter::UpdateWaypoints(bool focus) {
    (void)focus;
    if (roverVessel != nullptr && !Root()->IsFinished()) {
        startwp->celestialName = TargetBody->GetName();
        endwp->celestialName = startwp->celestialName;
        startwp->name = roverVessel->loaded ? roverVessel->GetDisplayName() : roverVessel->protoVessel->GetDisplayName();
        startwp->latitude = CraftStartLatitude();
        startwp->longitude = CraftStartLongitude();
        startwp->altitude = 0.0;
        startwp->isOnSurface = true;
        endwp->latitude = CraftEndLatitude();
        endwp->longitude = CraftEndLongitude();
        endwp->altitude = 0.0;
        endwp->isOnSurface = true;
    }
}
// RoverWayPointParameter.cs:286
void RoverWayPointParameter::UpdateRoverInfo(Vessel* vsl) {
    roverVessel = vsl;
    roverVslId = vsl->persistentId;
}
// RoverWayPointParameter.cs:292
void RoverWayPointParameter::CleanupWaypoints() {
    if (submittedWaypoint) {
        WpRemove(startwp.get());
        WpRemove(endwp.get());
        submittedWaypoint = false;
    }
}
// RoverWayPointParameter.cs:302
void RoverWayPointParameter::VesselIdChanged(uint32_t oldId, uint32_t newId) {
    if (roverVslId == oldId) roverVslId = newId;
}

// ---- SurveyWaypointParameter ----------------------------------------------------------------------------------------------------------

// SurveyWaypointParameter.cs:45 ContextualAnomalyName
std::string SurveyWaypointParameter::ContextualAnomalyName() {
    if (!contextualAnomalyName.empty()) return contextualAnomalyName;
    auto* surveyContract = dynamic_cast<ksp::FinePrint::Contracts::SurveyContract*>(Root());
    if (surveyContract == nullptr) return Fmt("#autoLOC_285665");
    contextualAnomalyName = Fmt("#autoLOC_8004457", {surveyContract->dataName, surveyContract->anomalyName});
    return contextualAnomalyName;
}
// SurveyWaypointParameter.cs:62
SurveyWaypointParameter::SurveyWaypointParameter()
    : targetBody(Planetarium::fetch->Home), band(FlightBand::NONE), submittedWaypoint(false), outerWarning(false), verified(false),
      actionDescription("Perform a survey"), experiment("crewReport"), clusterState(WaypointClusterState::MIXED), thresholdAltitude(10000.0) {
    wp = std::make_unique<Waypoint>();
    wp->id = "default";
}
// SurveyWaypointParameter.cs:77
SurveyWaypointParameter::SurveyWaypointParameter(const std::string& experiment_, const std::string& actionDescription_, CelestialBody* targetBody_,
                                                 const Waypoint& wp_, FlightBand band_)
    : clusterState(WaypointClusterState::MIXED), experiment(experiment_), actionDescription(actionDescription_), band(band_), targetBody(targetBody_),
      submittedWaypoint(false), outerWarning(false), verified(false), contextual(false), bouncesLeft(2147483647), bounceOriginLatitude(0.0),
      bounceOriginLongitude(0.0) {
    wp = std::make_unique<Waypoint>(wp_);
    ProcessWaypoint();
}
// SurveyWaypointParameter.cs:95
SurveyWaypointParameter::SurveyWaypointParameter(const std::string& experiment_, const std::string& actionDescription_, CelestialBody* targetBody_,
                                                 const Waypoint& wp_, FlightBand band_, bool contextual_, int bouncesLeft_, double bounceOriginLatitude_,
                                                 double bounceOriginLongitude_)
    : clusterState(WaypointClusterState::MIXED), experiment(experiment_), actionDescription(actionDescription_), band(band_), targetBody(targetBody_),
      submittedWaypoint(false), outerWarning(false), verified(false), contextual(contextual_), bouncesLeft(bouncesLeft_),
      bounceOriginLatitude(bounceOriginLatitude_), bounceOriginLongitude(bounceOriginLongitude_) {
    wp = std::make_unique<Waypoint>(wp_);
    ProcessWaypoint();
}
// SurveyWaypointParameter.cs:113
std::string SurveyWaypointParameter::GetHashString() { return I(L::SuperSeed(*Root())) + ID(); }
// SurveyWaypointParameter.cs:118
std::string SurveyWaypointParameter::GetTitle() {
    bool value = L::CelestialUtilities::IsFlyablePlanet(targetBody);
    if (value && band == FlightBand::HIGH) {
        ScienceExperiment* scienceExperiment = ResearchAndDevelopment::GetExperiment(experiment);
        if (scienceExperiment != nullptr && !scienceExperiment->IsAvailableWhile(ExperimentSituations::FlyingHigh, *targetBody)) value = false;
    }
    switch (band) {
        default: return Fmt("#autoLOC_6001088", {actionDescription, SiteString()});
        case FlightBand::GROUND: return Fmt("#autoLOC_6001085", {actionDescription, SiteString()});
        case FlightBand::LOW:
            return Fmt("#autoLOC_6001086", {actionDescription, I(value ? 1 : 0), RoundedThousands(thresholdAltitude), SiteString()});
        case FlightBand::HIGH:
            return Fmt("#autoLOC_6001087", {actionDescription, I(value ? 1 : 0), RoundedThousands(thresholdAltitude), SiteString()});
    }
}
// SurveyWaypointParameter.cs:142
std::string SurveyWaypointParameter::GetMessageComplete() {
    if (clusterState == WaypointClusterState::FULL)
        return Fmt("#autoLOC_285754", {L::StringUtilities::GenerateSiteName(wp->seed, targetBody, wp->landLocked, !contextual), L::StringUtilities::IntegerToGreek(wp->index),
                                       targetBody->displayName()});
    if (clusterState == WaypointClusterState::MIXED && band == FlightBand::GROUND)
        return Fmt("#autoLOC_285757", {L::StringUtilities::GenerateSiteName(wp->seed, targetBody, wp->landLocked, !contextual), L::StringUtilities::IntegerToGreek(wp->index),
                                       targetBody->displayName()});
    return Fmt("#autoLOC_285759", {L::StringUtilities::GenerateSiteName(wp->uniqueSeed(), targetBody, wp->landLocked, !contextual), targetBody->displayName()});
}
// SurveyWaypointParameter.cs:155
void SurveyWaypointParameter::OnRegister() {
    if (Root()->ContractState() == Contract::State::Active && !eventsAdded) {
        GE::OnExperimentDeployed.Add(this, &SurveyWaypointParameter::CheckExperimentResults);
        eventsAdded = true;
    }
}
// SurveyWaypointParameter.cs:164
void SurveyWaypointParameter::OnUnregister() {
    if (eventsAdded) GE::OnExperimentDeployed.Remove(this, &SurveyWaypointParameter::CheckExperimentResults);
    if (submittedWaypoint) {
        if (Scene() == GameScenes::FLIGHT) {
            auto* fetch = L::FinePrintEnvironment::navWaypoint;
            if (fetch != nullptr && fetch->isActive && fetch->isActive() && fetch->isUsing && fetch->isUsing(wp.get()) && fetch->deactivate) fetch->deactivate();
        }
        WpRemove(wp.get());
    }
}
// SurveyWaypointParameter.cs:180
void SurveyWaypointParameter::OnSave(ConfigNode& node) {
    node.AddValue("targetBody", static_cast<int32_t>(targetBody->flightGlobalsIndex));
    node.AddValue("actionDescription", actionDescription);
    node.AddValue("experiment", experiment);
    node.AddValue("flightThreshold", thresholdAltitude);
    node.AddValue("band", static_cast<int32_t>(band));
    node.AddValue("clusterState", dotnet::EnumToString(clusterState));
    node.AddValue("verified", verified);
    node.AddValue("contextual", contextual);
    if (contextual) {
        node.AddValue("bouncesLeft", static_cast<int32_t>(bouncesLeft));
        node.AddValue("bounceOriginLatitude", bounceOriginLatitude);
        node.AddValue("bounceOriginLongitude", bounceOriginLongitude);
    }
    node.AddValue("wpSeed", static_cast<int32_t>(wp->seed));
    node.AddValue("wpIndex", static_cast<int32_t>(wp->index));
    node.AddValue("wpTexture", wp->id);
    node.AddValue("wpLatitude", wp->latitude);
    node.AddValue("wpLongitude", wp->longitude);
    node.AddValue("wpLandlocked", wp->landLocked);
    node.AddValue("wpNavigationId", wp->navigationId.ToString());
}
// SurveyWaypointParameter.cs:205
void SurveyWaypointParameter::OnLoad(ConfigNode& node) {
    L::LoadNode(node, "SurveyWaypointParameter", "targetBody", targetBody, Planetarium::fetch->Home);
    L::LoadNode(node, "SurveyWaypointParameter", "actionDescription", actionDescription, std::string("Perform a survey"));
    L::LoadNode(node, "SurveyWaypointParameter", "experiment", experiment, std::string("crewReport"));
    L::LoadNode(node, "SurveyWaypointParameter", "flightThreshold", thresholdAltitude, 10000.0);
    L::LoadNode(node, "SurveyWaypointParameter", "band", band, FlightBand::NONE);
    L::LoadNode(node, "SurveyWaypointParameter", "clusterState", clusterState, WaypointClusterState::MIXED);
    L::LoadNode(node, "SurveyWaypointParameter", "verified", verified, false);
    L::LoadNode(node, "SurveyWaypointParameter", "contextual", contextual, false);
    if (contextual) {
        L::LoadNode(node, "SurveyWaypointParameter", "bouncesLeft", bouncesLeft, 0);
        L::LoadNode(node, "SurveyWaypointParameter", "bounceOriginLatitude", bounceOriginLatitude, 0.0);
        L::LoadNode(node, "SurveyWaypointParameter", "bounceOriginLongitude", bounceOriginLongitude, 0.0);
    }
    L::LoadNode(node, "SurveyWaypointParameter", "wpSeed", wp->seed, 1337);
    L::LoadNode(node, "SurveyWaypointParameter", "wpIndex", wp->index, 0);
    L::LoadNode(node, "SurveyWaypointParameter", "wpTexture", wp->id, std::string("default"));
    L::LoadNode(node, "SurveyWaypointParameter", "wpLatitude", wp->latitude, 0.0);
    L::LoadNode(node, "SurveyWaypointParameter", "wpLongitude", wp->longitude, 0.0);
    L::LoadNode(node, "SurveyWaypointParameter", "wpLandlocked", wp->landLocked, false);
    L::LoadNode(node, "SurveyWaypointParameter", "wpNavigationId", wp->navigationId, Contracts::ContractsEnvironment::Current().NewGuid());
    if (wp->navigationId == Guid::Empty()) wp->navigationId = Contracts::ContractsEnvironment::Current().NewGuid();
    ProcessWaypoint();
}
// SurveyWaypointParameter.cs:236
void SurveyWaypointParameter::OnUpdate() {
    if (!L::FlightIsReady(Root()->ContractState(), Contract::State::Active, true)) return;
    Vessel* activeVessel = FlightRegistry::ActiveVessel;
    if (!submittedWaypoint || !(activeVessel->mainBody() == targetBody) || !WpManagerPresent()) return;
    const float num = LateralDistance(wp.get(), nullptr);
    const double num2 = TriggerRange(band);
    if (static_cast<double>(num) > num2 && outerWarning) {
        std::string text = wp->name;
        if (wp->isClustered) text = text + " " + L::StringUtilities::IntegerToGreek(wp->index);
        outerWarning = false;
        L::FinePrintEnvironment::Post(Fmt("#autoLOC_285861", {text}), 5.0f, L::ScreenMessageStyle::UPPER_CENTER);
    }
    if (static_cast<double>(num) <= num2 && !outerWarning) {
        std::string text2 = wp->name;
        if (wp->isClustered) text2 = text2 + " " + L::StringUtilities::IntegerToGreek(wp->index);
        outerWarning = true;
        L::FinePrintEnvironment::Post(Fmt("#autoLOC_285872", {text2}), 5.0f, L::ScreenMessageStyle::UPPER_CENTER);
    }
    auto* fetch = L::FinePrintEnvironment::navWaypoint;
    if (fetch == nullptr || !fetch->isActive || !fetch->isActive() || !fetch->isUsing || !fetch->isUsing(wp.get())) return;
    if (static_cast<double>(num) <= num2) {
        if (!fetch->isBlinking()) fetch->setBlinking(true);
    } else if (fetch->isBlinking()) {
        fetch->setBlinking(false);
    }
}
// SurveyWaypointParameter.cs:287
void SurveyWaypointParameter::CheckExperimentResults(ScienceData* experimentData) {
    if (Root()->ContractState() != Contract::State::Active || !L::FlightIsReady(true, targetBody) || !submittedWaypoint || !WpManagerPresent() ||
        experimentData->subjectID.rfind(experiment + "@", 0) != 0 || L::GetFlightBand(thresholdAltitude) != band)
        return;
    const float num = LateralDistance(wp.get(), nullptr);
    const double num2 = TriggerRange(band);
    if (!(static_cast<double>(num) < num2)) return;
    if (contextual && bouncesLeft > 0) {
        Bounce();
        return;
    }
    std::string text = wp->name;
    if (wp->isClustered) text = text + " " + L::StringUtilities::IntegerToGreek(wp->index);
    if (!contextual) {
        UpdateNavWaypoint(NextWaypointInCluster());
        L::FinePrintEnvironment::Post(Fmt("#autoLOC_285927", {text}), 5.0f, L::ScreenMessageStyle::UPPER_LEFT);
    } else {
        auto* fetch = L::FinePrintEnvironment::navWaypoint;
        if (fetch != nullptr && fetch->isActive && fetch->isActive() && fetch->isUsing && fetch->isUsing(wp.get()) && fetch->deactivate) fetch->deactivate();
        L::FinePrintEnvironment::Post(Fmt("#autoLOC_285932", {ContextualAnomalyName()}), 5.0f, L::ScreenMessageStyle::UPPER_LEFT);
    }
    wp->isExplored = true;
    WpRemove(wp.get());
    submittedWaypoint = false;
    SetComplete();
}
// SurveyWaypointParameter.cs:325
void SurveyWaypointParameter::Bounce() {
    if (submittedWaypoint) {
        std::string text = wp->name;
        if (wp->isClustered) text = text + " " + L::StringUtilities::IntegerToGreek(wp->index);
        L::FinePrintEnvironment::Post(Fmt("#autoLOC_285955", {text, ContextualAnomalyName()}), 5.0f, L::ScreenMessageStyle::UPPER_LEFT);
        const double num = M::Max(ksp::FinePrint::Contracts::SurveyContract::ParamRange(Root()->Prestige(), targetBody, band), DefD("Survey", "", "MinimumTriggerRange"));
        auto waypoint = std::make_unique<Waypoint>(*wp);
        waypoint->index++;
        waypoint->RandomizeAwayFrom(bounceOriginLatitude, bounceOriginLongitude, num, num * 2.0, 3, !waypoint->landLocked);
        wp->isExplored = true;
        WpRemove(wp.get());
        submittedWaypoint = false;
        auto added = std::make_unique<SurveyWaypointParameter>(experiment, actionDescription, targetBody, *waypoint, band, contextual, bouncesLeft - 1,
                                                               bounceOriginLatitude, bounceOriginLongitude);
        auto* surveyWaypointParameter = dynamic_cast<SurveyWaypointParameter*>(Root()->AddParameter(std::move(added)));
        if (surveyWaypointParameter != nullptr) {
            UpdateNavWaypoint(surveyWaypointParameter->wp.get());
            surveyWaypointParameter->clusterState = clusterState;
            surveyWaypointParameter->Register();
            surveyWaypointParameter->FundsCompletion = FundsCompletion;
            surveyWaypointParameter->ScienceCompletion = ScienceCompletion;
            surveyWaypointParameter->ReputationCompletion = ReputationCompletion;
            surveyWaypointParameter->ProcessWaypoint();
            SetComplete();
        }
    }
}
// SurveyWaypointParameter.cs:356
double SurveyWaypointParameter::TriggerRange(FlightBand band) {
    const double minimumTriggerRange = DefD("Survey", "", "MinimumTriggerRange");
    double num = DefD("Survey", "", "MaximumTriggerRange");
    if (minimumTriggerRange == num) num += 1.0;
    const double num2 = M::Min(minimumTriggerRange, num);
    const double num3 = M::Max(minimumTriggerRange, num);
    switch (band) {
        default: return 0.0;
        case FlightBand::GROUND: return num2;
        case FlightBand::LOW: return (num2 + num3) / 2.0;
        case FlightBand::HIGH: return num3;
    }
}
// SurveyWaypointParameter.cs:379
void SurveyWaypointParameter::ProcessWaypoint() {
    wp->celestialName = targetBody->GetName();
    wp->isOnSurface = true;
    wp->isNavigatable = true;
    wp->contractReference = Root();
    wp->SetFadeRange();
    if (band == FlightBand::GROUND && clusterState != WaypointClusterState::SINGLE) {
        wp->isClustered = true;
        wp->setName(false, !contextual);
    } else {
        wp->setName(true, !contextual);
    }
    const double terrainAltitude = L::CelestialUtilities::TerrainAltitude(targetBody, wp->latitude, wp->longitude);
    CalculateAltitudes(terrainAltitude);
    if (!verified) VerifyWaypoint(terrainAltitude);
    wp->nodeCaption2 = Fmt("#autoLOC_286029");
    switch (band) {
        default: wp->nodeCaption2 += KSPUtil::PrintSI(thresholdAltitude, Fmt("#autoLOC_7001411")); break;
        case FlightBand::GROUND: wp->nodeCaption2 += Fmt("#autoLOC_286034"); break;
        case FlightBand::LOW: wp->nodeCaption2 += Fmt("#autoLOC_7003013", {KSPUtil::PrintSI(thresholdAltitude, Fmt("#autoLOC_7001411"))}); break;
        case FlightBand::HIGH: wp->nodeCaption2 += Fmt("#autoLOC_7003012", {KSPUtil::PrintSI(thresholdAltitude, Fmt("#autoLOC_7001411"))}); break;
    }
    if (!submittedWaypoint && Root() != nullptr) {
        if (Scene() == GameScenes::FLIGHT && Root()->ContractState() == Contract::State::Active && State() == ParameterState::Incomplete) {
            WpAdd(wp.get());
            submittedWaypoint = true;
        }
        if (Scene() == GameScenes::TRACKSTATION && State() == ParameterState::Incomplete &&
            (Root()->ContractState() == Contract::State::Active || (!Root()->IsFinished() && ContractDefB("DisplayOfferedWaypoints")))) {
            WpAdd(wp.get());
            submittedWaypoint = true;
        }
    }
}
// SurveyWaypointParameter.cs:432
void SurveyWaypointParameter::CalculateAltitudes(double terrainAltitude) {
    if (targetBody == nullptr || wp == nullptr) return;
    KSPRandom kSPRandom(wp->uniqueSeed());
    double num = M::Max(M::Min(static_cast<double>(DefF("Survey", "", "ThresholdDeviancy")), static_cast<double>(100.0f)), static_cast<double>(0.0f));
    if (num < 1.0) num *= 100.0;
    num = static_cast<double>(kSPRandom.Next(static_cast<int>(num))) / 100.0;
    thresholdAltitude = DefD("Survey", "", "MinimumThreshold") +
                        targetBody->GeeASL * (DefD("Survey", "", "MaximumThreshold") - DefD("Survey", "", "MinimumThreshold"));
    if (targetBody->atmosphere) {
        if (static_cast<double>(targetBody->scienceValues.flyingAltitudeThreshold) > terrainAltitude)
            thresholdAltitude = static_cast<double>(targetBody->scienceValues.flyingAltitudeThreshold) - terrainAltitude;
        if (band == FlightBand::HIGH) {
            ScienceExperiment* scienceExperiment = ResearchAndDevelopment::GetExperiment(experiment);
            if (scienceExperiment != nullptr && !scienceExperiment->IsAvailableWhile(ExperimentSituations::FlyingHigh, *targetBody))
                thresholdAltitude = targetBody->atmosphereDepth;
        }
    }
    switch (band) {
        default: wp->altitude = 0.0; break;
        case FlightBand::GROUND: wp->altitude = 0.0; break;
        case FlightBand::LOW: wp->altitude = thresholdAltitude - thresholdAltitude / 3.0; break;
        case FlightBand::HIGH: wp->altitude = thresholdAltitude + thresholdAltitude / 3.0; break;
    }
    num = thresholdAltitude * num;
    if (L::CoinFlip(&kSPRandom))
        thresholdAltitude -= num;
    else
        thresholdAltitude += num;
    thresholdAltitude += terrainAltitude;
    thresholdAltitude = M::Round(thresholdAltitude / 100.0) * 100.0;
}
// SurveyWaypointParameter.cs:489
void SurveyWaypointParameter::VerifyWaypoint(double terrainAltitude) {
    KSPRandom generator(wp->uniqueSeed());
    int num = 0;
    do {
        if (!L::ProgressUtilities::ExperimentPossibleAt(&experiment, targetBody, wp->latitude, wp->longitude, wp->altitude + terrainAltitude, terrainAltitude)) {
            WaypointManager::ChooseRandomPosition(wp->latitude, wp->longitude, targetBody->name(), true, false, &generator);
            terrainAltitude = L::CelestialUtilities::TerrainAltitude(targetBody, wp->latitude, wp->longitude);
            CalculateAltitudes(terrainAltitude);
            num++;
            continue;
        }
        verified = true;
        return;
    } while (num <= 25);
    log::Info("Contract Log: Survey unable to place " + experiment + " waypoint - the survey definition is misconfigured.");
}
// SurveyWaypointParameter.cs:510
std::string SurveyWaypointParameter::SiteString() {
    std::string text;
    if (clusterState == WaypointClusterState::FULL)
        text += Fmt("#autoLOC_285739", {L::StringUtilities::IntegerToGreek(wp->index)});
    else if (clusterState != WaypointClusterState::MIXED || band != FlightBand::GROUND)
        text += L::StringUtilities::GenerateSiteName(wp->uniqueSeed(), targetBody, wp->landLocked, !contextual);
    else
        text += L::StringUtilities::GenerateSiteName(wp->seed, targetBody, wp->landLocked, !contextual) + " " + L::StringUtilities::IntegerToGreek(wp->index);
    if (contextual) text += Fmt("#autoLOC_285746", {ContextualAnomalyName()});
    return text;
}
// SurveyWaypointParameter.cs:521
Waypoint* SurveyWaypointParameter::NextWaypointInCluster() {
    Waypoint* result = nullptr;
    double num = 1.7976931348623157e308;
    std::vector<ContractParameter*> all;
    Root()->AllParameters(all);
    for (ContractParameter* p : all) {
        auto* s = dynamic_cast<SurveyWaypointParameter*>(p);
        if (s != nullptr && s->State() == ParameterState::Incomplete && s != this) {
            const double num2 = L::CelestialUtilities::GreatCircleDistance(targetBody, wp->latitude, wp->longitude, s->wp->latitude, s->wp->longitude);
            if (num2 < num) {
                num = num2;
                result = s->wp.get();
            }
        }
    }
    return result;
}
// SurveyWaypointParameter.cs:541
void SurveyWaypointParameter::UpdateNavWaypoint(Waypoint* newWaypoint) {
    auto* fetch = L::FinePrintEnvironment::navWaypoint;
    if (fetch != nullptr && fetch->isActive && fetch->isActive() && fetch->isUsing && fetch->isUsing(wp.get())) {
        if (newWaypoint == nullptr) {
            if (fetch->deactivate) fetch->deactivate();
            return;
        }
        const std::string text = newWaypoint->isClustered ? (newWaypoint->name + " " + L::StringUtilities::IntegerToGreek(newWaypoint->index)) : newWaypoint->name;
        L::FinePrintEnvironment::Post(Fmt("#autoLOC_286222", {text}), 2.5f, L::ScreenMessageStyle::UPPER_CENTER);
        if (fetch->setup) fetch->setup(newWaypoint);
    }
}

}  // namespace ksp::Contracts::Parameters
