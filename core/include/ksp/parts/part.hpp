// WP-13: Part (Part.cs), the part model and part tree.
//
// ksp::Part IS the prefab record of WP-05 (PartPrefab: the C# public fields in `fields`, the attach nodes, resources, drag
// cubes, module records) plus the run-time state and logic of Part.cs that does not need Unity rendering, physics or UI:
//   tree          parent / children / symmetryCounterparts, setParent, clearParent, addChild, removeChild, hasIndirectChild /
//                 Parent, FindNonPhysicslessChildren / Parent, GetPhysicslessChildMass, FindPartThroughNodes, SetHierarchyRoot,
//                 GetPartsOutTo, FindChildPart, CleanSymmetryReferences
//   attach nodes  AddAttachNode, UpdateAttachNodes, SetupAttachNodes, FindAttachNode(s), FindAttachNodeByPart, SetMirror
//   modules       AddModule (name / node), LoadModule, RemoveModule(s), FindModuleImplementing, GetModuleStartState,
//                 ModulesOnStart / Fixed / Update ..., InitializeModules over the WP-06 module host (PartModuleList)
//   resources     SetupResources, AddResource, SetResource, RemoveResource, GetResourceMass
//   mass, cost    UpdateMass, prefabMass, GetModuleMass / GetModuleCosts / GetModuleSize, the Start() thermal-mass block
//   crew          CrewCapacity, AddCrewmember(At), RemoveCrewmember
//   copy          Instantiate (UnityEngine.Object.Instantiate of a prefab), OnWillBeCopied / OnCopy / OnWasCopied
//   events        SendEvent over the part, its modules, parent, children, fuelLookupTargets and attached parts
//   state         State / CanFail, physicalSignificance and PhysicsSignificance, resource priority, UpdateStageability
// Not ported (Unity rendering, colliders, joints, FX, audio, UI, Vessel): see STATUS.md "WP-13" for the line ranges.
//
// Lifetime: a Part has an address (modules, attach nodes and children point at it): it is created with
// `std::make_unique<Part>()` (PartLoader) or Part::Instantiate and never moved or copied.
#pragma once

#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/modules/base_event.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/parts/available_part.hpp"
#include "ksp/parts/part_types.hpp"
#include "ksp/save/crew.hpp"
#include "ksp/physics/vessel_ranges.hpp"
#include "ksp/save/proto_vessel.hpp"
#include "ksp/unity/bounds.hpp"
#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

class ModulePartVariants;
class ShipConstruct;

// Unity Transform stand-in for the Part's own transform (position, rotation, localScale): only what the engine-agnostic Part
// maths reads (AttachNode.ReverseSrfNodeDirection / ChangeSrfNodePosition, UpdateOrgPosAndRot, SetMirror's model scale).
// TransformPoint = rotation * Scale(localScale, p) + position (UnityEngine.Transform with a parentless transform; the
// native matrix rounding order is not captured).
struct PartTransform {
    Vector3f position{};
    Quaternionf rotation = Quaternionf::identity();
    Vector3f localScale{1.0f, 1.0f, 1.0f};
    Vector3f TransformPoint(const Vector3f& p) const;
    Vector3f InverseTransformPoint(const Vector3f& p) const;
    Vector3f TransformDirection(const Vector3f& d) const;
    Vector3f InverseTransformDirection(const Vector3f& d) const;
    Vector3f up() const;       // rotation * Vector3.up
    Vector3f forward() const;  // rotation * Vector3.forward
};

// One renderer of the part model as the UE import (or a model reader) reports it: the data Part.FindModelComponents<Renderer>
// feeds PartGeometryUtil (world-space bounds, GameObject name, skinned or not).
struct PartModelRenderer {
    std::string name;
    bool skinned = false;
    bool particleSystem = false;  // ParticleSystemRenderer (GetRendererBoundsWithoutParticles skips it)
    Bounds bounds{};
};

class Part : public PartPrefab {
public:
    Part();
    ~Part();
    Part(const Part&) = delete;
    Part& operator=(const Part&) = delete;

