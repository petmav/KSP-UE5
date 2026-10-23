// WP-29d unit tests: the model data of the 13 stock wheel / gear / leg parts (the WheelCollider and Animation chunks of the .mu reader, the
// part model rig: Part.FindModelTransform, part-space poses, Animation sampling, animated colliders).
//
// Gate: tools/wp29d/wheel_model_reference.py (an independent python parse of the ORIGINAL .mu files, numpy float32 per operation) writes
// tests/data/wp29d_models_reference.json: per part every transform's local TRS bits, tag / layer, collider kinds, the WheelCollider chunk (18
// float bit patterns), the Animation components (clips, curves, FNV-1a of the key bytes), the part-space pose of every transform
// (PartLoader.CompileModel placement) and 6 samples of every clip (Unity's cached Hermite cubic, rotations renormalised). The tests compare
// bit for bit against the committed reduced models (core/data/wheel_models, origin and sha256 of the originals in the manifest); when the
// originals are staged (core/data/mu_staging_wheels, local only) they are parsed too and must equal the reduced content.
#include <bit>
#include <cmath>
#include <functional>
#include <cstring>
#include <filesystem>
#include <fstream>

#include "doctest/doctest.h"
#include "ksp/parts/mu_model.hpp"
#include "ksp/parts/part_colliders.hpp"
#include "ksp/parts/part_model_rig.hpp"
#include "ksp/physics/wheel_models.hpp"
#include "nlohmann/json.hpp"

using namespace ksp;
using nlohmann::json;

namespace {

const std::string kData = std::string(KSPCORE_TEST_DATA_DIR) + "/";

const json& Ref() {
    static const json ref = [] {
        std::ifstream f(kData + "wp29d_models_reference.json", std::ios::binary);
        REQUIRE(f.good());
        return json::parse(f);
    }();
    return ref;
}

const physics::WheelModelLibrary& Lib() {
    static const physics::WheelModelLibrary lib(std::string(KSPCORE_DATA_DIR) + "/wheel_models");
    return lib;
}

uint32_t Bits(float f) {
    uint32_t u;
    std::memcpy(&u, &f, 4);
    return u;
}
uint32_t HexBits(const json& j) { return static_cast<uint32_t>(std::stoul(j.get<std::string>(), nullptr, 16)); }
uint64_t Fnv(const void* data, size_t n) {
    uint64_t h = 0xcbf29ce484222325ull;
    const uint8_t* p = static_cast<const uint8_t*>(data);
    for (size_t i = 0; i < n; ++i) h = (h ^ p[i]) * 0x100000001b3ull;
    return h;
}
uint64_t Hex64(const json& j) { return std::stoull(j.get<std::string>(), nullptr, 16); }

size_t g_compared = 0;
#define CHECK_BITS(label, value, hexjson)                                                  \
    do {                                                                                   \
        ++g_compared;                                                                      \
        CHECK_MESSAGE(Bits(value) == HexBits(hexjson), (label) << " got " << (value));       \
    } while (0)

void PoseBits(const mu::RigPose& p, std::vector<uint32_t>& out) {
    out.insert(out.end(), {Bits(p.position.x), Bits(p.position.y), Bits(p.position.z), Bits(p.rotation.x), Bits(p.rotation.y), Bits(p.rotation.z),
                           Bits(p.rotation.w), Bits(p.scale.x), Bits(p.scale.y), Bits(p.scale.z)});
}

// the nodes of a rig in pre-order, as the reference lists them
void PreOrder(const mu::PartModelRig& rig, int i, const std::string& parentPath, std::vector<std::pair<int, std::string>>& out) {
    const std::string path = parentPath.empty() ? rig.At(i).name : parentPath + "/" + rig.At(i).name;
    out.emplace_back(i, path);
    for (int c : rig.At(i).children) PreOrder(rig, c, path, out);
}

const char* KindName(mu::ColliderKind k) {
    switch (k) {
        case mu::ColliderKind::Mesh: return "mesh";
        case mu::ColliderKind::Sphere: return "sphere";
        case mu::ColliderKind::Capsule: return "capsule";
        case mu::ColliderKind::Box: return "box";
        default: return "wheel";
    }
}

float RescaleOf(const json& part) { return std::bit_cast<float>(HexBits(part.at("rescale"))); }

}  // namespace

