// WP-12b: collider activation of PQSCity / PQSCity2 scenery objects (the KSC, the airfield, launch sites, set dressing).
//
// What KSP does (Assembly-CSharp/PQSCity.cs, PQSCity2.cs): the scenery of a city is a GameObject subtree under the city transform
// (prefab data: KSC.prefab etc., with BoxCollider / MeshCollider components on the buildings, runway and launch pad). The PQS mod
// callbacks switch GameObjects on and off, and a disabled GameObject has no collider:
//   PQSCity  : the city's direct children are active while the sphere has a target and is alive (`transformsActive`:
//              OnSphereStart / OnSphereReset deactivate them, OnUpdateFinished reactivates them); LODRange entries (sorted by
//              visibleRange in OnSetup) switch their `objects` and renderers by the float distance target-to-city
//              (`num2 < visibleRange`) in OnUpdateFinished; OnSphereInactive switches every LOD range off.
//   PQSCity2 : no direct children toggling; LodObject entries (visibleRange, objects, KeepActive) are all switched off by
//              Setup / SetInactive (OnSetup, OnSphereStart without a live sphere, OnSphereReset, OnSphereInactive, a missing
//              target) and by `setVisibility` in OnUpdateFinished: active iff sqrMagnitude(target - city) < visibleRange^2,
//              KeepActive forces active.
// This file ports exactly that structure; the colliders themselves are DATA the caller supplies per object (CityCollider): the
// Unity prefab colliders are not extracted yet (STATUS.md WP-12b "Needs data": the KSC prefab is in the level scene files, and the
// MeshCollider meshes need the asset pipeline of WP-12c / WP-47), so tests and hosts use labelled fixtures. An active object puts
// its colliders into the PhysicsWorld as static bodies at the city pose (position: planet-relative double in the physics frame;
// rotation: the city's localRotation), an inactive object removes them.
//
// Changed vs KSP: distances are measured in doubles in the physics frame and narrowed to float for the comparison (KSP: float
// world-space positions under the floating origin); the two agree except within a float ulp of a visibleRange boundary.
// The Unity events the callbacks also fire (OnPQSCityLoaded / POI range) are WP-42's (pqs_city.hpp), not repeated here.
#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ksp/math/quaterniond.hpp"
#include "ksp/math/vector3d.hpp"
#include "ksp/physics/physics_world.hpp"

namespace ksp {

// One collider component of a scenery object, in the object's local frame (the city transform: Unity local space).
struct CityCollider {
    enum class Kind { Box, Sphere, Capsule, Mesh };
    Kind kind = Kind::Box;
    physics::ShapeDesc shape;                       // Box / Sphere / Capsule (half extents / radius / half height)
    Vector3d localPosition;                         // shape centre / mesh origin in the city frame
    QuaternionD localRotation = QuaternionD::identity();
    std::vector<Vector3f> vertices;                 // Mesh: local-frame vertices
    std::vector<int> triangles;                     // Mesh: index triples
    float friction = 0.6f;                          // body friction when no material is given (Jolt's own combine); with a material see below
    // WP-12c: the collider's PhysicMaterial (data/scenery: null in every stock scenery collider, i.e. Physics.defaultMaterial); null =
    // the world's default material
    std::shared_ptr<const physics::PhysicMaterialDesc> material;
};

// A GameObject of the city subtree with its colliders. Index = order of Add.
struct CityObject {
    std::vector<CityCollider> colliders;
    // WP-12c: the GameObject the colliders belong to, as the contact records read it (layer, tag, name; CrashObjectName found upwards;
    // PositionMobileLaunchPad in the parents)
    int layer = 15;
    std::string tag = "Untagged";
    std::string name;
    std::string crashObjectName;
    bool mobileLaunchPad = false;
};

// The engine side of GameObject.SetActive for colliders: places an object's colliders into the world at the city pose.
class CityColliderSet {
public:
    CityColliderSet(physics::PhysicsWorld& world, uint64_t keyBase) : world_(world), keyBase_(keyBase) {}
    ~CityColliderSet();
    CityColliderSet(const CityColliderSet&) = delete;
    CityColliderSet& operator=(const CityColliderSet&) = delete;

