// WP-12d: the terrain-following box colliders of PQS_Collider.cs, PQS_ColliderAdvanced.cs, PQS_PartCollider.cs and the sphere
// collider of PQSMod_CreateSphereCollider.cs.
//
// What KSP does: a MonoBehaviour on a rigid body (PQS_PartCollider: `Part.terrainCollider`, Part.cs:255) keeps one or three
// BoxCollider GameObjects under or ahead of the body: a "gravity" box whose top face lies on the terrain under the body (oriented
// to the local terrain normal, sampled with three rotated heights) and, for the two advanced classes, a "velocity" box at the
// point where the velocity probe (a ray of 2 s of travel, halved while inside the terrain) meets the ground plus a tall "cliff"
// box under it. PQS_Collider has one box switched with GameObject.SetActive; the advanced classes keep their boxes active and
// switch them per owner with Physics.IgnoreCollision (every other owner's colliders ignore them, RegisterColliders). An owner
// below the surface by more than 0.25 m is put back 1 m above it with 80 % of its velocity.
// None of the four is instantiated by shipped code (WP-12b grep: nothing creates Part.terrainCollider, no stock prefab carries
// PQSMod_CreateSphereCollider): stock behaviour is unchanged by this port; mods and hosts can use it.
//
// What is engine-side here (data): the Unity GameObjects are `PQSBoxCollider` records (name, localScale, parent, localPosition /
// position, rotation as the Transform.up setter writes it, activeSelf, layer) and Physics.IgnoreCollision is an ordered call log
// with the resulting per-pair state (`IgnoreCollisionLog`). The owner's transform / Rigidbody / Part are hooks (`SurfaceColliderOwner`).
// The debug cubes of `collidersVisibleDEBUG` / `showNormalsDEBUG` (renderers and colours) are rendering and are not ported; the
// debug sample positions ShowDebugNormals computes are kept (`debugSamples`). `BoxWorldPose` turns a record into a world pose a
// physics host can place (PhysicsWorld::AddStaticShape); PhysicsWorld has no per-pair ignore, so a host that hosts the advanced
// boxes applies `IgnoreCollisionLog::IsIgnored` itself.
//
// Numerics: managed Unity float code under the Mono float rule (relPos / relVel of PQS_ColliderAdvanced and everything of
// PQS_Collider are float Vector3), KSP's Vector3d / QuaternionD in double, Quaternion.FromToRotation (the Transform.up setter
// and the velocity frame) native float32 (verified against the C3 capture, quaternion_native.hpp).
#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "ksp/math/quaterniond.hpp"
#include "ksp/math/vector3d.hpp"
#include "ksp/terrain/pqs.hpp"
#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

class CelestialBody;
class Part;

// A BoxCollider GameObject made by `new GameObject().AddComponent<BoxCollider>()` (or GameObject.CreatePrimitive(Cube) in debug
// mode: the same unit box collider plus a renderer): size (1, 1, 1), centre 0, so the box is the transform's localScale.
struct PQSBoxCollider {
    enum class Parent { Root, Owner, Sphere };
    int id = 0;                                    // collider identity for the IgnoreCollision log (unique per process)
    std::string name;                              // gameObject.name
    Vector3f localScale{1.0f, 1.0f, 1.0f};
    Parent parent = Parent::Root;                  // transform.parent: none, the owner's transform, sphere.transform
    Vector3f localPosition{0.0f, 0.0f, 0.0f};      // transform.localPosition (Parent::Sphere / Owner)
    Vector3f position{0.0f, 0.0f, 0.0f};           // transform.position (Parent::Root: PQS_PartCollider writes world positions)
    Quaternionf rotation = Quaternionf::identity();   // transform.rotation (world) as written by the Transform.up setter
    bool activeSelf = true;                        // GameObject.SetActive
    int layer = 0;                                 // gameObject.layer
    int writes = 0;                                // number of pose writes (tests)
};

