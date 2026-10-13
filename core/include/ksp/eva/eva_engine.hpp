// WP-32 seams: everything KerbalEVA (KerbalEVA.cs), RetractableLadder.cs, FlightEVA.cs and Kerbal.cs ask of the engine (Unity animation, Transform,
// Collider, Rigidbody, Physics queries, input, camera, flag sites, command seats) goes through this file. The ported logic never includes an engine
// header; the Unreal shell (or a test) implements IEvaEngine / EvaEnvironment.
//
// Design (see how WP-33 InventoryEnvironment, WP-53 and WP-54c did their seams):
//   * IEvaEngine           per kerbal: the Rigidbody (velocity, angular velocity, forces), the Animation component, ray / capsule queries, colliders,
//                          the model transforms (footPivot, ladderPivot, ragdoll nodes), the audio / FX groups. All methods have a harmless default,
//                          so a test overrides only what it drives. HeadlessEvaEngine is the ready-made data implementation (rb state is plain data,
//                          animation states are plain data, forces are accumulated) used by the unit tests and by headless hosts.
//   * EvaEnvironment      process-wide statics (like InventoryEnvironment): GameSettings key / axis polling, FlightCamera frame, FlagSite, KerbalSeat,
//                          portrait gallery, screen messages, input locks, AdvancedRagdoll, ExpansionsLoader.
// The kerbal's own Transform is the Part's partTransform (KerbalEVA is a PartModule on the kerbalEVA part, so `base.transform` IS part.transform).
#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ksp/parts/part.hpp"
#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

class KerbalEVA;
class Vessel;

// UnityEngine.PlayMode (Animation.Play / CrossFade): StopSameLayer = 0, StopAll = 4
enum class PlayMode : int { StopSameLayer = 0, StopAll = 4 };
// UnityEngine.QueueMode: CompleteOthers = 0, PlayNow = 2
enum class QueueMode : int { CompleteOthers = 0, PlayNow = 2 };
// UnityEngine.ForceMode
enum class ForceMode : int { Force = 0, Acceleration = 5, Impulse = 1, VelocityChange = 2 };

// UnityEngine.AnimationState as far as KerbalEVA reads / writes it
struct EvaAnimState {
    std::string name;
    float length = 1.0f;
    float time = 0.0f;
    float normalizedTime = 0.0f;
    float speed = 1.0f;
    float normalizedSpeed = 0.0f;
    float weight = 0.0f;
    bool enabled = false;
    int layer = 0;
};

// UnityEngine.Collider (+ its Transform, attachedRigidbody) as the ladder / airlock / clamber code reads it
struct EvaCollider {
    std::string tag;                       // CompareTag("Ladder" / "Airlock" / "ROC")
    Part* part = nullptr;                  // FlightGlobals.GetPartUpwardsCached(gameObject)
    bool partIsAttached = true;            // part.isAttached
    PartTransform pose;                    // collider.transform (position, rotation): up / forward / right come from the rotation
    bool enabled = true;                   // Collider.enabled
    bool activeInHierarchy = true;         // gameObject.activeInHierarchy
    bool isTrigger = false;
    int layer = 0;                         // gameObject.layer
    bool hasRigidbody = false;             // attachedRigidbody != null
    Vector3f rbVelocity, rbAngularVelocity, rbWorldCenterOfMass;
    Vector3f boundsClosestPoint;           // set by the host per query; see IEvaEngine::ClosestPointOnBounds
};

// UnityEngine.RaycastHit (the members KerbalEVA reads)
struct EvaRaycastHit {
    EvaCollider* collider = nullptr;
    Vector3f point, normal;
    float distance = 0.0f;
};

// The kerbal model's ragdoll node (KerbalRagdollNode.cs): go.transform.position, rb, collider, velocity bookkeeping
struct KerbalRagdollNode {
    Vector3f goPosition;      // go.transform.position (the host writes it each frame)
    Vector3f velocity;        // :14
    Vector3f lastPos;         // :16
    bool goActive = false;    // go.SetActive
    bool colliderEnabled = false;
    // KerbalRagdollNode.cs:18
    void updateVelocity(const Vector3f& rootPos, const Vector3d& rootVel, float fdtRecip);
};

