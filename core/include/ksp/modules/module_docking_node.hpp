// WP-25: ModuleDockingNode.cs (the docking port module): KSPFields (schema driven, module_members.inc), OnAwake / OnLoad / OnSave / OnStart,
// the KerbalFSM (SetupFSM: states Ready / Acquire / Acquire (dockee) / Docked (dockee) / Docked (docker) / Docked (same vessel) / Disengage /
// Disabled / PreAttached and their events), node matching (nodeType lists, gendered ports, snapRotation), CheckDockContact (Mono float),
// DockToVessel / Undock / Decouple through the WP-16 Part.Couple / Undock / decouple, the crossfeed events, the rotation (servo) maths and the
// ITargetable / IStageSeparator / IResourceConsumer face.
//
// Seams (DockingEnvironment, each documented): the node / control / rotation Transforms (the model's), the Rigidbody (velocity, AddForceAtPosition,
// AddTorque: the physics engine's, WP-20), the PartJoint work (PartJoint.Create / DestroyJoint, the rotation ConfigurableJoint: WP-20), the
// FlightInputHandler / FlightGlobals focus switching, the PAW (UIPartActionWindow) and ScreenMessages. Everything the C# decides from numbers or
// from the part / vessel graph is ported. Lifecycle (explicit calls in Unity's order): AddComponent -> Awake -> Load, then OnStart(state);
// per frame the host calls Update() (fsm.UpdateFSM), FixedUpdate() (fsm.FixedUpdateFSM) and LateUpdate(), and ModuleDockingNode::TickAll()
// once per frame for the coroutines (lateFSMStart, WaitAndSwitchFocus).
//
// Not ported (no engine / other package): Missions adjusters (AdjusterDockingNodeBase: IsAdjusterBlockingUndock is false), the Debug.DrawLine
// calls, EditorLogic part iteration of ModifyLocked / CycleAutoStrut (AutoStrut is WP-20's), FlightGlobals.PersistentLoadedPartIds bookkeeping of
// DockToVessel (no such registry), the PAW title / PartActionUI refresh.
#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "ksp/fsm/kerbal_fsm.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/orbit/targetable.hpp"
#include "ksp/parts/docked_vessel_info.hpp"
#include "ksp/resources/resource_types.hpp"
#include "ksp/staging/staging_types.hpp"
#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/transform.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

class Part;
class Vessel;
struct AttachNode;
class ModuleAnimateGeneric;
class UIPartActionWindow;

class ModuleDockingNode;

// The world pose of a Transform as the docking maths reads it (Transform.position / forward / up / right)
struct DockPose {
    Vector3f position, forward{0.0f, 0.0f, 1.0f}, up{0.0f, 1.0f, 0.0f}, right{1.0f, 0.0f, 0.0f};
};

