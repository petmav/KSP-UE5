// WP-33: StoredPart.cs, ModuleCargoPart.cs, ModuleInventoryPart.cs, KerbalInventoryScenario.cs.
//
// Ported: the stored-part dictionary (slot -> StoredPart, DictionaryValueList order), slot / stack / variant rules (CanStackInSlot,
// UpdateStackAmountAtSlot, FirstEmptySlot, ...), the pack / unpack paths (StoreCargoPartAtSlot, ClearPartAtSlot, RemoveNPartsFromInventory,
// OnCopy, EVA propellant refill), the mass / volume / cost maths (UpdateCapacityValues, PreviewLimits, HasCapacity, GetModuleMass,
// GetModuleCost, the over-limit flags), OnLoad / OnSave / SaveDefault (STOREDPARTS, legacy PART / STACKAMOUNTS and `inventory` formats,
// DEFAULTPARTS), the placement state of DeployInventoryItem / CreatePartObject / SetPositionFromTerrain (terrain raycast and bounds are
// hooks), ground-part deployment (GetProtoVesselNode / CreatePartNode), the container highlight decision of OnUpdate, ModuleCargoPart's
// settle / construction-highlight logic and KerbalInventoryScenario (name changes, instance registry, OnSave -> roster).
//
// Seams (all in InventoryEnvironment; every hook unset = the Unity object is absent / the UI is not open):
//   * HighLogic / GameSettings / CheatOptions values the C# reads (scene flags come from SaveEnvironment::Current().loadedScene)
//   * highlighting, AudioSource / FXGroup, UIPartActionController windows, the PAW grid (UI_Grid / UIPartActionInventory)
//   * Physics.Raycast / Part bounds for the placement code, Object.Destroy of parts, HighLogic.CurrentGame.AddVessel
//   * ProtoVessel.CreateVesselNode's Guid / Random.rotation / FlightGlobals.GetUniquepersistentId
//   * ResearchAndDevelopment.PartModelPurchased (PartLoader.GetUnlockedCargoPartNames)
#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/core/coroutine.hpp"
#include "ksp/math/vector3d.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/proto_vessel.hpp"
#include "ksp/unity/bounds.hpp"
#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"
#include "ksp/util/collections.hpp"

namespace ksp {

class Part;
class Vessel;
class AvailablePart;
class ProtoCrewMember;
class CelestialBody;
class ModuleInventoryPart;
class ModuleCargoPart;
class UIPartActionInventory;
class KerbalEVA;
class CompoundPart;
class Orbit;
enum class ConstructionEventType : int;
enum class VesselType : int;

// ================================================================ StoredPart (StoredPart.cs:4)

class StoredPart {
public:
    int32_t slotIndex = -1;               // :7
    std::string partName;                 // :9
    std::shared_ptr<ProtoPartSnapshot> snapshot;  // :11
    int32_t quantity = 0;                 // :13
    int32_t stackCapacity = 1;            // :15
    std::string variantName;              // :17