// What KerbalEVA needs of the engine, per kerbal. Every method is virtual with a neutral default.
class IEvaEngine {
public:
    virtual ~IEvaEngine() = default;

    // ---- Rigidbody (this.GetComponentCached(ref _rigidbody), part.rb) ----
    virtual Vector3f RbVelocity() = 0;
    virtual void SetRbVelocity(const Vector3f& v) = 0;
    virtual Vector3f RbAngularVelocity() = 0;
    virtual void SetRbAngularVelocity(const Vector3f& v) = 0;
    virtual Vector3f RbWorldCenterOfMass() = 0;
    virtual float RbDrag() = 0;
    virtual float RbAngularDrag() = 0;
    virtual void SetRbDrag(float d) = 0;
    virtual void SetRbMass(float m) = 0;
    virtual void AddImpulse(const Vector3f& impulse) = 0;     // Part.AddImpulse
    virtual void AddForce(const Vector3f& force) = 0;         // Part.AddForce
    virtual void AddTorque(const Vector3f& torque) = 0;       // Part.AddTorque
    virtual void RbAddForce(const Vector3f& force, ForceMode mode) = 0;  // part.rb.AddForce
    virtual void SetFixedJointAnchor(bool anchored) = 0;      // AddRBAnchor / RemoveRBAnchor (AddComponent<FixedJoint> / Destroy)

    // ---- ragdoll nodes (rb per node) ----
    virtual void NodeAddForce(int index, const Vector3f& f, ForceMode mode) = 0;
    virtual void NodeSetVelocity(int index, const Vector3f& v) = 0;
    virtual Vector3f NodeVelocity(int index) = 0;
    virtual void NodeSetDrag(int index, float drag, float angularDrag) = 0;
    virtual void NodeSetActive(int index, bool active) = 0;
    virtual void SetCharacterCollidersEnabled(bool enabled) = 0;
    virtual void SetOtherRagdollCollidersEnabled(bool enabled) = 0;
    virtual void SetLadderColliderEnabled(bool enabled) = 0;
    virtual void SetCharacterCollidersTrigger(bool isTrigger) = 0;
    virtual void SetBodyColliderCenterY(float y) = 0;         // bodyColliders[i].center = Vector3.up * y

    // ---- Animation ----
    virtual EvaAnimState* AnimState(const std::string& name) = 0;  // animation[name] (nullptr when absent)
    virtual void AnimCrossFade(const std::string& name, float fadeLength, PlayMode mode) = 0;
    virtual EvaAnimState* AnimCrossFadeQueued(const std::string& name, float fadeLength, QueueMode queue, PlayMode mode) = 0;
    virtual void AnimBlend(const std::string& name, float targetWeight, float fadeLength) = 0;
    virtual void AnimPlay(const std::string& name, PlayMode mode) = 0;
    virtual void AnimStop(const std::string& name) = 0;
    virtual void AnimSyncLayer(int layer) = 0;
    virtual bool AnimEnabled() = 0;                           // animation.enabled
    virtual void SetAnimEnabled(bool enabled) = 0;

    // ---- model transforms / queries ----
    virtual Vector3f FootPivotPosition() = 0;                 // footPivot.position
    virtual Vector3f LadderPivotPosition() = 0;               // ladderPivot.position
    virtual Vector3f ColliderClosestPoint(EvaCollider& c, const Vector3f& point) = 0;  // Collider.ClosestPointOnBounds
    virtual bool Raycast(const Vector3f& origin, const Vector3f& direction, float maxDistance, int layerMask, EvaRaycastHit& hit) = 0;
    virtual bool ColliderRaycast(EvaCollider& c, const Vector3f& origin, const Vector3f& direction, float maxDistance, EvaRaycastHit& hit) = 0;
    virtual std::vector<EvaRaycastHit> CapsuleCastAllIgnoreSelf(const Vector3f& p1, const Vector3f& p2, float radius, const Vector3f& direction,
                                                                float maxDistance, int layerMask) = 0;
    virtual std::vector<EvaRaycastHit> RaycastAllIgnoreSelf(const Vector3f& origin, const Vector3f& direction, float maxDistance, int layerMask) = 0;
    virtual void SetTransformRotationSlerp(const Quaternionf& target, float t) = 0;  // transform.rotation = Slerp(rotation, target, t) (native Slerp)
    virtual void TransformLookAt(const Vector3f& worldTarget) = 0;                   // transform.LookAt(target)

