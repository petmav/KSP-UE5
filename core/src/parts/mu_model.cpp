// WP-mucol / WP-UE-05: .mu reader. See ksp/parts/mu_model.hpp. Every chunk follows Assembly-CSharp/PartReader.cs.
#include "ksp/parts/mu_model.hpp"

#include <cstring>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace ksp::mu {

static_assert(sizeof(Vector2f) == 8 && sizeof(Vector3f) == 12 && sizeof(Vector4f) == 16, "bulk float reads need packed vectors");

namespace {

// System.IO.BinaryReader over a buffer (little endian)
class Reader {
public:
    Reader(const uint8_t* d, size_t n) : d_(d), n_(n) {}
    bool eof() const { return p_ == n_; }
    void need(size_t k) const {
        if (k > n_ - p_) throw std::runtime_error("mu: read past the end of the file at offset " + std::to_string(p_));
    }
    int32_t i32() {
        need(4);
        int32_t v;
        std::memcpy(&v, d_ + p_, 4);
        p_ += 4;
        return v;
    }
    float f32() {
        need(4);
        float v;
        std::memcpy(&v, d_ + p_, 4);
        p_ += 4;
        return v;
    }
    bool boolean() {
        need(1);
        return d_[p_++] != 0;
    }
    uint8_t byte() {
        need(1);
        return d_[p_++];
    }
    void skip(size_t k) {
        need(k);
        p_ += k;
    }
    void skipFloats(size_t k) { skip(4 * k); }
    // k little-endian 4-byte values into dst (floats or int32s)
    void bulk4(void* dst, size_t k) {
        if (k > (n_ - p_) / 4) throw std::runtime_error("mu: read past the end of the file at offset " + std::to_string(p_));
        if (k) std::memcpy(dst, d_ + p_, 4 * k);
        p_ += 4 * k;
    }
    void bulk1(uint8_t* dst, size_t k) {
        need(k);
        if (k) std::memcpy(dst, d_ + p_, k);
        p_ += k;
    }
    Vector3f vec3() {
        const float x = f32(), y = f32(), z = f32();  // C# argument evaluation order: left to right
        return Vector3f(x, y, z);
    }
    // BinaryReader.ReadString: 7-bit encoded byte length, UTF-8
    std::string str() {
        uint32_t len = 0;
        int shift = 0;
        for (;;) {
            const uint8_t b = byte();
            len |= static_cast<uint32_t>(b & 0x7F) << shift;
            if (!(b & 0x80)) break;
            shift += 7;
            if (shift > 35) throw std::runtime_error("mu: bad string length");
        }
        need(len);
        std::string s(reinterpret_cast<const char*>(d_ + p_), len);
        p_ += len;
        return s;
    }
    size_t pos() const { return p_; }

private:
    const uint8_t* d_;
    size_t n_;
    size_t p_ = 0;
};

enum ShaderType : int32_t {
    kCustom, kDiffuse, kSpecular, kBumped, kBumpedSpecular, kEmissive, kEmissiveSpecular, kEmissiveBumpedSpecular, kAlphaCutout,
    kAlphaCutoutBumped, kAlpha, kAlphaSpecular, kAlphaUnlit, kUnlit, kParticleAlpha, kParticleAdditive, kBumpedSpecularMap
};

class Parser {
public:
    Parser(Reader& r, Model& m) : r_(r), m_(m) {}

    int32_t version = 0;