TEST_CASE("WP-29d: the reference covers the 13 stock wheel, gear and leg parts") {
    const json& parts = Ref().at("parts");
    CHECK(parts.size() == 13);
    CHECK(Lib().PartCount() == 13);
    for (const auto& [name, p] : parts.items()) {
        CHECK_MESSAGE(Lib().Has(name), name);
        CHECK(p.at("mu").size() >= 1);
        for (const json& m : p.at("mu")) {
            CHECK(m.at("origin").get<std::string>().rfind("GameData/", 0) == 0);
            CHECK(m.at("originalSha256").get<std::string>().size() == 64);
        }
    }
}

TEST_CASE("WP-29d: transforms, tags, collider chunks, WheelCollider chunks and part-space poses equal the reference bit for bit") {
    g_compared = 0;
    size_t wheelChunks = 0, nodesSeen = 0;
    for (const auto& [name, p] : Ref().at("parts").items()) {
        auto rig = Lib().BuildRig(name, RescaleOf(p));
        REQUIRE_MESSAGE(rig != nullptr, name);
        std::vector<std::pair<int, std::string>> order;
        PreOrder(*rig, 0, std::string(), order);
        const json& nodes = p.at("nodes");
        REQUIRE_MESSAGE(order.size() == nodes.size(), name);
        for (size_t k = 0; k < order.size(); ++k) {
            const json& j = nodes[k];
            const mu::RigNode& n = rig->At(order[k].first);
            const std::string label = name + ":" + order[k].second;
            ++nodesSeen;
            CHECK_MESSAGE(order[k].second == j.at("path").get<std::string>(), label);
            CHECK_BITS(label, n.lp.x, j["pos"][0]);
            CHECK_BITS(label, n.lp.y, j["pos"][1]);
            CHECK_BITS(label, n.lp.z, j["pos"][2]);
            CHECK_BITS(label, n.lr.x, j["rot"][0]);
            CHECK_BITS(label, n.lr.y, j["rot"][1]);
            CHECK_BITS(label, n.lr.z, j["rot"][2]);
            CHECK_BITS(label, n.lr.w, j["rot"][3]);
            CHECK_BITS(label, n.ls.x, j["scl"][0]);
            CHECK_BITS(label, n.ls.y, j["scl"][1]);
            CHECK_BITS(label, n.ls.z, j["scl"][2]);
            const mu::RigPose ps = rig->PartSpace(order[k].first);
            std::vector<uint32_t> got;
            PoseBits(ps, got);
            for (size_t c = 0; c < 10; ++c) {
                ++g_compared;
                CHECK_MESSAGE(got[c] == HexBits(j["ps"][c]), label << " part-space component " << c);
            }
            if (n.src == nullptr) {
                CHECK_MESSAGE(!j.contains("tag"), label);  // the synthetic "model" of a MODEL-node part
                continue;
            }
            if (j.at("tag").is_null()) {
                CHECK_MESSAGE(!n.src->hasTagAndLayer, label);
            } else {
                CHECK_MESSAGE(n.src->hasTagAndLayer, label);
                CHECK_MESSAGE(n.src->tag == j.at("tag").get<std::string>(), label);
                CHECK_MESSAGE(n.src->layer == j.at("layer").get<int>(), label);
            }
            const json& cols = j.at("colliders");  // the reference lists the shape chunks; the WheelCollider chunk is its own field
            std::vector<const mu::ColliderChunk*> shapes;
            size_t wheelIdx = 0;
            for (size_t c = 0; c < n.src->colliders.size(); ++c) {
                if (n.src->colliders[c].kind == mu::ColliderKind::Wheel) wheelIdx = c;
                else shapes.push_back(&n.src->colliders[c]);
            }
            CHECK_MESSAGE(shapes.size() == cols.size(), label);
            for (size_t c = 0; c < std::min(cols.size(), shapes.size()); ++c) {
                CHECK_MESSAGE(std::string(KindName(shapes[c]->kind)) == cols[c].at("kind").get<std::string>(), label);
                CHECK_MESSAGE(shapes[c]->trigger == cols[c].at("trigger").get<bool>(), label);
            }
            const bool hasWheel = !j.at("wheel").is_null();
            bool nodeHasWheel = false;
            for (const auto& c : n.src->colliders) nodeHasWheel |= c.kind == mu::ColliderKind::Wheel;
            CHECK_MESSAGE(nodeHasWheel == hasWheel, label);
            if (hasWheel && nodeHasWheel) {
                ++wheelChunks;
                const mu::WheelColliderChunk& w = n.src->colliders[wheelIdx].wheel;
                const json& wj = j["wheel"];
                CHECK_BITS(label, w.mass, wj[0]);
                CHECK_BITS(label, w.radius, wj[1]);
                CHECK_BITS(label, w.suspensionDistance, wj[2]);
                CHECK_BITS(label, w.center.x, wj[3]);
                CHECK_BITS(label, w.center.y, wj[4]);
                CHECK_BITS(label, w.center.z, wj[5]);
                CHECK_BITS(label, w.spring, wj[6]);
                CHECK_BITS(label, w.damper, wj[7]);
                CHECK_BITS(label, w.targetPosition, wj[8]);
                for (int q = 0; q < 5; ++q) {
                    CHECK_BITS(label, w.forwardFriction[q], wj[9 + q]);
                    CHECK_BITS(label, w.sidewaysFriction[q], wj[14 + q]);
                }
            }
        }
    }
    MESSAGE("nodes " << nodesSeen << ", wheel chunks " << wheelChunks << ", bit comparisons " << g_compared);
    CHECK(wheelChunks == 11);  // the LT-1 / LT-2 legs carry none: ModuleWheelBase finds no WheelCollider component and KSPWheelController.Create adds one
    CHECK(nodesSeen > 250);
}

