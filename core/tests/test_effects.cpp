// WP-UE-16a unit tests: the EFFECTS system (fx/effects.hpp, fx/effect_classes.hpp, fx/fx_group.hpp) and its callers.
//
// Gate: tools/wpue16/effects_reference.py is an independent Python implementation written from the decompiled C# (it shares no code with the
// C++). It parses the EFFECTS nodes of every stock part cfg in the GameData mirror itself, replays scripted sequences over every effect (power
// sweeps, transform indices, one-shot events, pause / unpause, EVA, early events before OnInitialize, OnSave), scripted ModuleEnginesFX and
// ModuleRCSFX sequences and the legacy FXGroups of every part with fx_* / sound_* lines, and writes tests/data/wpue16_reference.json: per
// scenario a FNV-1a 64 digest over the canonical text rows of every step (published parameters as IEEE bit patterns: 0 ULP) plus the digest of
// every step. The C++ side builds the same lists from the part prefabs the loader produced (Part.effects), replays the same steps and compares.
// There is no in-game FX capture: this gate is unit-gated against the C# semantics only (see core/STATUS.md "WP-UE-16a").
//
// Fixtures (labelled): the inputs the game's assets provide (transform counts, clip existence / length, particle base values, prefab lookups) are
// hash functions of the names, identical in both implementations, because the mirror's .mu / prefab / audio files are stubs. The EFFECTS node
// with the classes no stock part uses (AUDIO_SINGLE, AUDIO_MULTI, PREFAB_MULTI_PARTICLE, the animation effects, PREFAB_SPAWN, PARTICLE_CONFIG,
// ROCSAUDIO, an unknown class) is a test fixture inside the reference JSON, not a part definition.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "doctest/doctest.h"
#include "nlohmann/json.hpp"
#include "resource_fixture.hpp"

#include "ksp/core/exceptions.hpp"
#include "ksp/fx/effect_classes.hpp"
#include "ksp/fx/effects.hpp"
#include "ksp/fx/fx_group.hpp"
#include "ksp/fx/fx_modules.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/vessel/vessel.hpp"
#include "ksp/input/game_settings.hpp"
#include "ksp/modules/module_engines.hpp"
#include "ksp/modules/module_rcs.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/space_center/high_logic.hpp"
#include "ksp/unity/mathf.hpp"

using nlohmann::json;
using namespace ksp;
using namespace ksp::rtest;