    bool IsEmpty() const;                 // :19
    bool IsFull() const;                  // :29 (sic: quantity < stackCapacity, i.e. "has room")
    bool CanStack() const;                // :34
    StoredPart(std::string partName, int slotIndex);  // :47
    explicit StoredPart(const ConfigNode& node);      // :53 (internal)
    void Load(const ConfigNode& node);    // :58
    void Save(ConfigNode& node) const;    // :72
    std::shared_ptr<StoredPart> Copy() const;  // :87 (internal)
};

// ================================================================ environment / seams

struct PlacementRayHit {  // RaycastHit as SetPositionFromTerrain reads it
    Vector3f point;
    Vector3f normal;
    float distance = 0.0f;
    bool onTerrainLayer = false;  // collider.gameObject.layer == 15
    bool isROC = false;           // collider.CompareTag("ROC")
};

struct InventoryEnvironment {
    // ---- GameSettings.cs:1023-1027 defaults ----
    static inline float EVA_INVENTORY_RANGE = 5.0f;
    static inline float EVA_CONSTRUCTION_RANGE = 7.0f;
    static inline bool EVA_CONSTRUCTION_COMBINE_ENABLED = true;
    static inline bool EVA_CONSTRUCTION_COMBINE_NONENGINEERS = true;
    static inline float EVA_CONSTRUCTION_COMBINE_RANGE = 7.0f;
    static inline bool VAB_USE_ANGLE_SNAP = false;
    static inline float VAB_FINE_OFFSET_THRESHOLD = 20.0f;  // GameSettings.cs:969
    static inline bool MODIFIER_KEY_DOWN = false;           // GameSettings.MODIFIER_KEY.GetKey()
    // ---- CheatOptions.cs:23 / :25 ----
    static inline bool IgnoreEVAConstructionMassLimit = false;
    static inline bool IgnoreKerbalInventoryLimits = false;
    // ---- PartLoader.GetAvailableCargo*PartNames: ResearchAndDevelopment.PartModelPurchased; unset = every part is purchased ----
    static inline std::function<bool(const AvailablePart&)> partModelPurchased;
    static inline bool missionMode = false;  // HighLogic.CurrentGame.Mode == Game.Modes.MISSION
    // ---- scheduler for StartCoroutine (CreatePartObject, MakePartSettle, UpdateEngineerReport); unset: the coroutine is dropped ----
    static inline CoroutineScheduler* scheduler = nullptr;
    // ---- ScreenMessages.PostScreenMessage; unset: dropped ----
    static inline std::function<void(const std::string&, float)> postScreenMessage;
    static inline std::vector<std::string> screenMessages;  // every message posted (tests)
    // ---- engine / UI hooks (unset: the object is absent) ----
    static inline std::function<void(Part&, bool)> highlightPart;               // Part.Highlight(active) + HighlightActive
    static inline std::function<void(Part&)> destroyPartObject;                 // UnityEngine.Object.Destroy(part.gameObject)
    static inline std::function<bool(const Vector3f& origin, const Vector3f& dir, float maxDistance, PlacementRayHit& hit)> raycast;
    static inline std::function<float(Part&, const Vector3f& normal, float& centerPointOffset)> getBoundsPoints;  // Part.GetBoundsPoints
    static inline std::function<Bounds(Part&)> getPartRendererBound;            // Part.GetPartRendererBound
    static inline std::function<void(Part&)> placementPartCreated;              // after CreatePartObject set up the part (colliders, rb, highlight)
    static inline std::function<bool(ModuleInventoryPart&)> partMouseOver;      // part.MouseOver of the module's part
    static inline std::function<ProtoVessel*(const ConfigNode&)> addVessel;     // HighLogic.CurrentGame.AddVessel(node)
    static inline std::function<std::string()> newGuidN;                        // Guid.NewGuid().ToString("N")
    static inline std::function<Quaternionf()> randomRotation;                  // UnityEngine.Random.rotation
    static inline std::function<uint32_t()> uniquePersistentId;                 // FlightGlobals.GetUniquepersistentId()
    static inline std::function<uint32_t(Part&)> checkPartPersistentId;
    static inline std::function<std::string()> editorLastEventName;              // EditorLogic.fetch.lastEventName
    static inline std::function<bool(bool mode, ModuleInventoryPart* host)> setPartPlacementMode;  // evaController.SetPartPlacementMode (false = refused)
    static inline std::function<bool()> jetpackDeployed;                         // evaController.JetpackDeployed
    static inline std::function<void()> toggleJetpack;                           // evaController.ToggleJetpack()
    static inline bool alwaysShowInventory = false;                              // KerbalEVA.alwaysShowInventory
    static inline std::function<int(Part&)> currentCollisionCount;               // part.currentCollisions.Count
    static inline std::function<Vector3d()> bodyPosition;                        // mainBody.position (SetPositionFromTerrain's ray length)
    static inline std::function<Quaternionf()> bodyRotation;                     // mainBody.bodyTransform.rotation
    static inline std::function<uint32_t()> uniqueFlightID;                      // ShipConstruction.GetUniqueFlightID(flightState)
    static inline std::function<std::vector<Part*>()> persistentLoadedParts;     // FlightGlobals.PersistentLoadedPartIds.ValuesList
    static inline bool gameObjectActive = true;                                  // gameObject.activeInHierarchy
    static inline bool MODIFIER_KEY_EDGE = false;                                // MODIFIER_KEY.GetKeyDown() || GetKeyUp()
    static inline std::function<Quaternionf(Vessel&)> constructionOriginalRotation;  // LookRotation(ProjectOnPlane(body up - position, up), up)         // FlightGlobals.CheckPartpersistentId(id, part, false, true)
    static inline std::function<double()> universalTime;                        // Planetarium.GetUniversalTime(); unset: 0
    // welding sparks (AudioSource clips named "*sparking*"): count of clips, Random.Range, PlayDelayed(kerbalEVA.WeldFX.StartDelay), Stop
    static inline int weldSparkClipCount = 0;
    static inline std::function<int(int, int)> randomRange;                       // UnityEngine.Random.Range(min, max)
    static inline std::function<void(ModuleCargoPart&, int clip, float delay)> playWeldSparks;
    static inline std::function<void(ModuleCargoPart&)> stopWeldSparks;
    static inline std::function<float(KerbalEVA*)> weldStartDelay;               // kerbalEVA.WeldFX.StartDelay
    static inline std::function<void(Part&)> releasePartRigidbody;                // rb.constraints = None; rb.ResetInertiaTensor()
    static inline std::function<Vector3f()> activeVesselPosition;                 // FlightGlobals.ActiveVessel.vesselTransform.position
    static inline std::vector<std::string> log;  // Debug.LogWarning / LogError texts of this package, in order

