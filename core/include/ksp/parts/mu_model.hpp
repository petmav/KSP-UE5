// WP-mucol / WP-29d / WP-UE-05: the .mu (PartTools binary model) reader: colliders, animation AND the render chunks.
//
// Spec: Assembly-CSharp/PartReader.cs (ReadChild :205 and the Read* helpers; PartToolsLib FileType 76543 = ModelBinary, EntryType.cs).
// The file is a tree of transforms (name, local position / rotation / scale) each followed by component chunks. This reader consumes
// EVERY chunk exactly as PartReader does (so the stream stays aligned) and keeps:
//   * the transform tree, the TagAndLayer chunk, the collider chunks (WP-mucol) and the WheelCollider / Animation chunks (WP-29d);
//   * the render chunks (WP-UE-05): MeshFilter meshes (vertices, normals, tangents, uv, uv2, vertex colours, submesh triangle
//     lists, bone weights, bind poses), MeshRenderer / SkinnedMeshRenderer (shadow flags, material slots, bounds, bone names), the
//     Materials chunk (shader name and every property: colour / vector / float / range / texture id + scale + offset; the
//     version < 4 fixed table is translated into the same property list), the Textures chunk (file name, type), Light, Camera.
//   * ParticleEmitter chunks are PARSED AND SKIPPED except their count and material slot (the shell maps the cfg's particle effects).
// Not kept (parsed and dropped, listed in core/STATUS.md): the ParticleEmitter values, Mesh.RecalculateBounds (derived), the int
// after each animation keyframe (kept raw in AnimKey::unused), the second int of MeshStart (kept in RenderMesh::headerInt).
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
//   :340 WheelCollider   mass, radius, suspensionDistance, center, spring / damper / targetPosition, forward + sideways friction curves
//                        (WP-29d keeps the values; enabled = false in the prefab: it never collides)
//   :802 Animation       (WP-29d keeps the clips: per curve the transform path, the property, the wrap modes and the keyframes)
// WP-29d: the wheel collider chunk (WheelCollider, PartReader.cs:340-370) and the Animation chunk (ReadAnimation :802) are kept.  The
// values of a wheel collider chunk are mostly overwritten by ModuleWheelBase.wheelSetup (mass, radius, centre, suspension) and by
// VPWheelCollider.SetupWheelCollider (the friction curves); the TRANSFORM that carries the chunk is what places the wheel.
// Unity defaults that the chunks do not set: isTrigger false; CapsuleCollider.height 2 (legacy chunk); everything a chunk sets is
// overwritten.  A malformed file throws std::runtime_error (PartReader.Read catches the exception, logs it and returns the partial
// GameObject: the game would load a truncated model, which no shipped file is).
//
// Material slots: a renderer lists material ids; the Materials chunk gives material i to every renderer (and particle emitter) that
// listed id i (PartReader.cs:273-289: `renderers[count].sharedMaterial = material`, i.e. KSP assigns sharedMaterial, the FIRST slot of
// each renderer, once per material in ascending id order: a renderer that lists several ids ends with the last material in its
// slot 0 [Unity behaviour of the setter, UNVERIFIED]; the reader keeps the raw ids and the whole material table, the shell decides).
// PartReader would throw (:277, matDummies[i]) when a Materials chunk is longer than the number of referenced slots, and for a
// texture id below -1 (:113): this reader accepts both and records a warning (Model::warnings).
//
// Asset origin: GameData/Squad/Parts/**/*.mu (and every other GameData .mu) of the dev copy decompiled/KSP-dev.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
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

// A whole Mesh as PartReader.ReadMesh (:495) builds it. A has* flag says the chunk was present (the array may still be empty when the
// vertex count is 0); a chunk that appears twice replaces the first (as the Mesh property setters do).
struct RenderMesh {
    int32_t vertexCount = 0;  // :504 num
    int32_t headerInt = 0;    // :505 the second int of MeshStart, read and dropped by PartReader
    bool hasVertices = false, hasNormals = false, hasTangents = false, hasUv = false, hasUv2 = false, hasColors = false,
         hasBoneWeights = false, hasBindPoses = false;
    std::vector<Vector3f> vertices;                   // :529
    std::vector<Vector3f> normals;                    // :559
    std::vector<Vector4f> tangents;                   // :569
    std::vector<Vector2f> uv;                         // :539
    std::vector<Vector2f> uv2;                        // :549
    std::vector<uint8_t> colors32;                    // :519 Color32 r, g, b, a per vertex (4 bytes each)
    std::vector<std::vector<int32_t>> triangleLists;  // :579 one list per MeshTriangles entry = the submeshes in order
    std::vector<int32_t> boneIndices;                 // :595 4 per vertex (boneIndex0..3)
    std::vector<float> boneWeights;                   // :595 4 per vertex (weight0..3)
    std::vector<float> bindPoses;                     // :613 16 floats per pose: m00 m01 m02 m03 m10 ... m33 (file order)
    size_t BindPoseCount() const { return bindPoses.size() / 16; }
};

