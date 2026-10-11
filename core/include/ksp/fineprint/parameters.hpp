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
    int targetCapacity = 8;                                   // :11
    int successCounter = 0;                                   // :13
    bool validVessel = false;                                 // :15
    bool dirtyVessel = true;                                  // :17
    int activePartCount = 0;                                  // :19
    bool eventsAdded = false;                                 // :21
    SpecificVesselParameter* specificVesselSibling = nullptr;  // :23
    CrewCapacityParameter();                                  // :27
    explicit CrewCapacityParameter(int targetCapacity_);      // :33
    std::string TypeName() const override { return "CrewCapacityParameter"; }
    SpecificVesselParameter* SpecificVesselSibling();         // :25
    std::string GetHashString() override;                     // :41
    std::string GetTitle() override;                          // :46
    std::string GetNotes() override;                          // :51
    void OnRegister() override;                               // :68
    void OnUnregister() override;                             // :82
    void OnReset() override;                                  // :94
    void VesselModified(Vessel* v);                           // :100
    void PartModified(Part* p);                               // :108
    void OnDock(GE_::GameEvents::FromToAction<Part*, Part*> action);  // :116
    void OnSave(ConfigNode& node) override;                   // :124
    void OnLoad(ConfigNode& node) override;                   // :129
    void OnUpdate() override;                                 // :134
};

// CrewTraitParameter.cs:9
class CrewTraitParameter : public ContractParameter {
public:
    std::string vesselDescription;                            // :11
    std::string targetTrait = "Scientist";                    // :13
    int targetCount = 1;                                      // :15
    int successCounter = 0;                                   // :17
    bool validVessel = false;                                 // :19
    bool dirtyVessel = true;                                  // :21
    int activePartCount = 0;                                  // :23
    bool eventsAdded = false;                                 // :25
    SpecificVesselParameter* specificVesselSibling = nullptr;  // :27
    CrewTraitParameter();                                     // :48
    CrewTraitParameter(const std::string& targetTrait_, int targetCount_, const std::string& vesselDescription_);  // :54
    std::string TypeName() const override { return "CrewTraitParameter"; }
    std::string targetTraitDisplayName();                     // :29
    SpecificVesselParameter* SpecificVesselSibling();         // :46
    std::string GetHashString() override;                     // :62
    std::string GetTitle() override;                          // :67
    std::string GetNotes() override;                          // :76
    void OnRegister() override;                               // :95
    void OnUnregister() override;                             // :111
    void OnReset() override;                                  // :125
    void VesselModified(Vessel* v);                           // :131
    void PartModified(Part* p);                               // :139
    void OnDock(GE_::GameEvents::FromToAction<Part*, Part*> action);  // :147
    void CrewModified(Vessel* v);                             // :155
    void OnSave(ConfigNode& node) override;                   // :163
    void OnLoad(ConfigNode& node) override;                   // :170
    void OnUpdate() override;                                 // :177
};

// MobileBaseParameter.cs:9
class MobileBaseParameter : public ContractParameter {
public:
    int successCounter = 0;                                   // :11
    bool validVessel = false;                                 // :13
    bool dirtyVessel = true;                                  // :15
    int activePartCount = 0;                                  // :17
    bool eventsAdded = false;                                 // :19
    MobileBaseParameter();                                    // :21
    std::string TypeName() const override { return "MobileBaseParameter"; }
    std::string GetHashString() override;                     // :26
    std::string GetTitle() override;                          // :31
    void OnRegister() override;                               // :36
    void OnUnregister() override;                             // :52
    void OnReset() override;                                  // :66
    void VesselModified(Vessel* v);                           // :72
    void PartModified(Part* p);                               // :80
    void OnDock(GE_::GameEvents::FromToAction<Part*, Part*> action);  // :88
    void ChangeSituation(GE_::GameEvents::HostedFromToAction<Vessel*, Vessel::Situations> action);  // :96
    void OnUpdate() override;                                 // :104
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
    void OnRegister() override;                               // :46
    void OnUnregister() override;                             // :57
    void OnReset() override;                                  // :66
    void ChangeBody(GE_::GameEvents::HostedFromToAction<Vessel*, CelestialBody*> action);  // :72
    void ChangeSituation(GE_::GameEvents::HostedFromToAction<Vessel*, Vessel::Situations> action);  // :80
    std::string GetHashString() override;                     // :88
    std::string GetTitle() override;                          // :93
    void OnSave(ConfigNode& node) override;                   // :124
    void OnLoad(ConfigNode& node) override;                   // :132
    void OnUpdate() override;                                 // :142
};