    // ---- statics (Part.cs:67-100) ---------------------------------------------------------------------------------------
    static inline std::vector<Part*> allParts;    // :105 Part.allParts
    static inline uint32_t fuelRequestID = 0;     // Part.fuelRequestID
    static inline bool AlwaysRecheckShielding = true;
    static int NewRequestID();                    // :6284
    // What Part.cs reads from / calls on its Vessel (WP-14 installs the real ones; unset = the answer for "no vessel")
    struct VesselHooks {
        std::function<bool(const Vessel&)> isEVA;                                           // vessel.isEVA
        std::function<int(const Vessel&)> partCount;                                        // vessel.parts.Count
        std::function<void(Vessel&, const std::shared_ptr<ProtoCrewMember>&)> removeCrew;   // vessel.RemoveCrew
        std::function<void(Vessel&)> crewListSetDirty;                                      // vessel.CrewListSetDirty
        // !vessel.loaded || vessel.isUnloading || vessel.isBackingUp (ProtoPartModuleSnapshot saves its module at once)
        std::function<bool(const Vessel&)> snapshotSavesModules;
    };
    static inline VesselHooks vesselHooks;
    // PartLoader.getPartInfoByName (RelinkPrefab); unset: the partInfo link is kept
    static inline std::function<const AvailablePart*(const std::string&)> relinkPartInfo;
    // FindModelTransform(name) -> (world position, forward) of a model transform (UpdateAttachNodes / UpdateSrfAttachNode)
    std::function<std::optional<std::pair<Vector3f, Vector3f>>(const std::string&)> nodeTransformResolver;
    static uint32_t getFuelReqId();               // :4674

    // ---- public fields that need storage beyond PartFields (Part.cs:107-700) ----------------------------------------------
    Vessel* vessel = nullptr;                      // :107
    ShipConstruct* ship = nullptr;                 // :109
    std::vector<Part*> editorLinks;                // :111
    std::vector<Part*> symmetryCounterparts;       // :119
    Part* originalPart = nullptr;                  // :123
    Part* potentialParent = nullptr;               // :129
    ProtoPartSnapshot* protoPartSnapshot = nullptr;  // :133
    const AvailablePart* partInfo = nullptr;       // :135
    ModulePartVariants* variants = nullptr;        // :137 (set by PartLoader.CompilePartInfo)
    Part* parent = nullptr;                        // :154
    std::vector<Part*> fuelLookupTargets;          // :156
    std::vector<Part*> children;                   // :158
    PartTransform partTransform;                   // :160 (and Component.transform)
    std::vector<std::shared_ptr<ProtoCrewMember>> protoModuleCrew;  // :231
    std::vector<PartModule*> dockingPorts;         // :259
    std::shared_ptr<VesselNaming> vesselNaming;    // :845
    std::vector<Part_ForceHolder> forces;          // :839 (force / torque are PartFields)
    std::vector<std::string> fxGroupNames;         // fxGroups (FXGroup names: prelaunch, activate, active, deactivate, ...)
    // Part.cs:555-563 Callback fields
    Delegate<void()> OnJustAboutToDie, OnJustAboutToBeDestroyed, OnEditorAttach, OnEditorDetach, OnEditorDestroy;
    bool hasRigidbody = false;                     // `rb != null` (the Rigidbody itself is the physics engine's)
    // renderers of the model (PartGeometryUtil input); empty without a model reader
    std::vector<PartModelRenderer> modelRenderers;
    std::vector<Bounds> colliderBounds;            // FindModelComponents<Collider> bounds

    // ---- private / protected / internal members of Part.cs --------------------------------------------------------------
    float moduleMass = 0.0f;                       // :306 internal
    float inventoryMass = 0.0f;                    // :308
    float kerbalMass = 0.0f, kerbalInventoryMass = 0.0f, kerbalResourceMass = 0.0f;  // :310-314
    double partWeight = 0.0;                       // :580 internal
    Vector3f inertiaTensor{};                      // :446 private
    PartValues partValues;                         // :552 private; PartValues property