    Node ReadChild() {  // PartReader.cs:205
        Node node;
        node.name = r_.str();
        node.position = r_.vec3();
        {
            const float x = r_.f32(), y = r_.f32(), z = r_.f32(), w = r_.f32();
            node.rotation = Quaternionf(x, y, z, w);
        }
        node.scale = r_.vec3();
        while (!r_.eof()) {
            const int32_t et = r_.i32();
            switch (static_cast<EntryType>(et)) {
                case EntryType::ChildTransformStart:  // :216
                    node.children.push_back(ReadChild());
                    break;
                case EntryType::Animation:  // :219 (both fileVersion branches call ReadAnimation)
                    node.animations.push_back(ReadAnimation());
                    break;
                case EntryType::MeshCollider: {  // :229
                    ColliderChunk c = Chunk(EntryType::MeshCollider, ColliderKind::Mesh);
                    c.convexFlagInFile = r_.boolean();
                    c.convex = true;  // :233 `if (!convex) convex = true`
                    RenderMesh mesh = ReadMesh();
                    c.mesh.vertices = std::move(mesh.vertices);
                    c.mesh.triangleLists = std::move(mesh.triangleLists);
                    node.colliders.push_back(std::move(c));
                    break;
                }
                case EntryType::SphereCollider: {  // :240
                    ColliderChunk c = Chunk(EntryType::SphereCollider, ColliderKind::Sphere);
                    c.radius = r_.f32();
                    c.center = r_.vec3();
                    node.colliders.push_back(std::move(c));
                    break;
                }
                case EntryType::CapsuleCollider: {  // :247 (no height: Unity's default 2)
                    ColliderChunk c = Chunk(EntryType::CapsuleCollider, ColliderKind::Capsule);
                    c.radius = r_.f32();
                    c.direction = r_.i32();
                    c.center = r_.vec3();
                    node.colliders.push_back(std::move(c));
                    break;
                }
                case EntryType::BoxCollider: {  // :255
                    ColliderChunk c = Chunk(EntryType::BoxCollider, ColliderKind::Box);
                    c.size = r_.vec3();
                    c.center = r_.vec3();
                    node.colliders.push_back(std::move(c));
                    break;
                }
                case EntryType::MeshFilter:  // :262
                    node.meshFilters.push_back(ReadMesh());
                    break;
                case EntryType::MeshRenderer:  // :265
                    node.meshRenderers.push_back(ReadMeshRenderer());
                    break;
                case EntryType::SkinnedMeshRenderer:  // :268
                    node.skinnedRenderers.push_back(ReadSkinnedMeshRenderer());
                    break;
                case EntryType::Materials: {  // :271
                    const int32_t n = r_.i32();
                    ++m_.materialsChunkCount;
                    if (n > slotCount_)
                        m_.warnings.push_back("Materials chunk has " + std::to_string(n) + " entries but only " +
                                              std::to_string(slotCount_) + " material slots are referenced (PartReader.cs:277 would throw)");
                    for (int32_t i = 0; i < n; ++i) {
                        MaterialChunk mat = (version < 4) ? ReadMaterial() : ReadMaterial4();
                        if (static_cast<size_t>(i) >= m_.materials.size()) m_.materials.resize(static_cast<size_t>(i) + 1);
                        m_.materials[static_cast<size_t>(i)] = std::move(mat);
                    }
                    break;
                }
                case EntryType::Textures:  // :292
                    ReadTextures();
                    break;
                case EntryType::Light:  // :295
                    node.lights.push_back(ReadLight());
                    break;
                case EntryType::TagAndLayer:  // :298
                    node.tag = r_.str();
                    node.layer = r_.i32();
                    node.hasTagAndLayer = true;
                    break;
                case EntryType::MeshCollider2: {  // :301
                    ColliderChunk c = Chunk(EntryType::MeshCollider2, ColliderKind::Mesh);
                    c.trigger = r_.boolean();
                    c.convexFlagInFile = r_.boolean();
                    c.convex = true;  // :307
                    RenderMesh mesh = ReadMesh();
                    c.mesh.vertices = std::move(mesh.vertices);
                    c.mesh.triangleLists = std::move(mesh.triangleLists);
                    node.colliders.push_back(std::move(c));
                    break;
                }
                case EntryType::SphereCollider2: {  // :314
                    ColliderChunk c = Chunk(EntryType::SphereCollider2, ColliderKind::Sphere);
                    c.trigger = r_.boolean();
                    c.radius = r_.f32();
                    c.center = r_.vec3();
                    node.colliders.push_back(std::move(c));
                    break;
                }
                case EntryType::CapsuleCollider2: {  // :322
                    ColliderChunk c = Chunk(EntryType::CapsuleCollider2, ColliderKind::Capsule);
                    c.trigger = r_.boolean();
                    c.radius = r_.f32();
                    c.height = r_.f32();
                    c.direction = r_.i32();
                    c.center = r_.vec3();
                    node.colliders.push_back(std::move(c));
                    break;
                }
                case EntryType::BoxCollider2: {  // :332
                    ColliderChunk c = Chunk(EntryType::BoxCollider2, ColliderKind::Box);
                    c.trigger = r_.boolean();
                    c.size = r_.vec3();
                    c.center = r_.vec3();
                    node.colliders.push_back(std::move(c));
                    break;
                }
                case EntryType::WheelCollider: {  // :340: mass, radius, suspensionDistance, center, spring (3), two friction curves (5 each)
                    ColliderChunk c = Chunk(EntryType::WheelCollider, ColliderKind::Wheel);
                    WheelColliderChunk& w = c.wheel;
                    w.mass = r_.f32();
                    w.radius = r_.f32();
                    w.suspensionDistance = r_.f32();
                    w.center = r_.vec3();
                    w.spring = r_.f32();
                    w.damper = r_.f32();
                    w.targetPosition = r_.f32();
                    for (float& f : w.forwardFriction) f = r_.f32();
                    for (float& f : w.sidewaysFriction) f = r_.f32();
                    node.colliders.push_back(std::move(c));
                    break;
                }
                case EntryType::Camera:  // :372
                    node.cameras.push_back(ReadCamera());
                    break;
                case EntryType::ParticleEmitter:  // :375
                    node.particleEmitters.push_back(ReadParticles());
                    break;
                case EntryType::ChildTransformEnd:  // :378
                    return node;
                default:  // PartReader's switch has no case: nothing is consumed
                    ++m_.ignoredEntries[et];
                    break;
            }
        }
        return node;
    }

private:
    static ColliderChunk Chunk(EntryType e, ColliderKind k) {
        ColliderChunk c;
        c.entry = e;
        c.kind = k;
        return c;
    }

