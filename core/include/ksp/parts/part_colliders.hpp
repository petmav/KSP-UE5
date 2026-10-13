// WP-mucol: the collider shapes of a part in PART space, from its .mu model(s).
//
// What KSP does (Assembly-CSharp/PartLoader.cs:1765 CompileModel, PartReader.cs:205 ReadChild):
//   * the part GameObject has one child "model" (localScale = rescaleFactor, position 0, rotation identity);
//   * each MODEL node of the part cfg instantiates a .mu prefab below it (NestToParent resets position and rotation and puts the
//     instance under the transform found by `parent` (Transform.Find) or under "model"; then `name`, `scale` (default one),
//     `position`, `rotation` (Quaternion.Euler of the cfg degrees) are applied);
//   * a part cfg without MODEL nodes uses the first .mu of its directory: the instance IS "model" (position 0, rotation identity,
//     scale rescaleFactor, :1914-1927).
// Every collider component of the resulting hierarchy belongs to the part (the part's Rigidbody). Unity's collider geometry in the
// part frame is the shape scaled by the transform's lossyScale (component-wise product of the local scales down the chain, no shear
// handled) and placed by the transform: Box size and Capsule / Sphere radii scale by |scale|, a Sphere by the largest component, a
// Capsule's radius by the larger of the two non-axis components and its height by the axis component, a Mesh by its vertices.
//
// Float rule: the Transform code is native (UnityPlayer.dll): one float32 rounding per operation, the Quaternion product is
// UnityNative::Multiply, the vector rotation is Quaternion.cs's operator* formula evaluated in plain float (native TransformPoint
// does the same arithmetic; the Mono managed-double rule does not apply). tools/mucollider/mu_colliders_reference.py repeats it
// independently and the unit tests compare the raw bits.
#pragma once

#include <optional>
#include <string>
#include <vector>

#include "ksp/parts/mu_model.hpp"
#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp::mu {

// One model instantiated into the part (PartModelRef of the loader without the file handles).
struct ModelInstance {
    const Model* model = nullptr;
    bool fromModelNode = false;               // MODEL{} node; false = the part directory's model (becomes "model" itself)
    std::string parent;                       // MODEL parent = (empty: none)
    std::optional<std::string> name;          // MODEL name =
    std::optional<Vector3f> scale;            // MODEL scale = (else one)
    std::optional<Vector3f> position;         // MODEL position =
    std::optional<Vector3f> rotationEuler;    // MODEL rotation = (degrees)
};

struct PartColliderShape {
    std::string path;                  // transform path from "model", '/'-separated
    EntryType entry = EntryType::BoxCollider;
    ColliderKind kind = ColliderKind::Box;
    bool trigger = false;
    std::string tag = "Untagged";
    int32_t layer = 0;
    // the owning transform in part space
    Vector3f nodePosition;
    Quaternionf nodeRotation = Quaternionf::identity();
    Vector3f nodeScale{1.0f, 1.0f, 1.0f};
    // the shape in part space
    Vector3f center;                   // Box, Sphere, Capsule: world-of-part centre
    Vector3f halfExtents;              // Box: |size * scale| / 2 (axes = the node's axes)
    float radius = 0.0f;               // Sphere, Capsule
    float height = 0.0f;               // Capsule: total height (>= 2 radius is not enforced here)
    float halfCylinder = 0.0f;         // Capsule: Max(0, height / 2 - radius), the cylinder's half length along the node's `axis`
    int32_t axis = 1;                  // Capsule direction in the node's frame
    std::vector<Vector3f> vertices;    // Mesh: part-space vertices of the (convex) collider mesh
    std::vector<int32_t> triangles;    // Mesh: all submesh triangle lists concatenated
};

// Collider shapes of the part, pre-order over the transform hierarchy (a node's own colliders, then its children): exactly the
// order of GetComponentsInChildren<Collider>. WheelColliders are not shapes (enabled = false in the prefab) and are left out.
// Triggers are INCLUDED (flagged): the caller decides (the rigid body ignores them).
std::vector<PartColliderShape> BuildPartColliders(float rescaleFactor, const std::vector<ModelInstance>& instances);

// Unity-style Quaternion * Vector3 and the lossy-scale composition used above, float per operation (exposed for the tests).
Vector3f RotateNative(const Quaternionf& q, const Vector3f& v);

}  // namespace ksp::mu
