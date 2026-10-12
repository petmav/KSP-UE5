// WP-19b: the physics bubble's frame bookkeeping against the independent Python reference (tools/wp19b/frames_reference.py ->
// tests/data/wp19b_reference.json, written from the decompiled C# only): OrbitPhysicsManager.setRotatingFrame on the bodies,
// Vessel.ChangeWorldVelocity with krakensbaneAcc, Krakensbane.FixedUpdate through the bubble's host (the registry snapshot, AddExcess /
// Zero on VesselsLoaded), FloatingOrigin's decision and world shift. Every number is compared bit for bit (0 ULP). And the reversed
// orbit-driver chain of setDominantBody for every body (derived from OrbitPhysicsManager.cs:160-168).
#include <cstdint>
#include <cstring>
#include <fstream>
#include <string>

#include "doctest/doctest.h"
#include "ksp/physics/floating_origin.hpp"
#include "ksp/physics/krakensbane.hpp"
#include "nlohmann/json.hpp"
#include "physics_bubble_fixture.hpp"

using namespace ksp;
using namespace ksp::physics;
using namespace wp19b;
using json = nlohmann::json;

namespace {

const json& Ref() {
    static const json j = [] {
        std::ifstream f(std::string(KSPCORE_TEST_DATA_DIR) + "/wp19b_reference.json", std::ios::binary);
        REQUIRE(f.good());
        return json::parse(f);
    }();
    return j;
}

double D(const json& j) {
    const uint64_t u = std::stoull(j.get<std::string>(), nullptr, 16);
    double d;
    std::memcpy(&d, &u, sizeof d);
    return d;
}
float Fl(const json& j) {
    const uint32_t u = static_cast<uint32_t>(std::stoul(j.get<std::string>(), nullptr, 16));
    float f;
    std::memcpy(&f, &u, sizeof f);
    return f;
}
Vector3d V3(const json& j) { return {D(j[0]), D(j[1]), D(j[2])}; }
Vector3f V3f(const json& j) { return {Fl(j[0]), Fl(j[1]), Fl(j[2])}; }

// Three one-part vessels in physics in one bubble (their rigid bodies are the clusters whose velocities the C# writes part by part)
struct ThreeVessels {
    World w;
    BubbleScene s{w};
    std::vector<std::unique_ptr<Vessel>> vessels;
    std::vector<VesselPhysics*> vp;
    ThreeVessels() {
        CelestialBody& kerbin = w.Kerbin();
        Vector3d p0, v0;
        CircularStateAt(kerbin, kerbin.Radius + 120000.0, Vector3d(0.6, 0.48, -0.64), Vector3d(0.3, -0.9, 0.2), p0, v0);
        for (int i = 0; i < 3; ++i) {
            vessels.push_back(w.CraftHead("PhysProf-S", 1));
            PlaceOnOrbit(*vessels.back(), kerbin, p0 + v0.normalized() * (40.0 * i), v0, true);
        }
        s.Start(*vessels[0]);
        for (auto& v : vessels) v->GoOffRails();
        for (auto& v : vessels) {
            vp.push_back(s.bubble.Host().Find(*v));
            REQUIRE(vp.back() != nullptr);
            REQUIRE(vp.back()->ClusterCount() == 1);
        }
        REQUIRE(FlightGlobals::VesselsLoaded.size() == 3);
    }
    ~ThreeVessels() {
        for (auto& v : vessels) {  // back to a consistent registry before the vessels go
            v->state = Vessel::State::INACTIVE;
            v->packed = false;
        }
        FlightGlobals::VesselsLoaded = {vessels[0].get(), vessels[1].get(), vessels[2].get()};
        vessels.clear();  // (declared after the scene: the vessels go first, their bindings with them)
    }
    void SetVessel(size_t i, bool dead, bool packed, const Vector3f& vel, const Vector3d& kacc) {
        Vessel& v = *vessels[i];
        v.state = dead ? Vessel::State::DEAD : Vessel::State::INACTIVE;
        v.packed = packed;
        vp[i]->SetLinearVelocity(Vector3d(vel));
        v.krakensbaneAcc = kacc;
    }
    Vector3f Vel(size_t i) const { return vp[i]->ClusterVelocity(0).ToVector3f(); }
};

}  // namespace

