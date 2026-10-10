// WP-28 resource-processing part modules (numeric / data side):
//   BaseConverter.cs, BaseDrill.cs, ModuleResourceConverter.cs, ModuleResourceHarvester.cs, ModuleAsteroidDrill.cs,
//   ModuleCometDrill.cs, ModuleGenerator.cs, ModuleAlternator.cs, ModuleFuelJettison.cs, ModuleResourceDrain.cs,
//   ModuleSpaceObjectInfo.cs / ModuleSpaceObjectResource.cs / ModuleSpaceObjectAnalysis.cs, ModuleAsteroid{Info,Resource,Analysis}.cs,
//   ModuleComet{Info,Resource,Analysis}.cs, ModuleAnalysisResource.cs, ModuleResourceScanner.cs, ModuleOrbitalScanner.cs,
//   ModuleOrbitalSurveyor.cs, ModuleBiomeScanner.cs.
// (ConversionRecipe.cs / ConverterResults.cs / ResourceConverter.cs were ported by WP-15; ModuleDeployableSolarPanel.cs is WP-27's,
// ModuleAsteroid.cs / ModuleComet.cs are WP-09's.)
//
// Lifecycle: the Unity messages (FixedUpdate, Update, Start, OnDestroy) of these modules are public member functions the host calls;
// OnAwake / OnLoad / OnStart / OnSave and the [KSPEvent] / [KSPAction] bodies are the PartModule hooks / bound handlers.
//
// Everything a module reads from outside the part (Part.vessel and its state, FlightGlobals, Time, Planetarium, the overlay generator,
// the model's transforms) comes through ResourceProcessingEnvironment: plain inputs the host sets, outputs it can read. Not modelled
// (documented in STATUS.md): the PAW / screen-message presentation (only the message texts are recorded), Transform lookups and
// raycasts (host hooks), GetInfo text, the module adjusters (AdjusterPartModuleBase lists), animations.
#pragma once

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "ksp/modules/part_module.hpp"
#include "ksp/physics/vessel_ranges.hpp"
#include "ksp/resources/resource_broker.hpp"
#include "ksp/resources/resource_types.hpp"
#include "ksp/unity/vector.hpp"
#include "ksp/vessel/vessel_types.hpp"