    static std::vector<std::string> GetAvailableCargoPartNames();           // PartLoader.cs:2152
    static std::vector<std::string> GetAvailableCargoInventoryPartNames();  // PartLoader.cs:2157
    static bool CargoPartsLoaded();                                         // PartLoader.cs:67
    static void Post(const std::string& text, float duration);             // ScreenMessages.PostScreenMessage
    static double UT();
    static void Reset();
};

// ProtoVessel.CreateVesselNode / CreatePartNode (ProtoVessel.cs:853 / :907): not part of the WP-14 data side
struct ProtoVesselNodes {
    static ConfigNode::NodePtr CreateVesselNode(const std::string& vesselName, VesselType vesselType, const Orbit& orbit, int rootPartIndex,
                                                const std::vector<ConfigNode::NodePtr>& partNodes,
                                                const std::vector<CelestialBody*>& bodies);
    static ConfigNode::NodePtr CreatePartNode(Part* part, const std::string& partName, uint32_t id,
                                               const std::vector<std::shared_ptr<ProtoCrewMember>>& crew);
};

// the [KSPField]s of ModuleCargoPart on any module of that class (the ModuleGroundPart family is WP-53 and stays the generic PartModule)
inline float CargoPackedVolume(PartModule& m) { return m.Get<float>("packedVolume"); }
inline int CargoStackable(PartModule& m) { return m.Get<int32_t>("stackableQuantity"); }

// ================================================================ ModuleCargoPart (ModuleCargoPart.cs:8)

class ModuleCargoPart : public PartModule {
public:
    using PartModule::PartModule;
    ~ModuleCargoPart() override;

    // [KSPField] values live in the typed store
    float packedVolume() const { return Get<float>("packedVolume"); }
    float kinematicDelay() const { return Get<float>("kinematicDelay"); }
    float placementMaxRivotVelocity() const { return Get<float>("placementMaxRivotVelocity"); }
    int stackableQuantity() const { return Get<int32_t>("stackableQuantity"); }
    bool beingAttached() const { return Get<bool>("beingAttached"); }
    bool beingSettled() const { return Get<bool>("beingSettled"); }
    bool reinitResourcesOnStoreInVessel() const { return Get<bool>("reinitResourcesOnStoreInVessel"); }

    // state of the C# private / protected fields
    bool UIHidden = false;
    float preSettlingPrefabMass = 0.0f;
    bool inConstructionMode = false;
    bool constructionModeHighlight = false;
    float distanceToEngineer = 0.0f;
    std::vector<Part*> linkedCompoundParts;
    std::shared_ptr<Coroutine> settleCoroutine;
    // construction highlight output (Part.SetHighlight*): last applied state (tests / the shell)
    std::function<void(Part&, bool)> setConstructionHighlight;  // unset: recorded in constructionHighlightApplied
    std::optional<bool> constructionHighlightApplied;