namespace {

const std::string kData = std::string(KSPCORE_TEST_DATA_DIR) + "/";

const json& Reference() {
    static const json ref = [] {
        std::ifstream f(kData + "wpue16_reference.json", std::ios::binary);
        REQUIRE(f.good());
        return json::parse(f);
    }();
    return ref;
}

uint64_t Fnv(const std::string& s, uint64_t h = 0xcbf29ce484222325ull) {
    for (unsigned char c : s) {
        h ^= c;
        h *= 0x100000001b3ull;
    }
    return h;
}
std::string Hex8(float f) {
    uint32_t b;
    std::memcpy(&b, &f, 4);
    char buf[16];
    std::snprintf(buf, sizeof buf, "%08x", b);
    return buf;
}
float FromHex(const std::string& s) {
    const uint32_t b = static_cast<uint32_t>(std::stoul(s, nullptr, 16));
    float f;
    std::memcpy(&f, &b, 4);
    return f;
}
std::string Hex16(uint64_t v) {
    char buf[32];
    std::snprintf(buf, sizeof buf, "%016llx", static_cast<unsigned long long>(v));
    return buf;
}
std::vector<std::string> Split(const std::string& s, char c) {
    std::vector<std::string> out;
    std::string cur;
    for (char ch : s) {
        if (ch == c) {
            out.push_back(cur);
            cur.clear();
        } else {
            cur += ch;
        }
    }
    out.push_back(cur);
    return out;
}

// ---- fixtures (the same functions as FIX_* of the reference) ------------------------------------------------------------------------------
std::string g_part;
uint64_t FnvStr(const std::string& s) { return Fnv(s); }

void InstallFixtures(bool eva) {
    EffectEnvironment::Reset();
    EffectEnvironment::findModelTransforms = [](Part&, const std::string& t) { return static_cast<int>(FnvStr(g_part + "|" + t) % 4); };
    EffectEnvironment::transformActiveInHierarchy = [](Part&, const std::string& t, int i) { return FnvStr(g_part + "|" + t + "|" + std::to_string(i)) % 5 != 0; };
    EffectEnvironment::audioClipExists = [](const std::string& c) { return FnvStr(c) % 13 != 0; };
    EffectEnvironment::audioClipLength = [](const std::string& c) { return static_cast<float>(0.25 * (1 + FnvStr(c) % 16)); };
    EffectEnvironment::modelEmitter = [](const std::string& name) {
        const uint64_t h = FnvStr(name);
        ModelEmitterInfo i;
        i.modelFound = h % 23 != 0;
        i.hasEmitter = h % 29 != 0;
        i.baseKnown = true;
        i.base.minEmission = static_cast<float>(10 + h % 50);
        i.base.maxEmission = static_cast<float>(i.base.minEmission + static_cast<double>((h >> 8) % 20));
        i.base.minEnergy = static_cast<float>(0.1 * (1 + (h >> 16) % 9));
        i.base.maxEnergy = static_cast<float>(static_cast<double>(i.base.minEnergy) + 0.5);
        i.base.localVelocity = Vector3f(static_cast<float>(((h >> 24) % 5) * 0.25), static_cast<float>(((h >> 28) % 5) * -0.5), static_cast<float>(((h >> 32) % 7) * 0.5));
        return i;
    };
    EffectEnvironment::prefabParticle = [](const std::string& name) {
        const uint64_t h = FnvStr(name);
        PrefabParticleInfo i;
        i.found = h % 17 != 0;
        i.hasParticleSystem = h % 19 != 0;
        i.baseKnown = true;
        i.burstCount = static_cast<int>((h >> 3) & 1);
        i.rateOverTime = static_cast<float>(20 + (h >> 5) % 40);
        i.startLifetimeMin = static_cast<float>(0.2 * (1 + (h >> 11) % 6));
        i.startLifetimeMax = static_cast<float>(static_cast<double>(i.startLifetimeMin) + 0.75);
        i.velocityOverLifetime = Vector3f(static_cast<float>(((h >> 24) % 5) * 0.25), static_cast<float>(((h >> 28) % 5) * -0.5), static_cast<float>(((h >> 32) % 7) * 0.5));
        return i;
    };
    EffectEnvironment::prefabExists = [](const std::string& n) { return FnvStr(n) % 17 != 0; };
    EffectEnvironment::findModelAnimator = [](Part&, const std::string& t, const std::string& clip) { return FnvStr(g_part + "|" + t + "|" + clip) % 3 != 0; };
    EffectEnvironment::findEffectAnimation = [](const std::string& n, const std::string& a, const std::string& c) { return FnvStr(n + "|" + a + "|" + c) % 5 != 0; };
    if (eva) EffectEnvironment::vesselIsEVA = [](Part&) { return true; };
    GameSettings::SHIP_VOLUME = 0.75f;
    GameSettings::VOICE_VOLUME = 0.5f;
    GameSettings::AMBIENCE_VOLUME = 0.625f;
    GameSettings::MUSIC_VOLUME = 0.4f;
    GameSettings::UI_VOLUME = 0.9f;
    FXGroupEnvironment::randomRange = [](float min, float) { return min; };
    FXGroupEnvironment::fxPrefabParticleSystems = [](const std::string& k) {
        const uint64_t h = FnvStr(k);
        FXParticleSystem p;
        p.startSizeMin = static_cast<float>(0.1 + static_cast<double>(h % 5) * 0.1);
        p.startSizeMax = static_cast<float>(0.5 + static_cast<double>(h % 7) * 0.25);
        p.startLifetimeMin = static_cast<float>(0.5 + static_cast<double>((h >> 8) % 5) * 0.5);
        p.startLifetimeMax = static_cast<float>(1.0 + static_cast<double>((h >> 12) % 5) * 0.5);
        return std::vector<FXParticleSystem>{p};
    };
}

// ---- rows (the canonical text of the reference) -------------------------------------------------------------------------------------------
std::string SrcRow(const AudioSourceState& s) {
    std::ostringstream o;
    o << "src " << s.id << "|" << s.clip << "|" << (s.loop ? 1 : 0) << "|" << Hex8(s.volume) << "|" << Hex8(s.pitch) << "|" << Hex8(s.spatialBlend) << "|"
      << Hex8(s.maxDistance) << "|" << (s.playing ? 1 : 0) << "|" << (s.paused ? 1 : 0) << "|" << s.transformIndex << "|" << Hex8(s.elapsed);
    return o.str();
}
std::string EmRow(const EmitterState& e) {
    std::ostringstream o;
    o << "em " << e.index << "|" << (e.emit ? 1 : 0) << "|" << (e.playing ? 1 : 0) << "|" << (e.burstMode ? 1 : 0) << "|" << (e.resolved ? 1 : 0) << "|" << e.minEmission
      << "|" << e.maxEmission << "|" << Hex8(e.minEnergy) << "|" << Hex8(e.maxEnergy) << "|" << Hex8(e.velocity.x) << "|" << Hex8(e.velocity.y) << "|" << Hex8(e.velocity.z)
      << "|" << Hex8(e.emissionPower) << "|" << Hex8(e.energyPower) << "|" << Hex8(e.speedPower);
    return o.str();
}
// the state rows: per instance the header, floats, strings, sources, emitters (the reference sorts its rows into the same order)
void StateRows(EffectList& lst, std::vector<std::string>& rows) {
    for (const EffectInstanceState& s : lst.Snapshot()) {
        rows.push_back("inst " + s.effectName + "|" + s.instanceName + "|" + s.className);
        for (const auto& f : s.floats) rows.push_back("f " + f.first + " " + Hex8(f.second));
        for (const auto& t : s.strings) rows.push_back("s " + t.first + "=" + t.second);
        for (const auto& src : s.sources) rows.push_back(SrcRow(src));
        for (const auto& e : s.emitters) rows.push_back(EmRow(e));
    }
}
void EventRows(EffectList& lst, std::vector<std::string>& rows) {
    for (const EffectEvent& e : lst.TakeEvents()) {
        rows.push_back("ev " + e.kind + "|" + e.effectName + "|" + e.instanceName + "|" + e.className + "|" + std::to_string(e.transformIdx) + "|" +
                       std::to_string(e.emitterIndex) + "|" + (e.hasPower ? "1" : "0") + "|" + Hex8(e.power));
    }
}

struct Hasher {
    uint64_t h = 0xcbf29ce484222325ull;
    std::string stepDigests;
    void Step(const std::vector<std::string>& rows) {
        for (const std::string& r : rows) h = Fnv(r + "\n", h);
        char buf[16];
        std::snprintf(buf, sizeof buf, "%08x", static_cast<uint32_t>(h & 0xFFFFFFFFull));
        stepDigests += buf;
    }
};

// compare the digest string of one scenario; on a mismatch name the first step
void CheckScenario(const std::string& part, const std::string& scen, const Hasher& hs, const json& expect) {
    const std::string want = expect.at("steps_digest").get<std::string>();
    if (hs.stepDigests == want && Hex16(hs.h) == expect.at("digest").get<std::string>()) {
        CHECK(true);
        return;
    }
    size_t first = 0;
    while ((first + 1) * 8 <= want.size() && (first + 1) * 8 <= hs.stepDigests.size() && want.compare(first * 8, 8, hs.stepDigests, first * 8, 8) == 0) first++;
    INFO(part << " / " << scen << ": first differing step " << first << " (python --dump " << part << " " << scen << ")");
    CHECK(hs.stepDigests == want);
    CHECK(Hex16(hs.h) == expect.at("digest").get<std::string>());
}

// one step of the list scripts: i (Initialize), e <effect> <power bits|-> <idx>, u <dt bits>, p (pause), r (unpause)
void ListStep(EffectList& lst, const std::string& step, std::vector<std::string>& rows) {
    const std::vector<std::string> t = Split(step, ' ');
    try {
        if (t[0] == "i") lst.Initialize();
        else if (t[0] == "e") {
            if (t[2] == "-") lst.Event(t[1], std::stoi(t[3]));
            else lst.Event(t[1], FromHex(t[2]), std::stoi(t[3]));
        } else if (t[0] == "u") lst.Update(FromHex(t[1]));
        else if (t[0] == "p") lst.OnGamePause();
        else if (t[0] == "r") lst.OnGameUnpause();
    } catch (const std::out_of_range&) {
        rows.push_back("exc IndexError");
    } catch (const NullReferenceException&) {
        rows.push_back("exc RuntimeError");
    }
    rows.push_back("step " + step);
    StateRows(lst, rows);
    EventRows(lst, rows);
}

struct Source {
    std::vector<ConfigNode::NodePtr> nodes;
    std::unique_ptr<Part> host;
    std::unique_ptr<EffectList> list;
};

}  // namespace