TEST_CASE("WP-29d: Part.FindModelTransform is the pre-order search from \"model\"") {
    for (const auto& [name, p] : Ref().at("parts").items()) {
        auto rig = Lib().BuildRig(name, RescaleOf(p));
        REQUIRE(rig != nullptr);
        CHECK(rig->At(0).name == "model");
        CHECK(rig->Find("model") == 0);
        CHECK(rig->Find("no such transform") == -1);
        for (const auto& [key, tn] : p.at("names").items()) {
            const std::string t = tn.get<std::string>();
            const int i = rig->Find(t);
            CHECK_MESSAGE(i >= 0, name << " " << key << " = " << t);
            const auto all = rig->FindAll(t);
            CHECK_MESSAGE(!all.empty(), name << " " << t);
            if (i >= 0 && !all.empty()) CHECK(all.front() == i);
        }
    }
}

TEST_CASE("WP-29d: animation components, clips and curves equal the reference; samples are bit-exact") {
    size_t samples = 0, curves = 0;
    for (const auto& [name, p] : Ref().at("parts").items()) {
        auto rig = Lib().BuildRig(name, RescaleOf(p));
        REQUIRE(rig != nullptr);
        const json& anims = p.at("anims");
        REQUIRE_MESSAGE(rig->Animations().size() == anims.size(), name);
        std::vector<std::pair<int, std::string>> order;
        PreOrder(*rig, 0, std::string(), order);
        for (size_t a = 0; a < anims.size(); ++a) {
            const mu::RigAnimation& ra = rig->Animations()[a];
            const json& aj = anims[a];
            std::string ownerPath;
            for (const auto& [ni, path] : order)
                if (ni == ra.owner) ownerPath = path;
            CHECK(ownerPath == aj.at("owner").get<std::string>());
            CHECK(ra.chunk->defaultClip == aj.at("default").get<std::string>());
            CHECK(ra.chunk->playAutomatically == aj.at("auto").get<bool>());
            REQUIRE(ra.chunk->clips.size() == aj.at("clips").size());
            for (size_t c = 0; c < ra.chunk->clips.size(); ++c) {
                const mu::AnimClip& cl = ra.chunk->clips[c];
                const json& cj = aj["clips"][c];
                CHECK(cl.name == cj.at("name").get<std::string>());
                CHECK(cl.wrapMode == cj.at("wrap").get<int>());
                CHECK_BITS(name + " clip length", cl.Length(), cj.at("length"));
                REQUIRE(cl.curves.size() == cj.at("curves").size());
                for (size_t k = 0; k < cl.curves.size(); ++k) {
                    const mu::AnimCurve& cu = cl.curves[k];
                    const json& kj = cj["curves"][k];
                    ++curves;
                    CHECK(cu.path == kj.at("path").get<std::string>());
                    CHECK(cu.property == kj.at("prop").get<std::string>());
                    CHECK(cu.type == kj.at("type").get<int>());
                    CHECK(cu.preWrapMode == kj.at("pre").get<int>());
                    CHECK(cu.postWrapMode == kj.at("post").get<int>());
                    REQUIRE(cu.keys.size() == kj.at("n").get<size_t>());
                    std::vector<float> raw;
                    for (const mu::AnimKey& key : cu.keys) raw.insert(raw.end(), {key.time, key.value, key.inTangent, key.outTangent});
                    CHECK_MESSAGE(Fnv(raw.data(), raw.size() * 4) == Hex64(kj.at("fnv")), name << " " << cl.name << " curve " << k);
                }
            }
        }
        // samples, in the reference's order: per anim, per clip, per fraction
        const json& sj = p.at("samples");
        size_t si = 0;
        for (size_t a = 0; a < rig->Animations().size(); ++a) {
            for (size_t c = 0; c < rig->Animations()[a].chunk->clips.size(); ++c) {
                const float length = rig->Animations()[a].chunk->clips[c].Length();
                for (const json& fj : Ref().at("sampleFractions")) {
                    REQUIRE(si < sj.size());
                    const json& s = sj[si++];
                    ++samples;
                    const float t = std::bit_cast<float>(HexBits(fj)) * length;
                    CHECK_BITS(name + " sample time", t, s.at("t"));
                    rig->ResetLocal();
                    rig->SampleClip(static_cast<int>(a), static_cast<int>(c), t);
                    std::vector<uint32_t> all;
                    std::vector<std::pair<int, std::string>> ord;
                    PreOrder(*rig, 0, std::string(), ord);
                    for (const auto& [ni, path] : ord) PoseBits(rig->PartSpace(ni), all);
                    CHECK_MESSAGE(Fnv(all.data(), all.size() * 4) == Hex64(s.at("all")), name << " " << rig->Animations()[a].chunk->clips[c].name << " t=" << t);
                    for (const auto& [nm, bitsj] : s.at("named").items()) {
                        const int ni = rig->Find(nm);
                        REQUIRE(ni >= 0);
                        std::vector<uint32_t> got;
                        PoseBits(rig->PartSpace(ni), got);
                        for (size_t q = 0; q < 10; ++q) CHECK_MESSAGE(got[q] == HexBits(bitsj[q]), name << " " << nm << " sample " << si << " component " << q);
                    }
                }
                rig->ResetLocal();
            }
        }
        CHECK(si == sj.size());
    }
    MESSAGE("curves " << curves << ", samples " << samples);
    CHECK(samples == 7 * 6 * 1);  // 7 animated parts, one clip each, 6 fractions
}