namespace ksp {

class Part;
class Vessel;
struct PartResource;

// A crew member as the converters see them (ProtoCrewMember.HasEffect(effect) / experienceLevel)
struct ProcessingCrewMember {
    std::set<std::string> effects;
    int experienceLevel = 0;
};

// The CelestialBody values the modules read (vessel.mainBody / FlightGlobals.currentMainBody)
struct ProcessingBody {
    int flightGlobalsIndex = 1;
    double Radius = 600000.0;
    double sphereOfInfluence = 0.0;
    bool atmosphere = false;
    bool hasBiomeMap = true;  // BiomeMap != null
    std::string bodyName, displayName;
    float recoveryValue = 1.0f;  // scienceValues.RecoveryValue
};

// The Vessel values the modules read (Part.vessel and what hangs off it)
struct ProcessingVessel {
    bool Landed = false, Splashed = false;
    VesselSituations situation = VesselSituations::PRELAUNCH;
    double altitude = 0.0, radarAltitude = 0.0, latitude = 0.0, longitude = 0.0;
    double atmDensity = 0.0, srfSpeed = 0.0, horizontalSrfSpeed = 0.0, solarFlux = 0.0;
    Vector3d srf_vel_direction;
    bool isActiveVessel = false;
    bool isControllable = true;                      // vessel.IsControllable
    ControlLevel controlLevel = ControlLevel::FULL;  // vessel.CurrentControlLevel
    float mainThrottle = 0.0f;                       // vessel.ctrlState.mainThrottle
    std::string SituationString;
    ProcessingBody mainBody;
    double PeA = 0.0, ApA = 0.0, inclination = 0.0;  // vessel.orbit
    std::vector<Part*> parts;                        // vessel.parts
    std::vector<ProcessingCrewMember> crew;          // vessel.GetVesselCrew()
    bool LandedOrSplashed() const { return Landed || Splashed; }
};

// OverlayGenerator.Instance (the resource map overlay of the orbital scanner; the rendering is not modelled, its state is)
struct OverlayGeneratorState {
    struct ResourceItem {
        std::string name, displayName;
    };
    bool IsActive = false, IsMapView = false;
    int Cutoff = 0;
    int DisplayMode = 0;       // MapDisplayTypes: Monochrome, Inverse, HeatMapGreen, HeatMapBlue
    int DisplayBody = -1;      // flightGlobalsIndex, -1 = null
    std::vector<ResourceItem> ResourceList;
    int DisplayResource = -1;  // index into ResourceList, -1 = null
    HarvestTypes DisplayResourceType = HarvestTypes::Planetary;
    // calls received (OverlayGenerator.FetchNextResource / GenerateOverlay(checkForLock) / ClearDisplay)
    int fetchNextResourceCalls = 0, generateOverlayCalls = 0, clearDisplayCalls = 0;
    bool lastCheckForLock = false;
};

struct ResourceProcessingEnvironment {
    // Part.vessel as a value record; nullptr = `vessel == null`
    static inline std::function<const ProcessingVessel*(const Part&)> vesselOf;
    // Planetarium.GetUniversalTime(); unset: ResourceEnvironment::universalTime, then the Planetarium
    static inline std::function<double()> universalTime;
    static double UniversalTime();
    static inline float timeSinceLevelLoad = 10.0f;  // Time.timeSinceLevelLoad
    static inline bool flightReady = true;           // FlightGlobals.ready
    // ModuleCoreHeat.CoreTemperature of a part's core heat module (a property of the thermal package); unset: 0
    static inline std::function<double(PartModule&)> coreTemperature;
    // ModuleAnimationGroup.isDeployed (animation package); unset: deployed
    static inline std::function<bool(PartModule&)> animationGroupDeployed;
    // the model-dependent checks: Physics.Raycast along the impact transform (drills / harvester); unset: true (a hit)
    static inline std::function<bool(PartModule&)> impactRaycast;
    // CBAttributeMapSO.GetAtt(lat, lon) of the vessel's main body, through ResourceUtilities.GetBiome (radians); nullopt = no biome
    struct Biome {
        std::string name, displayname;
    };
    static inline std::function<std::optional<Biome>(double lat, double lon, const Part&)> biomeAt;
    // the intake transform's forward (Transform.forward of FindModelTransform(intakeTransformName))
    static inline std::function<std::optional<Vector3f>(PartModule&)> intakeForward;
    // ScreenMessages.PostScreenMessage texts, in order
    static inline std::vector<std::string> screenMessages;
    static inline OverlayGeneratorState overlay;
    static void Reset();
    static const ProcessingVessel* VesselOf(const Part* p);
    // the record of a real Vessel (the host installs `vesselOf` with it)
    static ProcessingVessel FromVessel(Vessel& v);
};

// the C# `Part part = ...; part.Modules[i] is ModuleAsteroid` mass setter: ModuleAsteroid.SetAsteroidMass / ModuleComet.SetCometMass
class ISpaceObjectMass {
public:
    virtual ~ISpaceObjectMass() = default;
    virtual void SetSpaceObjectMass(float nMass) = 0;
};

// ======================================================= BaseConverter =========================================================

// BaseConverter.cs:6 (abstract; IAnimatedModule / IOverheatDisplay / IConstruction members are plain methods)
class BaseConverter : public PartModule {
public:
    using PartModule::PartModule;
    std::vector<ResourceRatio> inputList, outputList, reqList;  // :8-:12 (null until OnAwake)
    bool listsCreated = false;
    std::unique_ptr<IResourceBroker> ownedBroker_;      // :71 _resBroker
    IResourceBroker* _resBroker = nullptr;
    std::unique_ptr<ResourceConverter> _resConverter;   // :73
    double lastUpdateTime = 0.0, lastHeatFlux = 0.0, lastTimeFactor = 0.0, statusPercent = 0.0;
    BaseEvent* startEvt = nullptr;
    BaseEvent* stopEvt = nullptr;
    bool _preCalculateEfficiency = false;
    std::vector<std::pair<std::string, float>> EfficiencyModifiers;  // Dictionary<string, float> (insertion order; nothing removes)
    bool efficiencyModifiersCreated = false;
    float _totalEfficiencyModifiers = 0.0f;
    bool hasAnimationGroup = false;
    PartModule* partAnimationModule = nullptr;
    int catchupRetries = 0;
    static constexpr int MAX_CATCHUP_RETRIES = 10;