    // ---- properties -------------------------------------------------------------------------------------------------------
    bool isVesselEVA() const;                      // :903
    Part* localRoot();                             // :930
    PartStates State() const { return state_; }    // :942
    void set_State(PartStates value);              // :946
    bool hasStagingIcon() const;                   // :967 stagingIcon != string.Empty
    bool isAttached() const { return attached_; }  // :972
    void set_isAttached(bool v) { attached_ = v; }
    bool isCompund() const { return compund_; }    // :984
    bool isAttachable() const;                     // :989
    bool CanFail() const;                          // :1056 (MakingHistory installed && _canFail)
    void set_CanFail(bool v);
    DragCubeList& GetDragCubes() { return DragCubes; }  // :1086 DragCubes property
    const std::string& ClassName() const { return className; }  // :1115 PartAttributes.className
    int ClassID() const;                           // :1119 className.GetHashCode()
    PartModuleList& Modules() { return modules_; } // :1143 (C#: Modules property)
    const PartModuleList& Modules() const { return modules_; }
    std::vector<PartResource>& ResourceList() { return Resources; }  // :1155 Resources (PartResourceList is WP-15)
    std::vector<PartResource>& SimulationResources() { return simulationResources_; }  // :1157
    Part* RigidBodyPart();                         // :1236
    bool ShieldedFromAirstream() const { return partIsShielded_; }  // :1171
    void set_ShieldedFromAirstream(bool v);        // :1171 (also updates isShieldedDisplay)
    // GetReferenceTransform().name / SetReferenceTransform (:1547, :1567): the model transform is the model reader's; the
    // part's own transform is named like the GameObject
    const std::string& GetReferenceTransformName() const { return referenceTransformName_.empty() ? gameObjectName : referenceTransformName_; }
    void SetReferenceTransformName(const std::string& name) { referenceTransformName_ = name; }
    // The part's own EVENTS / ACTIONS / EFFECTS nodes as ConfigurePart loaded them (Part.Events / Actions are not modelled as
    // BaseEventLists: their node is kept and written back as loaded)
    ConfigNode::NodePtr partEventsNode, partActionsNode;

    // ---- symmetry and type queries (:1227-1522) ---------------------------------------------------------------------------
    Part* getSymmetryCounterPart(int index);       // :1227
    bool isSymmetryCounterPart(const Part* cPart) const;  // :1237
    bool isKerbalEVA();                            // :1254 HasModuleImplementing<KerbalEVA>()
    bool isCargoPart();                            // :1259
    bool isInventoryPart();                        // :1264
    bool isGroundDeployable();                     // :1269
    bool isLaunchClamp();                          // :1274
    bool isDecoupler();                            // :1285 (ModuleDecouple)
    bool isAnchoredDecoupler();                    // :1279
    bool isDockingPort();                          // :1291
    bool isAirIntake();                            // :1297
    bool isSolarPanel();                           // :1307
    bool isRadiator();                             // :1313
    bool isAntenna();                              // :1319
    bool isGenerator();                            // :1325
    bool isFairing();                              // :1331
    bool isEngine();                               // :1336
    bool isParachute();                            // :1351
    bool isRobotic();                              // :1356
    bool isControlSurface();                       // :1458
    bool isKerbalSeat();                           // :1498
    const std::string* findFxGroup(const std::string& groupID) const;  // :1525 (FXGroup name only)

    // ---- lifecycle (:1593-1663) -------------------------------------------------------------------------------------------
    void OnLoad();                                 // :1593 (fxGroups prelaunch / activate / active / deactivate)
    void RelinkPrefab();                           // :1603
    void Awake();                                  // :1615 (engine-agnostic part: modules list, resources, partName, flags)
    // The data block of Start() (:1663): skinMaxTemp / absorptiveConstant fallbacks, PhysicsSignificance ->
    // physicalSignificance, and in flight the thermal mass (resourceMass, thermalMass, thermalMassReciprocal, 288 K
    // initial temperatures). `flight` selects the flight-scene block.
    struct StartContext {
        bool flight = false;
        bool editor = false;
        double standardSpecificHeatCapacity = 0.0;  // PhysicsGlobals.StandardSpecificHeatCapacity
        bool isRootOfVessel = false;                // vessel.rootPart == this
        // FlightGlobals.getExternalTemperature(altitude, mainBody, ...) at the part's position (the part temperature starts
        // at (float)that when it is still negative); unset: the field is left alone
        std::optional<double> externalTemperature;
    };
    void StartData(const StartContext& ctx);

