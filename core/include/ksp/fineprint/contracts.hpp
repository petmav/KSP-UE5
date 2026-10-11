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
    bool CanBeCancelled() override { return true; }                       // :122
    bool CanBeDeclined() override { return true; }                        // :127
    std::string GetHashString() override;                                 // :132
    std::string GetTitle() override;                                      // :137
    std::string GetDescription() override;                                // :150
    std::string GetSynopsys() override;                                   // :155
    std::string MessageCompleted() override;                              // :185
    void OnSave(ConfigNode& node) override;                               // :203
    void OnLoad(ConfigNode& node) override;                               // :210
    bool MeetRequirements() override;                                     // :217
    std::vector<CelestialBody*> WeightBodies() override { return {targetBody}; }  // (C++ addition) GetWeightBodies
    bool HasWeightBodies() override { return true; }
};

// FinePrint/Contracts/BaseContract.cs:11
class BaseContract : public Contract, public IUpdateWaypoints, public ICheckSpecificVessels {
public:
    CelestialBody* targetBody = nullptr;                                  // :14
    int capacity = 0;                                                     // :16
    bool contextual = false;                                              // :18
    static inline std::vector<std::string> objectiveTypes;                // :20
    std::string TypeName() const override { return "BaseContract"; }
    bool Generate() override;                                             // :22
    bool CanBeCancelled() override { return true; }                       // :164
    bool CanBeDeclined() override { return true; }                        // :169
    std::string GetHashString() override;                                 // :174
    std::string GetTitle() override;                                      // :179
    std::string GetDescription() override;                                // :188
    std::string GetSynopsys() override;                                   // :195
    std::string MessageCompleted() override;                              // :204
    void OnSave(ConfigNode& node) override;                               // :213
    void OnLoad(ConfigNode& node) override;                               // :220
    bool MeetRequirements() override;                                     // :227
    std::vector<CelestialBody*> WeightBodies() override { return {targetBody}; }
    bool HasWeightBodies() override { return true; }
    void OnAccepted() override;                                           // :241
    void OnFinished() override;                                           // :250
    static bool AreFacilitiesUnlocked();                                  // :255
};

// FinePrint/Contracts/StationContract.cs:11
class StationContract : public Contract, public IUpdateWaypoints, public ICheckSpecificVessels {
public:
    CelestialBody* targetBody = nullptr;                                  // :14
    int capacity = 0;                                                     // :16
    bool contextual = false;                                              // :18
    static inline std::vector<std::string> objectiveTypes;                // :20
    std::string TypeName() const override { return "StationContract"; }
    bool Generate() override;                                             // :22
    bool CanBeCancelled() override { return true; }                       // :178
    bool CanBeDeclined() override { return true; }                        // :183
    std::string GetHashString() override;                                 // :188
    std::string GetTitle() override;                                      // :193
    std::string GetDescription() override;                                // :200
    std::string GetSynopsys() override;                                   // :207
    std::string MessageCompleted() override;                              // :220
    void OnSave(ConfigNode& node) override;                               // :233
    void OnLoad(ConfigNode& node) override;                               // :240
    bool MeetRequirements() override;                                     // :247
    std::vector<CelestialBody*> WeightBodies() override { return {targetBody}; }
    bool HasWeightBodies() override { return true; }
    void OnAccepted() override;                                           // :261
    void OnFinished() override;                                           // :270
    static bool AreFacilitiesUnlocked();                                  // :275
    static bool AreAsteroidsUnlocked();                                   // :284
};

// FinePrint/Contracts/ISRUContract.cs:11
class ISRUContract : public Contract {
public:
    CelestialBody* targetBody = nullptr;                                  // :14
    CelestialBody* deliveryBody = nullptr;                                // :16
    Vessel::Situations deliverySituation = Vessel::Situations::ORBITING;  // :18
    bool isDelivering = false;                                            // :20
    float gatherGoal = 1000.0f;                                           // :22
    std::string targetResource = "Matter";                                // :24
    std::string resourceTitle = "matter";                                 // :26
    std::string TypeName() const override { return "ISRUContract"; }
    bool Generate() override;                                             // :28
    bool CanBeCancelled() override { return true; }                       // :159
    bool CanBeDeclined() override { return true; }                        // :164
    std::string GetHashString() override;                                 // :169
    std::string GetTitle() override;                                      // :174
    std::string GetDescription() override;                                // :183
    std::string GetSynopsys() override;                                   // :188
    std::string MessageCompleted() override;                              // :197
    void OnSave(ConfigNode& node) override;                               // :206
    void OnLoad(ConfigNode& node) override;                               // :220
    bool MeetRequirements() override;                                     // :239
    std::vector<CelestialBody*> WeightBodies() override;                  // (C++ addition) GetWeightBodies
    bool HasWeightBodies() override { return true; }
    std::vector<ConfigNode*> PossibleResources();                         // :258
    static bool CelestialIsForbidden(CelestialBody* body, ConfigNode* resourceRequest);  // :279
};