    // ---- FX / audio / props (cosmetic; recorded by the headless engine) ----
    virtual void Effect(const std::string& name) = 0;         // part.Effect(name)
    virtual void FxGroupSet(const std::string& group, bool latch, float power) = 0;  // xPos.SetLatch / SetPowerLatch (one call per group per frame)
    virtual void Named(const std::string& effect) = 0;        // any other named engine effect ("portrait:..", "weld:..", "visor:..", "prop:..")
};

// Data implementation of IEvaEngine: rb state is plain data, forces accumulate, animation states are plain data, queries answer from the scripted
// results. This is the engine of the unit tests and of any host that runs the controller without a physics bubble.
class HeadlessEvaEngine : public IEvaEngine {
public:
    // rigidbody
    Vector3f velocity, angularVelocity, worldCenterOfMass;
    float drag = 0.0f, angularDrag = 0.05f, mass = 0.0f;
    std::vector<Vector3f> impulses, forces, torques;           // Part.AddImpulse / AddForce / AddTorque, in call order
    std::vector<std::pair<Vector3f, ForceMode>> rbForces;      // part.rb.AddForce
    bool anchored = false;
    // nodes
    struct Node {
        Vector3f velocity;
        float drag = 0.0f, angularDrag = 0.0f;
        bool active = false;
        std::vector<std::pair<Vector3f, ForceMode>> forces;
    };
    std::vector<Node> nodes;
    bool characterCollidersEnabled = true, otherRagdollCollidersEnabled = false, ladderColliderEnabled = true, characterCollidersTrigger = false;
    float bodyColliderCenterY = 0.0f;
    // animation
    std::map<std::string, EvaAnimState> anims;                 // keyed by clip name (including "jp_" variants)
    bool animEnabled = true;
    struct AnimCall {
        std::string op, name;
        float a = 0.0f, b = 0.0f;
        int mode = 0;
    };
    std::vector<AnimCall> animCalls;                           // every Animation call, in order
    std::vector<int> syncedLayers;
    // transforms and queries
    Vector3f footPivot, ladderPivot;
    std::vector<std::pair<Vector3f, Vector3f>> rayLog;         // origin, direction of Raycast calls
    std::function<bool(const Vector3f&, const Vector3f&, float, int, EvaRaycastHit&)> raycast;
    std::function<bool(EvaCollider&, const Vector3f&, const Vector3f&, float, EvaRaycastHit&)> colliderRaycast;
    std::function<std::vector<EvaRaycastHit>(const Vector3f&, const Vector3f&, float, const Vector3f&, float, int)> capsuleCastAll;
    std::function<std::vector<EvaRaycastHit>(const Vector3f&, const Vector3f&, float, int)> raycastAll;
    std::vector<Quaternionf> slerpTargets;
    std::vector<float> slerpT;
    std::vector<Vector3f> lookAts;
    std::vector<std::string> effects;                          // Effect(name) and Named(...)
    std::map<std::string, std::pair<bool, float>> fxGroups;    // last latch / power per group

