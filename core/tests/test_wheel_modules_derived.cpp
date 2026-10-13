// WP-29b unit tests derived from the C# control flow of the wheel part modules (include/ksp/wheels/wheel_modules.hpp). The numeric gate is
// tests/test_wheel_modules.cpp (the independent Python reference); the cases here have no reference: they follow the decompiled C# line by line
// (ModuleWheelBase quirks, error paths, persistence, adjusters, IConstruction).
#include <cmath>
#include <cstring>
#include <fstream>

#include "doctest/doctest.h"
#include "flight_fixture.hpp"
#include "ksp/input/game_settings.hpp"
#include "ksp/modules/control_types.hpp"
#include "ksp/wheels/wheel_modules.hpp"
#include "nlohmann/json.hpp"
#include "resource_fixture.hpp"

using namespace ksp;
using namespace ksp::rtest;
using namespace ksp::VehiclePhysics;
using namespace ksp::ModuleWheels;
using json = nlohmann::json;

namespace {

std::vector<std::string> StockWheelParts() {
    std::ifstream f(std::string(KSPCORE_TEST_DATA_DIR) + "/wp29b_reference.json", std::ios::binary);
    REQUIRE(f.good());
    const json j = json::parse(f);
    return j["parts"].get<std::vector<std::string>>();
}

template <class T>
T* FindMod(Part& p) {
    for (size_t i = 0; i < p.Modules().Count(); i++)
        if (auto* m = dynamic_cast<T*>(p.Modules()[i])) return m;
    return nullptr;
}

bool SameBits(float a, float b) {
    uint32_t x, y;
    std::memcpy(&x, &a, 4);
    std::memcpy(&y, &b, 4);
    return x == y;
}

struct WheelGlobals {
    bool a = GameSettings::WHEEL_DAMAGE_IMPACTCOLLIDER_ENABLED, b = GameSettings::WHEEL_DAMAGE_WHEELCOLLIDER_ENABLED;
    ~WheelGlobals() {
        GameSettings::WHEEL_DAMAGE_IMPACTCOLLIDER_ENABLED = a;
        GameSettings::WHEEL_DAMAGE_WHEELCOLLIDER_ENABLED = b;
    }
};

}  // namespace

TEST_CASE("WP-29b derived: ModuleWheelBase quirks (SOIChange never applies, ActAutoFrictionToggle's inverted test, LandedDetectionNeedsUpdate table)") {
    if (!GameAvailable()) return;
    flighttest::FlightEnv env;
    Game15 g;
    SceneScope flight(GameScenes::FLIGHT);
    auto part = g.New("roverWheel1");
    auto* b = FindMod<ModuleWheelBase>(*part);
    REQUIRE(b != nullptr);
    // ModuleWheelBase.cs:815 `FromTo.from == base.vessel` compares a CelestialBody with a Vessel: the gee bias is never re-applied
    Vessel* v = env.MakeVessel("v", env.Body("Kerbin"), 700000.0, 0.0, true, false, OrbitDriver::UpdateMode::UPDATE, false);
    part->vessel = v;
    b->Set<float>("frictionMultiplier", 3.25f);
    b->SOIChange(v, env.Body("Kerbin"), env.Body("Mun"));
    CHECK(b->Get<float>("frictionMultiplier") == 3.25f);
    // ModuleWheelBase.cs:1065 `autoFriction = act.type != 0 || (act.type == Toggle && !autoFriction)`: Activate (0) turns it OFF, Deactivate and Toggle ON
    KSPActionParam act(0, KSPActionType::Activate), deact(0, KSPActionType::Deactivate), tog(0, KSPActionType::Toggle);
    b->Set<bool>("autoFriction", true);
    b->ActAutoFrictionToggle(act);
    CHECK_FALSE(b->Get<bool>("autoFriction"));
    b->ActAutoFrictionToggle(deact);
    CHECK(b->Get<bool>("autoFriction"));
    b->Set<bool>("autoFriction", false);
    b->ActAutoFrictionToggle(tog);
    CHECK(b->Get<bool>("autoFriction"));
    // ModuleWheelBase.cs:553 LandedDetectionNeedsUpdate
    Collider c1, c2;
    v->Landed = false;
    CHECK(b->LandedDetectionNeedsUpdate(nullptr, nullptr, nullptr, true));    // grounded on a vessel that is not landed
    v->Landed = true;
    CHECK_FALSE(b->LandedDetectionNeedsUpdate(nullptr, nullptr, nullptr, false));
    CHECK_FALSE(b->LandedDetectionNeedsUpdate(&c1, &c1, nullptr, true));
    CHECK(b->LandedDetectionNeedsUpdate(&c1, &c2, nullptr, true));            // another collider than last time
    CHECK(b->LandedDetectionNeedsUpdate(&c1, &c1, nullptr, false));           // a collider but not grounded
    CHECK(b->LandedDetectionNeedsUpdate(nullptr, nullptr, nullptr, true));    // grounded without a collider
    Vessel* other = env.MakeVessel("o", env.Body("Kerbin"), 700000.0, 0.0, true, false, OrbitDriver::UpdateMode::UPDATE, false);
    other->Landed = false;
    CHECK(b->LandedDetectionNeedsUpdate(&c1, &c1, other, true));              // the vessel it rests on is no longer landed
    other->Landed = true;
    CHECK_FALSE(b->LandedDetectionNeedsUpdate(&c1, &c1, other, true));
    part->vessel = nullptr;
}

