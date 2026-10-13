// WP-52: BaseServo.cs (Assembly-CSharp/Expansions/Serenity) and the world the Breaking Ground servos reach (ServoEnvironment).
//
// BaseServo is the abstract PartModule of ModuleRoboticServoHinge / Piston / Rotor / RotationServo (servo_modules.hpp). It owns the motor
// (engaged / locked / braking state, motor size and limit, the resource draw), the rate-of-motion average (20 readings), the lock logic, the axis
// field limits the controller uses (IAxisFieldLimits), the mass / cost modifiers, the joint lock state the part joint rules read (IJointLockState:
// Part.HasFreePivot) and the construction vetoes (IConstruction). Persistent state and defaults are the module schema (ksp/parts/module_schema.inc);
// every [KSPField] is read and written with Get<T> / Set<T>.
//
// The Unity side is a seam, see servo_joint.hpp for the decision: the ConfigurableJoint is ConfigurableJointData, the servo Rigidbody a ServoBody,
// the GameObjects the module touches are ServoNodes (ServoRig, filled by the host from the part model; the C# `gameObject.GetChild(name)`).
//
// Unity messages (FixedUpdate / Update / LateUpdate / Start / OnDestroy) are explicit calls of the host, as for the other ported modules; the
// PartModule hooks (OnStart, OnUpdate, OnLoad, ...) are called by the part host. NOTE: BaseServo declares `protected new abstract void
// OnFixedUpdate()`, which HIDES PartModule.OnFixedUpdate: the game never calls the servo's from the part, only from its own FixedUpdate. The C++
// name of that abstract method is ServoFixedUpdate().
//
// Not ported (UI / PAW / editor-GUI / FX): UpdatePAWUI and every UI_* slider (min / max / step / visibility), GetInfo / GetModuleDisplayName text,
// Localizer-cached strings, the editor GameObject re-parenting of SetChildParentTransform, Debug.Log. Listed in STATUS.md (WP-52).
#pragma once

#include <cfloat>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "ksp/core/coroutine.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/parts/part_data.hpp"
#include "ksp/parts/part_types.hpp"
#include "ksp/resources/resource_types.hpp"
#include "ksp/serenity/serenity_misc.hpp"
#include "ksp/serenity/servo_joint.hpp"
#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"
#include "ksp/util/collections.hpp"

