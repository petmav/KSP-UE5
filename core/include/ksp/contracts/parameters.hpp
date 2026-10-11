// WP-37: the stock contract parameters (Assembly-CSharp/Contracts/Parameters/*.cs): AcquireCrew, AcquirePart, CollectScience, EnterOrbit,
// EnterSOI, KerbalDeaths, LandOnBody, OR, XOR, PartTest, PlantFlag, ReachAltitudeEnvelope, ReachBiome, ReachDestination,
// ReachFlightEnvelope, ReachSituation, ReachSpeedEnvelope, RecoverKerbal, RecoverPart.
//
// Every parameter registers/unregisters its GameEvents listeners in OnRegister / OnUnregister (the seams to the flight events), reads the
// active vessel through FlightRegistry (FlightGlobals.ActiveVessel / ready) and the scene through SaveEnvironment (HighLogic.LoadedScene).
// SystemUtilities.LoadNode / FlightIsReady come from fineprint_lite.hpp. The biome lookup (BiomeMap.GetAtt) is ContractsEnvironment::biomeNameAt.
#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ksp/contracts/contract.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/vessel/vessel.hpp"
#include "ksp/vessel/vessel_types.hpp"

namespace ksp {
struct AvailablePart;
class Part;
class ProtoVessel;
class Vessel;
class ScienceSubject;
class ScienceData;
class ModuleTestSubject;
}  // namespace ksp

namespace ksp::Contracts::Templates {
class RecoverAsset;
}