TEST_CASE("WP-29b derived: ModuleWheelSubmodule.SetWheelBase resolves baseModuleIndex (ModuleWheelSubmodule.cs:24)") {
    if (!GameAvailable()) return;
    Game15 g;
    auto part = g.New("roverWheel1");
    auto* base = FindMod<ModuleWheelBase>(*part);
    auto* brakes = FindMod<ModuleWheelBrakes>(*part);
    REQUIRE(brakes != nullptr);
    CHECK(brakes->WheelBase() == base);   // baseModuleIndex = 0 in the cfg: Modules[0] as ModuleWheelBase
    // an index that is not a ModuleWheelBase logs an error and keeps the previous wheelBase
    LogCapture log;
    brakes->Set<int>("baseModuleIndex", part->Modules().IndexOf(brakes));
    brakes->SetWheelBase();
    CHECK(log.Has(log::Level::Error, "is not a ModuleWheelBase type"));
    CHECK(brakes->WheelBase() == base);
    // -1: GetComponent<ModuleWheelBase>()
    brakes->Set<int>("baseModuleIndex", -1);
    brakes->SetWheelBase();
    CHECK(brakes->WheelBase() == base);
}

TEST_CASE("WP-29b derived: persistent fields of every wheel module round-trip through Save / Load; Deployment.position and slaveModules") {
    if (!GameAvailable()) return;
    Game15 g;
    int modules = 0, fields = 0;
    for (const std::string& name : StockWheelParts()) {
        auto src = g.New(name.c_str());
        auto dst = g.New(name.c_str());
        // change every persistent value of the source (a recognisable value per type), then Save / Load into a fresh part
        for (size_t i = 0; i < src->Modules().Count(); i++) {
            PartModule* m = src->Modules()[i];
            if (dynamic_cast<ModuleWheelBase*>(m) == nullptr && dynamic_cast<ModuleWheelSubmodule*>(m) == nullptr) continue;
            BaseFieldList& fl = m->Fields();
            for (size_t k = 0; k < fl.Count(); k++) {
                BaseField& f = *fl[k];
                if (!f.isPersistant) continue;
                ModuleValue* mv = m->Value(f.name());
                if (auto* bv = std::get_if<bool>(mv)) *bv = !*bv;
                else if (auto* fv = std::get_if<float>(mv)) *fv = *fv + 0.5f;
                else if (auto* sv = std::get_if<std::string>(mv)) *sv = "x" + *sv;
                else if (auto* v3 = std::get_if<Vector3f>(mv)) *v3 = Vector3f(1.5f, 2.5f, 3.5f);
            }
        }
        for (size_t i = 0; i < src->Modules().Count(); i++) {
            PartModule* m = src->Modules()[i];
            if (dynamic_cast<ModuleWheelBase*>(m) == nullptr && dynamic_cast<ModuleWheelSubmodule*>(m) == nullptr) continue;
            ConfigNode node("MODULE");
            m->Save(node);
            PartModule* m2 = dst->Modules()[i];
            m2->Load(node);
            modules++;
            BaseFieldList& fl = m->Fields();
            for (size_t k = 0; k < fl.Count(); k++) {
                BaseField& f = *fl[k];
                if (!f.isPersistant) continue;
                const ModuleValue* a = m->Value(f.name());
                const ModuleValue* b2 = m2->Value(f.name());
                INFO(name << " " << m->ClassName() << "." << f.name());
                if (auto* x = std::get_if<bool>(a)) CHECK(*x == std::get<bool>(*b2));
                else if (auto* x2 = std::get_if<float>(a)) CHECK(SameBits(*x2, std::get<float>(*b2)));
                else if (auto* x3 = std::get_if<std::string>(a)) CHECK(*x3 == std::get<std::string>(*b2));
                else if (auto* x4 = std::get_if<Vector3f>(a)) CHECK(((x4->x == std::get<Vector3f>(*b2).x) && (x4->z == std::get<Vector3f>(*b2).z)));
                fields++;
            }
        }
    }
    CHECK(modules >= 55);
    CHECK(fields >= 150);
    // ModuleWheelDeployment.cs:153 OnLoad: position and the slave module indices (public non-field members; Instantiate copies them)
    auto p = g.New("GearMedium");
    auto* dep = FindMod<ModuleWheelDeployment>(*p);
    REQUIRE(dep != nullptr);
    ConfigNode n("MODULE");
    n.AddValue("name", "ModuleWheelDeployment");
    n.AddValue("position", "0.4");
    n.AddValue("slaveModules", "8;3");
    dep->OnLoad(n);
    CHECK(dep->position == std::strtof("0.4", nullptr));
    REQUIRE(dep->slaveModuleIndices.size() == 2);
    CHECK(dep->slaveModuleIndices[0] == 8);
    CHECK(dep->slaveModuleIndices[1] == 3);
    auto q = g.New("GearMedium");
    FindMod<ModuleWheelDeployment>(*q)->CopySerializedFrom(*dep);
    CHECK(FindMod<ModuleWheelDeployment>(*q)->position == dep->position);
    CHECK(FindMod<ModuleWheelDeployment>(*q)->slaveModuleIndices == dep->slaveModuleIndices);
    // OnSave writes the position only once the animation state exists (OnStart)
    ConfigNode out("MODULE");
    dep->OnSave(out);
    CHECK_FALSE(out.HasValue("position"));
}