    float MassForWeightTesting() const;                       // :44 (internal)
    void OnDestroy();                                         // :64
    virtual void OnPartPack() {}                              // :76
    virtual void OnPartUnpack();                              // :80
    void OnAwake() override;
    void OnLoad(const ConfigNode& node) override;             // :96
    void OnStart(StartState state) override;                  // :107
    void OnInitialize() override;                             // :158
    std::string GetInfo() override;                           // :164
    std::string GetModuleDisplayName() override;              // :449
    void OnPartWillDie(Part* p);                              // :183
    void HideUI();                                            // :194
    void ShowUI();                                            // :201
    virtual Enumerator MakePartSettle(bool waitForMotionStop);  // :206
    virtual void OnWeldStart(KerbalEVA* kerbalEVA);           // :229
    virtual void OnWeldFinish(KerbalEVA* kerbalEVA);          // :239
    bool weldSparksPlaying = false;                           // weldingSparkingsFX.isPlaying
    void OnEVAConstructionMode(bool opened);                  // :248
    void OnUpdate() override;                                 // :279
    void OnUpdateHighlight();                                 // :286
    void SetConstructionHighlight(bool state);                // :264
    virtual std::string GetTooltip();                         // :330
    bool IsDeployedSciencePart();                             // :339
    virtual void EnableModule();                              // :348
    void OnCompoundPartLinked(CompoundPart* linkedPart);      // :358
    void OnCompoundPartDetached(CompoundPart* linkedPart);    // :371
    bool HasLinkedCompoundParts() const;                      // :384
    // ExpansionsLoader.IsExpansionInstalled("Serenity"); unset: installed
    static inline std::function<bool(const std::string&)> isExpansionInstalled;

    Part* PartOf() const;
};

// The ModuleGroundPart family (ModuleGroundPart.cs:7 : ModuleCargoPart; ModuleGroundSciencePart, ModuleGroundCommsPart, ModuleGroundExpControl,
// ModuleGroundExperiment) is WP-53 (Breaking Ground). Until then these classes are ModuleCargoPart with its OnLoad / construction logic, so
// every `FindModuleImplementing<ModuleCargoPart>()` finds them as in C#; the deploy animation / science behaviour is absent.
class GroundPartSeam : public ModuleCargoPart {
public:
    using ModuleCargoPart::ModuleCargoPart;
};

// ================================================================ ModuleInventoryPart (ModuleInventoryPart.cs:9)

class ModuleInventoryPart : public PartModule, public IPartMassModifier, public IPartCostModifier {
public:
    using PartModule::PartModule;
    ~ModuleInventoryPart() override;
    // OnUpdate's placement block (:1300-:1403) minus the input polling: the host passes the key state
    struct PlacementInput {
        bool escapeDown = false, jumpDown = false, rcsToggleDown = false;
        bool up = false, down = false, left = false, right = false, fwd = false, back = false;  // TRANSLATE_* held
        float deltaTime = 0.0f;
        bool jetpackDeployed = false;  // vessel.isEVA && evaController.JetpackDeployed
        bool isActiveVessel = true;
    };
    IPartMassModifier* AsMassModifier() override { return this; }
    IPartCostModifier* AsCostModifier() override { return this; }

