// WP-35 science part modules: ModuleScienceExperiment.cs, ModuleScienceContainer.cs, ModuleScienceLab.cs, ModuleScienceConverter.cs,
// ModuleEnviroSensor.cs, ModuleSampleContainer.cs, ScienceLabSearch.cs and ExperimentTransfer.cs (the data side: IsValidPart).
//
// Lifecycle: OnAwake / OnLoad / OnStart / OnSave and the [KSPEvent] / [KSPAction] bodies are the PartModule hooks / bound handlers; the Unity
// messages (Update, FixedUpdate, OnDestroy) are public members the host calls (the destructor does OnDestroy's unsubscription). The C# iterators
// (gatherData, OnScienceCompleteDelay, SetFXModules, resetExperiment, ModuleScienceLab.CleanUpVesselExperiments / gatherProcessResources, the
// ProcessData iterator) are C++20 coroutines on ScienceEnvironment::Scheduler(), started with the same StartCoroutine semantics (the WP-00
// scheduler: run to the first yield now).
//
// Seams (see ScienceEnvironment in ksp/science/science.hpp and ScienceModuleEnvironment below): the result dialogs (ExperimentsResultDialog,
// PopupDialog, UIConfirmDialog, PartItemTransfer / ExperimentTransfer's selection UI) are hooks / recorded pages; ScreenMessages are recorded;
// KerbalEVA actions (PlayGolf / Banana / Dzhanibekov), the inventory, the engine-side transforms are hooks.
#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ksp/events/game_events.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/modules/resource_processing.hpp"
#include "ksp/science/science.hpp"

namespace ksp {

class ModuleResource;
class ModuleScienceContainer;
class ModuleScienceExperiment;
class ModuleScienceLab;
class PartResourceDefinition;

struct ScienceModuleEnvironment {
    // KerbalEVA.PlayGolf / Banana / Dzhanibekov (callback = afterX): the host runs the animation and calls the callback when it ends. Unset: the
    // callback is called at once.
    static inline std::function<void(const std::string& kerbalAction, Part& evaPart, std::function<void()> afterAction)> evaAction;
    // typeof(KerbalEVA).GetMethod(name) != null. Unset: the three public stock methods (KerbalEVA.cs:6226 PlayGolf, :6342 Banana, Dzhanibekov).
    static inline std::function<bool(const std::string& name)> kerbalEvaHasMethod;
    // ModuleInventoryPart.ContainsPart(name) (the inventory package). Unset: false.
    static inline std::function<bool(PartModule& inventory, const std::string& partName)> inventoryContainsPart;
    // UIPartActionController.Instance != null && ItemListContains(part, false). Unset: ResourceFlowEnvironment::partActionUIContains, else false.
    static inline std::function<bool(const Part*)> partActionUIContains;
    // FlightGlobals.getGeeForceAtPosition(transform.position).magnitude for ModuleEnviroSensor GRAV. Unset: Vessel::geeForce.
    static inline std::function<double(Part&)> geeForceAtPosition;
    // GameEvents / PartItemTransfer.DismissActive (a no-op here). The list of ExperimentTransfer.Create requests: (source part, container).
    static inline std::vector<std::pair<Part*, IScienceDataContainer*>> experimentTransfers;
    static void Reset();
};

// ScienceLabSearch.cs:4
class ScienceLabSearch {
public:
    enum class SearchError { NONE, BUG, MIXED, NO_LABS, NO_MANNED, NO_SPACE, ALL_RESEARCHED, ALL_PROCESSING };

    Vessel* vessel = nullptr;                 // :18 (Vessel)
    std::shared_ptr<ScienceData> Data;        // :20
    ModuleScienceLab* NextLabForData = nullptr;  // :22
    SearchError Error = SearchError::NONE;    // :24
    int LabsTotal = 0, LabsManned = 0, LabsWithSpace = 0, LabsNotResearched = 0, LabsNotProcessing = 0;  // :26-:34

