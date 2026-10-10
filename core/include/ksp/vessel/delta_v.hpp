// WP-17: the stock delta-v calculation of KSP 1.12.5 (VesselDeltaV.cs, DeltaVStageInfo.cs, DeltaVEngineInfo.cs, DeltaVPartInfo.cs,
// DeltaVEngineStageSet.cs, DeltaVPropellantInfo.cs, DeltaVEngineBurnTotals.cs, DeltaVEngineResourcePartInfo.cs, DeltaVCalc.cs,
// DeltaVGlobals.cs, DeltaVSituationOptions.cs and the four extension files; plus the data half of DeltaVAppValues.cs).
//
// What it is: a per-stage burn simulation. DeltaVPartInfo derives decouple / activation stages for every part, DeltaVStageInfo
// simulates the burn of the stage's engines in time steps (DeltaVGlobals / GameSettings.DELTAV_CALCULATIONS_TIMESTEP) against the
// SIMULATION copies of the part resources (Part.SimulationResources, drained through WP-15's PartSet flow with simulate = true),
// and VesselDeltaV drives the stages from the last to stage 0 and sums them.
//
// Porting notes:
//   * VesselDeltaV is a MonoBehaviour whose RunCalculations is a coroutine: the yields only wait frames, so RunCalculations() here
//     runs to the end (`simulation` is "running" while it does, which decides which engine / stage list instance the
//     DeltaVEngineStageSet properties return). The Unity driver (CheckDirtyAndRun / FixedUpdate: `frames` delay, the editor's
//     re-run while syncListInstances is set) is DriveToCompletion().
//   * The scene (HighLogic.LoadedSceneIsEditor / LoadedSceneIsFlight, FlightGlobals.GetHomeBody(), DeltaVGlobals.DeltaVAppValues) is
//     DeltaVGlobals' static state; the stage numbers come from StageManager (WP-16).
//   * The GameEvents subscriptions of VesselDeltaV.Start are not wired (the host calls SetCalcsDirty where the event would).
//   * Thrust directions come from ModuleEngines.OnCenterOfThrustQuery over `thrustTransforms` (the .mu model reader fills them; a
//     tool may set them directly).
//   * Float code follows the Mono rule (fx() / f32store()); runtime maths goes through ksp::dotnet::Math.
#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "ksp/math/mathfx.hpp"
#include "ksp/modules/module_engines.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/resources/flow_graph.hpp"
#include "ksp/resources/part_resource_list.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

class CelestialBody;
class ShipConstruct;
class Vessel;
class ModuleDecouple;
class ModuleAnchoredDecoupler;
class ModuleResourceIntake;
class IStageSeparator;
class VesselDeltaV;
class DeltaVStageInfo;
class DeltaVEngineInfo;
class DeltaVPartInfo;
class DeltaVEngineStageSet;

// DeltaVSituationOptions.cs
enum class DeltaVSituationOptions : int { SeaLevel, Altitude, Vaccum };

// DeltaVExtensions.cs:3 GetSwitchedValue<T>
template <class T>
T GetSwitchedValue(DeltaVSituationOptions situation, T asl, T actual, T vac) {
    switch (situation) {
        case DeltaVSituationOptions::SeaLevel: return asl;
        case DeltaVSituationOptions::Altitude: return actual;
        default: return vac;
    }
}

// GameSettings.cs:1080-1088 (the defaults of GameSettings.ApplyDefaultSettings)
struct DeltaVGameSettings {
    static inline bool DELTAV_CALCULATIONS_ENABLED = true;
    static inline bool DELTAV_USE_TIMED_VESSELCALCS = false;
    static inline float DELTAV_ACTIVE_STAGE_UPDATE_SECS = 0.2f;
    static inline float DELTAV_ALL_STAGES_UPDATE_SECS = 4.0f;
    static inline float DELTAV_VESSEL_EVENT_DELAY_SECS = 1.0f;
    static inline float DELTAV_ACTIVE_VESSEL_TIMESTEP = 5.0f;
    static inline float DELTAV_CALCULATIONS_TIMESTEP = 5.0f;
    static inline float DELTAV_CALCULATIONS_BIGTIMESTEP = 100.0f;
};

