// WP-37: the small, stateless FinePrint helpers that the WP-37 files call (FinePrint.Utilities.SystemUtilities / StringUtilities /
// CelestialUtilities / ProgressUtilities, WP-38 owns the full classes). Ported here from decompiled/KSP-Decompiled/Assembly-CSharp/FinePrint/
// Utilities/*.cs so the contract templates run on real data; WP-38 replaces them by the full port (same signatures where possible).
// Everything here lives in ksp::Contracts::Lite so it cannot clash with the WP-38 names.
#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/contracts/contract.hpp"
#include "ksp/contracts/contract_environment.hpp"
#include <memory>
#include "ksp/math/vector3d.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/enum_parse.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/random.hpp"
#include "ksp/parts/available_part.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/save/proto_vessel.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/util/ksp_util.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp::Contracts::Lite {

// FinePrint/Utilities/StringUtilities.cs:215 ShortName
inline std::string ShortName(const std::string& verbose) {
    const size_t p = verbose.rfind('.');
    return p == std::string::npos ? verbose : verbose.substr(p + 1);
}

namespace detail {
template <class T>
bool TryConvert(const std::string& input, T& value) {
    if constexpr (std::is_enum_v<T>) {
        try {
            value = dotnet::EnumParseFull<T>(input);
            return true;
        } catch (const std::exception&) {
            value = T{};
            return false;
        }
    } else if constexpr (std::is_same_v<T, bool>) {
        bool b = false;
        const bool ok = dotnet::TryParseBool(input, b);
        value = ok ? b : false;
        return ok;
    } else if constexpr (std::is_same_v<T, double>) {
        return dotnet::TryParseDouble(input, value);
    } else if constexpr (std::is_same_v<T, float>) {
        return dotnet::TryParseSingle(input, value);
    } else if constexpr (std::is_same_v<T, int32_t>) {
        return dotnet::TryParseInt32(input, value);
    } else if constexpr (std::is_same_v<T, uint32_t>) {
        return dotnet::TryParseUInt32(input, value);
    } else if constexpr (std::is_same_v<T, std::string>) {
        value = input;
        return true;
    } else if constexpr (std::is_same_v<T, Guid>) {
        return Guid::TryParse(input, value);
    }
}
}  // namespace detail

// FinePrint/Utilities/SystemUtilities.cs:113 LoadNode<T>(node, className, valueName, ref value, defaultValue, logging): value = the parsed
// node value (Convert.ChangeType / Enum.Parse), the default when the node is null, has no such value or the value is invalid.
template <class T>
void LoadNode(ConfigNode* node, const char* className, const char* valueName, T& value, const T& defaultValue, bool logging = true) {
    enum class R { Null, Success, Invalid, NoValue } result = R::Null;
    std::string name;
    if (node != nullptr) {
        name = ShortName(valueName);
        if (node->HasValue(name))
            result = detail::TryConvert<T>(*node->GetValue(name), value) ? R::Success : R::Invalid;
        else
            result = R::NoValue;
    }
    if (result == R::Success) return;
    if (logging) {
        if (result == R::Null) log::Warning(std::string("Contract Log: ") + className + " cannot load " + valueName + " from a null node.");
        else if (result == R::NoValue) log::Warning(std::string("Contract Log: ") + className + " cannot load " + valueName + ", it is not in the node.");
        else log::Warning(std::string("Contract Log: ") + className + " parsed an invalid value from " + valueName + ".");
    }
    value = defaultValue;
}
template <class T>
void LoadNode(ConfigNode& node, const char* className, const char* valueName, T& value, const T& defaultValue, bool logging = true) {
    LoadNode(&node, className, valueName, value, defaultValue, logging);
}

// SystemUtilities.cs:113 LoadNode<CelestialBody>: the node value is a FlightGlobals.Bodies index (WP-38: the typeof(T) == typeof(CelestialBody) branch)
void LoadNode(ConfigNode* node, const char* className, const char* valueName, CelestialBody*& value, CelestialBody* defaultValue, bool logging = true);
inline void LoadNode(ConfigNode& node, const char* className, const char* valueName, CelestialBody*& value, CelestialBody* defaultValue, bool logging = true) {
    LoadNode(&node, className, valueName, value, defaultValue, logging);
}