// FinePrint/Contracts/CometSampleContract.cs:13
class CometSampleContract : public Contract {
public:
    uint32_t cometPersistentId = 0;                                       // :13
    std::string cometName;                                                // :15
    std::string scienceSubjectId;                                         // :17
    ksp::Contracts::Parameters::CometScienceParameter* scienceParameter = nullptr;  // :19
    uint32_t CometPersistentId() const { return cometPersistentId; }      // :21
    const std::string& CometName() const { return cometName; }            // :23
    const std::string& ScienceSubjectId() const { return scienceSubjectId; }  // :25
    std::string TypeName() const override { return "CometSampleContract"; }
    bool Generate() override;                                             // :27
    void OnRegister() override;                                           // :93
    void OnUnregister() override;                                         // :101
    void OnVesselRename(events::GameEvents::HostedFromToAction<Vessel*, std::string> hostedFromTo);  // :108
    void OnDiscoverableObjectExpired(Vessel* v);                          // :124
    void VesselIdChanged(uint32_t oldId, uint32_t newId);                 // :140
    bool CanBeCancelled() override { return true; }                       // :148
    bool CanBeDeclined() override { return true; }                        // :153
    std::string GetHashString() override;                                 // :158
    std::string GetTitle() override;                                      // :163
    std::string GetDescription() override;                                // :168
    std::string GetSynopsys() override;                                   // :173
    std::string MessageCompleted() override;                              // :199
    void OnLoad(ConfigNode& node) override;                               // :204
    void OnSave(ConfigNode& node) override;                               // :211
    bool MeetRequirements() override;                                     // :218
    bool ValidCometAvailable();                                           // :227
};

// FinePrint/Contracts/SatelliteContract.cs:13
class SatelliteContract : public Contract, public IUpdateWaypoints, public ICheckSpecificVessels {
public:
    enum class SatellitePlacementMode : int { NEW, ADJUST, NETWORK, NETWORKADJUST };  // :14
    CelestialBody* targetBody = nullptr;                                  // :22
    double deviation = 10.0;                                              // :24
    OrbitType orbitType = OrbitType::RANDOM;                              // :26
    std::unique_ptr<KSPRandom> generator;                                 // :28
    double altitudeFactor = 0.5;                                          // :30
    double inclinationFactor = 0.5;                                       // :32
    SatellitePlacementMode placementMode = SatellitePlacementMode::NEW;   // :34
    static inline std::vector<std::string> objectiveTypes;                // :36
    std::string TypeName() const override { return "SatelliteContract"; }
    bool Generate() override;                                             // :38
    bool CanBeCancelled() override { return true; }                       // :297
    bool CanBeDeclined() override { return true; }                        // :302
    std::string GetHashString() override;                                 // :307
    std::string GetTitle() override;                                      // :312
    std::string GetDescription() override;                                // :391
    std::string GetSynopsys() override;                                   // :423
    std::string MessageCompleted() override;                              // :502
    void OnSave(ConfigNode& node) override;                               // :581
    void OnLoad(ConfigNode& node) override;                               // :589
    bool MeetRequirements() override;                                     // :597
    std::vector<CelestialBody*> WeightBodies() override { return {targetBody}; }
    bool HasWeightBodies() override { return true; }
    void OnAccepted() override;                                           // :615
    void OnFinished() override;                                           // :624
    static bool AreSatellitesUnlocked();                                  // :632
    static bool ProbeCoresUnlocked();                                     // :641
    void pickEasy();                                                      // :658
    void pickMedium();                                                    // :678
    void pickHard();                                                      // :698
    void setOrbitType(OrbitType targetType, double altitudeFactor_);      // :726
    bool PrestigeAppropriateOrbitType(OrbitType ot);                      // :825
    int NetworkSatelliteCount(CelestialBody* body);                       // :840
    bool CheckSatelliteNetwork(int networkCount, std::vector<Vessel*>& possibleSatellites, OrbitType orbitType_, std::unique_ptr<Orbit>& orbit,
                               double& stationaryLongitude, Vessel*& adjustVessel, Vessel* startVessel = nullptr);  // :848
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
    static std::shared_ptr<ProtoCrewMember> GetKerbal(const std::string& name);  // :70
    bool Generate() override;                                             // :79
    bool CanBeCancelled() override { return true; }                       // :226
    bool CanBeDeclined() override { return true; }                        // :231
    std::string GetHashString() override;                                 // :236
    std::string GetTitle() override;                                      // :241
    std::string GetDescription() override;                                // :250
    std::string GetSynopsys() override;                                   // :256
    std::string GetNotes() override;                                      // :266
    std::string MessageCompleted() override;                              // :285
    std::string MessageFailed() override;                                 // :295
    std::string MessageCancelled() override;                              // :300
    std::string MessageDeadlineExpired() override;                        // :306
    void OnSave(ConfigNode& node) override;                               // :312
    void OnLoad(ConfigNode& node) override;                               // :323
    bool MeetRequirements() override;                                     // :332
    std::vector<CelestialBody*> WeightBodies() override;                  // (C++ addition) GetWeightBodies
    bool HasWeightBodies() override { return true; }
    void OnAccepted() override;                                           // :364
    using Itinerary = std::pair<CelestialBody*, FlightLog_EntryType>;
    std::vector<Itinerary> BuildPossibleItineraries(KSPRandom* generator = nullptr);  // :379
    std::vector<CelestialBody*> PossibleBodies(KSPRandom* generator = nullptr);        // :413
    std::vector<FlightLog_EntryType> PossibleSituations(CelestialBody* body);          // :437
    std::vector<CelestialBody*> RandomReachedBodyGrouping(const std::vector<CelestialBody*>& reachedBodies, KSPRandom* generator = nullptr);  // :473
    Itinerary WeightedItineraryChoice(const std::vector<Itinerary>& itineraryChoices, KSPRandom* generator = nullptr);  // :506
    void ClearKerbalsSoft();                                              // :533
    void OnCancelled() override;                                          // :553
    void OnDeadlineExpired() override;                                    // :558
    void OnFailed() override;                                             // :563
    void OnFinished() override;                                           // :568
    void ClearKerbalsHard();                                              // :573
    void OnGenerateFailed() override;                                     // :586
    void OnWithdrawn() override;                                          // :591
    void OnDeclined() override;                                           // :596
    void OnOfferExpired() override;                                       // :601
    void GetContractSubject();                                            // :606
    std::string Pluralize(const std::string& single, const std::string& plural);  // :624
};

