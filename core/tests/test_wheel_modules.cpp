// WP-29b unit tests: the KSP wheel part modules (include/ksp/wheels/wheel_modules.hpp, src/wheels/wheel_{modules,base,submodules}.cpp).
//
// The numeric gate is tests/data/wp29b_reference.json, written by tools/wp29b/wheel_reference.py: an independent Python implementation of the
// same C# (ModuleWheelBase.cs, ModuleWheels/*.cs, SuspensionLoadBalancer.cs) that shares no code with the port and was written before it. Its
// inputs are generated there (seeded) and replayed here; the stock wheel parameters are parsed there from the part .cfg files and this file
// asserts that the module host read the same bits. It covers
//   * WheelUtil, ModuleWheelBase.ApplyGeeBias / UpdateFriction over a grid (13 stock wheel parts x gee x autoFriction x model x geeBias x with /
//     without a controller), OnPutToGround, SuspensionLoadBalancer.FixedUpdate;
//   * 39 scripted runs (every stock wheel part, 3 bodies / physics-warp rates, 200 ticks, FixedUpdate / Update / LateUpdate in the same order):
//     wheelSetup, FixedUpdate of the base (slip displacement, landing detection, schloompa freeze, drift fix), the motor (torque curve, 0.01
//     rounding, traction control, electric charge draw), motor steering, steering (Ackermann turn centre, curves), brakes, lock, suspension (auto
//     spring / damper, boost, high gee), damage (stress, impact, repair, Unity Random), deployment (the KerbalFSM over the Animation), bogey,
//     events and actions, pack / docking handlers, the sub-system enable / disable cascade; every output 0 ULP (NaN == NaN, sign of zero included).
// The native seams (WheelCollider state, transforms, colliders, the animation clock) are scripted per tick from the same file. The remaining
// cases are derived from the C# control flow (registration, persistence, errors, quirks); they are labelled.
#include <array>
#include <cmath>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>

#include "doctest/doctest.h"
#include "flight_fixture.hpp"
#include "ksp/core/coroutine.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/input/game_settings.hpp"
#include "ksp/modules/control_types.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/unity/random.hpp"
#include "ksp/vessel/flight_ctrl_state.hpp"
#include "ksp/wheels/wheel_modules.hpp"
#include "nlohmann/json.hpp"
#include "resource_fixture.hpp"

using namespace ksp;
using namespace ksp::rtest;
using namespace ksp::VehiclePhysics;
using namespace ksp::ModuleWheels;
using json = nlohmann::json;