TEST_CASE("WP-UE-16a FXCurve forms: the cfg lines of FXCurve.Load and the Hermite evaluation") {
    FXCurve none("volume", 1.0f);
    CHECK(none.Value(0.3f) == 1.0f);  // no line: evalSingle with the constructor's value
    auto node = ConfigNode::Parse("volume = 0.5\n");
    none.Load("volume", *node);
    CHECK(none.Value(7.0f) == 0.5f);
    CHECK(none.implicitFloat() == 0.5f);
    // ONE two-token line yields the first token (the time): a C# quirk
    FXCurve one("pitch", 1.0f);
    one.Load("pitch", *ConfigNode::Parse("pitch = 0.25 3\n"));
    CHECK(one.Value(0.9f) == 0.25f);
    // two lines: keyframes with zero tangents (smoothstep between keys), ClampForever outside
    FXCurve c("volume", 1.0f);
    c.Load("volume", *ConfigNode::Parse("volume = 0.0 0.0\nvolume = 1.0 2.0\n"));
    CHECK_FALSE(c.evalSingle);
    CHECK(c.Value(-5.0f) == 0.0f);
    CHECK(c.Value(0.5f) == 1.0f);  // smoothstep midpoint of 0 -> 2
    CHECK(c.Value(9.0f) == 2.0f);
    CHECK(c.Value(0.25f) == doctest::Approx(0.3125f).epsilon(1e-6));  // 2 * (3 * 0.0625 - 2 * 0.015625)
    // an invalid line (one token) is skipped
    FXCurve d("volume", 1.0f);
    d.Load("volume", *ConfigNode::Parse("volume = 0.0 1.0\nvolume = 7\nvolume = 1.0 3.0\n"));
    CHECK(d.keyFrames.size() == 2);
    // Save
    ConfigNode out("n");
    c.Save(out);
    CHECK(out.values().Count() == 2);
    CHECK(*out.GetValue("volume", 1) == "1 2");
}

TEST_CASE("WP-UE-16a EffectList: loading, naming rules, unknown classes, OnSave, Instantiate copy") {
    InstallFixtures(false);
    g_part = "unit";
    Part host;
    EffectList lst(&host);
    auto node = ConfigNode::Parse(
        "running\n{\n AUDIO\n {\n  clip = a\n  channel = Ambient\n }\n AUDIO\n {\n  clip = b\n }\n NOT_A_CLASS\n {\n }\n AUDIO\n {\n  name = third\n  clip = c\n }\n}\n"
        "power\n{\n NOT_A_CLASS\n {\n }\n}\n");
    lst.OnLoad(*node);
    // two unnamed AUDIO nodes under one effect share ONE behaviour (a null instanceName matches any effect of the class): the second load overwrites
    REQUIRE(lst.entries().size() == 2);
    CHECK(lst.entries()[0].second.size() == 2);
    auto* a = dynamic_cast<AudioFX*>(lst.entries()[0].second[0].get());
    REQUIRE(a != nullptr);
    CHECK(a->clip == "b");
    CHECK(a->channel == AudioChannel::Ambient);  // not reset by the second node (no `channel` value there)
    CHECK_FALSE(a->instanceName.has_value());
    CHECK(lst.loadErrors.size() == 2);
    CHECK(lst.EffectsStartingWith("ow") == std::vector<std::string>{"power"});
    lst.PruneEmpty();  // `power` has no behaviours
    CHECK(lst.entries().size() == 1);
    // OnSave: effect node, class node, `name` only when non-empty, then the persistent fields
    ConfigNode saved("EFFECTS");
    lst.OnSave(saved);
    REQUIRE(saved.nodes().Count() == 1);
    ConfigNode* r = saved.nodes()[0];
    CHECK(r->name == "running");
    REQUIRE(r->nodes().Count() == 2);
    CHECK(r->nodes()[0]->name == "AUDIO");
    CHECK_FALSE(r->nodes()[0]->HasValue("name"));
    CHECK(*r->nodes()[0]->GetValue("channel") == "Ambient");
    CHECK(*r->nodes()[0]->GetValue("loop") == "False");
    CHECK(*r->nodes()[1]->GetValue("name") == "third");
    // an Instantiate copy has the serialized fields and fresh private state
    Part host2;
    EffectList copy(&host2);
    copy.CopyFrom(lst);
    REQUIRE(copy.entries().size() == 1);
    auto* a2 = dynamic_cast<AudioFX*>(copy.entries()[0].second[0].get());
    REQUIRE(a2 != nullptr);
    CHECK(a2->clip == "b");
    CHECK(a2->owner == &copy);
    EffectEnvironment::Reset();
}