    ResourceConverter& ResConverter();   // :130 (virtual property)
    IResourceBroker& ResBroker();        // :130
    virtual void UpdateDebugInfo(const ConverterResults& result, double deltaTime);  // :134
    virtual float GetCrewEfficiencyBonus();                                          // :144
    virtual float GetCrewHeatBonus();                                                // :158
    virtual void SetupDebugging();                                                   // :169
    virtual void EnableModule();                                                     // :178
    virtual void DisableModule();                                                    // :185
    virtual bool ModuleIsActive();                                                   // :193
    bool IsAnimationGroupDeployed();                                                 // :198 (private)
    void StartResourceConverter();                                                   // :208 [KSPEvent]
    void StopResourceConverter();                                                    // :222 [KSPEvent]
    void StopResourceConverterAction(KSPActionParam& param);                         // :229 [KSPAction]
    void StartResourceConverterAction(KSPActionParam& param);                        // :235
    void ToggleResourceConverterAction(KSPActionParam& param);                       // :241
    virtual void SetEfficiencyBonus(float bonus);                                    // :253
    virtual double GetDeltaTime();                                                   // :258
    double GetBestDeltaTime(double deltaTime);                                       // :292
    virtual void UpdateConverterStatus();                                            // :326
    void OnLoad(const ConfigNode& node) override;                                    // :342
    virtual void SetupLabels();                                                      // :400
    virtual void SetupModule();                                                      // :420
    void OnStart(StartState state) override;                                         // :434
    void OnAwake() override;                                                         // :460
    void OnSave(ConfigNode& node) override;                                          // :486
    // Unity's Instantiate: the public serialisable members (the three ResourceRatio lists) travel with the KSPFields
    void CopySerializedFrom(const PartModule& from) override;
    virtual double GetEfficiencyMultiplier();                                        // :493
    virtual void FixedUpdate();                                                      // :500 (Unity message)
    virtual void TallyEfficiencyModifiers();                                         // :557
    virtual void ConvertRecipeToUnits(ConversionRecipe& recipe);                     // :566
    virtual float GetHeatThrottle();                                                 // :621
    virtual void CheckForShutdown();                                                 // :635
    virtual int HasSpecialist(const std::string& effect);                            // :648
    virtual bool IsOverheating();                                                    // :671
    virtual void PreProcessing() {}                                                  // :681
    virtual void PostUpdateCleanup() {}                                              // :685
    virtual void PostProcess(const ConverterResults& result, double deltaTime);      // :689
    virtual ConversionRecipe* PrepareRecipe(double deltatime);                       // :708
    virtual bool IsSituationValid();                                                 // :714
    virtual void OnOverheat(double amount = 1.0);                                    // :720
    virtual double GetFlux() { return lastHeatFlux; }                                // :727
    virtual bool DisplayCoreHeat();                                                  // :732
    virtual double GetCoreTemperature();                                             // :737
    virtual double GetGoalTemperature();                                             // :747
    virtual bool CanBeDetached() { return !Get<bool>("IsActivated"); }               // :757
    virtual bool CanBeOffset() { return !Get<bool>("IsActivated"); }                 // :762
    virtual bool CanBeRotated() { return !Get<bool>("IsActivated"); }                // :767
    std::string GetModuleDisplayName() override;                                     // :772
    static void CacheLocalStrings();                                                 // :777

    Part* GetPart() const;
    const ProcessingVessel* Vessel() const;

protected:
    static bool ShowDebugOptions();  // ResourceSetup.Instance.ResConfig.ShowDebugOptions
};

// BaseDrill.cs:3
class BaseDrill : public BaseConverter {
public:
    using BaseConverter::BaseConverter;
    std::string GetModuleDisplayName() override;  // :14
};

// ModuleResourceConverter.cs:7
class ModuleResourceConverter : public BaseConverter {
public:
    using BaseConverter::BaseConverter;
    std::unique_ptr<ConversionRecipe> _recipe;                      // :11
    virtual ConversionRecipe& Recipe();                             // :13
    ConversionRecipe* PrepareRecipe(double deltatime) override;     // :15
    virtual std::unique_ptr<ConversionRecipe> LoadRecipe();         // :25
    bool IsSituationValid() override { return true; }               // :120
    std::string GetModuleDisplayName() override;                    // :125
};

// ModuleResourceHarvester.cs:8 (IResourceConsumer)
class ModuleResourceHarvester : public BaseDrill, public IResourceConsumer {
public:
    using BaseDrill::BaseDrill;
    double _resFlow = 0.0, _displayedResFlow = 0.0;
    bool _isValidSituation = true;
    bool hasImpactTransform = false;                 // impactTransformCache != null (the model's transform)
    std::string intakeTransformName = "Intake";
    ConversionRecipe recipe;
    ResourceRatio eInput;
    bool hasEInput = false;
    bool cachedWasNotAsteroid = true, cachedWasNotComet = true;
    int partCountCache = -1;