// VesselDestinationParameter.cs:11
class VesselDestinationParameter : public ContractParameter {
public:
    CelestialBody* targetBody = nullptr;                      // :13
    FlightLog_EntryType targetType = FlightLog_EntryType::Suborbit;  // :15
    bool eventsAdded = false;                                 // :17
    VesselDestinationParameter();                             // :19
    VesselDestinationParameter(CelestialBody* targetBody_, FlightLog_EntryType targetType_);  // :25
    std::string TypeName() const override { return "VesselDestinationParameter"; }
    std::string GetHashString() override;                     // :31
    std::string GetTitle() override;                          // :36
    void OnRegister() override;                               // :56
    void OnUnregister() override;                             // :68
    void OnReset() override;                                  // :79
    void OnSave(ConfigNode& node) override;                   // :87
    void OnLoad(ConfigNode& node) override;                   // :93
    void CheckFlightLog(Vessel* v);                           // :99
};

// KerbalDestinationParameter.cs:11
class KerbalDestinationParameter : public ContractParameter {
public:
    CelestialBody* targetBody = nullptr;                      // :13
    FlightLog_EntryType targetType = FlightLog_EntryType::Suborbit;  // :15
    std::string kerbalName = "Jebediah Kerman";               // :17
    bool eventsAdded = false;                                 // :19
    KerbalDestinationParameter();                             // :21
    KerbalDestinationParameter(CelestialBody* targetBody_, FlightLog_EntryType targetType_, const std::string& kerbalName_);  // :29
    std::string TypeName() const override { return "KerbalDestinationParameter"; }
    std::string GetHashString() override;                     // :37
    std::string GetTitle() override;                          // :42
    void OnRegister() override;                               // :62
    void OnUnregister() override;                             // :74
    void OnSave(ConfigNode& node) override;                   // :82
    void OnLoad(ConfigNode& node) override;                   // :90
    void CheckFlightLog(Vessel* v);                           // :98
    bool CheckFlightLogs();                                   // :122
};

// KerbalGeeAdventureParameter.cs:9
class KerbalGeeAdventureParameter : public ContractParameter {
public:
    std::string kerbalName = "Jebediah Kerman";               // :11
    bool eventsAdded = false;                                 // :13
    KerbalGeeAdventureParameter();                            // :15
    explicit KerbalGeeAdventureParameter(const std::string& kerbalName_);  // :20
    std::string TypeName() const override { return "KerbalGeeAdventureParameter"; }
    std::string GetHashString() override;                     // :26
    std::string GetTitle() override;                          // :31
    void OnRegister() override;                               // :36
    void OnUnregister() override;                             // :48
    void OnSave(ConfigNode& node) override;                   // :58
    void OnLoad(ConfigNode& node) override;                   // :63
    void OnCrewKilled(EventReport* evt);                      // :68
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
    void OnUnregister() override;                             // :56
    void OnSave(ConfigNode& node) override;                   // :69
    void OnLoad(ConfigNode& node) override;                   // :75
    void OnCrewKilled(EventReport* evt);                      // :81
    void OnParameterStateChange(ContractParameter* p) override;  // :94
    void OnVesselRecovered(ProtoVessel* v, KSP::UI::Screens::MissionRecoveryDialog* mrd, float x);  // :107
    void OnKerbalInactiveChange(ProtoCrewMember* pcm, bool oldValue, bool newValue);  // :114
    std::string GetMessageFailed() override;                  // :127
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
    VesselSystemsParameter();                                 // :48
    VesselSystemsParameter(const std::vector<std::string>& checkModuleTypes_, const std::vector<std::string>& checkModuleDescriptions_,
                           const std::string& vesselDescription_, MannedStatus mannedStatus_ = MannedStatus::ANY, bool requireNew_ = true);  // :56
    std::string TypeName() const override { return "VesselSystemsParameter"; }
    std::string mannedString();                               // :33
    std::string GetHashString() override;                     // :66
    std::string GetTitle() override;                          // :71
    std::string GetNotes() override;                          // :90
    void OnRegister() override;                               // :99
    void OnUnregister() override;                             // :115
    void OnReset() override;                                  // :129
    void VesselModified(Vessel* v);                           // :135
    void PartModified(Part* p);                               // :143
    void OnDock(GE_::GameEvents::FromToAction<Part*, Part*> action);  // :151
    void OnSave(ConfigNode& node) override;                   // :159
    void OnLoad(ConfigNode& node) override;                   // :171
    void OnUpdate() override;                                 // :184
    bool isVesselValid(Vessel* v);                            // :220
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
    explicit PartRequestParameter(ConfigNode& requestNode, Vessel* v = nullptr);  // :48
    std::string TypeName() const override { return "PartRequestParameter"; }
    std::string GetHashString() override;                     // :68
    std::string GetTitle() override;                          // :73
    void OnRegister() override;                               // :82
    void OnUnregister() override;                             // :95
    void OnReset() override;                                  // :106
    void VesselModified(Vessel* v);                           // :112
    void PartModified(Part* p);                               // :120
    void OnDock(GE_::GameEvents::FromToAction<Part*, Part*> action);  // :128
    void OnSave(ConfigNode& node) override;                   // :136
    void OnLoad(ConfigNode& node) override;                   // :146
    void OnUpdate() override;                                 // :157
    void CheckLists();                                        // :189
    bool VesselHasPartRequest(Vessel* v = nullptr);           // :198
    void ScanVesselForExistingParts(Vessel* v);               // :274
};