namespace ksp {

class Part;
class Vessel;
class CompoundPart;
struct KSPActionParam;
namespace physics {
struct PartJointSpec;
}

// IJointLockState.cs: `bool IsJointUnlocked()` (Part.HasFreePivot, Part.cs:8511, asks every module that implements it)
class IJointLockState {
public:
    virtual ~IJointLockState() = default;
    virtual bool IsJointUnlocked() = 0;
};

namespace Expansions::Serenity {

// One Transform of the part model the servo touches (the moving transform, the base, the joint parent, an attach node's transform). Plain local
// data; `localToWorld` is the host's world matrix of the node (servoParentTransform needs the joint parent's). UNVERIFIED: the host fills it from
// the .mu hierarchy (not in the repo).
struct ServoNode {
    std::string name;
    Vector3f localPosition{0, 0, 0};
    Quaternionf localRotation = Quaternionf::identity();
    Vector3f localScale{1, 1, 1};
    std::shared_ptr<ServoNode> parent;
    Matrix4x4f localToWorld = Matrix4x4f::identity();
    Quaternionf worldRotation = Quaternionf::identity();  // transform.rotation (CheckSymmetry's TransformDirection)
    Vector3f worldPosition{0, 0, 0};
    bool active = true;                                   // GameObject.SetActive
    // Transform.TransformDirection (rotation only), Transform.localEulerAngles (native Quaternion.eulerAngles of the local rotation)
    Vector3f TransformDirection(const Vector3f& d) const { return worldRotation * d; }
    Vector3f localEulerAngles() const;
};

// UnityEngine.Rigidbody as BaseServo.CreateServoRigidbody sets it up
struct ServoBody {
    bool isKinematic = false;
    bool useGravity = true;
    float maxAngularVelocity = 7.0f;
    float mass = 1.0f;
    Vector3f centerOfMass{0, 0, 0};
    Quaternionf inertiaTensorRotation = Quaternionf::identity();
    int constraints = 0;  // RigidbodyConstraints flags: FreezeRotationX = 16, Y = 32, Z = 64
};
constexpr int kFreezeRotationX = 16, kFreezeRotationY = 32, kFreezeRotationZ = 64;

// What the module reads of the part model: the named transforms (`base.gameObject.GetChild(name)` / `part.FindModelTransform(name)`)
struct ServoRig {
    std::map<std::string, std::shared_ptr<ServoNode>> nodes;
    Matrix4x4f partWorldToLocal = Matrix4x4f::identity();  // part.gameObject.transform.worldToLocalMatrix
    Vector3f partLocalPosition{0, 0, 0};                    // part.transform.localPosition / localRotation (the editor's moveBase of the piston)
    Quaternionf partLocalRotation = Quaternionf::identity();
    Vector3f modulePosition{0, 0, 0};                       // base.transform.position (CheckSymmetry)
    std::shared_ptr<ServoNode> Find(const std::string& name) const {
        auto it = nodes.find(name);
        return it == nodes.end() ? nullptr : it->second;
    }
};

// Everything the servo code reaches outside the part module (each member is a seam; "unset" says what happens then)
struct ServoEnvironment {
    static inline CoroutineScheduler* scheduler = nullptr;  // MonoBehaviour.StartCoroutine host (the editor WaitAndVisualizeServo coroutine)
    // HighLogic.LoadedSceneIsFlight / LoadedSceneIsEditor / LoadedSceneIsMissionBuilder; unset: SaveEnvironment::Current().loadedScene
    static inline std::function<bool()> sceneIsFlight, sceneIsEditor, sceneIsMissionBuilder;
    static bool SceneIsFlight();
    static bool SceneIsEditor();
    // FlightDriver.Pause (unset: false)
    static inline std::function<bool()> flightDriverPause;
    // ScreenMessages.PostScreenMessage texts, in order (the Localizer.Format key is stored, as the UI would resolve it)
    static inline std::vector<std::string> screenMessages;
    // Vessel.CycleAllAutoStrut / Part.CycleAutoStrut (PhysicsWorld: VesselPhysics::CycleAllAutoStrut); counted when unset
    static inline std::function<void(Vessel&)> cycleAllAutoStrut;
    static inline std::function<void(Part&)> cycleAutoStrut;
    static inline int cycleAllAutoStrutCalls = 0, cycleAutoStrutCalls = 0;
    // The attach joint of a part (part.attachJoint.Joint): InitJoint's `servoJoint = pJoint.Joint` for a servo with no moving transform. Unset: none.
    static inline std::function<std::shared_ptr<ConfigurableJointData>(Part&)> attachJointData;
    // Part.PartToVesselSpacePos(pos, child, vessel, PartSpaceMode.Pristine) for InitJoint's `pivot` (a value nothing else reads). Unset: pos itself.
    static inline std::function<Vector3f(Part& child, const Vector3f& anchor)> partToVesselSpacePos;
    // EditorLogic.fetch.ship.parts (the part order of the ship being edited); unset: the part's own tree from its localRoot, depth first
    static inline std::function<std::vector<Part*>()> editorShipParts;
    // ExpansionsLoader.IsExpansionInstalled("Serenity") (BaseServo.Start): unset = SaveEnvironment
    static inline std::function<bool()> serenityInstalled;
    // Planetarium.GetUniversalTime() (BaseServo.Update's stopped-moving timer); unset: Planetarium::GetUniversalTime()
    static inline std::function<double()> universalTime;
    // vessel.resourcePartSet.ContainsPart(part) (OnResourceNonemptyEmpty of the four servos). Unset: true (the part is on the vessel's resource set)
    static inline std::function<bool(Vessel&, Part&)> resourcePartSetContains;
    static void Reset();
    static void PostScreenMessage(std::string text) { screenMessages.push_back(std::move(text)); }
};

// BaseServo.cs:10
class BaseServo : public PartModule,
                  public IAxisFieldLimits,
                  public IJointLockState,
                  public IPartMassModifier,
                  public IPartCostModifier,
                  public IConstruction,
                  public IResourceConsumer {
public:
    enum class ResourceConsumptionTypes : int { VelocityLimit = 0, CurrentVelocity = 1 };  // :12

    explicit BaseServo(const ModuleClassInfo& cls);
    ~BaseServo() override;

    // ---- the Unity seam ----
    ServoRig rig;                                         // the part model's transforms
    std::shared_ptr<ServoNode> movingPartObject;          // :165 (null: not found)
    std::shared_ptr<ServoNode> basePartObject;            // :162
    std::shared_ptr<ServoNode> jointParent;               // :32
    std::shared_ptr<ServoBody> movingPartRB;              // :174
    std::shared_ptr<ConfigurableJointData> servoJoint;    // :150 (null: no joint)
    std::map<std::string, std::shared_ptr<ServoNode>> attachNodeTransforms;  // AttachNode.nodeTransform of the servo's nodes, by node id
    Part* PartOf() const;                                 // the Part this module belongs to
    // Host seams for engines that track the joint coordinate directly instead of the Transform: when set they replace currentTransformAngle() /
    // currentTransformPosition() (the moving transform's localEulerAngles / localPosition along mainAxis). Unset: the Transform, as in C#.
    std::function<float()> angleSource, positionSource;

    // ---- plain state (non-KSPField members of BaseServo.cs) ----
    bool hasEnoughResources = false;                      // :141
    bool servoIsBraking = false;                          // :143
    bool motorManualDisengaged = false;                   // :145
    Vector3f axis, secAxis, pivot;                        // :153-:159
    Matrix4x4f servoParentTransform = Matrix4x4f::identity(), servoParentTransformInverse = Matrix4x4f::identity();  // :168-:171
    std::vector<AttachNode*> attachNodes;                 // :177
    Quaternionf cachedStartingRotation = Quaternionf::identity();  // :180
    float cachedStartingRotationOffset = 0.0f;            // :183
    std::vector<std::string> servoSrfMeshes;              // :186
    Quaternionf targetRotation = Quaternionf::identity();  // :189
    float lockAngle = 0.0f;                               // :191
    double rate = 0.0;                                    // :197
    bool prevServoMotorIsEngaged = false, prevServoIsMotorized = false, prevServoIsLocked = false;  // :199-:203
    bool partActionMenuOpen = false;                      // :205
    bool wasMoving = false;                               // :207
    double timeStoppedMoving = 0.0;                       // :209
    bool stoppedMovingTimerOn = false;                    // :211
    std::string OutputUnit;                               // :213
    float motorOutput = 0.0f;                             // :215
    float driveUnit = 0.0f;                               // :217
    float currentVelocityLimit = 0.0f;                    // :219 internal
    float previousDisplacement = 0.0f;                    // :225 internal
    float currentFrameVelocity = 0.0f;                    // :227 internal
    float transformRateOfMotion = 0.0f;                   // :229 internal
    int velocityAverageReadings = 20;                     // :231
    std::vector<float> velocityReadings;                  // :233
    int velocityReadingIndex = 0;                         // :235
    float velocityReadingsSum = 0.0f;                     // :237
    std::set<uint32_t> upstreamParts, downstreamParts;    // :239-:241 (craftIDs)
    std::vector<CompoundPart*> spanningParts;             // :243
    bool partIsPhysicsLess = false;                       // :245
    bool servoInitComplete = false;                       // :147
    bool servoTransformPosLoaded = false, servoTransformRotLoaded = false, jointParentRotationLoaded = false;  // :193 :195 :37
    // PartJoint pJoint (the part's attach joint): the host's handle, opaque; only presence is read
    bool hasPJoint = false;
    // C++: the module lives in a prefab until the part is instantiated; set once OnDestroy ran
    bool destroyed = false, behaviourEnabled = true;

    // ---- properties ----
    bool HasEnoughResources() const { return hasEnoughResources; }        // :266
    bool ServoInitComplete() const { return servoInitComplete; }          // :268
    float CurrentVelocityLimit() const { return currentVelocityLimit; }   // :272
    ResourceConsumptionTypes resourceConsumptionMode() const { return EnumName("resourceConsumptionMode") == "VelocityLimit" ? ResourceConsumptionTypes::VelocityLimit : ResourceConsumptionTypes::CurrentVelocity; }

    // ---- [KSPField] accessors used by the subclasses ----
    bool servoIsLocked() const { return Get<bool>("servoIsLocked"); }
    bool servoIsMotorized() const { return Get<bool>("servoIsMotorized"); }
    bool servoMotorIsEngaged() const { return Get<bool>("servoMotorIsEngaged"); }
    void set_servoMotorIsEngaged(bool v) { Set<bool>("servoMotorIsEngaged", v); }
    float servoMotorSize() const { return Get<float>("servoMotorSize"); }
    float servoMotorLimit() const { return Get<float>("servoMotorLimit"); }
    float maxMotorOutput() const { return Get<float>("maxMotorOutput"); }
    bool lockPartOnPowerLoss() const { return Get<bool>("lockPartOnPowerLoss"); }
    std::string mainAxis() const { return Get<std::string>("mainAxis"); }
    Vector2f hardMinMaxLimits() const { return Get<Vector2f>("hardMinMaxLimits"); }
    Vector2f traverseVelocityLimits() const { return Get<Vector2f>("traverseVelocityLimits"); }
    float launchPosition() const { return Get<float>("launchPosition"); }
    bool useMultipleDragCubes() const { return Get<bool>("useMultipleDragCubes"); }

    // ---- IResourceConsumer ----
    std::vector<const PartResourceDefinition*> GetConsumedResources() override { return consumedResources_; }  // :341

    // ---- [KSPAction] / [KSPEvent] methods (:277-:339) ----
    void ToggleServoLockedAction(KSPActionParam&);   // :277
    void ServoEngageLockAction(KSPActionParam&);     // :290
    void ServoDisgageLockAction(KSPActionParam&);    // :299
    void ToggleMotorAction(KSPActionParam&);         // :305
    void MotorOnAction(KSPActionParam&);             // :318
    void MotorOffAction(KSPActionParam&);            // :324
    void ResetPositionAction(KSPActionParam&) { ResetLaunchPosition(); }  // :330
    void ResetPositionEvent() { ResetLaunchPosition(); }                  // :336

    // ---- the abstract surface (:346-:362, :388-:402, :522, :1701-:1714) ----
    virtual void OnJointInit(bool goodSetup) = 0;
    virtual void OnPostStartJointInit() = 0;
    virtual void OnPreModifyServo() = 0;
    virtual void OnModifyServo() = 0;
    virtual void OnVisualizeServo(bool rotateBase) = 0;
    virtual void OnServoLockApplied() = 0;
    virtual void OnServoLockRemoved() = 0;
    virtual void OnServoMotorEngaged() = 0;
    virtual void OnServoMotorDisengaged() = 0;
    virtual float GetFrameDisplacement() = 0;
    virtual void SetInitialDisplacement() = 0;
    virtual bool IsMoving() = 0;
    virtual void ServoFixedUpdate() = 0;                // :437 `protected new abstract void OnFixedUpdate()`
    virtual void ResetLaunchPosition() = 0;
    virtual void InitAxisFieldLimits() = 0;
    virtual void UpdateAxisFieldHardLimit(const std::string& fieldName, const Vector2f& newlimits) = 0;
    virtual void UpdateAxisFieldSoftLimit(const std::string& fieldName, const Vector2f& newlimits) = 0;

    // ---- virtuals with a body ----
    virtual double CalculatePower();                    // :364
    virtual void CalculateResourceDrain();              // :404
    virtual void OnSaveShip();                          // :454 (onAboutToSaveShip)
    virtual void ApplyCoordsUpdate();                   // :465
    virtual void RecurseCoordUpdate(Part* p, Part* rootPart);  // :479
    virtual void RecurseAttachNodeUpdate(Part* p);      // :491
    virtual void SetLaunchPosition(float val);          // :514
    virtual void Update();                              // :911
    virtual void UpdatePAWUI(UI_Scene currentScene);    // :1480 (state side only: motorState, resourceConsumption, the field gui flags; the UI controls are not ported)
    virtual bool CanBeDetached() override { return !IsMoving(); }  // :1795
    virtual bool CanBeOffset() override { return !IsMoving(); }    // :1800
    virtual bool CanBeRotated() override { return !IsMoving(); }   // :1805
    virtual void OnDestroy();                           // :439

    // ---- PartModule hooks ----
    void OnAwake() override;                            // :524
    void OnLoad(const ConfigNode& node) override;       // :546
    void OnSave(ConfigNode& node) override;             // :564
    void OnStart(StartState state) override;            // :587
    void OnStartBeforePartAttachJoint(StartState modStartState) override;  // :799
    void PromoteToPhysicalPart() override;              // :818
    void DemoteToPhysicslessPart() override;            // :826
    void OnCopy(PartModule& fromModule) override;       // :850
    IPartMassModifier* AsMassModifier() override { return this; }
    IPartCostModifier* AsCostModifier() override { return this; }
    IConstruction* AsConstruction() override { return this; }

    // ---- Unity messages the host sends ----
    void Start();                                       // :865
    void FixedUpdate();                                 // :873

    // ---- lock / motor ----
    bool EngageServoLock();                             // :960
    void DisengageServoLock();                          // :975
    void EngageMotor();                                 // :984
    void DisengageMotor();                              // :993
    void ModifyServo(const ModuleValue* field = nullptr);   // :1213
    void ModifyLocked(const ModuleValue* field = nullptr);  // :1237
    void ModifyEngaged(const ModuleValue* field = nullptr); // :1280
    void ManuallyModifyEngaged(const ModuleValue* field = nullptr);  // :1293
    void OnVesselModified(Vessel* v);                   // :1307

    // ---- events the host forwards ----
    void OnPartMenuOpen(Part* inpPart);                 // :1002 (UIPartActionWindow shown)
    void OnPartMenuClose(Part* inpPart);                // :1018
    void OnEditorPartPlaced(Part* placedPart);          // :1026
    void OnEditorPartPicked(Part* pickedPart);          // :1050
    void OnEditorCompoundPartLinked(CompoundPart* linkedPart);  // :1070
    void OnPartPack() { ApplyCoordsUpdate(); }          // :1177
    void OnPartUnpack() { ApplyCoordsUpdate(); }        // :1182

    // ---- joint ----
    void InitJoint();                                   // :1091
    void PostStartInitJoint();                          // :1132
    void SetServoMass();                                // :1187
    void CreateServoRigidbody();                        // :1196
    void SaveSpanningPartTargets();                     // :1322
    void RestoreSpanningPartTargets();                  // :1342
    void VisualizeServo(bool moveChildren);             // :1354
    Vector3f GetMainAxis() const;                       // :1460
    float StartingRotationOffset();                     // :1524
    float currentTransformAngle();                      // :1546
    float currentTransformPosition();                   // :1572
    float UpdateFieldLimits(BaseAxisField& axisField, const Vector2f& newlimits, float currentValue);  // :1590 (the UI_FloatRange half is UI)
    // returns the servo body when `node` belongs to the moving part, else the part's own (nullptr here: the part's Rigidbody is the physics engine's)
    enum class BodyOwner { Servo, Part };
    BodyOwner AttachServoRigidBody(const AttachNode& node);  // :1602
    BodyOwner NodeRigidBody(const AttachNode& node);         // :1627
    bool ServoTransformCollider(const std::string& colName); // :1642
    ServoNode* MovingObject() { return movingPartObject.get(); }  // :1661
    ServoNode* BaseObject() { return basePartObject.get(); }      // :1666
    void SetDriveUnit();                                // :1232 (private)
    void CalculateAverageRateOfMovement();              // :392

    // ---- IPartCostModifier / IPartMassModifier ----
    float GetModuleCost(float defaultCost, ModifierStagingSituation sit) override;       // :1671
    ModifierChangeWhen GetModuleCostChangeWhen() override { return ModifierChangeWhen::CONSTANTLY; }  // :1681
    float GetModuleMass(float defaultMass, ModifierStagingSituation sit) override;       // :1686
    ModifierChangeWhen GetModuleMassChangeWhen() override { return ModifierChangeWhen::CONSTANTLY; }  // :1696

    // ---- IAxisFieldLimits ----
    void UpdateAxisFieldLimit(const std::string& fieldName, const Vector2f& hardLimits, const Vector2f& softLimits);  // :1703
    void SetHardLimits(const std::string& fieldName, const Vector2f& newLimits) override;   // :1716
    void SetSoftLimits(const std::string& fieldName, const Vector2f& newLimits) override;   // :1721
    bool HasAxisFieldLimits() override;                                                     // :1732
    bool HasAxisFieldLimit(const std::string& fieldName) override;                          // :1741
    std::vector<AxisFieldLimit>& GetAxisFieldLimits() override { return axisFieldLimitList_; }  // :1750
    AxisFieldLimit* GetAxisFieldLimit(const std::string& fieldName) override;               // :1759
    Vector2f GetHardLimits(const std::string& fieldName) override;                          // :1772
    Vector2f GetSoftLimits(const std::string& fieldName) override;                          // :1781
    // IJointLockState
    bool IsJointUnlocked() override { return !servoIsLocked(); }                            // :1790

    // axisFieldLimits.Add(name, limit) (InitAxisFieldLimits of the subclasses)
    void AddAxisFieldLimit(const std::string& name, const AxisFieldLimit& limit);

protected:
    std::vector<const PartResourceDefinition*> consumedResources_;
    std::vector<std::string> axisFieldNames_;
    std::vector<AxisFieldLimit> axisFieldLimitList_;
    std::shared_ptr<Coroutine> visualizeCoroutine_;
    std::shared_ptr<RoboticCollisions> roboticCollisions_;  // the RoboticCollisions component InitJoint adds to the moving object
    bool servoSrfMeshesSet_ = false;                         // servoSrfMeshes != null (the C# array is null until servoSrfMeshNames is set)
    bool PartIsFlightScene() const;
    Enumerator WaitAndVisualizeServo();                      // :791

private:
    void RecurseBuildPartSets(Part* p, std::set<uint32_t>* partSet);   // :741
    void RecurseRemovePartSets(Part* p, std::set<uint32_t>* partSet);  // :762
    void BuildPartSets();                                              // :783
    void SetChildParentTransform(Part* p);                             // :1393 (editor re-parenting of GameObjects: not ported, see header)
    friend struct ServoTestAccess;
};

// VesselPhysicsOptions::isArticulation helper: the PartJointSpec of a robotic part's moving side is an articulation point of the reduced model
// (its child is welded to the cluster otherwise). Pass `[](const physics::PartJointSpec& j){ return ServoArticulation(j); }`.
bool ServoArticulation(const physics::PartJointSpec& j);

}  // namespace Expansions::Serenity
}  // namespace ksp