// DeltaVAppValues.cs: the data half (the info lines are the flight / editor UI, WP-43)
class DeltaVAppValues {
public:
    const CelestialBody* body = nullptr;                                 // :71
    DeltaVSituationOptions situation = DeltaVSituationOptions::Vaccum;  // :73
    double altitude = 0.0;                                               // :75
    DeltaVAppValues() = default;                                         // :86 (body = FlightGlobals.GetHomeBody())
    explicit DeltaVAppValues(const CelestialBody* homeBody) : body(homeBody) {}
    float atmPressure() const;   // :81 (float)(body.GetPressure(altitude) * 0.009869232667160128)
    double atmDensity() const;   // :83 body.GetDensity(body.GetPressure(altitude), body.GetTemperature(altitude))
};

// DeltaVGlobals.cs
class DeltaVGlobals {
public:
    static inline bool ready = false;                         // :6
    static inline std::unique_ptr<DeltaVAppValues> appValues;  // `fetch.deltaVAppValues` (null: no DeltaVGlobals in the scene)
    static DeltaVAppValues* DeltaVAppValuesPtr() { return appValues.get(); }  // :19 DeltaVAppValues
    static inline bool _doStockSimulations = true;            // :14
    static bool DoStockSimulations() { return _doStockSimulations; }  // :58
    static const std::vector<int>& PropellantsToIgnore();     // :60 (the hash codes of "IntakeAir" and "ElectricCharge")
    static void SetPropellantsToIgnore(std::vector<int> v);   // :73
    static void ResetPropellantsToIgnore();
    static bool EnableStockSimluations();                     // :129
    static bool DisableStockSimluations();                    // :140
    // HighLogic.LoadedSceneIsEditor / LoadedSceneIsFlight (SaveEnvironment::loadedScene) and FlightGlobals.GetHomeBody()
    static bool LoadedSceneIsEditor();
    static bool LoadedSceneIsFlight();
    static inline const CelestialBody* homeBody = nullptr;
    static bool PropellantIgnored(int id);
private:
    static inline std::vector<int> propellantsToIgnore_;
    static inline bool propellantsInit_ = false;
};

// DeltaVPropellantInfo.cs
class DeltaVPropellantInfo {
public:
    Propellant* propellant = nullptr;       // :6 (the engine's own Propellant record)
    double amountAvailable = 0.0;           // :8
    double amountBurnt = 0.0;               // :10
    double maxBurnTime = 0.0;               // :12
    double setThrottleBurnTime = 0.0;       // :14
    double currentBurnTime = 0.0;           // :16
    double amountPerSecondMaxThrottle = 0.0;     // :18
    double amountPerSecondSetThrottle = 0.0;     // :20
    double amountPerSecondCurrentThrottle = 0.0;  // :22
    double pendingDemand = 0.0;             // :24
    float timeLeftSetThrottle = 0.0f;       // :26
    float timeLeftCurrentThrottle = 0.0f;   // :28
    explicit DeltaVPropellantInfo(Propellant* p);   // :30
    DeltaVPropellantInfo Copy() const;              // :43
    float MassBurnt() const;                        // :57
};

// DeltaVEngineBurnTotals.cs
class DeltaVEngineBurnTotals {
public:
    double maxTotalBurnTime = 0.0;             // :5
    double setFuelFlowTotalBurnTime = 0.0;     // :7
    double currentFuelFlowTotalBurnTime = 0.0;  // :9
    std::vector<DeltaVPropellantInfo> propellantInfo;  // :11
};

// DeltaVEngineResourcePartInfo.cs
class DeltaVEngineResourcePartInfo {
public:
    std::shared_ptr<DeltaVPartInfo> resourcePart;  // :5
    std::vector<int> resourceIdsUsed;              // :7
    float fuelMassUsed = 0.0f;                     // :9
    explicit DeltaVEngineResourcePartInfo(std::shared_ptr<DeltaVPartInfo> partInfo) : resourcePart(std::move(partInfo)) {}  // :11
};