// ResourcePossessionParameter.cs:8
class ResourcePossessionParameter : public ContractParameter {
public:
    int successCounter = 0;                                   // :10
    double currentResource = 0.0;                             // :12
    double goalResource = 1000.0;                             // :14
    std::string resourceName = "Matter";                      // :16
    std::string resourceTitle = "matter";                     // :18
    std::string vesselName = "vessel";                        // :20
    SpecificVesselParameter* specificVesselSibling = nullptr;  // :22
    ResourcePossessionParameter();                            // :26
    ResourcePossessionParameter(const std::string& resourceName_, const std::string& resourceTitle_, const std::string& vesselName_, double goalResource_);  // :35
    std::string TypeName() const override { return "ResourcePossessionParameter"; }
    SpecificVesselParameter* SpecificVesselSibling();         // :24
    std::string GetHashString() override;                     // :45
    std::string GetTitle() override;                          // :50
    std::string GetNotes() override;                          // :55
    void OnRegister() override;                               // :71
    void OnReset() override;                                  // :76
    void OnSave(ConfigNode& node) override;                   // :81
    void OnLoad(ConfigNode& node) override;                   // :89
    void OnUpdate() override;                                 // :97
};

// ResourceExtractionParameter.cs:9
class ResourceExtractionParameter : public ContractParameter {
public:
    CelestialBody* targetBody = nullptr;                      // :11
    int successCounter = 0;                                   // :13
    double totalHarvested = 0.0;                              // :15
    double goalHarvested = 1000.0;                            // :17
    std::string resourceName = "Matter";                      // :19
    std::string resourceTitle = "matter";                     // :21
    std::vector<std::string> modules;                         // :23
    bool eventsAdded = false;                                 // :25
    float notifyLevel = 0.25f;                                // :27
    ResourceExtractionParameter();                            // :29
    ResourceExtractionParameter(const std::string& resourceName_, const std::string& resourceTitle_, double goalHarvested_, CelestialBody* targetBody_,
                                const std::vector<std::string>& modules_);  // :41
    std::string TypeName() const override { return "ResourceExtractionParameter"; }
    std::string GetHashString() override;                     // :53
    std::string GetTitle() override;                          // :58
    void OnRegister() override;                               // :63
    void OnUnregister() override;                             // :73
    void OnReset() override;                                  // :81
    void ResourcesHarvested(PartModule* module, const std::string& resource, double amount);  // :86
    void OnSave(ConfigNode& node) override;                   // :104
    void OnLoad(ConfigNode& node) override;                   // :116
    void OnUpdate() override;                                 // :127
};