    // PartReader.cs:841 / 460 / 475 / 1061: matDummies grows to hold the id (a negative id throws in the game)
    void NoteMaterialSlot(int32_t id) {
        if (id < 0) throw std::runtime_error("mu: negative material id");
        if (id >= slotCount_) slotCount_ = id + 1;
    }

    // PartReader.cs:495 ReadMesh: every array is kept; collider meshes take vertices + triangle lists out of the result.
    RenderMesh ReadMesh() {
        RenderMesh mesh;
        if (static_cast<EntryType>(r_.i32()) != EntryType::MeshStart) throw std::runtime_error("mu: Mesh Error");
        const int32_t n = r_.i32();
        mesh.headerInt = r_.i32();
        if (n < 0) throw std::runtime_error("mu: negative vertex count");
        mesh.vertexCount = n;
        const size_t un = static_cast<size_t>(n);
        for (;;) {
            const EntryType et = static_cast<EntryType>(r_.i32());
            if (et == EntryType::MeshEnd) break;
            switch (et) {
                case EntryType::MeshVertexColors:  // :519
                    r_.need(4 * un);
                    mesh.colors32.resize(4 * un);
                    r_.bulk1(mesh.colors32.data(), 4 * un);
                    mesh.hasColors = true;
                    break;
                case EntryType::MeshVerts:  // :529
                    r_.need(12 * un);
                    mesh.vertices.resize(un);
                    r_.bulk4(mesh.vertices.data(), 3 * un);
                    mesh.hasVertices = true;
                    break;
                case EntryType::MeshNormals:  // :559
                    r_.need(12 * un);
                    mesh.normals.resize(un);
                    r_.bulk4(mesh.normals.data(), 3 * un);
                    mesh.hasNormals = true;
                    break;
                case EntryType::MeshUV:  // :539
                    r_.need(8 * un);
                    mesh.uv.resize(un);
                    r_.bulk4(mesh.uv.data(), 2 * un);
                    mesh.hasUv = true;
                    break;
                case EntryType::MeshUV2:  // :549
                    r_.need(8 * un);
                    mesh.uv2.resize(un);
                    r_.bulk4(mesh.uv2.data(), 2 * un);
                    mesh.hasUv2 = true;
                    break;
                case EntryType::MeshTangents:  // :569
                    r_.need(16 * un);
                    mesh.tangents.resize(un);
                    r_.bulk4(mesh.tangents.data(), 4 * un);
                    mesh.hasTangents = true;
                    break;
                case EntryType::MeshTriangles: {  // :579
                    const int32_t k = r_.i32();
                    if (k < 0) throw std::runtime_error("mu: negative triangle count");
                    r_.need(4 * static_cast<size_t>(k));
                    std::vector<int32_t> t(static_cast<size_t>(k));
                    r_.bulk4(t.data(), static_cast<size_t>(k));
                    mesh.triangleLists.push_back(std::move(t));
                    break;
                }
                case EntryType::MeshBoneWeights: {  // :595: 4 x (int index, float weight) per vertex
                    r_.need(32 * un);
                    mesh.boneIndices.assign(4 * un, 0);
                    mesh.boneWeights.assign(4 * un, 0.0f);
                    for (size_t i = 0; i < un; ++i)
                        for (size_t j = 0; j < 4; ++j) {
                            mesh.boneIndices[4 * i + j] = r_.i32();
                            mesh.boneWeights[4 * i + j] = r_.f32();
                        }
                    mesh.hasBoneWeights = true;
                    break;
                }
                case EntryType::MeshBindPoses: {  // :613
                    const int32_t k = r_.i32();
                    if (k < 0) throw std::runtime_error("mu: negative bind pose count");
                    r_.need(64 * static_cast<size_t>(k));
                    mesh.bindPoses.resize(16 * static_cast<size_t>(k));
                    r_.bulk4(mesh.bindPoses.data(), 16 * static_cast<size_t>(k));
                    mesh.hasBindPoses = true;
                    break;
                }
                default:  // no case in PartReader's switch: nothing consumed
                    ++m_.ignoredMeshEntries[static_cast<int32_t>(et)];
                    break;
            }
        }
        return mesh;
    }

