// WP-38 slice 2: the remaining FinePrint.Utilities (Assembly-CSharp/FinePrint/Utilities/*.cs): VesselUtilities (complete), SystemUtilities
// (SaveNodeList / LoadNodeList / RandomSplitChoice / ProcessSideRequests), StringUtilities (site names, Greek map, delimited lists,
// pronouns, ValueAndUnits, ...), ProgressUtilities (HavePartTech / HaveModuleTech / HaveAnyTech / ExperimentPossibleAt / CurrentTrackRecord),
// the contract objective module table (PartModule.IsValidContractObjective) and FinePrintEnvironment, the seams of the FinePrint
// parameters and contracts.
//
// The slice-1 helpers live in ksp::Contracts::Lite (contracts/fineprint_lite.hpp, extended in place); this header extends the same namespace so
// the call sites read like the C# static classes. C# `protected` members are public (tests drive them).
//
// Seams (FinePrintEnvironment): ScreenMessages.PostScreenMessage (recorded), StartCoroutine (deferred queue, RunDeferred()), the
// SiteGeneratorInfo TextAsset (not in the repo: "needs data"), WaypointManager.Instance().LateralDistanceToVessel, the
// ContractOrbitRenderer, NavWaypoint.
#pragma once

#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "ksp/contracts/fineprint_lite.hpp"
#include "ksp/contracts/waypoints.hpp"
#include "ksp/fineprint/fineprint.hpp"
#include "ksp/dotnet/random.hpp"
#include "ksp/orbit/orbit.hpp"

namespace ksp::Contracts::Lite {

// FinePrint/Utilities/FlightBand.cs:3
enum class FlightBand : int { NONE, GROUND, LOW, HIGH, LOWMIX, HIGHMIX, ANYMIX };
// OrbitalProperties.cs:3
enum class OrbitalProperties : int { INC, ARG, LAN, ECC, SMA, PER, APO };
// WaypointClusterState.cs:3
enum class WaypointClusterState : int { NONE, SINGLE, MIXED, FULL };
// GrammaticalGender.cs:3 / PronounCasing.cs:3 / RecordTrackType.cs:3
enum class GrammaticalGender : int { NEUTRAL, MASCULINE, FEMININE };
enum class PronounCasing : int { SUBJECTIVE, OBJECTIVE, POSSESSIVE };
enum class RecordTrackType : int { NULL_, ALTITUDE, DEPTH, DISTANCE, SPEED };
// ScreenMessageStyle (UNITY UI, ScreenMessages.cs): only the member names the FinePrint code passes
enum class ScreenMessageStyle : int { UPPER_CENTER, UPPER_LEFT, UPPER_RIGHT, LOWER_CENTER };
// WheelType.cs:3
enum class WheelType : int { FREE, MOTORIZED, LEG };

struct FinePrintEnvironment {
    // ScreenMessages.PostScreenMessage(text, duration, style): every call is appended to screenMessages (in order); the seam is optional
    static inline std::function<void(const std::string& text, float duration, ScreenMessageStyle style)> postScreenMessage;
    static inline std::vector<std::string> screenMessages;
    // ContractSystem.Instance.StartCoroutine(KillContract()): the C# coroutine runs its first segment at once and the rest on the next
    // frame; the deferred part is queued here and RunDeferred() plays the frames
    static inline std::vector<std::function<void()>> deferred;
    // StringUtilities.LoadSiteGenerationInfo: the node of TextAssets/SiteGenerator/<lang>/SiteGeneratorInfo. Unset: every list is empty.
    static inline const ConfigNode* siteGeneratorInfo = nullptr;
    // WaypointManager.Instance() != null and LateralDistanceToVessel(wp[, vessel]) (meters); unset: the manager is absent
    static inline std::function<bool()> waypointManagerPresent;
    static inline std::function<float(FinePrint::Waypoint*, Vessel*)> lateralDistanceToVessel;
    // NavWaypoint.fetch (UI): IsActive / IsUsing(wp) / IsBlinking / Deactivate / Setup. Unset: no nav waypoint (fetch == null)
    struct NavWaypointSeam {
        std::function<bool()> isActive;
        std::function<bool(FinePrint::Waypoint*)> isUsing;
        std::function<bool()> isBlinking;
        std::function<void(bool)> setBlinking;
        std::function<void()> deactivate;
        std::function<void(FinePrint::Waypoint*)> setup;
    };
    static inline NavWaypointSeam* navWaypoint = nullptr;
    // ContractOrbitRenderer.Setup(contract, orbit) / Cleanup() (map rendering, WP-46). Unset: no renderer (null)
    static inline std::function<void(Contract&, Orbit&)> setupOrbitRenderer;
    static inline std::function<void()> cleanupOrbitRenderer;
    // The orbit of the renderer's driver (orbitRenderer.driver.orbit): read by SpecificOrbitParameter.OnUpdate; unset: no driver (null)
    static inline std::function<Orbit*()> orbitRendererDriverOrbit;
    // Vessel.Comet.typeName (CometVessel is not ported; Sentinel/comet package). Unset: ""
    static inline std::function<std::string(Vessel&)> cometTypeName;
    // R&D overrides for hosts / tests that have no loaded part database: ResearchAndDevelopment.ResearchedValidContractObjectives(types),
    // ProgressUtilities.HaveModuleTech(name), SatelliteContract.ProbeCoresUnlocked(). Unset: computed from PartLoader.LoadedPartsList like the C#.
    static inline std::function<bool(const std::vector<std::string>&)> researchedValidContractObjectives;
    static inline std::function<bool(const std::string&)> haveModuleTech;
    static inline std::function<bool()> probeCoresUnlocked;
    // TimeWarp / frame: Time.deltaTime (StabilityParameter.OnUpdate). Unset: Vessel::env.deltaTime
    static inline std::function<float()> deltaTime;