namespace {

const json& LoadRef29b() {
    static const json j = [] {
        std::ifstream f(std::string(KSPCORE_TEST_DATA_DIR) + "/wp29b_reference.json", std::ios::binary);
        REQUIRE(f.good());
        return json::parse(f);
    }();
    return j;
}

// the reference writes a float as a number, or "nan" / "inf" / "-inf" / "-0"
float J(const json& v) {
    if (v.is_string()) {
        const std::string s = v.get<std::string>();
        if (s == "nan") return std::nanf("");
        if (s == "inf") return INFINITY;
        if (s == "-inf") return -INFINITY;
        if (s == "-0") return -0.0f;
        FAIL("bad float token " << s);
    }
    return v.get<float>();
}
double JD(const json& v) { return static_cast<double>(J(v)); }
bool Same(float a, float b) {
    if (std::isnan(a) && std::isnan(b)) return true;
    uint32_t x, y;
    std::memcpy(&x, &a, 4);
    std::memcpy(&y, &b, 4);
    return x == y;
}
int64_t UlpDiff(float a, float b) {
    int32_t x, y;
    std::memcpy(&x, &a, 4);
    std::memcpy(&y, &b, 4);
    if (x < 0) x = INT32_MIN - x;
    if (y < 0) y = INT32_MIN - y;
    return static_cast<int64_t>(x) - static_cast<int64_t>(y);
}
Vector3f V3j(const json& a) { return Vector3f(J(a[0]), J(a[1]), J(a[2])); }
Quaternionf Qj(const json& a) { return Quaternionf(J(a[0]), J(a[1]), J(a[2]), J(a[3])); }

struct Diff {
    int64_t bad = 0, total = 0;
    std::string first;
    void Check(float got, const json& exp, const std::string& what) {
        total++;
        const float e = J(exp);
        if (!Same(got, e)) {
            if (bad++ < 12) {
                std::ostringstream os;
                os.precision(9);
                os << what << ": got " << got << " expected " << e << " (" << UlpDiff(got, e) << " ULP)\n";
                first += os.str();
            }
        }
    }
    void CheckStr(const std::string& got, const std::string& exp, const std::string& what) {
        total++;
        if (got != exp) {
            if (bad++ < 12) first += what + ": got '" + got + "' expected '" + exp + "'\n";
        }
    }
    void CheckVec(const std::vector<float>& got, const json& exp, const std::string& what) {
        if (got.size() != exp.size()) {
            total++;
            bad++;
            first += what + ": size " + std::to_string(got.size()) + " vs " + std::to_string(exp.size()) + "\n";
            return;
        }
        for (size_t i = 0; i < got.size(); i++) Check(got[i], exp[i], what + "[" + std::to_string(i) + "]");
    }
};

// Time, game settings, warp rate and the wheel environment: put back
struct GlobalsScope {
    float dt = Time::deltaTime, fdt = Time::fixedDeltaTime;
    GlobalsScope() = default;
    ~GlobalsScope() {
        Time::deltaTime = dt;
        Time::fixedDeltaTime = fdt;
        WheelModuleEnvironment::Reset();
    }
};

void ApplyGameSettings(const json& env) {
    GameSettings::PART_REPAIR_MASS_PER_KIT = J(env["PART_REPAIR_MASS_PER_KIT"]);
    GameSettings::PART_REPAIR_MAX_KIT_AMOUNT = env["PART_REPAIR_MAX_KIT_AMOUNT"].get<int>();
    GameSettings::WHEEL_WEIGHT_STRESS_MULTIPLIER = J(env["WHEEL_WEIGHT_STRESS_MULTIPLIER"]);
    GameSettings::WHEEL_SLIP_STRESS_MULTIPLIER = J(env["WHEEL_SLIP_STRESS_MULTIPLIER"]);
    GameSettings::WHEEL_SUBSTEPS_ACTIVE = env["WHEEL_SUBSTEPS_ACTIVE"].get<int>();
    GameSettings::WHEEL_SUBSTEPS_INACTIVE = env["WHEEL_SUBSTEPS_INACTIVE"].get<int>();
    GameSettings::WHEEL_AUTO_SPRINGDAMPER = env["WHEEL_AUTO_SPRINGDAMPER"].get<bool>();
    GameSettings::WHEEL_AUTO_STEERINGADJUST = env["WHEEL_AUTO_STEERINGADJUST"].get<bool>();
    GameSettings::WHEEL_DAMAGE_IMPACTCOLLIDER_ENABLED = env["WHEEL_DAMAGE_IMPACTCOLLIDER_ENABLED"].get<bool>();
    GameSettings::WHEEL_DAMAGE_WHEELCOLLIDER_ENABLED = env["WHEEL_DAMAGE_WHEELCOLLIDER_ENABLED"].get<bool>();
}

// A bare vessel owning no parts plus the resource sets of its parts (as tests/test_command.cpp)
struct TestVesselHost final : IResourceVessel {
    Vessel* v;
    VesselResourceSets sets;
    explicit TestVesselHost(Vessel* vessel) : v(vessel) {
        ResourceFlowEnvironment::vesselHost = [this](Vessel* x) -> IResourceVessel* { return x == v ? this : nullptr; };
    }
    ~TestVesselHost() override {
        ResourceFlowEnvironment::vesselHost = nullptr;
        for (Part* p : v->parts) {
            p->crossfeedPartSet.reset();
            p->simulationCrossfeedPartSet.reset();
        }
    }
    void Update() { sets.UpdateResourceSets(v, v->parts); }
    const std::vector<Part*>& ResourceParts() const override { return v->parts; }
    double RequestResource(Part* part, int id, double demand, bool usePriority, bool simulate) override {
        return sets.RequestResource(part, id, demand, usePriority, simulate);
    }
    void GetConnectedResourceTotals(int id, bool simulate, double& amount, double& maxAmount, bool pulling) override {
        sets.GetConnectedResourceTotals(id, simulate, amount, maxAmount, pulling);
    }
    void GetConnectedResourceTotals(int id, bool simulate, double& amount, double& maxAmount, double threshold, bool pulling) override {
        sets.GetConnectedResourceTotals(id, simulate, amount, maxAmount, threshold, pulling);
    }
};

template <class T>
T* FindMod(Part& p) {
    for (size_t i = 0; i < p.Modules().Count(); i++)
        if (auto* m = dynamic_cast<T*>(p.Modules()[i])) return m;
    return nullptr;
}

struct StubLight final : IWheelStatusLight {
    bool on = false;
    int calls = 0;
    void SetStatus(bool s) override {
        on = s;
        calls++;
    }
    bool IsOn() const override { return on; }
};

// quaternion AngleAxis as the Python reference writes it (tests/test_wheels.cpp RefAngleAxis)
Quaternionf RefAngleAxis(float angle, const Vector3f& axis) {
    float t = angle / 360.0f;
    t = t * 2.0f;
    const float rad = static_cast<float>(static_cast<double>(t) * 3.14159265358979323846);
    const float s2 = axis.x * axis.x + axis.y * axis.y + axis.z * axis.z;
    const float mag = static_cast<float>(std::sqrt(static_cast<double>(s2)));
    if (mag > 1e-6f) {
        const float half = rad * 0.5f;
        const float w = static_cast<float>(std::cos(static_cast<double>(half)));
        const float sn = static_cast<float>(std::sin(static_cast<double>(half))) / mag;
        return Quaternionf(sn * axis.x, sn * axis.y, sn * axis.z, w);
    }
    return Quaternionf::identity();
}
struct AngleAxisScope {
    std::function<Quaternionf(float, const Vector3f&)> saved = NativeOps::AngleAxis;
    AngleAxisScope() { NativeOps::AngleAxis = RefAngleAxis; }
    ~AngleAxisScope() { NativeOps::AngleAxis = saved; }
};

// The world of one scripted run. Declaration order = reverse destruction order: the vessels (owned by `env`) are destroyed last.
struct Rig {
    flighttest::FlightEnv env;
    Game15 g;
    SceneScope flight{GameScenes::FLIGHT};
    GlobalsScope globals;
    AngleAxisScope aa;
    CoroutineScheduler sched;
    std::vector<std::string> trace;
    // native objects
    NativeRigidbody rb, otherRbs[8];
    NativeTransform rbTr, colliderHostTr, wheelTr, deployTgtTr, retractTr;
    std::map<std::string, std::unique_ptr<NativeTransform>> dummies;
    Collider colliders[8];
    Animation anim;
    StubLight light;
    Vector3f sampleAxis{};
    std::string animState;
    std::unique_ptr<Part> part, otherPart4, otherPart5;
    Vessel *v = nullptr, *vOther4 = nullptr, *vOther5 = nullptr;
    std::unique_ptr<TestVesselHost> host;
    json kinds;
    ~Rig() {
        host.reset();
        if (v != nullptr) v->parts.clear();
        if (part) part->vessel = nullptr;
        if (otherPart4) otherPart4->vessel = nullptr;
        if (otherPart5) otherPart5->vessel = nullptr;
    }
};

}  // namespace

