// WP-53: the deployed-science runtime: Expansions/Serenity/DeployedScience/Runtime/{DeployedScience, DeployedScienceCluster,
// DeployedSciencePart, DeployedScienceExperiment, DeployedScienceExtensions}.cs.
//
// The C# classes are Unity components on GameObjects (DeployedScienceCluster / Part / Experiment under the DeployedScienceGameObject).
// kspcore models a component as a `SerenityBehaviour` registered in a process-wide registry:
//   * AddComponent    -> `Spawn(...)` returns a raw pointer; the registry owns the object (shared_ptr),
//   * Start()         -> runs the frame after creation: `SerenityBehaviour::RunPendingStarts()` (DeployedScience::FixedUpdate calls it),
//   * Object.Destroy  -> `DestroyGameObject()` marks the object destroyed and queues it; `SerenityBehaviour::FlushDestroyed()` is the end of
//                        the frame: it calls OnDestroy and releases the object. Until then the object still exists and still receives
//                        GameEvents, exactly like a destroyed MonoBehaviour whose C# object is alive,
//   * "== null"       -> `destroyed` (Unity's overloaded null check) for objects, nullptr for pointers.
// Coroutines (StartCoroutine) run on ScienceEnvironment::Scheduler() (WP-00 model).
#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/core/coroutine.hpp"
#include "ksp/core/delegate.hpp"
#include "ksp/curves/float_curve.hpp"
#include "ksp/save/game.hpp"
#include "ksp/science/science.hpp"
#include "ksp/serenity/ground_modules.hpp"
#include "ksp/serenity/serenity_env.hpp"
#include "ksp/util/collections.hpp"

namespace ksp {
class CelestialBody;
class Vessel;
}

namespace ksp::Expansions::Serenity::DeployedScience::Runtime {

class DeployedScience;
class DeployedScienceCluster;
class DeployedSciencePart;
class DeployedScienceExperiment;

// The MonoBehaviour stand-in (see the file comment).
class SerenityBehaviour {
public:
    virtual ~SerenityBehaviour() = default;
    bool destroyed = false;  // Object.Destroy called (Unity == null)
    bool started = false;    // Start() has run

    virtual void Start() {}
    virtual void OnDestroy() {}
    void DestroyGameObject();  // GameObjectExtension.DestroyGameObject -> Object.Destroy (deferred)

    // Component creation: the registry takes the object and returns it.
    template <class T>
    static T* Add(std::shared_ptr<T> obj) {
        T* raw = obj.get();
        live().push_back(std::move(obj));
        return raw;
    }
    static void RunPendingStarts();   // Start() of every live behaviour that has not started (creation order)
    static void FlushDestroyed();     // end of frame: OnDestroy + release of every destroyed behaviour (children destroyed in OnDestroy too)
    static void DestroyAll();         // scene teardown / test reset: OnDestroy of everything
    static size_t LiveCount() { return live().size(); }

private:
    static std::vector<std::shared_ptr<SerenityBehaviour>>& live();
    static inline bool running_ = false;
};

// ---------------------------------------------------------------------------------------------------------------------------------
// DeployedScienceExperiment.cs:11
class DeployedScienceExperiment : public SerenityBehaviour {
public:
    // :14
    struct SeismicPartData {
        float velocity = 0.0f;
        uint32_t partId = 0;
        float mass = 0.0f;
        float resourceMass = 0.0f;
        double impactDistance = 0.0;
        float impactDistanceMultiplier = 0.0f;
    };
    // :29 delegate float SeismicDataCalc(DeployedScienceExperiment, float impactJoules, float impactJoulesMaxValue, List<SeismicPartData>)
    using SeismicDataCalc = ksp::Delegate<float(DeployedScienceExperiment*, float, float, std::vector<std::shared_ptr<SeismicPartData>>&)>;