// What the docking / grapple modules read from and ask of the engine
struct DockingEnvironment {
    // Part.FindModelTransform(name): the pose of a model transform of a part; nullopt = no such transform. Unset: no model transforms.
    static inline std::function<std::optional<DockPose>(Part&, const std::string&)> modelTransformPose;
    // Rigidbody.velocity of a part (`part.rb.velocity`, ModuleDockingNode.cs:1052 / ModuleGrappleNode.cs:496). Unset: zero.
    static inline std::function<Vector3f(Part&)> partVelocity;
    // Part.AddTorque(Vector3) (Part.cs: a force holder applied by the physics step). Unset: dropped.
    static inline std::function<void(Part&, const Vector3f&)> addTorque;
    // FlightInputHandler.SetNeutralControls(). Unset: nothing.
    static inline std::function<void()> setNeutralControls;
    // PartJoint.Create(part, node.part, referenceNode, node.referenceNode, node.part.attachMode) / DestroyJoint (WP-20). The handle is opaque;
    // unset: a non-null marker so the "joint exists" tests behave (the joint itself is the physics engine's).
    static inline std::function<std::shared_ptr<void>(ModuleDockingNode&, ModuleDockingNode&)> createSameVesselJoint;
    static inline std::function<void(ModuleDockingNode&, std::shared_ptr<void>&)> destroySameVesselJoint;
    // `part.attachJoint.Joint` of a part (the ConfigurableJoint of its PartJoint, WP-20): an opaque identity, nullptr when the part has no attachJoint
    // (or no Joint). Unset: nullptr.
    static inline std::function<const void*(Part&)> attachJointOf;
    // The writes on that ConfigurableJoint (SetJointHighLowLimits, UpdateAlignmentRotation, OnStartFinished)
    static inline std::function<void(const void* joint, const Quaternionf&)> setTargetRotationLocal;       // joint.SetTargetRotationLocal(q, Quaternion.identity)
    static inline std::function<void(const void* joint)> limitAngularXMotion;                                // joint.angularXMotion = ConfigurableJointMotion.Limited
    static inline std::function<void(const void* joint, float lowerLimit, float upperLimit)> setAngularXLimits;  // low / highAngularXLimit.limit
    static inline std::function<void(const void* joint, float factor, bool divide)> scaleBreakForces;      // breakTorque / breakForce *= factor or /= factor
    // rotationTransform.localRotation.eulerAngles (get) and the write `localRotation = Quaternion.Euler(initialEuler) * delta` (the native
    // Euler / product are the engine's; `delta` is the AngleAxis part SetTargetRotation built)
    static inline std::function<Vector3f(ModuleDockingNode&)> rotationTransformEuler;
    static inline std::function<void(ModuleDockingNode&, const Vector3f& initialEuler, const Quaternionf& delta)> setRotationTransformLocalRotation;
    // ScreenMessages.PostScreenMessage texts
    static inline std::vector<std::string> screenMessages;
    // GameEvents / HighLogic scene flags: the SaveEnvironment scene (see ResourceFlowEnvironment)
    static bool SceneIsFlight();
    static bool SceneIsEditor();
    static inline std::function<bool()> sceneIsMissionBuilder;  // HighLogic.LoadedSceneIsMissionBuilder; unset: false
    // FlightGlobals.fetch.VesselTarget / SetVesselTarget(ITargetable*) (the target handling in Update); unset: no target
    static inline std::function<ITargetable*()> vesselTarget;
    static inline std::function<void(ITargetable*)> setVesselTarget;
    // Time.fixedDeltaTime
    static inline float fixedDeltaTime = 0.02f;
};

class ModuleDockingNode : public PartModule, public ITargetable, public IStageSeparator, public IResourceConsumer {
public:
    explicit ModuleDockingNode(const ModuleClassInfo& cls);
    ~ModuleDockingNode() override;

    // ---- non-KSPField public state (ModuleDockingNode.cs:200-318) ----
    std::optional<DockedVesselInfo> vesselInfo;  // :200
    std::string state = "Ready";                 // :202
    UnityEngine::Transform nodeTransformMirror;  // what GetTransform() hands out (the node transform's pose, refreshed on demand)
    bool nodeTransformPresent = false;           // `nodeTransform != null`
    std::string controlTransformName_;           // the name of the Transform controlTransform resolved to ("" = the part's own transform)
    double TatUndock = 0.0;                      // :208
    uint32_t dockedPartUId = 0;                  // :210
    int dockingNodeModuleIndex = 0;              // :212
    ModuleDockingNode* otherNode = nullptr;      // :214
    std::set<std::string> nodeTypes;             // :216 (HashSet<string>)
    ModuleAnimateGeneric* deployAnimator = nullptr;  // :218
    AttachNode* referenceNode = nullptr;         // :220
    bool setStagingState = true;                 // :222 protected
    std::shared_ptr<void> sameVesselDockJoint;   // :234 PartJoint
    bool DebugFSMState = false;                  // :230
    std::unique_ptr<KerbalFSM> fsm;              // :238
    // :240-:280 states and events (owned here; created by SetupFSM)
    std::unique_ptr<KFSMState> st_ready, st_acquire, st_acquire_dockee, st_docked_dockee, st_docked_docker, st_docker_sameVessel, st_disengage,
        st_disabled, st_preattached;
    std::unique_ptr<KFSMEvent> on_nodeApproach, on_nodeDistance, on_capture, on_capture_dockee, on_capture_docker, on_capture_docker_sameVessel,
        on_undock, on_sameVessel_disconnect, on_disable, on_enable, on_decouple, on_preattachedDecouple, on_swapPrimary, on_swapSecondary,
        on_construction_Attach, on_construction_Detach;
    float maxAnglePerFrame = 0.0f;               // :284
    float driveTargetAngle = 0.0f;               // :286
    float visualTargetAngle = 0.0f;              // :288
    bool IsRotating = false;                     // :298 { get; set; }

