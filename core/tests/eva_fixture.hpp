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
    EvaWorld() {
        pg.physicsDatabaseFilename = ksp::testing::GameRoot() + "/Physics.cfg";
        pg.Awake("");
        REQUIRE(pg.LoadDatabase());
        planetarium.Awake();
        sys = CelestialSystem::Spawn(KerbolSystemPrefab(), planetarium);
        SaveEnvironment::Current().localBodies = sys->SpawnOrder();
        ScienceEnvironment::scheduler = &sched;
        EvaEnvironment::Reset();
        FlightEvaEnvironment::Reset();
        Time::time = 100.0f;
        Time::frameCount = 1000;
        Time::deltaTime = 0.02f;
        Time::fixedDeltaTime = 0.02f;
    }
    ~EvaWorld() {
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

}  // namespace ksp::eva32