// Physics.IgnoreCollision(a, b, ignore) calls in order, and the resulting state per unordered pair (default: not ignored).
class IgnoreCollisionLog {
public:
    struct Call {
        int a = 0, b = 0;
        bool ignore = true;
    };
    void Record(int a, int b, bool ignore = true);
    bool IsIgnored(int a, int b) const;
    const std::vector<Call>& Calls() const { return calls_; }
    void Clear();

private:
    std::vector<Call> calls_;
    std::map<std::pair<int, int>, bool> state_;
};
// the process-wide log (Physics.IgnoreCollision is a static engine call)
IgnoreCollisionLog& PhysicsIgnoreCollisions();
// a new collider identity (the owner's own colliders take theirs from here too)
int NewSurfaceColliderId();

// The owner MonoBehaviour's GameObject, Transform and Rigidbody (hooks; unset = absent / zero)
struct SurfaceColliderOwner {
    std::string name;                                   // gameObject.name
    std::function<Vector3f()> position;                 // transform.position (world, float)
    std::function<void(const Vector3f&)> setPosition;   // transform.position = v
    std::function<Vector3f()> velocity;                 // GetComponent<Rigidbody>().velocity
    std::function<void(const Vector3f&)> setVelocity;
    std::function<bool()> hasRigidbody;                 // GetComponent<Rigidbody>() != null (unset: true)
    std::function<bool()> isKinematic;                  // rigidbody.isKinematic (unset: false)
    int colliderId = 0;                                 // GetComponent<Collider>() (PQS_ColliderAdvanced)
    std::vector<int> colliderIds;                       // GetComponentsInChildren<Collider>() (PQS_PartCollider)
};

// The world pose of a box record: centre (sphere-parented boxes through PQS.GetWorldPosition = sphere.transform.TransformPoint),
// rotation and half extents (localScale / 2: a unit BoxCollider scaled by the transform).
struct PQSBoxWorldPose {
    Vector3d centre;
    QuaternionD rotation = QuaternionD::identity();
    Vector3f halfExtents{0.5f, 0.5f, 0.5f};
};
PQSBoxWorldPose BoxWorldPose(const PQSBoxCollider& box, const PQS* sphere, const Vector3d& ownerPosition);

// ---- PQS_Collider.cs ------------------------------------------------------------------------------------------------------------
class PQS_Collider {
public:
    PQS* sphere = nullptr;              // PQS_Collider.cs:5
    float colliderDepth = 100.0f;       // :7 (Reset :19)
    float colliderMaxDistance = 10.0f;  // :9
    bool collidersVisibleDEBUG = false; // :11
    SurfaceColliderOwner owner;
    // the sphere's height function (default sphere->GetSurfaceHeight(v): PQS.cs:3088)
    std::function<double(const Vector3d&)> surfaceHeight;

    PQSBoxCollider colliderObject;      // :13
    bool IsColliderActive() const { return isColliderActive; }
    float HalfDepth() const { return halfDepth; }

    void Reset();                       // :19
    void Start();                       // :25
    void SetTargetSphere(PQS* sphere);  // :44
    void FixedUpdate();                 // :60

private:
    void UpdateCollider();              // :65
    void ActivateCollider();            // :86
    void DeactivateCollider();          // :95
    double Height(const Vector3d& v) const;
    bool isColliderActive = false;      // :15
    float halfDepth = 0.0f;             // :17
};

// ---- PQS_ColliderAdvanced.cs ----------------------------------------------------------------------------------------------------
class PQS_ColliderAdvanced {
public:
    PQS_ColliderAdvanced();
    ~PQS_ColliderAdvanced();            // OnDestroy (:257) when started
    PQS_ColliderAdvanced(const PQS_ColliderAdvanced&) = delete;
    PQS_ColliderAdvanced& operator=(const PQS_ColliderAdvanced&) = delete;

    PQS* sphere = nullptr;                              // :12
    Vector2f colliderSize{1.0f, 1.0f};                  // :14 (Reset :86)
    bool useGravityCollider = false;                    // :16
    bool useVelocityCollider = false;                   // :22
    bool collidersVisibleDEBUG = false;                 // :30
    bool showNormalsDEBUG = false;                      // :32
    SurfaceColliderOwner owner;
    std::function<double(const Vector3d&)> surfaceHeight;   // default sphere->GetSurfaceHeight