    // ---- properties (:300-:441) ----
    float JointTargetAngle();                    // :297
    float VisualTargetAngle();                   // :321 (private)
    const void* RotationJoint();                 // :346 (the ConfigurableJoint identity from the seam, nullptr = null)
    bool IsDisabled();                           // :391
    ModuleDockingNode* GetDominantNode(ModuleDockingNode* m1, ModuleDockingNode* m2);  // :1694

    // ---- actions / events ----
    void ToggleLockedAction(KSPActionParam& param);       // :404
    void ServoEngageLockAction(KSPActionParam& param);    // :417
    void ServoDisgageLockAction(KSPActionParam& param);   // :426
    bool EngageNodeLock();                                // :431
    void DisengageNodeLock();                             // :446
    void ModifyLocked(const ModuleValue& v);              // :455 OnValueModified callback
    void MakePrimary();                                   // :1470
    void UndockAction(KSPActionParam& param);             // :1480
    void Undock();                                        // :1493
    void UndockSameVessel();                              // :1723
    void DecoupleAction(KSPActionParam& param);           // :1751
    void Decouple();                                      // :1760
    void SetAsTarget();                                   // :1860 (named per the C# method)
    void UnsetTarget();
    void EnableXFeed();
    void DisableXFeed();
    void MakeReferenceToggle(KSPActionParam& act);
    void MakeReferenceTransform();
    void EnableXFeedAction(KSPActionParam& param);
    void DisableXFeedAction(KSPActionParam& param);
    void ToggleXFeedAction(KSPActionParam& param);

    // ---- lifecycle ----
    void OnAwake() override;                              // :479
    void OnLoad(const ConfigNode& node) override;         // :510
    void OnSave(ConfigNode& node) override;               // :546
    void OnStart(StartState st) override;                 // :561
    void OnStartFinished(StartState st) override;         // :631
    void OnInitialize() override { PartModule::OnInitialize(); }  // :698
    void OnPartPack();                                    // :681 (private in C#; the host calls it from GameEvents-driven Part.Pack)
    void OnPartUnpack();                                  // :689
    void OnInventoryModeEnable();                         // :708
    void OnInventoryModeDisable();                        // :745
    void DemoteToPhysicslessPart() override;              // :759
    void PromoteToPhysicalPart() override;                // :764
    void OnActive() override;                             // :1788
    void Update();                                        // :1239
    void FixedUpdate();                                   // :1277
    void LateUpdate();                                    // :1301
    // One Unity frame for the coroutines of every live docking node (lateFSMStart, WaitAndSwitchFocus)
    static void TickAll();

    void OnPartMenuOpen(UIPartActionWindow* window, Part* inpPart);  // :769
    void OnPartMenuClose(Part* inpPart);                             // :779

    // ---- docking ----
    ModuleDockingNode* FindOtherNode();                              // :785
    void lateFSMStart(StartState st);                                // :797 (the body after its first yield)
    void SetupFSM();                                                 // :849
    ModuleDockingNode* FindNodeApproaches();                         // :1338
    // The matching expression of FindNodeApproaches (:1257-:1270): `this` approaches `other` when they share a nodeType, agree on gendered (opposite
    // genderFemale when gendered) and on snapRotation (same snapOffset when snapping)
    bool CanDockWith(ModuleDockingNode& other);
    // :1281. The pose-level kernel of CheckDockContact is DockContact() so unit tests can reach the intermediate roll dot.
    bool CheckDockContact(ModuleDockingNode* m1, ModuleDockingNode* m2, float minDist, float minFwdDot, float minRollDot);
    static bool DockContact(const DockPose& p1, const DockPose& p2, bool snapRotation, float snapOffset, float minDist, float minFwdDot, float minRollDot,
                            float* rollDot = nullptr);
    void DockToVessel(ModuleDockingNode* node);                      // :1424
    void WaitAndSwitchFocus();                                       // :1527
    void OnOtherNodeUndock();                                        // :1534
    void DockToSameVessel(ModuleDockingNode* node);                  // :1703
    void DestroySameVesselJoint();                                   // :1709
    void OnOtherNodeSameVesselDisconnect();                          // :1729
    bool NodeIsTooFar();                                             // :1734
    void OnConstructionAttach();                                     // :1796
    DockPose NodePose();                                             // nodeTransform.position / forward / up / right (the seam)
    bool CrossfeedToggleableEditor() { return true; }                // :1976
    bool CrossfeedToggleableFlight() { return true; }                // :1981
    std::string GetContractObjectiveType() { return "Dock"; }        // :1986
    bool CheckContractObjectiveValidity() { return true; }           // :1991
    bool IsJointUnlocked();                                          // :2051 (IJointLockState)