    // ---- copy (:2102-2195) ------------------------------------------------------------------------------------------------
    // UnityEngine.Object.Instantiate(original): the serialized fields, attach nodes (cloned, owner = the new part), drag
    // cubes, resources and one module per prefab module (serialized KSPField values copied except private ones), each
    // Awake'd; then Awake of the part itself. The result is parentless.
    static std::unique_ptr<Part> Instantiate(const Part& original);
    void OnWillBeCopied(bool asSymCounterpart);                // :2102
    void OnWasCopied(Part* newPart, bool asSymCounterpart);    // :2115
    void OnCopy(Part* original, bool asSymCounterpart);        // :2138
    bool OnWillBeMirrored(Quaternionf& rotation, AttachNode* selPartNode, Part* partParent);  // :2187

    // ---- crew (:2260-2370) ------------------------------------------------------------------------------------------------
    bool AddCrewmember(const std::shared_ptr<ProtoCrewMember>& crew);              // :2279
    bool AddCrewmemberAt(const std::shared_ptr<ProtoCrewMember>& crew, int seatIndex);  // :2310
    void RemoveCrewmember(const std::shared_ptr<ProtoCrewMember>& crew);          // :2260

    // ---- tree (:2437-2540, 2888-3150, 6949) ---------------------------------------------------------------------------------
    void setParent(Part* p = nullptr);             // :2437
    void clearParent();                            // :2454
    void addChild(Part* child);                    // :2461
    void removeChild(Part* child);                 // :2467
    bool hasIndirectChild(const Part* tgtPart) const;   // :2484
    bool hasIndirectParent(const Part* tgtPart) const;  // :2508 (sic: recurses with its own parent)
    void onAttach(Part* parent, bool first = true);     // :2521 (data part)
    void FindNonPhysicslessChildren(std::vector<Part*>& parts);  // :2888
    Part* FindNonPhysicslessParent();              // :2907
    void PromoteToPhysicalPart();                  // :2920 (state change; Rigidbody / joints are the physics engine's)
    void DemoteToPhysicslessPart();                // :2990
    float GetPhysicslessChildMass();               // :3027
    AttachNode* FindPartThroughNodes(Part* tgtPart, Part* src = nullptr);  // :3043
    void CleanSymmetryReferences();                // :3113
    void CleanSymmetryVesselReferences();          // :3129
    void CleanSymmetryVesselReferencesRecursively();  // :3101
    void SetHierarchyRoot(Part* root);             // :3214
    void UpdateOrgPosAndRot(Part* newRoot);        // :3257
    static void GetPartsOutTo(Part* part, std::vector<Part*>& parts, int maxLinks);  // :6949 (HashSet: insertion-ordered)
    Part* FindChildPart(const std::string& childName, bool recursive = false);  // :4910 (gameObject.name)
    static Part* FindChildPart(Part* parent, const std::string& childName);     // :4941
    void RemoveFromSymmetry();                     // :8848 (data part)
    bool PartCanBeDetached();                      // :8960
    bool PartCanBeOffset();                        // :8978
    bool PartCanBeRotated();                       // :9001