TEST_CASE("WP-UE-16a effects of every stock part and the fixture EFFECTS node match the independent reference (0 ULP, every step)") {
    if (!GameAvailable()) return;
    const json& ref = Reference();
    Game15 g;
    const json& totals = ref.at("totals");
    int partsChecked = 0, listScenarios = 0, saveScenarios = 0, loaderChecked = 0;
    std::map<std::string, int> behaviours;
    for (const json& entry : ref.at("parts")) {
        const std::string rel = entry.at("part").get<std::string>();
        if (!entry.contains("effects")) continue;
        g_part = rel;
        if (std::getenv("WPUE16_TRACE")) std::fprintf(stderr, "part %s\n", rel.c_str());
        std::vector<ConfigNode::NodePtr> nodes;
        ConfigNode::NodePtr fixtureRoot, cfgRoot;
        if (entry.contains("fixture_cfg")) {
            fixtureRoot = ConfigNode::Parse(entry.at("fixture_cfg").get<std::string>());
            REQUIRE(fixtureRoot->nodes().Count() == 1);
            nodes.push_back(fixtureRoot->nodes().Shared(0));
        } else {
            // the EFFECTS nodes straight from the mirror cfg (PART number k of the file; the loader does not load the zDeprecated parts)
            const size_t hash = rel.find('#');
            const std::string file = rel.substr(0, hash);
            const int which = hash == std::string::npos ? 0 : std::stoi(rel.substr(hash + 1));
            cfgRoot = ConfigNode::Load(std::string(KSPCORE_TESTDATA_DIR) + "/GameData-mirror/GameData/" + file);
            REQUIRE_MESSAGE(cfgRoot != nullptr, file);
            int seen = 0;
            ConfigNode* partNode = nullptr;
            for (int i = 0; i < cfgRoot->nodes().Count(); i++)
                if (cfgRoot->nodes()[i]->name == "PART" && seen++ == which) partNode = cfgRoot->nodes()[i];
            REQUIRE_MESSAGE(partNode != nullptr, rel);
            for (int i = 0; i < partNode->nodes().Count(); i++)
                if (partNode->nodes()[i]->name == "EFFECTS") nodes.push_back(partNode->nodes().Shared(i));
            // the loader keeps the same EFFECTS nodes on the prefab (Part.effects) for every part it loads
            if (const AvailablePart* ap = g.data.parts.getPartInfoByName(entry.at("name").get<std::string>())) {
                REQUIRE(ap->partPrefab != nullptr);
                CHECK(ap->partPrefab->effects.size() == nodes.size());
                loaderChecked++;
            }
        }
        partsChecked++;
        for (const auto& kv : entry.at("scenarios").items()) {
            const std::string scen = kv.key();
            const json& sj = kv.value();
            if (std::getenv("WPUE16_TRACE")) std::fprintf(stderr, "  scenario %s\n", scen.c_str());
            if (scen == "sweep" || scen == "early" || scen == "eva" || scen == "loading") {
                const bool eva = scen == "eva";
                InstallFixtures(eva);
                SaveEnvironment::Current().loadedScene = scen == "loading" ? GameScenes::LOADING : GameScenes::FLIGHT;
                AudioMultiPooledFX::ResetPool();
                EffectEnvironment::nextSourceId = 1;
                Part host;
                EffectList lst(&host);
                for (const auto& n : nodes) lst.OnLoad(*n);
                lst.PruneEmpty();
                Hasher hs;
                for (const json& st : sj.at("steps")) {
                    std::vector<std::string> rows;
                    ListStep(lst, st.get<std::string>(), rows);
                    hs.Step(rows);
                }
                CheckScenario(rel, scen, hs, sj);
                listScenarios++;
                if (scen == "sweep") {
                    for (const auto& b : entry.at("behaviours")) behaviours[b.at(1).get<std::string>()]++;
                    // the loaded behaviours equal the reference's (names, classes, instance names)
                    std::vector<std::vector<std::string>> mine;
                    for (const auto& e : lst.entries())
                        for (const auto& b : e.second) mine.push_back({b->effectName, b->nodeName(), b->instanceName.value_or("<null>")});
                    std::vector<std::vector<std::string>> want;
                    for (const auto& b : entry.at("behaviours")) want.push_back({b.at(0).get<std::string>(), b.at(1).get<std::string>(), b.at(2).is_null() ? "<null>" : b.at(2).get<std::string>()});
                    CHECK(mine == want);
                }
                AudioMultiPooledFX::ResetPool();
            } else if (scen == "save") {
                InstallFixtures(false);
                Part host;
                EffectList lst(&host);
                for (const auto& n : nodes) lst.OnLoad(*n);
                lst.PruneEmpty();
                ConfigNode out("EFFECTS");
                lst.OnSave(out);
                uint64_t h = 0xcbf29ce484222325ull;
                size_t rows = 0;
                for (int i = 0; i < out.nodes().Count(); i++) {
                    h = Fnv("node " + out.nodes()[i]->name + "\n", h);
                    rows++;
                    for (int j = 0; j < out.nodes()[i]->nodes().Count(); j++) {
                        ConfigNode* c = out.nodes()[i]->nodes()[j];
                        h = Fnv(" node " + c->name + "\n", h);
                        rows++;
                        for (const auto& v : c->values()) {
                            h = Fnv("  v " + v.name + "=" + v.value + "\n", h);
                            rows++;
                        }
                    }
                }
                INFO(rel << " save");
                CHECK(Hex16(h) == sj.at("digest").get<std::string>());
                CHECK(rows == sj.at("rows").get<size_t>());
                saveScenarios++;
            }
        }
    }
    CHECK(partsChecked == static_cast<int>(totals.at("parts_with_effects").get<int>()));
    CHECK(partsChecked == 53);  // 52 stock parts + the fixture
    CHECK(listScenarios == partsChecked * 4);
    CHECK(saveScenarios == partsChecked);
    CHECK(loaderChecked > 40);
    MESSAGE("effects: " << partsChecked << " parts (" << loaderChecked << " also through the loader's Part.effects), " << listScenarios << " list scenarios + " << saveScenarios
                          << " OnSave texts, " << totals.at("effects").get<int>() << " effect names, " << totals.at("rows").get<long long>() << " rows hashed in the reference run");
    for (const auto& b : totals.at("behaviours").items()) CHECK(behaviours[b.key()] == b.value().get<int>());
    // every stock EFFECTS class node loads: the only unknown class is the fixture's
    CHECK(totals.at("unknown_classes").size() == 1);
    CHECK(totals.at("unknown_classes").contains("UNKNOWN_CLASS"));
    EffectEnvironment::Reset();
}

