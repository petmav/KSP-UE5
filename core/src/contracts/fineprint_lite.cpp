// WP-37: ProgressMilestone / ProgressUtilities / CelestialUtilities (FinePrint/Utilities/*.cs): the part the contract templates use.
#include "ksp/contracts/fineprint_lite.hpp"

#include "ksp/career/career.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/contracts/templates.hpp"
#include "ksp/dotnet/collections.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/kerbals/crew_generator.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/save/proto_vessel.hpp"

namespace ksp::Contracts::Lite {

// ---- ProgressMilestone ------------------------------------------------------------------------------------------------------

// ProgressMilestone.cs:234
ProgressMilestone::ProgressMilestone() : body(Planetarium::fetch->Sun), type(ProgressType::SPLASHDOWN), manned(MannedStatus::ANY) {}
// ProgressMilestone.cs:234
ProgressMilestone::ProgressMilestone(CelestialBody* body_, ProgressType type_, MannedStatus manned_) : body(body_), type(type_), manned(manned_) {}

// ProgressMilestone.cs:15
KSPAchievements::CelestialBodySubtree* ProgressMilestone::progress() {
    if (progressTree != nullptr) return progressTree;
    if (body == nullptr) return nullptr;
    progressTree = body->progressTree;
    if (progressTree != nullptr) return progressTree;
    int num = static_cast<int>(ProgressTracking::Instance()->celestialBodyNodes.size());
    while (num-- > 0) {
        KSPAchievements::CelestialBodySubtree* sub = ProgressTracking::Instance()->celestialBodyNodes[static_cast<size_t>(num)];
        if (!(sub->Body != body)) {
            progressTree = sub;
            break;
        }
    }
    return progressTree;
}

// ProgressMilestone.cs:46
bool ProgressMilestone::complete() {
    if (!possible()) return false;
    switch (type) {
        case ProgressType::SPEEDRECORD: return CheckCompletion("RecordsSpeed");
        case ProgressType::REACHSPACE: return CheckCompletion("ReachedSpace");
        case ProgressType::ALTITUDERECORD: return CheckCompletion("RecordsAltitude");
        case ProgressType::CREWRECOVERY: return CheckCompletion("FirstCrewToSurvive");
        case ProgressType::DEPTHRECORD: return CheckCompletion("RecordsDepth");
        case ProgressType::DISTANCERECORD: return CheckCompletion("RecordsDistance");
        case ProgressType::FIRSTLAUNCH: return CheckCompletion("FirstLaunch");
        default: break;
    }
    KSPAchievements::CelestialBodySubtree* p = progress();
    if (p != nullptr && p->IsReached()) {
        switch (type) {
            case ProgressType::BASECONSTRUCTION: return CheckCompletion(p->baseConstruction);
            case ProgressType::CREWTRANSFER: return CheckCompletion(p->crewTransfer);
            case ProgressType::DOCKING: return CheckCompletion(p->docking);
            case ProgressType::ESCAPE: return CheckCompletion(p->escape);
            case ProgressType::FLAGPLANT: return CheckCompletion(p->flagPlant);
            case ProgressType::FLIGHT: return CheckCompletion(p->flight);
            case ProgressType::FLYBY: return CheckCompletion(p->flyBy);
            case ProgressType::FLYBYRETURN: return CheckCompletion(p->returnFromFlyby);
            case ProgressType::LANDING: return CheckCompletion(p->landing);
            case ProgressType::LANDINGRETURN: return CheckCompletion(p->returnFromSurface);
            case ProgressType::ORBIT: return CheckCompletion(p->orbit);
            case ProgressType::ORBITRETURN: return CheckCompletion(p->returnFromOrbit);
            case ProgressType::RENDEZVOUS: return CheckCompletion(p->rendezvous);
            case ProgressType::SCIENCE: return CheckCompletion(p->science);
            case ProgressType::SPACEWALK: return CheckCompletion(p->spacewalk);
            case ProgressType::SPLASHDOWN: return CheckCompletion(p->splashdown);
            case ProgressType::STATIONCONSTRUCTION: return CheckCompletion(p->stationConstruction);
            case ProgressType::SUBORBIT: return CheckCompletion(p->suborbit);
            case ProgressType::SURFACEEVA: return CheckCompletion(p->surfaceEVA);
            default: return false;
        }
    }
    return false;
}

// ProgressMilestone.cs:122
bool ProgressMilestone::possible() const {
    if (type == ProgressType::SPLASHDOWN && (body == nullptr || !body->ocean)) return false;
    if (type == ProgressType::FLIGHT && (body == nullptr || !body->atmosphere)) return false;
    if (manned == MannedStatus::UNMANNED && impliesManned()) return false;
    if (CelestialUtilities::IsGasGiant(body)) {
        switch (type) {
            case ProgressType::BASECONSTRUCTION:
            case ProgressType::FLAGPLANT:
            case ProgressType::LANDING:
            case ProgressType::LANDINGRETURN:
            case ProgressType::SURFACEEVA: return false;
            default: break;
        }
    }
    return true;
}

// ProgressMilestone.cs:154
bool ProgressMilestone::bodySensitive() const {
    switch (type) {
        case ProgressType::BASECONSTRUCTION:
        case ProgressType::CREWTRANSFER:
        case ProgressType::DOCKING:
        case ProgressType::ESCAPE:
        case ProgressType::FLAGPLANT:
        case ProgressType::FLIGHT:
        case ProgressType::FLYBY:
        case ProgressType::FLYBYRETURN:
        case ProgressType::LANDING:
        case ProgressType::LANDINGRETURN:
        case ProgressType::ORBIT:
        case ProgressType::ORBITRETURN:
        case ProgressType::RENDEZVOUS:
        case ProgressType::SCIENCE:
        case ProgressType::SPACEWALK:
        case ProgressType::SPLASHDOWN:
        case ProgressType::STATIONCONSTRUCTION:
        case ProgressType::SUBORBIT:
        case ProgressType::SURFACEEVA: return true;
        default: return false;
    }
}

// ProgressMilestone.cs:186
bool ProgressMilestone::crewSensitive() const {
    switch (type) {
        case ProgressType::BASECONSTRUCTION:
        case ProgressType::DOCKING:
        case ProgressType::ESCAPE:
        case ProgressType::FIRSTLAUNCH:
        case ProgressType::FLIGHT:
        case ProgressType::FLYBY:
        case ProgressType::FLYBYRETURN:
        case ProgressType::LANDING:
        case ProgressType::LANDINGRETURN:
        case ProgressType::ORBIT:
        case ProgressType::ORBITRETURN:
        case ProgressType::REACHSPACE:
        case ProgressType::RENDEZVOUS:
        case ProgressType::SCIENCE:
        case ProgressType::SPLASHDOWN:
        case ProgressType::STATIONCONSTRUCTION:
        case ProgressType::SUBORBIT: return true;
        default: return false;
    }
}

// ProgressMilestone.cs:216
bool ProgressMilestone::impliesManned() const {
    switch (type) {
        case ProgressType::CREWTRANSFER:
        case ProgressType::CREWRECOVERY:
        case ProgressType::FLAGPLANT:
        case ProgressType::SPACEWALK:
        case ProgressType::SURFACEEVA: return true;
        default: return false;
    }
}

// ProgressMilestone.cs:248
bool ProgressMilestone::CheckCompletion(ProgressNode* node) const {
    if (node == nullptr) return false;
    switch (manned) {
        default: return node->IsComplete();
        case MannedStatus::UNMANNED: return node->IsCompleteUnmanned();
        case MannedStatus::MANNED: return node->IsCompleteManned();
    }
}

// ProgressMilestone.cs:248
bool ProgressMilestone::CheckCompletion(const std::string& nodeID) const {
    if (ProgressTracking::Instance() == nullptr) return false;
    switch (manned) {
        default: return ProgressTracking::Instance()->NodeComplete({nodeID});
        case MannedStatus::UNMANNED: return ProgressTracking::Instance()->NodeCompleteUnmanned({nodeID});
        case MannedStatus::MANNED: return ProgressTracking::Instance()->NodeCompleteManned({nodeID});
    }
}

// ---- ProgressUtilities ------------------------------------------------------------------------------------------------------

namespace ProgressUtilities {

// ProgressUtilities.cs:94
bool GetBodyProgress(ProgressType progress, CelestialBody* body, MannedStatus manned) { return ProgressMilestone(body, progress, manned).complete(); }

// ProgressUtilities.cs:99
bool GetAnyBodyProgress(CelestialBody* body, MannedStatus manned) {
    if (body->isHomeWorld) return true;
    // Enum.GetValues(typeof(ProgressType)): NULL .. SURFACEEVA, walked from the end
    constexpr int count = static_cast<int>(ProgressType::SURFACEEVA) + 1;
    ProgressMilestone progressMilestone(body, ProgressType::ORBIT, manned);
    int num = count;
    do {
        if (num-- > 0) {
            progressMilestone.type = static_cast<ProgressType>(num);
            continue;
        }
        return false;
    } while (!progressMilestone.bodySensitive() || (manned != MannedStatus::ANY && !progressMilestone.crewSensitive() && !progressMilestone.impliesManned()) ||
             !progressMilestone.complete());
    return true;
}

namespace {
const std::vector<CelestialBody*>& BodiesOr(const std::vector<CelestialBody*>* bodies) { return bodies != nullptr ? *bodies : FlightRegistry::Bodies(); }
}  // namespace

// ProgressUtilities.cs:121
std::vector<CelestialBody*> GetBodiesProgress(ProgressType type, bool bodyReached, bool progressComplete, MannedStatus manned, const Where& where,
                                              const std::vector<CelestialBody*>* bodies) {
    const std::vector<CelestialBody*>& src = BodiesOr(bodies);
    std::vector<CelestialBody*> list;
    int count = static_cast<int>(src.size());
    while (count-- > 0) {
        CelestialBody* cb = src[static_cast<size_t>(count)];
        if (cb != nullptr && (!where || where(cb)) && GetAnyBodyProgress(cb, manned) == bodyReached && GetBodyProgress(type, cb, manned) == progressComplete) list.push_back(cb);
    }
    return list;
}

// ProgressUtilities.cs:147
std::vector<CelestialBody*> GetBodiesProgress(ProgressType type, bool bodyReached, bool progressComplete, const Where& where, const std::vector<CelestialBody*>* bodies) {
    return GetBodiesProgress(type, bodyReached, progressComplete, MannedStatus::ANY, where, bodies);
}

// ProgressUtilities.cs:153
std::vector<CelestialBody*> GetBodiesProgressNoReach(ProgressType type, bool progressComplete, MannedStatus manned, const Where& where,
                                                     const std::vector<CelestialBody*>* bodies) {
    const std::vector<CelestialBody*>& src = BodiesOr(bodies);
    std::vector<CelestialBody*> list;
    int count = static_cast<int>(src.size());
    while (count-- > 0) {
        CelestialBody* cb = src[static_cast<size_t>(count)];
        if (cb != nullptr && (!where || where(cb)) && GetBodyProgress(type, cb, manned) == progressComplete) list.push_back(cb);
    }
    return list;
}

// ProgressUtilities.cs:171
std::vector<CelestialBody*> GetBodiesProgressNoReach(ProgressType type, bool progressComplete, const Where& where, const std::vector<CelestialBody*>* bodies) {
    return GetBodiesProgressNoReach(type, progressComplete, MannedStatus::ANY, where, bodies);
}

// ProgressUtilities.cs:176
std::vector<CelestialBody*> GetBodiesReached(bool bodyReached, MannedStatus manned, const Where& where, const std::vector<CelestialBody*>* bodies) {
    const std::vector<CelestialBody*>& src = BodiesOr(bodies);
    std::vector<CelestialBody*> list;
    int count = static_cast<int>(src.size());
    while (count-- > 0) {
        CelestialBody* cb = src[static_cast<size_t>(count)];
        if (cb != nullptr && (!where || where(cb)) && GetAnyBodyProgress(cb, manned) == bodyReached) list.push_back(cb);
    }
    return list;
}

// ProgressUtilities.cs:193
std::vector<CelestialBody*> GetBodiesReached(bool bodyReached, const Where& where, const std::vector<CelestialBody*>* bodies) {
    return GetBodiesReached(bodyReached, MannedStatus::ANY, where, bodies);
}

// ProgressUtilities.cs:242
std::map<CelestialBody*, int> CelestialCrewCounts(const std::vector<Vessel::Situations>& situations) {
    std::map<CelestialBody*, int> dictionary;
    auto& protoVessels = SaveEnvironment::Current().currentGame->flightState->protoVessels;
    int count = static_cast<int>(protoVessels.size());
    while (count-- > 0) {
        ProtoVessel& protoVessel = *protoVessels[static_cast<size_t>(count)];
        const int count2 = static_cast<int>(protoVessel.GetVesselCrew().size());
        if (count2 == 0) continue;
        int count3 = static_cast<int>(situations.size());
        while (count3-- > 0) {
            if (protoVessel.situation == situations[static_cast<size_t>(count3)]) {
                CelestialBody* main = protoVessel.vesselRef->mainBody();
                auto it = dictionary.find(main);
                if (it != dictionary.end())
                    it->second += count2;
                else
                    dictionary[main] = count2;
            }
        }
    }
    return dictionary;
}

// ProgressUtilities.cs:193
std::vector<CelestialBody*> GetNextUnreached(int count, MannedStatus manned, const Where& where) {
    std::vector<CelestialBody*> stack;  // Stack<CelestialBody>: back = top
    std::vector<CelestialBody*> list;
    std::vector<CelestialBody*> list2;
    CelestialBody* sun = Planetarium::fetch->Sun;
    const auto byRecovery = [](CelestialBody* b1, CelestialBody* b2) {
        const float a = b1->scienceValues.RecoveryValue;
        const float b = b2->scienceValues.RecoveryValue;
        return a < b ? -1 : (a > b ? 1 : (a == b ? 0 : (std::isnan(a) ? (std::isnan(b) ? 0 : -1) : 1)));
    };
    if (sun != nullptr) {
        list.push_back(sun);
        for (CelestialBody* cb : sun->orbitingBodies) list.push_back(cb);
    }
    dotnet::ListSort(list, byRecovery);
    for (int num = static_cast<int>(list.size()) - 1; num >= 0; --num) stack.push_back(list[static_cast<size_t>(num)]);
    while (!stack.empty() && static_cast<int>(list2.size()) < count) {
        CelestialBody* celestialBody = stack.back();
        stack.pop_back();
        if (celestialBody == nullptr) continue;
        if (!GetAnyBodyProgress(celestialBody, manned) && (!where || where(celestialBody))) list2.push_back(celestialBody);
        if (!(celestialBody == sun)) {
            list = celestialBody->orbitingBodies;
            dotnet::ListSort(list, byRecovery);
            int count2 = static_cast<int>(list.size());
            while (count2-- > 0) stack.push_back(list[static_cast<size_t>(count2)]);
        }
    }
    return list2;
}

// ProgressUtilities.cs:237
std::vector<CelestialBody*> GetNextUnreached(int count, const Where& where) { return GetNextUnreached(count, MannedStatus::ANY, where); }

// ProgressUtilities.cs:305
float AverageFacilityLevel() {
    using F = ScenarioUpgradeableFacilities;
    return f32store((fx(F::GetFacilityLevel(SpaceCenterFacility::Administration)) + fx(F::GetFacilityLevel(SpaceCenterFacility::AstronautComplex)) +
                     fx(F::GetFacilityLevel(SpaceCenterFacility::LaunchPad)) + fx(F::GetFacilityLevel(SpaceCenterFacility::MissionControl)) +
                     fx(F::GetFacilityLevel(SpaceCenterFacility::ResearchAndDevelopment)) + fx(F::GetFacilityLevel(SpaceCenterFacility::Runway)) +
                     fx(F::GetFacilityLevel(SpaceCenterFacility::SpaceplaneHangar)) + fx(F::GetFacilityLevel(SpaceCenterFacility::TrackingStation)) +
                     fx(F::GetFacilityLevel(SpaceCenterFacility::VehicleAssemblyBuilding))) / 9.0);
}

// ProgressUtilities.cs:310
int GetProgressLevel() {
    int num = 0;
    const auto& bodies = FlightRegistry::Bodies();
    int count = static_cast<int>(bodies.size());
    while (count-- > 0) {
        CelestialBody* cb = bodies[static_cast<size_t>(count)];
        if (cb != nullptr && cb != Planetarium::fetch->Sun && cb != Planetarium::fetch->Home && ProgressTracking::Instance()->NodeReached({cb->name()})) ++num;
    }
    if (ProgressTracking::Instance()->NodeComplete({"FirstLaunch"})) ++num;
    if (ProgressTracking::Instance()->NodeComplete({Planetarium::fetch->Home->name(), "Orbit"})) ++num;
    const double ratio = fx(AverageFacilityLevel()) / fx(ContractDefsLite::F("", "", "FacilityProgressionFactor"));
    return num + dotnet::Math::Max(0, dotnet::ConvToInt32(dotnet::Math::Round(ratio)));
}

// ProgressUtilities.cs:372
float ScoreProgressType(ProgressType type, CelestialBody* body) {
    const bool flag = body == nullptr || body == Planetarium::fetch->Home;
    switch (type) {
        case ProgressType::ESCAPE: return !flag ? 0.6f : 1.1f;
        default: return 0.1f;
        case ProgressType::FLIGHT: return !flag ? 1.2f : 0.5f;
        case ProgressType::LANDING:
        case ProgressType::LANDINGRETURN: return !flag ? 1.5f : 0.1f;
        case ProgressType::ORBIT:
        case ProgressType::ORBITRETURN:
        case ProgressType::RENDEZVOUS: return !flag ? 0.8f : 1.0f;
        case ProgressType::SPACEWALK: return 0.6f;
        case ProgressType::ALTITUDERECORD:
        case ProgressType::DEPTHRECORD:
        case ProgressType::DISTANCERECORD:
        case ProgressType::SPEEDRECORD: return 2.0f;
        case ProgressType::SPLASHDOWN: return !flag ? 1.5f : 0.3f;
        case ProgressType::REACHSPACE:
        case ProgressType::STATIONCONSTRUCTION: return 0.8f;
        case ProgressType::BASECONSTRUCTION:
        case ProgressType::CREWTRANSFER:
        case ProgressType::DOCKING:
        case ProgressType::POINTOFINTEREST:
        case ProgressType::STUNT: return !flag ? 0.8f : 0.3f;
        case ProgressType::FLYBY:
        case ProgressType::FLYBYRETURN:
        case ProgressType::SUBORBIT: return !flag ? 1.0f : 0.8f;
        case ProgressType::FLAGPLANT:
        case ProgressType::SCIENCE:
        case ProgressType::SURFACEEVA: return !flag ? 0.6f : 0.2f;
    }
}

}  // namespace ProgressUtilities

namespace StringUtilities {

// StringUtilities.cs:189
std::string AlphaNumericDesignation(int seed) {
    const std::vector<std::string>& alphaNumeric = Templates::TemplateEnvironment::alphaNumeric;
    std::string text;
    KSPRandom kSPRandom(seed);
    bool flag = false;
    const int num = kSPRandom.Next(4, 7);
    for (int i = 1; i <= num; ++i) {
        if (kSPRandom.Next(101) > 70 && i != 1 && i != num && !flag) {
            text += "-";
            flag = true;
        } else {
            text += alphaNumeric.at(static_cast<size_t>(kSPRandom.Next(static_cast<int>(alphaNumeric.size()))));
        }
    }
    return text;
}

// StringUtilities.cs:226
std::string ThisThisAndThat(const std::vector<std::string>& stringList, const std::string& connector_, const std::string& conjugation_) {
    if (stringList.empty()) return "(NO ELEMENTS)";
    if (stringList.size() == 1) return stringList[0];
    std::string sb;
    const std::string connector = KSP::Localization::Localizer::Format(connector_);
    const std::string conjugation = KSP::Localization::Localizer::Format(conjugation_);
    for (size_t i = 0; i + 2 < stringList.size(); ++i) {
        sb += KSP::Localization::Localizer::Format(stringList[i]);
        sb += connector;
    }
    sb += KSP::Localization::Localizer::Format(stringList[stringList.size() - 2]);
    sb += conjugation;
    sb += KSP::Localization::Localizer::Format(stringList[stringList.size() - 1]);
    return sb;
}

// StringUtilities.cs:252
std::string ShortKerbalName(const std::string& fullKerbalName) { return CrewGenerator::RemoveLastName(fullKerbalName); }

}  // namespace StringUtilities

namespace CelestialUtilities {

// CelestialUtilities.cs:272
bool IsGasGiant(CelestialBody* body) {
    if (body == nullptr) return false;
    return !body->hasSolidSurface;
}

// CelestialUtilities.cs:281
bool IsFlyablePlanet(CelestialBody* body) {
    if (body != nullptr && body != Planetarium::fetch->Sun) return body->atmosphere;
    return false;
}

// CelestialUtilities.cs:220
double GetAltitudeForDensity(CelestialBody* body, double density) {
    if (body->atmosphere && body->GetDensity(body->GetPressure(0.0), body->GetTemperature(0.0)) >= density) {
        double num = 0.0;
        double num2 = body->atmosphereDepth;
        double num3 = 0.0;
        int num4 = 99;
        while (true) {
            if (num4 >= 0) {
                num3 = num + (num2 - num) * 0.5;
                const double density2 = body->GetDensity(body->GetPressure(num3), body->GetTemperature(num3));
                if (density2 == density) break;
                if (density2 > density)
                    num = num3;
                else
                    num2 = num3;
                num4--;
                continue;
            }
            return num3;
        }
        return num3;
    }
    return 0.0;
}

// CelestialUtilities.cs:99 (AddChildren)
std::vector<CelestialBody*> ChildrenOf(CelestialBody* parentBody) {
    std::vector<CelestialBody*> list;
    std::function<void(CelestialBody*)> add = [&](CelestialBody* parent) {
        int count = static_cast<int>(parent->orbitingBodies.size());
        while (count-- > 0) {
            CelestialBody* cb = parent->orbitingBodies[static_cast<size_t>(count)];
            list.push_back(cb);
            add(cb);
        }
    };
    add(parentBody);
    return list;
}

}  // namespace CelestialUtilities

}  // namespace ksp::Contracts::Lite