TEST_CASE("WP-19b reference: setRotatingFrame on the bodies (OrbitPhysicsManager.cs:235-302), 300 scenes, 0 ULP") {
    if (!wp20b::GameAvailable()) return;
    ThreeVessels t;
    CelestialBody& kerbin = t.w.Kerbin();
    REQUIRE(t.s.opm->dominantBody == &kerbin);
    int n = 0, changed = 0;
    for (const json& sc : Ref().at("rotating_frame")) {
        kerbin.position = V3(sc["body_pos"]);
        kerbin.angularVelocity = V3(sc["ang"]);
        for (size_t i = 0; i < 3; ++i) {
            const json& jv = sc["vessels"][i];
            t.SetVessel(i, jv["dead"].get<bool>(), jv["packed"].get<bool>(), V3f(jv["vel"]), V3(jv["kacc"]));
            t.vessels[i]->vesselTransform.position = V3f(jv["pos"]);
        }
        const bool rotating = sc["rotating"].get<bool>();
        t.s.bubble.setRotatingFrame(*t.s.opm, rotating);
        CHECK(kerbin.inverseRotation == rotating);
        for (size_t i = 0; i < 3; ++i) {
            const json& o = sc["out"][i];
            INFO("scene " << n << " vessel " << i);
            CHECK(SameBits(t.Vel(i), V3f(o["vel"])));
            CHECK(SameBits(t.vessels[i]->krakensbaneAcc, V3(o["kacc"])));
            if (!SameBits(V3f(o["vel"]), V3f(sc["vessels"][i]["vel"]))) ++changed;
        }
        ++n;
    }
    MESSAGE(n << " scenes, " << changed << " vessel velocities re-expressed");
    CHECK(changed > 400);
}

TEST_CASE("WP-19b reference: Vessel.ChangeWorldVelocity (Vessel.cs:3197) with krakensbaneAcc, 300 scenes, 0 ULP") {
    if (!wp20b::GameAvailable()) return;
    ThreeVessels t;
    int n = 0;
    for (const json& sc : Ref().at("change_world_velocity")) {
        Time::fixedDeltaTime = Fl(sc["fdt"]);  // TimeWarp.fixedDeltaTime in HIGH mode at 1x
        t.SetVessel(0, sc["dead"].get<bool>(), sc["packed"].get<bool>(), V3f(sc["vel"]), V3(sc["kacc"]));
        t.vessels[0]->ChangeWorldVelocity(V3(sc["dv"]));
        INFO("scene " << n);
        CHECK(SameBits(t.Vel(0), V3f(sc["out_vel"])));
        CHECK(SameBits(t.vessels[0]->krakensbaneAcc, V3(sc["out_kacc"])));
        ++n;
    }
    Time::fixedDeltaTime = 0.02f;
    CHECK(n == 300);
}

