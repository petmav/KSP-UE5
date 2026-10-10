// WP-39: ModuleDataTransmitter.cs (antennas: CommNet range / power, and the science transmission bookkeeping).
//
// Ported: the [KSPField] schema (module_members.inc), OnAwake (range / science DoubleCurves, consumedResources), OnLoad (DeployFxModules /
// ProgressFxModules indices, CommsData queue, requiredResource fallback), UpdatePowerText, OnSave, OnStart (FX module lookup, the PAW state of
// the events / actions, a restored queue restarts the transmit coroutine), GetVesselSignalStrength, EvaluateScienceMultiplier, StartTransmission
// (the checks and the CommNet CanScienceTo test), onStartTransmission, transmitQueuedData (the coroutine: packets = CeilToInt(data / packetSize),
// capacitor charge against packetResourceCost, EC drawn through resHandler, abort / hold, WaitForFixedSeconds, FX module animation), the FX helper
// coroutines, TransmitIncompleteToggle, StopTransmission, AbortTransmission, ReturnDataToContainer, CanTransmit, IsBusy, TransmitData,
// ICommAntenna (CanScienceTo, CanComm, CanCommUnloaded, CommPowerUnloaded with the UPGRADESAPPLIED node, the adjusters), DataRate,
// DataResourceCost, GetInfo.
//
// Seams (TransmitterEnvironment): the coroutine scheduler (StartCoroutine); ScreenMessages (recorded); the science side of WP-35 (the vessel's
// IScienceDataContainer list and the transmit dialog -> `collectVesselData`; RnDCommsStream -> `createCommsStream`; GameEvents.OnTriggeredDataTransmission is fired
// with a null ScienceData*); the part-upgrade pipeline's ApplyUpgradeNode for CommPowerUnloaded (`applyUpgrades`).
#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ksp/commnet/comm_network.hpp"
#include "ksp/commnet/science_transmitter.hpp"
#include "ksp/core/coroutine.hpp"
#include "ksp/modules/module_commnet_parts.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/resources/resource_types.hpp"