    MeshRendererChunk ReadMeshRenderer() {  // :441
        MeshRendererChunk c;
        if (version >= 1) {
            c.hasShadowFields = true;
            c.castShadows = r_.boolean();  // :446
            c.receiveShadows = r_.boolean();
        }
        const int32_t n = r_.i32();
        for (int32_t i = 0; i < n; ++i) {
            const int32_t id = r_.i32();
            NoteMaterialSlot(id);
            c.materialSlots.push_back(id);
        }
        return c;
    }

    SkinnedRendererChunk ReadSkinnedMeshRenderer() {  // :468
        SkinnedRendererChunk c;
        const int32_t n = r_.i32();
        for (int32_t i = 0; i < n; ++i) {
            const int32_t id = r_.i32();
            NoteMaterialSlot(id);
            c.materialSlots.push_back(id);
        }
        for (float& f : c.localBounds) f = r_.f32();  // :481 centre then size
        c.quality = r_.i32();
        c.updateWhenOffscreen = r_.boolean();
        const int32_t nb = r_.i32();
        for (int32_t i = 0; i < nb; ++i) c.boneNames.push_back(r_.str());
        c.mesh = ReadMesh();
        return c;
    }

    // PartReader.cs:790 ReadMaterialTexture: id, scale (x, y), offset (x, y)
    MaterialProperty TexProp(const char* name) {
        MaterialProperty p;
        p.name = name;
        p.kind = PropertyKind::Texture;
        p.textureId = r_.i32();
        if (p.textureId < -1) {
            m_.warnings.push_back("texture id " + std::to_string(p.textureId) + " (PartReader.cs:113 would throw)");
        } else if (p.textureId + 1 > textureSlots_) {
            textureSlots_ = p.textureId + 1;  // TextureDummyList.AddTextureDummy grows to textureID + 1
        }
        p.texScale[0] = r_.f32();
        p.texScale[1] = r_.f32();
        p.texOffset[0] = r_.f32();
        p.texOffset[1] = r_.f32();
        return p;
    }
    MaterialProperty ColProp(const char* name) {  // ReadColor :1085
        MaterialProperty p;
        p.name = name;
        p.kind = PropertyKind::Color;
        for (float& f : p.value) f = r_.f32();
        return p;
    }
    MaterialProperty FloatProp(const char* name) {
        MaterialProperty p;
        p.name = name;
        p.kind = PropertyKind::Float;
        p.value[0] = r_.f32();
        return p;
    }