// ---------------------------------------------------------------------------------------------------------------------------------------
TEST_CASE("WP-29b WheelUtil: SpeedToRpm / RpmToSpeed / RpmToDegsSec vs the independent Python reference, 0 ULP") {
    const json& ref = LoadRef29b();
    Diff d;
    for (const json& r : ref["util"]) {
        d.Check(WheelUtil::SpeedToRpm(J(r["speed"]), J(r["r"])), r["toRpm"], "toRpm");
        d.Check(WheelUtil::RpmToSpeed(J(r["rpm"]), J(r["r"])), r["toSpeed"], "toSpeed");
        d.Check(WheelUtil::RpmToDegsSec(J(r["rpm"])), r["toDegs"], "toDegs");
    }
    INFO(d.first);
    CHECK(d.bad == 0);
    CHECK(d.total == 900);
}

// ---------------------------------------------------------------------------------------------------------------------------------------
TEST_CASE("WP-29b the stock wheel parts are the real module classes with the cfg values the reference parsed") {
    if (!GameAvailable()) return;
    const json& ref = LoadRef29b();
    Rig rig;
    ApplyGameSettings(ref["env"]);
    int modules = 0;
    for (const std::string& name : ref["parts"].get<std::vector<std::string>>()) {
        INFO(name);
        auto p = rig.g.New(name.c_str());
        auto* base = FindMod<ModuleWheelBase>(*p);
        REQUIRE(base != nullptr);
        for (size_t i = 0; i < p->Modules().Count(); i++) {
            PartModule* m = p->Modules()[i];
            if (m->IsA("ModuleWheelSuspension")) REQUIRE(dynamic_cast<ModuleWheelSuspension*>(m) != nullptr);
            if (m->IsA("ModuleWheelSteering")) REQUIRE(dynamic_cast<ModuleWheelSteering*>(m) != nullptr);
            if (m->IsA("ModuleWheelMotor")) REQUIRE(dynamic_cast<ModuleWheelMotor*>(m) != nullptr);
            if (m->IsA("ModuleWheelMotorSteering")) REQUIRE(dynamic_cast<ModuleWheelMotorSteering*>(m) != nullptr);
            if (m->IsA("ModuleWheelBrakes")) REQUIRE(dynamic_cast<ModuleWheelBrakes*>(m) != nullptr);
            if (m->IsA("ModuleWheelLock")) REQUIRE(dynamic_cast<ModuleWheelLock*>(m) != nullptr);
            if (m->IsA("ModuleWheelDamage")) REQUIRE(dynamic_cast<ModuleWheelDamage*>(m) != nullptr);
            if (m->IsA("ModuleWheelDeployment")) REQUIRE(dynamic_cast<ModuleWheelDeployment*>(m) != nullptr);
            if (m->IsA("ModuleWheelBogey")) REQUIRE(dynamic_cast<ModuleWheelBogey*>(m) != nullptr);
            if (dynamic_cast<ModuleWheelSubmodule*>(m) != nullptr) modules++;
        }
        // the curves exist after Awake (ModuleWheelMotor.cs:91 / ModuleWheelSteering.cs:149)
        if (auto* mo = FindMod<ModuleWheelMotor>(*p)) CHECK_FALSE(mo->IsNull("torqueCurve"));
        if (auto* st = FindMod<ModuleWheelSteering>(*p)) {
            CHECK_FALSE(st->IsNull("steeringCurve"));
            CHECK_FALSE(st->IsNull("steeringMaxAngleCurve"));
        }
        // OnAwake: the suspension is auto-tuned unless the cfg says otherwise (maximumLoad is not loaded yet when the prefab Awakes)
        if (auto* su = FindMod<ModuleWheelSuspension>(*p)) CHECK(su->Get<bool>("autoSpringDamper"));
    }
    CHECK(modules >= 50);
}

// ---------------------------------------------------------------------------------------------------------------------------------------
TEST_CASE("WP-29b ModuleWheelBase.ApplyGeeBias / UpdateFriction vs the independent Python reference, 0 ULP") {
    if (!GameAvailable()) return;
    const json& ref = LoadRef29b();
    Rig rig;
    ApplyGameSettings(ref["env"]);
    NativeRigidbody rb;
    NativeTransform tr, ctr;
    Diff d;
    std::string lastPart;
    std::unique_ptr<Part> part;
    for (const json& r : ref["geeBias"]) {
        const std::string name = r["part"].get<std::string>();
        part = rig.g.New(name.c_str());
        auto* b = FindMod<ModuleWheelBase>(*part);
        REQUIRE(b != nullptr);
        b->Set<bool>("autoFriction", r["auto"].get<bool>());
        b->Set<bool>("useNewFrictionModel", r["newModel"].get<bool>());
        if (!r["geeBias0"].is_null()) b->Set<float>("geeBias", J(r["geeBias0"]));
        if (r["hasWheel"].get<bool>()) b->wheel = KSPWheelController::Create(&rb, &tr, &ctr);
        const float ret = b->ApplyGeeBias(J(r["gee"]));
        const std::string w = name + " gee " + std::to_string(J(r["gee"]));
        d.Check(ret, r["ret"], w + " ret");
        d.Check(b->Get<float>("geeBias"), r["geeBias"], w + " geeBias");
        d.Check(b->Get<float>("minimumDownforce"), r["minDown"], w + " minDown");
        d.Check(b->Get<float>("frictionMultiplier"), r["fm"], w + " fm");
        if (r["hasWheel"].get<bool>()) {
            const auto& s = *b->wheel->tireFriction->settings;
            const float got[6] = {s.adherent.x, s.adherent.y, s.peak.x, s.peak.y, s.limit.x, s.limit.y};
            for (int i = 0; i < 6; i++) d.Check(got[i], r["tf"][static_cast<size_t>(i)], w + " tf" + std::to_string(i));
        }
    }
    INFO(d.first);
    CHECK(d.bad == 0);
    CHECK(d.total > 15000);
}