TEST_CASE("WP-19b reference: Krakensbane.FixedUpdate through the bubble's host (registry snapshot, AddExcess / Zero on VesselsLoaded), 400 scenes, 0 ULP") {
    if (!wp20b::GameAvailable()) return;
    ThreeVessels t;
    Krakensbane& kb = t.s.bubble.KrakensbaneInstance();
    FloatingOrigin& fo = t.s.bubble.FloatingOriginInstance();
    std::vector<Vessel*> all{t.vessels[0].get(), t.vessels[1].get(), t.vessels[2].get()};
    int n = 0;
    for (const json& sc : Ref().at("krakensbane_host")) {
        Time::fixedDeltaTime = Fl(sc["fdt"]);
        const size_t count = sc["vessels"].size();
        FlightGlobals::VesselsLoaded.assign(all.begin(), all.begin() + static_cast<long>(count));
        for (size_t i = 0; i < count; ++i) {
            const json& jv = sc["vessels"][i];
            t.SetVessel(i, jv["dead"].get<bool>(), jv["packed"].get<bool>(), V3f(jv["vel"]), V3(jv["kacc"]));
            t.vessels[i]->Landed = jv["landed"].get<bool>();
            t.vessels[i]->Splashed = false;
            t.vessels[i]->radarAltitude = D(jv["radar"]);
        }
        Vessel* active = all[static_cast<size_t>(sc["active"].get<int>())];
        FlightGlobals::ActiveVessel = active;
        active->velocityD = V3(sc["velocity_d"]);
        active->rb_velocityD = V3(sc["rb_velocity_d"]);
        active->upAxis = V3(sc["up"]);
        kb.FrameVel = V3(sc["frame_vel0"]);
        kb.excessV = Vector3d();
        kb.lastCorrection = Vector3d();
        fo.outOfFrameAdditional = Vector3d();
        fo.canEngageThisFrame = true;
        kb.FixedUpdate();
        INFO("scene " << n);
        CHECK(SameBits(kb.FrameVel, V3(sc["frame_vel"])));
        CHECK(SameBits(kb.excessV, V3(sc["excess"])));
        CHECK(SameBits(kb.lastCorrection, V3(sc["last_corr"])));
        CHECK(SameBits(fo.outOfFrameAdditional, V3(sc["out_of_frame"])));
        CHECK(fo.canEngageThisFrame == sc["can_engage"].get<bool>());
        for (size_t i = 0; i < count; ++i) {
            CHECK(SameBits(t.Vel(i), V3f(sc["out_vel"][i])));
            CHECK(SameBits(t.vessels[i]->krakensbaneAcc, V3(sc["out_kacc"][i])));
        }
        ++n;
    }
    Time::fixedDeltaTime = 0.02f;
    FlightGlobals::ActiveVessel = all[0];
    CHECK(n == 400);
}

