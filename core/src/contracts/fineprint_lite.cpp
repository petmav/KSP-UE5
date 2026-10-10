// WP-37: ProgressMilestone / ProgressUtilities / CelestialUtilities (FinePrint/Utilities/*.cs): the part the contract templates use.
#include "ksp/contracts/fineprint_lite.hpp"

#include "ksp/career/career.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/planetarium.hpp"
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

}  // namespace ProgressUtilities

namespace CelestialUtilities {

// CelestialUtilities.cs:272
bool IsGasGiant(CelestialBody* body) {
    if (body == nullptr) return false;
    return !body->hasSolidSurface;
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