// ---------------------------------------------------------------------------------------------------------------------------------------
TEST_CASE("WP-29b ModuleWheelBase.OnPutToGround vs the independent Python reference, 0 ULP") {
    if (!GameAvailable()) return;
    const json& ref = LoadRef29b();
    Rig rig;
    ApplyGameSettings(ref["env"]);
    Diff d;
    for (const json& r : ref["putToGround"]) {
        auto part = rig.g.New(r["part"].get<std::string>().c_str());
        auto* b = FindMod<ModuleWheelBase>(*part);
        part->partTransform.position = V3j(r["partPos"]);
        part->partTransform.rotation = Qj(r["partRot"]);
        b->WheelOrgPosR = V3j(r["orgPos"]);
        b->Set<float>("groundHeightOffset", J(r["offset"]));
        if (r["retracted"].get<bool>()) {
            auto* dep = FindMod<ModuleWheelDeployment>(*part);
            REQUIRE(dep != nullptr);
            dep->Set<std::string>("stateString", "Retracted");
        }
        PartHeightQuery q(J(r["point0"]));
        q.lowestOnParts[part.get()] = J(r["lowest0"]);
        b->OnPutToGround(q);
        d.Check(q.lowestOnParts[part.get()], r["lowest"], r["part"].get<std::string>() + " lowest");
        d.Check(q.lowestPoint, r["point"], r["part"].get<std::string>() + " point");
    }
    INFO(d.first);
    CHECK(d.bad == 0);
}

// ---------------------------------------------------------------------------------------------------------------------------------------
TEST_CASE("WP-29b SuspensionLoadBalancer.FixedUpdate vs the independent Python reference, 0 ULP") {
    if (!GameAvailable()) return;
    const json& ref = LoadRef29b();
    Rig rig;
    ApplyGameSettings(ref["env"]);
    Diff d;
    NativeRigidbody rb;
    NativeTransform tr, ctr;
    int rows = 0;
    for (const json& r : ref["balancer"]) {
        Vessel* v = rig.env.MakeVessel("balancer", rig.env.Body("Kerbin"), 700000.0, 0.0, true, false, OrbitDriver::UpdateMode::UPDATE, false);
        std::vector<std::unique_ptr<Part>> parts;
        std::vector<std::shared_ptr<KSPWheelController>> wheels;
        const size_t n = r["mods"].size();
        for (size_t i = 0; i < n; i++) {
            auto p = rig.g.New("roverWheel1");
            p->fields.mass = (i == 0) ? J(r["mass"]) : 0.0f;
            p->vessel = v;
            v->parts.push_back(p.get());
            auto* su = FindMod<ModuleWheelSuspension>(*p);
            REQUIRE(su != nullptr);
            if (r["mods"][i]["has_wheel"].get<bool>()) {
                wheels.push_back(KSPWheelController::Create(&rb, &tr, &ctr));
                wheels.back()->Initialize();
                wheels.back()->IsGrounded(r["mods"][i]["grounded"].get<bool>());
                su->wheel = wheels.back().get();
            } else {
                wheels.push_back(nullptr);
            }
            parts.push_back(std::move(p));
        }
        v->Landed = r["landed"].get<bool>();
        v->gravityMultiplier = JD(r["gm"]);
        rig.env.Body("Kerbin")->GeeASL = JD(r["gee"]);
        SuspensionLoadBalancer lb(v);
        lb.multiplier = J(r["mult"]);
        lb.minMass = J(r["minMass"]);
        lb.OnStart();
        lb.FixedUpdate();
        REQUIRE(lb.SuspensionModules().size() == n);
        for (size_t i = 0; i < n; i++) {
            if (r["out"][i].is_null()) continue;
            d.Check(lb.SuspensionModules()[i]->sprungMassGravity, r["out"][i], "balancer row " + std::to_string(rows) + " mod " + std::to_string(i));
        }
        rows++;
        v->parts.clear();
        for (auto& p : parts) p->vessel = nullptr;
        for (auto& w : wheels)
            if (w) w->IsGrounded(false);
        // the suspension modules point at controllers that die with `wheels`: forget them first
        for (auto& p : parts) FindMod<ModuleWheelSuspension>(*p)->wheel = nullptr;
    }
    INFO(d.first);
    CHECK(d.bad == 0);
    CHECK(rows == 400);
}