    static void Post(const std::string& text, float duration, ScreenMessageStyle style);
    static void RunDeferred();
    static void Reset();
};

// ---- ConfigNode helpers ---------------------------------------------------------------------------------------------------------------------

namespace StringUtilities {
// StringUtilities.cs:272 PackDelimitedString(items, '|') / :281 UnpackDelimitedString (values parsed with the TypeConverter of T)
std::string PackDelimitedString(const std::vector<std::string>& items, char delimiter = '|');
std::string PackDelimitedString(const std::vector<int32_t>& items, char delimiter = '|');
std::string PackDelimitedString(const std::vector<uint32_t>& items, char delimiter = '|');
std::vector<std::string> UnpackDelimitedStrings(const std::string& items, char delimiter = '|');
std::vector<int32_t> UnpackDelimitedInt32(const std::string& items, char delimiter = '|');
std::vector<uint32_t> UnpackDelimitedUInt32(const std::string& items, char delimiter = '|');
// :89 LoadSiteGenerationInfo (from FinePrintEnvironment::siteGeneratorInfo; also fills TemplateEnvironment::alphaNumeric)
void LoadSiteGenerationInfo();
// :121 GenerateSiteName(seed, body, landLocked, allowNamed)
std::string GenerateSiteName(int seed, CelestialBody* body, bool landLocked, bool allowNamed = true);
// :146 / :165
std::string NamedSitePrefix(KSPRandom* generator = nullptr);
std::string NamedSiteSuffix(CelestialBody* body, bool landLocked, KSPRandom* generator = nullptr);
// :210
std::string IntegerToGreek(int x);
// :255 CultureInfo.CurrentCulture.TextInfo.ToTitleCase(str) (invariant-culture English: each word's first letter upper case, the rest lower)
std::string TitleCase(const std::string& str);
// :320
std::string ValueAndUnits(RecordTrackType trackType, double trackValue);
// :346 / :359 / :414 / :444 / :460
GrammaticalGender KerbalGrammaticalGender(ProtoCrewMember& pcm);
std::string AppropriatePronoun(PronounCasing casing, GrammaticalGender gender, bool postFix = false);
std::string CardinalDirectionBetween(double originLatitude, double originLongitude, double destinationLatitude, double destinationLongitude);
bool IsVowel(char c);
float CalculateReadDuration(const std::string& s);
// :559
std::string SpecificVesselName(Contract& contract);
}  // namespace StringUtilities

// LingoonaGrammarExtensions.cs:50 / :63 LocalizeName(string, ProtoCrewMember.Gender / GrammaticalGender) -> LocalizeNoun(gender, isProperNoun: true)
namespace LingoonaGrammar {
std::string LocalizeName(std::string input, ProtoCrewMember_Gender gender);
std::string LocalizeName(std::string input, GrammaticalGender gender);
}  // namespace LingoonaGrammar

namespace SystemUtilities {
// :181 / :191 (LoadNodeList<T> / SaveNodeList<T> for the list types the FinePrint parameters use)
void LoadNodeList(ConfigNode* node, const char* className, const char* valueName, std::vector<std::string>& list);
void LoadNodeList(ConfigNode* node, const char* className, const char* valueName, std::vector<int32_t>& list);
void LoadNodeList(ConfigNode* node, const char* className, const char* valueName, std::vector<uint32_t>& list);
void SaveNodeList(ConfigNode* node, const char* listName, const std::vector<std::string>& list);
void SaveNodeList(ConfigNode* node, const char* listName, const std::vector<int32_t>& list);
void SaveNodeList(ConfigNode* node, const char* listName, const std::vector<uint32_t>& list);
// :578 RandomSplitChoice<string>(listOfLists, generator) (null lists are removed from the argument)
std::string RandomSplitChoice(std::vector<std::vector<std::string>*>& listOfLists, KSPRandom* generator = nullptr);
// :292 ProcessSideRequests(contract, contractNode, targetBody, vesselName, ref funds, ref science, ref reputation, vessel)
void ProcessSideRequests(Contract& contract, ConfigNode& contractNode, CelestialBody* targetBody, const std::string& vesselName, float& fundsMultiplier,
                         float& scienceMultiplier, float& reputationMultiplier, Vessel* vessel = nullptr);
// :621 CheckTouristRecoveryContractKerbals (FinePrint::Install() also installs it as TemplateEnvironment::checkTouristRecoveryContractKerbals)
bool CheckTouristRecoveryContractKerbals(const std::string& kerbalName);
}  // namespace SystemUtilities

namespace ProgressUtilities {
// :11 / :29 / :67 (ResearchAndDevelopment.PartTechAvailable over PartLoader)
bool HavePartTech(std::string partName, bool logging = true);
bool HaveModuleTech(const std::string& moduleName, const std::string* excludeModule = nullptr);
bool HaveAnyTech(const std::vector<std::string>* partNames, const std::vector<std::string>* moduleNames, bool logging = true);
// :559 ExperimentPossibleAt(experimentID, body, latitude, longitude, altitude, terrainHeight)
bool ExperimentPossibleAt(const std::string* experimentID, CelestialBody* body, double latitude, double longitude, double altitude, double terrainHeight);
// :273 CurrentTrackRecord(type)
double CurrentTrackRecord(RecordTrackType type);
}  // namespace ProgressUtilities

// PartModule.cs:964 IsValidContractObjective / Part.cs:8716 HasValidContractObjective / Vessel.cs:4313 HasValidContractObjectives: the
// IContractObjectiveModule implementations (ModuleDataTransmitter "Antenna", ModuleDeployableSolarPanel and ModuleGenerator "Generator",
// ModuleDockingNode "Dock", ModuleGrappleNode "Grapple", ModuleWheelBase "Wheel")
bool IsValidContractObjective(PartModule& module, const std::string& objectiveType);
bool HasValidContractObjective(Part& part, const std::string& objectiveType);
bool HasValidContractObjectives(Vessel& vessel, std::vector<std::string> objectiveTypes, bool copy = true);
// ResearchAndDevelopment.cs:1177 ResearchedValidContractObjectives (the version of ResearchAndDevelopment returns false: IsValidContractObjective was missing)
bool ResearchedValidContractObjectives(std::vector<std::string> objectiveTypes, bool copy = true);

// ---- VesselUtilities (VesselUtilities.cs) ---------------------------------------------------------------------------------------------------
// ActiveVesselFallback, GetPartName and VesselHasPartName are declared in fineprint_lite.hpp (WP-37).
int HighestPodCapacity();                                                                             // (C++ addition)
bool VesselHasModuleName(const std::string& moduleName, Vessel* v = nullptr);                         // (C++ addition)
double VesselResourceAmount(const std::string& resourceName, Vessel* v = nullptr);                    // (C++ addition)
bool VesselHasAnyParts(const std::vector<std::string>* partList, Vessel* v = nullptr);                // (C++ addition)
bool VesselHasAnyModules(const std::vector<std::string>* moduleList, Vessel* v = nullptr);            // (C++ addition)
bool VesselHasAnyPartsOrModules(const std::vector<std::string>* partList, const std::vector<std::string>* moduleList, Vessel* v = nullptr);  // (C++ addition)
int VesselPartAndModuleCount(const std::vector<std::string>& partList, const std::vector<std::string>& moduleList, Vessel* v = nullptr);    // (C++ addition)
std::vector<uint32_t> GetPartIDList(Vessel* v = nullptr);                                             // (C++ addition)
Vessel* FindVesselWithPartIDs(const std::vector<uint32_t>* partIDs);                                  // (C++ addition)
VesselType ClassifyVesselType(Vessel* v = nullptr, bool useVesselType = true);                        // (C++ addition)
std::vector<std::pair<Vessel*, VesselType>> ClassifyAllVesselsAt(CelestialBody* body, bool useVesselType = true);  // (C++ addition) (Dictionary: insertion order)
std::vector<Vessel*> SpecificVesselClassAt(CelestialBody* body, VesselType vesselType, bool requireOwned = false, bool excludeActive = false,
                                           bool useVesselType = true);                                // (C++ addition)
std::vector<Vessel*> SpecificVesselClassAt(Vessel::Situations situation, CelestialBody* body, VesselType vesselType, bool requireOwned = false,
                                           bool excludeActive = false, bool useVesselType = true);  // (C++ addition)
bool VesselHasWheelsOnGround(Vessel* v, const std::vector<WheelType>& validWheelTypes);               // (C++ addition)
FlightBand GetFlightBand(double threshold, Vessel* v = nullptr);                                      // (C++ addition)
int ActualCrewCapacity(Vessel* v = nullptr);                                                          // (C++ addition)
bool VesselLaunchedAfterID(uint32_t launchID, Vessel* v, std::vector<std::string> ignore);            // (C++ addition)
bool VesselIsOwned(Vessel* v = nullptr);                                                              // (C++ addition)
void DiscoverVessel(Vessel* v = nullptr);                                                             // (C++ addition)
uint32_t VesselID(Vessel* v = nullptr);                                                               // (C++ addition)
std::vector<std::shared_ptr<ProtoCrewMember>> VesselCrewWithTrait(const std::string& trait, Vessel* v = nullptr);  // (C++ addition)
int VesselCrewWithTraitCount(const std::string& trait, Vessel* v = nullptr);                          // (C++ addition)
int CrewTraitMissionAvailability(const std::string& trait, CelestialBody* targetBody = nullptr, Vessel* excludeVessel = nullptr);  // (C++ addition)
Orbit GenerateAdjustedVesselOrbit(double minimumDeviation, double modificationLength, int numberOfModifications, KSPRandom* generator = nullptr,
                                  Vessel* v = nullptr);                                               // (C++ addition)
bool VesselAtOrbit(Orbit* o, double deviationWindow, Vessel* v = nullptr);                            // (C++ addition)
double VesselAtOrbitAccuracy(Orbit* o, double deviationWindow, Vessel* v = nullptr);                  // (C++ addition)
Part* FindFirstPartOrModuleName(const std::vector<std::string>* partNames, const std::vector<std::string>* moduleNames);  // (C++ addition)


// ---- SurveyDefinition / SurveyDefinitionParameter (FinePrint/Utilities/SurveyDefinition*.cs) ------------------------------------------------
class SurveyDefinition;
// SurveyDefinitionParameter.cs:6
class SurveyDefinitionParameter {
public:
    std::string Experiment = "experiment";           // :7
    std::string Description = "Perform a survey";    // :9
    std::string Texture = "default";                 // :11
    bool AllowGround = true;                         // :13
    bool AllowLow = true;                            // :15
    bool AllowHigh = true;                           // :17
    bool AllowWater = true;                          // :19
    bool AllowVacuum = true;                         // :21
    bool CrewRequired = false;                       // :23
    bool EVARequired = false;                        // :25
    float FundsMultiplier = 1.0f;                    // :27
    float ScienceMultiplier = 1.0f;                  // :29
    float ReputationMultiplier = 1.0f;               // :31
    std::vector<std::string> Tech;                   // :33
    const ConfigNode* Node = nullptr;                // :35
    SurveyDefinition* Definition = nullptr;          // :37
    SurveyDefinitionParameter();                                          // :39
    SurveyDefinitionParameter(const SurveyDefinitionParameter& that);     // :46
    SurveyDefinitionParameter(SurveyDefinition* definition, const ConfigNode* node);  // :75
};

// SurveyDefinition.cs:6
class SurveyDefinition {
public:
    std::string DataName = "generic";                // :7
    std::string AnomalyName = "anomalies";           // :9
    std::string ResultName = "report";               // :11
    float FundsAdvance = 0.0f;                       // :13 (ContractDefs.Survey.Funds.DefaultAdvance)
    float FundsReward = 0.0f;                        // :15
    float FundsFailure = 0.0f;                       // :17
    float ScienceReward = 0.0f;                      // :19
    float ReputationReward = 0.0f;                   // :21
    float ReputationFailure = 0.0f;                  // :23
    std::vector<std::unique_ptr<SurveyDefinitionParameter>> Parameters;  // :25
    const ConfigNode* Node = nullptr;                // :27
    SurveyDefinition();                              // :29
    SurveyDefinition(const SurveyDefinition& that);  // :35
    SurveyDefinition(const ConfigNode* node);        // :55
    SurveyDefinition& operator=(const SurveyDefinition&) = delete;
};
// ContractDefs.cs:1359 SurveyDefinitions (a fresh copy of the loaded definitions, in node order)
std::vector<std::unique_ptr<SurveyDefinition>> SurveyDefinitions();
void ResetSurveyDefinitions();

}  // namespace ksp::Contracts::Lite