    // PartReader.cs:646 (fileVersion < 4): a fixed table of shader types
    MaterialChunk ReadMaterial() {
        MaterialChunk mat;
        mat.name = r_.str();
        const int32_t st = r_.i32();
        mat.legacyShaderType = st;
        auto& P = mat.properties;
        switch (st) {
            default:  // :653 Custom and every unknown value
                mat.shaderName = "KSP/Diffuse";
                P.push_back(TexProp("_MainTex"));
                break;
            case kSpecular:  // :657
                mat.shaderName = "KSP/Specular";
                P.push_back(TexProp("_MainTex"));
                P.push_back(ColProp("_SpecColor"));
                P.push_back(FloatProp("_Shininess"));
                break;
            case kBumped:  // :663
                mat.shaderName = "KSP/Bumped";
                P.push_back(TexProp("_MainTex"));
                P.push_back(TexProp("_BumpMap"));
                break;
            case kBumpedSpecular:  // :668
                mat.shaderName = "KSP/Bumped Specular";
                P.push_back(TexProp("_MainTex"));
                P.push_back(TexProp("_BumpMap"));
                P.push_back(ColProp("_SpecColor"));
                P.push_back(FloatProp("_Shininess"));
                break;
            case kEmissive:  // :675
                mat.shaderName = "KSP/Emissive/Diffuse";
                P.push_back(TexProp("_MainTex"));
                P.push_back(TexProp("_Emissive"));
                P.push_back(ColProp("_EmissiveColor"));  // PropertyIDs._EmissiveColor = PropertyToID("_EmissiveColor")
                break;
            case kEmissiveSpecular:  // :681
                mat.shaderName = "KSP/Emissive/Specular";
                P.push_back(TexProp("_MainTex"));
                P.push_back(ColProp("_SpecColor"));
                P.push_back(FloatProp("_Shininess"));
                P.push_back(TexProp("_Emissive"));
                P.push_back(ColProp("_EmissiveColor"));
                break;
            case kEmissiveBumpedSpecular:  // :689
                mat.shaderName = "KSP/Emissive/Bumped Specular";
                P.push_back(TexProp("_MainTex"));
                P.push_back(TexProp("_BumpMap"));
                P.push_back(ColProp("_SpecColor"));
                P.push_back(FloatProp("_Shininess"));
                P.push_back(TexProp("_Emissive"));
                P.push_back(ColProp("_EmissiveColor"));
                break;
            case kAlphaCutout:  // :698
                mat.shaderName = "KSP/Alpha/Cutoff";
                P.push_back(TexProp("_MainTex"));
                P.push_back(FloatProp("_Cutoff"));
                break;
            case kAlphaCutoutBumped:  // :703
                mat.shaderName = "KSP/Alpha/Cutoff Bumped";
                P.push_back(TexProp("_MainTex"));
                P.push_back(TexProp("_BumpMap"));
                P.push_back(FloatProp("_Cutoff"));
                break;
            case kAlpha:  // :709
                mat.shaderName = "KSP/Alpha/Translucent";
                P.push_back(TexProp("_MainTex"));
                break;
            case kAlphaSpecular:  // :713
                mat.shaderName = "KSP/Alpha/Translucent Specular";
                P.push_back(TexProp("_MainTex"));
                P.push_back(FloatProp("_Gloss"));
                P.push_back(ColProp("_SpecColor"));
                P.push_back(FloatProp("_Shininess"));
                break;
            case kAlphaUnlit:  // :720
                mat.shaderName = "KSP/Alpha/Unlit Transparent";
                P.push_back(TexProp("_MainTex"));
                P.push_back(ColProp("_Color"));
                break;
            case kUnlit:  // :725
                mat.shaderName = "KSP/Unlit";
                P.push_back(TexProp("_MainTex"));
                P.push_back(ColProp("_Color"));
                break;
            case kParticleAlpha:  // :730
                mat.shaderName = "KSP/Particles/Alpha Blended";
                P.push_back(TexProp("_MainTex"));
                P.push_back(ColProp("_Color"));
                P.push_back(FloatProp("_InvFade"));
                break;
            case kParticleAdditive:  // :736
                mat.shaderName = "KSP/Particles/Additive";
                P.push_back(TexProp("_MainTex"));
                P.push_back(ColProp("_Color"));
                P.push_back(FloatProp("_InvFade"));
                break;
            case kBumpedSpecularMap:  // :742
                mat.shaderName = "KSP/Bumped Specular (Mapped)";
                P.push_back(TexProp("_MainTex"));
                P.push_back(TexProp("_BumpMap"));
                P.push_back(TexProp("_SpecMap"));
                P.push_back(FloatProp("_SpecTint"));
                P.push_back(FloatProp("_Shininess"));
                break;
        }
        return mat;
    }