// SystemUtilities.cs:219 FlightIsReady(checkVessel, targetBody)
inline bool FlightIsReady(bool checkVessel = false, CelestialBody* targetBody = nullptr) {
    if (SaveEnvironment::Current().loadedScene != GameScenes::FLIGHT) return false;
    if (!FlightRegistry::ready) return false;
    if (!checkVessel) return true;
    if (FlightRegistry::ActiveVessel == nullptr) return false;
    if (targetBody == nullptr) return true;
    return FlightRegistry::ActiveVessel->mainBody() == targetBody;
}
// SystemUtilities.cs:244 FlightIsReady(currentState, targetState, checkVessel, targetBody)
inline bool FlightIsReady(Contract::State currentState, Contract::State targetState, bool checkVessel = false, CelestialBody* targetBody = nullptr) {
    if (currentState != targetState) return false;
    return FlightIsReady(checkVessel, targetBody);
}

// SystemUtilities.cs:273 CoinFlip(generator): a null generator is `new KSPRandom()` (tick-count seeded)
inline bool CoinFlip(KSPRandom* generator = nullptr) {
    if (generator != nullptr) return generator->Next(2) == 1;
    KSPRandom fresh(ContractsEnvironment::Current().NewRandomSeed());
    return fresh.Next(2) == 1;
}

// SystemUtilities.cs:287 SuperSeed(c): KSPUtil.GenerateSuperSeed(c.ContractGuid, c.MissionSeed)
inline int SuperSeed(Contract& c) { return KSPUtil::GenerateSuperSeed(c.ContractGuid(), c.MissionSeed()); }

// SystemUtilities.cs:279 HashNumber
inline uint32_t HashNumber(uint32_t x) {
    x = ((x >> 16) ^ x) * 73244475u;
    x = ((x >> 16) ^ x) * 73244475u;
    x = (x >> 16) ^ x;
    return x;
}

// VesselUtilities.cs:1282 GetPartName
inline std::string GetPartName(Part* p) {
    if (p != nullptr && p->partInfo != nullptr) return p->partInfo->name;
    return "";
}

// VesselUtilities.cs:36 ActiveVesselFallback(ref v, logging)
inline bool ActiveVesselFallback(Vessel*& v, bool logging = true) {
    if (v == nullptr) {
        if (SaveEnvironment::Current().loadedScene != GameScenes::FLIGHT || !FlightRegistry::ready || FlightRegistry::ActiveVessel == nullptr) {
            if (logging) log::Warning("Contract Log: Vessel check cannot fall back to active vessel.");
            return false;
        }
        v = FlightRegistry::ActiveVessel;
    }
    return true;
}

// VesselUtilities.cs:53 VesselHasPartName(partName, v)
inline bool VesselHasPartName(std::string partName, Vessel* v = nullptr) {
    if (!ActiveVesselFallback(v)) return false;
    for (char& c : partName)
        if (c == '_') c = '.';
    if (v->loaded) {
        int count = static_cast<int>(v->Parts().size());
        while (count-- > 0)
            if (GetPartName(v->Parts()[static_cast<size_t>(count)]) == partName) return true;
    } else if (v->protoVessel != nullptr) {
        int count2 = static_cast<int>(v->protoVessel->protoPartSnapshots.size());
        while (count2-- > 0)
            if (v->protoVessel->protoPartSnapshots[static_cast<size_t>(count2)]->partName == partName) return true;
    } else if (!v->Parts().empty()) {
        int count3 = static_cast<int>(v->Parts().size());
        while (count3-- > 0)
            if (GetPartName(v->Parts()[static_cast<size_t>(count3)]) == partName) return true;
    }
    return false;
}

