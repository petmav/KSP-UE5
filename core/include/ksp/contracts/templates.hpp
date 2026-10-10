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
    CelestialBody* TargetBody() const { return targetBody; }                  // :17
    bool MeetRequirements() override;                                         // :19
    std::vector<CelestialBody*> WeightBodies() override { return {targetBody}; }  // :31
    bool HasWeightBodies() override { return true; }
    std::string GetDescription() override;                                    // :36
    std::string GetSynopsys() override;                                       // :41
    std::string GetTitle() override;                                          // :46
    std::string MessageCompleted() override;                                  // :51
    void OnLoad(ConfigNode& node) override;                                   // :56
    void OnSave(ConfigNode& node) override;                                   // :64
    bool Generate() override;                                                 // :70
    std::string GetHashString() override;                                     // :129

protected:
    CelestialBody* targetBody = nullptr;
};

// Contracts/Templates/CollectScience.cs:10
class CollectScience : public Contract {
public:
    std::string TypeName() const override { return "CollectScience"; }
    CelestialBody* TargetBody() const { return targetBody; }                  // :17
    BodyLocation TargetLocation() const { return targetLocation; }            // :19
    bool MeetRequirements() override;                                         // :21
    std::vector<CelestialBody*> WeightBodies() override { return {targetBody}; }  // :29
    bool HasWeightBodies() override { return true; }
    std::string GetDescription() override;                                    // :34
    std::string GetSynopsys() override;                                       // :43
    std::string GetTitle() override;                                          // :54
    std::string MessageCompleted() override;                                  // :63
    void OnLoad(ConfigNode& node) override;                                   // :72
    void OnSave(ConfigNode& node) override;                                   // :84
    bool Generate() override;                                                 // :90

protected:
    CelestialBody* targetBody = nullptr;
    BodyLocation targetLocation = BodyLocation::Space;

private:
    bool GenerateContract_Orbit();                                            // :104
    bool GenerateContract_Surface();                                          // :140
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
    static void Reset();
};

// Contracts/Templates/RecoverAsset.cs:14
class RecoverAsset : public Contract {
public:
    enum class RecoveryLocation : int { NONE, ORBITLOW, ORBITHIGH, SURFACE };          // :16
    enum class RecoveryType : int { NONE, KERBAL, PART, COMPOUND };                    // :24
    RecoverAsset();                                                                    // :105
    std::string TypeName() const override { return "RecoverAsset"; }
    std::shared_ptr<ProtoCrewMember> RecoveryKerbal() const { return recoveryKerbal; } // :46
    CelestialBody* TargetBody() const { return targetBody; }
    RecoveryLocation Location() const { return recoveryLocation; }
    RecoveryType Type() const { return recoveryType; }
    const std::string& PartNameValue() const { return partName; }
    uint32_t PartID() const { return partID; }
    void SetPartID(uint32_t id) { partID = id; }
    bool Generate() override;                                                          // :127
    void OnAccepted() override;                                                        // :200
    void OnParameterStateChange(ContractParameter* p) override;                        // :221
    std::string GetHashString() override;                                              // :255
    std::string GetTitle() override;                                                   // :260
    std::string GetDescription() override;                                             // :302
    std::string GetSynopsys() override;                                                // :316
    std::string GetNotes() override;                                                   // :374
    std::string MessageCompleted() override;                                           // :392
    std::string MessageFailed() override;                                              // :405
    std::string MessageCancelled() override;                                           // :438
    std::string MessageDeadlineExpired() override;                                     // :452
    void OnLoad(ConfigNode& node) override;                                            // :466
    void OnSave(ConfigNode& node) override;                                            // :479
    bool MeetRequirements() override;                                                  // :490
    std::vector<CelestialBody*> WeightBodies() override { return {targetBody}; }       // :512
    bool HasWeightBodies() override { return true; }
    void OnGenerateFailed() override { Cleanup(); }                                    // :569
    void OnDeclined() override { Cleanup(); }                                          // :574
    void OnOfferExpired() override { Cleanup(); }                                      // :579
    void OnFinished() override { Cleanup(); }                                          // :584
    void DiscoverAsset(Vessel* v);                                                     // :960

    std::string VesselDescription();                                                   // :63
    std::string VesselName();                                                          // :78

private:
    bool PartIsValid() const;                                                          // :52
    bool RecoveringKerbal() const { return recoveryType == RecoveryType::KERBAL || recoveryType == RecoveryType::COMPOUND; }  // :66
    bool RecoveringPart() const { return recoveryType == RecoveryType::PART || recoveryType == RecoveryType::COMPOUND; }      // :78
    std::shared_ptr<ProtoCrewMember> ProtoKerbal();                                    // :99
    void Cleanup();                                                                    // :522
    bool Initialize();                                                                 // :588
    void ChooseLocation(KSPRandom* generator = nullptr);                               // :630
    std::string ChooseVesselType(KSPRandom* generator = nullptr);                      // :684
    bool AcceptablePart(Part* p);                                                      // :718
    bool PartHasSeats(const std::string& part);                                        // :833
    float GetMultiplier();                                                             // :887
    void CheckComplete();                                                              // :913
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
    std::string PartName;                                                              // :17
    bool PartIsExperimental = false;                                                   // :20
    bool Hauled() const { return hauled; }                                             // :30
    CelestialBody* Destination() const { return destination; }
    Vessel::Situations TargetSituation() const { return tgtSituation; }
    const AvailablePart* TargetPart() const { return tgtPart; }
    bool MeetRequirements() override;                                                  // :32
    std::vector<CelestialBody*> WeightBodies() override { return {destination}; }      // :40
    bool HasWeightBodies() override { return true; }
    std::string GetDescription() override;                                             // :45
    std::string GetSynopsys() override;                                                // :50
    std::string GetTitle() override;                                                   // :59
    std::string MessageCompleted() override;                                           // :68
    std::string GetSituationString(Vessel::Situations sit, CelestialBody* body, const std::string& biomeName);  // :77
    void OnLoad(ConfigNode& node) override;                                            // :250
    void OnSave(ConfigNode& node) override;                                            // :281
    bool Generate() override;                                                          // :290
    void OnRegister() override;                                                        // :955
    void OnUnregister() override;                                                      // :960
    void OnAccepted() override;                                                        // :965
    void OnPartResearched(AvailablePart* ap);                                          // :978
    void OnFinished() override;                                                        // :990

protected:
    const AvailablePart* tgtPart = nullptr;
    CelestialBody* destination = nullptr;
    Vessel::Situations tgtSituation = static_cast<Vessel::Situations>(0);
    bool hauled = false;
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