// DeltaVCalc.cs
class DeltaVCalc {
public:
    double dVinVac = 0.0, dVatASL = 0.0, dVActual = 0.0, time = 0.0;
    std::vector<std::shared_ptr<DeltaVEngineInfo>> activeEngines;
    double ispVAC = 0.0, ispASL = 0.0, ispActual = 0.0;
    float TWRVac = 0.0f, TWRASL = 0.0f, TWRActual = 0.0f, startMass = 0.0f, endMass = 0.0f;
    double thrustVac = 0.0, thrustASL = 0.0, thrustActual = 0.0;
};

using DeltaVEngineInfoPtr = std::shared_ptr<DeltaVEngineInfo>;
using DeltaVPartInfoPtr = std::shared_ptr<DeltaVPartInfo>;
using DeltaVStageInfoPtr = std::shared_ptr<DeltaVStageInfo>;

// DeltaVPartInfo.cs
class DeltaVPartInfo {
public:
    // :9 PartStageFuelMass
    struct PartStageFuelMass {
        float startMass = 0.0f;
        float endMass = 0.0f;
        PartResourceList resources;
        explicit PartStageFuelMass(Part& part) : resources(&part, &part.Resources, true) {}  // :17
    };
    VesselDeltaV* vesselDeltaV = nullptr;   // :27
    Part* part = nullptr;                   // :29
    int decoupleStage = 0;                  // :31
    bool decoupleBeforeBurn = false;        // :33
    int activationStage = 0;                // :35
    bool isDecoupler = false, isStageSeparator = false, isFairing = false, isDockingPort = false, isEngine = false;  // :37-45
    bool isJettison = false, isIntake = false, isSolarPanel = false, isGenerator = false;                            // :47-53
    ModuleResourceIntake* moduleResourceIntake = nullptr;    // :55
    PartModule* moduleDeployableSolarPanel = nullptr;       // :57
    PartModule* moduleGenerator = nullptr;                  // :59
    PartModule* moduleDockingNode = nullptr;                // :61
    ModuleDecouple* moduleDecoupler = nullptr;              // :63
    ModuleAnchoredDecoupler* moduleAnchoredDecoupler = nullptr;  // :65
    IStageSeparator* moduleStageSeparator = nullptr;        // :67
    std::vector<ModuleEngines*> engines;                    // :69
    float dryMass = 0.0f, fuelMass = 0.0f, jettisonMass = 0.0f;  // :71-75
    std::unordered_map<int, std::unique_ptr<PartStageFuelMass>> stageFuelMass;  // :77 DictionaryValueList<int, PartStageFuelMass>
    Part* jettisonPart = nullptr;                           // :79

    DeltaVPartInfo(Part* part, VesselDeltaV* vesselDeltaV);   // :76
    void StageStartFuelMass(int stage);                      // :176
    void StageEndFuelMass(int stage);                        // :191
    void CalculateMassValues();                              // :198
    void CalculateStagingValues();                           // :231
    void ResetStartSimulationResources(int stage);           // :305
    bool JettisonInStage(int stage) const;                   // :317
    float GetCurrentFuelMass() const;                        // :326
    float GetStageStartMass(int stage) const;                // :340
    float GetStageEndMass(int stage) const;                  // :349
private:
    int CheckDecoupler(int chkStage);                        // :99
    int GetSeparationStage(int chkStage) const;              // :159
    bool FindDecouplerParent(Part* inPart, Part*& decouplerParent) const;  // :290
};

// DeltaVEngineInfo.cs
class DeltaVEngineInfo {
public:
    VesselDeltaV* vesselDeltaV = nullptr;     // :8
    ModuleEngines* engine = nullptr;          // :12
    MultiModeEngine* multiModeEngine = nullptr;  // :14
    std::vector<DeltaVPropellantInfo> propellantInfo;  // :16
    double atmosphere = 0.0;                  // :18
    double ispVac = 0.0, ispASL = 0.0, ispActual = 0.0;  // :20-24
    Vector3f thrustVectorVac, thrustVectorASL, thrustVectorActual;  // :26-30
    float thrustVac = 0.0f, thrustASL = 0.0f, thrustActual = 0.0f;  // :32-36
    int startBurnStage = 0;                   // :38
    bool deprived = false;                    // :40
    float maxTimeStep = 0.0f;                 // :42
    std::unordered_map<int, DeltaVEngineBurnTotals> stageBurnTotals;  // :45 DictionaryValueList<int, DeltaVEngineBurnTotals>
    bool requiresAir = false;                 // :47
    bool throttleLimited = false;             // :49