// AsteroidParameter.cs:11 / CometParameter.cs:11 (same body: ModuleAsteroid / ModuleComet)
class AsteroidParameter : public ContractParameter {
public:
    bool forStation = false;                                  // :13
    std::vector<int32_t> asteroidSeeds;                       // :15
    std::string asteroidClass = "A";                          // :17
    int successCounter = 0;                                   // :19
    bool validVessel = false;                                 // :21
    bool dirtyVessel = true;                                  // :23
    int activePartCount = 0;                                  // :25
    bool eventsAdded = false;                                 // :27
    AsteroidParameter();                                      // :29
    AsteroidParameter(const std::string& size, bool forStation_);  // :35
    std::string TypeName() const override { return "AsteroidParameter"; }
    std::string GetHashString() override;                     // :43
    std::string GetTitle() override;                          // :48
    std::string GetNotes() override;                          // :57
    void OnRegister() override;                               // :62
    void OnUnregister() override;                             // :77
    void OnReset() override;                                  // :91
    void VesselModified(Vessel* v);                           // :97
    void PartModified(Part* p);                               // :105
    void OnDock(GE_::GameEvents::FromToAction<Part*, Part*> action);  // :113
    void OnSave(ConfigNode& node) override;                   // :171
    void OnLoad(ConfigNode& node) override;                   // :178
    void OnUpdate() override;                                 // :185
    bool VesselTowingAsteroid(Vessel* v);                     // :213
};

class CometParameter : public ContractParameter {
public:
    bool forStation = false;                                  // :13
    std::vector<int32_t> cometSeeds;                          // :15
    std::string cometClass = "A";                             // :17
    int successCounter = 0;                                   // :19
    bool validVessel = false;                                 // :21
    bool dirtyVessel = true;                                  // :23
    int activePartCount = 0;                                  // :25
    bool eventsAdded = false;                                 // :27
    CometParameter();                                         // :29
    CometParameter(const std::string& size, bool forStation_);  // :35
    std::string TypeName() const override { return "CometParameter"; }
    std::string GetHashString() override;                     // :43
    std::string GetTitle() override;                          // :48
    std::string GetNotes() override;                          // :57
    void OnRegister() override;                               // :62
    void OnUnregister() override;                             // :77
    void OnReset() override;                                  // :91
    void VesselModified(Vessel* v);                           // :97
    void PartModified(Part* p);                               // :105
    void OnDock(GE_::GameEvents::FromToAction<Part*, Part*> action);  // :113
    void OnSave(ConfigNode& node) override;                   // :171
    void OnLoad(ConfigNode& node) override;                   // :178
    void OnUpdate() override;                                 // :185
    bool VesselTowingComet(Vessel* v);                        // :213
};

// CometDetectionParameter.cs:10
class CometDetectionParameter : public ContractParameter {
public:
    int cometCountRequired = 0;                               // :12
    int cometCountDetected = 0;                               // :14
    std::string sentinelPartName;                             // :16
    int RemainingDiscoveries = 0;                             // :18
    CometDetectionParameter();                                // :20
    CometDetectionParameter(int cometCountRequired_, const std::string& sentinelPartName_);  // :25
    std::string TypeName() const override { return "CometDetectionParameter"; }
    std::string GetHashString() override;                     // :33
    std::string GetTitle() override;                          // :38
    std::string GetNotes() override;                          // :43
    void OnRegister() override;                               // :48
    void OnUnregister() override;                             // :54
    void CometDetected(Vessel* data);                         // :59
    void OnReset() override;                                  // :75
    void OnSave(ConfigNode& node) override;                   // :80
    void OnLoad(ConfigNode& node) override;                   // :86
};