    // PartReader.cs:758 (fileVersion >= 4): name, shader name, count, then (name, ShaderPropertyType, value)
    MaterialChunk ReadMaterial4() {
        MaterialChunk mat;
        mat.name = r_.str();
        mat.shaderName = r_.str();
        const int32_t n = r_.i32();
        for (int32_t i = 0; i < n; ++i) {
            const std::string pname = r_.str();
            const int32_t kind = r_.i32();
            switch (kind) {  // ShaderPropertyType: Color, Vector, Float, Range, TexEnv; any other value reads nothing (no default case)
                case 0:
                case 1: {
                    MaterialProperty p;
                    p.name = pname;
                    p.kind = kind == 0 ? PropertyKind::Color : PropertyKind::Vector;  // ReadColor :1085 / ReadVector4 :1100
                    for (float& f : p.value) f = r_.f32();
                    mat.properties.push_back(std::move(p));
                    break;
                }
                case 2:
                case 3: {
                    MaterialProperty p;
                    p.name = pname;
                    p.kind = kind == 2 ? PropertyKind::Float : PropertyKind::Range;
                    p.value[0] = r_.f32();
                    mat.properties.push_back(std::move(p));
                    break;
                }
                case 4: {
                    MaterialProperty p = TexProp(pname.c_str());
                    mat.properties.push_back(std::move(p));
                    break;
                }
                default:
                    m_.warnings.push_back("material '" + mat.name + "' property '" + pname + "' has unknown ShaderPropertyType " +
                                          std::to_string(kind) + " (no value read)");
                    break;
            }
        }
        return mat;
    }

    void ReadTextures() {  // :385
        const int32_t n = r_.i32();
        if (n != textureSlots_)  // PartReader logs "TextureError" and returns: the stream would be misaligned
            throw std::runtime_error("mu: TextureError " + std::to_string(n) + " " + std::to_string(textureSlots_));
        ++m_.texturesChunkCount;
        m_.textures.assign(static_cast<size_t>(n), TextureRef());
        for (int32_t i = 0; i < n; ++i) {
            TextureRef& t = m_.textures[static_cast<size_t>(i)];
            t.file = r_.str();
            t.name = FileNameWithoutExtension(t.file);  // :395
            t.type = r_.i32();                           // :396
        }
    }

    AnimationChunk ReadAnimation() {  // :802 (all clips are kept; the legacy Animation component is rebuilt by the host)
        AnimationChunk a;
        const int32_t clips = r_.i32();
        bool flag = false;  // :806
        for (int32_t i = 0; i < clips; ++i) {
            AnimClip clip;
            clip.name = r_.str();
            for (int b = 0; b < 6; ++b) clip.bounds[b] = r_.f32();  // localBounds centre, size
            clip.wrapMode = r_.i32();
            const int32_t curves = r_.i32();
            for (int32_t j = 0; j < curves; ++j) {
                AnimCurve c;
                c.path = r_.str();
                c.property = r_.str();
                c.type = r_.i32();  // AnimationType (PartToolsLib: Transform, Material, Light, AudioSource, ParticleEmitter)
                c.preWrapMode = r_.i32();
                c.postWrapMode = r_.i32();
                const int32_t keys = r_.i32();
                if (keys < 0) throw std::runtime_error("mu: negative key count");
                for (int32_t k = 0; k < keys; ++k) {
                    AnimKey key;
                    key.time = r_.f32();
                    key.value = r_.f32();
                    key.inTangent = r_.f32();
                    key.outTangent = r_.f32();
                    key.unused = r_.i32();
                    c.keys.push_back(key);
                }
                if (c.type < 0 || c.type > 3) flag = true;  // :851: no Unity type for the value -> :857 flag = true
                clip.curves.push_back(std::move(c));
            }
            clip.droppedByKsp = flag;  // :859 `if (!flag) animation.AddClip`
            a.clips.push_back(std::move(clip));
        }
        a.defaultClip = r_.str();
        a.defaultClipApplied = !a.defaultClip.empty() && !flag;  // :865
        a.playAutomatically = r_.boolean();
        return a;
    }

    LightChunk ReadLight() {  // :950
        LightChunk l;
        l.type = r_.i32();
        l.intensity = r_.f32();
        l.range = r_.f32();
        for (float& f : l.color) f = r_.f32();
        l.cullingMask = r_.i32();
        if (version > 1) {
            l.hasSpotAngle = true;
            l.spotAngle = r_.f32();
        }
        return l;
    }