    DeltaVEngineInfo(VesselDeltaV* inVesselDeltaV, ModuleEngines* inEngine, MultiModeEngine* inMultiModeEngine = nullptr);  // :68
    DeltaVPartInfo* partInfo();               // :56 (the cached DeltaVPartInfo of engine.part)
    double GetSituationISP(DeltaVSituationOptions s) const { return GetSwitchedValue(s, ispASL, ispActual, ispVac); }   // :105
    float GetSituationThrust(DeltaVSituationOptions s) const { return GetSwitchedValue(s, thrustASL, thrustActual, thrustVac); }  // :110
    Vector3f GetSituationThrustVector(DeltaVSituationOptions s) const {                                                          // :115
        return GetSwitchedValue(s, thrustVectorASL, thrustVectorActual, thrustVectorVac);
    }
    void Reset();                             // :120
    bool PropellantStarved();                 // :147
    void CalculateBurn(DeltaVStageInfo* deltaVStage, std::vector<DeltaVEngineInfoPtr>& enginesStillActive, float deltaTime, bool runningActive,
                       bool logMsgs, bool infiniteFuel, bool reCalc, bool& checkDeprived, float& minTimeStep);  // :167
    void ApplyBurn(DeltaVStageInfo* deltaVStage, float deltaTime, bool runningActive, bool logMsgs, bool infiniteFuel);  // :289
    void ResetCalcVariables();                // :347
    void SwitchEngine();                      // :354
    void CalculateFuelTime(int stage);        // :379
    void ResetPropellantBurnValues();         // :506
    void CalcThrustActual(bool engineIngitedCheck = false);  // :535
    float PropellantMassBurnt() const;        // :569
    double GetFuelTimeMaxThrottle(int stage) const;        // :579
    double GetFuelTimeAtThrottle(int stage) const;         // :588
    double GetFuelTimeAtActiveThrottle(int stage) const;   // :597
    void CalculateISP(bool engineIngitedCheck = false);    // :610
    double GetISPVac() { CalculateISP(); return ispVac; }       // :669
    double GetISPASL() { CalculateISP(); return ispASL; }       // :675
    double GetISPActual() { CalculateISP(); return ispActual; } // :681
    void CalculateThrustVector(bool engineIngitedCheck = false);  // :687
    Vector3f GetThrustVectorVac() { CalculateThrustVector(); return thrustVectorVac; }        // :714
    Vector3f GetThrustVectorASL() { CalculateThrustVector(); return thrustVectorASL; }        // :720
    Vector3f GetThrustVectorActual() { CalculateThrustVector(); return thrustVectorActual; }  // :726
    float GetThrustVac() { CalculateThrustVector(); return thrustVac; }        // :732
    float GetThrustASL() { CalculateThrustVector(); return thrustASL; }        // :738
    float GetThrustActual() { CalculateThrustVector(); return thrustActual; }  // :744
    bool RequiresAir();                       // :750
    bool ThrottleLimited();                   // :771
    double PropellantDemand(int id, bool runningActive);  // :782

private:
    DeltaVPartInfo* partInfo_ = nullptr;      // :52 _partInfo
    bool lastStep = false;                    // :51
    static constexpr double epsilon = 1E-05;  // :53
    void ResetResourceTotals();               // :514
    void ResetStageInfo();                    // :529
};