    // ---- modules (:5515-6006) ----------------------------------------------------------------------------------------------
    // Part.AddModule(string): class lookup, Cargo / Inventory exclusivity, AddComponent (constructor chain), optional Awake.
    // Returns the record (nullptr when refused); the module object is record->module.
    PartModuleData* AddModule(std::string_view moduleName, bool forceAwake = false);       // :5787
    PartModuleData* AddModule(ConfigNode& node, bool forceAwake = false);                  // :5823
    PartModule* LoadModule(ConfigNode& node, int& moduleIndex);                            // :5843
    void RemoveModule(PartModule* module);                                                 // :5901
    void RemoveModules();                                                                  // :5913
    void ClearModuleReferenceCache() {}                                                    // :5918 (no cache: searches are linear)
    // FindModuleImplementing<T>(): `is` = the C# `is T` test on a module
    PartModule* FindModuleImplementing(const std::function<bool(PartModule&)>& is);        // :5930
    PartModule* FindModuleOfClass(std::string_view className);                             // FindModuleImplementing<ModuleX>()
    bool HasModuleImplementing(const std::function<bool(PartModule&)>& is);                // :5957
    PartModule* FindParentModuleImplementing(const std::function<bool(PartModule&)>& is);  // :5962
    std::vector<PartModule*> FindModulesImplementing(const std::function<bool(PartModule&)>& is);  // :5976
    // HighLogic.LoadedSceneIsEditor / LoadedSceneIsFlight and Vessel.situation (nullopt: no vessel) are passed in
    StartState GetModuleStartState(bool editor, bool flight, std::optional<VesselSituations> situation) const;  // :5569
    void ModulesOnActivate();                      // :5534
    void ModulesOnDeactivate();                    // :5553
    void ModulesOnStart(StartState state);          // :5615 (mass update included)
    void ModulesOnStartFinished(StartState state);  // :5674
    void ModulesBeforePartAttachJoint(StartState state);  // :5694
    void ModulesOnFixedUpdate();                   // :5712
    void ModulesOnUpdate();                        // :5751
    void InitializeModules();                      // :5770
    // PartLoader hooks: the record list parallels the module list (PartModuleData::module is the module object)
    std::vector<PartModuleData>& ModuleRecords() { return ModuleData; }

    // ---- attach nodes (:6006-6160) ------------------------------------------------------------------------------------------
    // Part.AddAttachNode(ConfigNode) with the model transform resolved by `findTransform` (position, forward); the
    // default resolver is absent (a NODE{} node needs the model: kspcore records the transform name).
    AttachNode* AddAttachNode(const ConfigNode& node,
                              const std::function<std::optional<std::pair<Vector3f, Vector3f>>(const std::string&)>& findTransform);  // :6006
    void UpdateAttachNodes();                      // :6054
    void UpdateSrfAttachNode();                    // :6068
    void SetupAttachNodes();                       // :6077
    AttachNode* FindAttachNode(std::string_view nodeId);                   // :6091
    std::vector<AttachNode*> FindAttachNodes(std::string_view partialNodeId);  // :6113 (empty = null)
    AttachNode* FindAttachNodeByPart(const Part* connectedPart);           // :6132
    AttachNode* TopNode() { return FindAttachNode("top"); }                // topNode (:671; SetupAttachNodes / Start)
    void SetMirror(const Vector3f& mirrorVector);  // :7001

    // ---- model component lists (:5353-5426): copies of the renderer lists the model reader supplied (the C# cache returns
    // `new List<>(cache)`, so callers may mutate their copy)
    std::vector<PartModelRenderer> FindModelMeshRenderersCached() const;           // :5353 (MeshRenderers: not skinned)
    std::vector<PartModelRenderer> FindModelSkinnedMeshRenderersCached() const;    // :5378
    std::vector<PartModelRenderer> FindModelRenderersCached() const;               // :5403 (every renderer)

    // ---- effects, events (:6162-6300) --------------------------------------------------------------------------------------
    void LoadEffects(const ConfigNode& node);      // :6175 (EFFECTS nodes are kept: EffectList is Unity FX)
    void SaveEffects(ConfigNode& node) const;      // :6180
    // Part.SendEvent: the events of the part's modules, then parent / children / fuelLookupTargets / attached parts
    void SendEvent(std::string_view eventName, BaseEventDetails* data = nullptr, int maxDepth = INT32_MAX);  // :6207
    bool AlreadyProcessedRequest(int requestID);   // :6297