    // ---- serialized state (:31-:112) ----
    float totalScienceGenerated = 0.0f;
    float scienceLimit = 0.0f;
    float scienceValue = 0.0f;
    float storedScienceData = 0.0f;
    float transmittedScienceData = 0.0f;
    uint32_t partId = 0;
    std::string experimentId, experimentName;
    float scienceModifierRate = 0.0f;
    float scienceDiminishingModifierRate = 0.0f;
    ScienceExperiment* Experiment = nullptr;  // :61
    Vessel* ExperimentVessel = nullptr;       // :63
    DeployedScienceCluster* Cluster = nullptr;  // :65
    DeployedSciencePart* sciencePart = nullptr;  // :67
    double LastScienceGeneratedUT = 0.0;      // :69
    bool experimentSituationValid = false;
    ModuleGroundExperiment* moduleGroundExperiment = nullptr;  // :75
    float xmitDataScalar = 1.0f;              // :77
    std::shared_ptr<ScienceSubject> subject;  // :80
    Guid vesselID{};                          // :83 Guid.Empty
    Vessel* ControllerVessel = nullptr;       // :86
    float solarMultiplier = 0.0f;
    bool seismicEventDataSendPending = false;
    bool noCommNetMsgPosted = false;
    bool noPowerMsgPosted = false;
    float seismicEventVelocity = 0.0f;
    std::vector<std::shared_ptr<SeismicPartData>> seismicPartList;  // :102 (SeismicPartData is a class: identity matters; null until first use: `seismicPartListValid`)
    bool seismicPartListValid = false;
    FloatCurve distanceCurve;                 // :104
    double distanceMultiplier = 1.0;          // :106
    double totalJoules = 0.0;                 // :108
    float impactRate = 0.0f;                  // :110
    SeismicDataCalc seismicDataCalcOverride;  // :112

    // properties (:114-:165)
    float TotalScienceGenerated() const { return totalScienceGenerated; }
    float ScienceLimit() const { return scienceLimit; }
    float ScienceValue() const { return scienceValue; }
    float StoredScienceData() const { return storedScienceData; }
    float TransmittedScienceData() const { return transmittedScienceData; }
    uint32_t PartId() const { return partId; }
    const std::string& ExperimentId() const { return experimentId; }
    const std::string& ExperimentName() const { return experimentName; }
    float ScienceModifierRate() const { return scienceModifierRate; }
    float ScienceDiminishingModifierRate() const { return scienceDiminishingModifierRate; }
    void SetScienceDiminishingModifierRate(float v) { scienceDiminishingModifierRate = v; }  // internal set
    bool ExperimentSituationValid() const { return experimentSituationValid; }
    ModuleGroundExperiment* GroundExperimentModule();  // :146
    bool SeismicEventDataSendPending() const { return seismicEventDataSendPending; }

    static DeployedScienceExperiment* Spawn();                                                          // :167 private
    static DeployedScienceExperiment* Spawn(ModuleGroundExperiment* moduleGroundExperiment, DeployedScienceCluster* cluster);  // :180
    static DeployedScienceExperiment* SpawnandLoad(const ConfigNode& node);                            // :202
    ~DeployedScienceExperiment() override;
    void Awake();                                                                                       // :210
    void Start() override;                                                                              // :218
    void FixedUpdate();                                                                                 // :274
    void OnDestroy() override;                                                                          // :282
    void SeismicDataCalcOverrideAdd(const SeismicDataCalc& method);                                     // :291
    void SeismicDataCalcOverrideRemove(const SeismicDataCalc& method);                                  // :303
    void GroundScienceClusterPowerStateChanged(DeployedScienceCluster* cluster);                        // :315
    void GroundSciencePartEnabledStateChanged(ModuleGroundSciencePart* sciencePart);                    // :336
    Enumerator CalcScience();                                                                           // :356
    void UpdateScienceLimitExperiment(ModuleGroundExperiment* moduleGroundExperiment);                  // :370
    void UpdateScienceValueExperiment(ModuleGroundExperiment* moduleGroundExperiment);                  // :379
    void UpdateScienceExperiment(ModuleGroundExperiment* moduleGroundExperiment);                       // :388
    void CalculateScience(float solarMultiplier);                                                       // :399
    void CalcSendSeismicScience(Vessel* vessel, Part* part, float solarMultiplier);                     // :431
    void processImpactVesselPart(Vessel* vessel, Part* part);                                           // :534 private
    bool SeismicListContains(uint32_t partId);                                                          // :558 private
    void ProcessSeismicEvent();                                                                         // :577 private
    bool TimeToSendStoredData();                                                                        // :638
    bool GatherScienceData(std::shared_ptr<ScienceData>& experimentData);                               // :659
    bool SendDataToComms();                                                                             // :675
    void SetControllerVessel();                                                                         // :762 private
    void Load(const ConfigNode& node);                                                                  // :777
    void Save(ConfigNode& node);                                                                        // :802