// Enum.ToString / Enum.Parse metadata of the FinePrint enums (the names are the C# member names)
template <>
struct ksp::dotnet::EnumTraits<ksp::Contracts::Lite::MannedStatus> {
    using E = ksp::Contracts::Lite::MannedStatus;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::MANNED, "MANNED"}, {E::UNMANNED, "UNMANNED"}, {E::ANY, "ANY"}};
};
template <>
struct ksp::dotnet::EnumTraits<ksp::Contracts::Lite::FlightBand> {
    using E = ksp::Contracts::Lite::FlightBand;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::NONE, "NONE"},       {E::GROUND, "GROUND"},   {E::LOW, "LOW"},         {E::HIGH, "HIGH"},
                                                {E::LOWMIX, "LOWMIX"},   {E::HIGHMIX, "HIGHMIX"}, {E::ANYMIX, "ANYMIX"}};
};
template <>
struct ksp::dotnet::EnumTraits<ksp::Contracts::Lite::WaypointClusterState> {
    using E = ksp::Contracts::Lite::WaypointClusterState;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::NONE, "NONE"}, {E::SINGLE, "SINGLE"}, {E::MIXED, "MIXED"}, {E::FULL, "FULL"}};
};
template <>
struct ksp::dotnet::EnumTraits<ksp::FinePrint::OrbitType> {
    using E = ksp::FinePrint::OrbitType;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::SYNCHRONOUS, "SYNCHRONOUS"}, {E::STATIONARY, "STATIONARY"}, {E::POLAR, "POLAR"},
                                                {E::EQUATORIAL, "EQUATORIAL"},   {E::KOLNIYA, "KOLNIYA"},     {E::TUNDRA, "TUNDRA"},
                                                {E::RANDOM, "RANDOM"}};
};
template <>
struct ksp::dotnet::EnumTraits<ksp::Contracts::Lite::PronounCasing> {
    using E = ksp::Contracts::Lite::PronounCasing;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::SUBJECTIVE, "SUBJECTIVE"}, {E::OBJECTIVE, "OBJECTIVE"}, {E::POSSESSIVE, "POSSESSIVE"}};
};
template <>
struct ksp::dotnet::EnumTraits<ksp::Contracts::Lite::GrammaticalGender> {
    using E = ksp::Contracts::Lite::GrammaticalGender;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::NEUTRAL, "NEUTRAL"}, {E::MASCULINE, "MASCULINE"}, {E::FEMININE, "FEMININE"}};
};
template <>
struct ksp::dotnet::EnumTraits<ksp::Contracts::Lite::RecordTrackType> {
    using E = ksp::Contracts::Lite::RecordTrackType;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::NULL_, "NULL"}, {E::ALTITUDE, "ALTITUDE"}, {E::DEPTH, "DEPTH"}, {E::DISTANCE, "DISTANCE"}, {E::SPEED, "SPEED"}};
};
