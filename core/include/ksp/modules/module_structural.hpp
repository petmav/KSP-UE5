// WP-26: the structural PartModules: ModuleCargoBay.cs, ModuleJettison.cs, ModuleServiceModule.cs, LaunchClamp.cs,
// ModuleStructuralNode.cs, ModuleStructuralNodeToggle.cs, ModuleDynamicNodes.cs, DynamicNodeSet.cs (ModuleProceduralFairing:
// module_procedural_fairing.hpp, CompoundPart: compound_part.hpp).
//
// The schema (KSPFields, KSPEvent / KSPAction attribute data) comes from the generated module_schema.inc / module_members.inc.
// What needs the model or the physics engine goes through the `hooks` structs of each class (unset: the model has no such object,
// i.e. `FindModelTransform` finds nothing and there are no colliders), so the numeric / bookkeeping behaviour is exact without a
// renderer. Unity coroutines are state advanced by the host (`ModuleCargoBay::RunScheduledColliderReset`).
#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ksp/aero/aero_interfaces.hpp"
#include "ksp/events/event_data.hpp"
#include "ksp/modules/module_procedural_fairing.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/parts/part_types.hpp"
#include "ksp/staging/staging_types.hpp"
#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

class Part;
class Vessel;
struct AttachNode;
class ModuleJettison;
class ShipConstruct;
struct PartVariant;

// What the model gives the structural modules: transform lookups by name (Part.FindModelTransform) and their GameObject active state.
struct StructuralModelHooks {
    std::function<bool(Part&, const std::string&)> hasModelTransform;        // FindModelTransform(name) != null
    std::function<bool(Part&, const std::string&)> modelTransformActive;     // transform.gameObject.activeSelf
    std::function<void(Part&, const std::string&, bool)> setModelTransformActive;  // transform.gameObject.SetActive(value)
    std::function<Vector3f(Part&)> partColliderBoundsSize;                  // base.part.collider.bounds.size
};
inline StructuralModelHooks& StructuralModel() {
    static StructuralModelHooks h;
    return h;
}

// ModuleCargoBay.cs:11 PartCollider (the Collider is an opaque model handle)
struct CargoBayPartCollider {
    Part* owner = nullptr;
    const void* collider = nullptr;
    bool operator==(const CargoBayPartCollider& o) const { return owner == o.owner && collider == o.collider; }
};

// ModuleCargoBay.cs:8
class ModuleCargoBay : public PartModule, public IAirstreamShield {
public:
    using PartCollider = CargoBayPartCollider;
    explicit ModuleCargoBay(const ModuleClassInfo& cls) : PartModule(cls) {}
    ~ModuleCargoBay() override;

    // Physics queries of the cargo volume (Physics.OverlapSphere / Collider bounds tests): the physics package installs them.
    struct Hooks {
        std::function<std::vector<PartCollider>(Part&)> findPartColliders;  // :590 body (FindModelComponents<Collider> + tag / layer filter)
        std::function<std::vector<Part*>(ModuleCargoBay&, float radius, const Vector3f& center)> findNearbyParts;  // :437
        std::function<std::vector<Part*>(ModuleCargoBay&, const std::vector<Part*>&, const std::vector<PartCollider>&)> findEnclosedParts;  // :359
        std::function<Vector3f(Vessel&)> vesselPosition;                    // v.vesselTransform.position (:255)
        std::function<std::vector<Part*>()> editorShipParts;                // EditorLogic.fetch.ship.parts (:497)
    };
    static inline Hooks hooks;