    size_t Add(CityObject object);
    size_t ObjectCount() const { return objects_.size(); }
    // The city transform in the physics frame. Active objects are re-placed.
    void SetPose(const Vector3d& position, const QuaternionD& rotation);
    // GameObject.SetActive(active): idempotent
    void SetObjectActive(size_t index, bool active);
    bool IsObjectActive(size_t index) const { return objects_[index].active; }
    size_t ActiveObjectCount() const;
    // number of this set's static bodies in the world
    size_t BodyCount() const;
    const Vector3d& position() const { return position_; }
    uint64_t KeyOf(size_t objectIndex, size_t colliderIndex) const {
        return keyBase_ + (static_cast<uint64_t>(objectIndex) << 16) + static_cast<uint64_t>(colliderIndex);
    }
    const CityObject& Object(size_t index) const { return objects_[index].object; }
    // WP-12c: called after a collider's static body is created / before it is removed (the ContactHost registration of layer and tag)
    std::function<void(uint64_t key, const CityObject& object)> onPlaced;
    std::function<void(uint64_t key)> onRemoved;

private:
    struct Slot {
        CityObject object;
        bool active = false;
    };
    void Place(size_t index);
    void Remove(size_t index);
    physics::PhysicsWorld& world_;
    uint64_t keyBase_;
    Vector3d position_;
    QuaternionD rotation_ = QuaternionD::identity();
    std::vector<Slot> objects_;
};

// PQSCity.cs:11 collider switching (LODRange, transformsActive)
class PQSCityColliders {
public:
    // PQSCity.cs:11 LODRange: visibleRange, the objects it switches (renderers are the host's)
    struct LODRange {
        float visibleRange = 0.0f;
        std::vector<size_t> objects;
        bool isActive = false;
    };

    PQSCityColliders(physics::PhysicsWorld& world, uint64_t keyBase) : set(world, keyBase) {}

    CityColliderSet set;
    std::vector<size_t> children;   // objects that are direct children of the city transform
    std::vector<LODRange> lod;
    bool transformsActive = true;   // PQSCity.cs:103

    void Setup(LODRange& r);                       // PQSCity.cs:22 (LODRange.Setup)
    void SetActive(LODRange& r, bool active);      // PQSCity.cs:41 (LODRange.SetActive)
    void OnSetup();                                // :168
    bool OnSphereStart(bool sphereIsAlive);        // :179
    void OnSphereReset();                          // :208
    void OnSphereActive(bool hasTarget, const Vector3d& targetPosition);   // :220
    void OnSphereInactive();                       // :225
    void OnUpdateFinished(bool hasTarget, const Vector3d& targetPosition); // :234

private:
    void SetChildrenActive(bool active);
};

// PQSCity2.cs:6 collider switching (LodObject, SetInactive, setVisibility)
class PQSCity2Colliders {
public:
    // PQSCity2.cs:10 LodObject
    struct LodObject {
        float visibleRange = 0.0f;
        std::vector<size_t> objects;
        bool isActive = true;      // :16
        bool KeepActive = false;   // :18
        float visibleRangeSqr = 0.0f;
    };

    PQSCity2Colliders(physics::PhysicsWorld& world, uint64_t keyBase) : set(world, keyBase) {}

    CityColliderSet set;
    std::vector<LodObject> objects;
    bool inVisibleRange = false;

    void SetRange(LodObject& o);                   // PQSCity2.cs:35
    void Setup(LodObject& o);                      // :24
    void SetActive(LodObject& o, bool active);     // :40
    void OnSetup();                                // :407
    bool OnSphereStart(bool sphereIsAlive);        // :425
    void OnSphereReset();                          // :443
    void OnSphereActive(bool hasTarget, const Vector3d& targetPosition);   // :448
    void OnSphereInactive();                       // :453
    void OnUpdateFinished(bool hasTarget, const Vector3d& targetPosition); // :458

private:
    void SetInactive();                            // :367
    void setVisibility(const Vector3d& targetPosition);   // :491
};

}  // namespace ksp