namespace ksp::Contracts::Parameters {

// the shared "body" parameters (EnterOrbit, EnterSOI, LandOnBody, PlantFlag all carry targetBody)
class BodyParameterBase : public ContractParameter {
public:
    CelestialBody* TargetBody() const { return targetBody; }
    std::string GetHashString() override;
    void OnLoad(ConfigNode& node) override;
    void OnSave(ConfigNode& node) override;

protected:
    CelestialBody* targetBody = nullptr;
};

// EnterOrbit.cs:9
class EnterOrbit : public BodyParameterBase {
public:
    EnterOrbit() = default;                                                        // :13
    explicit EnterOrbit(CelestialBody* body) { targetBody = body; }                // :17
    std::string TypeName() const override { return "EnterOrbit"; }
    std::string GetTitle() override;                                               // :27
    void OnRegister() override;                                                    // :45
    void OnUnregister() override;                                                  // :50
    void OnEnterOrbit(Vessel* vessel, CelestialBody* body);                        // :55
};

// EnterSOI.cs:9
class EnterSOI : public BodyParameterBase {
public:
    EnterSOI() = default;
    explicit EnterSOI(CelestialBody* body) { targetBody = body; }
    std::string TypeName() const override { return "EnterSOI"; }
    std::string GetTitle() override;
    void OnRegister() override;
    void OnUnregister() override;
    void OnEnterSOI(Vessel* vessel, CelestialBody* body);
};

// LandOnBody.cs:9
class LandOnBody : public BodyParameterBase {
public:
    LandOnBody() = default;
    explicit LandOnBody(CelestialBody* body) { targetBody = body; }
    std::string TypeName() const override { return "LandOnBody"; }
    std::string GetTitle() override;
    void OnRegister() override;
    void OnUnregister() override;
    void OnVesselLand(Vessel* vessel, CelestialBody* body);
};

// Parameters/PlantFlag.cs:9
class PlantFlag : public BodyParameterBase {
public:
    PlantFlag() = default;
    explicit PlantFlag(CelestialBody* body) { targetBody = body; }
    std::string TypeName() const override { return "PlantFlag"; }
    std::string GetTitle() override;
    void OnRegister() override;
    void OnUnregister() override;
    void OnPlantFlag(Vessel* vessel);
};

// OR.cs:7
class OR : public ContractParameter {
public:
    std::string TypeName() const override { return "OR"; }
    std::string GetHashString() override { return "OR"; }                  // :8
    void OnParameterStateChange(ContractParameter* p) override;            // :13
};

// XOR.cs:7
class XOR : public ContractParameter {
public:
    std::string TypeName() const override { return "XOR"; }
    std::string GetHashString() override { return "XOR"; }                 // :8
    void OnParameterStateChange(ContractParameter* p) override;            // :13
};

// KerbalDeaths.cs:9
class KerbalDeaths : public ContractParameter {
public:
    KerbalDeaths() = default;                                              // :17
    explicit KerbalDeaths(int countMax_);                                  // :21
    std::string TypeName() const override { return "KerbalDeaths"; }
    int CountMax() const { return countMax; }                              // :13
    int CountCurrent() const { return countCurrent; }                      // :15
    std::string GetHashString() override { return ""; }                    // :28 (null)
    std::string GetTitle() override;                                       // :33
    void OnLoad(ConfigNode& node) override;                                // :42
    void OnSave(ConfigNode& node) override;                                // :54
    void OnRegister() override;                                            // :60
    void OnUnregister() override;                                          // :65
    void OnCrewKilled(EventReport* report);                                // :70

protected:
    int countMax = 0;
    int countCurrent = 0;
};

// Parameters/CollectScience.cs:9
class CollectScience : public ContractParameter {
public:
    CollectScience() = default;                                            // :17
    CollectScience(CelestialBody* body, BodyLocation location);            // :21
    std::string TypeName() const override { return "CollectScience"; }
    CelestialBody* TargetBody() const { return targetBody; }               // :13
    BodyLocation TargetLocation() const { return targetLocation; }         // :15
    std::string GetHashString() override;                                  // :27
    std::string GetTitle() override;                                       // :32
    void OnLoad(ConfigNode& node) override;                                // :41
    void OnSave(ConfigNode& node) override;                                // :53
    void OnRegister() override;                                            // :59
    void OnUnregister() override;                                          // :65
    void OnScience(float science, ScienceSubject* subject, ProtoVessel* pv, bool reverseEngineered);  // :71
    void OnTriggeredScience(ScienceData* data, Vessel* origin, bool xmitAborted);                      // :90

protected:
    CelestialBody* targetBody = nullptr;
    BodyLocation targetLocation = BodyLocation::Space;
};

// ReachSituation.cs:9
class ReachSituation : public ContractParameter {
public:
    ReachSituation() = default;                                            // :18
    ReachSituation(Vessel::Situations sit, std::string title_);            // :18
    std::string TypeName() const override { return "ReachSituation"; }
    Vessel::Situations Situation = static_cast<Vessel::Situations>(0);     // :10
    std::string GetTitle() override;                                       // :24
    static std::string GetTitleStringShort(Vessel::Situations sit);        // :29
    void OnLoad(ConfigNode& node) override;                                // :52
    void OnSave(ConfigNode& node) override;                                // :64
    void OnRegister() override;                                            // :70
    void OnUnregister() override;                                          // :75
    void OnReset() override;                                               // :80
    void OnVesselSituationChange(events::GameEvents::HostedFromToAction<Vessel*, Vessel::Situations> v);  // :92
    bool checkVesselSituation(Vessel* v);                                  // :112

protected:
    std::string title;
    void TrackVessel(Vessel* v);                                           // :100
};

// ReachDestination.cs:9
class ReachDestination : public ContractParameter {
public:
    ReachDestination() = default;
    ReachDestination(CelestialBody* destination, std::string title_);
    std::string TypeName() const override { return "ReachDestination"; }
    CelestialBody* Destination = nullptr;                                  // :10
    std::string GetTitle() override;                                       // :24
    static std::string GetTitleStringShort(CelestialBody* dest);           // :29
    void OnLoad(ConfigNode& node) override;                                // :34
    void OnSave(ConfigNode& node) override;                                // :46
    void OnRegister() override;                                            // :52
    void OnUnregister() override;                                          // :57
    void OnReset() override;                                               // :62
    void OnVesselSOIChange(events::GameEvents::HostedFromToAction<Vessel*, CelestialBody*> v);  // :74
    bool checkVesselDestination(Vessel* v);                                // :94

protected:
    std::string title;
    void TrackVessel(Vessel* v);                                           // :82
};

// ReachBiome.cs:9
class ReachBiome : public ContractParameter {
public:
    ReachBiome() = default;
    ReachBiome(const std::string* biomeName, std::string title_);          // :20 (biomeName null -> "")
    std::string TypeName() const override { return "ReachBiome"; }
    std::string BiomeName;                                                 // :8
    std::string GetTitle() override;                                       // :26
    static std::string GetTitleStringShort(const std::string& biomeName);  // :31
    void OnReset() override;                                               // :36
    void OnLoad(ConfigNode& node) override;                                // :41
    void OnSave(ConfigNode& node) override;                                // :53
    void OnUpdate() override;                                              // :59
    bool IsWithinBiome = false;                                            // :14

protected:
    std::string title;
    bool trackerActive = false;
    bool checkVesselWithinBiome(Vessel* v);                                // :79
    bool bodyHasBiome(CelestialBody* body);                                // :92
};

// ReachAltitudeEnvelope.cs:9
class ReachAltitudeEnvelope : public ContractParameter {
public:
    ReachAltitudeEnvelope();                                               // :14
    ReachAltitudeEnvelope(float minAlt, float maxAlt);                     // :20
    std::string TypeName() const override { return "ReachAltitudeEnvelope"; }
    double minAltitude = 0.0;                                              // :10
    double maxAltitude = 0.0;                                              // :12
    std::string GetTitle() override;                                       // :26
    void OnReset() override;                                               // :31
    void OnLoad(ConfigNode& node) override;                                // :36
    void OnSave(ConfigNode& node) override;                                // :42
    void OnUpdate() override;                                              // :48
};

// ReachSpeedEnvelope.cs:9
class ReachSpeedEnvelope : public ContractParameter {
public:
    ReachSpeedEnvelope();                                                  // :14
    ReachSpeedEnvelope(float minSpd, float maxSpd);                        // :20
    std::string TypeName() const override { return "ReachSpeedEnvelope"; }
    double minSpeed = 0.0;                                                 // :10
    double maxSpeed = 0.0;                                                 // :12
    std::string GetTitle() override;                                       // :26
    void OnReset() override;                                               // :31
    void OnLoad(ConfigNode& node) override;                                // :36
    void OnSave(ConfigNode& node) override;                                // :42
    void OnUpdate() override;                                              // :48
    bool checkVesselWithinEnvelope(Vessel* v);                             // :64
};

// ReachFlightEnvelope.cs:11
class ReachFlightEnvelope : public ContractParameter {
public:
    ReachFlightEnvelope() = default;                                       // :35
    ReachFlightEnvelope(CelestialBody* dest, Vessel::Situations sit, const std::string* biomeName, float maxAlt, float minAlt, float maxSpd,
                        float minSpd, std::string title_);                 // :39
    std::string TypeName() const override { return "ReachFlightEnvelope"; }
    CelestialBody* Destination = nullptr;                                  // :11
    Vessel::Situations Situation = static_cast<Vessel::Situations>(0);     // :13
    std::string BiomeName;                                                 // :15
    bool useAltLimits = false, useSpdLimits = false;                       // :17, :21
    double maxAltitude = 0.0, minAltitude = 0.0, maxSpeed = 0.0, minSpeed = 0.0;  // :21-:29
    std::string GetTitle() override;                                       // :53
    static std::string GetSituationStringShort(Vessel::Situations sit, CelestialBody* body, const std::string& biomeName);  // :67
    void OnLoad(ConfigNode& node) override;                                // :112
    void OnSave(ConfigNode& node) override;                                // :156
    void OnRegister() override;                                            // :170
    void OnUnregister() override;                                          // :185
    void OnFlightReady();                                                  // :196
    void OnVesselChange(Vessel* v);                                        // :202
    void OnVesselSituationChange(events::GameEvents::HostedFromToAction<Vessel*, Vessel::Situations> v);  // :208
    Enumerator TrackVessel(Vessel* v);                                     // :216
    bool checkVesselWithinBiome(Vessel* v);                                // :240
    bool checkVesselWithinFlightEnvelope(Vessel* v);                       // :253

protected:
    std::string title;
    Vessel* tgtVessel = nullptr;
    bool trackerActive = false;
};

// RecoverKerbal.cs:11
class RecoverKerbal : public ContractParameter {
public:
    enum class CompleteCondition : int { All, Any };                       // :11
    RecoverKerbal() = default;                                             // :31
    explicit RecoverKerbal(std::string title_, CompleteCondition win = CompleteCondition::All, CompleteCondition fail = CompleteCondition::All);  // :35
    std::string TypeName() const override { return "RecoverKerbal"; }
    std::string GetTitle() override { return title; }                      // :43
    void OnLoad(ConfigNode& node) override;                                // :48
    void OnSave(ConfigNode& node) override;                                // :70
    void OnRegister() override;                                            // :82
    void OnUnregister() override;                                          // :95
    void OnVesselRecovered(ProtoVessel* v, bool quick);                    // :107
    void OnCrewKilled(EventReport* evt);                                   // :156
    void OnPartDied(Part* p);                                              // :198
    void AddKerbal(const std::string& kerbalName);                         // :221
    void RemoveKerbal(const std::string& kerbalName);                      // :229
    void SetTitle(const std::string& t) { title = t; }                     // :237
    void OnVesselLoad(Vessel* v);                                          // :242
    void OnFlightReady();                                                  // :250
    std::vector<std::string> kerbalsToRecover;                             // :18
    std::string title;                                                     // :21
    CompleteCondition failCondition = CompleteCondition::All;              // :24
    CompleteCondition winCondition = CompleteCondition::All;               // :27

private:
    bool eventsAdded = false;
};

// RecoverPart.cs:10
class RecoverPart : public ContractParameter {
public:
    enum class CompleteCondition : int { None, AllCandidates, AnyCandidate };  // :11
    RecoverPart();                                                         // :36
    RecoverPart(std::string title_, CompleteCondition win, CompleteCondition fail);  // :36
    std::string TypeName() const override { return "RecoverPart"; }
    std::string GetTitle() override { return title; }                      // :44
    void OnLoad(ConfigNode& node) override;                                // :49
    void OnSave(ConfigNode& node) override;                                // :62
    void OnRegister() override;                                            // :74
    void OnUnregister() override;                                          // :86
    void OnVesselRecovered(ProtoVessel* v, bool quick);                    // :97
    void OnPartDie(Part* p);                                               // :134
    void AddPartToRecover(uint32_t partFlightId);                          // :153
    void RemovePartToRecover(uint32_t partFlightId);                       // :161
    void OnVesselLoad(Vessel* v);                                          // :169
    void OnFlightReady();                                                  // :177
    std::vector<uint32_t> partsToRecover;                                  // :18
    std::string title;                                                     // :20
    CompleteCondition failCondition;                                       // :22
    CompleteCondition winCondition;                                        // :24

private:
    bool eventsAdded = false;
};

// AcquireCrew.cs:11
class AcquireCrew : public ContractParameter {
public:
    enum class CompleteCondition : int { All, Any };                       // :11
    AcquireCrew() = default;                                               // :28
    explicit AcquireCrew(std::string title_, CompleteCondition win = CompleteCondition::All);  // :32
    std::string TypeName() const override { return "AcquireCrew"; }
    void SetTitle(const std::string& t) { title = t; }                     // :39
    std::string GetTitle() override { return title; }                      // :44
    void OnLoad(ConfigNode& node) override;                                // :49
    void OnSave(ConfigNode& node) override;                                // :61
    void OnRegister() override;                                            // :72
    void OnUnregister() override;                                          // :86
    void OnReset() override;                                               // :98
    void OnCrewBoardVessel(events::GameEvents::FromToAction<Part*, Part*> action);                       // :106
    void OnCrewTransferred(events::GameEvents::HostedFromToAction<ProtoCrewMember*, Part*> action);      // :111
    void OnCouple(events::GameEvents::FromToAction<Part*, Part*> action);                                // :116
    void OnCrewKilled(EventReport* evt);                                   // :121
    void OnPartDied(Part* p);                                              // :126
    void AddKerbal(const std::string& kerbalName);                         // :135
    void RemoveKerbal(const std::string& kerbalName);                      // :143
    std::vector<std::string> kerbalsToRecover;                             // :18
    std::string title;                                                     // :21
    CompleteCondition winCondition = CompleteCondition::All;               // :24

private:
    void ScanVessel(Vessel* v);                                            // :151
    void CheckNewKerbal(const std::string& kerbalName);                    // :165
    void KillKerbal(const std::string& kerbalName);                        // :181
    bool eventsAdded = false;
};

// AcquirePart.cs:10
class AcquirePart : public ContractParameter {
public:
    enum class CompleteCondition : int { All, Any };                       // :11
    AcquirePart() = default;
    explicit AcquirePart(std::string title_, CompleteCondition win = CompleteCondition::All);
    std::string TypeName() const override { return "AcquirePart"; }
    void SetTitle(const std::string& t) { title = t; }
    std::string GetTitle() override { return title; }
    void OnLoad(ConfigNode& node) override;
    void OnSave(ConfigNode& node) override;
    void OnRegister() override;
    void OnUnregister() override;
    void OnReset() override;
    void OnCouple(events::GameEvents::FromToAction<Part*, Part*> action);
    void OnPartDied(Part* p);
    void AddPart(uint32_t id);
    void RemovePart(uint32_t id);
    std::vector<uint32_t> partsToRecover;
    std::string title;
    CompleteCondition winCondition = CompleteCondition::All;

private:
    void ScanVessel(Vessel* v);
    void CheckNewPart(uint32_t id);
    void DestroyPart(uint32_t id);
    bool eventsAdded = false;
};

// Parameters/PartTest.cs:12 (Contracts.Parameters.PartTest)
class PartTest : public ContractParameter {
public:
    PartTest();                                                            // :42
    PartTest(const AvailablePart* tgtPart, const std::optional<std::string>& testNotes_, int partRepeatability, CelestialBody* tgtBody, Vessel::Situations tgtSit,
             const std::string& id_, bool hauled_);                        // :36
    std::string TypeName() const override { return "PartTest"; }
    std::string partName;                                                  // :12
    const AvailablePart* tgtPartInfo = nullptr;                            // :14
    std::vector<uint32_t> testedParts;                                     // :16
    std::optional<std::string> testNotes;                                  // :18 (null = nullopt)
    std::string body;                                                      // :20
    std::string situation;                                                 // :22
    std::string uniqueID;                                                  // :24
    int repeatability = 2;                                                 // :26 PartTestConstraint.TestRepeatability.ONCEPERPART
    bool hauled = false;                                                   // :28
    std::string title;                                                     // :40
    std::string GetHashString() override;                                  // :60
    std::string GetTitle() override;                                       // :75
    std::string GetNotes() override { return testNotes.value_or(std::string()); }  // :89
    void OnLoad(ConfigNode& node) override;                                // :94
    void OnSave(ConfigNode& node) override;                                // :155
    void OnRegister() override;                                            // :178
    void OnUnregister() override;                                          // :198
    void OnPartRunTest(ModuleTestSubject* p);                              // :216
    void OnTestPartDestroyed(ModuleTestSubject* p);                        // :228
    void OnVesselChanged(Vessel* v);                                       // :236
    void OnReset() override;                                               // :265
    void OnUpdate() override;                                              // :298

private:
    bool hasTestedPart(Vessel* v);                                         // :251
    void VesselModified(Vessel* v);                                        // :274
    void PartModified(Part* p);                                            // :282
    void OnDock(events::GameEvents::FromToAction<Part*, Part*> action);    // :290
    bool eventsAdded = false;
    bool validVessel = false;
    bool dirtyVessel = true;
    int successCounter = 0;
    int activePartCount = 0;
};

}  // namespace ksp::Contracts::Parameters

template <>
struct ksp::dotnet::EnumTraits<ksp::Contracts::Parameters::RecoverKerbal::CompleteCondition> {
    using E = ksp::Contracts::Parameters::RecoverKerbal::CompleteCondition;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::All, "All"}, {E::Any, "Any"}};
};
template <>
struct ksp::dotnet::EnumTraits<ksp::Contracts::Parameters::RecoverPart::CompleteCondition> {
    using E = ksp::Contracts::Parameters::RecoverPart::CompleteCondition;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::None, "None"}, {E::AllCandidates, "AllCandidates"}, {E::AnyCandidate, "AnyCandidate"}};
};
template <>
struct ksp::dotnet::EnumTraits<ksp::Contracts::Parameters::AcquireCrew::CompleteCondition> {
    using E = ksp::Contracts::Parameters::AcquireCrew::CompleteCondition;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::All, "All"}, {E::Any, "Any"}};
};
template <>
struct ksp::dotnet::EnumTraits<ksp::Contracts::Parameters::AcquirePart::CompleteCondition> {
    using E = ksp::Contracts::Parameters::AcquirePart::CompleteCondition;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::All, "All"}, {E::Any, "Any"}};
};