// ---- FinePrint.Utilities.ProgressType / MannedStatus / ProgressMilestone / ProgressUtilities (the part the templates use) --------------------

using ProgressType = FinePrint::Utilities::ProgressType;  // ProgressType.cs:3 (declared by WP-36 in career_environment.hpp)
using ProgressRewardType = FinePrint::Utilities::ProgressRewardType;  // ProgressRewardType.cs:3

// MannedStatus.cs:3
enum class MannedStatus : int { MANNED, UNMANNED, ANY };

// ProgressMilestone.cs:5
class ProgressMilestone {
public:
    CelestialBody* body = nullptr;
    ProgressType type = ProgressType::SPLASHDOWN;
    MannedStatus manned = MannedStatus::ANY;
    ProgressMilestone();                                                                    // :234
    ProgressMilestone(CelestialBody* body_, ProgressType type_, MannedStatus manned_ = MannedStatus::ANY);  // :234
    KSPAchievements::CelestialBodySubtree* progress();                                      // :15
    bool complete();                                                                        // :46
    bool possible() const;                                                                  // :122
    bool bodySensitive() const;                                                             // :154
    bool crewSensitive() const;                                                             // :186
    bool impliesManned() const;                                                             // :216

private:
    bool CheckCompletion(ProgressNode* node) const;                                         // :248
    bool CheckCompletion(const std::string& nodeID) const;                                  // :248
    KSPAchievements::CelestialBodySubtree* progressTree = nullptr;
};

namespace ProgressUtilities {
using Where = std::function<bool(CelestialBody*)>;
bool GetBodyProgress(ProgressType progress, CelestialBody* body, MannedStatus manned = MannedStatus::ANY);   // ProgressUtilities.cs:94
bool GetAnyBodyProgress(CelestialBody* body, MannedStatus manned = MannedStatus::ANY);                        // :99
// :121 GetBodiesProgress(type, bodyReached, progressComplete, manned, where, bodies)
std::vector<CelestialBody*> GetBodiesProgress(ProgressType type, bool bodyReached, bool progressComplete, MannedStatus manned, const Where& where = nullptr,
                                              const std::vector<CelestialBody*>* bodies = nullptr);
// :147 (manned = ANY)
std::vector<CelestialBody*> GetBodiesProgress(ProgressType type, bool bodyReached, bool progressComplete, const Where& where = nullptr,
                                              const std::vector<CelestialBody*>* bodies = nullptr);
// :153 GetBodiesProgress(type, progressComplete, manned, where, bodies)
std::vector<CelestialBody*> GetBodiesProgressNoReach(ProgressType type, bool progressComplete, MannedStatus manned, const Where& where = nullptr,
                                                     const std::vector<CelestialBody*>* bodies = nullptr);
// :171 GetBodiesProgress(type, progressComplete, where, bodies)
std::vector<CelestialBody*> GetBodiesProgressNoReach(ProgressType type, bool progressComplete, const Where& where = nullptr,
                                                     const std::vector<CelestialBody*>* bodies = nullptr);
// :176 GetBodiesProgress(bodyReached, manned, where, bodies)
std::vector<CelestialBody*> GetBodiesReached(bool bodyReached, MannedStatus manned, const Where& where = nullptr, const std::vector<CelestialBody*>* bodies = nullptr);
std::vector<CelestialBody*> GetBodiesReached(bool bodyReached, const Where& where = nullptr, const std::vector<CelestialBody*>* bodies = nullptr);  // (C++ addition)
// :242 CelestialCrewCounts(situations)
std::map<CelestialBody*, int> CelestialCrewCounts(const std::vector<Vessel::Situations>& situations);
// :193 GetNextUnreached(count, manned, where) and :237 (manned = ANY)
std::vector<CelestialBody*> GetNextUnreached(int count, MannedStatus manned, const Where& where = nullptr);
std::vector<CelestialBody*> GetNextUnreached(int count, const Where& where = nullptr);
float AverageFacilityLevel();                       // :305
int GetProgressLevel();                             // :310
float ScoreProgressType(ProgressType type, CelestialBody* body);  // :372
// WP-38 (full port of the remaining ProgressUtilities members)
bool IntroWorldFirstContract(ProgressMilestone& milestone);                       // :338
bool OutlierWorldFirstContract(ProgressType type, CelestialBody* body);           // :350
bool OutlierWorldFirstContract(ProgressMilestone& milestone);                     // :367
ContractPrestige ProgressTypePrestige(ProgressType type, CelestialBody* body = nullptr);  // :437
ContractPrestige ProgressTypePrestige(ProgressMilestone& milestone);             // :467
float WorldFirstStandardReward(ProgressRewardType reward, Currency currency, ProgressType progress, CelestialBody* body = nullptr);  // :488
float WorldFirstIntervalReward(ProgressRewardType reward, Currency currency, ProgressType progress, CelestialBody* body = nullptr, int currentInterval = 1,
                               int totalIntervals = 10);                          // :530
bool ReachedHomeBodies();                                                         // :574
double FindNextRecord(double currentRecord, double maximumRecord, double roundValue);                  // :603
double FindNextRecord(double currentRecord, double maximumRecord, double roundValue, int& interval);  // :624
bool VisitedSurfaceOf(CelestialBody* body, MannedStatus manned = MannedStatus::ANY);  // :645
}  // namespace ProgressUtilities