    int CoroutineOwner() const { return static_cast<int>(reinterpret_cast<uintptr_t>(this) >> 4); }
    std::shared_ptr<Coroutine> StartCoroutine(Enumerator e);
    void StopAllCoroutines();

private:
    DeployedScienceExperiment() = default;
    bool subscribed_ = false;
};

// ---------------------------------------------------------------------------------------------------------------------------------
// DeployedSciencePart.cs:5
class DeployedSciencePart : public SerenityBehaviour {
public:
    uint32_t partId = 0;
    bool deployedOnGround = false;
    bool moduleEnabled = false;
    int powerUnitsProduced = 0;
    int actualPowerUnitsProduced = 0;
    int powerUnitsRequired = 0;
    bool isSolarPanel = false;
    bool isAntenna = false;
    double antennaBoosterPower = 0.0;
    DeployedScienceExperiment* Experiment = nullptr;  // :34
    uint32_t controllerId = 0;
    DeployedScienceCluster* Cluster = nullptr;        // :39
    ModuleGroundSciencePart* loadedPartModule = nullptr;  // :41

    uint32_t PartId() const { return partId; }
    bool DeployedOnGround() const { return deployedOnGround; }
    bool Enabled();                                    // :47 getter
    void SetEnabled(bool value);                       // (C++ addition) setter
    int PowerUnitsProduced() const { return powerUnitsProduced; }
    int ActualPowerUnitsProduced() const { return actualPowerUnitsProduced; }
    int PowerUnitsRequired() const { return powerUnitsRequired; }
    bool IsSolarPanel() const { return isSolarPanel; }
    bool IsAntenna() const { return isAntenna; }
    double AntennaBoosterPower() const { return antennaBoosterPower; }
    uint32_t ControllerId() const { return controllerId; }