    Vector3f RbVelocity() override { return velocity; }
    void SetRbVelocity(const Vector3f& v) override { velocity = v; }
    Vector3f RbAngularVelocity() override { return angularVelocity; }
    void SetRbAngularVelocity(const Vector3f& v) override { angularVelocity = v; }
    Vector3f RbWorldCenterOfMass() override { return worldCenterOfMass; }
    float RbDrag() override { return drag; }
    float RbAngularDrag() override { return angularDrag; }
    void SetRbDrag(float d) override { drag = d; }
    void SetRbMass(float m) override { mass = m; }
    void AddImpulse(const Vector3f& i) override { impulses.push_back(i); }
    void AddForce(const Vector3f& f) override { forces.push_back(f); }
    void AddTorque(const Vector3f& t) override { torques.push_back(t); }
    void RbAddForce(const Vector3f& f, ForceMode m) override { rbForces.emplace_back(f, m); }
    void SetFixedJointAnchor(bool a) override { anchored = a; }

    void NodeAddForce(int i, const Vector3f& f, ForceMode m) override { Node_(i).forces.emplace_back(f, m); }
    void NodeSetVelocity(int i, const Vector3f& v) override { Node_(i).velocity = v; }
    Vector3f NodeVelocity(int i) override { return Node_(i).velocity; }
    void NodeSetDrag(int i, float d, float ad) override { Node_(i).drag = d; Node_(i).angularDrag = ad; }
    void NodeSetActive(int i, bool a) override { Node_(i).active = a; }
    void SetCharacterCollidersEnabled(bool e) override { characterCollidersEnabled = e; }
    void SetOtherRagdollCollidersEnabled(bool e) override { otherRagdollCollidersEnabled = e; }
    void SetLadderColliderEnabled(bool e) override { ladderColliderEnabled = e; }
    void SetCharacterCollidersTrigger(bool t) override { characterCollidersTrigger = t; }
    void SetBodyColliderCenterY(float y) override { bodyColliderCenterY = y; }

    // an AnimationState is created on first use (length 1) unless the test registered the clip with its real length
    EvaAnimState* AnimState(const std::string& name) override {
        auto it = anims.find(name);
        if (it == anims.end()) {
            EvaAnimState s;
            s.name = name;
            it = anims.emplace(name, s).first;
        }
        return &it->second;
    }
    void AnimCrossFade(const std::string& n, float f, PlayMode m) override { animCalls.push_back({"CrossFade", n, f, 0.0f, static_cast<int>(m)}); AnimState(n)->enabled = true; }
    EvaAnimState* AnimCrossFadeQueued(const std::string& n, float f, QueueMode q, PlayMode m) override {
        animCalls.push_back({"CrossFadeQueued", n, f, static_cast<float>(static_cast<int>(q)), static_cast<int>(m)});
        EvaAnimState* s = AnimState(n);
        s->enabled = true;
        return s;
    }
    void AnimBlend(const std::string& n, float w, float f) override { animCalls.push_back({"Blend", n, w, f, 0}); AnimState(n)->weight = w; }
    void AnimPlay(const std::string& n, PlayMode m) override { animCalls.push_back({"Play", n, 0.0f, 0.0f, static_cast<int>(m)}); AnimState(n)->enabled = true; }
    void AnimStop(const std::string& n) override { animCalls.push_back({"Stop", n, 0.0f, 0.0f, 0}); AnimState(n)->enabled = false; }
    void AnimSyncLayer(int l) override { syncedLayers.push_back(l); }
    bool AnimEnabled() override { return animEnabled; }
    void SetAnimEnabled(bool e) override { animEnabled = e; }