    // ---- fields that are not [KSPField] (C# members) ----
    float volumeCapacity = 0.0f;       // :36
    float volumeOccupied = 0.0f;       // :41
    bool volumeCapacityReached = false;  // :43
    float massCapacity = 0.0f;         // :45
    float massOccupied = 0.0f;         // :50
    bool massCapacityReached = false;  // :52
    DictionaryValueList<int32_t, std::shared_ptr<StoredPart>> storedParts;  // :67 (null until InitializeInventory: see storedPartsNull)
    bool storedPartsNull = true;
    bool showPreview = false;          // :69
    bool hasInitialized = false;       // :71
    std::vector<std::string> availableCargoParts;           // :73
    std::vector<std::string> availableCargoInventoryParts;  // :75
    std::vector<std::string> inventoryPartsByName;          // :61
    std::vector<std::string> defaultInventoryParts;         // :64
    bool defaultInventoryPartsNull = true;
    float spawnDistance = 1.0f;        // :78
    int maxInventorySlots = 30;        // :81
    int transferToSlot = -1;           // :84 (obsolete)
    int transferFromSlot = -1;         // :87 (obsolete)
    bool isGridShowing = false;        // :89
    std::shared_ptr<ProtoCrewMember> kerbalReference;  // :91
    int placementPendingIndex = -1;    // :94
    Part* selectedPart = nullptr;      // :97
    std::unique_ptr<Part> selectedPartOwner;  // the part CreatePartObject made (Object.Destroy ends its life)
    bool placementAllowXRotation = false;  // :106
    bool placementAllowYRotation = false;
    bool placementAllowZRotation = false;
    bool placementCoolDown = false;
    double placementCooldownTimer = 0.0;
    bool placementPositionOk = false;
    bool placementInsideCap = false;
    bool placementonTerrain = false;
    bool partFullyCreated = false;
    bool partBeingRetrieved = false;   // :130
    Vector3f placementStartingPosition;
    bool inConstructionMode = false;   // :141
    bool containerHighlight = false;   // :144
    float distanceToVessel = 0.0f;     // :147
    bool gridPresent = false;          // `grid != null`
    bool gridUpdateSlotItems = false;  // grid.updateSlotItems
    UIPartActionInventory* pawInventory = nullptr;           // grid.pawInventory
    bool pawInventoryActive = false;                         // pawInventory.gameObject.activeInHierarchy
    UIPartActionInventory* constructorModeInventory = nullptr;  // :58
    bool partHighlightActive = false;                        // Part.HighlightActive
    Bounds selectedPartBounds{};                             // :103
    Vector3f transformPosition{};                            // module.transform.position of a kerbal-mode module (not in the world)
    PlacementInput placementInput;                           // the key state OnUpdate's placement block polls (the host fills it)
    int placementSoundPlays = 0, soundsPlayed = 0, effectPlays = 0;
    std::vector<std::string> engineerReportRequests;  // RequestUpdateEngineerReport calls (tests)
    std::vector<std::string> highlightLog;

    // ---- [KSPField] accessors ----
    int InventorySlots() const { return Get<int32_t>("InventorySlots"); }
    void SetInventorySlots(int n) { Set<int32_t>("InventorySlots", n); }
    float packedVolumeLimit() const { return Get<float>("packedVolumeLimit"); }
    float massLimit() const { return Get<float>("massLimit"); }
    float placementRotateSpeed() const { return Get<float>("placementRotateSpeed"); }
    float placementGroundOffset() const { return Get<float>("placementGroundOffset"); }
    float placementGroundOffsetCap() const { return Get<float>("placementGroundOffsetCap"); }
    std::string Inventory() const;
    void SetInventory(const std::string& v);

    // ---- properties ----
    bool HasPackedVolumeLimit() const { return packedVolumeLimit() > 0.0f; }  // :149
    bool HasMassLimit() const { return massLimit() > 0.0f; }                  // :151
    std::vector<std::string> InventoryPartsList();                            // :154
    bool InventoryIsEmpty() const;                                            // :175
    bool InventoryIsFull() const;                                             // :187
    int InventoryItemCount() const;                                           // :199
    bool AbleToPlaceParts() const;                                            // :211
    bool IsKerbalOnEVA() const;                                               // :223
    bool kerbalMode() const;                                                  // :235
    Part* SelectedPart() const { return selectedPart; }                       // :246

    // ---- editor part events / highlight ----
    void OnEditorPartEvent(ConstructionEventType evt, Part* p);   // :254
    void VesselEditorPartHighlighter(Part* p);                    // :280
    void PartDroppedOnInventory(ConstructionEventType evt, Part* p);  // :308