    void OnStart(StartState state) override;                              // :154
    void CopySerializedFrom(const PartModule& from) override;             // Instantiate: eInput / hasEInput / intakeTransformName
    ConversionRecipe* PrepareRecipe(double deltaTime) override;           // :176
    double GetIntakeMultiplier();                                         // :250 (private)
    void PostUpdateCleanup() override;                                    // :271
    virtual void LoadRecipe(double harvestRate);                          // :291
    void PostProcess(const ConverterResults& result, double deltaTime) override;  // :324
    virtual bool CheckForImpact();                                        // :348
    bool IsSituationValid() override;                                     // :358
    void PreProcessing() override;                                        // :428
    std::vector<const PartResourceDefinition*> GetConsumedResources() override;  // :443
    std::string GetModuleDisplayName() override;                          // :448
    static void CacheLocalStrings();                                      // :453
};

// ModuleAsteroidDrill.cs:8 / ModuleCometDrill.cs:8 share one body; the two classes differ in the potato / info / resource module
// class names and two status texts.
class ModuleSpaceObjectDrillBase : public BaseDrill {
public:
    using BaseDrill::BaseDrill;
    bool _isValidSituation = true;
    bool hasImpactTransform = false;
    ConversionRecipe recipe;
    double _drilledMass = 0.0;
    std::string _status;
    Part* _potato = nullptr;
    PartModule* _info = nullptr;  // ModuleAsteroidInfo / ModuleCometInfo

