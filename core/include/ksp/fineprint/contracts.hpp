// WP-38 slice 2: the FinePrint contract types besides the ExplorationContract (Assembly-CSharp/FinePrint/Contracts/*.cs): ARMContract,
// BaseContract, StationContract, ISRUContract, SatelliteContract, CometSampleContract, TourismContract, SurveyContract.
//
// C# `protected` / `private` members are public (the tests drive them). The contract definitions (ContractDefs.*) are read through
// ContractDefsLite; the seams the contracts reach are TemplateEnvironment::researchedValidContractObjectives (R&D), the progress tree
// (ProgressTracking), GameVariables, FinePrintEnvironment (fineprint/utilities.hpp) and the CometManager (below).
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "ksp/fineprint/fineprint.hpp"
#include "ksp/fineprint/parameters.hpp"
#include "ksp/fineprint/utilities.hpp"

namespace ksp::FinePrint {
// FinePrint/Utilities/IUpdateWaypoints.cs:3 / ICheckSpecificVessels.cs:3 (marker interfaces)
class IUpdateWaypoints {
public:
    virtual ~IUpdateWaypoints() = default;
};
class ICheckSpecificVessels {
public:
    virtual ~ICheckSpecificVessels() = default;
};
}  // namespace ksp::FinePrint

namespace ksp::FinePrint::Contracts {

using ksp::Contracts::Contract;
using ksp::Contracts::ContractParameter;
using ksp::Contracts::Parameters::SpecificVesselParameter;
using ksp::Contracts::Lite::FlightBand;
namespace Lite = ::ksp::Contracts::Lite;

// CometManager.Instance.DiscoveredComets (the comet package is not ported): the vessels of the discovered comets. Unset: none.
struct CometManagerSeam {
    static inline std::function<std::vector<Vessel*>()> discoveredComets;
};

// FinePrint/Contracts/ARMContract.cs:11
class ARMContract : public Contract {
public:
    CelestialBody* targetBody = nullptr;                                  // :13
    std::string asteroidClass = "A";                                      // :15
    bool isLanding = false;                                               // :17
    std::string TypeName() const override { return "ARMContract"; }
    bool Generate() override;                                             // :19
    bool CanBeCancelled() override { return true; }                       // :171
    bool CanBeDeclined() override { return true; }                        // :176
    std::string GetHashString() override;                                 // :181
    std::string GetTitle() override;                                      // :186
    std::string GetDescription() override;                                // :197
    std::string GetSynopsys() override;                                   // :202
    std::string MessageCompleted() override;                              // :238
    void OnSave(ConfigNode& node) override;                               // :257
    void OnLoad(ConfigNode& node) override;                               // :264
    bool MeetRequirements() override;                                     // :271
    std::vector<CelestialBody*> WeightBodies() override { return {targetBody}; }  // :285 GetWeightBodies
    bool HasWeightBodies() override { return true; }
};

// FinePrint/Contracts/BaseContract.cs:11
class BaseContract : public Contract, public IUpdateWaypoints, public ICheckSpecificVessels {
public:
    CelestialBody* targetBody = nullptr;                                  // :13
    int capacity = 0;                                                     // :15
    bool contextual = false;                                              // :17
    static inline std::vector<std::string> objectiveTypes;                // :19
    std::string TypeName() const override { return "BaseContract"; }
    bool Generate() override;                                             // :21
    bool CanBeCancelled() override { return true; }                       // :178
    bool CanBeDeclined() override { return true; }                        // :183
    std::string GetHashString() override;                                 // :188
    std::string GetTitle() override;                                      // :193
    std::string GetDescription() override;                                // :202
    std::string GetSynopsys() override;                                   // :209
    std::string MessageCompleted() override;                              // :219
    void OnSave(ConfigNode& node) override;                               // :229
    void OnLoad(ConfigNode& node) override;                               // :236
    bool MeetRequirements() override;                                     // :243
    std::vector<CelestialBody*> WeightBodies() override { return {targetBody}; }
    bool HasWeightBodies() override { return true; }
    void OnAccepted() override;                                           // :256
    void OnFinished() override;                                           // :266
    static bool AreFacilitiesUnlocked();                                  // :271
};

// FinePrint/Contracts/StationContract.cs:11
class StationContract : public Contract, public IUpdateWaypoints, public ICheckSpecificVessels {
public:
    CelestialBody* targetBody = nullptr;                                  // :13
    int capacity = 0;                                                     // :15
    bool contextual = false;                                              // :17
    static inline std::vector<std::string> objectiveTypes;                // :19
    std::string TypeName() const override { return "StationContract"; }
    bool Generate() override;                                             // :21
    bool CanBeCancelled() override { return true; }                       // :193
    bool CanBeDeclined() override { return true; }                        // :198
    std::string GetHashString() override;                                 // :203
    std::string GetTitle() override;                                      // :208
    std::string GetDescription() override;                                // :215
    std::string GetSynopsys() override;                                   // :221
    std::string MessageCompleted() override;                              // :233
    void OnSave(ConfigNode& node) override;                               // :246
    void OnLoad(ConfigNode& node) override;                               // :253
    bool MeetRequirements() override;                                     // :260
    std::vector<CelestialBody*> WeightBodies() override { return {targetBody}; }
    bool HasWeightBodies() override { return true; }
    void OnAccepted() override;                                           // :274
    void OnFinished() override;                                           // :282
    static bool AreFacilitiesUnlocked();                                  // :287
    static bool AreAsteroidsUnlocked();                                   // :296
};

// FinePrint/Contracts/ISRUContract.cs:11
class ISRUContract : public Contract {
public:
    CelestialBody* targetBody = nullptr;                                  // :13
    CelestialBody* deliveryBody = nullptr;                                // :15
    Vessel::Situations deliverySituation = Vessel::Situations::ORBITING;  // :17
    bool isDelivering = false;                                            // :19
    float gatherGoal = 1000.0f;                                           // :21
    std::string targetResource = "Matter";                                // :23
    std::string resourceTitle = "matter";                                 // :25
    std::string TypeName() const override { return "ISRUContract"; }
    bool Generate() override;                                             // :27
    bool CanBeCancelled() override { return true; }                       // :195
    bool CanBeDeclined() override { return true; }                        // :200
    std::string GetHashString() override;                                 // :205
    std::string GetTitle() override;                                      // :210
    std::string GetDescription() override;                                // :219
    std::string GetSynopsys() override;                                   // :224
    std::string MessageCompleted() override;                              // :234
    void OnSave(ConfigNode& node) override;                               // :244
    void OnLoad(ConfigNode& node) override;                               // :258
    bool MeetRequirements() override;                                     // :277
    std::vector<CelestialBody*> WeightBodies() override;                  // :286 GetWeightBodies
    bool HasWeightBodies() override { return true; }
    std::vector<ConfigNode*> PossibleResources();                         // :296
    static bool CelestialIsForbidden(CelestialBody* body, ConfigNode* resourceRequest);  // :316
};

// FinePrint/Contracts/CometSampleContract.cs:13
class CometSampleContract : public Contract {
public:
    uint32_t cometPersistentId = 0;                                       // :15
    std::string cometName;                                                // :17
    std::string scienceSubjectId;                                         // :19
    ksp::Contracts::Parameters::CometScienceParameter* scienceParameter = nullptr;  // :21
    uint32_t CometPersistentId() const { return cometPersistentId; }      // :23
    const std::string& CometName() const { return cometName; }            // :25
    const std::string& ScienceSubjectId() const { return scienceSubjectId; }  // :27
    std::string TypeName() const override { return "CometSampleContract"; }
    bool Generate() override;                                             // :29
    void OnRegister() override;                                           // :105
    void OnUnregister() override;                                         // :113
    void OnVesselRename(events::GameEvents::HostedFromToAction<Vessel*, std::string> hostedFromTo);  // :121
    void OnDiscoverableObjectExpired(Vessel* v);                          // :134
    void VesselIdChanged(uint32_t oldId, uint32_t newId);                 // :149
    bool CanBeCancelled() override { return true; }                       // :157
    bool CanBeDeclined() override { return true; }                        // :162
    std::string GetHashString() override;                                 // :167
    std::string GetTitle() override;                                      // :172
    std::string GetDescription() override;                                // :177
    std::string GetSynopsys() override;                                   // :182
    std::string MessageCompleted() override;                              // :219
    void OnLoad(ConfigNode& node) override;                               // :224
    void OnSave(ConfigNode& node) override;                               // :231
    bool MeetRequirements() override;                                     // :238
    bool ValidCometAvailable();                                           // :247
};

// FinePrint/Contracts/SatelliteContract.cs:13
class SatelliteContract : public Contract, public IUpdateWaypoints, public ICheckSpecificVessels {
public:
    enum class SatellitePlacementMode : int { NEW, ADJUST, NETWORK, NETWORKADJUST };  // :15
    CelestialBody* targetBody = nullptr;                                  // :23
    double deviation = 10.0;                                              // :25
    OrbitType orbitType = OrbitType::RANDOM;                              // :27
    std::unique_ptr<KSPRandom> generator;                                 // :29
    double altitudeFactor = 0.5;                                          // :31
    double inclinationFactor = 0.5;                                       // :33
    SatellitePlacementMode placementMode = SatellitePlacementMode::NEW;   // :35
    static inline std::vector<std::string> objectiveTypes;                // :37
    std::string TypeName() const override { return "SatelliteContract"; }
    bool Generate() override;                                             // :39
    bool CanBeCancelled() override { return true; }                       // :278
    bool CanBeDeclined() override { return true; }                        // :283
    std::string GetHashString() override;                                 // :288
    std::string GetTitle() override;                                      // :293
    std::string GetDescription() override;                                // :371
    std::string GetSynopsys() override;                                   // :402
    std::string MessageCompleted() override;                              // :480
    void OnSave(ConfigNode& node) override;                               // :558
    void OnLoad(ConfigNode& node) override;                               // :565
    bool MeetRequirements() override;                                     // :572
    std::vector<CelestialBody*> WeightBodies() override { return {targetBody}; }
    bool HasWeightBodies() override { return true; }
    void OnAccepted() override;                                           // :588
    void OnFinished() override;                                           // :596
    static bool AreSatellitesUnlocked();                                  // :603
    static bool ProbeCoresUnlocked();                                     // :611
    void pickEasy();                                                      // :627
    void pickMedium();                                                    // :646
    void pickHard();                                                      // :665
    void setOrbitType(OrbitType targetType, double altitudeFactor_);      // :692
    bool PrestigeAppropriateOrbitType(OrbitType ot);                      // :790
    int NetworkSatelliteCount(CelestialBody* body);                       // :804
    bool CheckSatelliteNetwork(int networkCount, std::vector<Vessel*>& possibleSatellites, OrbitType orbitType_, std::unique_ptr<Orbit>& orbit,
                               double& stationaryLongitude, Vessel*& adjustVessel, Vessel* startVessel = nullptr);  // :811
};

// FinePrint/Contracts/TourismContract.cs:12
class TourismContract : public Contract {
public:
    std::string contractSubject;                                          // :14
    std::vector<std::string> tourists;                                    // :16
    std::string preposition;                                              // :18
    bool homeDestinations = false;                                        // :20
    bool isGeeAdventure = false;                                          // :22
    std::string TypeName() const override { return "TourismContract"; }
    const std::vector<std::string>& Tourists() const { return tourists; }  // :24
    int TouristCount() const;                                             // :26
    bool SingleDestination() { return ParameterCount() == 1; }            // :38
    std::string FirstTourist() const;                                     // :40
    std::string TouristNamesJoined() const;                               // :52
    static std::shared_ptr<ProtoCrewMember> GetKerbal(const std::string& name);  // :69
    bool Generate() override;                                             // :78
    bool CanBeCancelled() override { return true; }                       // :280
    bool CanBeDeclined() override { return true; }                        // :285
    std::string GetHashString() override;                                 // :290
    std::string GetTitle() override;                                      // :295
    std::string GetDescription() override;                                // :304
    std::string GetSynopsys() override;                                   // :310
    std::string GetNotes() override;                                      // :322
    std::string MessageCompleted() override;                              // :348
    std::string MessageFailed() override;                                 // :359
    std::string MessageCancelled() override;                              // :364
    std::string MessageDeadlineExpired() override;                        // :370
    void OnSave(ConfigNode& node) override;                               // :376
    void OnLoad(ConfigNode& node) override;                               // :388
    bool MeetRequirements() override;                                     // :398
    std::vector<CelestialBody*> WeightBodies() override;                  // :407 GetWeightBodies
    bool HasWeightBodies() override { return true; }
    void OnAccepted() override;                                           // :433
    using Itinerary = std::pair<CelestialBody*, FlightLog_EntryType>;
    std::vector<Itinerary> BuildPossibleItineraries(KSPRandom* generator = nullptr);  // :450
    std::vector<CelestialBody*> PossibleBodies(KSPRandom* generator = nullptr);        // :482
    std::vector<FlightLog_EntryType> PossibleSituations(CelestialBody* body);          // :507
    std::vector<CelestialBody*> RandomReachedBodyGrouping(const std::vector<CelestialBody*>& reachedBodies, KSPRandom* generator = nullptr);  // :539
    Itinerary WeightedItineraryChoice(const std::vector<Itinerary>& itineraryChoices, KSPRandom* generator = nullptr);  // :569
    void ClearKerbalsSoft();                                              // :596
    void OnCancelled() override;                                          // :618
    void OnDeadlineExpired() override;                                    // :623
    void OnFailed() override;                                             // :628
    void OnFinished() override;                                           // :633
    void ClearKerbalsHard();                                              // :638
    void OnGenerateFailed() override;                                     // :651
    void OnWithdrawn() override;                                          // :656
    void OnDeclined() override;                                           // :661
    void OnOfferExpired() override;                                       // :666
    void GetContractSubject();                                            // :671
    std::string Pluralize(const std::string& single, const std::string& plural);  // :688
};

// FinePrint/Contracts/SurveyContract.cs:13
class SurveyContract : public Contract {
public:
    CelestialBody* targetBody = nullptr;                                  // :15
    double centerLatitude = 0.0;                                          // :17
    double centerLongitude = 0.0;                                         // :19
    std::string dataName = "generic";                                     // :21
    std::string anomalyName = "anomalies";                                // :23
    std::string resultName = "results";                                   // :25
    std::string locationName = "location";                                // :27
    bool focusedSurvey = false;                                           // :29
    FlightBand targetBand = FlightBand::NONE;                             // :31
    bool contextual = false;                                              // :33
    std::string vesselName = "vessel";                                    // :35
    std::string cardinalName = "cardinal";                                // :37
    std::string TypeName() const override { return "SurveyContract"; }
    bool Generate() override;                                             // :39
    bool CanBeCancelled() override { return true; }                       // :482
    bool CanBeDeclined() override { return true; }                        // :487
    std::string GetHashString() override;                                 // :492
    std::string GetTitle() override;                                      // :497
    std::string GetDescription() override;                                // :506
    std::string GetSynopsys() override;                                   // :511
    std::string MessageCompleted() override;                              // :526
    std::string GetNotes() override;                                      // :531
    void OnSave(ConfigNode& node) override;                               // :546
    void OnLoad(ConfigNode& node) override;                               // :565
    bool MeetRequirements() override;                                     // :590
    std::vector<CelestialBody*> WeightBodies() override { return {targetBody}; }
    bool HasWeightBodies() override { return true; }
    void SetupParam(int paramIndex, const std::pair<Lite::SurveyDefinitionParameter*, FlightBand>& pair, int totalParameters, Vessel* contextualVessel = nullptr);  // :610
    static std::vector<std::pair<Vessel*, std::vector<std::string>>> CachePossibleVessels(CelestialBody* body, std::vector<std::unique_ptr<Lite::SurveyDefinition>>& originalDefinitions);  // :660
    std::vector<std::unique_ptr<Lite::SurveyDefinition>> PossibleDefinitions();            // :790
    static double ParamRange(Contract::ContractPrestige prestige, CelestialBody* body, FlightBand band);  // :877
    void RandomizeNearKSC(Contract::ContractPrestige prestigeLevel, bool allowWater, KSPRandom* generator = nullptr);  // :907
};

}  // namespace ksp::FinePrint::Contracts

template <>
struct ksp::dotnet::EnumTraits<ksp::FinePrint::Contracts::SatelliteContract::SatellitePlacementMode> {
    using E = ksp::FinePrint::Contracts::SatelliteContract::SatellitePlacementMode;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::NEW, "NEW"}, {E::ADJUST, "ADJUST"}, {E::NETWORK, "NETWORK"}, {E::NETWORKADJUST, "NETWORKADJUST"}};
};

namespace ksp::FinePrint {
// Install() plus every contract and parameter type of slice 2, ScenarioContractEvents, and the seams the FinePrint ports answer
// (CareerEnvironment::valueAndUnits, TemplateEnvironment::researchedValidContractObjectives / checkTouristRecoveryContractKerbals /
// discoverVessel / the GrandTour parameter factories). Idempotent.
void InstallAll();
}  // namespace ksp::FinePrint