    bool HasAnyLabs() const { return LabsTotal > 0; }          // :36
    bool HasMultipleLabs() const { return LabsTotal > 1; }     // :38
    bool NextLabForDataFound() const { return NextLabForData != nullptr; }  // :40
    std::string ErrorString() const;                           // :42
    double ScienceExpectation() const;                         // :82
    double TimeExpectation() const;                            // :94
    std::string DataExpectationSummary() const;                // :106
    ScienceLabSearch(Vessel* vessel, const std::shared_ptr<ScienceData>& data);  // :118
    void PostErrorToScreen() const;                            // :199
};

// ExperimentTransfer.cs:3 (PartItemTransfer is UI; the part filter and the create guard are the data side)
class ExperimentTransfer {
public:
    // :12: false (`ExperimentsResultDialog.Instance != null && pages.Count > 0`, posts a screen message) or the transfer request is recorded in
    // ScienceModuleEnvironment::experimentTransfers
    static bool Create(Part& srcPart, IScienceDataContainer& cont);
    static bool IsSemiValidPart(Part& p);  // :107
    static bool IsValidPart(Part& p);      // :112
};

// ModuleScienceExperiment.cs:10
class ModuleScienceExperiment : public PartModule, public IScienceDataContainer {
public:
    explicit ModuleScienceExperiment(const ModuleClassInfo& cls) : PartModule(cls) {}
    ~ModuleScienceExperiment() override;

    // ModuleScienceExperiment.EVASituation (:15, a ScriptableObject)
    struct EVASituation {
        int priority = 1;
        uint32_t situationMask = 0;
        bool RequiresAtmosphere = false;
        bool RequiresNoAtmosphere = false;
        float MinTemp = -3.4028234663852886e+38f;  // float.MinValue
        float MaxTemp = 3.4028234663852886e+38f;   // float.MaxValue
        std::string KerbalAction;
        std::vector<std::string> ResultString;
        float dialogDelay = 0.0f;
        void Load(const ConfigNode& node);  // :35
        void Save(ConfigNode&) {}           // :49
    };

    using ResetCallback = std::function<void()>;
    ResetCallback OnExperimentReset;                       // :99
    ScienceExperiment* experiment = nullptr;               // :170
    bool DeployEventDisabled = false;                      // :176
    std::optional<std::vector<int>> fxModuleIndices;       // :182
    std::vector<IScalarModule*> fxModules;                 // :184 (null until OnStart finds fxModuleIndices)
    bool fxModulesCreated = false;
    std::optional<std::vector<std::shared_ptr<EVASituation>>> evaSituations;  // :140
    bool containersDirty = false;                          // :198
    bool hasContainer = false;                             // :200
    bool useCooldown = false;                              // :162
    bool HasExperimentData() const { return experimentData != nullptr; }  // :202
    std::shared_ptr<ScienceData> experimentData;           // :174
    std::shared_ptr<ScienceSubject> subject;               // :172
    std::shared_ptr<ExperimentResultDialogPage> resultsDialog;  // :178
    bool overwriteDialogOpen = false;                      // :180
    PartModule* inventory = nullptr;                       // :186
    std::shared_ptr<EVASituation> validEvaSituation;       // :188
    bool kerbalInSeat = false;                             // :148
    bool resettingExperiment = false;                      // :190
    double dialogTimer = 0.0;                              // :192
    bool showDialogAfter = false;                          // :194
    ExperimentSituations situation = ExperimentSituations::SrfLanded;  // :196
    std::string lastIconColor;                             // stackIcon.SetIconColor(XKCDColors.Cyan / Color.white): "cyan" / "white" (UI)