    ConversionRecipe* PrepareRecipe(double deltaTime) override;           // :50
    virtual bool CheckForImpact();                                        // :154
    virtual Part* GetAttachedPotato();                                    // :170
    bool IsSituationValid() override;                                     // :208
    void OnUpdate() override;                                             // :205
    void OnStart(StartState state) override;                              // :222
    void PostProcess(const ConverterResults& result, double deltaTime) override;  // :...

protected:
    virtual const char* ObjectClass() const = 0;     // "ModuleAsteroid" / "ModuleComet"
    virtual const char* InfoClass() const = 0;       // "ModuleAsteroidInfo" / "ModuleCometInfo"
    virtual const char* ResourceClass() const = 0;   // "ModuleAsteroidResource" / "ModuleCometResource"
    virtual std::string NoObjectStatus() const = 0;      // autoLOC_258405 / autoLOC_6002530
    virtual std::string NotAttachedStatus() const = 0;   // autoLOC_258412 / autoLOC_6002531
};
class ModuleAsteroidDrill : public ModuleSpaceObjectDrillBase {
public:
    using ModuleSpaceObjectDrillBase::ModuleSpaceObjectDrillBase;
    std::string GetModuleDisplayName() override;
    static void CacheLocalStrings();
protected:
    const char* ObjectClass() const override { return "ModuleAsteroid"; }
    const char* InfoClass() const override { return "ModuleAsteroidInfo"; }
    const char* ResourceClass() const override { return "ModuleAsteroidResource"; }
    std::string NoObjectStatus() const override;
    std::string NotAttachedStatus() const override;
};
class ModuleCometDrill : public ModuleSpaceObjectDrillBase {
public:
    using ModuleSpaceObjectDrillBase::ModuleSpaceObjectDrillBase;
    std::string GetModuleDisplayName() override;
    static void CacheLocalStrings();
protected:
    const char* ObjectClass() const override { return "ModuleComet"; }
    const char* InfoClass() const override { return "ModuleCometInfo"; }
    const char* ResourceClass() const override { return "ModuleCometResource"; }
    std::string NoObjectStatus() const override;
    std::string NotAttachedStatus() const override;
};

// ================================================== space-object info / resource / analysis ===================================

// ModuleSpaceObjectResource.cs:5 (ModuleAsteroidResource.cs / ModuleCometResource.cs derive without members)
class ModuleSpaceObjectResource : public PartModule {
public:
    using PartModule::PartModule;
    ResourceFlowMode _flowMode = ResourceFlowMode::NULL_;  // :26
    void OnLoad(const ConfigNode& node) override;          // :28
    void CopySerializedFrom(const PartModule& from) override;  // Instantiate: _flowMode
};

// ModuleSpaceObjectInfo.cs:4
class ModuleSpaceObjectInfo : public PartModule {
public:
    using PartModule::PartModule;
    float totalResourceMass = 0.0f;       // :26
    double totalResourcePercent = 0.0;    // :28
    void CopySerializedFrom(const PartModule& from) override;  // Instantiate: totalResourceMass / totalResourcePercent
    double currentMassVal() const;        // :30 (double.TryParse; 0 on failure)
    void set_currentMassVal(double v);    // :31 (G17)
    double massThresholdVal() const;      // :46
    void set_massThresholdVal(double v);  // :45
    void SetupSpaceObjectResources(const std::vector<ModuleSpaceObjectResource*>& resInfoList);  // :59
};

// ModuleAsteroidInfo.cs:4 / ModuleCometInfo.cs:4 (the same body; `baseMod` is ModuleAsteroid / ModuleComet)
class ModuleSpaceObjectInfoBase : public ModuleSpaceObjectInfo {
public:
    using ModuleSpaceObjectInfo::ModuleSpaceObjectInfo;
    PartModule* baseMod = nullptr;
    std::vector<ModuleSpaceObjectResource*> resInfoList;
    void OnStart(StartState state) override;  // :11
    virtual void SetupAsteroidResources();    // :31
    virtual void Update();                    // :37
protected:
    virtual const char* ObjectClass() const = 0;
    virtual const char* ResourceClass() const = 0;
};
class ModuleAsteroidInfo : public ModuleSpaceObjectInfoBase {
public:
    using ModuleSpaceObjectInfoBase::ModuleSpaceObjectInfoBase;
protected:
    const char* ObjectClass() const override { return "ModuleAsteroid"; }
    const char* ResourceClass() const override { return "ModuleAsteroidResource"; }
};
class ModuleCometInfo : public ModuleSpaceObjectInfoBase {
public:
    using ModuleSpaceObjectInfoBase::ModuleSpaceObjectInfoBase;
protected:
    const char* ObjectClass() const override { return "ModuleComet"; }
    const char* ResourceClass() const override { return "ModuleCometResource"; }
};

// ModuleAnalysisResource.cs:1
class ModuleAnalysisResource : public PartModule {
public:
    using PartModule::PartModule;
    bool statusFldGuiActive = false;  // statusFld.guiActive (the field's flag after SetupAnalysis)
    void OnStart(StartState state) override;  // :16
    void Update();                            // :22
    void SetupAnalysis();                     // :30
};

// ModuleSpaceObjectAnalysis.cs:3
class ModuleSpaceObjectAnalysis : public PartModule {
public:
    using PartModule::PartModule;
    std::map<std::string, ModuleAnalysisResource*> analyzers;  // :5 (Dictionary<string, ModuleAnalysisResource>: null values allowed)
    ModuleAnalysisResource* FindMatchingAnalyzer(Part& p, const std::string& rName);  // :7
};

// ModuleAsteroidAnalysis.cs:3 / ModuleCometAnalysis.cs:3 (the same body)
class ModuleSpaceObjectAnalysisBase : public ModuleSpaceObjectAnalysis {
public:
    using ModuleSpaceObjectAnalysis::ModuleSpaceObjectAnalysis;
    Part* potatoPart = nullptr;
    void OnStart(StartState state) override;  // :5
    void OnDestroy();                         // :15
    void CheckForPotato(Vessel* v);           // :21 (CheckForAsteroid for the comet)
    ~ModuleSpaceObjectAnalysisBase() override;
    void ClearAsteroidResources();            // :34
    void FindAsteroidResources(Part* p);      // :56
protected:
    virtual const char* ObjectClass() const = 0;
    virtual const char* ResourceClass() const = 0;
private:
    bool subscribed_ = false;
};
class ModuleAsteroidAnalysis : public ModuleSpaceObjectAnalysisBase {
public:
    using ModuleSpaceObjectAnalysisBase::ModuleSpaceObjectAnalysisBase;
protected:
    const char* ObjectClass() const override { return "ModuleAsteroid"; }
    const char* ResourceClass() const override { return "ModuleAsteroidResource"; }
};
class ModuleCometAnalysis : public ModuleSpaceObjectAnalysisBase {
public:
    using ModuleSpaceObjectAnalysisBase::ModuleSpaceObjectAnalysisBase;
protected:
    const char* ObjectClass() const override { return "ModuleComet"; }
    const char* ResourceClass() const override { return "ModuleCometResource"; }
};

// ================================================== generator, alternator, jettison, drain =====================================

// ModuleGenerator.cs:5 (IContractObjectiveModule)
class ModuleGenerator : public PartModule {
public:
    using PartModule::PartModule;
    ModuleResource* currentResource = nullptr;  // :52
    double inputRecieved = 0.0;                 // :54
    bool lackingResources = false;              // :56
    std::vector<std::function<float(float)>> efficiencyAdjusters;  // adjusterCache (AdjusterGeneratorBase.ApplyEfficiencyAdjustment)
    void OnAwake() override;                    // (binds the events / actions)
    void ToggleAction(KSPActionParam& param);   // :65
    void ActivateAction(KSPActionParam&);       // :78
    void ShutdownAction(KSPActionParam&);       // :84
    void Activate();                            // :90 [KSPEvent]
    void Shutdown();                            // :95 [KSPEvent]
    void OnStart(StartState state) override;    // :111
    void FixedUpdate();                         // :147 (Unity message)
    std::string GetContractObjectiveType() const { return "Generator"; }  // :224
    bool CheckContractObjectiveValidity() const { return !Get<bool>("isGroundFixture"); }  // :229
    std::string GetModuleDisplayName() override;  // :234
    float ApplyEfficiencyAdjustments(float efficiency);  // :255
};

// ModuleAlternator.cs:9
class ModuleAlternator : public PartModule {
public:
    using PartModule::PartModule;
    // IEngineStatus.normalizedOutput of the engine found by FindEngineNearby / ModuleRoboticServoRotor.normalizedOutput (the engine and
    // robotics packages own them): the host binds them
    std::function<float()> engineNormalizedOutput;   // `engine != null` when set
    std::function<float()> rotorNormalizedOutput;    // ModuleRoboticServoRotor.normalizedOutput (`rotor != null` is the module being found)
    PartModule* rotor = nullptr;                     // :7 (FindModuleImplementing<ModuleRoboticServoRotor>)
    bool serenityInstalled = true;                   // ExpansionsLoader.IsExpansionInstalled("Serenity")
    std::vector<std::function<float(float)>> outputAdjusters;  // AdjusterAlternatorBase.ApplyOutputAdjustment
    void OnAwake() override;                         // :44
    void OnLoad(const ConfigNode& node) override;    // :50
    void OnStart(StartState state) override;         // :57
    void FixedUpdate();                              // :110 (Unity message)
    float ApplyOutputAdjustments(float output);      // :150
    std::string GetModuleDisplayName() override;     // :159
};

// ModuleFuelJettison.cs:3
class ModuleFuelJettison : public PartModule {
public:
    using PartModule::PartModule;
    void OnAwake() override;
    void FuelJettison();                         // :9 [KSPEvent]
    void Dump(PartResource& res);                // :28
    void FuelJettisonAction(KSPActionParam&);    // :35 [KSPAction]
    void OnStart(StartState state) override;     // :40
};

// ModuleResourceDrain.cs:6 (IConstruction)
class ModuleResourceDrain : public PartModule {
public:
    using PartModule::PartModule;
    struct ResourceDrainStatus {  // :9
        const PartResourceDefinition* resource = nullptr;
        bool isDraining = false, showDrainFX = false;
        std::string drainFXName;
        int fxPriority = 0;
        float drainISP = 0.0f;
    };
    float minDrainRate = 1.0f, maxDrainRate = 20.0f;  // :36-:38
    std::vector<PartResource*> resourcesAvailable;    // :73
    std::vector<ResourceDrainStatus> resourceDrainStatus;  // :75
    bool isInactive = false;
    bool enabledBehaviour = true;                     // MonoBehaviour.enabled
    float perResourceExhaustVel = 0.0f, perResourceThrust = 0.0f;
    int drainedResources = 0;
    double drainAmount = 0.0, amtReceived = 0.0;
    int highestValue = 0, highestIndex = 0;
    std::string loadedSavedResources;                 // :80
    std::optional<std::vector<std::string>> loadedResources;
    // forces added by DrainPart: Part.AddForce(part.transform.forward * thrust): the thrust magnitudes, in order
    std::vector<float> addedForces;
    static inline double epsilon = 1E-15;            // :46