namespace ksp {

class Vessel;
class ModuleDataTransmitter;

struct TransmitterEnvironment {
    // MonoBehaviour.StartCoroutine host. Unset: StartCoroutine logs an error and drops the routine.
    static inline CoroutineScheduler* scheduler = nullptr;
    // ScreenMessages.PostScreenMessage texts, in order.
    static inline std::vector<std::string> screenMessages;
    // StartTransmission's part after the CommNet checks: vessel.FindPartModulesImplementing<IScienceDataContainer>() -> (optional popup choice)
    // -> queueVesselData (GetData + DumpData). Returns the queued data, or nullopt when the dialog was cancelled. Unset: an empty queue.
    static inline std::function<std::optional<std::vector<CommNet::TransmissionItem>>(ModuleDataTransmitter&)> collectVesselData;
    // ModuleDataTransmitter.ReturnDataToContainer: the container whose part flightID == data.container takes the data back. Unset: nothing.
    static inline std::function<void(ModuleDataTransmitter&, const CommNet::TransmissionItem&)> returnData;
    // new RnDCommsStream(subject, totalData, interval, baseTransmitValue * transmitBonus, scienceValueRatio, xmitIncomplete, R&D) when
    // ResearchAndDevelopment.Instance != null. Unset: no R&D (null stream).
    static inline std::function<std::unique_ptr<CommNet::ICommsStream>(const CommNet::TransmissionItem&, float totalData, float interval, bool xmitIncomplete)>
        createCommsStream;
    // GameEvents.OnTriggeredDataTransmission.Fire(data, vessel, aborted). Unset: nothing.
    static inline std::function<void(const CommNet::TransmissionItem&, Vessel*, bool)> onTriggeredDataTransmission;
    // part-upgrade ApplyUpgradeNode over the snapshot's UPGRADESAPPLIED list (PartModule.LoadUpgradesApplied + ApplyUpgradeNode(.., doLoad: false)):
    // fills `node` with the upgraded values (antennaPower). Unset: the snapshot's upgrades change nothing.
    static inline std::function<void(ModuleDataTransmitter&, ProtoPartModuleSnapshot&, ConfigNode& node)> applyUpgrades;
    static void Reset();
};

// ModuleDataTransmitter.cs:9
class ModuleDataTransmitter : public PartModule,
                              public CommNet::IScienceDataTransmitter,
                              public IResourceConsumer,
                              public CommNet::ICommAntenna,
                              public IModuleInfo {
public:
    explicit ModuleDataTransmitter(const ModuleClassInfo& cls) : PartModule(cls) {}
    ~ModuleDataTransmitter() override;  // StopAllCoroutines (the routines capture `this`)

    // ---- non-KSPField state ----
    std::optional<std::vector<int>> DeployFxModuleIndices;    // public int[] (null until OnLoad)
    std::optional<std::vector<int>> ProgressFxModuleIndices;
    std::vector<CommNet::TransmissionItem> transmissionQueue;
    std::optional<std::vector<IScalarModule*>> deployFxModules;    // null until OnStart
    std::optional<std::vector<IScalarModule*>> progressFxModules;
    std::string statusMessage, progressMessage, errorMessage;      // ScreenMessage.message
    std::string errorStr;

    // ---- KSPField accessors ----
    AntennaType antennaType() const;
    float packetInterval() const { return Get<float>("packetInterval"); }
    float packetSize() const { return Get<float>("packetSize"); }
    double packetResourceCost() const { return Get<double>("packetResourceCost"); }
    double antennaPower() const { return Get<double>("antennaPower"); }
    bool antennaCombinable() const { return Get<bool>("antennaCombinable"); }
    double antennaCombinableExponent() const { return Get<double>("antennaCombinableExponent"); }
    bool xmitIncomplete() const { return Get<bool>("xmitIncomplete"); }
    bool moduleIsEnabled() const { return isEnabled; }
    bool busy() const { return busy_; }
    bool xmitAborted() const { return xmitAborted_; }
    bool xmitOnHold() const { return xmitOnHold_; }
    double capacitorCharge() const { return capacitorCharge_; }

    // ---- lifecycle ----
    void OnAwake() override;                            // :107
    void CopySerializedFrom(const PartModule& from) override;  // Unity Instantiate: DeployFxModuleIndices / ProgressFxModuleIndices (public int[])
    void OnLoad(const ConfigNode& node) override;       // :133
    void OnSave(ConfigNode& node) override;             // :170
    void OnStart(StartState state) override;            // :183
    void UpdatePowerText();                             // :165
    IModuleInfo* AsModuleInfo() override { return this; }

    // ---- IScienceDataTransmitter ----
    float DataRate() const override;                    // :86
    double DataResourceCost() const override;           // :88
    bool CanTransmit() override;                        // :631
    bool IsBusy() override { return busy_; }            // :644
    void TransmitData(std::vector<CommNet::TransmissionItem> dataQueue) override;                    // :649
    void TransmitData(std::vector<CommNet::TransmissionItem> dataQueue, Callback<> callback) override;  // :658
    // ---- IResourceConsumer ----
    std::vector<const PartResourceDefinition*> GetConsumedResources() override { return consumedResources_; }  // :102
    // ---- ICommAntenna ----
    bool CommCombinable() const override { return antennaCombinable(); }                       // :90
    double CommCombinableExponent() const override { return antennaCombinableExponent(); }     // :92
    double CommPower() override;                                                               // :100
    AntennaType CommType() const override { return antennaType(); }                            // :94
    const DoubleCurve* CommRangeCurve() const override;                                        // :96
    const DoubleCurve* CommScienceCurve() const override;                                      // :98
    bool CanComm() override;                                                                   // :715
    bool CanCommUnloaded(ProtoPartModuleSnapshot* mSnap) override;                             // :724
    bool CanScienceTo(bool combined, double bPower, double sqrDistance) override;              // :706
    double CommPowerUnloaded(ProtoPartModuleSnapshot* mSnap) override;                         // :746
    // ---- IModuleInfo ----
    std::string GetModuleTitle() override { return "Data Transmitter"; }  // :808
    std::string GetPrimaryField() override { return std::string(); }      // :818 (null)
    std::string GetInfo() override;                                        // :667
    std::string GetModuleDisplayName() override;                           // :823
    // IContractObjectiveModule
    virtual std::string GetContractObjectiveType() { return "Antenna"; }   // :696
    virtual bool CheckContractObjectiveValidity() { return antennaType() != AntennaType::INTERNAL; }  // :701

    // ---- adjusters (Missions; OnModuleAdjusterAdded / Removed :764 / :773) ----
    void OnModuleAdjusterAdded(CommNet::IDataTransmitterAdjuster* adjuster) { adjusterCache_.push_back(adjuster); }
    void OnModuleAdjusterRemoved(CommNet::IDataTransmitterAdjuster* adjuster);

    // ---- behaviour ----
    virtual float GetVesselSignalStrength();                           // :207
    virtual double EvaluateScienceMultiplier(double signalStrength);   // :220
    virtual void StartTransmissionAction();                            // :256
    virtual void StartTransmission();                                  // :262
    virtual void TransmitIncompleteToggle();                           // :574
    virtual void StopTransmission();                                   // :595
    virtual void onStartTransmission(std::vector<CommNet::TransmissionItem> queue);  // :330 (queueVesselData already applied)
    virtual Enumerator transmitQueuedData(float transmitInterval, float dataPacketSize, Callback<> callback = Callback<>(), bool sendData = true);  // :358

protected:
    virtual std::optional<std::vector<IScalarModule*>> findFxModules(const std::optional<std::vector<int>>& indices, bool showUI);  // :229
    Enumerator WaitForFixedSeconds(float seconds);                      // :472
    virtual bool CanSetFXModules(const std::optional<std::vector<IScalarModule*>>& modules);  // :480
    virtual float GetFxModuleScalar(const std::optional<std::vector<IScalarModule*>>& modules);    // :494
    virtual float GetModulesScalarMax(const std::vector<IScalarModule*>& modules);   // :507
    virtual float GetModulesScalarMin(const std::vector<IScalarModule*>& modules);   // :522
    virtual Enumerator SetFXModules(std::optional<std::vector<IScalarModule*>> modules, float tgtValue);  // :537
    virtual void SetFXModulesUI(const std::optional<std::vector<IScalarModule*>>& fxModules, bool readState, bool writeState);  // :563
    virtual void AbortTransmission(const std::string& message);        // :600
    virtual void ReturnDataToContainer(const CommNet::TransmissionItem& data);  // :609
    double ApplyPowerAdjustments(double power, const std::vector<CommNet::IDataTransmitterAdjuster*>& adjusterList);  // :780
    bool IsDataTransmitterBroken(const std::vector<CommNet::IDataTransmitterAdjuster*>& adjusterList);  // :789
    std::shared_ptr<Coroutine> StartCoroutine(Enumerator e);
    int CoroutineOwner() const { return static_cast<int>(reinterpret_cast<uintptr_t>(this) >> 4); }
    Vessel* VesselPtr() const;
    std::string PartTitle() const;

    bool busy_ = false;
    bool xmitAborted_ = false;
    bool xmitOnHold_ = false;
    float deployFxModuleStartPosition_ = 0.0f;
    double capacitorCharge_ = 0.0;
    std::unique_ptr<CommNet::ICommsStream> commStream_;
    std::vector<const PartResourceDefinition*> consumedResources_;
    std::vector<CommNet::IDataTransmitterAdjuster*> adjusterCache_;
};

}  // namespace ksp