// ---------------------------------------------------------------------------------------------------------------------------------------
namespace {

std::vector<float> SnapFloats(const std::vector<float>& v) { return v; }

float B(bool b) { return b ? 1.0f : 0.0f; }

// The per-tick observation (tools/wp29b/wheel_reference.py snap())
void Observe(Rig& rig, std::map<std::string, std::vector<float>>& nums, std::string& deployState) {
    Part* p = rig.part.get();
    auto* base = FindMod<ModuleWheelBase>(*p);
    KSPWheelController* wh = base->Wheel();
    nums.clear();
    {
        const auto& s = *wh->tireFriction->settings;
        nums["wheel"] = {wh->maxDriveTorque, wh->maxBrakeTorque, wh->maxSteerAngle, wh->maxRpm, wh->wheelDamping, wh->driveInput, wh->brakeInput,
                         wh->steerInput, wh->steeringResponse, wh->driveResponse, wh->brakeResponse, wh->minimumdownForce,
                         static_cast<float>(wh->integrationSteps), s.adherent.x, s.adherent.y, s.peak.x, s.peak.y, s.limit.x, s.limit.y,
                         B(wh->enabled()), B(wh->IsGrounded())};
        VPWheelCollider* c = wh->wheelCollider;
        nums["col"] = {c->springRate, c->damperRate, c->suspensionDistance, c->suspensionAnchor, c->suspensionOffset, c->radius, c->mass,
                       c->springSlerpRate, B(c->updateSuspension), B(c->updateCaliper), B(c->updateWheel), c->center.x, c->center.y, c->center.z,
                       static_cast<float>(c->layerMask)};
    }
    nums["base"] = {B(base->Get<bool>("isGrounded")), base->Get<float>("frictionMultiplier"), base->Get<float>("geeBias"),
                    base->Get<float>("minimumDownforce"), base->slipDisplacement.x, base->slipDisplacement.y,
                    static_cast<float>(base->InopSystems()->Types()), B(p->fields.GroundContact), B(rig.v->Landed),
                    static_cast<float>(rig.v->GroundContacts.size()), B(base->rbBrakeConstraints)};
    for (size_t i = 0; i < p->Modules().Count(); i++) {
        PartModule* m = p->Modules()[i];
        if (auto* mo = dynamic_cast<ModuleWheelMotor*>(m)) {
            auto* ms = dynamic_cast<ModuleWheelMotorSteering*>(m);
            double ec = 0.0;
            if (PartResource* r = p->GetResource("ElectricCharge")) ec = r->amount;
            nums["motor"] = {mo->Get<float>("driveOutput"), mo->Get<float>("driveLimiter"), static_cast<float>(static_cast<int>(mo->state)),
                             static_cast<float>(mo->resourceFraction), mo->driveInput, mo->maxTorque, ms ? ms->wDot : 0.0f,
                             B(mo->Get<bool>("motorEnabled")), static_cast<float>(ec)};
        } else if (auto* st = dynamic_cast<ModuleWheelSteering*>(m)) {
            nums["steer"] = {st->steeringInput, st->steerAngle, st->Get<float>("steeringRange"), st->Get<float>("angleTweakable"),
                             st->Get<float>("responseTweakable"), st->CoMfwdLength(), B(st->CaliperUpdating()), st->h, st->tPivot.x, st->tPivot.y,
                             st->tPivot.z, st->tCenter.x, st->tCenter.y, st->tCenter.z};
        } else if (auto* br = dynamic_cast<ModuleWheelBrakes*>(m)) {
            nums["brakes"] = {br->Get<float>("brakeInput"), br->Get<float>("brakeTweakable"), B(rig.light.on), static_cast<float>(rig.light.calls)};
            if (br->statusLight == nullptr) nums["brakes"][2] = 0.0f, nums["brakes"][3] = 0.0f;
        } else if (auto* su = dynamic_cast<ModuleWheelSuspension*>(m)) {
            nums["susp"] = {su->spring, su->damper, su->Get<float>("springTweakable"), su->Get<float>("damperTweakable"), su->Get<float>("autoBoost"),
                            su->boost, su->suspCompression, su->suspMass, su->sprungMassGravity, B(su->highGeeOverride), B(su->autoSpringDamperSet),
                            su->springClampMax, su->damperClampMax, su->damperFudge, B(su->Get<bool>("autoSpringDamper"))};
        } else if (auto* da = dynamic_cast<ModuleWheelDamage*>(m)) {
            nums["damage"] = {da->currentDeflection, da->currentSlip, da->currentDownForce, da->totalStress, da->Get<float>("stressPercent"),
                              B(da->Get<bool>("isDamaged")), da->startupTime, da->stressTime(), da->stressVariability, da->repairImmunityTimeLeft(),
                              da->repairImmunityTimeTotal(), static_cast<float>(da->repairKitsNecessary)};
        } else if (auto* de = dynamic_cast<ModuleWheelDeployment*>(m)) {
            AnimationState* as = rig.anim[rig.animState];
            nums["deploy"] = {de->position, de->Position(), B(de->subsystemsDisabled), as->normalizedTime(), as->speed};
            deployState = de->Get<std::string>("stateString");
        } else if (auto* lk = dynamic_cast<ModuleWheelLock*>(m)) {
            nums["lock"] = {lk->Get<float>("maxTorque")};
        } else if (auto* bg = dynamic_cast<ModuleWheelBogey*>(m)) {
            nums["bogey"] = {bg->bogeyAngle, bg->bogeyUp.x, bg->bogeyUp.y, bg->bogeyUp.z, bg->bogeyRot.x, bg->bogeyRot.y, bg->bogeyRot.z, bg->bogeyRot.w};
        }
    }
}

// the observation after a step against the reference's: every numeric section bit for bit, the deployment state, the trace in order
void CompareObs(Rig& rig, const json& out, const std::string& where, Diff& d, long& trSteps) {
    std::map<std::string, std::vector<float>> nums;
    std::string deployState;
    Observe(rig, nums, deployState);
    for (auto it = out.begin(); it != out.end(); ++it) {
        const std::string key = it.key();
        if (key == "trace") continue;
        if (key == "deployState") {
            d.CheckStr(deployState, it.value().get<std::string>(), where + " deployState");
            continue;
        }
        auto f = nums.find(key);
        if (f == nums.end()) {
            d.bad++;
            d.total++;
            d.first += where + ": missing section " + key + "\n";
            continue;
        }
        d.CheckVec(f->second, it.value(), where + " " + key);
    }
    const std::vector<std::string> et = out["trace"].get<std::vector<std::string>>();
    d.CheckStr(std::to_string(rig.trace.size()), std::to_string(et.size()), where + " trace length");
    for (size_t i = 0; i < std::min(et.size(), rig.trace.size()); i++) d.CheckStr(rig.trace[i], et[i], where + " trace " + std::to_string(i));
    trSteps += static_cast<long>(et.size());
    rig.trace.clear();
}

void ReplayRun(Rig& rig, const json& run, const json& ref, Diff& d, long& ticks, long& trSteps) {
    const std::string name = run["part"].get<std::string>();
    INFO(name << " gee " << run["gee"] << " seed " << run["seed"]);
    ApplyGameSettings(ref["env"]);
    rig.kinds = ref["kinds"];
    const float dt = J(run["dt"]), fdt = J(run["fdt"]);
    Time::deltaTime = dt;
    Time::fixedDeltaTime = fdt;
    rig.env.tw.curr_rate = J(run["rate"]);
    UnityRandom::Global().InitState(run["seed"].get<int>() + 7);
    // the vessel
    rig.sched = CoroutineScheduler();
    rig.light = StubLight();
    rig.trace.clear();
    rig.v = rig.env.MakeVessel("wheel", rig.env.Body("Kerbin"), 700000.0, 0.0, true, false, OrbitDriver::UpdateMode::UPDATE, false);
    rig.v->Landed = false;
    rig.v->situation = Vessel::Situations::LANDED;
    rig.env.Body("Kerbin")->GeeASL = JD(run["gee"]);
    rig.v->vesselTransform.rotation = Qj(run["refRot"]);
    rig.v->CoM = V3j(run["CoM"]);
    rig.v->upAxis = Vector3d(JD(run["upAxis"][0]), JD(run["upAxis"][1]), JD(run["upAxis"][2]));
    rig.v->gravityForPos = Vector3d(JD(run["gravityForPos"][0]), JD(run["gravityForPos"][1]), JD(run["gravityForPos"][2]));
    rig.v->gravityMultiplier = 1.0;
    rig.v->totalMass = JD(run["totalMass"]);
    // the other vessels a wheel can touch (colliders 4 and 5)
    rig.vOther4 = rig.env.MakeVessel("other4", rig.env.Body("Kerbin"), 700000.0, 0.0, true, false, OrbitDriver::UpdateMode::UPDATE, false);
    rig.vOther4->Landed = true;
    rig.vOther4->landedAt = "KSC";
    rig.vOther4->displaylandedAt = "KSC Pad";
    rig.vOther5 = rig.env.MakeVessel("other5", rig.env.Body("Kerbin"), 700000.0, 0.0, true, false, OrbitDriver::UpdateMode::UPDATE, false);
    rig.otherPart4 = std::make_unique<Part>();
    rig.otherPart4->vessel = rig.vOther4;
    rig.otherPart5 = std::make_unique<Part>();
    rig.otherPart5->vessel = rig.vOther5;
    // the part
    rig.part = rig.g.New(name.c_str());
    Part* p = rig.part.get();
    p->vessel = rig.v;
    rig.v->parts.push_back(p);
    p->partTransform.position = V3j(run["partPos"]);
    p->partTransform.rotation = Qj(run["partRot"]);
    rig.rb.velocity = V3j(run["rbVel"]);
    rig.rbTr.rotation = Qj(run["partRot"]);
    rig.rbTr.position = V3j(run["partPos"]);
    for (NativeTransform* t : {&rig.colliderHostTr, &rig.wheelTr}) {
        t->position = V3j(run["wheelTrPos"]);
        t->rotation = Qj(run["wheelTrRot"]);
    }
    {
        ConfigNode rn("RESOURCE");
        rn.AddValue("name", "ElectricCharge");
        rn.AddValue("amount", "0");
        rn.AddValue("maxAmount", "100");
        p->AddResource(rn);
        PartResource* r = p->GetResource("ElectricCharge");
        r->amount = run["ec0"].get<double>();
        p->ResetSimulationResources();
    }
    rig.host = std::make_unique<TestVesselHost>(rig.v);
    rig.host->Update();
    // the colliders
    for (int k = 0; k < 8; k++) {
        const json& kd = rig.kinds[std::to_string(k)];
        rig.colliders[k].attachedRigidbody = nullptr;
        if (!kd["rbVel"].is_null()) {
            rig.otherRbs[k].velocity = V3j(kd["rbVel"]);
            rig.colliders[k].attachedRigidbody = &rig.otherRbs[k];
        }
    }
    // the animation clock of the deployment module
    ModuleWheelDeployment* dep = FindMod<ModuleWheelDeployment>(*p);
    rig.anim = Animation();
    if (dep != nullptr) {
        rig.animState = dep->Get<std::string>("animationStateName");
        rig.anim.AddClip(rig.animState, J(run["animLen"]));
        rig.sampleAxis = V3j(run["sampleLine"]["axis"]);
        rig.retractTr.position = Vector3f::zero();
    }
    // hooks
    WheelModuleEnvironment::scheduler = &rig.sched;
    WheelModuleEnvironment::maximumSurfaceGeeASL = [&] { return JD(ref["env"]["MAX_SURFACE_GEE"]); };
    WheelModuleEnvironment::timeSinceLevelLoad = [] { return 10.0f; };
    WheelModuleEnvironment::trace = [&rig](const std::string& s) { rig.trace.push_back(s); };
    WheelModuleEnvironment::describeCollider = [&rig](const Collider* c) {
        WheelColliderInfo info;
        for (int k = 0; k < 8; k++) {
            if (&rig.colliders[k] != c) continue;
            const json& kd = rig.kinds[std::to_string(k)];
            info.layer = kd["layer"].get<int>();
            info.tag = kd["tag"].get<std::string>();
            info.hasCrashObjectName = kd.contains("crash") && kd["crash"].get<bool>();
            if (kd.contains("roc")) {
                info.rocTerrain = true;
                info.rocTerrainTag = kd["roc"].get<std::string>();
            }
            if (k == 4) info.part = rig.otherPart4.get();
            if (k == 5) info.part = rig.otherPart5.get();
        }
        return info;
    };
    WheelModuleEnvironment::findModelTransform = [&rig](Part& pp, const std::string& nm) -> NativeTransform* {
        auto* b = FindMod<ModuleWheelBase>(pp);
        if (nm == b->Get<std::string>("wheelColliderTransformName")) return &rig.colliderHostTr;
        if (!b->Get<std::string>("wheelTransformName").empty() && nm == b->Get<std::string>("wheelTransformName")) return &rig.wheelTr;
        auto* d2 = FindMod<ModuleWheelDeployment>(pp);
        if (d2 != nullptr && !d2->Get<std::string>("retractTransformName").empty() && nm == d2->Get<std::string>("retractTransformName")) return &rig.retractTr;
        auto& slot = rig.dummies[nm];
        if (!slot) slot = std::make_unique<NativeTransform>();
        return slot.get();
    };
    WheelModuleEnvironment::findModelAnimator = [&rig](Part&, const std::string&, const std::string&) -> Animation* { return &rig.anim; };
    WheelModuleEnvironment::sampleAnimation = [&rig](Animation& a) {
        rig.retractTr.position = rig.sampleAxis * a[rig.animState]->normalizedTime();
    };
    WheelModuleEnvironment::asStatusLight = [&rig](PartModule& m) -> IWheelStatusLight* { return m.IsA("ModuleStatusLight") ? &rig.light : nullptr; };
    WheelModuleEnvironment::isActiveVessel = [](const Vessel*) { return true; };
    WheelModuleEnvironment::shieldedFromAirstream = [](const Part&) { return false; };

    auto* base = FindMod<ModuleWheelBase>(*p);
    REQUIRE(base != nullptr);
    base->partRigidbody = &rig.rb;
    base->partRigidbodyTransform = &rig.rbTr;
    base->driftCorrection = run["drift"].get<bool>();
    if (auto* dm = FindMod<ModuleWheelDamage>(*p)) {
        if (!run["stressTol"].is_null()) dm->Set<float>("stressTolerance", J(run["stressTol"]));
        if (!run["impactTol"].is_null()) dm->Set<float>("impactTolerance", J(run["impactTol"]));
    }
    auto* bogey = FindMod<ModuleWheelBogey>(*p);
    if (bogey != nullptr) {
        bogey->bogeyParentRotation = Qj(run["bogeyParentRot"]);
        bogey->bogeyRefLocalRotation = Qj(run["bogeyRefLocalRot"]);
    }

    // OnStart of every module in cfg order while Part.Start has not finished, then the next frame's coroutine pass runs wheelSetup
    p->fields.started = false;
    for (size_t i = 0; i < p->Modules().Count(); i++) {
        PartModule* m = p->Modules()[i];
        if (m->IsA("ModuleWheelBase") || dynamic_cast<ModuleWheelSubmodule*>(m) != nullptr) m->OnStart(StartState::Flying);
    }
    p->fields.started = true;
    rig.sched.AdvanceFrame(dt);
    rig.sched.RunDelayedDynamicFrameRate();
    REQUIRE(base->IsWheelSetUp());
    CompareObs(rig, run["afterStart"], name + " gee " + std::to_string(run["gee"].get<double>()) + " after set-up", d, trSteps);
    for (size_t t = 0; t < run["ticks"].size(); t++) {
        const json& tk = run["ticks"][t];
        const std::string where = name + " gee " + std::to_string(run["gee"].get<double>()) + " tick " + std::to_string(t);
        Vessel::env.frameCount = static_cast<int>(t) + 1;
        rig.env.tw.curr_rate = J(tk["rate"]);
        rig.sched.AdvanceFrame(dt);
        rig.v->ctrlState.wheelThrottle = J(tk["ctrl"][0]);
        rig.v->ctrlState.wheelThrottleTrim = J(tk["ctrl"][1]);
        rig.v->ctrlState.wheelSteer = J(tk["ctrl"][2]);
        rig.v->ctrlState.wheelSteerTrim = J(tk["ctrl"][3]);
        rig.v->srfSpeed = JD(tk["srfSpeed"]);
        if (!tk["ecSet"].is_null()) {
            PartResource* r = p->GetResource("ElectricCharge");
            r->amount = JD(tk["ecSet"]);
            p->ResetSimulationResources();
        }
        auto* brakes = FindMod<ModuleWheelBrakes>(*p);
        auto* motor = FindMod<ModuleWheelMotor>(*p);
        auto* steer = FindMod<ModuleWheelSteering>(*p);
        auto* susp = FindMod<ModuleWheelSuspension>(*p);
        auto* dmg = FindMod<ModuleWheelDamage>(*p);
        for (const json& ev : tk["events"]) {
            const std::string en = ev[0].get<std::string>();
            const int typ = ev[1].get<int>();
            const float val = J(ev[2]);
            KSPActionParam param(0, static_cast<KSPActionType>(typ));
            if (en == "brake" && brakes) brakes->BrakeAction(param);
            else if (en == "brakeAction" && brakes) brakes->Set<float>("brakeInput", val > 1.0f ? 0.5f : brakes->Get<float>("brakeInput"));
            else if (en == "motor" && motor) {
                if (typ == 2) motor->MotorToggle(param);
                else if (typ == 0) motor->MotorEnable(param);
                else motor->MotorDisable(param);
            } else if (en == "tractionOff" && motor) motor->EvtAutoTorqueToggle();
            else if (en == "gear" && dep) {
                if (typ == 2 && val > 1.0f) dep->EventToggle();
                else dep->ActionToggle(param);
            } else if (en == "friction") {
                if (val > 1.0f) base->EvtAutoFrictionToggle();
                else base->ActAutoFrictionToggle(param);
            } else if (en == "steer" && steer) steer->SteeringToggle(param);
            else if (en == "susp" && susp) susp->EvtAutoSpringDamperToggle();
            else if (en == "startup0" && dmg) dmg->startupTime = 0.0f;
            else if (en == "suspEnable") {
                if (typ == 0) base->EnableSuspension(param);
                else base->DisableSuspension(param);
            } else if (en == "damage" && dmg) {
                if (typ == 1) dmg->Repair();
                else dmg->DamageWheel();
            } else if (en == "pack") {
                p->fields.packed = (typ != 1);
                if (typ == 1) base->OnPartUnpack(p);
                else base->OnPartPack(p);
            } else if (en == "soi") {
                rig.env.Body("Kerbin")->GeeASL = static_cast<double>(val);
                base->SOIChange(rig.v, rig.env.Body("Kerbin"), rig.env.Body("Kerbin"));
            } else if (en == "dock") base->OnDockingComplete(p, rig.otherPart4.get());
            else if (en == "undock") base->OnVesselUndocking(rig.v, rig.vOther4);
        }
        // Update phase
        rig.sched.RunDelayedDynamicFrameRate();
        if (dep != nullptr) dep->Update();
        if (KSPWheelController* wh = base->Wheel(); wh != nullptr && wh->currentState() != nullptr) {
            VehicleBase::WheelState& cs = *wh->currentState();
            cs.grounded = tk["grounded"].get<bool>();
            cs.angularVelocity = J(tk["angVel"]);
            cs.localWheelVelocity = Vector2f(J(tk["lwv"][0]), J(tk["lwv"][1]));
            cs.suspensionCompression = J(tk["comp"]);
            cs.hit.sidewaysSlip = J(tk["side"]);
            cs.hit.normal = V3j(tk["normal"]);
            cs.downforce = J(tk["down"]);
            cs.contactDepth = J(tk["depth"]);
            const int kind = tk["coll"].get<int>();
            cs.hit.collider = kind == 0 ? nullptr : &rig.colliders[kind];
            wh->wheelCollider->wheelColliderData.suspensionSpring.spring = J(tk["spring"]);
        }
        // FixedUpdate phase (cfg order)
        for (size_t i = 0; i < p->Modules().Count(); i++) {
            PartModule* m = p->Modules()[i];
            if (auto* x = dynamic_cast<ModuleWheelBase*>(m)) x->FixedUpdate();
            else if (auto* x2 = dynamic_cast<ModuleWheelMotor*>(m)) x2->FixedUpdate();
            else if (auto* x3 = dynamic_cast<ModuleWheelBrakes*>(m)) x3->FixedUpdate();
            else if (auto* x4 = dynamic_cast<ModuleWheelSuspension*>(m)) x4->FixedUpdate();
            else if (auto* x5 = dynamic_cast<ModuleWheelSteering*>(m)) x5->FixedUpdate();
            else if (auto* x6 = dynamic_cast<ModuleWheelDamage*>(m)) x6->FixedUpdate();
            else if (auto* x7 = dynamic_cast<ModuleWheelDeployment*>(m)) x7->FixedUpdate();
        }
        if (dep != nullptr) {
            rig.anim.Tick(fdt);
            WheelModuleEnvironment::sampleAnimation(rig.anim);
        }
        for (size_t i = 0; i < p->Modules().Count(); i++) {
            PartModule* m = p->Modules()[i];
            if (auto* x = dynamic_cast<ModuleWheelSuspension*>(m)) x->LateUpdate();
            else if (auto* x2 = dynamic_cast<ModuleWheelDeployment*>(m)) x2->LateUpdate();
            else if (auto* x3 = dynamic_cast<ModuleWheelBogey*>(m)) x3->LateUpdate();
        }
        p->partValues.SteeringRadius.Update();
        CompareObs(rig, run["out"][t], where, d, trSteps);
        ticks++;
        if (d.bad > 0) break;   // the first divergence is the root cause
    }
    // teardown of the run
    WheelModuleEnvironment::Reset();
    rig.host.reset();
    rig.v->parts.clear();
    p->vessel = nullptr;
    base->wheel.reset();
    rig.part.reset();
    rig.sched = CoroutineScheduler();
}

}  // namespace

TEST_CASE("WP-29b wheel part modules over 39 scripted runs (every stock wheel part, 3 bodies) vs the independent Python reference, 0 ULP") {
    if (!GameAvailable()) return;
    const json& ref = LoadRef29b();
    Diff d;
    long ticks = 0, trSteps = 0;
    Rig rig;
    for (const json& run : ref["runs"]) {
        ReplayRun(rig, run, ref, d, ticks, trSteps);
        if (d.bad > 0) break;
    }
    INFO(d.first);
    CHECK(d.bad == 0);
    CHECK(ticks == 39 * 200);
    CHECK(d.total > 500000);
    CHECK(trSteps > 1000);
}