    // ---- rotation (servo) ----
    void UpdateAlignmentRotation();                                  // :1539
    Quaternionf SetTargetRotation(const Quaternionf& startingRotation, float rotationAngle, bool setRotation);  // :1592
    Quaternionf SetTargetRotation(const Quaternionf& startingRotation, float rotationAngle, bool setRotation, const Vector3f& rotationAxis);  // :1592
    Vector3f GetRotationAxis();                                      // :1602
    void CalculateResourceDrain();                                   // :1622
    void SetJointHighLowLimits();                                    // :1654
    void ApplyCoordsUpdate();                                        // :1669
    void RecurseCoordUpdate(Part* p, Part* rootPart);                // :1677
    float motorRate = 0.0f;                                          // :150 protected
    bool hasEnoughResources = true;                                  // :290 protected
    bool rotationInitComplete = false;
    float cachedInitialAngle = 0.0f;
    Vector3f initialRotation;
    Quaternionf targetRotation = Quaternionf::identity();  // the AngleAxis delta applied on Quaternion.Euler(initialRotation)
    bool partJointUnbreakable = false;
    float lowerRotationLimit = 0.0f, upperRotationLimit = 0.0f;  // SoftJointLimit.limit of the two limits

    // ---- ITargetable (:1851-:1900) ----
    UnityEngine::Transform* GetTransform() override;
    Vector3f GetObtVelocity() override;
    Vector3f GetSrfVelocity() override;
    Vector3f GetFwdVector() override;
    Vessel* GetVessel() override;
    std::string GetName() override;
    std::string GetDisplayName() override;
    Orbit* GetOrbit() override;
    OrbitDriver* GetOrbitDriver() override;
    VesselTargetModes GetTargetingMode() override { return VesselTargetModes::DirectionVelocityAndOrientation; }
    bool GetActiveTargetable() override { return false; }
    // ---- IResourceConsumer (:1678) ----
    std::vector<const PartResourceDefinition*> GetConsumedResources() override { return consumedResources_; }
    // ---- IStageSeparator / staging (:1944-:1974) ----
    int GetStageIndex(int fallback) override { return fallback + 1; }
    bool IsStageable() override;
    bool StagingEnabled() override;
    bool StagingToggleEnabledEditor() override;
    bool StagingToggleEnabledFlight() override;
    std::string GetStagingEnableText() override;
    std::string GetStagingDisableText() override;
    std::string GetInfo() override;                                  // :1939
    std::string GetModuleDisplayName() override;                     // :2044

    Part* PartOf() const;
    IStageSeparator* AsStageSeparator() { return this; }

private:
    struct Delayed {
        int ticks;
        std::function<void()> fn;
    };
    void StartCoroutine(int ticks, std::function<void()> fn) { delayed_.push_back({ticks, std::move(fn)}); }
    void UpdatePAWUI();                                              // :1316
    bool IsAdjusterBlockingUndock() { return false; }               // :2030 (no Missions adjusters)
    void OnFSMStateChange(KFSMState* oldState, KFSMState* newState, KFSMEvent* fsmEvent);  // :1223
    void OnFSMEventCalled(KFSMEvent* fsmEvent);                      // :1231
    void PositionAdjustments();

    ModuleDockingNode* sameVesselUndockNode = nullptr;               // :224
    ModuleDockingNode* sameVesselUndockOtherNode = nullptr;
    bool sameVesselUndockRedock = false;
    bool partActionMenuOpen = false;
    bool undockPreAttached = false;
    bool physicsLessMode = false;
    BaseEvent* evtSetAsTarget = nullptr;
    BaseEvent* evtUnsetTarget = nullptr;
    std::vector<const PartResourceDefinition*> consumedResources_;
    bool consumedResourcesSet_ = false;
    bool subscribed_ = false;
    bool rotationTransformPresent_ = false;
    std::vector<Delayed> delayed_;
    static inline std::vector<ModuleDockingNode*> live_;
};

}  // namespace ksp