TEST_CASE("WP-29b derived: torque adjusters chain in order (Motor, Brakes); ModuleWheelDamage impact enable table (ModuleWheelDamage.cs:105)") {
    if (!GameAvailable()) return;
    WheelGlobals restore;
    Game15 g;
    struct Scale : IWheelTorqueAdjuster {
        float k;
        explicit Scale(float kk) : k(kk) {}
        float ApplyTorqueAdjustment(float t) override { return t * k; }
    } a(2.0f), b(0.5f), c(3.0f);
    auto p = g.New("roverWheel1");
    auto* mo = FindMod<ModuleWheelMotor>(*p);
    auto* br = FindMod<ModuleWheelBrakes>(*p);
    auto* da = FindMod<ModuleWheelDamage>(*p);
    REQUIRE(mo != nullptr);
    REQUIRE(br != nullptr);
    REQUIRE(da != nullptr);
    mo->AddAdjuster(&a);
    mo->AddAdjuster(&c);
    br->AddAdjuster(&c);
    br->AddAdjuster(&b);
    CHECK(mo->ApplyTorqueAdjustments(1.5f) == 9.0f);
    CHECK(br->ApplyTorqueAdjustments(1.5f) == 2.25f);
    mo->RemoveAdjuster(&c);
    CHECK(mo->ApplyTorqueAdjustments(1.5f) == 3.0f);
    // impactDamageVelocity > 0: the impact collider counts when GameSettings enables it and the collider exists, else the wheel collider setting decides
    da->Set<float>("impactDamageVelocity", 5.0f);
    da->impactDamageColliderPresent = true;
    GameSettings::WHEEL_DAMAGE_IMPACTCOLLIDER_ENABLED = true;
    GameSettings::WHEEL_DAMAGE_WHEELCOLLIDER_ENABLED = false;
    CHECK(da->IsImpactDamageEnable());
    da->impactDamageColliderPresent = false;
    CHECK_FALSE(da->IsImpactDamageEnable());
    GameSettings::WHEEL_DAMAGE_WHEELCOLLIDER_ENABLED = true;
    CHECK(da->IsImpactDamageEnable());
    da->Set<float>("impactDamageVelocity", 0.0f);
    CHECK_FALSE(da->IsImpactDamageEnable());
    // ModuleWheelDamage implements IConstruction: a damaged wheel cannot be detached, offset or rotated
    da->Set<bool>("isDamaged", true);
    CHECK_FALSE(da->AsConstruction()->CanBeDetached());
    da->Set<bool>("isDamaged", false);
    CHECK(da->AsConstruction()->CanBeRotated());
}