    // state (:25-104)
    int DeployModuleIndex() const { return Get<int32_t>("DeployModuleIndex"); }
    IScalarModule* deployModule = nullptr;
    std::vector<Part*> partsInCargo, partsInCargoPrev;
    std::vector<Part*> onShdModifiedCallbacks;  // the `Part.OnShieldModified` callbacks AddShield returned, one per enclosed part
    std::vector<PartCollider> ownColliders;
    AttachNode *NodeOuterFore = nullptr, *NodeOuterAft = nullptr, *NodeInnerFore = nullptr, *NodeInnerAft = nullptr;
    ModuleProceduralFairing* fairing = nullptr;     // :84 (unused by the C# beyond the field)
    PartModule* serviceModule = nullptr;            // ModuleServiceModule
    std::vector<Part*> connectingParts;
    std::vector<ModuleCargoBay*> connectedCargoBays;
    bool colliderResetCoroutineComplete = false;
    bool shouldEnableShieldedVolume = false;
    bool enableShieldedVolumeAfterColliderReset = false;
    bool colliderResetPending = false;

    void OnAwake() override;                        // :108
    void OnStart(StartState state) override;        // :117
    void OnStartFinished(StartState state) override;  // :142
    std::string GetModuleDisplayName() override;    // :938

    void FindAttachNodes();                         // :171
    void ScheduleColliderReset();                   // :212 (the 0.5 s coroutine: RunScheduledColliderReset is its body)
    void RunScheduledColliderReset();               // :225-241
    void ColliderReset(bool enableShieldedVolume);  // :242
    void OnEditorVesselModified(ShipConstruct* ship);  // :207
    void SetupDynamicCargoOccluders(bool testActive);  // :493
    void OnVesselModified(Vessel* v);               // :253
    void OnCargoBayDoorsMoving(float from, float to);  // :284 (the deploy module's OnMoving)
    void OnCargoBayDoorsStopped(float at);          // :293 (the deploy module's OnStop)
    void OnPartDestroyed(Part* p);                  // :312
    void ColliderListCleanUp();                     // :337
    IScalarModule* FindModule(int index);           // :349
    void EnableShieldedVolume();                    // :457
    void DisableShieldedVolume();                   // :483
    std::vector<Part*> ShieldEnclosedParts(std::vector<Part*>& enclosedParts);  // :522 (returns the callbacks as owning parts)
    void UnShieldEnclosedParts(std::vector<Part*>& enclosedParts);  // :533
    void ModifyShieldInEnclosedParts(bool shieldStatus);  // :551
    void NotifyShieldModified(bool notifyConnected);  // :563
    void FindBayColliders(Part* originalPart, ModuleCargoBay* origin, std::vector<PartCollider>& cList, std::vector<ModuleCargoBay*>& cBays);  // :619
    std::vector<AttachNode*> FindConnectingNodes(Part* origin);  // :661
    AttachNode* GetOpposingNode(Part* origin);      // :686
    void AddConnectingPart(Part* p);                // :703
    void RemoveConnectingPart(Part* p);             // :712
    void ClearConnectingParts();                    // :721
    void ScheduleColliderResetForAllBays();         // :730
    void SetLookupRadius(float radius);             // :740
    void SetLookupCenter(const Vector3f& p);        // :745
    bool SelfClosedAndLocked();                     // :768
    bool EndCapped();                               // :777
    bool TestAttachmentFit(AttachNode* n);          // :804

    // IAirstreamShield
    bool ClosedAndLocked() override;                // :750
    Vessel* GetVessel() override;                   // :814
    Part* GetPart() override;                       // :819

private:
    Part* PartOf() const;
};

// ModuleJettison.cs:5
class ModuleJettison : public PartModule, public IActivateOnDecouple, public IScalarModule, public IMultipleDragCube, public IPartMassModifier {
public:
    explicit ModuleJettison(const ModuleClassInfo& cls);
    static bool IsAlive(const ModuleJettison* m);  // kspcore: C# keeps a destroyed module reachable; the engine plate asks before unsubscribing
    ~ModuleJettison() override;