// DeltaVStageInfo.cs
class DeltaVStageInfo {
public:
    int stage = 0;                            // :12
    int separationIndex = 0;                  // :14
    VesselDeltaV* vesselDeltaV = nullptr;     // :17
    bool payloadStage = false;                // :19
    std::vector<DeltaVPartInfoPtr> parts;     // :21
    std::vector<ModuleResourceIntake*> airIntakeParts;  // :23
    std::vector<DeltaVEngineInfoPtr> enginesActiveInStage, enginesInStage;  // :25-27
    float stageMass = 0.0f, dryMass = 0.0f, fuelMass = 0.0f, startMass = 0.0f, endMass = 0.0f, decoupledMass = 0.0f;  // :29-39
    double ispVac = 0.0, ispASL = 0.0, ispActual = 0.0;                      // :41-45
    float TWRVac = 0.0f, TWRASL = 0.0f, TWRActual = 0.0f;                   // :47-51
    float thrustVac = 0.0f, thrustASL = 0.0f, thrustActual = 0.0f;          // :53-57
    float vectoredThrustVac = 0.0f, vectoredThrustASL = 0.0f, vectoredThrustActual = 0.0f;  // :59-63
    float deltaVinVac = 0.0f, deltaVatASL = 0.0f, deltaVActual = 0.0f;      // :65-69
    double stageBurnTime = 0.0;               // :71
    std::vector<DeltaVCalc> deltaVCalcs;      // :73
    float totalExhaustVelocityVAC = 0.0f, totalExhaustVelocityASL = 0.0f, totalExhaustVelocityActual = 0.0f;  // :75-79
    Vector3f vectoredExhaustVelocityVAC, vectoredExhaustVelocityASL, vectoredExhaustVelocityActual;          // :81-85

    DeltaVStageInfo(ShipConstruct* ship, int inStage, VesselDeltaV* vesselDeltaV);  // :110
    DeltaVStageInfo(Vessel* vessel, int inStage, VesselDeltaV* vesselDeltaV);       // :126
    double GetSituationISP(DeltaVSituationOptions s) const { return GetSwitchedValue(s, ispASL, ispActual, ispVac); }   // :142
    float GetSituationTWR(DeltaVSituationOptions s) const { return GetSwitchedValue(s, TWRASL, TWRActual, TWRVac); }    // :147
    float GetSituationThrust(DeltaVSituationOptions s) const { return GetSwitchedValue(s, thrustASL, thrustActual, thrustVac); }  // :152
    float GetSituationVectoredThrust(DeltaVSituationOptions s) const {                                                           // :157
        return GetSwitchedValue(s, vectoredThrustASL, vectoredThrustActual, vectoredThrustVac);
    }
    float GetSituationDeltaV(DeltaVSituationOptions s) const { return GetSwitchedValue(s, deltaVatASL, deltaVActual, deltaVinVac); }  // :162
    float GetSituationTotalExhaustVelocity(DeltaVSituationOptions s) const {                                                          // :167
        return GetSwitchedValue(s, totalExhaustVelocityASL, totalExhaustVelocityActual, totalExhaustVelocityVAC);
    }
    Vector3f GetSituationVectoredExhaustVelocity(DeltaVSituationOptions s) const {                                                    // :172
        return GetSwitchedValue(s, vectoredExhaustVelocityASL, vectoredExhaustVelocityActual, vectoredExhaustVelocityVAC);
    }
    void Reset(int inStage, VesselDeltaV* vesselDeltaV);   // :177
    void SimulateDeltaV(bool runningActive, bool infiniteFuel, float timeStep = 0.2f, bool logMsgs = true, bool thisStageActive = false);  // :531
    double CalculateTimeRequiredDV(bool runningActive, float deltaVRequested);  // :871
    void CalcLerpDeltaV();                                 // :895
    void ProcessActiveEngines();                           // :961
    void ResetPartCaches();                                // :983
    float GetStageMass(float& dryMass, float& fuelMass, float& decoupledDryMass, float& decoupledFuelMass, float& jettisonedDryMass,
                       int& decoupledPartCount);          // :989
    float GetCurrentStageMass();                           // :1017
    bool ContainsAnchoredDecoupler() const;                // :1033
    bool ContainsDecoupler() const;                        // :1052
    void CalculateISP(std::vector<DeltaVEngineInfoPtr>* engines = nullptr);  // :1099
    void CalculateTWR(float mass = -1.0f);                 // :1152
    int PartsActiveInStage() const;                        // :1175
    int PartsActivateInStage(std::vector<DeltaVPartInfoPtr>& activeParts);   // :1188
    int PartsDecoupledInStage() const;                     // :1212

private:
    Vector3f enginesThrustVac, enginesThrustASL, enginesThrustActual;   // :91-95
    std::vector<Part*> cachedResourcePartSetParts;         // :97 (null list: cachedResourcePartSetValid_ false)
    bool cachedResourcePartSetValid_ = false;
    std::vector<DeltaVPartInfoPtr> cachedActivatedParts;   // :99
    std::unique_ptr<RUI::Algorithms::SCCFlowGraph> cachedFlowGraph;  // :101
    std::unordered_map<uint32_t, DeltaVPartInfoPtr> partInfoDictionary;  // :103
    float removedFuelMass = 0.0f;                          // :105
    bool stageContainsOnlyLaunchClamps = false;            // :108
    void ProcessParts(int inStage);                        // :230
    void CalculateStartMass();                             // :318
    void StoreInterimDeltaV(std::vector<DeltaVEngineInfoPtr>& engines, std::vector<DeltaVCalc>& deltaVCalcsList, float startingMass,
                            float currentMass, double simulationTime, bool logMsgs);   // :331
    int GetHighestSeparationIndex(std::vector<DeltaVEngineInfoPtr>& engines);          // :354
    std::vector<DeltaVEngineInfoPtr> MatchingSeparationIndex(std::vector<DeltaVEngineInfoPtr>& engines, int index);  // :364
    std::vector<DeltaVEngineResourcePartInfo> GetEngineResourceParts(std::vector<DeltaVEngineInfoPtr>& engines);    // :381
    void CalculateEngineResourceFuelMass(std::vector<DeltaVEngineResourcePartInfo>& resourceParts);                // :420
    bool EnginesDeprived(std::vector<DeltaVEngineInfoPtr>& enginesToStageNext, bool allEngines);                   // :436
    void StoreStartFuelMass();                             // :468
    void StoreEndFuelMass();                               // :476
    float CheckTimeStep(float timeStep, float simulationBurnTime, float fuelMassRemoved, std::vector<DeltaVEngineInfoPtr>& enginesStillActive,
                        std::vector<DeltaVEngineResourcePartInfo>& resourcePartsToStageNext, bool logMsgs);  // :484
};

