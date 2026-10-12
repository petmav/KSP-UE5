// WP-32 unit tests, part 2: the movement arithmetic (KerbalEVA.cs walk / run / low-g bound blending, UpdateMovement, UpdateHeading, the orientation PID,
// the jetpack linear / angular / fuel update) against tools/wp32/eva_reference.py (independent Python from the C#, float32 stores under the Mono rule):
// 0 ULP / exact bit patterns. The transform rotations are exact permutations (axes with components 0 / +-1), so the comparison is of KerbalEVA's
// arithmetic, not of the Unity rotation maths.
#include "eva_fixture.hpp"

#include "ksp/resources/part_resource_library.hpp"

using namespace ksp;
using namespace ksp::eva32;

namespace {

void ResetAnims(HeadlessEvaEngine& eng) {
    eng.animCalls.clear();
    for (auto& kv : eng.anims) {
        kv.second.speed = 1.0f;
        kv.second.normalizedSpeed = 0.0f;
        kv.second.normalizedTime = 0.0f;
        kv.second.weight = 0.0f;
        kv.second.enabled = false;
    }
}

}  // namespace

// KerbalEVA.cs:2074 / :2148 / :2233
TEST_CASE("WP-32 walk / run / low-g bound (FPS): tgtSpeed, animation calls, blend weights equal the Python reference (0 ULP)") {
    if (!GameAvailable()) return;
    nlohmann::json ref = LoadRef();
    EvaWorld w;
    EvaKerbal k(w, true);
    KerbalEVA& e = *k.eva;
    int n = 0;
    for (const auto& c : ref["fps"]) {
        ResetAnims(k.engine);
        const std::string kind = c["kind"].get<std::string>();
        w.Kerbin().GeeASL = c["gee"].get<double>();
        e.set_JetpackDeployed(c["jp"].get<bool>());
        k.part->partTransform.rotation = Rot(ref, c["rot"].get<int>());
        e.tgtRpos = V3(c["tgt"]);
        e.set_lastBoundStep(F(c["lastStep"]));
        for (KerbalAnimationState* a : {&e.Animations().walkLowGee, &e.Animations().walkBack, &e.Animations().strafeRight, &e.Animations().strafeLeft}) {
            a->State()->normalizedSpeed = F(c["ns"]);
            a->State()->normalizedTime = F(c["nt"]);
        }
        if (kind == "walk") e.walk_fps_OnUpdate();
        else if (kind == "run") e.run_fps_OnUpdate();
        else e.bound_gr_fps_OnUpdate();
        const std::string what = kind + " case " + std::to_string(n);
        CheckBits(e.tgtSpeed, c["tgtSpeed"], what + " tgtSpeed");
        const auto& calls = c["calls"];
        REQUIRE_MESSAGE(k.engine.animCalls.size() == calls.size(), what);
        for (size_t i = 0; i < calls.size(); i++) {
            const auto& want = calls[i];
            const auto& got = k.engine.animCalls[i];
            INFO(what << " call " << i);
            CHECK(got.op == want[0].get<std::string>());
            CHECK(got.name == want[1].get<std::string>());
            CheckBits(got.a, want[2], what + " call a");
            CheckBits(got.b, want[3], what + " call b");
            CHECK(got.mode == want[4].get<int>());
        }
        if (kind != "bound") {
            CheckBits(e.Animations().walkLowGee.State()->speed, c["lowGeeSpeed"], what + " walkLowGee.speed");
        } else {
            if (c.contains("boundStepSpeed")) CheckBits(e.boundStepSpeed, c["boundStepSpeed"], what + " boundStepSpeed");
            if (c.contains("s1")) CheckBits(k.engine.anims[c["s1"][0].get<std::string>()].speed, c["s1"][1], what + " s1.speed");
            if (c.contains("s2")) {
                EvaAnimState& s2 = k.engine.anims[c["s2"][0].get<std::string>()];
                CheckBits(s2.normalizedSpeed, c["s2"][1], what + " s2.normalizedSpeed");
                CheckBits(s2.normalizedTime, c["s2"][2], what + " s2.normalizedTime");
            }
        }
        n++;
    }
    CHECK(n == 252);
}