namespace StringUtilities {
std::string AlphaNumericDesignation(int seed);                       // StringUtilities.cs:189
std::string ThisThisAndThat(const std::vector<std::string>& stringList, const std::string& connector = "#autoLOC_6002373",
                            const std::string& conjugation = "#autoLOC_6002374");  // :226
std::string ShortKerbalName(const std::string& fullKerbalName);      // :250
std::string PossessiveString(std::string thing);                     // :220
}  // namespace StringUtilities

// SystemUtilities.cs (WP-38: the pure maths / list helpers)
namespace SystemUtilities {
bool WithinDeviation(double v1, double v2, double deviation);                    // :519
bool WithinDeviationByValue(double v1, double v2, double deviation, double fullValue);  // :525
double MeasureDeviation(double v1, double v2);                                   // :531
double MeasureDeviationByValue(double v1, double v2, double fullValue);          // :536
double WithinDeviationAccuracy(double v1, double v2, double deviation);          // :549
double WithinDeviationByReferenceAccuracy(double v1, double v2, double reference, double deviation);  // :557
// :565 ShuffleList(ref list, generator): generator null -> new KSPRandom()
template <class T>
void ShuffleList(std::vector<T>& list, KSPRandom* generator = nullptr);
}  // namespace SystemUtilities

namespace CelestialUtilities {
bool IsGasGiant(CelestialBody* body);                         // CelestialUtilities.cs:272
bool IsFlyablePlanet(CelestialBody* body);                    // :281
double GetAltitudeForDensity(CelestialBody* body, double density);  // :220
std::vector<CelestialBody*> ChildrenOf(CelestialBody* parent);  // :99
// WP-38: the rest of CelestialUtilities.cs
CelestialBody* RandomBody(const std::vector<CelestialBody*>& bodies);          // :10 (new KSPRandom(): ContractsEnvironment::newRandomSeed)
CelestialBody* HighestBody(const std::vector<CelestialBody*>& bodies);         // :16
CelestialBody* LowestBody(const std::vector<CelestialBody*>& bodies);          // :44
std::vector<CelestialBody*> GetNeighbors(CelestialBody* body, const std::function<bool(CelestialBody*)>& where = nullptr);  // :72
// :111 GetHighestPeak: the PQSMod reflection walk (deformity / heightMapDeformity / heightMapOffset). Unset: 0 (no PQS mod sum available).
extern std::function<double(CelestialBody*)> highestPeakProvider;
double GetHighestPeak(CelestialBody* body);                                    // :111
double GetNextTimeWarp(CelestialBody* body, double altitude);                  // :148
double GetMinimumOrbitalDistance(CelestialBody* body, float margin);           // :184
Vessel::Situations ApplicableSituation(int seed, CelestialBody* body, bool splashAllowed);  // :243
float PlanetScienceRanking(CelestialBody* body);                               // :292
Vector3f LLAtoECEF(double lat, double lon, double alt, double radius);         // :306
double SynchronousSMA(CelestialBody* body);                                    // :314
bool CanBodyBeSynchronous(CelestialBody* body, double eccentricity);           // :324
double KolniyaSMA(CelestialBody* body);                                        // :339
bool CanBodyBeKolniya(CelestialBody* body);                                    // :350
bool CanBodyBeTundra(CelestialBody* body);                                     // :367
double TerrainAltitude(CelestialBody* body, double latitude, double longitude, bool underwater = false);  // :383
double GetSolarExtents();                                                      // :432
double GreatCircleDistance(CelestialBody* body, double latitude1, double longitude1, double latitude2, double longitude2);  // :446
double GreatCircleDistance(CelestialBody* body, Vector3d position1, Vector3d position2);  // :451
CelestialBody* GetHostPlanet(CelestialBody* body);                             // :456
std::vector<CelestialBody*> GetPlanetarySystem(CelestialBody* body);           // :466
}  // namespace CelestialUtilities