    Vector3f FootPivotPosition() override { return footPivot; }
    Vector3f LadderPivotPosition() override { return ladderPivot; }
    Vector3f ColliderClosestPoint(EvaCollider& c, const Vector3f&) override { return c.boundsClosestPoint; }
    bool Raycast(const Vector3f& o, const Vector3f& d, float dist, int mask, EvaRaycastHit& hit) override {
        rayLog.emplace_back(o, d);
        return raycast ? raycast(o, d, dist, mask, hit) : false;
    }
    bool ColliderRaycast(EvaCollider& c, const Vector3f& o, const Vector3f& d, float dist, EvaRaycastHit& hit) override {
        return colliderRaycast ? colliderRaycast(c, o, d, dist, hit) : false;
    }
    std::vector<EvaRaycastHit> CapsuleCastAllIgnoreSelf(const Vector3f& p1, const Vector3f& p2, float r, const Vector3f& d, float dist, int mask) override {
        return capsuleCastAll ? capsuleCastAll(p1, p2, r, d, dist, mask) : std::vector<EvaRaycastHit>{};
    }
    std::vector<EvaRaycastHit> RaycastAllIgnoreSelf(const Vector3f& o, const Vector3f& d, float dist, int mask) override {
        return raycastAll ? raycastAll(o, d, dist, mask) : std::vector<EvaRaycastHit>{};
    }
    void SetTransformRotationSlerp(const Quaternionf& q, float t) override { slerpTargets.push_back(q); slerpT.push_back(t); }
    void TransformLookAt(const Vector3f& p) override { lookAts.push_back(p); }
    void Effect(const std::string& n) override { effects.push_back(n); }
    void FxGroupSet(const std::string& g, bool latch, float power) override { fxGroups[g] = {latch, power}; }
    void Named(const std::string& e) override { effects.push_back(e); }

private:
    Node& Node_(int i) {
        if (static_cast<size_t>(i) >= nodes.size()) nodes.resize(static_cast<size_t>(i) + 1);
        return nodes[static_cast<size_t>(i)];
    }
};

// GameSettings keys the controller polls
enum class EvaKey : int {
    Forward, Back, Left, Right, YawLeft, YawRight, PackForward, PackBack, PackLeft, PackRight, PackUp, PackDown, Jump, Run, TogglePack, Use, Board,
    Orient, Lights, Helmet, ChuteDeploy, ToggleMovementMode
};
enum class KeyQuery : int { Held, Down, Up };
enum class EvaAxis : int { TranslateX, TranslateY, TranslateZ, Pitch, Yaw, Roll };

// KerbalSeat.cs as KerbalEVA reads it (the seat is another package's: this is the narrow view)
class IEvaSeat {
public:
    virtual ~IEvaSeat() = default;
    virtual Part* SeatPart() = 0;                   // kerbalSeat.part
    virtual Part* Occupant() = 0;                   // kerbalSeat.Occupant
    virtual Vector3f EjectDirection() = 0;          // kerbalSeat.ejectDirection
    virtual float EjectionForce() = 0;              // kerbalSeat.EjectionForce
    virtual Vector3f EjectionForceDirection() = 0;  // kerbalSeat.ejectionForceDirection
    virtual Vector3f SeatPivotPosition() = 0;       // kerbalSeat.SeatPivot.position
    virtual Quaternionf SeatPivotRotation() = 0;    // kerbalSeat.SeatPivot.rotation
};

// FlagSite.cs as KerbalEVA reads it
class IEvaFlag {
public:
    virtual ~IEvaFlag() = default;
    virtual void OnPlacementFail() = 0;
    virtual void OnPlacementComplete() = 0;
};