    // ---- PartModule hooks ----
    void OnAwake() override;                               // :204
    // Unity Instantiate: the public serialisable members (ModuleScienceExperiment.cs:99-:200) travel with the KSPFields
    void CopySerializedFrom(const PartModule& from) override;
    void OnLoad(const ConfigNode& node) override;          // :213
    void OnSave(ConfigNode& node) override;                // :240
    void OnStart(StartState state) override;               // :248
    void Update();                                         // :303
    void OnInventoryModeDisable();                         // :326
    void OnInventoryModeEnable();                          // :331
    void OnVesselSituationChange(GameEvents::HostedFromToAction<Vessel*, Vessel_Situations> vcs);  // :336
    void OnModuleInventoryChanged(ModuleInventoryPart* inventoryPart);  // :348
    void OnCommandSeatInteraction(Part* evaPart, bool entered);          // :360 (eva.part; KerbalEVA is WP-32)
    void OnVesselModified(Vessel* v);                      // :1175
    void OnActive() override;                              // :562
    bool IsStageable() override { return Get<bool>("useStaging"); }          // :1183
    bool StagingEnabled() override { return Get<bool>("useStaging") ? Get<bool>("stagingEnabled") : false; }  // :1188
    std::string GetStagingEnableText() override;           // :1207
    std::string GetStagingDisableText() override;          // :1216
    std::string GetInfo() override;                        // :1097
    std::string GetModuleDisplayName() override;           // :1119
    virtual bool CanBeDetached() { return !resettingExperiment; }  // :1225
    virtual bool CanBeOffset() { return !resettingExperiment; }    // :1230
    virtual bool CanBeRotated() { return !resettingExperiment; }   // :1235

    // ---- [KSPEvent] / [KSPAction] ----
    void DeployExperiment();                               // :489
    void DeployAction(KSPActionParam& actParams);          // :525
    void CollectDataExternalEvent();                       // :835
    void ReviewDataEvent();                                // :907
    void ResetExperiment();                                // :926
    void ResetAction(KSPActionParam& actParams);           // :933
    void DeployExperimentExternal();                       // :939
    void ResetExperimentExternal();                        // :975
    void CleanUpExperimentExternal();                      // :1068
    void TransferDataEvent();                              // :1125

    void TransferToContainer(ModuleScienceContainer* container, const std::string& destName);  // :841
    void onCollectData(ModuleScienceContainer* vesselContainer);  // :882
    void OnDataTransfer(bool itemMoved, Part* containerPart);      // :1166 (PartItemTransfer.DismissAction.ItemMoved)
    void SetInoperable();                                  // :1060

    // ---- IScienceDataContainer ----
    std::vector<std::shared_ptr<ScienceData>> GetData() override;           // :1005
    void ReturnData(const std::shared_ptr<ScienceData>& data) override;     // :1014
    void DumpData(const std::shared_ptr<ScienceData>& data) override;       // :1025
    void ReviewData() override;                                             // :1037
    void ReviewDataItem(const std::shared_ptr<ScienceData>& data) override; // :1089
    int GetScienceCount() override;                                         // :1046
    bool IsRerunnable() override;                                           // :1055

    // The user's choices on the result page (the private callbacks the page holds): endExperiment (discard), keepData, sendDataToComms, sendDataToLab
    void endExperiment(const std::shared_ptr<ScienceData>& pageData);   // :759
    void keepData(const std::shared_ptr<ScienceData>& pageData);        // :919
    void sendDataToComms(const std::shared_ptr<ScienceData>& pageData); // :777
    void sendDataToLab(const std::shared_ptr<ScienceData>& pageData);   // :810
    void updateModuleUI();                                              // :393
    void FindContainer();                                               // :1137
    bool ValidEVASituation();                                           // :440

    // The coroutines in flight (stopped by the destructor)
    std::vector<std::shared_ptr<Coroutine>> running;

private:
    Part* PartOf() const;
    Vessel* VesselOf() const;
    Enumerator gatherData(bool showDialog);        // :589
    Enumerator OnScienceCompleteDelay();           // :669
    void OnScienceComplete();                      // :664
    Enumerator SetFXModules(float tgt);            // :732
    Enumerator resetExperiment();                  // :981
    void dumpData();                               // :770
    void reviewData();                             // :913
    void Start_(Enumerator e);
    bool subscribed_ = false;
};

// ModuleScienceContainer.cs:6
class ModuleScienceContainer : public PartModule, public IScienceDataContainer {
public:
    explicit ModuleScienceContainer(const ModuleClassInfo& cls) : PartModule(cls) {}
    ~ModuleScienceContainer() override;

    std::vector<std::shared_ptr<ScienceData>> dataStore;       // :11
    std::vector<std::shared_ptr<ScienceData>> pagesWithDialogs;  // :13

    void OnLoad(const ConfigNode& node) override;              // :57
    void OnSave(ConfigNode& node) override;                    // :76
    void OnStart(StartState state) override;                   // :85
    void OnAwake() override;
    std::string GetModuleDisplayName() override;               // :619