    // ---- slots / stacks ----
    int GetStackAmountAtSlot(int slotIndex);                                  // :386
    int GetStackCapacityAtSlot(int slotIndex);                                // :395
    bool IsStackable(int slotIndex);                                          // :404
    bool CanStackInSlot(const AvailablePart& part, const std::string& partVariantName, int slotIndex);  // :413
    bool UpdateStackAmountAtSlot(int slotIndex, int howMany, const std::string& variantName = "");  // :429
    std::string GetVariantNameForSlot(int slotIndex);                         // :452
    bool HasStackingSpace(int slotIndex);                                     // :461
    bool IsSlotEmpty(int slotIndex);                                          // :470
    int FirstEmptySlot();                                                     // :1701
    int FirstFullSlot();                                                      // :1720
    int TotalEmptySlots();                                                    // :1739
    bool ContainsPart(const std::string& partName);                           // :1754
    int TotalAmountOfPartStored(const std::string& requestedPartName);        // :1862
    bool RemoveNPartsFromInventory(const std::string& requestedPartName, int n, bool playSound = false);  // :1881

    // ---- deploy / placement ----
    virtual void DeployInventoryItem(int index);                              // :475
    virtual bool InventoryItemCanBeDeployed(int index);                       // :489
    Enumerator CreatePartObject(std::shared_ptr<StoredPart> storedPart);      // :502
    void SetPositionFromTerrain();                                            // :598
    void DeletePartObject();                                                  // :672
    void CancelPartPlacementMode();                                           // :1691
    void DeployGroundPart(StoredPart& storedPart, const Vector3f& partPosition, const Quaternionf& rotation);  // :1769
    ConfigNode::NodePtr GetProtoVesselNode(StoredPart& storedPart, const Vector3f& partPosition, const Quaternionf& rotation);  // :1775
    ConfigNode::NodePtr CreatePartNode(uint32_t id, StoredPart& storedPart);  // :1816
    void UpdatePlacement(const PlacementInput& in);

    // ---- store / clear ----
    bool StoreCargoPartAtSlot(const std::string& partName, bool updatePAW = true, int index = -1);  // :708 (obsolete)
    bool StoreCargoPartAtSlot(const std::string& partName, int slotIndex);    // :722
    bool StoreCargoPartAtSlot(Part* sPart, int slotIndex);                    // :733
    bool StoreCargoPartAtSlot(std::shared_ptr<ProtoPartSnapshot> pPart, int slotIndex);  // :762
    void RefillEVAPropellantOnStoring(ProtoPartSnapshot& ppSS);               // :803
    void RefillEVAPropellantOnBoarding(Part& p);                              // :808 (internal)
    void RefillEVAPropellant(ProtoPartSnapshot& ppSS, Part& p);               // :820
    bool ClearPartAtSlot(int slotIndex);                                      // :842
    bool ClearPartAtSlot(int slotIndex, bool bypassEvent);                    // :847 (internal)
    bool CheckPartStorage(const std::string& partName, int index = -1);       // :865
    void RequestUpdateEngineerReport();                                       // :870
    Enumerator UpdateEngineerReport();                                        // :882
    void OnModuleInventoryChanged(ModuleInventoryPart* moduleInventory);      // :891
    void OnPartPurchased(AvailablePart* part);                                // :904

    // ---- lifecycle ----
    void OnStart(StartState state) override;                                  // :910
    void OnDestroy();                                                         // :1031
    void OnLoad(const ConfigNode& node) override;                             // :1040
    void OnSave(ConfigNode& node) override;                                   // :1130
    void SaveDefault(ConfigNode& node);                                       // :1175
    void Update();                                                            // :1181 (internal)
    void OnUpdate() override;                                                 // :1223
    std::string GetInfo() override;                                           // :1406
    std::string GetModuleDisplayName() override;                              // :1424
    void OnCopy(PartModule& fromModule) override;                             // :1744
    // Unity Instantiate of the prefab module: the [SerializeField] members (storedParts is a non-null, empty copy; the dictionary half of a
    // DictionaryValueList is not serializable), defaultInventoryParts, inventoryPartsByName, maxInventorySlots, spawnDistance
    void CopySerializedFrom(const PartModule& from) override;
    void OnAwake() override;