// Process-wide seams (statics, like InventoryEnvironment). Reset() restores the defaults (tests call it between cases).
struct EvaEnvironment {
    // GameSettings.EVA_* KeyBinding.GetKey / GetKeyDown / GetKeyUp. Unset: the GameSettings binding when present, else false.
    static inline std::function<bool(EvaKey, KeyQuery)> key;
    // GameSettings.axis_EVA_* .GetAxis(). Unset: 0.
    static inline std::function<float(EvaAxis)> axis;
    // FlightCamera: getReferenceFrame() * Vector3.up and mainCamera.transform.forward. Unset: up = Vector3.up, camera forward = Vector3.forward.
    static inline std::function<Vector3f()> cameraReferenceUp;
    static inline std::function<Vector3f()> cameraForward;
    // Input.GetMouseButton(0) / Input.GetAxis("Mouse X"/"Mouse Y") and the part-action-window drag test. Unset: no mouse.
    static inline std::function<bool()> mouseButtonDown;
    static inline std::function<float(bool /*x*/)> mouseAxis;
    static inline std::function<bool(Part&)> partActionWindowDragging;
    // SpaceNavigator: Translation / Rotation(Pitch, Yaw, Roll) and the device presence. Unset: no device.
    static inline std::function<bool()> spaceNavActive;
    static inline std::function<Vector3f()> spaceNavTranslation;
    static inline std::function<Vector3f()> spaceNavRotationPitchYawRoll;
    // ScreenMessages.PostScreenMessage(text, duration, style): the log is kept in postedMessages as well
    static inline std::function<void(const std::string&, float, int /*ScreenMessageStyle*/)> postScreenMessage;
    static inline std::vector<std::string> postedMessages;
    // InputLockManager.SetControlLock / RemoveControlLock go to the real InputLockManager; this is the log of them ("set:<id>" / "remove:<id>")
    static inline std::vector<std::string> controlLockLog;
    // FlightGlobals.getUpAxis() (the active vessel's up); unset: FlightGlobals::getUpAxis()
    static inline std::function<Vector3f()> upAxis;
    // HighLogic.CurrentGame.Parameters.Flight.CanBoard; unset: true
    static inline std::function<bool()> canBoard;
    // Planetarium.GetUniversalTime(); unset: Planetarium::GetUniversalTime()
    static inline std::function<double()> universalTime;
    // FlagSite.ScanSurroundingTerrain / CreateFlag
    static inline std::function<Vector3f(const Vector3f& pos, const Vector3f& fwd, float reach)> flagScanSurroundingTerrain;
    static inline std::function<IEvaFlag*(const Vector3f& spot, const Quaternionf& rot, Part& planter)> flagCreate;
    // ExpansionsLoader.IsExpansionInstalled(name); unset: installed (same convention as ModuleCargoPart::isExpansionInstalled)
    static inline std::function<bool(const std::string&)> isExpansionInstalled;
    // Physics.defaultContactOffset (Unity default 0.01) and LayerUtil.DefaultEquivalent
    static inline float defaultContactOffset = 0.01f;
    // the flight scene flags HighLogic.LoadedSceneIsFlight / MapView.MapIsEnabled (PostInteractionScreenMessage) and LoadedSceneIsMissionBuilder
    static inline bool loadedSceneIsFlight = true;
    static inline bool mapIsEnabled = false;
    // KerbalPortraitGallery register / unregister (by kerbal) and AvatarUpdate cameras (named effects only)
    static inline std::function<void(KerbalEVA&, bool /*register*/)> portraitGallery;
    // CollisionManager.UpdateAllColliders(), FlightInputHandler.ResumeVesselCtrlState / SetNeutralControls, Vessel.CrewWasModified: engine / UI only
    static inline std::function<void(const std::string&)> named;
    static inline std::vector<std::string> namedLog;           // every named effect, in order
    // the FlightGlobals.GetPartUpwardsCached(collider.gameObject) lookup for colliders that carry no part pointer: unset -> collider.part
    static inline std::function<Part*(EvaCollider&)> partUpwards;

    static bool IsInstalled(const std::string& expansion) { return isExpansionInstalled ? isExpansionInstalled(expansion) : true; }
    static void Effect(const std::string& s) {
        namedLog.push_back(s);
        if (named) named(s);
    }
    static void Reset() {
        key = nullptr; axis = nullptr; cameraReferenceUp = nullptr; cameraForward = nullptr; mouseButtonDown = nullptr; mouseAxis = nullptr;
        partActionWindowDragging = nullptr; spaceNavActive = nullptr; spaceNavTranslation = nullptr; spaceNavRotationPitchYawRoll = nullptr;
        postScreenMessage = nullptr; postedMessages.clear(); controlLockLog.clear(); universalTime = nullptr; upAxis = nullptr; canBoard = nullptr;
        flagScanSurroundingTerrain = nullptr; flagCreate = nullptr; isExpansionInstalled = nullptr; defaultContactOffset = 0.01f;
        loadedSceneIsFlight = true; mapIsEnabled = false; portraitGallery = nullptr; named = nullptr; namedLog.clear(); partUpwards = nullptr;
    }
};

}  // namespace ksp