TEST_CASE("WP-UE-16a legacy FXGroups of every stock part with fx_* / sound_* lines match the reference") {
    if (!GameAvailable()) return;
    const json& ref = Reference();
    Game15 g;
    int parts = 0;
    int skipped = 0;
    int groupsChecked = 0;
    for (const json& entry : ref.at("parts")) {
        if (!entry.at("scenarios").contains("groups")) continue;
        const json& sj = entry.at("scenarios").at("groups");
        g_part = entry.at("part").get<std::string>();
        if (std::getenv("WPUE16_TRACE")) std::fprintf(stderr, "groups %s\n", g_part.c_str());
        InstallFixtures(false);
        if (g.data.parts.getPartInfoByName(entry.at("name").get<std::string>()) == nullptr) {  // zDeprecated parts are not loaded
            CHECK(g_part.find("zDeprecated") != std::string::npos);
            skipped++;
            continue;
        }
        // a bare Part carrying the loader's fx_* / sound_* definitions of the prefab (Part.OnLoad adds the four default groups); the modules'
        // Awake (the EVA kerbal's included) is not needed for the group construction
        const AvailablePart* info = g.data.parts.getPartInfoByName(entry.at("name").get<std::string>());
        REQUIRE(info != nullptr);
        auto partOwner = std::make_unique<Part>();
        Part* part = partOwner.get();
        part->OnLoad();
        part->fxDefinitions = info->partPrefab->fxDefinitions;
        part->soundDefinitions = info->partPrefab->soundDefinitions;
        Hasher hs;
        for (const json& stj : sj.at("steps")) {
            const std::string step = stj.get<std::string>();
            const std::vector<std::string> t = Split(step, ' ');
            std::vector<std::string> rows;
            if (t[0] == "b") {
                part->BeginFXGroups();
            } else {
                FXGroup* grp = part->FindFXGroup(t[1]);
                REQUIRE_MESSAGE(grp != nullptr, g_part << " " << t[1]);
                const std::string& op = t[2];
                if (op == "a") grp->setActive(t[3] == "1");
                else if (op == "p") grp->SetPower(FromHex(t[3]));
                else if (op == "pl") grp->SetPowerLatch(FromHex(t[3]));
                else if (op == "burst") grp->Burst();
                else if (op == "mm") grp->SetVisualMinMax(FromHex(t[3]), FromHex(t[4]));
                else if (op == "latch") grp->SetLatch(true);
                else if (op == "var") grp->powerVariation = FromHex(t[3]);
                else if (op == "unlatch") grp->Unlatch();
            }
            rows.push_back("step " + step);
            for (const json& nj : sj.at("order")) {
                const FXGroupState s = part->FindFXGroup(nj.get<std::string>())->Snapshot();
                std::ostringstream o;
                o << "grp " << s.name << "|" << (s.valid ? 1 : 0) << "|" << (s.active ? 1 : 0) << "|" << (s.activeLatch ? 1 : 0) << "|" << (s.clampVisualPower ? 1 : 0)
                  << "|" << Hex8(s.power) << "|" << Hex8(s.powerVariation) << "|" << Hex8(s.minVisualPower) << "|" << Hex8(s.maxVisualPower) << "|" << s.sfx << "|"
                  << (s.hasAudio ? 1 : 0) << "|" << s.oneShots;
                rows.push_back(o.str());
                for (const auto& p : s.emitters) {
                    std::ostringstream q;
                    q << " ps " << (p.playing ? 1 : 0) << "|" << p.playCount << "|" << Hex8(p.startSizeMin) << "|" << Hex8(p.startSizeMax) << "|" << Hex8(p.startLifetimeMin)
                      << "|" << Hex8(p.startLifetimeMax) << "|" << (p.twoConstants ? 1 : 0);
                    rows.push_back(q.str());
                }
                for (const auto& l : s.lights) rows.push_back(std::string(" li ") + (l.enabled ? "1" : "0") + "|" + Hex8(l.intensity));
                if (s.hasAudio) rows.push_back(" " + SrcRow(s.audio));
                groupsChecked++;
            }
            hs.Step(rows);
        }
        CheckScenario(g_part, "groups", hs, sj);
        parts++;
    }
    MESSAGE("legacy FXGroups: " << parts << " parts, " << groupsChecked << " group snapshots compared, " << skipped << " zDeprecated parts not loaded by the loader");
    CHECK(parts + skipped == ref.at("totals").at("parts_with_groups").get<int>());
    CHECK(skipped > 0);  // only the zDeprecated parts the loader does not load
    CHECK(groupsChecked > 0);
    EffectEnvironment::Reset();
}