// DeltaVEngineStageSet.cs
class DeltaVEngineStageSet {
public:
    // :9 DeltaVStageInfoEntry
    struct DeltaVStageInfoEntry {
        DeltaVStageInfoPtr stageInfo_InstanceOne;
        DeltaVStageInfoPtr stageInfo_InstanceTwo;
        VesselDeltaV* vesselDeltaV = nullptr;
        explicit DeltaVStageInfoEntry(VesselDeltaV* v) : vesselDeltaV(v) {}
    };
    std::vector<DeltaVEngineInfoPtr> engineInfo_InstanceOne, engineInfo_InstanceTwo;  // :54-58
    int operatingIndex = 0, workingIndex = 0, flightSceneOperatingIndex = 0;          // :60-64
    VesselDeltaV* vesselDeltaV = nullptr;                                             // :66
    std::vector<int> payloadStages;                                                   // :68

    explicit DeltaVEngineStageSet(VesselDeltaV* vesselDeltaV);                        // :205
    std::vector<DeltaVStageInfoPtr> WorkingStageInfo();                               // :72
    std::vector<DeltaVStageInfoPtr> OperatingStageInfo();                             // :104
    std::vector<DeltaVEngineInfoPtr>& WorkingEngineInfo();                            // :171
    std::vector<DeltaVEngineInfoPtr>& OperatingEngineInfo();                          // :195
    void UpdateStageInfo(int stage);                                                  // :243
    void UpdateStageInfo();                                                           // :344
    void RemoveStagedStages(bool sortStages = false);                                 // :405
    void AddEngineWorkingSet(DeltaVEngineInfoPtr engineInfo);                         // :450
    void RemoveInvalidEnginesWorkingSet();                                            // :470

private:
    std::vector<DeltaVStageInfoEntry> stageInfoEntries;                               // :48
    void AddStageInfo(DeltaVStageInfoPtr stageInfo);                                  // :217
    void RemoveStaleStages();                                                         // :359
    void SetPayload(DeltaVStageInfo* stageInfo);                                      // :390
    void SortStages();                                                                // :444
};