// KerbalEVA.cs:4776
TEST_CASE("WP-32 UpdateMovement: currentSpd lerp, cmdDir lerp, slope-corrected rigidbody velocity equal the Python reference (0 ULP)") {
    if (!GameAvailable()) return;
    nlohmann::json ref = LoadRef();
    EvaWorld w;
    EvaKerbal k(w, true);
    KerbalEVA& e = *k.eva;
    EvaEnvironment::upAxis = [] { return Vector3f(0.0f, 1.0f, 0.0f); };
    e.heightFromSurfaceHitNormal = Vector3f(0.0f, 1.0f, 0.0f);  // FromToRotation(up, up) is the identity
    int n = 0;
    for (const auto& c : ref["movement"]) {
        k.part->partTransform.rotation = Rot(ref, c["rot"].get<int>());
        e.fsm->CurrentState()->TimeAtStateEnter = c["enter"].get<double>();
        Time::time = F(c["time"]);
        e.lastTgtSpeed = F(c["lastTgt"]);
        e.tgtSpeed = F(c["tgtSpeed"]);
        e.tgtRpos = V3(c["tgtRpos"]);
        e.cmdDir = V3(c["cmdDirIn"]);
        e.CharacterFrameMode = c["charFrame"].get<bool>();
        e.fUp = V3(c["fUp"]);
        e.UpdateMovement();
        const std::string what = "movement case " + std::to_string(n++);
        CheckBits(e.currentSpd, c["currentSpd"], what + " currentSpd");
        CheckV3(e.cmdDir, c["cmdDir"], what + " cmdDir");
        CheckV3(k.engine.velocity, c["vel"], what + " velocity");
    }
    CHECK(n == 180);
}

// KerbalEVA.cs:4793
TEST_CASE("WP-32 UpdateHeading: deltaHdg and the commanded angular velocity equal the Python reference (0 ULP)") {
    if (!GameAvailable()) return;
    nlohmann::json ref = LoadRef();
    EvaWorld w;
    EvaKerbal k(w, true);
    KerbalEVA& e = *k.eva;
    int n = 0;
    for (const auto& c : ref["heading"]) {
        k.part->partTransform.rotation = Rot(ref, c["rot"].get<int>());
        e.tgtFwd = V3(c["tgtFwd"]);
        e.tgtRpos = V3(c["tgtRpos"]);
        e.deltaHdg = F(c["deltaIn"]);
        e.fUp = V3(c["fUp"]);
        const Vector3f sentinel(7.0f, 7.0f, 7.0f);
        k.engine.angularVelocity = sentinel;
        e.UpdateHeading();
        const std::string what = "heading case " + std::to_string(n++);
        if (c["returned"].get<bool>()) {
            CheckBits(e.deltaHdg, c["delta"], what + " deltaHdg (returned early)");
            CHECK(k.engine.angularVelocity == sentinel);
        } else {
            CheckBits(e.deltaHdg, c["delta"], what + " deltaHdg");
            CheckV3(k.engine.angularVelocity, c["ang"], what + " angularVelocity");
        }
    }
    CHECK(n == 180);
}

// KerbalEVA.cs:4853
TEST_CASE("WP-32 UpdateOrientationPID: error, integral, derivative, boost and cmdRot equal the Python reference (0 ULP)") {
    if (!GameAvailable()) return;
    nlohmann::json ref = LoadRef();
    EvaWorld w;
    EvaKerbal k(w, true);
    KerbalEVA& e = *k.eva;
    int n = 0;
    for (const auto& c : ref["pid"]) {
        k.part->partTransform.rotation = Rot(ref, c["rot"].get<int>());
        e.tgtFwd = V3(c["tgtFwd"]);
        e.tgtUp = V3(c["tgtUp"]);
        e.integral = V3(c["integralIn"]);
        e.prev_error = V3(c["prevIn"]);
        e.set_thrustPercentage(F(c["thrust"]));
        e.thrustPercentagePIDBoost = c["boostFlag"].get<bool>();
        e.manualAxisControl = c["manual"].get<bool>();
        e.cmdRot = Vector3f(9.0f, 9.0f, 9.0f);
        e.error = Vector3f(8.0f, 8.0f, 8.0f);
        Time::fixedDeltaTime = 0.02f;
        e.UpdateOrientationPID();
        const std::string what = "pid case " + std::to_string(n++);
        if (c["manual"].get<bool>()) {
            CHECK(e.cmdRot == Vector3f(9.0f, 9.0f, 9.0f));
            continue;
        }
        CheckBits(e.PIDBoost, c["boost"], what + " PIDBoost");
        CheckV3(e.error, c["error"], what + " error");
        CheckV3(e.integral, c["integral"], what + " integral");
        CheckV3(e.derivative, c["derivative"], what + " derivative");
        CheckV3(e.cmdRot, c["cmdRot"], what + " cmdRot");
        CheckV3(e.prev_error, c["error"], what + " prev_error");
    }
    CHECK(n == 150);
}