    // Jettison's physical half (physicalObject.ConvertToPhysicalObject of the jettison transform, rb velocities, forces): the physics package
    struct Hooks {
        std::function<void(Part&, ModuleJettison&)> jettisonTransformToPhysical;
        // `jettisonTransform.parent != null && parent.gameObject.GetComponent<ModuleDockingNode>() != null` (:151, WP-25): the model hierarchy says
        // whether the jettison transform hangs directly under the part's root object. The module test is real (Part::isDockingPort). Unset: false.
        std::function<bool(Part&, ModuleJettison&)> jettisonTransformParentIsPartRoot;
    };
    static inline Hooks hooks;

    float forceScalar = 10.0f;                      // :75 public
    // jettisonTransform (:7, [SerializeField]): the model's transform, by name; `jettisonTransform != null` is `HasJettisonTransform()`
    bool HasJettisonTransform() const { return jettisonTransformPresent_; }
    const std::string& JettisonTransformName() const { return jettisonTransformName_; }
    events::EventData<float, float> OnMoving{"JettisonStart"};  // :84
    events::EventData<float> OnStop{"JettisonEnd"};             // :86
    float PartDensity() const { return partDensity_; }
    bool InEditor() const { return inEditor_; }

    void OnLoad(const ConfigNode& node) override;   // :238
    void OnStart(StartState state) override;        // :297
    void OnActive() override;                       // :451
    void LateUpdate();                              // :474
    void OnInventoryModeDisable();                  // :244 (SetVariants + SetupJettisonState)
    void Jettison();                                // :136 [KSPEvent]
    void JettisonAction(KSPActionParam& param);     // :130 [KSPAction]
    void CheckAttachNode();                         // :498
    void SetupJettisonState(bool isEditor);         // :344
    void SetVariants();                             // :329
    void SetFairingDragCube(bool fairingState);     // :524
    void OnVariantApplied(Part* eventPart, PartVariant* variant);  // :233
    void OnAwake() override;

    // IActivateOnDecouple (:594)
    void DecoupleAction(const std::string& nodeName, bool weDecouple) override;
    // IMultipleDragCube (:559, :564, :589)
    bool IsMultipleCubesActive() const override { return Get<bool>("useMultipleDragCubes"); }
    std::vector<std::string> GetDragCubeNames() const override { return {"Fairing", "Clean"}; }
    void AssumeDragCubePosition(const std::string& name) override;
    bool UsesProceduralDragCubes() const override { return Get<bool>("useProceduralDragCubes"); }
    // IScalarModule (:624-640)
    std::string ScalarModuleID() const override { return Get<std::string>("moduleID"); }
    float GetScalar() const override { return Get<bool>("isJettisoned") ? 1.0f : 0.0f; }
    bool CanMove() const override { return !Get<bool>("isJettisoned"); }
    void SetScalar(float) override {}
    void SetUIRead(bool) override {}
    void SetUIWrite(bool) override {}
    bool IsMoving() override { return false; }
    // IPartMassModifier (:602, :619)
    IPartMassModifier* AsMassModifier() override { return this; }
    IScalarModule* AsScalarModule() override { return this; }
    float GetModuleMass(float defaultMass, ModifierStagingSituation sit) override;
    ModifierChangeWhen GetModuleMassChangeWhen() override { return ModifierChangeWhen::STAGED; }
    // staging (:641-661)
    bool IsStageable() override;
    std::string GetStagingEnableText() override;
    std::string GetStagingDisableText() override;

private:
    Part* PartOf() const;
    bool FetchActiveMeshTransform(const std::string& nameList);  // :261: sets jettisonTransform* and activejettisonName
    void LoadJettisonMeshState();                   // :285
    void SetActionsSuffix();                        // :337
    bool jettisonTransformPresent_ = false;
    std::string jettisonTransformName_;
    bool inEditor_ = false;
    float partDensity_ = 0.0f;
    bool hasVariantDragCubes_ = false;
    bool setupComplete_ = false;
    bool variantSubscribed_ = false;
};

// ModuleServiceModule.cs:5
class ModuleServiceModule : public PartModule, public IStageSeparator, public IStageSeparatorChild, public IScalarModule {
public:
    explicit ModuleServiceModule(const ModuleClassInfo& cls) : PartModule(cls) {}
    ~ModuleServiceModule() override;

