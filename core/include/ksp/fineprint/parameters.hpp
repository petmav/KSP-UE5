// WP-38 slice 2: the 24 FinePrint contract parameters that were deferred in slice 1 (Assembly-CSharp/FinePrint/Contracts/Parameters/*.cs; the
// ProgressTrackingParameter is in fineprint.hpp): AsteroidParameter, CometDetectionParameter, CometParameter, CometScienceParameter,
// ConstructionParameter, CrewCapacityParameter, CrewTraitParameter, KerbalDestinationParameter, KerbalGeeAdventureParameter, KerbalTourParameter,
// LocationAndSituationParameter, MobileBaseParameter, PartRequestParameter, RepairPartParameter, ResourceExtractionParameter,
// ResourcePossessionParameter, RoverWayPointParameter, SpecificOrbitParameter, SpecificVesselParameter, StabilityParameter,
// StationaryPointParameter, SurveyWaypointParameter, VesselDestinationParameter, VesselSystemsParameter, and the abstract WaypointParameter
// (FinePrint/Utilities/WaypointParameter.cs).
//
// The members keep the C# names; C# `protected` / `private` members are public (the tests drive them). The OnUpdate bodies read the active vessel
// through FlightRegistry and the scene through SaveEnvironment, like the WP-37 parameters. The seams are in FinePrintEnvironment
// (fineprint/utilities.hpp).
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "ksp/fineprint/fineprint.hpp"
#include "ksp/fineprint/utilities.hpp"