    CameraChunk ReadCamera() {  // :970
        CameraChunk c;
        c.clearFlags = r_.i32();
        for (float& f : c.backgroundColor) f = r_.f32();
        c.cullingMask = r_.i32();
        c.orthographic = r_.boolean();
        c.fieldOfView = r_.f32();
        c.nearClipPlane = r_.f32();
        c.farClipPlane = r_.f32();
        c.depth = r_.f32();
        return c;
    }

    ParticleEmitterChunk ReadParticles() {  // :985
        r_.boolean();
        r_.i32();
        r_.skipFloats(3 + 2 + 1);  // shape3D, shape2D, shape1D
        r_.skipFloats(4);          // color
        r_.boolean();
        r_.skipFloats(4);  // minSize, maxSize, minEnergy, maxEnergy
        r_.i32();
        r_.i32();
        r_.skipFloats(3 * 3);  // world / local / random velocity
        r_.skipFloats(3);      // emitterVelocityScale, angularVelocity, rndAngularVelocity
        r_.boolean();
        r_.boolean();
        r_.skipFloats(4 * 5);  // colorAnimation[5]
        r_.skipFloats(6);      // world / local rotation axis
        r_.skipFloats(1 + 3 + 3 + 1);  // sizeGrow, rndForce, force, damping
        r_.boolean();
        r_.boolean();
        r_.skipFloats(3);  // lengthScale, velocityScale, maxParticleSize
        r_.i32();          // render mode
        r_.i32();
        r_.i32();
        r_.i32();          // uvAnimation x / y / cycles
        ParticleEmitterChunk p;
        p.materialSlot = r_.i32();  // :1060 material index
        NoteMaterialSlot(p.materialSlot);
        return p;
    }

    Reader& r_;
    Model& m_;
    int32_t slotCount_ = 0;     // matDummies.Count
    int32_t textureSlots_ = 0;  // textureDummies.Count
};

size_t CountColliders(const Node& n) {
    size_t c = n.colliders.size();
    for (const Node& ch : n.children) c += CountColliders(ch);
    return c;
}

// Preorder ids, then the bone names of the skinned renderers (PartReader.cs:165-178 + FindChildByName :1068: the first node of a
// preorder walk from the root whose name matches)
void AssignIds(Node& n, int32_t& next, std::vector<Node*>& order) {
    n.id = next++;
    order.push_back(&n);
    for (Node& c : n.children) AssignIds(c, next, order);
}

}  // namespace

size_t Model::TotalColliders() const { return CountColliders(root); }

size_t Model::NodeCount() const {
    size_t n = 0;
    std::vector<const Node*> stack{&root};
    while (!stack.empty()) {
        const Node* x = stack.back();
        stack.pop_back();
        ++n;
        for (const Node& c : x->children) stack.push_back(&c);
    }
    return n;
}

std::string FileNameWithoutExtension(const std::string& path) {
    size_t start = 0;
    for (size_t i = 0; i < path.size(); ++i)
        if (path[i] == '/' || path[i] == '\\') start = i + 1;
    std::string file = path.substr(start);
    const size_t dot = file.rfind('.');
    if (dot != std::string::npos) file.resize(dot);
    return file;
}

Model ReadModel(const uint8_t* data, size_t size) {
    Reader r(data, size);
    Model m;
    m.fileType = r.i32();
    m.version = r.i32();
    m.header = r.str();
    if (m.fileType != kFileTypeModelBinary) throw std::runtime_error("mu: incorrect file type");  // PartReader.cs:155
    Parser p(r, m);
    p.version = m.version;
    m.root = p.ReadChild();
    int32_t next = 0;
    std::vector<Node*> order;
    AssignIds(m.root, next, order);
    for (Node* n : order)
        for (SkinnedRendererChunk& s : n->skinnedRenderers)
            for (const std::string& bone : s.boneNames) {
                int32_t id = -1;
                for (const Node* c : order)
                    if (c->name == bone) {
                        id = c->id;
                        break;
                    }
                s.boneNodeIds.push_back(id);
            }
    return m;
}

Model ReadModelFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("mu: cannot open " + path);
    std::vector<uint8_t> buf((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    return ReadModel(buf.data(), buf.size());
}

// AnimationClip.length: the end time of the clip, the latest key time of any curve
float AnimClip::Length() const {
    float len = 0.0f;
    for (const AnimCurve& c : curves)
        for (const AnimKey& k : c.keys)
            if (k.time > len) len = k.time;
    return len;
}

}  // namespace ksp::mu