// CometScienceParameter.cs:8
class CometScienceParameter : public ContractParameter {
public:
    uint32_t cometPersistentId = 0;                           // :12
    std::string cometName;                                    // :14
    std::string scienceSubjectId;                             // :16
    bool scienceDataCollected = false;                        // :18
    uint32_t CometPersistentId() const { return cometPersistentId; }   // :20
    const std::string& CometName() const { return cometName; }         // :22
    const std::string& ScienceSubjectId() const { return scienceSubjectId; }  // :24
    bool ScienceDataCollected() const { return scienceDataCollected; }  // :26
    CometScienceParameter() = default;                        // :28
    CometScienceParameter(uint32_t cometId, const std::string& cometName_, const std::string& scienceSubjectId_);  // :32
    std::string TypeName() const override { return "CometScienceParameter"; }
    std::string GetHashString() override;                     // :40
    std::string GetTitle() override;                          // :45
    void OnLoad(ConfigNode& node) override;                   // :50
    void OnSave(ConfigNode& node) override;                   // :58
    void OnRegister() override;                               // :66
    void OnUnregister() override;                             // :75
    void OnExperimentDeployed(ScienceData* experimentData);   // :84
    void OnVesselRename(GE_::GameEvents::HostedFromToAction<Vessel*, std::string> hostedFromTo);  // :92
    void VesselIdChanged(uint32_t oldId, uint32_t newId);     // :104
    void OnScience(float science, ScienceSubject* subject, ProtoVessel* pv, bool reverseEngineered);  // :112
    void OnTriggeredScience(ScienceData* data, Vessel* origin, bool xmitAborted);  // :120
};

// ConstructionParameter.cs:9
class ConstructionParameter : public ContractParameter {
public:
    CelestialBody* targetBody = nullptr;                      // :12
    uint32_t vesselPersistentId = 0;                          // :14
    std::string partName;                                     // :16
    std::string vesselName;                                   // :18
    const AvailablePart* ap = nullptr;                        // :20
    Vessel* vsl = nullptr;                                    // :22
    CelestialBody* TargetBody() const { return targetBody; }  // :24
    uint32_t VesselPersistentId() const { return vesselPersistentId; }  // :26
    const std::string& PartName() const { return partName; }  // :28
    const std::string& VesselName() const { return vesselName; }  // :30
    ConstructionParameter() = default;                        // :32
    ConstructionParameter(const std::string& partName_, uint32_t vesselId, const std::string& vesselName_, CelestialBody* targetBody_);  // :36
    std::string TypeName() const override { return "ConstructionParameter"; }
    void UpdatePartInfo(const std::string& partName_, uint32_t vesselId, const std::string& vesselName_, CelestialBody* targetBody_);  // :41
    std::string GetHashString() override;                     // :60
    std::string GetTitle() override;                          // :65
    void OnLoad(ConfigNode& node) override;                   // :74
    void OnSave(ConfigNode& node) override;                   // :90
    void OnRegister() override;                               // :98
    void OnUnregister() override;                             // :104
    std::string GetMessageComplete() override;                // :110
    void VesselIdChanged(uint32_t oldId, uint32_t newId);     // :115
    void OnConstructionPartAttached(Vessel* vsl_, Part* p);   // :123
};

// RepairPartParameter.cs:9
class RepairPartParameter : public ContractParameter {
public:
    CelestialBody* targetBody = nullptr;                      // :12
    uint32_t vesselPersistentId = 0;                          // :14
    uint32_t partPersistentId = 0;                            // :16
    std::string partName;                                     // :18
    std::string vesselName = "";                              // :20
    const AvailablePart* ap = nullptr;                        // :22
    Vessel* vsl = nullptr;                                    // :24
    CelestialBody* TargetBody() const { return targetBody; }  // :26
    uint32_t VesselPersistentId() const { return vesselPersistentId; }  // :28
    uint32_t PartPersistentId() const { return partPersistentId; }      // :30
    const std::string& PartName() const { return partName; }  // :32
    const std::string& VesselName() const { return vesselName; }  // :34
    RepairPartParameter() = default;                          // :36
    RepairPartParameter(uint32_t partId, const std::string& partName_, uint32_t vesselId, const std::string& vesselName_, CelestialBody* targetBody_);  // :40
    std::string TypeName() const override { return "RepairPartParameter"; }
    void UpdatePartInfo(uint32_t partId, const std::string& partName_, uint32_t vesselId, const std::string& vesselName_, CelestialBody* targetBody_);  // :45
    std::string GetHashString() override;                     // :66
    std::string GetTitle() override;                          // :71
    void OnLoad(ConfigNode& node) override;                   // :80
    void OnSave(ConfigNode& node) override;                   // :96
    void OnRegister() override;                               // :108
    void OnUnregister() override;                             // :115
    std::string GetMessageComplete() override;                // :122
    void PartIdChanged(uint32_t vslId, uint32_t oldId, uint32_t newId);  // :127
    void VesselIdChanged(uint32_t oldId, uint32_t newId);     // :135
    void OnPartRepaired(Part* p);                             // :143
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
    explicit SpecificVesselParameter(Vessel* v);              // :45
    std::string TypeName() const override { return "SpecificVesselParameter"; }
    std::string GetHashString() override;                     // :57
    std::string GetTitle() override;                          // :62
    std::string GetMessageComplete() override;                // :67
    std::string GetNotes() override;                          // :72
    void OnRegister() override;                               // :99
    void OnUnregister() override;                             // :104
    void OnReset() override;                                  // :109
    void OnSave(ConfigNode& node) override;                   // :114
    void OnLoad(ConfigNode& node) override;                   // :125
    void OnUpdate() override;                                 // :199
    void UpdateWaypoints(bool focus) override;                // :228
    void SpecificVesselRename(const Guid& id, const std::string& name);  // :257
    void SpecificVesselScan(const std::vector<Guid>& ids);    // :267
    void TargetVesselSearch(bool cached);                     // :287
    void CleanupWaypoints() override;                         // :331
    void KillContract();                                      // :340
    // ContractsApp.Instance (UI) != null: the SpecificVesselRename refresh is a seam (unset: no ContractsApp)
    static inline std::function<void(Contract&, bool changePositions)> refreshContractRequested;
};