// KerbalEVA.cs:5047 / :4873
TEST_CASE("WP-32 jetpack: UpdatePackLinear / UpdatePackAngular forces, torques and fuel flow equal the Python reference (0 ULP)") {
    if (!GameAvailable()) return;
    nlohmann::json ref = LoadRef();
    EvaWorld w;
    PartResourceLibrary lib;
    lib.LoadDefinitions(w.g.data.database);
    EvaKerbal k(w, true);
    KerbalEVA& e = *k.eva;
    // EVA propellant resource (off the part's list, like StartEVA leaves it): the definition of the game's ResourcesGeneric.cfg
    PartResource prop(k.part.get());
    prop.info = lib.GetDefinition("EVA Propellant");
    REQUIRE(prop.info != nullptr);
    prop.resourceName = "EVA Propellant";
    e.propellantResource = &prop;
    // StartEVA: linPower *= massMultiplier
    e.set_linPower(f32store(fx(e.linPower()) * fx(e.massMultiplier())));
    int n = 0;
    for (const auto& c : ref["pack"]["linear"]) {
        k.engine.forces.clear();
        k.engine.torques.clear();
        e.set_JetpackDeployed(c["jet"].get<bool>());
        k.vessel->packed = c["packed"].get<bool>();
        e.isRagdoll = c["ragdoll"].get<bool>();
        e.set_thrustPercentage(F(c["thrust"]));
        e.packTgtRPos = V3(c["packTgt"]);
        e.cmdRot = V3(c["cmdRot"]);
        prop.amount = c["fuel"].get<double>();
        prop.maxAmount = 5.0;
        e.fuelFlowRate = F(c["ffrIn"]);
        e.packLinear = Vector3f::zero();
        Time::fixedDeltaTime = 0.02f;
        e.UpdatePackLinear();
        e.UpdatePackAngular();
        const std::string what = "pack case " + std::to_string(n++);
        CheckBits(e.fuelFlowRate, c["ffr"], what + " fuelFlowRate");
        CheckV3(e.packLinear, c["packLinear"], what + " packLinear");
        REQUIRE_MESSAGE(k.engine.forces.size() == c["forces"].size(), what);
        for (size_t i = 0; i < c["forces"].size(); i++) CheckV3(k.engine.forces[i], c["forces"][i], what + " force");
        REQUIRE_MESSAGE(k.engine.torques.size() == c["torques"].size(), what);
        for (size_t i = 0; i < c["torques"].size(); i++) CheckV3(k.engine.torques[i], c["torques"][i], what + " torque");
    }
    k.vessel->packed = false;
    CHECK(n == 120);
    // UpdatePackFuel: the drain of the off-list propellant resource and of the inventory snapshots in priority order
    int d = 0;
    for (const auto& c : ref["pack"]["drains"]) {
        std::vector<std::unique_ptr<ProtoPartResourceSnapshot>> snaps;
        e.inventoryPropellantResources.clear();
        for (const auto& amt : c["snaps"]) {
            ConfigNode node("RESOURCE");
            node.AddValue("name", "EVA Propellant");
            node.AddValue("amount", amt.get<double>());
            node.AddValue("maxAmount", 3.0);
            node.AddValue("flowState", true);
            snaps.push_back(std::make_unique<ProtoPartResourceSnapshot>(node));
            snaps.back()->amount = amt.get<double>();
            EvaResourceListItem item;
            item.pPResourceSnapshot = snaps.back().get();
            e.inventoryPropellantResources.push_back(item);
        }
        prop.amount = c["resource"].get<double>();
        prop.maxAmount = 100.0;
        prop.flowMode = PartResourceFlowMode::Both;
        e.fuelFlowRate = F(c["ffr"]);
        ResourceFlowEnvironment::InfinitePropellant = false;
        e.UpdatePackFuel();
        const std::string what = "drain case " + std::to_string(d++);
        INFO(what);
        CHECK(prop.amount == c["resourceAfter"].get<double>());
        for (size_t i = 0; i < snaps.size(); i++) CHECK(snaps[i]->amount == c["snapsAfter"][i].get<double>());
        // cheat: InfinitePropellant zeroes the flow and drains nothing
        ResourceFlowEnvironment::InfinitePropellant = true;
        e.fuelFlowRate = 1.0f;
        const double before = prop.amount;
        e.UpdatePackFuel();
        ResourceFlowEnvironment::InfinitePropellant = false;
        CHECK(e.fuelFlowRate == 0.0f);
        CHECK(prop.amount == before);
    }
    CHECK(d == 60);
    e.propellantResource = nullptr;
}