TEST_CASE("WP-UE-16a ModuleEnginesFX and ModuleRCSFX drive the part's effects: the call sequence and every published parameter match the reference") {
    if (!GameAvailable()) return;
    const json& ref = Reference();
    Game15 g;
    int engines = 0, rcs = 0, engineSteps = 0, skippedModules = 0;
    for (const json& entry : ref.at("parts")) {
        if (entry.contains("fixture_cfg")) continue;  // the fixture part has no modules
        const json& sc = entry.at("scenarios");
        const std::string rel = entry.at("part").get<std::string>();
        for (const char* key : {"engine_resp", "engine_direct", "rcs"}) {
            if (!sc.contains(key)) continue;
            const json& sj = sc.at(key);
            const bool isRcs = std::string(key) == "rcs";
            g_part = rel;
            InstallFixtures(false);
            SaveEnvironment::Current().loadedScene = GameScenes::EDITOR;
            AudioMultiPooledFX::ResetPool();
            EffectEnvironment::nextSourceId = 1;
            if (g.data.parts.getPartInfoByName(entry.at("name").get<std::string>()) == nullptr) {  // zDeprecated parts are not loaded
                CHECK(rel.find("zDeprecated") != std::string::npos);
                skippedModules++;
                continue;
            }
            auto part = g.New(entry.at("name").get<std::string>().c_str());
            ModuleEnginesFX* eng = nullptr;
            ModuleRCSFX* rcsMod = nullptr;
            for (PartModule* m : part->Modules().GetModules([](PartModule&) { return true; })) {
                if (!eng) eng = dynamic_cast<ModuleEnginesFX*>(m);
                if (!rcsMod) rcsMod = dynamic_cast<ModuleRCSFX*>(m);
            }
            std::vector<std::string> calls;
            Part::effectCallHook = [&](Part&, const std::string& name, bool hasPower, float power, int idx) {
                calls.push_back("call " + name + "|" + (hasPower ? Hex8(power) : std::string("-")) + "|" + std::to_string(idx));
            };
            part->InitializeEffects();
            part->Effects().TakeEvents();
            calls.clear();
            if (!isRcs) {
                REQUIRE_MESSAGE(eng != nullptr, rel);
                eng->OnStart(StartState::Landed);
                eng->Set<bool>("useEngineResponseTime", std::string(key) == "engine_resp");
                calls.clear();
            } else {
                REQUIRE_MESSAGE(rcsMod != nullptr, rel);
            }
            Hasher hs;
            for (const json& stj : sj.at("steps")) {
                const std::string step = stj.get<std::string>();
                const std::vector<std::string> t = Split(step, ' ');
                std::vector<std::string> rows;
                if (t[0] == "m") {
                    if (!isRcs) {
                        if (t[1] == "activate") eng->Activate();
                        else if (t[1] == "shutdown") eng->Shutdown();
                        else if (t[1] == "flameout") eng->Flameout("flameout", t.size() > 2);
                        else if (t[1] == "unflameout") eng->UnFlameout();
                        else if (t[1] == "set") {
                            eng->Set<float>("currentThrottle", FromHex(t[2]));
                            eng->requestedThrottle = FromHex(t[3]);
                            eng->Set<float>("finalThrust", FromHex(t[4]));
                            eng->FXUpdate(FromHex(t[5]));
                        }
                    } else {
                        if (t[1] == "up") rcsMod->UpdatePowerFX(t[4] == "1", std::stoi(t[2]), FromHex(t[3]));
                        else if (t[1] == "deact") rcsMod->DeactivateFX();
                        else if (t[1] == "deactpower") rcsMod->DeactivatePowerFX();
                    }
                } else if (t[0] == "u") {
                    part->Effects().Update(FromHex(t[1]));
                }
                rows = calls;
                calls.clear();
                rows.push_back("step " + step);
                StateRows(part->Effects(), rows);
                EventRows(part->Effects(), rows);
                hs.Step(rows);
                engineSteps++;
            }
            Part::effectCallHook = nullptr;
            CheckScenario(rel, key, hs, sj);
            if (isRcs) rcs++;
            else engines++;
        }
    }
    MESSAGE("modules: " << engines << " ModuleEnginesFX sequences, " << rcs << " ModuleRCSFX sequences, " << engineSteps << " steps, " << skippedModules << " zDeprecated parts not loaded");
    CHECK(engines > 0);
    CHECK(rcs > 0);
    CHECK(skippedModules >= 0);
    CHECK(engineSteps > 0);
    EffectEnvironment::Reset();
}