    static DeployedSciencePart* Spawn();                                                                            // :89
    static DeployedSciencePart* Spawn(ModuleGroundSciencePart* part, ModuleGroundExpControl* controlUnit, DeployedScienceCluster* cluster);  // :96
    static DeployedSciencePart* SpawnandLoad(const ConfigNode& node, DeployedScienceCluster* cluster);              // :117
    ~DeployedSciencePart() override;
    void Awake();                                                                                                   // :137
    void Start() override;                                                                                          // :145
    void OnDestroy() override;                                                                                      // :152
    void OnGroundSciencePartRemoved(ModuleGroundSciencePart* sciencePartRemoved);                                   // :163
    void UpdateSciencePart(ModuleGroundSciencePart* part);                                                          // :171
    Part* PartIsLoaded();                                                                                           // :222
    void Load(const ConfigNode& node);                                                                              // :233
    void Save(ConfigNode& node);                                                                                    // :251

private:
    DeployedSciencePart() = default;
    bool subscribed_ = false;
};

// ---------------------------------------------------------------------------------------------------------------------------------
// DeployedScienceExtensions.cs:5 (extension methods of List<DeployedSciencePart> / List<ModuleGroundSciencePart>)
namespace DeployedScienceExtensions {
DeployedSciencePart* Get(const std::vector<DeployedSciencePart*>& list, uint32_t persistentId);                  // :7
DeployedSciencePart* GetExperiment(const std::vector<DeployedSciencePart*>& list, const std::string& experimentId);  // :26
std::vector<std::string> GetExperiments(const std::vector<DeployedSciencePart*>& list);                          // :45
ModuleGroundSciencePart* Get(const std::vector<ModuleGroundSciencePart*>& list, uint32_t persistentId);         // :58
DeployedScienceExperiment* GetExperimentByPartID(const std::vector<DeployedSciencePart*>& list, uint32_t persistentId);  // :77
}  // namespace DeployedScienceExtensions

// ---------------------------------------------------------------------------------------------------------------------------------
// DeployedScienceCluster.cs:7
class DeployedScienceCluster : public SerenityBehaviour {
public:
    bool isPowered = false;
    CelestialBody* DeployedBody = nullptr;  // :12 public
    bool partialPower = false;
    float partialPowerMultiplier = 1.0f;
    std::vector<DeployedSciencePart*> DeployedScienceParts;  // :20 public
    int powerRequired = 0;
    int powerAvailable = 0;
    std::vector<DeployedSciencePart*> solarPanelParts;  // :29 internal
    std::vector<DeployedSciencePart*> antennaParts;     // :32 internal
    uint32_t controlModulePartId = 0;
    bool controllerPartEnabled = false;
    double lastScienceGeneratedUT = 0.0;
    double lastScienceTransmittedUT = 0.0;
    bool updatingScience = false;
    bool dataSendFailed = false;

    bool IsPowered() const { return isPowered; }
    bool PartialPower() const { return partialPower; }
    float PartialPowerMultiplier() const { return partialPowerMultiplier; }
    int PowerRequired() const { return powerRequired; }
    int PowerAvailable() const { return powerAvailable; }
    std::vector<DeployedSciencePart*>& SolarPanelParts() { return solarPanelParts; }
    bool HasSolarPanels() const { return !solarPanelParts.empty(); }
    std::vector<DeployedSciencePart*>& AntennaParts() { return antennaParts; }
    uint32_t ControlModulePartId() const { return controlModulePartId; }
    bool ControllerPartEnabled() const { return controllerPartEnabled; }
    double LastScienceGeneratedUT() const { return lastScienceGeneratedUT; }
    double LastScienceTransmittedUT() const { return lastScienceTransmittedUT; }
    bool UpdatingScience() const { return updatingScience; }
    void SetUpdatingScience(bool v) { updatingScience = v; }
    bool DataSendFailed() const { return dataSendFailed; }

    static DeployedScienceCluster* Spawn();                                    // :100 private
    static DeployedScienceCluster* Spawn(ModuleGroundExpControl* controlUnit); // :113
    static DeployedScienceCluster* SpawnandLoad(const ConfigNode& node);       // :143
    ~DeployedScienceCluster() override;
    void Awake();                                                              // :152
    void Start() override;                                                     // :160
    void OnDestroy() override;                                                 // :167
    int SolarPanelUnitsProduced();                                             // :178
    void UpdatePowerState();                                                   // :188
    void UpdateCluster(ModuleGroundExpControl* controlUnit, bool replaceList, std::vector<ModuleGroundSciencePart*>* scienceParts);  // :263
    void UpdateExperimentDiminishReturns();                                    // :342
    void GroundSciencePartEnabledStateChanged(ModuleGroundSciencePart* sciencePart);  // :353 private
    void GroundScienceModuleRemoved(ModuleGroundSciencePart* part);            // :370 private
    Enumerator UpdateScience();                                                // :398
    void Load(const ConfigNode& node);                                         // :435
    void Save(ConfigNode& node);                                               // :467

private:
    DeployedScienceCluster() = default;
    bool subscribed_ = false;
};

// ---------------------------------------------------------------------------------------------------------------------------------
// DeployedScience.cs:14 (KSPScenario creation options 3198, scenes FLIGHT / TRACKSTATION / SPACECENTER / EDITOR)
class DeployedScience : public ksp::ScenarioModule {
public:
    DeployedScience();
    ~DeployedScience() override;
    static inline DeployedScience* Instance = nullptr;  // :34
    static inline bool IsActive = false;                // :36
    static inline std::string SeismicExperimentId = "deployedSeismicSensor";  // :22