    // ---- resources (:6307-6400, 6542-6590) -----------------------------------------------------------------------------------
    void SetupResources();                         // :6307
    void SetupSimulationResources();               // :6315
    void ResetSimulationResources();               // :6327
    void ResetSimulation();                        // :6337
    PartResource* AddResource(const ConfigNode& node);   // :6349
    void SetResource(const ConfigNode& node);      // :6365
    bool RemoveResource(const PartResource* res);  // :6382
    bool RemoveResource(std::string_view rName);   // :6389
    bool RemoveResource(int resID);                // :6396
    PartResource* GetResource(std::string_view name);   // PartResourceList.Get(name)
    float GetResourceMass() { return GetResourceMass(false); }  // :6542
    float GetResourceMass(bool simulate);          // :6547
    float GetResourceMass(float& thermalMass);     // :6559
    float GetResourceMass(double& thermalMass);    // :6574
    // `info` of every resource (the definition lookup PartResourceLibrary does in PartResource.Load)

    // ---- mass, cost, size (:4026, 7518-7561) --------------------------------------------------------------------------------
    void UpdateMass();                             // :4026
    float GetModuleCosts(float defaultCost, ModifierStagingSituation sit = ModifierStagingSituation::CURRENT);  // :7518
    float GetModuleMass(float defaultMass, ModifierStagingSituation sit = ModifierStagingSituation::CURRENT);   // :7532
    Vector3f GetModuleSize(const Vector3f& defaultSize, ModifierStagingSituation sit = ModifierStagingSituation::CURRENT);  // :7546

    // ---- staging, priority (:7744-7865) -------------------------------------------------------------------------------------
    int GetResourcePriority();                     // :7744
    void ResetPri();                               // :7749
    void ChangeResourcePriority(int offset);       // :7763
    void UpdateStageability(bool propagate, bool iconUpdate, int stageCount = 0);  // :7777

    // ---- virtual hooks (:4513-4670, empty in Part) ----------------------------------------------------------------------------
    virtual void onBackup() {}
    virtual void onFlightStateSave(KSPParseableDictionary&) {}
    virtual void onFlightStateLoad(KSPParseableDictionary&) {}
    virtual void OnSave(ConfigNode&) {}
    virtual void OnLoad(ConfigNode&) {}
    virtual void onPartLoad() {}
    virtual void onPartAwake() {}
    virtual void onCopy(Part*, bool) {}                      // :4553
    virtual void onWillBeCopied(bool) {}                     // :4557
    virtual void onWasCopied(Part*, bool) {}                 // :4561
    virtual void onPartAttach(Part*) {}                      // :4569

    // ---- ids -----------------------------------------------------------------------------------------------------------------
    uint32_t instanceID = 0;                       // Object.GetInstanceID() (craftID = (uint)GetInstanceID())

    // the Unity-visible name (gameObject.name == partName of the prefab) is PartPrefab::gameObjectName

private:
    PartStates state_ = PartStates::IDLE;          // :148 protected
    bool connected_ = false;                       // :208
    bool attached_ = false;                        // :212
    bool compund_ = false;                         // :214
    bool canFail_ = true;                          // :414
    int lastRequestID_ = 0;                        // :676
    static inline int ResourceRequestID_ = 0;      // :678
    PartModuleList modules_;                       // :640 (PartModuleList)
    std::vector<PartResource> simulationResources_;
    bool awoken_ = false;
    bool partIsShielded_ = false;                  // :682 private
    std::string referenceTransformName_;
    bool resourcesSetUp_ = false;
    bool simulationSetUp_ = false;
    bool stageIconCreated_ = false;  // stackIcon.StageIcon != null (the icon itself is UI)
    void CreateEventListRecursive(int eventID, int requestID, std::vector<BaseEvent*>& eventList, int depth, int maxDepth);  // :6234
    std::vector<ConfigNode::NodePtr> effectNodes_;
    static inline uint32_t nextInstanceID_ = 1;

    friend class PartLoader;
};

}  // namespace ksp