// FloatingOrigin.FixedUpdate's decision and setOffset's shift of the bodies and of unloaded vessels (the bubble's ShiftWorld), and the
// physics anchor moving with the world. Vessels without parts, unloaded: SetPosition moves their transform, CoMD / CoM follow.
TEST_CASE("WP-19b reference: FloatingOrigin decision (continuous / 500 m threshold / canEngage) and the world shift, 300 scenes, 0 ULP") {
    if (!wp20b::GameAvailable()) return;
    World w;
    BubbleScene s(w);
    std::vector<std::unique_ptr<Vessel>> vessels;
    for (int i = 0; i < 4; ++i) {
        auto v = std::make_unique<Vessel>();
        v->persistentId = static_cast<uint32_t>(7100 + i);
        v->loaded = false;
        v->packed = true;
        FlightGlobals::AddVessel(v.get());
        vessels.push_back(std::move(v));
    }
    FlightGlobals::ActiveVessel = vessels[0].get();
    FlightGlobals::ready = true;
    s.tw.warpRates = {1.0f, 2.0f, 3.0f, 4.0f, 1000.0f, 10000.0f, 100000.0f, 1000000.0f};
    s.tw.maxPhysicsRate_index = 3;
    REQUIRE(TimeWarp::MaxPhysicsRate() == 4.0f);
    FloatingOrigin& fo = s.bubble.FloatingOriginInstance();
    const std::vector<CelestialBody*>& bodies = FlightGlobals::Bodies();
    int n = 0, shifted = 0;
    for (const json& sc : Ref().at("floating_origin")) {
        REQUIRE(sc["bodies"].size() == bodies.size());
        REQUIRE(TimeWarp::MaxPhysicsRate() == Fl(sc["max_phys"]));
        for (size_t i = 0; i < bodies.size(); ++i) bodies[i]->position = V3(sc["bodies"][i]);
        for (size_t i = 0; i < vessels.size(); ++i) {
            const json& jv = sc["vessels"][i];
            vessels[i]->state = jv["dead"].get<bool>() ? Vessel::State::DEAD : Vessel::State::INACTIVE;
            vessels[i]->vesselTransform.position = V3f(jv["pos"]);
            vessels[i]->CoMD = V3(jv["comd"]);
        }
        Vessel& a = *vessels[0];
        a.Landed = sc["landed"].get<bool>();
        a.rb_velocity = V3f(sc["ship_vel"]);
        s.tw.curr_rate = Fl(sc["cur_rate"]);
        s.tw.Mode = sc["mode_high"].get<bool>() ? TimeWarp::Modes::HIGH : TimeWarp::Modes::LOW;
        s.bubble.SetFrameOriginWorld(V3(sc["anchor0"]));
        fo.ResetFrameOffset();
        fo.offsetNonKrakensbane = Vector3d();
        fo.outOfFrameAdditional = V3(sc["out_of_frame"]);
        fo.canEngageThisFrame = sc["can_engage"].get<bool>();
        const size_t before = fo.ShiftCount();
        fo.FixedUpdate();
        INFO("scene " << n);
        const bool didShift = fo.ShiftCount() == before + 1;
        CHECK(didShift == sc["shifted"].get<bool>());
        CHECK(fo.continuous == sc["continuous"].get<bool>());
        CHECK(SameBits(fo.offset, V3(sc["offset"])));
        CHECK(SameBits(fo.reverseoffset, V3(sc["reverse"])));
        CHECK(SameBits(fo.offsetNonKrakensbane, V3(sc["onk"])));
        CHECK(SameBits(fo.FrameOffset(), V3(sc["frame_offset"])));
        CHECK(SameBits(s.bubble.FrameOriginWorld(), V3(sc["anchor"])));
        for (size_t i = 0; i < bodies.size(); ++i) CHECK(SameBits(bodies[i]->position, V3(sc["out_bodies"][i])));
        for (size_t i = 0; i < vessels.size(); ++i) {
            const json& o = sc["out_vessels"][i];
            CHECK(SameBits(vessels[i]->vesselTransform.position, V3f(o["pos"])));
            CHECK(SameBits(vessels[i]->CoMD, V3(o["comd"])));
            if (didShift && vessels[i]->state != Vessel::State::DEAD) CHECK(SameBits(vessels[i]->CoM, V3f(o["com"])));
        }
        CHECK(fo.canEngageThisFrame);           // reset for the next tick
        CHECK(fo.outOfFrameAdditional.IsZero());
        if (didShift) ++shifted;
        ++n;
    }
    MESSAGE(n << " scenes, " << shifted << " world shifts");
    CHECK(shifted > 100);
    for (auto& v : vessels) v->state = Vessel::State::INACTIVE;
    vessels.clear();
}

// OrbitPhysicsManager.cs:160-168: every orbit driver's reverse flag cleared, then SetReverseOrbit from the new dominant body up its chain of
// reference bodies (the Sun has no driver): the dominant body stays still in the world (Planetarium.UpdateCBsRecursive starts there).
TEST_CASE("WP-19b setDominantBody: the reversed orbit-driver chain for every body; Planetarium's root body follows") {
    if (!wp20b::GameAvailable()) return;
    World w;
    BubbleScene s(w);
    s.opm = std::make_unique<OrbitPhysicsManager>();
    const std::vector<CelestialBody*>& bodies = FlightGlobals::Bodies();
    for (CelestialBody* dominant : bodies) {
        if (!dominant->orbitDriver) continue;  // the Sun cannot be the dominant body of a vessel in KSP's system
        s.bubble.setDominantBody(*s.opm, dominant);
        CHECK(s.opm->dominantBody == dominant);
        std::vector<CelestialBody*> chain;
        for (CelestialBody* b = dominant; b != nullptr && b->orbitDriver; b = b->orbitDriver->referenceBody) chain.push_back(b);
        for (CelestialBody* b : bodies) {
            if (!b->orbitDriver) continue;
            const bool want = std::find(chain.begin(), chain.end(), b) != chain.end();
            INFO(dominant->bodyName << " / " << b->bodyName);
            CHECK(b->orbitDriver->reverse == want);
        }
        w.planetarium.FixedUpdate();
        CHECK(w.planetarium.CurrentMainBody == dominant);
    }
    s.bubble.setDominantBody(*s.opm, &w.Kerbin());  // back to the system's home configuration
    CHECK(w.Kerbin().orbitDriver->reverse);
}