namespace ksp::Contracts::Parameters {

namespace GE_ = ksp::events;
using Lite::FlightBand;
using Lite::MannedStatus;
using Lite::WaypointClusterState;
using ksp::FinePrint::OrbitType;
using ksp::FinePrint::Waypoint;

// FinePrint/Utilities/WaypointParameter.cs:5 (abstract)
class WaypointParameter : public ContractParameter {
public:
    CelestialBody* TargetBody = nullptr;                      // :7
    virtual void SetupWaypoints() {}                          // :9
    virtual void CleanupWaypoints() {}                        // :13
    virtual void UpdateWaypoints(bool focused) { (void)focused; }  // :17
};

// StabilityParameter.cs:9
class StabilityParameter : public ContractParameter {
public:
    float holdSeconds = 10.0f;                                // :11
    float holdTimer = 10.0f;                                  // :13
    StabilityParameter();                                     // :15
    explicit StabilityParameter(float holdSeconds_);          // :21
    std::string TypeName() const override { return "StabilityParameter"; }
    std::string GetHashString() override;                     // :27
    std::string GetTitle() override;                          // :32
    void OnRegister() override;                               // :37
    void OnReset() override;                                  // :42
    void OnSave(ConfigNode& node) override;                   // :47
    void OnLoad(ConfigNode& node) override;                   // :52
    void OnUpdate() override;                                 // :57
};

// CrewCapacityParameter.cs:9
class SpecificVesselParameter;
class CrewCapacityParameter : public ContractParameter {
public:
    int targetCapacity = 8;                                   // :10
    int successCounter = 0;                                   // :12
    bool validVessel = false;                                 // :14
    bool dirtyVessel = true;                                  // :16
    int activePartCount = 0;                                  // :18
    bool eventsAdded = false;                                 // :20
    SpecificVesselParameter* specificVesselSibling = nullptr;  // :22
    CrewCapacityParameter();                                  // :26
    explicit CrewCapacityParameter(int targetCapacity_);      // :32
    std::string TypeName() const override { return "CrewCapacityParameter"; }
    SpecificVesselParameter* SpecificVesselSibling();         // :24
    std::string GetHashString() override;                     // :38
    std::string GetTitle() override;                          // :43
    std::string GetNotes() override;                          // :48
    void OnRegister() override;                               // :66
    void OnUnregister() override;                             // :79
    void OnReset() override;                                  // :89
    void VesselModified(Vessel* v);                           // :95
    void PartModified(Part* p);                               // :103
    void OnDock(GE_::GameEvents::FromToAction<Part*, Part*> action);  // :111
    void OnSave(ConfigNode& node) override;                   // :119
    void OnLoad(ConfigNode& node) override;                   // :124
    void OnUpdate() override;                                 // :129
};

// CrewTraitParameter.cs:9
class CrewTraitParameter : public ContractParameter {
public:
    std::string vesselDescription;                            // :10
    std::string targetTrait = "Scientist";                    // :12
    int targetCount = 1;                                      // :14
    int successCounter = 0;                                   // :16
    bool validVessel = false;                                 // :18
    bool dirtyVessel = true;                                  // :20
    int activePartCount = 0;                                  // :22
    bool eventsAdded = false;                                 // :24
    SpecificVesselParameter* specificVesselSibling = nullptr;  // :26
    CrewTraitParameter();                                     // :48
    CrewTraitParameter(const std::string& targetTrait_, int targetCount_, const std::string& vesselDescription_);  // :48
    std::string TypeName() const override { return "CrewTraitParameter"; }
    std::string targetTraitDisplayName();                     // :28
    SpecificVesselParameter* SpecificVesselSibling();         // :40
    std::string GetHashString() override;                     // :55
    std::string GetTitle() override;                          // :60
    std::string GetNotes() override;                          // :69
    void OnRegister() override;                               // :87
    void OnUnregister() override;                             // :101
    void OnReset() override;                                  // :112
    void VesselModified(Vessel* v);                           // :118
    void PartModified(Part* p);                               // :126
    void OnDock(GE_::GameEvents::FromToAction<Part*, Part*> action);  // :134
    void CrewModified(Vessel* v);                             // :142
    void OnSave(ConfigNode& node) override;                   // :150
    void OnLoad(ConfigNode& node) override;                   // :157
    void OnUpdate() override;                                 // :164
};

// MobileBaseParameter.cs:9
class MobileBaseParameter : public ContractParameter {
public:
    int successCounter = 0;                                   // :10
    bool validVessel = false;                                 // :12
    bool dirtyVessel = true;                                  // :14
    int activePartCount = 0;                                  // :16
    bool eventsAdded = false;                                 // :18
    MobileBaseParameter();                                    // :20
    std::string TypeName() const override { return "MobileBaseParameter"; }
    std::string GetHashString() override;                     // :25
    std::string GetTitle() override;                          // :30
    void OnRegister() override;                               // :35
    void OnUnregister() override;                             // :48
    void OnReset() override;                                  // :59
    void VesselModified(Vessel* v);                           // :65
    void PartModified(Part* p);                               // :73
    void OnDock(GE_::GameEvents::FromToAction<Part*, Part*> action);  // :81
    void ChangeSituation(GE_::GameEvents::HostedFromToAction<Vessel*, Vessel::Situations> action);  // :89
    void OnUpdate() override;                                 // :97
};

// LocationAndSituationParameter.cs:9
class LocationAndSituationParameter : public ContractParameter {
public:
    CelestialBody* targetBody = nullptr;                      // :11
    Vessel::Situations targetSituation = Vessel::Situations::ESCAPING;  // :13
    std::string noun = "potato";                              // :15
    bool finalObjective = false;                              // :17
    bool validVessel = false;                                 // :19
    bool dirtyVessel = true;                                  // :21
    int successCounter = 0;                                   // :23
    bool eventsAdded = false;                                 // :25
    LocationAndSituationParameter();                          // :27
    LocationAndSituationParameter(CelestialBody* targetBody_, Vessel::Situations targetSituation_, const std::string& noun_, bool finalObjective_ = false);  // :36
    std::string TypeName() const override { return "LocationAndSituationParameter"; }
    void OnRegister() override;                               // :45
    void OnUnregister() override;                             // :56
    void OnReset() override;                                  // :65
    void ChangeBody(GE_::GameEvents::HostedFromToAction<Vessel*, CelestialBody*> action);  // :71
    void ChangeSituation(GE_::GameEvents::HostedFromToAction<Vessel*, Vessel::Situations> action);  // :79
    std::string GetHashString() override;                     // :87
    std::string GetTitle() override;                          // :92
    void OnSave(ConfigNode& node) override;                   // :121
    void OnLoad(ConfigNode& node) override;                   // :129
    void OnUpdate() override;                                 // :137
};

// VesselDestinationParameter.cs:11
class VesselDestinationParameter : public ContractParameter {
public:
    CelestialBody* targetBody = nullptr;                      // :11
    FlightLog_EntryType targetType = FlightLog_EntryType::Suborbit;  // :13
    bool eventsAdded = false;                                 // :15
    VesselDestinationParameter();                             // :17
    VesselDestinationParameter(CelestialBody* targetBody_, FlightLog_EntryType targetType_);  // :23
    std::string TypeName() const override { return "VesselDestinationParameter"; }
    std::string GetHashString() override;                     // :29
    std::string GetTitle() override;                          // :34
    void OnRegister() override;                               // :53
    void OnUnregister() override;                             // :64
    void OnReset() override;                                  // :73
    void OnSave(ConfigNode& node) override;                   // :81
    void OnLoad(ConfigNode& node) override;                   // :87
    void CheckFlightLog(Vessel* v);                           // :93
};

// KerbalDestinationParameter.cs:11
class KerbalDestinationParameter : public ContractParameter {
public:
    CelestialBody* targetBody = nullptr;                      // :11
    FlightLog_EntryType targetType = FlightLog_EntryType::Suborbit;  // :13
    std::string kerbalName = "Jebediah Kerman";               // :15
    bool eventsAdded = false;                                 // :17
    KerbalDestinationParameter();                             // :19
    KerbalDestinationParameter(CelestialBody* targetBody_, FlightLog_EntryType targetType_, const std::string& kerbalName_);  // :26
    std::string TypeName() const override { return "KerbalDestinationParameter"; }
    std::string GetHashString() override;                     // :33
    std::string GetTitle() override;                          // :38
    void OnRegister() override;                               // :57
    void OnUnregister() override;                             // :67
    void OnSave(ConfigNode& node) override;                   // :75
    void OnLoad(ConfigNode& node) override;                   // :82
    void CheckFlightLog(Vessel* v);                           // :89
    bool CheckFlightLogs();                                   // :113
};

// KerbalGeeAdventureParameter.cs:9
class KerbalGeeAdventureParameter : public ContractParameter {
public:
    std::string kerbalName = "Jebediah Kerman";               // :10
    bool eventsAdded = false;                                 // :12
    KerbalGeeAdventureParameter();                            // :14
    explicit KerbalGeeAdventureParameter(const std::string& kerbalName_);  // :19
    std::string TypeName() const override { return "KerbalGeeAdventureParameter"; }
    std::string GetHashString() override;                     // :25
    std::string GetTitle() override;                          // :30
    void OnRegister() override;                               // :35
    void OnUnregister() override;                             // :46
    void OnSave(ConfigNode& node) override;                   // :55
    void OnLoad(ConfigNode& node) override;                   // :60
    void OnCrewKilled(EventReport* evt);                      // :65
    void OnKerbalPassedOut(ProtoCrewMember* pcm);             // :81
};

// KerbalTourParameter.cs:9
class KerbalTourParameter : public ContractParameter {
public:
    std::string kerbalName = "Jebediah Kerman";               // :11
    ProtoCrewMember_Gender kerbalGender = ProtoCrewMember_Gender::Male;  // :13
    bool eventsAdded = false;                                 // :15
    bool kerbalInactive = false;                              // :17
    KerbalTourParameter();                                    // :19
    KerbalTourParameter(const std::string& kerbalName_, ProtoCrewMember_Gender kerbalGender_);  // :25
    std::string TypeName() const override { return "KerbalTourParameter"; }
    std::string GetHashString() override;                     // :32
    std::string GetTitle() override;                          // :37
    void OnRegister() override;                               // :42
    void OnUnregister() override;                             // :54
    void OnSave(ConfigNode& node) override;                   // :64
    void OnLoad(ConfigNode& node) override;                   // :70
    void OnCrewKilled(EventReport* evt);                      // :76
    void OnParameterStateChange(ContractParameter* p) override;  // :92
    void OnVesselRecovered(ProtoVessel* v, KSP::UI::Screens::MissionRecoveryDialog* mrd, float x);  // :104
    void OnKerbalInactiveChange(ProtoCrewMember* pcm, bool oldValue, bool newValue);  // :112
    std::string GetMessageFailed() override;                  // :125
};

// VesselSystemsParameter.cs:9
class VesselSystemsParameter : public ContractParameter {
public:
    bool requireNew = false;                                  // :11
    std::vector<std::string> checkModuleTypes;                // :13
    std::vector<std::string> checkModuleDescriptions;         // :15
    MannedStatus mannedStatus = MannedStatus::MANNED;         // :17 (default(MannedStatus) = MANNED)
    std::string vesselDescription;                            // :19
    uint32_t launchID = 0;                                    // :21
    int successCounter = 0;                                   // :23
    bool validVessel = false;                                 // :25
    bool dirtyVessel = true;                                  // :27
    int activePartCount = 0;                                  // :29
    bool eventsAdded = false;                                 // :31
    VesselSystemsParameter();                                 // :49
    VesselSystemsParameter(const std::vector<std::string>& checkModuleTypes_, const std::vector<std::string>& checkModuleDescriptions_,
                           const std::string& vesselDescription_, MannedStatus mannedStatus_ = MannedStatus::ANY, bool requireNew_ = true);  // (C++ addition)
    std::string TypeName() const override { return "VesselSystemsParameter"; }
    std::string mannedString();                               // :33
    std::string GetHashString() override;                     // :66
    std::string GetTitle() override;                          // :71
    std::string GetNotes() override;                          // :92
    void OnRegister() override;                               // :101
    void OnUnregister() override;                             // :114
    void OnReset() override;                                  // :124
    void VesselModified(Vessel* v);                           // :130
    void PartModified(Part* p);                               // :138
    void OnDock(GE_::GameEvents::FromToAction<Part*, Part*> action);  // :146
    void OnSave(ConfigNode& node) override;                   // :154
    void OnLoad(ConfigNode& node) override;                   // :167
    void OnUpdate() override;                                 // :180
    bool isVesselValid(Vessel* v);                            // :218
};

// PartRequestParameter.cs:9
class PartRequestParameter : public ContractParameter {
public:
    int successCounter = 0;                                   // :11
    std::string article = "a";                                // :13
    std::string partDescription = "part";                     // :15
    std::string vesselDescription = "vessel";                 // :17
    std::vector<std::string> partNames;                       // :19
    std::vector<std::string> moduleNames;                     // :21
    std::vector<uint32_t> existingParts;                      // :23
    bool validVessel = false;                                 // :25
    bool dirtyVessel = true;                                  // :27
    int activePartCount = 0;                                  // :29
    bool eventsAdded = false;                                 // :31
    PartRequestParameter();                                   // :33
    explicit PartRequestParameter(ConfigNode& requestNode, Vessel* v = nullptr);  // :46
    std::string TypeName() const override { return "PartRequestParameter"; }
    std::string GetHashString() override;                     // :67
    std::string GetTitle() override;                          // :72
    void OnRegister() override;                               // :81
    void OnUnregister() override;                             // :93
    void OnReset() override;                                  // :103
    void VesselModified(Vessel* v);                           // :109
    void PartModified(Part* p);                               // :117
    void OnDock(GE_::GameEvents::FromToAction<Part*, Part*> action);  // :125
    void OnSave(ConfigNode& node) override;                   // :133
    void OnLoad(ConfigNode& node) override;                   // :143
    void OnUpdate() override;                                 // :154
    void CheckLists();                                        // :192
    bool VesselHasPartRequest(Vessel* v = nullptr);           // :201
    void ScanVesselForExistingParts(Vessel* v);               // :276
};

// ResourcePossessionParameter.cs:8
class ResourcePossessionParameter : public ContractParameter {
public:
    int successCounter = 0;                                   // :11
    double currentResource = 0.0;                             // :13
    double goalResource = 1000.0;                             // :15
    std::string resourceName = "Matter";                      // :17
    std::string resourceTitle = "matter";                     // :19
    std::string vesselName = "vessel";                        // :21
    SpecificVesselParameter* specificVesselSibling = nullptr;  // :23
    ResourcePossessionParameter();                            // :27
    ResourcePossessionParameter(const std::string& resourceName_, const std::string& resourceTitle_, const std::string& vesselName_, double goalResource_);  // :37
    std::string TypeName() const override { return "ResourcePossessionParameter"; }
    SpecificVesselParameter* SpecificVesselSibling();         // :25
    std::string GetHashString() override;                     // :47
    std::string GetTitle() override;                          // :52
    std::string GetNotes() override;                          // :57
    void OnRegister() override;                               // :75
    void OnReset() override;                                  // :80
    void OnSave(ConfigNode& node) override;                   // :85
    void OnLoad(ConfigNode& node) override;                   // :93
    void OnUpdate() override;                                 // :101
};

// ResourceExtractionParameter.cs:9
class ResourceExtractionParameter : public ContractParameter {
public:
    CelestialBody* targetBody = nullptr;                      // :12
    int successCounter = 0;                                   // :14
    double totalHarvested = 0.0;                              // :16
    double goalHarvested = 1000.0;                            // :18
    std::string resourceName = "Matter";                      // :20
    std::string resourceTitle = "matter";                     // :22
    std::vector<std::string> modules;                         // :24
    bool eventsAdded = false;                                 // :26
    float notifyLevel = 0.25f;                                // :28
    ResourceExtractionParameter();                            // :30
    ResourceExtractionParameter(const std::string& resourceName_, const std::string& resourceTitle_, double goalHarvested_, CelestialBody* targetBody_,
                                const std::vector<std::string>& modules_);  // :41
    std::string TypeName() const override { return "ResourceExtractionParameter"; }
    std::string GetHashString() override;                     // :56
    std::string GetTitle() override;                          // :61
    void OnRegister() override;                               // :66
    void OnUnregister() override;                             // :76
    void OnReset() override;                                  // :84
    void ResourcesHarvested(PartModule* module, const std::string& resource, double amount);  // :89
    void OnSave(ConfigNode& node) override;                   // :105
    void OnLoad(ConfigNode& node) override;                   // :116
    void OnUpdate() override;                                 // :127
};

// AsteroidParameter.cs:11 / CometParameter.cs:11 (same body: ModuleAsteroid / ModuleComet)
class AsteroidParameter : public ContractParameter {
public:
    bool forStation = false;                                  // :11
    std::vector<int32_t> asteroidSeeds;                       // :13
    std::string asteroidClass = "A";                          // :15
    int successCounter = 0;                                   // :17
    bool validVessel = false;                                 // :19
    bool dirtyVessel = true;                                  // :21
    int activePartCount = 0;                                  // :23
    bool eventsAdded = false;                                 // :25
    AsteroidParameter();                                      // :27
    AsteroidParameter(const std::string& size, bool forStation_);  // :34
    std::string TypeName() const override { return "AsteroidParameter"; }
    std::string GetHashString() override;                     // :41
    std::string GetTitle() override;                          // :46
    std::string GetNotes() override;                          // :55
    void OnRegister() override;                               // :60
    void OnUnregister() override;                             // :73
    void OnReset() override;                                  // :83
    void VesselModified(Vessel* v);                           // :89
    void PartModified(Part* p);                               // :97
    void OnDock(GE_::GameEvents::FromToAction<Part*, Part*> action);  // :105
    void OnSave(ConfigNode& node) override;                   // :157
    void OnLoad(ConfigNode& node) override;                   // :164
    void OnUpdate() override;                                 // :171
    bool VesselTowingAsteroid(Vessel* v);                     // :207
};

class CometParameter : public ContractParameter {
public:
    bool forStation = false;                                  // :11
    std::vector<int32_t> cometSeeds;                          // (C++ addition)
    std::string cometClass = "A";                             // (C++ addition)
    int successCounter = 0;                                   // :17
    bool validVessel = false;                                 // :19
    bool dirtyVessel = true;                                  // :21
    int activePartCount = 0;                                  // :23
    bool eventsAdded = false;                                 // :25
    CometParameter();                                         // (C++ addition)
    CometParameter(const std::string& size, bool forStation_);  // (C++ addition)
    std::string TypeName() const override { return "CometParameter"; }
    std::string GetHashString() override;                     // :41
    std::string GetTitle() override;                          // :46
    std::string GetNotes() override;                          // :55
    void OnRegister() override;                               // :60
    void OnUnregister() override;                             // :73
    void OnReset() override;                                  // :83
    void VesselModified(Vessel* v);                           // :89
    void PartModified(Part* p);                               // :97
    void OnDock(GE_::GameEvents::FromToAction<Part*, Part*> action);  // :105
    void OnSave(ConfigNode& node) override;                   // :157
    void OnLoad(ConfigNode& node) override;                   // :164
    void OnUpdate() override;                                 // :171
    bool VesselTowingComet(Vessel* v);                        // (C++ addition)
};

// CometDetectionParameter.cs:10
class CometDetectionParameter : public ContractParameter {
public:
    int cometCountRequired = 0;                               // :11
    int cometCountDetected = 0;                               // :13
    std::string sentinelPartName;                             // :15
    int RemainingDiscoveries = 0;                             // :17
    CometDetectionParameter();                                // :19
    CometDetectionParameter(int cometCountRequired_, const std::string& sentinelPartName_);  // :24
    std::string TypeName() const override { return "CometDetectionParameter"; }
    std::string GetHashString() override;                     // :31
    std::string GetTitle() override;                          // :36
    std::string GetNotes() override;                          // :41
    void OnRegister() override;                               // :46
    void OnUnregister() override;                             // :52
    void CometDetected(Vessel* data);                         // :57
    void OnReset() override;                                  // :75
    void OnSave(ConfigNode& node) override;                   // :80
    void OnLoad(ConfigNode& node) override;                   // :86
};

// CometScienceParameter.cs:8
class CometScienceParameter : public ContractParameter {
public:
    uint32_t cometPersistentId = 0;                           // :10
    std::string cometName;                                    // :12
    std::string scienceSubjectId;                             // :14
    bool scienceDataCollected = false;                        // :16
    uint32_t CometPersistentId() const { return cometPersistentId; }   // :18
    const std::string& CometName() const { return cometName; }         // :20
    const std::string& ScienceSubjectId() const { return scienceSubjectId; }  // :22
    bool ScienceDataCollected() const { return scienceDataCollected; }  // :24
    CometScienceParameter() = default;                        // :26
    CometScienceParameter(uint32_t cometId, const std::string& cometName_, const std::string& scienceSubjectId_);  // :30
    std::string TypeName() const override { return "CometScienceParameter"; }
    std::string GetHashString() override;                     // :38
    std::string GetTitle() override;                          // :43
    void OnLoad(ConfigNode& node) override;                   // :48
    void OnSave(ConfigNode& node) override;                   // :56
    void OnRegister() override;                               // :64
    void OnUnregister() override;                             // :73
    void OnExperimentDeployed(ScienceData* experimentData);   // :82
    void OnVesselRename(GE_::GameEvents::HostedFromToAction<Vessel*, std::string> hostedFromTo);  // :90
    void VesselIdChanged(uint32_t oldId, uint32_t newId);     // :99
    void OnScience(float science, ScienceSubject* subject, ProtoVessel* pv, bool reverseEngineered);  // :107
    void OnTriggeredScience(ScienceData* data, Vessel* origin, bool xmitAborted);  // :116
};

// ConstructionParameter.cs:9
class ConstructionParameter : public ContractParameter {
public:
    CelestialBody* targetBody = nullptr;                      // :10
    uint32_t vesselPersistentId = 0;                          // :12
    std::string partName;                                     // :14
    std::string vesselName;                                   // :16
    const AvailablePart* ap = nullptr;                        // :18
    Vessel* vsl = nullptr;                                    // :20
    CelestialBody* TargetBody() const { return targetBody; }  // :22
    uint32_t VesselPersistentId() const { return vesselPersistentId; }  // :24
    const std::string& PartName() const { return partName; }  // :26
    const std::string& VesselName() const { return vesselName; }  // :28
    ConstructionParameter() = default;                        // :30
    ConstructionParameter(const std::string& partName_, uint32_t vesselId, const std::string& vesselName_, CelestialBody* targetBody_);  // :34
    std::string TypeName() const override { return "ConstructionParameter"; }
    void UpdatePartInfo(const std::string& partName_, uint32_t vesselId, const std::string& vesselName_, CelestialBody* targetBody_);  // :39
    std::string GetHashString() override;                     // :58
    std::string GetTitle() override;                          // :63
    void OnLoad(ConfigNode& node) override;                   // :72
    void OnSave(ConfigNode& node) override;                   // :86
    void OnRegister() override;                               // :94
    void OnUnregister() override;                             // :100
    std::string GetMessageComplete() override;                // :106
    void VesselIdChanged(uint32_t oldId, uint32_t newId);     // :111
    void OnConstructionPartAttached(Vessel* vsl_, Part* p);   // :119
};

// RepairPartParameter.cs:9
class RepairPartParameter : public ContractParameter {
public:
    CelestialBody* targetBody = nullptr;                      // :10
    uint32_t vesselPersistentId = 0;                          // :12
    uint32_t partPersistentId = 0;                            // :14
    std::string partName;                                     // :16
    std::string vesselName = "";                              // :18
    const AvailablePart* ap = nullptr;                        // :20
    Vessel* vsl = nullptr;                                    // :22
    CelestialBody* TargetBody() const { return targetBody; }  // :24
    uint32_t VesselPersistentId() const { return vesselPersistentId; }  // :26
    uint32_t PartPersistentId() const { return partPersistentId; }      // :28
    const std::string& PartName() const { return partName; }  // :30
    const std::string& VesselName() const { return vesselName; }  // :32
    RepairPartParameter() = default;                          // :34
    RepairPartParameter(uint32_t partId, const std::string& partName_, uint32_t vesselId, const std::string& vesselName_, CelestialBody* targetBody_);  // :38
    std::string TypeName() const override { return "RepairPartParameter"; }
    void UpdatePartInfo(uint32_t partId, const std::string& partName_, uint32_t vesselId, const std::string& vesselName_, CelestialBody* targetBody_);  // :43
    std::string GetHashString() override;                     // :63
    std::string GetTitle() override;                          // :68
    void OnLoad(ConfigNode& node) override;                   // :77
    void OnSave(ConfigNode& node) override;                   // :92
    void OnRegister() override;                               // :104
    void OnUnregister() override;                             // :111
    std::string GetMessageComplete() override;                // :118
    void PartIdChanged(uint32_t vslId, uint32_t oldId, uint32_t newId);  // :123
    void VesselIdChanged(uint32_t oldId, uint32_t newId);     // :131
    void OnPartRepaired(Part* p);                             // :139
};

// SpecificVesselParameter.cs:12
class SpecificVesselParameter : public WaypointParameter {
public:
    std::vector<uint32_t> partIDs;                            // :14
    Vessel* targetVessel = nullptr;                           // :16
    Guid targetVesselID;                                      // :18
    int targetVesselPartCount = 1;                            // :20
    std::string targetVesselName = "Vessel";                  // :22
    Vessel::Situations targetVesselSituation = Vessel::Situations::ORBITING;  // :24
    std::unique_ptr<Waypoint> wp;                             // :26
    bool tracked = false;                                     // :28
    bool submittedWaypoint = false;                           // :30
    int successCounter = 0;                                   // :32
    SpecificVesselParameter();                                // :34
    explicit SpecificVesselParameter(Vessel* v);              // :46
    std::string TypeName() const override { return "SpecificVesselParameter"; }
    std::string GetHashString() override;                     // :58
    std::string GetTitle() override;                          // :63
    std::string GetMessageComplete() override;                // :68
    std::string GetNotes() override;                          // :73
    void OnRegister() override;                               // :102
    void OnUnregister() override;                             // :107
    void OnReset() override;                                  // :112
    void OnSave(ConfigNode& node) override;                   // :117
    void OnLoad(ConfigNode& node) override;                   // :127
    void OnUpdate() override;                                 // :196
    void UpdateWaypoints(bool focus) override;                // :223
    void SpecificVesselRename(const Guid& id, const std::string& name);  // :251
    void SpecificVesselScan(const std::vector<Guid>& ids);    // :260
    void TargetVesselSearch(bool cached);                     // :281
    void CleanupWaypoints() override;                         // :326
    void KillContract();                                      // :335
    // ContractsApp.Instance (UI) != null: the SpecificVesselRename refresh is a seam (unset: no ContractsApp)
    static inline std::function<void(Contract&, bool changePositions)> refreshContractRequested;
};

// SpecificOrbitParameter.cs:11
class SpecificOrbitParameter : public ContractParameter {
public:
    double deviationWindow = 10.0;                            // :11
    int successCounter = 0;                                   // :15
    OrbitType orbitType = OrbitType::RANDOM;                  // :17
    std::unique_ptr<Orbit> orbit;                             // :19
    double inclination = 0.0;                                 // :21
    double eccentricity = 0.0;                                // :23
    double sma = 10000000.0;                                  // :25
    double lan = 0.0;                                         // :27
    double argumentOfPeriapsis = 0.0;                         // :29
    double meanAnomalyAtEpoch = 0.0;                          // :31
    double epoch = 0.0;                                       // :33
    CelestialBody* TargetBody = nullptr;                      // :35
    bool rendererSetUp = false;                               // (orbitRenderer != null)
    bool rendererActiveDraw = false;                          // orbitRenderer.activeDraw
    Orbit* Orbit_() { return orbit.get(); }                   // (C++ addition) Orbit
    SpecificOrbitParameter();                                 // :39
    SpecificOrbitParameter(OrbitType orbitType_, double inclination_, double eccentricity_, double sma_, double lan_, double argumentOfPeriapsis_,
                           double meanAnomalyAtEpoch_, double epoch_, CelestialBody* targetBody_, double deviationWindow_);  // :54
    std::string TypeName() const override { return "SpecificOrbitParameter"; }
    std::string GetHashString() override;                     // :71
    std::string GetTitle() override;                          // :76
    std::string GetNotes() override;                          // :126
    void OnRegister() override;                               // :147
    void OnUnregister() override;                             // :152
    void OnReset() override;                                  // :157
    void OnSave(ConfigNode& node) override;                   // :162
    void OnLoad(ConfigNode& node) override;                   // :181
    void OnUpdate() override;                                 // :208
    void SetupRenderer();                                     // :253
    void CleanupRenderer();                                   // :261
    void ValidateOrbit();                                     // :269
};

// StationaryPointParameter.cs:9
class StationaryPointParameter : public ContractParameter {
public:
    std::unique_ptr<Waypoint> wp;                             // :11
    CelestialBody* targetBody = nullptr;                      // :13
    bool submittedWaypoint = false;                           // :15
    double longitude = 0.0;                                   // :17
    double deviation = 0.0;                                   // :19
    int successCounter = 0;                                   // :21
    StationaryPointParameter();                               // :23
    StationaryPointParameter(CelestialBody* targetBody_, double longitude_, double deviation_);  // :33
    std::string TypeName() const override { return "StationaryPointParameter"; }
    std::string GetHashString() override;                     // :43
    std::string GetTitle() override;                          // :48
    std::string GetMessageComplete() override;                // :53
    void OnRegister() override;                               // :58
    void OnUnregister() override;                             // :63
    void OnReset() override;                                  // :71
    void OnSave(ConfigNode& node) override;                   // :76
    void OnLoad(ConfigNode& node) override;                   // :83
    void OnUpdate() override;                                 // :126
};

// RoverWayPointParameter.cs:12
class RoverWayPointParameter : public WaypointParameter {
public:
    Vessel* roverVessel = nullptr;                            // :10
    uint32_t roverVslId = 0;                                  // :12
    std::unique_ptr<Waypoint> startwp;                        // :14
    std::unique_ptr<Waypoint> endwp;                          // :16
    double craftStartLatitude = 0.0;                          // :18
    double craftStartLongitude = 0.0;                         // :20
    double craftEndLatitude = 0.0;                            // :22
    double craftEndLongitude = 0.0;                           // :24
    bool submittedWaypoint = false;                           // :26
    int successCounter = 0;                                   // :28
    bool outerWarning = false;                                // :30
    Vessel* RoverVessel() const { return roverVessel; }       // :32
    uint32_t RoverVslId() const { return roverVslId; }        // :34
    Waypoint* Startwp() const { return startwp.get(); }       // :36
    Waypoint* Endwp() const { return endwp.get(); }           // :38
    double CraftStartLatitude() const { return craftStartLatitude; }    // :40
    double CraftStartLongitude() const { return craftStartLongitude; }  // :42
    double CraftEndLatitude() const { return craftEndLatitude; }        // :44
    double CraftEndLongitude() const { return craftEndLongitude; }      // :46
    RoverWayPointParameter();                                 // :48
    RoverWayPointParameter(double CraftStartLatitude_, double CraftStartLongitude_, double CraftEndLatitude_, double CraftEndLongitude_,
                           CelestialBody* targetBody);        // :65
    std::string TypeName() const override { return "RoverWayPointParameter"; }
    std::string GetHashString() override;                     // :76
    std::string GetTitle() override;                          // :81
    std::string GetMessageComplete() override;                // :86
    std::string GetNotes() override;                          // :91
    void OnRegister() override;                               // :100
    void OnUnregister() override;                             // :106
    void OnReset() override;                                  // :112
    void OnSave(ConfigNode& node) override;                   // :117
    void OnLoad(ConfigNode& node) override;                   // :131
    void OnUpdate() override;                                 // :216
    void UpdateWaypoints(bool focus) override;                // :268
    void UpdateRoverInfo(Vessel* vsl);                        // :286
    void CleanupWaypoints() override;                         // :292
    void VesselIdChanged(uint32_t oldId, uint32_t newId);     // :302
};

// SurveyWaypointParameter.cs:13
class SurveyWaypointParameter : public ContractParameter {
public:
    std::unique_ptr<Waypoint> wp;                             // :13
    WaypointClusterState clusterState = WaypointClusterState::NONE;  // :15
    std::string experiment;                                   // :17
    std::string actionDescription;                            // :19
    double thresholdAltitude = 0.0;                           // :21
    FlightBand band = FlightBand::NONE;                       // :23
    CelestialBody* targetBody = nullptr;                      // :25
    bool submittedWaypoint = false;                           // :27
    bool outerWarning = false;                                // :29
    bool verified = false;                                    // :31
    bool contextual = false;                                  // :33
    int bouncesLeft = 0;                                      // :35
    double bounceOriginLatitude = 0.0;                        // :37
    double bounceOriginLongitude = 0.0;                       // :39
    bool eventsAdded = false;                                 // :41
    std::string contextualAnomalyName;                        // :43
    std::string ContextualAnomalyName();                      // :45
    SurveyWaypointParameter();                                // :62
    SurveyWaypointParameter(const std::string& experiment_, const std::string& actionDescription_, CelestialBody* targetBody_, const Waypoint& wp_,
                            FlightBand band_);                // :77
    SurveyWaypointParameter(const std::string& experiment_, const std::string& actionDescription_, CelestialBody* targetBody_, const Waypoint& wp_,
                            FlightBand band_, bool contextual_, int bouncesLeft_, double bounceOriginLatitude_, double bounceOriginLongitude_);  // :92
    std::string TypeName() const override { return "SurveyWaypointParameter"; }
    std::string GetHashString() override;                     // :113
    std::string GetTitle() override;                          // :118
    std::string GetMessageComplete() override;                // :142
    void OnRegister() override;                               // :155
    void OnUnregister() override;                             // :164
    void OnSave(ConfigNode& node) override;                   // :180
    void OnLoad(ConfigNode& node) override;                   // :205
    void OnUpdate() override;                                 // :236
    void CheckExperimentResults(ScienceData* experimentData);  // :287
    void Bounce();                                            // :325
    static double TriggerRange(FlightBand band);              // :356
    void ProcessWaypoint();                                   // :379
    void CalculateAltitudes(double terrainAltitude);          // :432
    void VerifyWaypoint(double terrainAltitude);              // :489
    std::string SiteString();                                 // :510
    Waypoint* NextWaypointInCluster();                        // :521
    void UpdateNavWaypoint(Waypoint* newWaypoint);            // :541
};

}  // namespace ksp::Contracts::Parameters
