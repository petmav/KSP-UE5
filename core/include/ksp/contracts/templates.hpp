// WP-37: the stock contract templates (Assembly-CSharp/Contracts/Templates/*.cs): PlantFlag, CollectScience, PartTest, RecoverAsset,
// GrandTour, VesselRepairContract, OrbitalConstructionContract, RoverConstructionContract.
//
// Seams (see TemplateEnvironment below): the FinePrint parameter classes the last four build (WP-38 owns VesselSystemsParameter,
// VesselDestinationParameter, LocationAndSituationParameter, RepairPartParameter, ConstructionParameter, RoverWayPointParameter,
// CrewTraitParameter), the PartLoader / R&D tech tree view PartTest.Generate walks, the waypoint manager, the pre-built craft tables.
#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ksp/contracts/contract.hpp"
#include "ksp/contracts/parameters.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {
class ProtoCrewMember;
class AvailablePart;
class ModuleTestSubject;
class Part;
class ProtoVessel;
class Vessel;
}  // namespace ksp

namespace ksp::Contracts::Templates {

// Contracts/Templates/PlantFlag.cs:11
class PlantFlag : public Contract {
public:
    std::string TypeName() const override { return "PlantFlag"; }
    CelestialBody* TargetBody() const { return targetBody; }                  // :18
    bool MeetRequirements() override;                                         // :20
    std::vector<CelestialBody*> WeightBodies() override { return {targetBody}; }  // (C++ addition)
    bool HasWeightBodies() override { return true; }
    std::string GetDescription() override;                                    // :38
    std::string GetSynopsys() override;                                       // :43
    std::string GetTitle() override;                                          // :48
    std::string MessageCompleted() override;                                  // :53
    void OnLoad(ConfigNode& node) override;                                   // :58
    void OnSave(ConfigNode& node) override;                                   // :66
    bool Generate() override;                                                 // :71
    std::string GetHashString() override;                                     // :132

protected:
    CelestialBody* targetBody = nullptr;
};

// Contracts/Templates/CollectScience.cs:10
class CollectScience : public Contract {
public:
    std::string TypeName() const override { return "CollectScience"; }
    CelestialBody* TargetBody() const { return targetBody; }                  // :19
    BodyLocation TargetLocation() const { return targetLocation; }            // :21
    bool MeetRequirements() override;                                         // :23
    std::vector<CelestialBody*> WeightBodies() override { return {targetBody}; }  // (C++ addition)
    bool HasWeightBodies() override { return true; }
    std::string GetDescription() override;                                    // :37
    std::string GetSynopsys() override;                                       // :46
    std::string GetTitle() override;                                          // :55
    std::string MessageCompleted() override;                                  // :64
    void OnLoad(ConfigNode& node) override;                                   // :73
    void OnSave(ConfigNode& node) override;                                   // :85
    bool Generate() override;                                                 // :91

protected:
    CelestialBody* targetBody = nullptr;
    BodyLocation targetLocation = BodyLocation::Space;

private:
    bool GenerateContract_Orbit();                                            // :105
    bool GenerateContract_Surface();                                          // :141
};

// What the templates read from systems that are other work packages (seams; the default is documented per member)
struct TemplateEnvironment {
    // ResearchAndDevelopment.ResearchedValidContractObjectives(type) (RecoverAsset "Grapple"). Unset: false.
    static inline std::function<bool(const std::string& type)> researchedValidContractObjectives;
    // SystemUtilities.CheckTouristRecoveryContractKerbals(name) (WP-38). Unset: false.
    static inline std::function<bool(const std::string& kerbalName)> checkTouristRecoveryContractKerbals;
    // SystemUtilities.ExpungeKerbal(pcm) (WP-38). Unset: nothing.
    static inline std::function<void(ProtoCrewMember&)> expungeKerbal;
    // StringUtilities.alphaNumeric (TextAssets/SiteGenerator/<lang>/SiteGeneratorInfo "Alphanumeric": a Unity TextAsset that is not in
    // the repo: needs data). Unset: empty (AlphaNumericDesignation then fails like the C# list index would).
    static inline std::vector<std::string> alphaNumeric;
    // The vessel spawning of RecoverAsset.OnAccepted (GenerateLandRecovery / GenerateOrbitalRecovery: ProtoVessel.CreateVesselNode,
    // WaypointManager.ChooseRandomPosition, Orbit generation, Game.AddVessel). The callee sets partID through RecoverAsset::SetPartID.
    // Unset: nothing is spawned.
    static inline std::function<void(class RecoverAsset&, bool land)> spawnRecoveryVessels;
    // RecoverAsset.Cleanup: the vessels carrying partID are destroyed unless owned (FlightGlobals.Vessels walk, Vessel.Die). Unset: nothing.
    static inline std::function<void(uint32_t partID)> destroyUnownedVesselsWithPart;
    // VesselUtilities.DiscoverVessel(v) (WP-38). Unset: nothing.
    static inline std::function<void(Vessel*)> discoverVessel;
    // VesselUtilities.EstimatePartSize(prefab) (GetNotes). Unset: (0, 0, 0).
    static inline std::function<Vector3f(const AvailablePart*)> estimatePartSize;
    // PartHasSeats(partName): InternalModel.seats of the internal of the part (PartLoader.Instance.internalParts). Unset: CrewCapacity > 0.
    static inline std::function<bool(const std::string& partName)> partHasSeats;
    // The WP-38 FinePrint parameters GrandTour builds (VesselSystemsParameter(null, null, title), VesselDestinationParameter(body, entryType),
    // LocationAndSituationParameter(body, situation, title, finalObjective)). entryType = FlightLog.EntryType as int. Unset: GrandTour.Generate fails.
    static inline std::function<std::unique_ptr<ContractParameter>(const std::string& title)> makeVesselSystemsParameter;
    static inline std::function<std::unique_ptr<ContractParameter>(CelestialBody*, int entryType)> makeVesselDestinationParameter;
    static inline std::function<std::unique_ptr<ContractParameter>(CelestialBody*, Vessel::Situations, const std::string& title, bool finalObjective)>
        makeLocationAndSituationParameter;
    // GrandTour.OnAccepted: VesselSystemsParameter.requireNew -> launchID = HighLogic.CurrentGame.launchID (WP-38 parameter). Unset: nothing.
    static inline std::function<void(Contract&, uint32_t launchID)> grandTourAccepted;
    static void Reset();
};

// Contracts/Templates/RecoverAsset.cs:14
class RecoverAsset : public Contract {
public:
    enum class RecoveryLocation : int { NONE, ORBITLOW, ORBITHIGH, SURFACE };          // :15
    enum class RecoveryType : int { NONE, KERBAL, PART, COMPOUND };                    // :23
    RecoverAsset();                                                                    // :120
    std::string TypeName() const override { return "RecoverAsset"; }
    std::shared_ptr<ProtoCrewMember> RecoveryKerbal() const { return recoveryKerbal; } // :55
    CelestialBody* TargetBody() const { return targetBody; }
    RecoveryLocation Location() const { return recoveryLocation; }
    RecoveryType Type() const { return recoveryType; }
    const std::string& PartNameValue() const { return partName; }
    uint32_t PartID() const { return partID; }
    void SetPartID(uint32_t id) { partID = id; }
    bool Generate() override;                                                          // :140
    void OnAccepted() override;                                                        // :207
    void OnParameterStateChange(ContractParameter* p) override;                        // :229
    std::string GetHashString() override;                                              // :258
    std::string GetTitle() override;                                                   // :263
    std::string GetDescription() override;                                             // :298
    std::string GetSynopsys() override;                                                // :311
    std::string GetNotes() override;                                                   // :366
    std::string MessageCompleted() override;                                           // :385
    std::string MessageFailed() override;                                              // :398
    std::string MessageCancelled() override;                                           // :437
    std::string MessageDeadlineExpired() override;                                     // :450
    void OnLoad(ConfigNode& node) override;                                            // :463
    void OnSave(ConfigNode& node) override;                                            // :475
    bool MeetRequirements() override;                                                  // :486
    std::vector<CelestialBody*> WeightBodies() override { return {targetBody}; }       // (C++ addition)
    bool HasWeightBodies() override { return true; }
    void OnGenerateFailed() override { Cleanup(); }                                    // :579
    void OnDeclined() override { Cleanup(); }                                          // :584
    void OnOfferExpired() override { Cleanup(); }                                      // :589
    void OnFinished() override { Cleanup(); }                                          // :594
    void DiscoverAsset(Vessel* v);                                                     // :998