TEST_CASE("WP-29d: a rig's colliders at the prefab pose equal BuildPartColliders; sampling moves the animated ones") {
    for (const auto& name : Lib().PartNames()) {
        auto rig = Lib().BuildRig(name, 1.25f);
        REQUIRE(rig != nullptr);
        const auto direct = mu::BuildPartColliders(1.25f, Lib().Instances(name));
        const auto viaRig = rig->Colliders();
        REQUIRE_MESSAGE(direct.size() == viaRig.size(), name);
        for (size_t i = 0; i < direct.size(); ++i) {
            CHECK(direct[i].path == viaRig[i].path);
            CHECK(Bits(direct[i].center.x) == Bits(viaRig[i].center.x));
            CHECK(Bits(direct[i].center.y) == Bits(viaRig[i].center.y));
            CHECK(Bits(direct[i].center.z) == Bits(viaRig[i].center.z));
            CHECK(Bits(direct[i].radius) == Bits(viaRig[i].radius));
            CHECK(direct[i].vertices.size() == viaRig[i].vertices.size());
        }
    }
    // a gear leg's collider under an animated transform moves when its clip is sampled
    auto rig = Lib().BuildRig("GearMedium", 1.0f);
    REQUIRE(rig != nullptr);
    REQUIRE(!rig->Animations().empty());
    rig->SampleClip(0, 0, 0.0f);
    const auto before = rig->Colliders();
    rig->SampleClip(0, 0, rig->Animations()[0].Length(rig->Animations()[0].chunk->clips[0].name));
    const auto after = rig->Colliders();
    REQUIRE(before.size() == after.size());
    size_t moved = 0;
    for (size_t i = 0; i < before.size(); ++i)
        if (before[i].center.x != after[i].center.x || before[i].center.y != after[i].center.y || before[i].center.z != after[i].center.z) ++moved;
    CHECK(moved >= 1);
}