// FinePrint/Contracts/SurveyContract.cs:13
class SurveyContract : public Contract {
public:
    CelestialBody* targetBody = nullptr;                                  // :14
    double centerLatitude = 0.0;                                          // :16
    double centerLongitude = 0.0;                                         // :18
    std::string dataName = "generic";                                     // :20
    std::string anomalyName = "anomalies";                                // :22
    std::string resultName = "results";                                   // :24
    std::string locationName = "location";                                // :26
    bool focusedSurvey = false;                                           // :28
    FlightBand targetBand = FlightBand::NONE;                             // :30
    bool contextual = false;                                              // :32
    std::string vesselName = "vessel";                                    // :34
    std::string cardinalName = "cardinal";                                // :36
    std::string TypeName() const override { return "SurveyContract"; }
    bool Generate() override;                                             // :38
    bool CanBeCancelled() override { return true; }                       // :508
    bool CanBeDeclined() override { return true; }                        // :513
    std::string GetHashString() override;                                 // :518
    std::string GetTitle() override;                                      // :523
    std::string GetDescription() override;                                // :532
    std::string GetSynopsys() override;                                   // :537
    std::string MessageCompleted() override;                              // :550
    std::string GetNotes() override;                                      // :555
    void OnSave(ConfigNode& node) override;                               // :570
    void OnLoad(ConfigNode& node) override;                               // :589
    bool MeetRequirements() override;                                     // :608
    std::vector<CelestialBody*> WeightBodies() override { return {targetBody}; }
    bool HasWeightBodies() override { return true; }
    void SetupParam(int paramIndex, const std::pair<Lite::SurveyDefinitionParameter*, FlightBand>& pair, int totalParameters, Vessel* contextualVessel = nullptr);  // :628
    static std::vector<std::pair<Vessel*, std::vector<std::string>>> CachePossibleVessels(CelestialBody* body, std::vector<std::unique_ptr<Lite::SurveyDefinition>>& originalDefinitions);  // :676
    std::vector<std::unique_ptr<Lite::SurveyDefinition>> PossibleDefinitions();            // :792
    static double ParamRange(Contract::ContractPrestige prestige, CelestialBody* body, FlightBand band);  // :873
    void RandomizeNearKSC(Contract::ContractPrestige prestigeLevel, bool allowWater, KSPRandom* generator = nullptr);  // :905
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