TEST_CASE("WP-UE-16a FXModuleAnimateThrottle / AnimateRCS / LookAtConstraint / ConstrainPosition on the stock parts match the reference") {
    if (!GameAvailable()) return;
    const json& ref = Reference();
    Game15 g;
    Time::fixedDeltaTime = 0.02f;
    std::map<std::string, int> done;
    for (const json& e : ref.at("fx_modules")) {
        const std::string rel = e.at("part").get<std::string>();
        const std::string kind = e.at("kind").get<std::string>();
        const std::string variant = e.at("variant").get<std::string>();
        g_part = rel;
        if (g.data.parts.getPartInfoByName(e.at("name").get<std::string>()) == nullptr) {  // zDeprecated parts are not loaded
            CHECK(rel.find("zDeprecated") != std::string::npos);
            done["skipped"]++;
            continue;
        }
        auto part = g.New(e.at("name").get<std::string>().c_str());
        std::vector<Part*> parts{part.get()};
        support::TestVessel vessel;
        vessel.Adopt(parts);
        Vessel* v = part->vessel;
        REQUIRE(v != nullptr);
        Hasher hs;
        const std::string cfgFile = rel.substr(0, rel.find('#'));
        const int which = rel.find('#') == std::string::npos ? 0 : std::stoi(rel.substr(rel.find('#') + 1));
        auto cfgRoot = ConfigNode::Load(std::string(KSPCORE_TESTDATA_DIR) + "/GameData-mirror/GameData/" + cfgFile);
        REQUIRE(cfgRoot != nullptr);
        ConfigNode* partNode = nullptr;
        int seen = 0;
        for (int i = 0; i < cfgRoot->nodes().Count(); i++)
            if (cfgRoot->nodes()[i]->name == "PART" && seen++ == which) partNode = cfgRoot->nodes()[i];
        REQUIRE(partNode != nullptr);
        auto moduleNode = [&](const char* cls) -> ConfigNode* {
            for (int i = 0; i < partNode->nodes().Count(); i++)
                if (partNode->nodes()[i]->name == "MODULE" && partNode->nodes()[i]->GetValue("name") == std::optional<std::string>(cls)) return partNode->nodes()[i];
            return nullptr;
        };
        if (kind == "animthrottle") {
            FXModuleAnimateThrottle* m = nullptr;
            ModuleEngines* eng = nullptr;
            for (PartModule* pm : part->Modules().GetModules([](PartModule&) { return true; })) {
                if (!m) m = dynamic_cast<FXModuleAnimateThrottle*>(pm);
                if (!eng) eng = dynamic_cast<ModuleEngines*>(pm);
            }
            REQUIRE(m != nullptr);
            REQUIRE(eng != nullptr);
            FXModuleEnvironment::findAnimations = [](Part&, const std::string&) { return std::vector<bool>{true}; };
            m->OnStart(StartState::Flying);
            for (const json& stj : e.at("steps")) {
                const std::string step = stj.get<std::string>();
                const std::vector<std::string> t = Split(step, ' ');
                HighLogic::LoadedSceneIsEditor = t[1] == "E";
                HighLogic::LoadedSceneIsFlight = t[1] == "F";
                v->ctrlState.mainThrottle = FromHex(t[3]);
                eng->Set<bool>("EngineIgnited", t[4] == "1");
                eng->Set<bool>("flameout", t[5] == "1");
                eng->Set<float>("currentThrottle", FromHex(t[6]));
                eng->Set<float>("finalThrust", FromHex(t[7]));
                if (t[2] == "0") part->vessel = nullptr;
                m->FixedUpdate();
                part->vessel = v;
                std::ostringstream o;
                o << "am " << (m->behaviourEnabled ? 1 : 0) << "|" << (m->Get<bool>("dependOnEngineState") ? 1 : 0) << "|" << Hex8(m->animation.weight) << "|"
                  << Hex8(m->animation.speed) << "|" << Hex8(m->animation.normalizedTime) << "|" << m->animation.layer << "|" << static_cast<int>(m->animation.wrapMode) << "|"
                  << (m->animation.playing ? 1 : 0) << "|" << Hex8(m->Get<float>("animState"));
                hs.Step({"step " + step, o.str()});
            }
            HighLogic::LoadedSceneIsEditor = false;
            HighLogic::LoadedSceneIsFlight = false;
            FXModuleEnvironment::findAnimations = nullptr;
        } else if (kind == "animrcs") {
            FXModuleAnimateRCS* m = nullptr;
            ModuleRCS* rcs = nullptr;
            for (PartModule* pm : part->Modules().GetModules([](PartModule&) { return true; })) {
                if (!m) m = dynamic_cast<FXModuleAnimateRCS*>(pm);
                if (!rcs) rcs = dynamic_cast<ModuleRCS*>(pm);
            }
            REQUIRE(m != nullptr);
            REQUIRE(rcs != nullptr);
            const uint64_t h = FnvStr(rel);
            FXModuleEnvironment::findAnimations = [h](Part&, const std::string&) { return std::vector<bool>(static_cast<size_t>(2 + h % 2), true); };
            FXModuleEnvironment::findModelTransformCount = [h](Part&, const std::string&) { return static_cast<int>(2 + h % 2 + (h % 5 == 0 ? 1 : 0)); };
            m->Set<bool>("affectTime", variant == "affectTime");
            m->OnStart(StartState::Flying);
            const size_t n = static_cast<size_t>(e.at("n").get<int>());
            for (const json& stj : e.at("steps")) {
                const std::string step = stj.get<std::string>();
                const std::vector<std::string> t = Split(step, ' ');
                HighLogic::LoadedSceneIsEditor = t[1] == "E";
                HighLogic::LoadedSceneIsFlight = t[1] == "F";
                rcs->thrustForces.assign(n, 0.0f);
                for (size_t i = 0; i < n; i++) rcs->thrustForces[i] = FromHex(t[2 + i]);
                m->FixedUpdate();
                std::vector<std::string> rows{"step " + step, std::string("ar ") + (m->behaviourEnabled ? "1" : "0")};
                for (size_t i = 0; i < m->anims.size(); i++) {
                    const AnimationStateData& a = *m->anims[i];
                    rows.push_back(" a " + Hex8(m->animState.at(i)) + "|" + Hex8(a.normalizedTime) + "|" + Hex8(a.speed) + "|" + (a.playing ? "1" : "0"));
                }
                hs.Step(rows);
            }
            ConfigNode saved("MODULE");
            m->OnSave(saved);
            hs.h = Fnv("save " + saved.GetValue("animStates").value_or("") + "\n", hs.h);  // the reference folds its `save` row into the digest last
            HighLogic::LoadedSceneIsEditor = false;
            HighLogic::LoadedSceneIsFlight = false;
            FXModuleEnvironment::findAnimations = nullptr;
            FXModuleEnvironment::findModelTransformCount = nullptr;
        } else {
            FXModuleEnvironment::findModelTransformCount = [](Part&, const std::string& name) { return static_cast<int>(FnvStr(g_part + "|" + name) % 4); };
            const char* cls = kind == "lookat" ? "FXModuleLookAtConstraint" : "FXModuleConstrainPosition";
            FXModuleLookAtConstraint* look = nullptr;
            FXModuleConstrainPosition* con = nullptr;
            for (PartModule* pm : part->Modules().GetModules([](PartModule&) { return true; })) {
                if (!look) look = dynamic_cast<FXModuleLookAtConstraint*>(pm);
                if (!con) con = dynamic_cast<FXModuleConstrainPosition*>(pm);
            }
            ConfigNode* mn = moduleNode(cls);
            REQUIRE(mn != nullptr);
            const std::string nodeName = kind == "lookat" ? "CONSTRAINLOOKFX" : "CONSTRAINFX";
            if (kind == "lookat") {
                REQUIRE(look != nullptr);
                look->ObjectsList.clear();
                look->OnLoad(*mn);
                look->OnStart(StartState::Flying);
            } else {
                REQUIRE(con != nullptr);
                con->ObjectsList.clear();
                con->OnLoad(*mn);
            }
            for (const json& stj : e.at("steps")) {
                const std::string step = stj.get<std::string>();
                const char c = step.back();
                std::vector<ConstraintDescription> tracked;
                int mode = 0;
                size_t nobj = 0;
                if (kind == "lookat") {
                    if (c == 'u') look->OnUpdate();
                    else if (c == 'f') look->OnFixedUpdate();
                    else look->LateUpdate();
                    tracked = look->TakeTracked();
                    mode = static_cast<int>(look->trackingMode);
                    nobj = look->ObjectsList.size();
                } else {
                    if (c == 'u') con->OnUpdate();
                    else if (c == 'f') con->OnFixedUpdate();
                    else con->LateUpdate();
                    tracked = con->TakeTracked();
                    mode = static_cast<int>(con->trackingMode);
                    nobj = con->ObjectsList.size();
                }
                std::vector<std::string> rows{"step " + step, "mode " + std::to_string(mode) + "|" + std::to_string(nobj) + "|" + (mn->HasNode(nodeName) ? "1" : "0")};
                for (const ConstraintDescription& d : tracked)
                    rows.push_back("track " + d.kind + "|" + d.targetName + "|" + d.moverName + "|" + std::to_string(d.moverIndex) + "|" + (d.upIsPartUp ? "1" : "0") + "|" +
                                   (d.matchPosition ? "1" : "0") + "|" + (d.matchRotation ? "1" : "0"));
                hs.Step(rows);
            }
            FXModuleEnvironment::findModelTransformCount = nullptr;
        }
        CheckScenario(rel, kind + "/" + variant, hs, e);
        done[kind]++;
    }
    MESSAGE("FXModules: " << done["animthrottle"] << " AnimateThrottle, " << done["animrcs"] << " AnimateRCS, " << done["lookat"] << " LookAt, " << done["constrain"] << " ConstrainPosition scripts, " << done["skipped"] << " zDeprecated parts not loaded");
    CHECK(done["animthrottle"] > 40);
    CHECK(done["animrcs"] > 0);
    CHECK(done["lookat"] > 0);
    CHECK(done["constrain"] > 0);
    for (const auto& kv : ref.at("totals").at("fx_modules").items()) CHECK(done[kv.key()] <= kv.value().get<int>());
}