    int GetActiveVesselDataCount();                            // :134
    int GetStoredDataCount();                                  // :158
    void StoreDataExternalEvent();                             // :177
    void CollectDataExternalEvent();                           // :209
    void TransferToContainer(ModuleScienceContainer* activeVesselContainer, const std::string& targetName);  // :215
    void TransferDataEvent();                                  // :253
    void CollectAllAction(KSPActionParam& actParams);          // :266
    void CollectAllEvent();                                    // :276
    void OnDataTransfer(bool itemMoved, Part* containerPart);  // :310
    // :319 (containers are consumed: the C# removes entries from the list it is given)
    bool StoreData(std::vector<IScienceDataContainer*>& containers, bool dumpRepeats, bool excludeScienceContainers = false,
                   bool scienceContainerPriority = false);
    bool AddData(const std::shared_ptr<ScienceData>& data);    // :449
    bool HasData(const ScienceData& data);                     // :470
    void RemoveData(const std::shared_ptr<ScienceData>& data); // :484
    void ReviewDataEvent();                                    // :525
    void discardData(const std::shared_ptr<ScienceData>& pageData);       // :558
    void keepData(const std::shared_ptr<ScienceData>& pageData);          // :565
    void sendDataToComms(const std::shared_ptr<ScienceData>& pageData);   // :571
    void sendDataToLab(const std::shared_ptr<ScienceData>& pageData);     // :593
    void updateModuleUI();                                     // :105

    std::vector<std::shared_ptr<ScienceData>> GetData() override;           // :490
    void ReturnData(const std::shared_ptr<ScienceData>& data) override;     // :495
    void DumpData(const std::shared_ptr<ScienceData>& data) override;       // :517
    void ReviewData() override;                                             // :531
    void ReviewDataItem(const std::shared_ptr<ScienceData>& data) override; // :549
    int GetScienceCount() override;                                         // :609
    bool IsRerunnable() override;                                           // :614

private:
    Part* PartOf() const;
    Vessel* VesselOf() const;
    void sortContainerList(std::vector<IScienceDataContainer*>& containers, bool scienceContainerPriority);  // :398
    void reviewData();                                         // :537
    void onPartActionUIOpened(Part* p);                        // :97
    bool subscribed_ = false;
};

// ModuleScienceLab.cs:11
class ModuleScienceLab : public PartModule, public IResourceConsumer, public IScienceDataContainer {
public:
    explicit ModuleScienceLab(const ModuleClassInfo& cls) : PartModule(cls) {}
    ~ModuleScienceLab() override;

    std::vector<std::string> ExperimentData;                   // :13
    bool processingData = false;                               // :53
    std::vector<std::shared_ptr<ModuleResource>> processResources;  // :57
    bool allResourcesAvailable = false;                        // :67
    std::vector<const PartResourceDefinition*> consumedResources;  // :69

    ModuleScienceConverter* Converter();                       // :73
    std::vector<const PartResourceDefinition*> GetConsumedResources() override { return consumedResources; }  // :75
    void TransmitScience();                                    // :81
    void OnAwake() override;                                   // :107
    // Unity Instantiate: ExperimentData / processResources are public serialisable members (ModuleScienceLab.cs:13, :57)
    void CopySerializedFrom(const PartModule& from) override;
    void OnLoad(const ConfigNode& node) override;             // :136
    void OnSave(ConfigNode& node) override;                   // :165
    void OnStart(StartState state) override;                   // :178
    bool IsStorable(const ScienceData& item);                  // :237
    Enumerator ProcessData(std::shared_ptr<ScienceData> item, std::function<void(const std::shared_ptr<ScienceData>&)> onComplete = nullptr);  // :254
    void CleanModulesEvent();                                  // :275
    Enumerator CleanUpVesselExperiments(Vessel* v);            // :280
    Enumerator CleanUpExperiment(ModuleScienceExperiment* exp);  // :303
    bool IsOperational();                                      // :372
    std::string GetInfo() override;                            // :381
    std::string GetModuleDisplayName() override;               // :505
    static void RecoverScienceLabs(ProtoVessel& protoVessel, KSP::UI::Screens::MissionRecoveryDialog* dialog);  // :426
    void OnTransmissionComplete(ScienceData* data, Vessel* origin, bool xmitAborted);  // :86
    void updateModuleUI();                                     // :200