// WheelCollider chunk (PartReader.cs:340): the values the prefab component is given before `enabled = false`
struct WheelColliderChunk {
    float mass = 0.0f, radius = 0.0f, suspensionDistance = 0.0f;
    Vector3f center;
    float spring = 0.0f, damper = 0.0f, targetPosition = 0.0f;       // JointSpring
    float forwardFriction[5] = {0, 0, 0, 0, 0};                      // extremumSlip, extremumValue, asymptoteSlip, asymptoteValue, stiffness
    float sidewaysFriction[5] = {0, 0, 0, 0, 0};
};

// Animation chunk (PartReader.cs:802 ReadAnimation): legacy clips of Transform / Material / Light / AudioSource curves
struct AnimKey {
    float time = 0.0f, value = 0.0f, inTangent = 0.0f, outTangent = 0.0f;
    int32_t unused = 0;  // the int32 after each keyframe, read and dropped by PartReader (:861)
};
enum class AnimationType : int32_t { Transform = 0, Material = 1, Light = 2, AudioSource = 3, ParticleEmitter = 4 };  // PartToolsLib order
struct AnimCurve {
    std::string path;      // relative to the transform that owns the Animation component ("" = the owner itself)
    std::string property;  // e.g. "m_LocalRotation.w", "m_LocalPosition.x", "m_LocalScale.y"
    int32_t type = 0;      // AnimationType
    int32_t preWrapMode = 0, postWrapMode = 0;  // UnityEngine.WrapMode values (8 = ClampForever)
    std::vector<AnimKey> keys;
};
struct AnimClip {
    std::string name;
    float bounds[6] = {0, 0, 0, 0, 0, 0};  // localBounds centre + size
    int32_t wrapMode = 0;
    // PartReader.cs:851-861: a curve whose AnimationType has no Unity type (4 = ParticleEmitter) sets `flag`; from then on the clip
    // being read and every later clip of the component are NOT added to the Animation (and the default clip is not set).
    bool droppedByKsp = false;
    std::vector<AnimCurve> curves;
    float Length() const;  // AnimationClip.length: the latest key time of any curve
};
struct AnimationChunk {
    std::vector<AnimClip> clips;
    std::string defaultClip;
    bool defaultClipApplied = false;  // :865 text3 != "" && !flag
    bool playAutomatically = false;
};

// ---- render chunks (WP-UE-05) ----------------------------------------------------------------------------------------

// PartReader.cs:441 ReadMeshRenderer: version >= 1 stores the two shadow booleans, otherwise the AddComponent defaults stay
struct MeshRendererChunk {
    bool hasShadowFields = false;
    bool castShadows = true;      // ShadowCastingMode.On (Unity default) / On when the stored bool is true, Off otherwise
    bool receiveShadows = true;   // Unity default true
    std::vector<int32_t> materialSlots;  // the ids of the Materials chunk entries this renderer uses
};

// PartReader.cs:468 ReadSkinnedMeshRenderer
struct SkinnedRendererChunk {
    std::vector<int32_t> materialSlots;
    float localBounds[6] = {0, 0, 0, 0, 0, 0};  // centre, size
    int32_t quality = 0;                        // SkinQuality
    bool updateWhenOffscreen = false;
    std::vector<std::string> boneNames;
    std::vector<int32_t> boneNodeIds;  // FindChildByName (:1068) from the root: preorder node id, -1 when the name is not found
    RenderMesh mesh;
};

enum class PropertyKind : int32_t { Color = 0, Vector = 1, Float = 2, Range = 3, Texture = 4 };  // PartReader.ShaderPropertyType