    // :34-42 (private static in C#)
    static inline int maxVelocityIteration = 4;
    static inline float velocityThinkAhead = 2.0f;
    static inline float halfColliderHeight = 100.0f;
    static inline float halfCliffHeight = 500.0f;
    static inline double sampleAngle = 0.0001;
    static const std::vector<QuaternionD>& SampleRotations() { return sampleRotationsD; }
    static const std::vector<PQS_ColliderAdvanced*>& ColliderList() { return colliderList; }
    static void ClearStatics();   // tests: sampleRotationsD = null, colliderList = null

    PQSBoxCollider gravityCollider, velocityCollider, velocityCliffCollider;   // :18 :24 :26
    bool hasGravityCollider = false, hasVelocityCollider = false;              // the objects exist (Start)
    bool IsGravityColliderActive() const { return isGravityColliderActive; }
    bool IsVelocityColliderActive() const { return isVelocityColliderActive; }
    // the C# private state the updates leave behind (read by tests and the reference gate)
    Vector3f relPos, relVel;
    Vector3d relDir, planeVelocity, vel, hitPoint, normal;
    double radDist = 0.0, surfaceHeightValue = 0.0, altitude = 0.0, radSpeed = 0.0, verticalSpeed = 0.0, speed = 0.0, surfHeight = 0.0;
    std::vector<Vector3d> debugSamples[2];   // ShowDebugNormals' sample points (cubes[set, i] positions), only with showNormalsDEBUG

    void Reset();                                   // :86
    void Start();                                   // :176
    void OnDestroy();                               // :257
    void SetTargetSphere(PQS* sphere);              // :274
    void FixedUpdate();                             // :312
    void OnReferenceBodyChange(CelestialBody& body);   // :448

private:
    void RegisterColliders();                       // :91
    void UnregisterColliders();                     // :133
    void ActivateGravityCollider();                 // :138
    void DeactivateGravityCollider();               // :147
    void ActivateVelocityCollider();                // :156
    void DeactivateVelocityCollider();              // :166
    void UpdateColliders();                         // :317
    void UpdateGravityCollider();                   // :334
    void UpdateVelocityCollider();                  // :368
    bool IterateVelocity(const Vector3d& start, const Vector3d& vel, double length, int itr, int maxItr);   // :388
    void UpdateVerticalCollider(PQSBoxCollider& col, double surfaceHeight, const Vector3d& radianNormalized);   // :403
    void UpdateNormalCollider(PQSBoxCollider& col, double depth, const Vector3d& radianNormalized);             // :409
    void ShowDebugNormals(int set, const Vector3d& radPos);   // :416
    Vector3d GetNormalAtPoint(const Vector3d& radian);        // :432
    double Height(const Vector3d& v) const;

    static inline std::vector<QuaternionD> sampleRotationsD;   // :44 (null = empty)
    static inline std::vector<PQS_ColliderAdvanced*> colliderList;   // :46
    static inline bool colliderListCreated = false;
    bool isGravityColliderActive = false;           // :20
    bool isVelocityColliderActive = false;          // :28
    bool started_ = false;
    Vector3d samplesD[3];                            // :48
    Vector3d edgeABd, edgeACd;                       // :50 :52
};

// ---- PQS_PartCollider.cs --------------------------------------------------------------------------------------------------------
class PQS_PartCollider {
public:
    PQS_PartCollider();
    ~PQS_PartCollider();                // OnDestroy (:274) when started
    PQS_PartCollider(const PQS_PartCollider&) = delete;
    PQS_PartCollider& operator=(const PQS_PartCollider&) = delete;

    PQS* sphere = nullptr;                              // :7
    Vector2f colliderSize{1.0f, 1.0f};                  // :9 (Reset :95)
    bool useGravityCollider = true;                     // :11
    bool useVelocityCollider = true;                    // :13
    int layerID = 0;                                    // :15
    SurfaceColliderOwner owner;
    std::function<double(const Vector3d&)> surfaceHeight;   // default sphere->GetSurfaceHeight
    // `part` (:33): the members FixedUpdate reads. Unset `partPresent` = part == null.
    struct PartHooks {
        std::function<bool()> packed;                   // part.packed
        std::function<bool()> hasVessel;                // part.vessel != null
        std::function<bool()> vesselLanded;             // part.vessel.Landed
        std::function<PQS*()> referenceBodySphere;      // part.vessel.orbitDriver.referenceBody.pqsController
    } part;
    bool partPresent = false;
    // binds the hooks to a live Part (its vessel, orbit driver and reference body)
    void BindPart(Part* p);