namespace SystemUtilities {
template <class T>
void ShuffleList(std::vector<T>& list, KSPRandom* generator) {
    std::unique_ptr<KSPRandom> own;
    if (generator == nullptr) {
        own = std::make_unique<KSPRandom>(ContractsEnvironment::Current().NewRandomSeed());
        generator = own.get();
    }
    for (int num = static_cast<int>(list.size()) - 1; num > 0; num--) {
        int index = generator->Next(0, num + 1);
        T value = list[static_cast<size_t>(num)];
        list[static_cast<size_t>(num)] = list[static_cast<size_t>(index)];
        list[static_cast<size_t>(index)] = value;
    }
}
}  // namespace SystemUtilities

}  // namespace ksp::Contracts::Lite

template <>
struct ksp::dotnet::EnumTraits<ksp::FinePrint::Utilities::ProgressType> {
    using E = ksp::FinePrint::Utilities::ProgressType;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {
        {E::NULL_, "NULL"}, {E::ALTITUDERECORD, "ALTITUDERECORD"}, {E::BASECONSTRUCTION, "BASECONSTRUCTION"}, {E::CREWTRANSFER, "CREWTRANSFER"},
        {E::CREWRECOVERY, "CREWRECOVERY"}, {E::DEPTHRECORD, "DEPTHRECORD"}, {E::DISTANCERECORD, "DISTANCERECORD"}, {E::DOCKING, "DOCKING"},
        {E::ESCAPE, "ESCAPE"}, {E::FIRSTLAUNCH, "FIRSTLAUNCH"}, {E::FLAGPLANT, "FLAGPLANT"}, {E::FLIGHT, "FLIGHT"}, {E::FLYBY, "FLYBY"},
        {E::FLYBYRETURN, "FLYBYRETURN"}, {E::LANDING, "LANDING"}, {E::LANDINGRETURN, "LANDINGRETURN"}, {E::ORBIT, "ORBIT"},
        {E::ORBITRETURN, "ORBITRETURN"}, {E::POINTOFINTEREST, "POINTOFINTEREST"}, {E::REACHSPACE, "REACHSPACE"}, {E::RENDEZVOUS, "RENDEZVOUS"},
        {E::SCIENCE, "SCIENCE"}, {E::SPACEWALK, "SPACEWALK"}, {E::SPEEDRECORD, "SPEEDRECORD"}, {E::SPLASHDOWN, "SPLASHDOWN"},
        {E::STATIONCONSTRUCTION, "STATIONCONSTRUCTION"}, {E::STUNT, "STUNT"}, {E::SUBORBIT, "SUBORBIT"}, {E::SURFACEEVA, "SURFACEEVA"}};
};