// One Material property. Colour / Vector use value[0..3]; Float / Range value[0]; Texture textureId (-1 = none), scale, offset
// (ReadMaterialTexture :790: id, scale x y, offset x y).
struct MaterialProperty {
    std::string name;
    PropertyKind kind = PropertyKind::Float;
    float value[4] = {0, 0, 0, 0};
    int32_t textureId = -1;
    float texScale[2] = {0, 0};
    float texOffset[2] = {0, 0};
};

struct MaterialChunk {
    std::string name;
    std::string shaderName;           // v >= 4: the stored name (Shader.Find); v < 4: the name the fixed table maps the ShaderType to
    int32_t legacyShaderType = -1;    // v < 4: PartToolsLib.ShaderType as stored (any value not in the table maps to KSP/Diffuse); -1 for v >= 4
    std::vector<MaterialProperty> properties;
};

struct TextureRef {
    std::string file;  // as stored (a file name, possibly with extension)
    std::string name;  // Path.GetFileNameWithoutExtension(file) (:395): the texture is `<model directory url>/<name>`
    int32_t type = 0;  // PartToolsLib.TextureType: 0 Texture, 1 NormalMap
};

struct LightChunk {  // :950
    int32_t type = 0;  // LightType
    float intensity = 0.0f, range = 0.0f;
    float color[4] = {0, 0, 0, 0};
    int32_t cullingMask = 0;
    bool hasSpotAngle = false;
    float spotAngle = 30.0f;  // Unity default; stored when fileVersion > 1
};

struct CameraChunk {  // :970
    int32_t clearFlags = 0;
    float backgroundColor[4] = {0, 0, 0, 0};
    int32_t cullingMask = 0;
    bool orthographic = false;
    float fieldOfView = 0.0f, nearClipPlane = 0.0f, farClipPlane = 0.0f, depth = 0.0f;
};

struct ParticleEmitterChunk {  // :985: only the material slot is kept (the other ~70 values are parsed and dropped)
    int32_t materialSlot = 0;
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
    WheelColliderChunk wheel;        // Wheel (WP-29d)
};

struct Node {
    std::string name;
    int32_t id = 0;        // preorder index in the file (root 0), assigned after the parse
    Vector3f position;
    Quaternionf rotation = Quaternionf::identity();
    Vector3f scale{1.0f, 1.0f, 1.0f};
    bool hasTagAndLayer = false;
    std::string tag = "Untagged";    // Unity default tag
    int32_t layer = 0;
    std::vector<ColliderChunk> colliders;
    std::vector<AnimationChunk> animations;  // Animation components on this transform (WP-29d)
    std::vector<RenderMesh> meshFilters;                 // MeshFilter chunks (WP-UE-05)
    std::vector<MeshRendererChunk> meshRenderers;
    std::vector<SkinnedRendererChunk> skinnedRenderers;
    std::vector<LightChunk> lights;
    std::vector<CameraChunk> cameras;
    std::vector<ParticleEmitterChunk> particleEmitters;
    std::vector<Node> children;
};

struct Model {
    int32_t fileType = 0;
    int32_t version = 0;
    std::string header;  // the string after the version (PartReader.cs:179, unused)
    Node root;
    // Materials chunks (:271): slot i = the i-th entry of the chunk (a later chunk overwrites the slots it covers), so the table is
    // indexed by the material ids the renderers list.
    std::vector<MaterialChunk> materials;
    int32_t materialsChunkCount = 0;
    std::vector<TextureRef> textures;  // Textures chunk (:385)
    int32_t texturesChunkCount = 0;
    // Entry type values that PartReader's ReadChild switch has no case for (consumed nothing): value -> occurrences
    std::map<int32_t, int32_t> ignoredEntries;
    std::map<int32_t, int32_t> ignoredMeshEntries;  // the same inside a mesh (ReadMesh's switch, :517)
    std::vector<std::string> warnings;  // things PartReader would throw on (see the header comment) and this reader accepted
    size_t TotalColliders() const;
    size_t NodeCount() const;
};

// PartReader.Read on a byte buffer / file. Throws std::runtime_error on a malformed or truncated file.
Model ReadModel(const uint8_t* data, size_t size);
Model ReadModelFile(const std::string& path);

// Path.GetFileNameWithoutExtension (the separators '/' and '\\')
std::string FileNameWithoutExtension(const std::string& path);

}  // namespace ksp::mu