// DeltaVEngineExtensions.cs / DeltaVPartExtensions.cs / DeltaVEngineResourcePartExtensions.cs
namespace deltav_ext {
bool Contains(const std::vector<DeltaVEngineInfoPtr>& list, const Part* part);                 // DeltaVEngineExtensions.cs:5
DeltaVEngineInfo* Get(const std::vector<DeltaVEngineInfoPtr>& list, const Part* part);         // DeltaVEngineExtensions.cs:20
bool ContainsPart(const std::vector<DeltaVPartInfoPtr>& list, const Part* part);               // DeltaVPartExtensions.cs:5
DeltaVPartInfo* Get(const std::vector<DeltaVPartInfoPtr>& list, const Part* part);             // DeltaVPartExtensions.cs:20
std::vector<Part*> PartsInStage(const std::vector<DeltaVPartInfoPtr>& list, int stage);        // DeltaVPartExtensions.cs:37
bool ContainsPartInfo(const std::vector<DeltaVEngineResourcePartInfo>& list, const DeltaVPartInfo& partInfo);   // DeltaVEngineResourcePartExtensions.cs:6
DeltaVEngineResourcePartInfo* Get(std::vector<DeltaVEngineResourcePartInfo>& list, const DeltaVPartInfo& partInfo);  // :20
int GetHighestPartSeparationIndex(const std::vector<DeltaVEngineResourcePartInfo>& list, int stage);          // :46
std::vector<DeltaVEngineResourcePartInfo> PartsMatchingSeparationIndex(const std::vector<DeltaVEngineResourcePartInfo>& input, int index);  // :58
}  // namespace deltav_ext

// VesselDeltaV.cs
class VesselDeltaV {
public:
    enum class Mode { Ship, Vessel };   // :18

    std::unique_ptr<DeltaVEngineStageSet> engineStageSet;   // :25
    bool currentStageActivated = false;                     // :52
    int lowestStageWithDeltaV = 0;                          // :54

    // :154 / :170 Create
    static std::unique_ptr<VesselDeltaV> Create(Vessel* vesselRef);
    static std::unique_ptr<VesselDeltaV> Create(ShipConstruct* shipRef);
    ~VesselDeltaV();

    // :187 Start (the data part; the event subscriptions are the host's)
    void Start();
    // The Unity driver of :234-282: runs RunCalculations() until nothing is dirty (the editor re-runs while syncListInstances is set)
    void DriveToCompletion();
    void RunCalculations();                                  // :853 (the coroutine, run to the end)