    // the explicit interface implementations (:474-:503): an empty container
    std::vector<std::shared_ptr<ScienceData>> GetData() override { return {}; }
    void ReturnData(const std::shared_ptr<ScienceData>&) override {}
    void DumpData(const std::shared_ptr<ScienceData>&) override {}
    void ReviewData() override {}
    void ReviewDataItem(const std::shared_ptr<ScienceData>&) override {}
    int GetScienceCount() override { return 0; }
    bool IsRerunnable() override { return false; }

    bool transmittingData = false;                             // :55
    std::vector<std::shared_ptr<Coroutine>> running;
    // ScreenMessage progressMessage.message of the last PostScreenMessage(progressMessage)
    std::string progressMessage;
    bool cleanModulesEventActive = false, transmitScienceEventActive = true;

private:
    Part* PartOf() const;
    Vessel* VesselOf() const;
    void StoreData(const std::shared_ptr<ScienceData>& item);  // :265
    void updateModuleUI_(Part* p);                             // :192 onPartActionUI
    Enumerator gatherProcessResources(float amount, std::string expParttitle, std::string progressMessageText, std::string shortageMessageText);  // :336
    void TransmissionErrorScreenMessage(const std::string& reason);  // :386
    void sendDataToComms();                                    // :391
    ModuleScienceConverter* converter_ = nullptr;
    ModuleScienceContainer* container_ = nullptr;
    bool subscribed_ = false;
};

// ModuleScienceConverter.cs:6
class ModuleScienceConverter : public BaseConverter {
public:
    explicit ModuleScienceConverter(const ModuleClassInfo& cls) : BaseConverter(cls) {}

    virtual ModuleScienceLab* Lab();                           // :43
    void OnStart(StartState state) override;                   // :45
    virtual std::string DataExpectationSummary(float dataAmount);  // :51
    virtual double CalculateScienceAmount(float dataAmount);   // :63
    virtual double CalculateScienceRate(float dataAmount);     // :68
    virtual double CalculateResearchTime(float dataAmount);    // :74
    virtual void UpdateDisplayStrings();                       // :80
    bool IsSituationValid() override { return true; }          // :87
    ConversionRecipe* PrepareRecipe(double deltatime) override;  // :92
    void PreProcessing() override;                             // :109
    void PostProcess(const ConverterResults& result, double deltaTime) override;  // :118
    virtual float GetScientists();                             // :164
    static void CacheLocalStrings();                           // :179

private:
    ModuleScienceLab* lab_ = nullptr;
    ConversionRecipe recipe_;
};

// ModuleEnviroSensor.cs:4
class ModuleEnviroSensor : public PartModule, public IResourceConsumer {
public:
    enum class SensorType { TEMP, GRAV, ACC, PRES };           // :6
    explicit ModuleEnviroSensor(const ModuleClassInfo& cls) : PartModule(cls) {}

    std::vector<const PartResourceDefinition*> consumedResources;  // :23
    std::vector<const PartResourceDefinition*> GetConsumedResources() override { return consumedResources; }  // :39
    void ToggleAction(KSPActionParam& param);                  // :45
    void Toggle();                                             // :58
    void OnAwake() override;                                   // :63
    void OnLoad(const ConfigNode& node) override;              // :80
    void FixedUpdate();                                        // :97
    std::string GetInfo() override;                            // :156
    std::string GetModuleDisplayName() override;               // :161
    static void CacheLocalStrings();                           // :166
};

// ModuleSampleContainer.cs:1
class ModuleSampleContainer : public PartModule {
public:
    explicit ModuleSampleContainer(const ModuleClassInfo& cls) : PartModule(cls) {}
    std::shared_ptr<PlanetarySample> sample;                   // :3
    void OnAwake() override;
    void OnStart(StartState state) override;                   // :14
    void TransferSample(const std::shared_ptr<PlanetarySample>& transferee);  // :22
    void Discard();                                            // :32
};

namespace detail {
void RegisterScienceModules();  // module_behaviours.cpp
}

}  // namespace ksp