    void OnAwake() override;                          // :110
    void OnStart(StartState state) override;          // :121
    void OnStartFinished(StartState state) override;  // :147
    void FixedUpdate();                               // :157
    void OnDestroy();                                 // :165
    void StartResourceDrainAction(KSPActionParam&);   // :173
    void StopResourceDrainAction(KSPActionParam&);    // :179
    void ToggleResourceDrainAction(KSPActionParam&);  // :185
    void ToggleResourceDrainFlowAction(KSPActionParam&);  // :191
    void TogglePartFlowMode();                        // :202
    void OnPartResourceListChanged(Part* hostPart);   // :207
    void Initialize();                                // :223
    void LoadResources();                             // :254
    void LoadResourcesAvailable();                    // :277
    bool HasEnoughDrainingResources();                // :350
    bool DrainResourceNotFinished(int rdsIndex);      // :376
    void UpdateResourcesToRelease(const std::vector<std::string>& resourceNames, bool exclude = false);  // :392
    virtual void DrainPart();                         // :455
    bool PartContainsResource(const std::string& partResource);  // :501
    void TurnOnDrain();                               // :510
    void TurnOffDrain();                              // :534
    void TurnOffDrain(const std::vector<std::string>& resourcesToStopDraining);  // :539
    void ToggleDrain();                               // :544
    bool IsResourceDraining(PartResource& pR);        // :557
    PartResource* GetPartResource(const std::string& resourceName);  // :561
    void TogglePartResource(PartResource* resource, bool status);    // :585
    void UpdateResourceDrainStatus(int resourceID, bool status);     // :595
    int ResourcesToDrain();                           // :607
    bool ResourcesToDrainHaveFX();                    // :618
    bool GetResourceDrainStatus(PartResource& resource);  // :636
    virtual bool CanBeDetached() { return !isDraining(); }  // :660
    virtual bool CanBeOffset() { return !isDraining(); }    // :665
    virtual bool CanBeRotated() { return !isDraining(); }   // :670
    void OnSave(ConfigNode& node) override;           // :675
    void OnLoad(const ConfigNode& node) override;     // :696
    bool IsDraining() const { return isDraining(); }  // :94 (property)
    void set_IsDraining(bool v);                      // :95
    bool IsInactive() const { return isInactive; }    // :108
    // the FX groups of the effects system (Part.findFxGroup / FXGroup.setActive / SetPower): groups are names of Part::fxGroupNames;
    // the active flag and power each group was last given are recorded
    std::vector<std::string> drainFXGroups;           // :77
    std::optional<std::string> highestPriorityFXGroup;  // :79 (null = nullopt)
    std::map<std::string, bool> fxGroupActive;
    std::map<std::string, float> fxGroupPower;

private:
    bool isDraining() const { return Get<bool>("isDraining"); }
    void UpdateVFXToUse();                            // :302
    void UpdatePriorityIndices();                     // :337
    void CheckDrainingForEffects();                   // :518 (object argument unused)
};

// ======================================================= scanners ==========================================================

// ModuleResourceScanner.cs:4 (IAnimatedModule)
class ModuleResourceScanner : public PartModule {
public:
    using PartModule::PartModule;
    double abundanceValue = 0.0;                      // :20
    std::string ResourceDisplayName;                  // :22
    int partCacheCount = -1;                          // :26
    bool wasValid = false;                            // :28
    bool aDispFldGuiActive = false;                   // aDispFld.guiActive
    double abundance() const { return abundanceValue; }  // :39
    void EnableModule() { isEnabled = true; }         // :41
    void DisableModule() { isEnabled = false; }       // :46
    bool ModuleIsActive() { return isEnabled; }       // :51
    std::string GetDisplayFormat();                   // :56
    std::string GetScanningMode(bool abbrevation);    // :58
    void OnAwake() override;                          // :82
    void OnStart(StartState state) override;          // :87
    void Update();                                    // :101 (Unity message)
    void CheckAbundanceDisplay();                     // :128
    void DisplayAbundance();                          // :146
    void ToggleEvent(const std::string& name, bool status);  // :194
    bool IsSituationValid();                          // :200
    std::string GetModuleDisplayName() override;      // :247
};

// ModuleOrbitalScanner.cs:3 (IAnimatedModule)
class ModuleOrbitalScanner : public PartModule {
public:
    using PartModule::PartModule;
    BaseEvent* switchR = nullptr;
    BaseEvent* toggleO = nullptr;
    BaseEvent* setCol = nullptr;
    bool _ShowSurveyMenu = false, _ShowOverlayMenu = false;
    void OnAwake() override;                          // (binds the events)
    virtual void SwitchResource();                    // :29 [KSPEvent]
    virtual void ToggleOverlay();                     // :40 [KSPEvent]
    virtual void GenerateOverlay();                   // :52
    virtual void HideOverlay();                       // :60
    virtual void SetColor();                          // :64 [KSPEvent]
    virtual std::string GetOverlayColor(int dispType);  // :79
    virtual void IncreaseCutoff();                    // :99 [KSPEvent]
    virtual void DecreaseCutoff();                    // :113
    virtual void ActivateScanner();                   // :127 [KSPEvent]
    virtual void ToggleSurvey(bool state);            // :132
    virtual void ToggleMenu(bool state);              // :141
    void OnActive() override;                         // :154
    virtual void UpdateGUI();                         // :163
    virtual void FixedUpdate();                       // :176 (Unity message)
    virtual void SetupMap();                          // :193
    virtual void Start();                             // :205 (Unity message)
    virtual void ShowResourceName();                  // :216
    virtual void EnableModule();                      // :245
    virtual void DisableModule();                     // :250
    virtual bool ModuleIsActive();                    // :259
    virtual bool IsSituationValid();                  // :264
    virtual void OnDestroy();                         // :274
    virtual void OnResourceMapLoaded();               // :279
    ~ModuleOrbitalScanner() override;
private:
    bool subscribed_ = false;
};

// ModuleOrbitalSurveyor.cs:5 (IAnimatedModule)
class ModuleOrbitalSurveyor : public PartModule {
public:
    using PartModule::PartModule;
    BaseEvent* perfSurv = nullptr;
    // sendDataToComms (:60): the best transmitter (ScienceUtil.GetBestTransmitter) is the science package's. `transmit` receives the
    // ScienceData's (dataAmount, subjectID, container flightID); unset: no transmitter (a screen message)
    std::function<bool(float dataAmount, const std::string& subjectID)> transmit;
    uint32_t flightID = 0;  // part.flightID
    // research and development (ResearchAndDevelopment.Instance.AddScience): unset: no instance
    std::function<void(float science)> addScience;
    void OnAwake() override;
    virtual void PerformSurvey();                     // :25 [KSPEvent]
    void OnStart(StartState state) override;          // :41
    void rescaleThresholds();                         // :44
    virtual void OnDestroy();                         // :72
    virtual void sendDataToComms();                   // :77
    // OnTriggeredDataTransmission(data, origin, xmitAborted): data.container, data.subjectID
    virtual void CompleteSurvey(uint32_t container, const std::string& subjectID, bool xmitAborted);  // :94
    virtual void CheckForScanData();                  // :119
    virtual void Update();                            // :130 (Unity message)
    virtual void EnableModule() { isEnabled = true; }   // :136
    virtual void DisableModule() { isEnabled = false; } // :141
    virtual bool ModuleIsActive() { return isEnabled; } // :146
    virtual bool IsSituationValid() { return true; }    // :151
    std::string GetModuleDisplayName() override;      // :161
    int surveysCompleted = 0;                         // OnOrbitalSurveyCompleted fired
    ~ModuleOrbitalSurveyor() override;
private:
    bool subscribed_ = false;
};

// ModuleBiomeScanner.cs:3
class ModuleBiomeScanner : public PartModule {
public:
    using PartModule::PartModule;
    void OnAwake() override;
    virtual void RunAnalysis();  // :6 [KSPEvent]
    void FixedUpdate();          // :28 (Unity message)
};

namespace detail {
void RegisterResourceProcessingModules();  // called by RegisterBuiltinModules
}

}  // namespace ksp