    std::vector<DeltaVEngineInfoPtr>& WorkingEngineInfo() { return engineStageSet->WorkingEngineInfo(); }  // :87
    std::vector<DeltaVEngineInfoPtr>& OperatingEngineInfo() { return engineStageSet->OperatingEngineInfo(); }  // :89
    Vessel* GetVessel() const { return _vessel; }            // :91
    ShipConstruct* Ship() const { return _ship; }            // :93
    std::vector<DeltaVStageInfoPtr> WorkingStageInfo() { return engineStageSet->WorkingStageInfo(); }     // :97
    std::vector<DeltaVStageInfoPtr> OperatingStageInfo() { return engineStageSet->OperatingStageInfo(); }  // :101
    const std::vector<int>& SeparationStageIndexes() const { return _separationStageIndexes; }           // :103
    std::vector<DeltaVPartInfoPtr>& PartInfo() { return _partInfo; }                                      // :105
    double TotalDeltaVVac() const { return _totalDeltaVVac; }       // :107
    double TotalDeltaVASL() const { return _totalDeltaVASL; }       // :109
    double TotalDeltaVActual() const { return _totalDeltaVActual; }  // :111
    double TotalBurnTime() const { return _totalBurnTime; }          // :113
    Mode ActiveMode() const { return _activeMode; }                  // :115
    bool SimulationRunning() const { return simulationRunning_ || updateFlightScene_; }  // :117 (simulation != null || updateFlightScene)
    bool IsReady() const { return _isReady; }                        // :127
    bool DoStockSimulation() const { return _doStockSimulation; }    // :129
    bool UpdateFlightScene() const { return updateFlightScene_; }    // :131
    bool EnableStockSimluation();                                    // :132
    bool DisableStockSimluation();                                   // :143
    double GetSituationTotalDeltaV(DeltaVSituationOptions situation) const {   // :149
        return GetSwitchedValue(situation, TotalDeltaVASL(), TotalDeltaVActual(), TotalDeltaVVac());
    }
    void SetCalcsDirty(bool resetPartCaches, bool syncListInstances = false);  // :471
    DeltaVStageInfo* GetStage(int stage);                            // :1201
    int GetActivatedEngines();                                       // :1221
    int GetHighestSeparationStage(int inStage) const;                // :1235
    void ResetSeparationIndexes();                                   // :1259
    void UpdateModuleEngines();                                      // :1085
    bool CalcsDirty() const { return calcsDirty_; }
    bool SyncListInstances() const { return syncListInstances_; }
    int RunCount() const { return runCount_; }
    int CompletedEvents() const { return completedEvents_; }          // GameEvents.onDeltaVCalcsCompleted fires
    // :270 `simulation = StartCoroutine(...)` is set while a run (and its frame delay) is pending
    void SimulateFlightScene();                                      // :943
    void SimulateLastStage(bool recalcVesselTotals = true);          // :982

private:
    enum class PartStageComparisonOperator { LessThan, NotEqual, Equal };  // :10
    VesselDeltaV() = default;
    Vessel* _vessel = nullptr;                          // :27
    ShipConstruct* _ship = nullptr;                     // :30
    bool syncListInstances_ = true;                     // :33
    bool useMultipleInfoLists = true;                   // :36
    std::vector<int> _separationStageIndexes;           // :39
    std::vector<DeltaVPartInfoPtr> _partInfo;           // :42
    double _totalDeltaVVac = 0.0, _totalDeltaVASL = 0.0, _totalDeltaVActual = 0.0, _totalBurnTime = 0.0;  // :45-54
    Mode _activeMode = Mode::Ship;                      // :57
    bool simulationRunning_ = false;                    // `simulation != null`
    bool _isReady = false;                              // :62
    bool _doStockSimulation = true;                     // :64
    bool updateFlightScene_ = false;                    // :66
    bool calcsDirty_ = false;                           // :73
    bool resetPartCaches_ = false;                      // :75
    int runCount_ = 0;
    int completedEvents_ = 0;

    void CalculateSeparationStageIndexes(const std::vector<Part*>& parts);   // :555
    void UpdateStageInfo();                                                  // :573
    void ProcessPayloadStages();                                             // :589
    void ResetPartInfo(const std::vector<Part*>& parts);                     // :624
    void ProcessEnginePartInfoChildren(DeltaVPartInfo* partInfoItem);        // :767
    void ProcessDecouplePartInfoChildren(DeltaVPartInfo* partInfoItem, PartStageComparisonOperator comparisonOp, int decoupleStage,
                                         int activationStage, bool bypassFairings, bool bypassJettison, bool setToParentDecoupleStage);  // :788
    void changePartInfoDecoupleStage(DeltaVPartInfo* partInfoItem, DeltaVPartInfo* childPart, bool setToParentDecoupleStage = false);  // :826
    void ReCalculateVesselTotals();                                          // :1012
    double stageDVActual(DeltaVStageInfo* stageInfo);                        // :1040
    void ResetSimulationResources();                                         // :1053
    void ResetStagePartCaches();                                             // :1071
    void ProcessActiveUpdateEngines(DeltaVStageInfo* currentDeltaVStage);    // :1163
    DeltaVStageInfo* GetWorkingStage(int stage);                             // :1181
    const std::vector<Part*>& PartsOfActive() const;
};

}  // namespace ksp
