// WP-mucol: .mu reader. See ksp/parts/mu_model.hpp. Every chunk follows Assembly-CSharp/PartReader.cs.
#include "ksp/parts/mu_model.hpp"

#include <cstring>
#include <fstream>
#include <stdexcept>

namespace ksp::mu {

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
    explicit Parser(Reader& r) : r_(r) {}

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
                    ReadAnimation();
                    break;
                case EntryType::MeshCollider: {  // :229
                    ColliderChunk c = Chunk(EntryType::MeshCollider, ColliderKind::Mesh);
                    c.convexFlagInFile = r_.boolean();
                    c.convex = true;  // :233 `if (!convex) convex = true`
                    ReadMesh(&c.mesh);
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
                    ReadMesh(nullptr);
                    break;
                case EntryType::MeshRenderer:  // :265
                    ReadMeshRenderer();
                    break;
                case EntryType::SkinnedMeshRenderer:  // :268
                    ReadSkinnedMeshRenderer();
                    break;
                case EntryType::Materials: {  // :271
                    const int32_t n = r_.i32();
                    for (int32_t i = 0; i < n; ++i) {
                        if (version < 4) ReadMaterial(); else ReadMaterial4();
                    }
                    break;
                }
                case EntryType::Textures:  // :292
                    ReadTextures();
                    break;
                case EntryType::Light:  // :295
                    ReadLight();
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
                    ReadMesh(&c.mesh);
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
                case EntryType::WheelCollider:  // :340: mass, radius, suspensionDistance, center, spring (3), two friction curves (5 each)
                    r_.skipFloats(1 + 1 + 1 + 3 + 3 + 5 + 5);
                    node.colliders.push_back(Chunk(EntryType::WheelCollider, ColliderKind::Wheel));
                    break;
                case EntryType::Camera:  // :372
                    ReadCamera();
                    break;
                case EntryType::ParticleEmitter:  // :375
                    ReadParticles();
                    break;
                case EntryType::ChildTransformEnd:  // :378
                    return node;
                default:  // PartReader's switch has no case: nothing is consumed
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

    // PartReader.cs:495. The collider meshes keep vertices and triangle lists.
    void ReadMesh(MeshData* out) {
        if (static_cast<EntryType>(r_.i32()) != EntryType::MeshStart) throw std::runtime_error("mu: Mesh Error");
        const int32_t n = r_.i32();
        r_.i32();
        if (n < 0) throw std::runtime_error("mu: negative vertex count");
        for (;;) {
            const EntryType et = static_cast<EntryType>(r_.i32());
            if (et == EntryType::MeshEnd) break;
            switch (et) {
                case EntryType::MeshVertexColors:  // :519
                    r_.skip(4 * static_cast<size_t>(n));
                    break;
                case EntryType::MeshVerts:  // :529
                    if (out) {
                        out->vertices.resize(static_cast<size_t>(n));
                        for (int32_t i = 0; i < n; ++i) out->vertices[static_cast<size_t>(i)] = r_.vec3();
                    } else {
                        r_.skipFloats(3 * static_cast<size_t>(n));
                    }
                    break;
                case EntryType::MeshNormals:  // :559
                    r_.skipFloats(3 * static_cast<size_t>(n));
                    break;
                case EntryType::MeshUV:  // :539
                case EntryType::MeshUV2:  // :549
                    r_.skipFloats(2 * static_cast<size_t>(n));
                    break;
                case EntryType::MeshTangents:  // :569
                    r_.skipFloats(4 * static_cast<size_t>(n));
                    break;
                case EntryType::MeshTriangles: {  // :579
                    const int32_t k = r_.i32();
                    if (k < 0) throw std::runtime_error("mu: negative triangle count");
                    if (out) {
                        std::vector<int32_t> t(static_cast<size_t>(k));
                        for (int32_t i = 0; i < k; ++i) t[static_cast<size_t>(i)] = r_.i32();
                        out->triangleLists.push_back(std::move(t));
                    } else {
                        r_.skip(4 * static_cast<size_t>(k));
                    }
                    break;
                }
                case EntryType::MeshBoneWeights:  // :595: 4 x (int index, float weight)
                    r_.skip(32 * static_cast<size_t>(n));
                    break;
                case EntryType::MeshBindPoses:  // :613
                    r_.skipFloats(16 * static_cast<size_t>(r_.i32()));
                    break;
                default:
                    break;
            }
        }
    }

    void ReadMeshRenderer() {  // :441
        if (version >= 1) {
            r_.boolean();
            r_.boolean();
        }
        const int32_t n = r_.i32();
        for (int32_t i = 0; i < n; ++i) r_.i32();
    }

    void ReadSkinnedMeshRenderer() {  // :468
        const int32_t n = r_.i32();
        for (int32_t i = 0; i < n; ++i) r_.i32();
        r_.skipFloats(6);  // localBounds
        r_.i32();          // quality
        r_.boolean();      // updateWhenOffscreen
        const int32_t nb = r_.i32();
        for (int32_t i = 0; i < nb; ++i) r_.str();
        ReadMesh(nullptr);
    }

    void ReadMaterialTexture() {  // :790
        const int32_t id = r_.i32();
        if (id != -1 && id >= textureDummies_) textureDummies_ = id + 1;  // TextureDummyList.AddTextureDummy
        r_.skipFloats(4);
    }
    void Color() { r_.skipFloats(4); }
    void Float() { r_.skipFloats(1); }

    void ReadMaterial() {  // :646 (fileVersion < 4)
        r_.str();
        const int32_t st = r_.i32();
        switch (st) {
            default: ReadMaterialTexture(); break;
            case kSpecular: ReadMaterialTexture(); Color(); Float(); break;
            case kBumped: ReadMaterialTexture(); ReadMaterialTexture(); break;
            case kBumpedSpecular: ReadMaterialTexture(); ReadMaterialTexture(); Color(); Float(); break;
            case kEmissive: ReadMaterialTexture(); ReadMaterialTexture(); Color(); break;
            case kEmissiveSpecular: ReadMaterialTexture(); Color(); Float(); ReadMaterialTexture(); Color(); break;
            case kEmissiveBumpedSpecular: ReadMaterialTexture(); ReadMaterialTexture(); Color(); Float(); ReadMaterialTexture(); Color(); break;
            case kAlphaCutout: ReadMaterialTexture(); Float(); break;
            case kAlphaCutoutBumped: ReadMaterialTexture(); ReadMaterialTexture(); Float(); break;
            case kAlpha: ReadMaterialTexture(); break;
            case kAlphaSpecular: ReadMaterialTexture(); Float(); Color(); Float(); break;
            case kAlphaUnlit: ReadMaterialTexture(); Color(); break;
            case kUnlit: ReadMaterialTexture(); Color(); break;
            case kParticleAlpha: ReadMaterialTexture(); Color(); Float(); break;
            case kParticleAdditive: ReadMaterialTexture(); Color(); Float(); break;
            case kBumpedSpecularMap: ReadMaterialTexture(); ReadMaterialTexture(); ReadMaterialTexture(); Float(); Float(); break;
        }
    }

    void ReadMaterial4() {  // :758
        r_.str();
        r_.str();
        const int32_t n = r_.i32();
        for (int32_t i = 0; i < n; ++i) {
            r_.str();
            switch (r_.i32()) {  // ShaderPropertyType: Color, Vector, Float, Range, TexEnv
                case 0: case 1: r_.skipFloats(4); break;
                case 2: case 3: r_.skipFloats(1); break;
                case 4: ReadMaterialTexture(); break;
                default: break;
            }
        }
    }

    void ReadTextures() {  // :385
        const int32_t n = r_.i32();
        if (n != textureDummies_)  // PartReader logs "TextureError" and returns: the stream would be misaligned
            throw std::runtime_error("mu: TextureError " + std::to_string(n) + " " + std::to_string(textureDummies_));
        for (int32_t i = 0; i < n; ++i) {
            r_.str();
            r_.i32();
        }
    }

    void ReadAnimation() {  // :802
        const int32_t clips = r_.i32();
        for (int32_t i = 0; i < clips; ++i) {
            r_.str();
            r_.skipFloats(6);  // localBounds
            r_.i32();          // wrapMode
            const int32_t curves = r_.i32();
            for (int32_t j = 0; j < curves; ++j) {
                r_.str();
                r_.str();
                r_.i32();  // AnimationType
                r_.i32();  // preWrapMode
                r_.i32();  // postWrapMode
                const int32_t keys = r_.i32();
                for (int32_t k = 0; k < keys; ++k) {
                    r_.skipFloats(4);
                    r_.i32();
                }
            }
        }
        r_.str();
        r_.boolean();
    }

    void ReadLight() {  // :950
        r_.i32();
        r_.skipFloats(2);
        Color();
        r_.i32();
        if (version > 1) Float();
    }

    void ReadCamera() {  // :970
        r_.i32();
        Color();
        r_.i32();
        r_.boolean();
        r_.skipFloats(4);
    }

    void ReadParticles() {  // :985
        r_.boolean();
        r_.i32();
        r_.skipFloats(3 + 2 + 1);  // shape3D, shape2D, shape1D
        Color();
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
        r_.i32();          // material index
    }

    Reader& r_;
    int32_t textureDummies_ = 0;
};

size_t CountColliders(const Node& n) {
    size_t c = n.colliders.size();
    for (const Node& ch : n.children) c += CountColliders(ch);
    return c;
}

}  // namespace

size_t Model::TotalColliders() const { return CountColliders(root); }

Model ReadModel(const uint8_t* data, size_t size) {
    Reader r(data, size);
    Model m;
    m.fileType = r.i32();
    m.version = r.i32();
    m.header = r.str();
    if (m.fileType != kFileTypeModelBinary) throw std::runtime_error("mu: incorrect file type");  // PartReader.cs:155
    Parser p(r);
    p.version = m.version;
    m.root = p.ReadChild();
    return m;
}

Model ReadModelFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("mu: cannot open " + path);
    std::vector<uint8_t> buf((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    return ReadModel(buf.data(), buf.size());
}

}  // namespace ksp::mu