TEST_CASE("WP-29d: the original staged models (local) hold exactly the reduced content") {
    namespace fs = std::filesystem;
    const fs::path staging = fs::path(KSPCORE_DATA_DIR) / "mu_staging_wheels";
    if (!fs::exists(staging / "MANIFEST.json")) {
        MESSAGE("core/data/mu_staging_wheels not present: skipped (local only)");
        return;
    }
    std::ifstream f(staging / "MANIFEST.json", std::ios::binary);
    const json man = json::parse(f);
    std::ifstream rf(fs::path(KSPCORE_DATA_DIR) / "wheel_models" / "MANIFEST.json", std::ios::binary);
    const json red = json::parse(rf);
    size_t files = 0;
    for (const auto& [name, pj] : man.items()) {
        const auto& models = red.at("parts").at(name).at("models");
        size_t mi = 0;
        for (const json& rec : pj.at("files")) {
            const std::string fn = rec.at("file").get<std::string>();
            if (fn.size() < 3 || fn.substr(fn.size() - 3) != ".mu") continue;
            const mu::Model orig = mu::ReadModelFile((staging / name / fn).string());
            const mu::Model reduced = mu::ReadModelFile((fs::path(KSPCORE_DATA_DIR) / "wheel_models" / models.at(mi++).at("file").get<std::string>()).string());
            ++files;
            std::function<void(const mu::Node&, const mu::Node&, const std::string&)> same = [&](const mu::Node& a, const mu::Node& b, const std::string& path) {
                CHECK_MESSAGE(a.name == b.name, path);
                CHECK(Bits(a.position.x) == Bits(b.position.x));
                CHECK(Bits(a.rotation.w) == Bits(b.rotation.w));
                CHECK(Bits(a.scale.y) == Bits(b.scale.y));
                CHECK(a.tag == b.tag);
                CHECK(a.layer == b.layer);
                CHECK(a.colliders.size() == b.colliders.size());
                CHECK(a.animations.size() == b.animations.size());
                REQUIRE(a.children.size() == b.children.size());
                for (size_t i = 0; i < a.children.size(); ++i) same(a.children[i], b.children[i], path + "/" + a.children[i].name);
            };
            same(orig.root, reduced.root, name);
                    CHECK(orig.TotalColliders() == reduced.TotalColliders());
        }
    }
    CHECK(files == 13);
}