// SpecificOrbitParameter.cs:11
class SpecificOrbitParameter : public ContractParameter {
public:
    double deviationWindow = 10.0;                            // :13
    int successCounter = 0;                                   // :17
    OrbitType orbitType = OrbitType::RANDOM;                  // :19
    std::unique_ptr<Orbit> orbit;                             // :21
    double inclination = 0.0;                                 // :23
    double eccentricity = 0.0;                                // :25
    double sma = 10000000.0;                                  // :27
    double lan = 0.0;                                         // :29
    double argumentOfPeriapsis = 0.0;                         // :31
    double meanAnomalyAtEpoch = 0.0;                          // :33
    double epoch = 0.0;                                       // :35
    CelestialBody* TargetBody = nullptr;                      // :37
    bool rendererSetUp = false;                               // (orbitRenderer != null)
    bool rendererActiveDraw = false;                          // orbitRenderer.activeDraw
    Orbit* Orbit_() { return orbit.get(); }                   // :39 Orbit
    SpecificOrbitParameter();                                 // :41
    SpecificOrbitParameter(OrbitType orbitType_, double inclination_, double eccentricity_, double sma_, double lan_, double argumentOfPeriapsis_,
                           double meanAnomalyAtEpoch_, double epoch_, CelestialBody* targetBody_, double deviationWindow_);  // :54
    std::string TypeName() const override { return "SpecificOrbitParameter"; }
    std::string GetHashString() override;                     // :72
    std::string GetTitle() override;                          // :77
    std::string GetNotes() override;                          // :135
    void OnRegister() override;                               // :157
    void OnUnregister() override;                             // :162
    void OnReset() override;                                  // :167
    void OnSave(ConfigNode& node) override;                   // :172
    void OnLoad(ConfigNode& node) override;                   // :192
    void OnUpdate() override;                                 // :216
    void SetupRenderer();                                     // :273
    void CleanupRenderer();                                   // :281
    void ValidateOrbit();                                     // :289
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
    void OnUpdate() override;                                 // :124
};

