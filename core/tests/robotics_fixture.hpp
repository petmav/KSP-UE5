// WP-52 test support: the reference loader, the bit comparisons and the one-part flight rig of tests/test_robotics*.cpp.
#pragma once

#include <array>
#include <cmath>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>

#include "doctest/doctest.h"
#include "flight_fixture.hpp"
#include "nlohmann/json.hpp"
#include "resource_fixture.hpp"

#include "ksp/core/coroutine.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/input/game_settings.hpp"
#include "ksp/modules/base_event.hpp"
#include "ksp/math/util_math.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/physics/vessel_physics.hpp"
#include "ksp/serenity/robotic_controller.hpp"
#include "ksp/serenity/servo_modules.hpp"
#include "ksp/unity/quaternion_native.hpp"

using namespace ksp;
using namespace ksp::rtest;
using namespace ksp::Expansions::Serenity;
using json = nlohmann::json;

namespace wp52t {
using namespace ksp;
using namespace ksp::rtest;
using namespace ksp::Expansions::Serenity;


inline const json& LoadRef52() {
    static const json j = [] {
        std::ifstream f(std::string(KSPCORE_TEST_DATA_DIR) + "/wp52_reference.json", std::ios::binary);
        REQUIRE(f.good());
        return json::parse(f);
    }();
    return j;
}

// the reference writes a float as a number, or "nan" / "inf" / "-inf" / "-0"
inline float J(const json& v) {
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
inline double JD(const json& v) {
    if (v.is_string()) return static_cast<double>(J(v));
    return v.get<double>();
}
inline bool Same(float a, float b) {
    if (std::isnan(a) && std::isnan(b)) return true;
    uint32_t x, y;
    std::memcpy(&x, &a, 4);
    std::memcpy(&y, &b, 4);
    return x == y;
}
inline bool SameD(double a, double b) {
    if (std::isnan(a) && std::isnan(b)) return true;
    uint64_t x, y;
    std::memcpy(&x, &a, 8);
    std::memcpy(&y, &b, 8);
    return x == y;
}
inline int64_t UlpDiff(float a, float b) {
    int32_t x, y;
    std::memcpy(&x, &a, 4);
    std::memcpy(&y, &b, 4);
    if (x < 0) x = INT32_MIN - x;
    if (y < 0) y = INT32_MIN - y;
    return static_cast<int64_t>(x) - static_cast<int64_t>(y);
}

struct Diff {
    long total = 0, bad = 0;
    std::string first;
    std::map<std::string, int> seen;  // one report per scenario + field
    void Report(const std::string& what, const std::string& text) {
        std::string key = what;
        const size_t t = key.find(" t");
        if (t != std::string::npos) {
            const size_t sp = key.find(' ', t + 2);
            key = key.substr(0, t) + (sp == std::string::npos ? "" : key.substr(sp));
        }
        if (seen[key]++ == 0 && seen.size() < 60) first += what + ": " + text + "\n";
    }
    void CheckF(float got, float want, const std::string& what) {
        total++;
        if (!Same(got, want)) {
            bad++;
            char buf[160];
            std::snprintf(buf, sizeof buf, "got %.9g want %.9g (%lld ulp)", got, want, static_cast<long long>(UlpDiff(got, want)));
            Report(what, buf);
        }
    }
    void CheckD(double got, double want, const std::string& what) {
        total++;
        if (!SameD(got, want)) {
            bad++;
            char buf[160];
            std::snprintf(buf, sizeof buf, "got %.17g want %.17g", got, want);
            Report(what, buf);
        }
    }
    void CheckB(bool got, bool want, const std::string& what) {
        total++;
        if (got != want) {
            bad++;
            Report(what, "got " + std::to_string(got) + " want " + std::to_string(want));
        }
    }
    void CheckS(const std::string& got, const std::string& want, const std::string& what) {
        total++;
        if (got != want) {
            bad++;
            Report(what, "got '" + got + "' want '" + want + "'");
        }
    }
};

// a bare vessel owning no parts plus the resource sets of its parts (as tests/test_wheel_modules.cpp)
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

struct EnvReset {
    ~EnvReset() {
        ServoEnvironment::Reset();
        ControllerEnvironment::Reset();
        SerenityMiscEnvironment::Reset();
        RoboticControllerManager::Instance = nullptr;
    }
};

// The world of one test case: the Kerbol system, the stock game, the flight scene. Declaration order = reverse destruction order.
struct Rig {
    flighttest::FlightEnv env;
    Game15 g;
    SceneScope flight{GameScenes::FLIGHT};
    EnvReset reset;
    struct One {
        Vessel* v = nullptr;
        std::unique_ptr<Part> part;
        std::unique_ptr<TestVesselHost> host;
        std::vector<std::unique_ptr<Part>> extra;  // more parts of the same vessel (AddPart)
        ~One() {
            host.reset();
            if (v != nullptr) {
                v->parts.clear();
                v->rootPart = nullptr;
            }
            if (part) part->vessel = nullptr;
            for (auto& e : extra) e->vessel = nullptr;
        }
    };
    // another part on the vessel of `o` (a distinct persistentId, its own electric charge store)
    Part* AddPart(One& o, const char* partName = "mk1pod.v2", double ec = 1.0e6) {
        auto q = g.New(partName);
        Part* p = q.get();
        p->vessel = o.v;
        o.v->parts.push_back(p);
        p->fields.persistentId = 9100 + static_cast<uint32_t>(o.extra.size());
        ConfigNode rn("RESOURCE");
        rn.AddValue("name", "ElectricCharge");
        rn.AddValue("amount", "0");
        rn.AddValue("maxAmount", "100");
        p->AddResource(rn);
        PartResource* r = p->GetResource("ElectricCharge");
        r->amount = ec;
        r->maxAmount = ec > 100.0 ? ec : 100.0;
        p->ResetSimulationResources();
        o.extra.push_back(std::move(q));
        o.host->Update();
        return p;
    }
    // One vessel with one stock part (a probe core) that carries `modules` (name + field overrides) and a big electric charge store
    std::unique_ptr<One> MakeOne(const char* partName = "mk1pod.v2", double ec = 1.0e6) {
        auto o = std::make_unique<One>();
        o->v = env.MakeVessel("servo", env.Body("Kerbin"), 700000.0, 0.0, true, false, OrbitDriver::UpdateMode::UPDATE, false);
        o->part = g.New(partName);
        Part* p = o->part.get();
        p->vessel = o->v;
        o->v->parts.push_back(p);
        o->v->rootPart = p;
        p->fields.persistentId = 9001;
        ConfigNode rn("RESOURCE");
        rn.AddValue("name", "ElectricCharge");
        rn.AddValue("amount", "0");
        rn.AddValue("maxAmount", "100");
        p->AddResource(rn);
        PartResource* r = p->GetResource("ElectricCharge");
        r->amount = ec;
        r->maxAmount = ec > 100.0 ? ec : 100.0;
        p->ResetSimulationResources();
        o->host = std::make_unique<TestVesselHost>(o->v);
        o->host->Update();
        return o;
    }
};

// MODULE node of a servo class with the SYNTHETIC input resource (the real servo parts are in SquadExpansion, not installed)
template <class T>
inline T* AddServo(Part& p, const char* cls, double ecRate = 1.0) {
    ConfigNode node("MODULE");
    node.AddValue("name", cls);
    ConfigNode* res = node.AddNode("RESOURCE");
    res->AddValue("name", "ElectricCharge");
    res->AddValue("rate", std::to_string(ecRate));
    REQUIRE_MESSAGE(p.AddModule(node, true) != nullptr, cls);
    T* t = dynamic_cast<T*>(p.FindModuleOfClass(cls));
    REQUIRE(t != nullptr);
    return t;
}

inline void WireRig(BaseServo& s, const Matrix4x4f& parentWorld = Matrix4x4f::identity()) {
    auto moving = std::make_shared<ServoNode>();
    auto base = std::make_shared<ServoNode>();
    auto jp = std::make_shared<ServoNode>();
    moving->name = "Moving";
    base->name = "Base";
    jp->name = "JointParent";
    jp->localToWorld = parentWorld;
    moving->parent = jp;
    s.rig.nodes["Moving"] = moving;
    s.rig.nodes["Base"] = base;
    s.rig.nodes["JointParent"] = jp;
    s.Set<std::string>("servoTransformName", "Moving");
    s.Set<std::string>("baseTransformName", "Base");
}

inline Vector2f V2j(const json& a) { return Vector2f(J(a[0]), J(a[1])); }
inline Matrix4x4f MatJ(const json& m) {
    Matrix4x4f r;
    r.m00 = J(m[0][0]); r.m01 = J(m[0][1]); r.m02 = J(m[0][2]); r.m03 = J(m[0][3]);
    r.m10 = J(m[1][0]); r.m11 = J(m[1][1]); r.m12 = J(m[1][2]); r.m13 = J(m[1][3]);
    r.m20 = J(m[2][0]); r.m21 = J(m[2][1]); r.m22 = J(m[2][2]); r.m23 = J(m[2][3]);
    r.m30 = J(m[3][0]); r.m31 = J(m[3][1]); r.m32 = J(m[3][2]); r.m33 = J(m[3][3]);
    return r;
}

// write one scripted field value into the module (typed by the JSON value and the stored type)
inline void SetFieldJ(BaseServo& s, const std::string& name, const json& v) {
    ModuleValue* cur = s.Value(name);
    REQUIRE_MESSAGE(cur != nullptr, name);
    if (std::holds_alternative<bool>(*cur)) *cur = v.get<bool>();
    else if (std::holds_alternative<float>(*cur)) *cur = J(v);
    else if (std::holds_alternative<std::string>(*cur)) *cur = v.get<std::string>();
    else if (std::holds_alternative<Vector2f>(*cur)) *cur = V2j(v);
    else if (std::holds_alternative<int32_t>(*cur)) *cur = v.get<int32_t>();
    else if (std::holds_alternative<EnumValue>(*cur)) s.SetEnumByName(name, v.get<std::string>());
    else FAIL("unsupported field type " << name);
}
inline void SetValueJ(BaseServo& s, const std::string& name, const json& v) {
    ModuleValue* cur = s.Value(name);
    REQUIRE_MESSAGE(cur != nullptr, name);
    if (std::holds_alternative<bool>(*cur)) s.Fields()[name]->SetValue(ModuleValue(v.get<bool>()));
    else s.Fields()[name]->SetValue(ModuleValue(J(v)));
}

using Obs = std::map<std::string, std::vector<float>>;

inline std::vector<float> Drive(const JointDrive& d) { return {d.positionSpring, d.positionDamper, d.maximumForce}; }
inline std::vector<float> V3(const Vector3f& v) { return {v.x, v.y, v.z}; }
inline float B(bool b) { return b ? 1.0f : 0.0f; }


// a ModuleRoboticController on the part (SYNTHETIC input resource; the real controller part is in SquadExpansion, not installed); the LAST one is returned
inline ModuleRoboticController* AddController(Part& p) {
    ConfigNode node("MODULE");
    node.AddValue("name", "ModuleRoboticController");
    ConfigNode* res = node.AddNode("RESOURCE");
    res->AddValue("name", "ElectricCharge");
    res->AddValue("rate", "1.0");
    REQUIRE(p.AddModule(node, true) != nullptr);
    ModuleRoboticController* last = nullptr;
    for (size_t i = 0; i < p.Modules().Count(); i++)
        if (auto* c = dynamic_cast<ModuleRoboticController*>(p.Modules()[i])) last = c;
    REQUIRE(last != nullptr);
    return last;
}

}  // namespace wp52t