    DictionaryValueList<uint32_t, DeployedScienceCluster*> deployedScienceClusters;  // :16
    DictionaryValueList<int32_t, float> diminishingReturns;                           // :20
    DictionaryValueList<std::string, float> seismicEnergyRates;                       // :24
    float MinimumSeismicEnergyRequired = 3000.0f;                                     // :26
    int SeismicScienceProcessingDelay = 3;                                            // :28
    double ScienceTimeDelay = 60.0;                                                   // :30
    double DataSendFailedTimeDelay = 600.0;                                           // :32

    DictionaryValueList<uint32_t, DeployedScienceCluster*>& DeployedScienceClusters() { return deployedScienceClusters; }  // :38
    DictionaryValueList<int32_t, float>& DiminishingReturns() { return diminishingReturns; }  // :53
    DictionaryValueList<std::string, float>& SeismicEnergyRates() { return seismicEnergyRates; }  // :55

    void OnAwake() override;                            // :57
    void FixedUpdate();                                 // :108 (Unity message; Starts of new components run first)
    void UpdateSciencePart(ModuleGroundSciencePart* sciencePart);  // :141 private
    void UpdateCluster(ModuleGroundExpControl* controlUnit, bool replaceList, std::vector<ModuleGroundSciencePart*>* deployableScienceParts);  // :150
    void OnVesselRecovered(ProtoVessel* vessel, bool quick);       // :158
    void OnVesselTerminated(ProtoVessel* vessel);                  // :169
    void OnVesselWillDestroy(Vessel* vessel);                      // :174
    void OnPartExplodeGroundCollision(Part* part);                 // :185
    void OnVesselExplodeGroundCollision(Vessel* vessel);           // :201
    void ScanAndProcessSeismicEvent(Vessel* vessel, Part* part);   // :213
    void OnGameStateLoad(ConfigNode* node);                        // :239
    float ExpDimReturnsRate(const std::string& experimentId, const std::string& celestialBodyName);   // :277
    float GetDiminishingReturnsMultiplier(int numExperiments);     // :299
    DeployedSciencePart* ExpMaxScienceMultiplier(const std::string& experimentId, const std::string& celestialBodyName);  // :321
    void SetDiminishingScienceRate(const std::string& experimentId, const std::string& celestialBodyName);  // :345
    void RegisterCluster(ModuleGroundExpControl* controlUnit, std::vector<ModuleGroundSciencePart*>* deployableScienceParts);  // :378
    void DeRegisterCluster(uint32_t controlUnitId);                // :397
    void SetDeployedSciencePartVesselType(Vessel* vessel);         // :437 private
    void OnLoad(ConfigNode& node) override;                        // :467
    void SetupDiminishingReturns(const ConfigNode* node);          // :529 private
    void SetupSeismicEnergy(const ConfigNode* node);               // :561 private
    void OnSave(ConfigNode& node) override;                        // :605

    // Unity DeployedScienceGameObject children (RemoveMannedScienceObjects): the live clusters
    static void Register();                                        // ScenarioModuleFactory ("DeployedScience")
    // ExpansionsLoader-gated Destroy(this)/Destroy(Instance): set by OnAwake when Serenity is not installed
    bool destroyed = false;

private:
    void RemoveMannedScienceObjects();                             // :451
    bool subscribed_ = false;
};

}  // namespace ksp::Expansions::Serenity::DeployedScience::Runtime