// RoverWayPointParameter.cs:12
class RoverWayPointParameter : public WaypointParameter {
public:
    Vessel* roverVessel = nullptr;                            // :14
    uint32_t roverVslId = 0;                                  // :16
    std::unique_ptr<Waypoint> startwp;                        // :18
    std::unique_ptr<Waypoint> endwp;                          // :20
    double craftStartLatitude = 0.0;                          // :22
    double craftStartLongitude = 0.0;                         // :24
    double craftEndLatitude = 0.0;                            // :26
    double craftEndLongitude = 0.0;                           // :28
    bool submittedWaypoint = false;                           // :30
    int successCounter = 0;                                   // :32
    bool outerWarning = false;                                // :34
    Vessel* RoverVessel() const { return roverVessel; }       // :36
    uint32_t RoverVslId() const { return roverVslId; }        // :38
    Waypoint* Startwp() const { return startwp.get(); }       // :40
    Waypoint* Endwp() const { return endwp.get(); }           // :42
    double CraftStartLatitude() const { return craftStartLatitude; }    // :44
    double CraftStartLongitude() const { return craftStartLongitude; }  // :46
    double CraftEndLatitude() const { return craftEndLatitude; }        // :48
    double CraftEndLongitude() const { return craftEndLongitude; }      // :50
    RoverWayPointParameter();                                 // :52
    RoverWayPointParameter(double CraftStartLatitude_, double CraftStartLongitude_, double CraftEndLatitude_, double CraftEndLongitude_,
                           CelestialBody* targetBody);        // :65
    std::string TypeName() const override { return "RoverWayPointParameter"; }
    std::string GetHashString() override;                     // :79
    std::string GetTitle() override;                          // :84
    std::string GetMessageComplete() override;                // :89
    std::string GetNotes() override;                          // :94
    void OnRegister() override;                               // :103
    void OnUnregister() override;                             // :109
    void OnReset() override;                                  // :116
    void OnSave(ConfigNode& node) override;                   // :121
    void OnLoad(ConfigNode& node) override;                   // :135
    void OnUpdate() override;                                 // :221
    void UpdateWaypoints(bool focus) override;                // :272
    void UpdateRoverInfo(Vessel* vsl);                        // :290
    void CleanupWaypoints() override;                         // :296
    void VesselIdChanged(uint32_t oldId, uint32_t newId);     // :305
};

// SurveyWaypointParameter.cs:13
class SurveyWaypointParameter : public ContractParameter {
public:
    std::unique_ptr<Waypoint> wp;                             // :15
    WaypointClusterState clusterState = WaypointClusterState::NONE;  // :17
    std::string experiment;                                   // :19
    std::string actionDescription;                            // :21
    double thresholdAltitude = 0.0;                           // :23
    FlightBand band = FlightBand::NONE;                       // :25
    CelestialBody* targetBody = nullptr;                      // :27
    bool submittedWaypoint = false;                           // :29
    bool outerWarning = false;                                // :31
    bool verified = false;                                    // :33
    bool contextual = false;                                  // :35
    int bouncesLeft = 0;                                      // :37
    double bounceOriginLatitude = 0.0;                        // :39
    double bounceOriginLongitude = 0.0;                       // :41
    bool eventsAdded = false;                                 // :43
    std::string contextualAnomalyName;                        // :45
    std::string ContextualAnomalyName();                      // :47
    SurveyWaypointParameter();                                // :63
    SurveyWaypointParameter(const std::string& experiment_, const std::string& actionDescription_, CelestialBody* targetBody_, const Waypoint& wp_,
                            FlightBand band_);                // :77
    SurveyWaypointParameter(const std::string& experiment_, const std::string& actionDescription_, CelestialBody* targetBody_, const Waypoint& wp_,
                            FlightBand band_, bool contextual_, int bouncesLeft_, double bounceOriginLatitude_, double bounceOriginLongitude_);  // :92
    std::string TypeName() const override { return "SurveyWaypointParameter"; }
    std::string GetHashString() override;                     // :109
    std::string GetTitle() override;                          // :114
    std::string GetMessageComplete() override;                // :143
    void OnRegister() override;                               // :156
    void OnUnregister() override;                             // :165
    void OnSave(ConfigNode& node) override;                   // :183
    void OnLoad(ConfigNode& node) override;                   // :207
    void OnUpdate() override;                                 // :244
    void CheckExperimentResults(ScienceData* experimentData);  // :309
    void Bounce();                                            // :346
    static double TriggerRange(FlightBand band);              // :384
    void ProcessWaypoint();                                   // :408
    void CalculateAltitudes(double terrainAltitude);          // :482
    void VerifyWaypoint(double terrainAltitude);              // :550
    std::string SiteString();                                 // :573
    Waypoint* NextWaypointInCluster();                        // :593
    void UpdateNavWaypoint(Waypoint* newWaypoint);            // :613
};

}  // namespace ksp::Contracts::Parameters
