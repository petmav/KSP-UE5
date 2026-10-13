// WP-mucol: the .mu (PartTools binary model) reader, collider side.
//
// Spec: Assembly-CSharp/PartReader.cs (ReadChild :205 and the Read* helpers; PartToolsLib FileType 76543 = ModelBinary, EntryType.cs).
// The file is a tree of transforms (name, local position / rotation / scale) each followed by component chunks. This reader consumes
// EVERY chunk exactly as PartReader does (so the stream stays aligned) and keeps what a physics engine needs: the transform tree, the
// TagAndLayer chunk and the collider chunks. Meshes of renderers, materials, textures, animations, lights, cameras and particle
// emitters are skipped (the Unreal import pipeline decodes those); meshes of MESH COLLIDERS are kept (vertices and triangle lists).
//
// Collider chunks (PartReader.cs line numbers):
//   :229 MeshCollider    bool convex, mesh.   convex is FORCED true (:233): every KSP mesh collider is a convex hull
//   :240 SphereCollider  radius, center
//   :247 CapsuleCollider radius, int direction, center.  No height field: the component keeps Unity's default of 2
//   :255 BoxCollider     size, center
//   :301 MeshCollider2   bool isTrigger, bool convex, mesh   (convex forced true, :307)
//   :314 SphereCollider2 isTrigger, radius, center
//   :322 CapsuleCollider2 isTrigger, radius, height, int direction, center
//   :332 BoxCollider2    isTrigger, size, center
//   :340 WheelCollider   parsed and kept as a marker only (enabled = false in the prefab: it never collides)
// Unity defaults that the chunks do not set: isTrigger false; CapsuleCollider.height 2 (legacy chunk); everything a chunk sets is
// overwritten.  A malformed file throws std::runtime_error (PartReader.Read catches the exception, logs it and returns the partial
// GameObject: the game would load a truncated model, which no shipped file is).
//
// Asset origin: GameData/Squad/Parts/**/*.mu (and every other GameData .mu) of the dev copy decompiled/KSP-dev.
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp::mu {

constexpr int32_t kFileTypeModelBinary = 76543;  // PartToolsLib FileType.ModelBinary

// PartToolsLib EntryType (declaration order = value)
enum class EntryType : int32_t {
    ChildTransformStart, ChildTransformEnd, Animation, MeshCollider, SphereCollider, CapsuleCollider, BoxCollider, MeshFilter,
    MeshRenderer, SkinnedMeshRenderer, Materials, Material, Textures, MeshStart, MeshVerts, MeshUV, MeshUV2, MeshNormals,
    MeshTangents, MeshTriangles, MeshBoneWeights, MeshBindPoses, MeshEnd, Light, TagAndLayer, MeshCollider2, SphereCollider2,
    CapsuleCollider2, BoxCollider2, WheelCollider, Camera, ParticleEmitter, MeshVertexColors
};

enum class ColliderKind : uint8_t { Mesh, Sphere, Capsule, Box, Wheel };

struct MeshData {
    std::vector<Vector3f> vertices;
    std::vector<std::vector<int32_t>> triangleLists;  // one list per MeshTriangles entry (a submesh each)
};

// One collider component as the chunk stores it (before any transform is applied).
struct ColliderChunk {
    EntryType entry = EntryType::BoxCollider;
    ColliderKind kind = ColliderKind::Box;
    bool trigger = false;
    bool convexFlagInFile = false;   // Mesh: the stored flag (the component is convex either way)
    bool convex = false;             // Mesh: always true after PartReader
    float radius = 0.5f;             // Sphere, Capsule
    float height = 2.0f;             // Capsule (legacy chunk: Unity default 2)
    int32_t direction = 1;           // Capsule: 0 = X, 1 = Y, 2 = Z axis
    Vector3f center;                 // Sphere, Capsule, Box
    Vector3f size{1.0f, 1.0f, 1.0f}; // Box
    MeshData mesh;                   // Mesh (collider meshes only)
};

struct Node {
    std::string name;
    Vector3f position;
    Quaternionf rotation = Quaternionf::identity();
    Vector3f scale{1.0f, 1.0f, 1.0f};
    bool hasTagAndLayer = false;
    std::string tag = "Untagged";    // Unity default tag
    int32_t layer = 0;
    std::vector<ColliderChunk> colliders;
    std::vector<Node> children;
};

struct Model {
    int32_t fileType = 0;
    int32_t version = 0;
    std::string header;  // the string after the version (PartReader.cs:179, unused)
    Node root;
    size_t TotalColliders() const;
};

// PartReader.Read on a byte buffer / file. Throws std::runtime_error on a malformed or truncated file.
Model ReadModel(const uint8_t* data, size_t size);
Model ReadModelFile(const std::string& path);

}  // namespace ksp::mu