    events::EventData<float, float> OnMoving{"ServiceModuleDeployStart"};  // :37
    events::EventData<float> OnStop{"ServiceModuleDeployEnd"};             // :39
    // BoxCollider of the jettison zones for BuildJettisonPartsList (:268 PointInBox): the zone's world-to-local transform and box
    struct BoxZone {
        Vector3f center, size;
        std::function<Vector3f(const Vector3f&)> inverseTransformPoint;  // box.transform.InverseTransformPoint
    };
    struct Hooks {
        std::function<std::optional<BoxZone>(Part&, const std::string& zoneName)> findZone;  // FindModelTransform(name).GetComponent<BoxCollider>()
        std::function<Vector3f(Part&)> partWorldPosition;                                    // part.transform.position
    };
    static inline Hooks hooks;

    bool IsShrouded() const { return Get<bool>("showMesh"); }  // :41
    std::vector<ModuleJettison*> jettisonModules;               // :27 (null until SetupJettisonModules)
    bool jettisonModulesNull = true;
    std::vector<Part*> jettisonParts;

    void OnStart(StartState state) override;        // :72
    void OnActive() override;                       // :209
    void OnFieldUpdated(const std::string& fieldName);  // :77 (UI_Toggle.onFieldChanged of showMesh / openCutaway)
    void CheckCutaway(Vessel* vFrom, Vessel* vTo);  // :197
    void ToggleCutaway(bool state);                 // :95
    void ToggleShrouds();                           // :127
    void SetupJettisonModules();                    // :160
    void Setup();                                   // :171
    void ShedExternalParts();                       // :233
    void BuildJettisonPartsList();                  // :242
    static bool PointInBox(const Vector3f& pointLocal, const Vector3f& center, const Vector3f& size);  // :296
    void UpdateStaging();                           // :317
    void UpdateStaging(bool state);                 // :317
    std::string GetModuleDisplayName() override { return "ServiceModule"; }  // :384
    // IStageSeparator / IStageSeparatorChild
    int GetStageIndex(int fallback) override;       // :345
    bool PartDetaches(std::vector<Part*>& decoupledParts) override;  // :350
    bool IsEnginePlate() override { return false; } // :357
    // IScalarModule (:362-378)
    std::string ScalarModuleID() const override { return Get<std::string>("moduleID"); }
    float GetScalar() const override { return Get<bool>("IsDeployed") ? 1.0f : 0.0f; }
    bool CanMove() const override { return !Get<bool>("IsDeployed"); }
    void SetScalar(float) override {}
    void SetUIRead(bool) override {}
    void SetUIWrite(bool) override {}
    bool IsMoving() override { return false; }
    IScalarModule* AsScalarModule() override { return this; }

private:
    Part* PartOf() const;
    void ProcessJettisonModules(bool state, bool updateShrouds);  // :109 with UpdateCutaway (false) / UpdateShrouds (true)
};

// LaunchClamp.cs:5
class LaunchClamp : public PartModule, public IStageSeparator {
public:
    explicit LaunchClamp(const ModuleClassInfo& cls) : PartModule(cls) {}

    // the tower's transforms (anchor / pivot / stretch positions, collider bounds) are the model's: `towerExtension` supplies what the C#
    // reads from them when the tower is extended in flight (ExtendTower :133, a ground raycast)
    struct Hooks {
        std::function<void(Part&)> setJoint;           // SetJoint (:278, ConfigurableJoint)
        std::function<void(Part&)> unsetJoint;         // UnsetJoint (:293)
        std::function<std::optional<float>(Part&)> initialHeightFromModel;  // Vector3.Distance(anchor.position, towerStretch.position)
    };
    static inline Hooks hooks;

    float initialHeight = -1.0f;                    // :25 public
    Quaternionf towerRotation = Quaternionf::identity();  // :29 private
    bool extension_enabled = false;                 // :33 private
    bool jointSet = false;

