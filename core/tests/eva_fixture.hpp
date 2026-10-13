// WP-32 test fixture: an EVA kerbal on Kerbin (the real kerbalEVA part of the GameData mirror, its KerbalEVA module with the game's cfg values),
// driven through the HeadlessEvaEngine seam. SYNTHETIC: the animation clip table (names and lengths) and the engine-side answers (rays, colliders);
// they stand for the Unity prefab's AnimationClips / physics, which only exist in the asset bundles (see STATUS.md "WP-32", deferred).
#pragma once

#include <cmath>
#include <cstring>
#include <fstream>

#include "doctest/doctest.h"
#include "nlohmann/json.hpp"
#include "inventory_fixture.hpp"

#include "ksp/celestial/celestial_system.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/eva/flight_eva.hpp"
#include "ksp/input/game_settings.hpp"
#include "ksp/eva/kerbal.hpp"
#include "ksp/eva/kerbal_eva.hpp"
#include "ksp/eva/retractable_ladder.hpp"
#include "ksp/kerbals/crew_generator.hpp"
#include "ksp/kerbals/experience.hpp"
#include "ksp/save/crew.hpp"
#include "ksp/save/game.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/science/science.hpp"

namespace ksp::eva32 {

using namespace ksp::rtest;

inline nlohmann::json LoadRef() {
    std::ifstream f(std::string(KSPCORE_TEST_DATA_DIR) + "/wp32_reference.json", std::ios::binary);
    REQUIRE(f.good());
    return nlohmann::json::parse(f);
}

inline uint32_t Bits(float f) {
    uint32_t u;
    std::memcpy(&u, &f, 4);
    return u;
}
inline float FromBits(uint32_t u) {
    float f;
    std::memcpy(&f, &u, 4);
    return f;
}
inline Vector3f V3(const nlohmann::json& j) { return Vector3f(FromBits(j[0].get<uint32_t>()), FromBits(j[1].get<uint32_t>()), FromBits(j[2].get<uint32_t>())); }
inline Quaternionf Q4(const nlohmann::json& j) {
    return Quaternionf(FromBits(j[0].get<uint32_t>()), FromBits(j[1].get<uint32_t>()), FromBits(j[2].get<uint32_t>()), FromBits(j[3].get<uint32_t>()));
}
inline Quaternionf Rot(const nlohmann::json& ref, int i) { return Q4(ref["rotations"][static_cast<size_t>(i)]); }
inline float F(const nlohmann::json& j) { return FromBits(j.get<uint32_t>()); }
inline void CheckBits(float got, const nlohmann::json& want, const std::string& what) {
    INFO(what << ": got " << got << " want " << FromBits(want.get<uint32_t>()));
    CHECK(Bits(got) == want.get<uint32_t>());
}
inline void CheckV3(const Vector3f& got, const nlohmann::json& want, const std::string& what) {
    CheckBits(got.x, want[0], what + ".x");
    CheckBits(got.y, want[1], what + ".y");
    CheckBits(got.z, want[2], what + ".z");
}

// Kerbin (CelestialSystem), the real Physics.cfg, a Planetarium and a coroutine scheduler
struct EvaWorld {
    Game15 g;
    PhysicsGlobals pg;
    Planetarium planetarium;
    std::unique_ptr<CelestialSystem> sys;
    CoroutineScheduler sched;
    Game game;
    PartResourceLibrary resourceLibrary;
    EvaWorld() {
        pg.physicsDatabaseFilename = ksp::testing::GameRoot() + "/Physics.cfg";
        pg.Awake("");
        REQUIRE(pg.LoadDatabase());
        planetarium.Awake();
        sys = CelestialSystem::Spawn(KerbolSystemPrefab(), planetarium);
        SaveEnvironment::Current().localBodies = sys->SpawnOrder();
        {
            // the CAREER game with a roster and the Squad experience traits (ProtoCrewMember.UpdateExperience reads HighLogic.CurrentGame)
            SaveEnvironment& env = SaveEnvironment::Current();
            std::vector<const ConfigNode*> traits;
            for (ConfigNode* n : g.data.database.GetConfigNodes("EXPERIENCE_TRAIT")) traits.push_back(n);
            env.experienceConfigs = std::make_shared<Experience::ExperienceSystemConfig>(traits);
            game.Mode = Game_Modes::CAREER;
            game.CrewRoster = std::make_shared<KerbalRoster>(Game_Modes::CAREER);
            env.currentGame = &game;
        }
        resourceLibrary.LoadDefinitions(g.data.database);
        ScienceEnvironment::scheduler = &sched;
        EvaEnvironment::Reset();
        FlightEvaEnvironment::Reset();
        Time::time = 100.0f;
        Time::frameCount = 1000;
        Time::deltaTime = 0.02f;
        Time::fixedDeltaTime = 0.02f;
    }
    ~EvaWorld() {
        SaveEnvironment::Current().currentGame = nullptr;
        FlightRegistry::Clear();
        KerbalEVA::UninstallHostHooks();
        EvaEnvironment::Reset();
        FlightEvaEnvironment::Reset();
        CrewHatchEnvironment::Reset();
        KerbalEnvironment::Reset();
        ScienceEnvironment::scheduler = nullptr;
        sys.reset();
        Planetarium::Zup = Planetarium::AwakeZup(0.0);
    }
    CelestialBody& Kerbin() { return *sys->GetBodyByName("Kerbin"); }
};

// The synthetic animation table: every KerbalAnimationState of the manager gets animationName = field name and a clip length (SYNTHETIC).
inline void SetupClips(KerbalEVA& eva, HeadlessEvaEngine& eng) {
    int i = 0;
    for (KerbalAnimationState* a : eva.Animations().GetAllAnimations()) {
        // field order index drives distinct, repeatable lengths 0.5 .. 3.0 s
        a->animationName = "clip" + std::to_string(i);
        a->hasJpVariant = (i % 3) == 0;
        a->start = 0.0f;
        a->end = 1.0f;
        a->speedAt1Gee = 1.0f;
        EvaAnimState& s = eng.anims[a->animationName];
        s.name = a->animationName;
        s.length = 0.5f + 0.125f * static_cast<float>(i % 21);
        EvaAnimState& j = eng.anims["jp_" + a->animationName];
        j.name = "jp_" + a->animationName;
        j.length = s.length;
        i++;
    }
}

// An EVA kerbal on Kerbin (landed, active, Valentina Kerman) with the KerbalEVA module of the real kerbalEVA part.
struct EvaKerbal {
    HeadlessEvaEngine engine;
    std::unique_ptr<Part> part;
    std::shared_ptr<ProtoCrewMember> crew;
    KerbalEVA* eva = nullptr;
    std::unique_ptr<Vessel> vessel;  // declared last: destroyed first
    explicit EvaKerbal(EvaWorld& w, bool started = false) {
        KerbalInventory::Install();
        vessel = inv33::BareVessel(VesselType::EVA, Vector3f());
        vessel->orbitDriver = std::make_unique<OrbitDriver>();
        vessel->orbitDriver->orbit->referenceBody = &w.Kerbin();
        vessel->Landed = true;
        vessel->situation = Vessel::Situations::LANDED;
        vessel->state = Vessel::State::ACTIVE;
        part = w.g.New("kerbalEVA");
        crew = std::make_shared<ProtoCrewMember>(ProtoCrewMember::KerbalType::Crew, "Valentina Kerman");
        part->protoModuleCrew.push_back(crew);
        part->vessel = vessel.get();
        vessel->parts.push_back(part.get());
        vessel->rootPart = part.get();
        FlightRegistry::VesselsLoaded.push_back(vessel.get());
        FlightRegistry::ActiveVessel = vessel.get();
        eva = dynamic_cast<KerbalEVA*>(part->FindModuleOfClass("KerbalEVA"));
        REQUIRE(eva != nullptr);
        vessel->evaController = eva;
        eva->engine = &engine;
        // the strings the C# caches (tags themselves: no Localizer needed)
        eva->cacheAutoLOC_6010008 = "L6010008";
        eva->cacheAutoLOC_6010009 = "L6010009";
        eva->cacheAutoLOC_6010010 = "L6010010";
        eva->cacheAutoLOC_6010011 = "L6010011";
        eva->cacheAutoLOC_8003204 = "L8003204";
        eva->cacheAutoLOC_6010015 = "L6010015";
        eva->cacheAutoLOC_6006049 = "L6006049";
        SetupClips(*eva, engine);
        if (started) Start();
    }
    // KerbalEVA.StartEVA minus the parts that need the prefab: animations, FSM, started in Idle (Grounded)
    void Start() {
        eva->SetupAnimations();
        eva->SetupFSM();
        eva->fsm->StartFSM(eva->st_idle_gr);
        eva->Ready = true;
    }
    ~EvaKerbal() {
        eva->engine = nullptr;
        part->vessel = nullptr;
        KerbalInventory::ClearAll();
        KerbalInventory::Uninstall();
    }
};

inline void Pump(CoroutineScheduler& s, int frames = 3) {
    for (int i = 0; i < frames; i++) {
        s.AdvanceFrame(0.02f);
        s.RunDelayedDynamicFrameRate();
    }
}


// The scripted world of the conditions / scenario families (tools/wp32/eva_ref_conditions.py, eva_ref_scenarios.py) applied to a live kerbal.
struct WorldState {
    std::vector<EvaCollider> colliders = std::vector<EvaCollider>(4);   // [0] the current ladder, [1..3] the triggers
    Part otherPart;
    nlohmann::json keys = nlohmann::json::object();
    int clrot = 0;
};

inline void ApplyWorld(EvaWorld& ew, EvaKerbal& k, WorldState& ws, const nlohmann::json& ref, const nlohmann::json& w, bool times) {
    KerbalEVA& e = *k.eva;
    static const char* order[] = {"tgt", "dh", "ldh", "gee", "cf", "landed", "splashed", "packed", "active", "hft", "hfs", "lbs", "jet", "pm", "keys",
                                  "noparent", "clrot", "lad_active", "lad_enabled", "cl", "ntrig", "inv", "ltr", "chk", "ccl", "cdn", "spd", "cpath", "cnt",
                                  "cend", "clen", "inconstr", "weldfx", "weldmesh", "airlock", "rbv", "vrb", "lcd", "lcn", "fup", "rot", "ut", "lct", "hss"};
    auto has = [&](const char* n) { return w.contains(n); };
    if (has("tgt")) e.tgtRpos = V3(w["tgt"]);
    if (has("dh")) e.deltaHdg = F(w["dh"]);
    if (has("ldh")) e.lastDeltaHdg = F(w["ldh"]);
    if (has("gee")) ew.Kerbin().GeeASL = w["gee"].get<double>();
    if (has("cf")) e.CharacterFrameMode = w["cf"].get<bool>();
    if (has("landed")) k.vessel->Landed = w["landed"].get<bool>();
    if (has("splashed")) k.vessel->Splashed = w["splashed"].get<bool>();
    if (has("packed")) k.vessel->packed = w["packed"].get<bool>();
    if (has("active")) k.vessel->state = w["active"].get<bool>() ? Vessel::State::ACTIVE : Vessel::State::INACTIVE;
    if (has("hft")) k.vessel->heightFromTerrain = F(w["hft"]);
    if (has("hfs")) k.vessel->heightFromSurface = F(w["hfs"]);
    if (times && has("t")) {
        Time::time = F(w["t"]);
        e.fsm->CurrentState()->TimeAtStateEnter = 0.0;
        Time::frameCount = w["frames"].get<int>();
        e.fsm->CurrentState()->FrameCountAtStateEnter = 0;
    }
    if (has("lbs")) e.set_lastBoundStep(F(w["lbs"]));
    if (has("jet")) e.set_JetpackDeployed(w["jet"].get<bool>());
    if (has("pm")) e.partPlacementMode = w["pm"].get<bool>();
    if (has("keys")) {
        ws.keys = w["keys"];
        const nlohmann::json* kp = &ws.keys;
        EvaEnvironment::key = [kp](EvaKey kk, KeyQuery q) {
            static const char* names[] = {"Forward", "Back", "Left", "Right", "YawLeft", "YawRight", "PackForward", "PackBack", "PackLeft", "PackRight", "PackUp",
                                          "PackDown", "Jump", "Run", "TogglePack", "Use", "Board", "Orient", "Lights", "Helmet", "ChuteDeploy", "ToggleMovementMode"};
            const std::string n = names[static_cast<int>(kk)];
            if (!kp->contains(n)) return false;
            const char* f = q == KeyQuery::Held ? "held" : (q == KeyQuery::Down ? "down" : "up");
            return (*kp)[n].contains(f) && (*kp)[n][f].get<bool>();
        };
    }
    if (has("noparent")) k.part->parent = w["noparent"].get<bool>() ? nullptr : &ws.otherPart;
    if (has("clrot")) {
        ws.clrot = w["clrot"].get<int>();
        for (auto& c : ws.colliders) c.pose.rotation = Rot(ref, ws.clrot);
    }
    if (has("lad_active")) for (auto& c : ws.colliders) c.activeInHierarchy = w["lad_active"].get<bool>();
    if (has("lad_enabled")) for (auto& c : ws.colliders) c.enabled = w["lad_enabled"].get<bool>();
    if (has("cl")) e.currentLadder = w["cl"].get<bool>() ? &ws.colliders[0] : nullptr;
    if (has("ntrig")) {
        e.currentLadderTriggers.clear();
        for (int i = 0; i < w["ntrig"].get<int>(); i++) e.currentLadderTriggers.push_back(&ws.colliders[static_cast<size_t>(i) + 1]);
    }
    if (has("inv")) e.invLadderAxis = w["inv"].get<bool>();
    if (has("ltr")) e.ladderTgtRPos = V3(w["ltr"]);
    if (has("chk")) GameSettings::EVA_LADDER_CHECK_END = w["chk"].get<bool>();
    if (has("ccl")) e.canClimb = w["ccl"].get<bool>();
    if (has("cdn")) e.canDescend = w["cdn"].get<bool>();
    if (has("spd")) e.currentSpd = F(w["spd"]);
    if (has("cpath")) {
        if (w["cpath"].get<bool>()) e.clamberPath = std::make_unique<ClamberPath>(Vector3f(), Vector3f(), Vector3f(), Vector3f(), Vector3f(), 0.0f, 0.0f);
        else e.clamberPath.reset();
    }
    if (has("cnt")) e.Animations().clamber.State()->normalizedTime = F(w["cnt"]);
    if (has("cend")) e.Animations().clamber.end = F(w["cend"]);
    if (has("clen")) e.Animations().clamber.State()->length = F(w["clen"]);
    if (has("inconstr")) e.InConstructionMode = w["inconstr"].get<bool>();
    if (has("weldfx")) {
        if (w["weldfx"].get<bool>()) {
            if (!e.weldFX) e.weldFX = std::make_unique<EvaWeldFx>();
        } else {
            e.weldFX.reset();
        }
    }
    if (has("weldmesh") && e.weldFX) e.weldFX->meshEnabled = w["weldmesh"].get<bool>();
    if (has("airlock")) e.currentAirlockPart = w["airlock"].get<bool>() ? &ws.otherPart : nullptr;
    if (has("rbv")) k.engine.velocity = V3(w["rbv"]);
    if (has("vrb")) k.vessel->rb_velocity = V3(w["vrb"]);
    if (has("lcd")) e.lastCollisionDirection = V3(w["lcd"]);
    if (has("lcn")) e.lastCollisionNormal = V3(w["lcn"]);
    if (has("fup")) e.fUp = V3(w["fup"]);
    if (has("rot")) k.part->partTransform.rotation = Rot(ref, w["rot"].get<int>());
    if (has("ut")) {
        const double ut = w["ut"].get<double>();
        EvaEnvironment::universalTime = [ut] { return ut; };
    }
    if (has("lct")) e.lastCollisionTime = w["lct"].get<double>();
    if (has("hss")) k.vessel->horizontalSrfSpeed = w["hss"].get<double>();
    (void)order;
}

}  // namespace ksp::eva32