    // :51-61 (public static)
    static inline int maxVelocityIteration = 2;
    static inline double velocityThinkAhead = 2.0;
    static inline double halfColliderHeight = 100.0;
    static inline double halfCliffHeight = 500.0;
    static inline double sampleAngle = 0.0001;
    static const std::vector<QuaternionD>& SampleRotations() { return sampleRotationsD; }
    static const std::vector<PQS_PartCollider*>& ColliderList() { return colliderList; }
    static const Vector3f& SafeSpot() { return safeSpot; }
    static void ClearStatics();

    PQSBoxCollider gravityCollider, velocityCollider, velocityCliffCollider;   // :18 :24 :27
    bool hasGravityCollider = false, hasVelocityCollider = false;
    bool isGravityColliderActive = false;    // :21 (public in C#)
    bool isVelocityColliderActive = false;   // :30
    std::vector<Vector3d> samplesD;          // :36
    bool collidersSet = false;               // `colliders != null` (:39)
    std::vector<int> colliders;              // :39 (the owner's colliders)
    // the C# private state
    Vector3d relPos, relVel, relDir, planeVelocity, relVelNorm, hitPoint;
    double radDist = 0.0, surfaceHeightValue = 0.0, altitude = 0.0, radSpeed = 0.0, verticalSpeed = 0.0, speed = 0.0, surfHeight = 0.0;
    int itr = 0;
    Quaternionf q = Quaternionf::identity();
    static const Vector3d& Normal() { return normal; }

    void Reset();                                   // :95
    void Awake();                                   // :228
    void Start();                                   // :243
    void OnDestroy();                               // :274
    void SetTargetSphere(PQS* sphere);              // :291
    void FixedUpdate();                             // :326
    void OnReferenceBodyChange(CelestialBody& body);   // :469

private:
    void RegisterColliders();                       // :102
    void UnregisterColliders();                     // :152
    void ActivateGravityCollider();                 // :158
    void DeactivateGravityCollider();               // :175
    void ActivateVelocityCollider();                // :192
    void DeactivateVelocityCollider();              // :210
    void UpdateColliders();                         // :349
    void UpdateGravityCollider();                   // :366
    void UpdateVelocityCollider();                  // :400
    bool IterateVelocity();                         // :420
    void UpdateVerticalCollider(PQSBoxCollider& col, double surfaceHeight, const Vector3d& radianNormalized);   // :437
    void UpdateNormalCollider(PQSBoxCollider& col, double depth, const Vector3d& radianNormalized);             // :443
    void GetNormalAtPoint(const Vector3d& radian);  // :450
    double Height(const Vector3d& v) const;

    static inline Vector3f safeSpot{3.40282347e38f, 3.40282347e38f, 3.40282347e38f};   // :41 (float.MaxValue)
    static inline std::vector<PQS_PartCollider*> colliderList;   // :43
    static inline bool colliderListCreated = false;
    static inline Vector3d edgeABd, edgeACd, normal;   // :45-49 (static in C#)
    static inline std::vector<QuaternionD> sampleRotationsD;   // :61
    bool started_ = false;
};

// ---- PQSMod_CreateSphereCollider.cs ------------------------------------------------------------------------------------------------
// OnSetup adds (once) a SphereCollider to the sphere's GameObject with radius (float)(sphere.radius + radiusOffset). Data: a host
// places a static sphere of `sphereColliderRadius` at the body centre.
class PQSMod_CreateSphereCollider : public PQSMod {
public:
    PQSMod_CreateSphereCollider() { className = "PQSMod_CreateSphereCollider"; }
    float radiusOffset = 0.0f;          // :6 (Reset :10)
    bool hasSphereCollider = false;     // GetComponent<SphereCollider>() != null
    float sphereColliderRadius = 0.0f;  // sphereCollider.radius
    void OnSetup() override;            // :15
    void VisitSerialized(const SerializedFieldVisitor& v) const override;
};

}  // namespace ksp