    void EnableExtension() { extension_enabled = true; }  // :55
    void OnLoad(const ConfigNode& node) override;   // :60
    void OnSave(ConfigNode& node) override;         // :68
    void OnPutToGround(PartHeightQuery& qr);        // :95
    void OnInitialize() override;                   // :152
    void OnStart(StartState state) override;        // :157
    void OnActive() override;                       // :316
    void OnVesselPack();                            // :324
    void OnVesselUnpack();                          // :332
    void ReleaseClamp(KSPActionParam& param);       // :341 [KSPAction]
    void Release();                                 // :347 [KSPEvent]
    void OnAwake() override;
    int GetStageIndex(int fallback) override;       // :369 (the clamp separates in its own stage)

private:
    Part* PartOf() const;
};

// ModuleStructuralNode.cs:6
class ModuleStructuralNode : public PartModule, public IActivateOnDecouple {
public:
    explicit ModuleStructuralNode(const ModuleClassInfo& cls) : PartModule(cls) {}
    bool structTransformPresent = false;            // :8 structTransform != null
    void OnStart(StartState state) override;        // :29
    void LateUpdate();                              // :45
    void CheckDisplayEditor(Part* p);               // :53
    void SpawnStructure() { Set<bool>("spawnState", true); }    // :61
    void DespawnStructure() { Set<bool>("spawnState", false); } // :66
    void SetNodeState(bool showNodes);              // :71
    // :95: the visibility the structure gets (structTransform.gameObject.SetActive(flag)); recorded in `structureVisible`
    bool CheckDisplay();
    bool structureVisible = false;
    void DecoupleAction(const std::string& nodeName, bool weDecouple) override;  // :132

private:
    Part* PartOf() const;
};

// ModuleStructuralNodeToggle.cs:6
class ModuleStructuralNodeToggle : public PartModule {
public:
    explicit ModuleStructuralNodeToggle(const ModuleClassInfo& cls) : PartModule(cls) {}
    void Start();                                   // :21 (Unity Start: the host calls it once)
    void OnFieldUpdated(const std::string& fieldName);  // :36 (UI_Toggle.onFieldChanged)
    void UpdateNodes();                             // :41
    std::vector<ModuleStructuralNode*> structNodes;
    bool structNodesNull = true;
};

// DynamicNodeSet.cs:5 (a struct)
struct DynamicNodeSet {
    std::string DisplayText, MeshTransform, NodePrefix;
    int SetCount = 0;
    int Symmetry = 0;
    void Load(const ConfigNode& node);              // :16
};

// ModuleDynamicNodes.cs:4
class ModuleDynamicNodes : public PartModule {
public:
    explicit ModuleDynamicNodes(const ModuleClassInfo& cls) : PartModule(cls) {}
    ~ModuleDynamicNodes() override;
    static constexpr float NODE_RADIUS_ENABLED = 0.4f;     // :8
    static constexpr float NODE_RADIUS_DISABLED = 0.0001f;  // :9
    std::vector<DynamicNodeSet> SetList;
    bool setListNull = true;

    void OnAwake() override;                        // :126
    void OnLoad(const ConfigNode& node) override;   // :134
    void OnStart(StartState state) override;        // :63
    void OnStartFinished(StartState state) override;  // :70
    void FixedUpdate();                             // :155 (editor)
    void ChangeNodeSet(int newIndex);               // :25
    void ToggleNodeSet(int idx, bool showNodes);    // :35
    void OnShipModified(ShipConstruct* ship);       // :78
    void SetupAutoStrut();                          // :83
    void InitializeNodeSets();                      // :106
    std::vector<std::string> stateNames;            // the UI_Cycle of setIndex

private:
    Part* PartOf() const;
    bool subscribed_ = false;
};

}  // namespace ksp

namespace ksp::detail {
void RegisterWp26Modules();  // called by RegisterBuiltinModules (module_behaviours.cpp)
}