    std::string VesselDescription();                                                   // :93
    std::string VesselName();                                                          // :106

private:
    bool PartIsValid() const;                                                          // :57
    bool RecoveringKerbal() const { return recoveryType == RecoveryType::KERBAL || recoveryType == RecoveryType::COMPOUND; }  // :69
    bool RecoveringPart() const { return recoveryType == RecoveryType::PART || recoveryType == RecoveryType::COMPOUND; }      // :81
    std::shared_ptr<ProtoCrewMember> ProtoKerbal();                                    // :118
    void Cleanup();                                                                    // :512
    bool Initialize();                                                                 // :599
    void ChooseLocation(KSPRandom* generator = nullptr);                               // :636
    std::string ChooseVesselType(KSPRandom* generator = nullptr);                      // :679
    bool AcceptablePart(Part* p);                                                      // :716
    bool PartHasSeats(const std::string& part);                                        // :846
    float GetMultiplier();                                                             // :925
    void CheckComplete();                                                              // :956
    CelestialBody* targetBody = nullptr;
    RecoveryLocation recoveryLocation = RecoveryLocation::NONE;
    RecoveryType recoveryType = RecoveryType::NONE;
    std::shared_ptr<ProtoCrewMember> recoveryKerbal;
    std::string partName = "";
    uint32_t partID = 0;
    static inline std::string RecoveringKerbalDescriptionStrings[10] = {"Shipwreck", "Wreckage", "Pod", "Capsule", "Derelict", "Heap", "Hulk", "Craft", "Debris", "Scrap"};
    static inline std::string DescriptionStrings[6] = {"Prototype", "Device", "Part", "Module", "Unit", "Component"};
};

// Contracts/Templates/PartTest.cs:14
class PartTest : public Contract {
public:
    std::string TypeName() const override { return "PartTest"; }
    std::string PartName;                                                              // :16
    bool PartIsExperimental = false;                                                   // :19
    bool Hauled() const { return hauled; }                                             // :33
    CelestialBody* Destination() const { return destination; }
    Vessel::Situations TargetSituation() const { return tgtSituation; }
    const AvailablePart* TargetPart() const { return tgtPart; }
    bool MeetRequirements() override;                                                  // :35
    std::vector<CelestialBody*> WeightBodies() override { return {destination}; }      // (C++ addition)
    bool HasWeightBodies() override { return true; }
    std::string GetDescription() override;                                             // :49
    std::string GetSynopsys() override;                                                // :54
    std::string GetTitle() override;                                                   // :63
    std::string MessageCompleted() override;                                           // :72
    std::string GetSituationString(Vessel::Situations sit, CelestialBody* body, const std::string& biomeName);  // :81
    void OnLoad(ConfigNode& node) override;                                            // :191
    void OnSave(ConfigNode& node) override;                                            // :216
    bool Generate() override;                                                          // :225
    void OnRegister() override;                                                        // :813
    void OnUnregister() override;                                                      // :818
    void OnAccepted() override;                                                        // :823
    void OnPartResearched(AvailablePart* ap);                                          // :835
    void OnFinished() override;                                                        // :845

protected:
    const AvailablePart* tgtPart = nullptr;
    CelestialBody* destination = nullptr;
    Vessel::Situations tgtSituation = static_cast<Vessel::Situations>(0);
    bool hauled = false;
};

// Contracts/Templates/GrandTour.cs:12
class GrandTour : public Contract {
public:
    std::string TypeName() const override { return "GrandTour"; }
    const std::vector<CelestialBody*>& TargetBodies() const { return targetBodies; }   // :30
    bool MeetRequirements() override;                                                  // :58
    std::vector<CelestialBody*> WeightBodies() override { return targetBodies; }       // (C++ addition)
    bool HasWeightBodies() override { return true; }
    std::string GetDescription() override;                                             // :68
    std::string GetSynopsys() override;                                                // :74
    std::string GetTitle() override;                                                   // :110
    std::string MessageCompleted() override;                                           // :175
    void OnAccepted() override;                                                        // :180
    std::string GetNotes() override;                                                   // :189
    void OnLoad(ConfigNode& node) override;                                            // :208
    void OnSave(ConfigNode& node) override;                                            // :234
    bool Generate() override;                                                          // :255
    std::string GetHashString() override;                                              // :547
    FinePrint::Utilities::ProgressType ProgressTypeValue() const { return progressType; }

private:
    int TotalBodies() const;                                                           // :32
    std::string FormattedDestinations() const;                                         // :44
    void AttemptRandomTour(KSPRandom* generator = nullptr);                            // :334
    void AttemptFocusedTour(KSPRandom* generator = nullptr);                           // :386
    std::vector<CelestialBody*> GetFocusedChallenges(FinePrint::Utilities::ProgressType type);    // :443
    void AttemptFinalDestination(KSPRandom* generator = nullptr);                      // :487
    std::vector<CelestialBody*> targetBodies;
    FinePrint::Utilities::ProgressType progressType = FinePrint::Utilities::ProgressType::NULL_;
    bool finalDestination = false;
    bool focused = false;
    CelestialBody* focusBody = nullptr;
    CelestialBody* finalBody = nullptr;
    Vessel::Situations finalSituation = static_cast<Vessel::Situations>(0);
};

}  // namespace ksp::Contracts::Templates

template <>
struct ksp::dotnet::EnumTraits<ksp::Contracts::Templates::RecoverAsset::RecoveryLocation> {
    using E = ksp::Contracts::Templates::RecoverAsset::RecoveryLocation;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::NONE, "NONE"}, {E::ORBITLOW, "ORBITLOW"}, {E::ORBITHIGH, "ORBITHIGH"}, {E::SURFACE, "SURFACE"}};
};
template <>
struct ksp::dotnet::EnumTraits<ksp::Contracts::Templates::RecoverAsset::RecoveryType> {
    using E = ksp::Contracts::Templates::RecoverAsset::RecoveryType;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::NONE, "NONE"}, {E::KERBAL, "KERBAL"}, {E::PART, "PART"}, {E::COMPOUND, "COMPOUND"}};
};