    // ---- capacity maths ----
    void UpdateCapacityValues();                                              // :1429
    void InitializeMassVolumeDisplay();                                       // :1449
    void UpdateMassVolumeDisplay(bool updateCapacities, bool constructionOnly);  // :1461
    void PreviewLimits(Part& newPart, int amountToStore, const AvailablePart* slotPart, int slotIndex);  // :1508
    void RestoreLimitsDisplay();                                              // :1566
    float GetPartMass(Part& part);                                            // :1571
    bool HasCapacity(Part& newPart);                                          // :1576
    void KerbalStateChanged();                                                // :1591 (internal)
    void ToggleInventoryVisibility(bool show);                                // :1599
    void onPartActionUIOpened(Part* p);                                       // :1605
    void InitializeInventory();                                               // :1613
    void SetInventoryDefaults();                                              // :1635
    void ResetInventoryPartsByName();                                         // :1657
    void UpdateModuleUI();                                                    // :1683
    float GetModuleMass(float defaultMass, ModifierStagingSituation sit) override;  // :1928
    ModifierChangeWhen GetModuleMassChangeWhen() override { return ModifierChangeWhen::CONSTANTLY; }  // :1961
    float GetModuleCost(float defaultCost, ModifierStagingSituation sit) override;  // :1966
    ModifierChangeWhen GetModuleCostChangeWhen() override { return ModifierChangeWhen::CONSTANTLY; }  // :1989
    bool IgnoreKerbalInventoryLimits();                                       // :1994

    Part* PartOf() const;
    Vessel* VesselOf() const;
    Vector3f TransformPosition() const;  // module.transform.position (kerbal-mode modules are not in the world: transformPosition)

private:
    bool eventsAdded_ = false;
    bool editorEventsAdded_ = false;
    std::shared_ptr<Coroutine> placementCoroutine_;
};

// ================================================================ KerbalInventoryScenario (KerbalInventoryScenario.cs:12)

class KerbalInventoryScenario : public ScenarioModule {
public:
    KerbalInventoryScenario();
    ~KerbalInventoryScenario() override;
    static inline KerbalInventoryScenario* Instance = nullptr;  // :20
    // KerbalInventoryGameObject children (names), standing in for the DontDestroyOnLoad GameObject
    static inline std::vector<std::string> kerbalInventoryGameObjectChildren;  // :22

    void OnAwake() override;                       // :36
    void Update();                                 // :48 (Unity message)
    void OnSave(ConfigNode& node) override;        // :56
    void OnDestroy();                              // :68
    void OnKerbalNameChanged(ProtoCrewMember* crew, std::string prevName, std::string newName);  // :88
    void AddKerbalInventoryInstance(const std::string& crewName, ModuleInventoryPart* inventory);  // :99
    bool ContainsCrew(const std::string& crewName) const;     // :113
    void RemoveKerbalInventoryInstance(const std::string& crewName);  // :118
    static void Register();                        // ScenarioModuleFactory registration
    const std::vector<std::pair<std::string, ModuleInventoryPart*>>& Inventories() const { return kerbalInventories_; }
    // the SaveInventory target (HighLogic.CurrentGame.CrewRoster[name]); unset: SaveEnvironment::Current().currentGame->CrewRoster
    static inline std::function<std::shared_ptr<ProtoCrewMember>(const std::string&)> findCrew;
    static void ClearStatics();

private:
    // Dictionary<string, ModuleInventoryPart>: insertion order (the dictionary has no removals before the end in KSP's usage)
    std::vector<std::pair<std::string, ModuleInventoryPart*>> kerbalInventories_;
    bool eventsAdded_ = false;
    int Find(const std::string& name) const;
};

// ================================================================ kerbal glue

// Installs the ProtoCrewMember hooks (kerbalModuleCost / kerbalModuleMass / kerbalPrefabResourceMass) and the roster's
// saveDefaultKerbalInventory on top of the real ModuleInventoryPart: ProtoCrewMember.kerbalModule (:348) instantiates the EVA prefab's
// module per kerbal.
struct KerbalInventory {
    // ProtoCrewMember.kerbalModule (ProtoCrewMember.cs:348): the kerbal's inventory module (created from the EVA part's prefab module)
    static ModuleInventoryPart* ModuleFor(ProtoCrewMember& crew);
    // the C# instanceKerbalModule dictionary lives on the crew member; here keyed by the crew member object
    static void Forget(ProtoCrewMember* crew);
    static void Install();    // hooks
    static void Uninstall();
    static void ClearAll();
};


// registers the KerbalInventoryScenario type with the ScenarioModuleFactory
void RegisterInventoryScenarios();

}  // namespace ksp
