// WP-25 unit tests: ModuleDockingNode, ModuleGrappleNode, CModuleStrut and DockedVesselInfo.
// No golden or capture of docking exists (harness/golden/MANIFEST.json and harness/captures/out*/ have no docking table), so every expectation is
// derived: from the decompiled C# (the derivation is in the comment above the check), from the game's own data (the part prefabs of the stock
// docking ports, the stock .craft files), or from tools/wp25/dock_reference.py (an independent numpy float32 implementation of CheckDockContact,
// the acquire force chain and the nodeType matching; tests/data/wp25_reference.json, compared bit for bit = 0 ULP).
// The poses of the ports are hand-constructed (the model transforms are the engine's: DockingEnvironment::modelTransformPose); the GAME DATA
// (ranges, dots, nodeType, forces) comes from the part cfgs. The .craft files are the game's.
#include <cmath>
#include <cstring>
#include <fstream>
#include <map>
#include <set>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/celestial_system.hpp"
#include "ksp/celestial/flight_globals.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/modules/animation_modules.hpp"
#include "ksp/modules/module_decouplers.hpp"
#include "ksp/modules/module_docking_node.hpp"
#include "ksp/modules/module_grapple_node.hpp"
#include "ksp/modules/module_strut.hpp"
#include "ksp/modules/vessel_module.hpp"
#include "ksp/parts/compound_part.hpp"
#include "ksp/parts/docked_vessel_info.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/save/ship_construct.hpp"
#include "ksp/staging/stage_manager.hpp"
#include "ksp/vessel/vessel.hpp"
#include "ksp/vessel/vessel_assembly.hpp"
#include "nlohmann/json.hpp"
#include "editor_staging.hpp"
#include "resource_fixture.hpp"

using namespace ksp;
using namespace ksp::rtest;
using namespace ksp::support;
namespace fs = std::filesystem;

namespace {

const std::string kHarness25 = KSPCORE_HARNESS_DIR;

nlohmann::json LoadRef25() {
    std::ifstream f(std::string(KSPCORE_TEST_DATA_DIR) + "/wp25_reference.json", std::ios::binary);
    REQUIRE(f.good());
    return nlohmann::json::parse(f);
}

float FromBits(uint32_t u) {
    float f;
    std::memcpy(&f, &u, 4);
    return f;
}
uint32_t BitsOf(float f) {
    uint32_t u;
    std::memcpy(&u, &f, 4);
    return u;
}
Vector3f V3(const nlohmann::json& a, size_t o) { return Vector3f(FromBits(a[o]), FromBits(a[o + 1]), FromBits(a[o + 2])); }
// pose_bits layout of the reference: position, forward, up, right
DockPose PoseOf(const nlohmann::json& a) {
    DockPose p;
    p.position = V3(a, 0);
    p.forward = V3(a, 3);
    p.up = V3(a, 6);
    p.right = V3(a, 9);
    return p;
}

// Every hook a test installs is cleared at the end of the test
struct EnvGuard {
    EnvGuard() { Reset(); }
    ~EnvGuard() { Reset(); }
    static void Reset() {
        DockingEnvironment::modelTransformPose = nullptr;
        DockingEnvironment::partVelocity = nullptr;
        DockingEnvironment::addTorque = nullptr;
        DockingEnvironment::setNeutralControls = nullptr;
        DockingEnvironment::createSameVesselJoint = nullptr;
        DockingEnvironment::destroySameVesselJoint = nullptr;
        DockingEnvironment::attachJointOf = nullptr;
        DockingEnvironment::setTargetRotationLocal = nullptr;
        DockingEnvironment::limitAngularXMotion = nullptr;
        DockingEnvironment::setAngularXLimits = nullptr;
        DockingEnvironment::scaleBreakForces = nullptr;
        DockingEnvironment::rotationTransformEuler = nullptr;
        DockingEnvironment::setRotationTransformLocalRotation = nullptr;
        DockingEnvironment::screenMessages.clear();
        DockingEnvironment::sceneIsMissionBuilder = nullptr;
        DockingEnvironment::vesselTarget = nullptr;
        DockingEnvironment::setVesselTarget = nullptr;
        GrappleEnvironment::raycast = nullptr;
        GrappleEnvironment::createPivot = nullptr;
        GrappleEnvironment::createSameVesselJoint = nullptr;
        GrappleEnvironment::destroySameVesselJoint = nullptr;
        GrappleEnvironment::evaOnGrapple = nullptr;
        GrappleEnvironment::resumeVesselCtrlState = nullptr;
        GrappleEnvironment::minDistanceBackaway = nullptr;
        GrappleEnvironment::setNodeRotation = nullptr;
        StrutEnvironment::createJoint = nullptr;
        StrutEnvironment::destroyJoint = nullptr;
        StrutEnvironment::evaConstructionOpen = nullptr;
        ModuleDecouplerBase::addForce = nullptr;
        ModuleDecouplerBase::addForceAtPosition = nullptr;
        ModuleDecouplerBase::forceSetActiveVessel = nullptr;
    }
};

// The model transforms of the ports: one pose per part (the node transform and the control transform answer with it)
struct Poses {
    std::map<const Part*, DockPose> byPart;
    Poses() {
        DockingEnvironment::modelTransformPose = [this](Part& p, const std::string&) -> std::optional<DockPose> {
            auto it = byPart.find(&p);
            if (it == byPart.end()) return std::nullopt;
            return it->second;
        };
    }
};

ModuleDockingNode* DockOf(Part* p) { return dynamic_cast<ModuleDockingNode*>(p->FindModuleOfClass("ModuleDockingNode")); }
ModuleDockingNode* DockOf(const AvailablePart& ap) { return DockOf(ap.partPrefab.get()); }

// The stock game + PhysicsGlobals + Planetarium + the Kerbol system, as the flight scene has them (same set-up as test_staging.cpp)
struct World25 {
    GameData data;
    PhysicsGlobals pg;
    Planetarium planetarium;
    std::unique_ptr<CelestialSystem> sys;
    World25() {
        SaveEnvironment::Reset();
        FlightRegistry::Clear();
        Vessel::SpawnedVessels().clear();
        GameData::Options o;
        o.gameRoot = ksp::testing::GameRoot();
        data.Load(o);
        SaveEnvironment& env = SaveEnvironment::Current();
        env.getPartInfoByName = [this](std::string_view n) { return data.parts.getPartInfoByName(n); };
        env.partExists = [this](std::string_view n) { return data.parts.getPartInfoByName(n) != nullptr; };
        env.newGuidHashCode = [] {
            static uint32_t s = 4242u;
            s = s * 1664525u + 1013904223u;
            return static_cast<int32_t>(s);
        };
        support::InstallUpgradeHooks(data);
        pg.physicsDatabaseFilename = kHarness25 + "/inputs/Physics.cfg";
        pg.Awake("");
        REQUIRE(pg.LoadDatabase());
        planetarium.Awake();
        sys = CelestialSystem::Spawn(KerbolSystemPrefab(), planetarium);
        env.localBodies = sys->SpawnOrder();
        Vessel::env = VesselEnvironment{};
        VesselModuleManager::CompileModules();
        env.loadedScene = GameScenes::FLIGHT;
    }
    ~World25() {
        Vessel::SpawnedVessels().clear();
        FlightRegistry::Clear();
        sys.reset();
        Planetarium::Zup = Planetarium::AwakeZup(0.0);
        SaveEnvironment::Reset();
        Vessel::env = VesselEnvironment{};
    }
    CelestialBody& Kerbin() { return *sys->GetBodyByName("Kerbin"); }
    Vector3f PlaceKerbinAtPad() {
        CelestialBody& k = Kerbin();
        k.position = Vector3d();
        const Vector3d pad = k.GetWorldSurfacePosition(-0.0972077486779459, -74.5576813534722, 75.0);
        k.position = Vector3d() - pad;
        return Vector3f();
    }
    std::function<const AvailablePart*(const std::string&)> Lookup() {
        return [this](const std::string& n) { return data.parts.getPartInfoByName(n); };
    }
    // A vessel assembled from one of the game's own craft files; flightIDs made unique per `base`
    std::unique_ptr<Vessel> Craft(const std::string& path, uint32_t base) {
        ShipConstruct ship;
        std::string err;
        REQUIRE_MESSAGE(support::LoadCraftFile(path, ship, err), err);
        AssembleOptions ao;
        ao.rootPosition = PlaceKerbinAtPad();
        auto v = AssembleVesselFromCraft(ship, Lookup(), ao);
        REQUIRE(v != nullptr);
        for (size_t i = 0; i < v->parts.size(); i++) v->parts[i]->fields.flightID = base + static_cast<uint32_t>(i);
        v->loaded = true;
        if (std::find(FlightRegistry::VesselsLoaded.begin(), FlightRegistry::VesselsLoaded.end(), v.get()) == FlightRegistry::VesselsLoaded.end())
            FlightRegistry::AddVessel(v.get());
        // the launch assembly leaves the vessel packed; a docking scene has it unpacked and under physics
        v->packed = false;
        for (Part* p : v->parts) p->fields.packed = false;
        return v;
    }
    // A vertical stack of stock parts (each the STACK child of the previous), assembled like AssembleForLaunch (fromShipAssembly): test_vessel.cpp Stack14
    std::unique_ptr<Vessel> Stack(std::initializer_list<const char*> names, uint32_t base) {
        std::vector<std::unique_ptr<Part>> owned;
        uint32_t id = base;
        for (const char* n : names) {
            const AvailablePart* ap = data.parts.getPartInfoByName(n);
            REQUIRE_MESSAGE(ap != nullptr, n);
            auto p = Part::Instantiate(*ap->partPrefab);
            p->partInfo = ap;
            p->gameObjectName = ap->name;
            p->fields.flightID = id++;
            p->fields.persistentId = id * 7;
            p->UpdateMass();
            if (!owned.empty()) p->setParent(owned.back().get());
            owned.push_back(std::move(p));
        }
        auto v = std::make_unique<Vessel>();
        Part* root = owned.front().get();
        root->partTransform.position = PlaceKerbinAtPad();
        v->vesselTransform = root->partTransform;
        v->AdoptParts(std::move(owned), root);
        v->vesselName = "Test vessel";
        REQUIRE(v->Initialize(true, false, false, false));
        v->loaded = true;
        if (std::find(FlightRegistry::VesselsLoaded.begin(), FlightRegistry::VesselsLoaded.end(), v.get()) == FlightRegistry::VesselsLoaded.end()) FlightRegistry::AddVessel(v.get());
        v->packed = false;
        for (Part* p : v->parts) p->fields.packed = false;
        return v;
    }
    std::unique_ptr<Vessel> Ship(const char* name, uint32_t base) { return Craft(kHarness25 + "/inputs/Ships/VAB/" + name + ".craft", base); }
};

Part* FirstDock(Vessel& v, size_t skip = 0) {
    for (Part* p : v.parts)
        if (p->isDockingPort() && skip-- == 0) return p;
    return nullptr;
}

// A face-to-face pair of poses: port 1 at the origin looking +Z, port 2 `dist` ahead looking back (Ry(180): up unchanged, right flipped)
DockPose Facing1() { return DockPose{Vector3f(0.0f, 0.0f, 0.0f), Vector3f(0.0f, 0.0f, 1.0f), Vector3f(0.0f, 1.0f, 0.0f), Vector3f(1.0f, 0.0f, 0.0f)}; }
DockPose Facing2(float dist) { return DockPose{Vector3f(0.0f, 0.0f, dist), Vector3f(0.0f, 0.0f, -1.0f), Vector3f(0.0f, 1.0f, 0.0f), Vector3f(-1.0f, 0.0f, 0.0f)}; }

struct ForceLog {
    std::vector<std::pair<const Part*, Vector3f>> forces;
    std::vector<std::pair<const Part*, Vector3f>> forcesAt;
    std::vector<std::pair<const Part*, Vector3f>> torques;
    ForceLog() {
        ModuleDecouplerBase::addForce = [this](Part& p, const Vector3f& f) { forces.emplace_back(&p, f); };
        ModuleDecouplerBase::addForceAtPosition = [this](Part& p, const Vector3f& f, const Vector3f&) { forcesAt.emplace_back(&p, f); };
        DockingEnvironment::addTorque = [this](Part& p, const Vector3f& t) { torques.emplace_back(&p, t); };
    }
};

}  // namespace

// ============================================================================================== DockedVesselInfo

// DockedVesselInfo.cs:31 / :96. The field names (vesselName, rootUId, vesselType) are the C# ones; core/testdata/saves holds no DOCKEDVESSEL node
// (only the physprofile save), so the round trip is checked on the C# field list and on every VesselType the save pipeline can write.
TEST_CASE("WP-25 DockedVesselInfo: Save / Load round trip of every vessel type, the legacy numeric types") {
    for (int t = 0; t <= static_cast<int>(VesselType::DeployedSciencePart); t++) {
        DockedVesselInfo a;
        a.name = "Station Alpha " + std::to_string(t);
        a.rootPartUId = 2468013579u + static_cast<uint32_t>(t);
        a.vesselType = static_cast<VesselType>(t);
        ConfigNode n("DOCKEDVESSEL");
        a.Save(n);
        REQUIRE(n.HasValue("vesselName"));
        REQUIRE(n.HasValue("rootUId"));
        REQUIRE(n.HasValue("vesselType"));
        CHECK(*n.GetValue("vesselName") == a.name);
        CHECK(*n.GetValue("rootUId") == std::to_string(a.rootPartUId));
        DockedVesselInfo b;
        b.Load(n);
        CHECK(b.name == a.name);
        CHECK(b.rootPartUId == a.rootPartUId);
        CHECK(b.vesselType == a.vesselType);
    }
    // KSP 1.2 saves wrote the numeric VesselType (DockedVesselInfo.cs:6 OldVesselType)
    for (int t = 0; t <= static_cast<int>(DockedVesselInfo::OldVesselType::DeployedSciencePart); t++) {
        ConfigNode n("DOCKEDVESSEL");
        n.AddValue("vesselName", "Old");
        n.AddValue("rootUId", "7");
        n.AddValue("vesselType", std::to_string(t));
        DockedVesselInfo b;
        b.Load(n);
        CHECK(b.rootPartUId == 7u);
        if (t == static_cast<int>(DockedVesselInfo::OldVesselType::Station)) CHECK(b.vesselType == VesselType::Station);
        if (t == static_cast<int>(DockedVesselInfo::OldVesselType::EVA)) CHECK(b.vesselType == VesselType::EVA);
        if (t == static_cast<int>(DockedVesselInfo::OldVesselType::Ship)) CHECK(b.vesselType == VesselType::Ship);
    }
}

// ============================================================================================== OnLoad / OnSave / KSPFields of the stock ports

TEST_CASE("WP-25 ModuleDockingNode OnLoad / KSPFields of every stock docking port against the part database capture") {
    if (!GameAvailable()) return;
    EnvGuard env;
    Game15 g;
    const nlohmann::json ref = LoadRef25();
    size_t ports = 0;
    for (const auto& [name, port] : ref["ports"].items()) {
        const AvailablePart& ap = g.Info(name.c_str());
        REQUIRE(ap.partPrefab != nullptr);
        ModuleDockingNode* m = DockOf(ap);
        REQUIRE_MESSAGE(m != nullptr, name);
        ports++;
        CHECK(ap.partPrefab->isDockingPort());
        // nodeType.Split(',', ' '), RemoveEmptyEntries (ModuleDockingNode.cs:547)
        std::set<std::string> expected;
        std::string cur;
        for (char ch : port["nodeType"].get<std::string>() + " ") {
            if (ch == ',' || ch == ' ') {
                if (!cur.empty()) expected.insert(cur);
                cur.clear();
            } else {
                cur.push_back(ch);
            }
        }
        CHECK_MESSAGE(m->nodeTypes == expected, name);
        CHECK(m->Get<bool>("crossfeed") == port["crossfeed"].get<bool>());
        // `base.part.fuelCrossFeed = crossfeed` (OnLoad) and the two crossfeed events' visibility
        CHECK_MESSAGE(ap.partPrefab->fields.fuelCrossFeed == port["crossfeed"].get<bool>(), name);
        CHECK(m->Events()["EnableXFeed"]->active == !m->Get<bool>("crossfeed"));
        CHECK(m->Events()["DisableXFeed"]->active == m->Get<bool>("crossfeed"));
        // the module is registered in the part's docking port list (OnAwake: dockingPorts.AddUnique)
        CHECK(std::count(ap.partPrefab->dockingPorts.begin(), ap.partPrefab->dockingPorts.end(), m) == 1);
        // hardMinMaxLimits clamp (-86..0, 0..86)
        const Vector2f lim = m->Get<Vector2f>("hardMinMaxLimits");
        CHECK(lim.x >= -86.0f);
        CHECK(lim.x <= 0.0f);
        CHECK(lim.y >= 0.0f);
        CHECK(lim.y <= 86.0f);
        for (const char* k : {"acquireRange", "acquireMinFwdDot", "acquireMinRollDot", "captureRange", "captureMinFwdDot", "captureMinRollDot", "acquireForce",
                              "acquireTorque", "acquireTorqueRoll", "acquireForceTweak", "snapOffset"})
            CHECK_MESSAGE(BitsOf(m->Get<float>(k)) == port[k].get<uint32_t>(), (name + std::string(" ") + k));
        // IStageSeparator.GetStageIndex (:1944) and the staging overrides (:1949-1970): stageable and enabled only while `staged`
        CHECK(m->GetStageIndex(3) == 4);
        CHECK(m->IsStageable() == m->Get<bool>("staged"));
    }
    CHECK(ports == 6);
}

TEST_CASE("WP-25 ModuleDockingNode OnLoad / OnSave: state, dockUId, dockNodeIdx and the DOCKEDVESSEL node") {
    if (!GameAvailable()) return;
    EnvGuard env;
    Game15 g;
    auto part = g.New("dockingPort2");
    ModuleDockingNode* m = DockOf(part.get());
    REQUIRE(m != nullptr);
    ConfigNode node("MODULE");
    node.AddValue("name", "ModuleDockingNode");
    node.AddValue("state", "Docked (docker)");
    node.AddValue("dockUId", "3735928559");
    node.AddValue("dockNodeIdx", "2");
    ConfigNode* dv = node.AddNode("DOCKEDVESSEL");
    dv->AddValue("vesselName", "Mun Lander");
    dv->AddValue("rootUId", "1234");
    dv->AddValue("vesselType", "Lander");
    m->Load(node);
    CHECK(m->state == "Docked (docker)");
    CHECK(m->dockedPartUId == 3735928559u);
    CHECK(m->dockingNodeModuleIndex == 2);
    REQUIRE(m->vesselInfo.has_value());
    CHECK(m->vesselInfo->name == "Mun Lander");
    CHECK(m->vesselInfo->rootPartUId == 1234u);
    CHECK(m->vesselInfo->vesselType == VesselType::Lander);
    // OnSave: fsm == null -> "Ready" (ModuleDockingNode.cs:572), the ids, the DOCKEDVESSEL node written back
    ConfigNode out("MODULE");
    m->OnSave(out);
    CHECK(*out.GetValue("state") == "Ready");
    CHECK(*out.GetValue("dockUId") == "3735928559");
    CHECK(*out.GetValue("dockNodeIdx") == "2");
    REQUIRE(out.HasNode("DOCKEDVESSEL"));
    CHECK(*out.GetNode("DOCKEDVESSEL")->GetValue("vesselName") == "Mun Lander");
    CHECK(*out.GetNode("DOCKEDVESSEL")->GetValue("vesselType") == "Lander");
    // no DOCKEDVESSEL when there is no info
    m->vesselInfo.reset();
    ConfigNode out2("MODULE");
    m->OnSave(out2);
    CHECK_FALSE(out2.HasNode("DOCKEDVESSEL"));
}

// ============================================================================================== matching, contact, acquire against the reference

// ModuleDockingNode.cs:1257-1270: this approaches other when a nodeType is shared, the gender / snapRotation flags agree. Reference: dock_reference.py matches()
TEST_CASE("WP-25 docking port matching (nodeTypes, gender, snapRotation) of every pair of stock ports = tools/wp25 reference") {
    if (!GameAvailable()) return;
    EnvGuard env;
    Game15 g;
    const nlohmann::json ref = LoadRef25();
    size_t yes = 0;
    for (const auto& m : ref["matching"]) {
        ModuleDockingNode* a = DockOf(g.Info(m["a"].get<std::string>().c_str()));
        ModuleDockingNode* b = DockOf(g.Info(m["b"].get<std::string>().c_str()));
        const bool got = a->CanDockWith(*b);
        CHECK_MESSAGE(got == m["match"].get<bool>(), (m["a"].get<std::string>() + " -> " + m["b"].get<std::string>()));
        yes += got;
    }
    CHECK(yes == 18);
    // synthetic multi-type and gendered ports (the field values are set on a real module): a multi-type port matches when ANY type is shared;
    // gendered ports pair only with the opposite gender; a snapping port needs the same snapOffset
    ModuleDockingNode* p1 = DockOf(g.Info("dockingPort1"));
    ModuleDockingNode* p2 = DockOf(g.Info("dockingPort2"));
    REQUIRE(p1 != nullptr);
    REQUIRE(p2 != nullptr);
    const std::set<std::string> t1 = p1->nodeTypes, t2 = p2->nodeTypes;
    p1->nodeTypes = {"size0", "size1", "mine"};
    p2->nodeTypes = {"other", "mine"};
    CHECK(p1->CanDockWith(*p2));
    CHECK(p2->CanDockWith(*p1));
    p2->nodeTypes = {"other"};
    CHECK_FALSE(p1->CanDockWith(*p2));
    p2->nodeTypes = {"size1"};
    p1->Set<bool>("gendered", true);
    p2->Set<bool>("gendered", true);
    p1->Set<bool>("genderFemale", true);
    p2->Set<bool>("genderFemale", true);
    CHECK_FALSE(p1->CanDockWith(*p2));  // two females
    p2->Set<bool>("genderFemale", false);
    CHECK(p1->CanDockWith(*p2));
    p2->Set<bool>("gendered", false);   // a gendered port against a non-gendered one
    CHECK_FALSE(p1->CanDockWith(*p2));
    p2->Set<bool>("gendered", true);
    p1->Set<bool>("snapRotation", true);
    CHECK_FALSE(p1->CanDockWith(*p2));  // snapRotation differs
    p2->Set<bool>("snapRotation", true);
    p1->Set<float>("snapOffset", 90.0f);
    p2->Set<float>("snapOffset", 60.0f);
    CHECK_FALSE(p1->CanDockWith(*p2));
    p2->Set<float>("snapOffset", 90.0f);
    CHECK(p1->CanDockWith(*p2));
    p1->nodeTypes = t1;
    p2->nodeTypes = t2;
}

TEST_CASE("WP-25 CheckDockContact: capture / acquire geometry on hand-constructed poses = tools/wp25 reference (0 ULP)") {
    const nlohmann::json ref = LoadRef25();
    size_t trues = 0, n = 0, tiny = 0;
    for (const auto& c : ref["contact"]) {
        const DockPose p1 = PoseOf(c["pose1"]), p2 = PoseOf(c["pose2"]);
        float roll = -1.0f;
        const bool got = ModuleDockingNode::DockContact(p1, p2, c["snapRotation"].get<bool>(), FromBits(c["snapOffset"]), FromBits(c["minDist"]),
                                                        FromBits(c["minFwdDot"]), FromBits(c["minRollDot"]), &roll);
        const bool want = c["result"].get<bool>();
        CHECK_MESSAGE(got == want, (c["set"].get<std::string>() + " " + c["kind"].get<std::string>() + " #" + std::to_string(n)));
        // The roll dot is exact (0 ULP) except where the snapped angle lands on a quarter turn: cos(~pi/2) is ~1e-17 and its last digits are the
        // libm's (the module uses the x87 FCOS semantics of Mono through ksp::dotnet, numpy uses the C library): those are compared absolutely.
        const float wantRoll = FromBits(c["rollDot"].get<uint32_t>());
        if (wantRoll != 0.0f && std::fabs(wantRoll) < 1e-9f) {
            CHECK_MESSAGE(std::fabs(roll - wantRoll) < 1e-15f, ("rollDot~0 " + c["set"].get<std::string>() + " #" + std::to_string(n)));
            tiny++;
        } else {
            CHECK_MESSAGE(BitsOf(roll) == c["rollDot"].get<uint32_t>(), ("rollDot " + c["set"].get<std::string>() + " #" + std::to_string(n)));
        }
        trues += got;
        n++;
    }
    CHECK(n == ref["contact"].size());
    CHECK(trues == 560);
    CHECK(tiny == 8);  // the cos(~pi/2) cases of the generated reference (tests/data/wp25_reference.json: 8 of 1080 rollDot values with 0 < |v| < 1e-9)
}

TEST_CASE("WP-25 CheckDockContact through the module: the node transform seam and the method on real stock ports") {
    if (!GameAvailable()) return;
    EnvGuard env;
    Game15 g;
    Poses poses;
    Part* pa = g.Info("dockingPort1").partPrefab.get();
    Part* pb = g.Info("dockingPort2").partPrefab.get();
    ModuleDockingNode* a = DockOf(pa);
    ModuleDockingNode* b = DockOf(pb);
    poses.byPart[pa] = Facing1();
    poses.byPart[pb] = Facing2(0.04f);
    CHECK(a->CheckDockContact(a, b, a->Get<float>("captureRange"), a->Get<float>("captureMinFwdDot"), a->Get<float>("captureMinRollDot")));
    poses.byPart[pb] = Facing2(0.4f);  // 0.4 m: acquire range (0.5) but not capture range (0.06)
    CHECK_FALSE(a->CheckDockContact(a, b, a->Get<float>("captureRange"), a->Get<float>("captureMinFwdDot"), a->Get<float>("captureMinRollDot")));
    CHECK(a->CheckDockContact(a, b, a->Get<float>("acquireRange"), a->Get<float>("acquireMinFwdDot"), a->Get<float>("acquireMinRollDot")));
    // facing the same way: the forward dot test fails
    DockPose same = Facing2(0.04f);
    same.forward = Vector3f(0.0f, 0.0f, 1.0f);
    poses.byPart[pb] = same;
    CHECK_FALSE(a->CheckDockContact(a, b, 1.0f, 0.7f, -3.0e38f));
    // a node transform that does not exist: the C# access to a null Transform
    poses.byPart.erase(pb);
    CHECK_THROWS(a->CheckDockContact(a, b, 1.0f, 0.7f, -3.0e38f));
}

TEST_CASE("WP-25 FSM Acquire force and torque chain (Mono float) = tools/wp25 reference (0 ULP)") {
    if (!GameAvailable()) return;
    EnvGuard env;
    Game15 g;
    const nlohmann::json ref = LoadRef25();
    Poses poses;
    ForceLog log;
    Part* pa = g.Info("dockingPort1").partPrefab.get();
    Part* pb = g.Info("dockingPort2").partPrefab.get();
    ModuleDockingNode* a = DockOf(pa);
    ModuleDockingNode* b = DockOf(pb);
    a->SetupFSM();
    b->SetupFSM();
    a->otherNode = b;
    b->otherNode = a;
    size_t n = 0;
    for (const auto& c : ref["acquire"]) {
        for (const char* k : {"acquireForce", "acquireTorque", "acquireTorqueRoll", "acquireForceTweak"}) {
            a->Set<float>(k, FromBits(c["paramsA"][k]));
            b->Set<float>(k, FromBits(c["paramsB"][k]));
        }
        poses.byPart[pa] = PoseOf(c["pose1"]);
        poses.byPart[pb] = PoseOf(c["pose2"]);
        log.forcesAt.clear();
        log.torques.clear();
        a->st_acquire->OnFixedUpdate();  // st_acquire.OnFixedUpdate (ModuleDockingNode.cs:895)
        REQUIRE(log.forcesAt.size() == 2);
        REQUIRE(log.torques.size() == 4);
        CHECK(log.forcesAt[0].first == pa);
        CHECK(log.forcesAt[1].first == pb);
        auto same = [&](const Vector3f& v, const nlohmann::json& bits, const char* what) {
            CHECK_MESSAGE(BitsOf(v.x) == bits[0].get<uint32_t>(), (std::string(what) + ".x #" + std::to_string(n)));
            CHECK_MESSAGE(BitsOf(v.y) == bits[1].get<uint32_t>(), (std::string(what) + ".y #" + std::to_string(n)));
            CHECK_MESSAGE(BitsOf(v.z) == bits[2].get<uint32_t>(), (std::string(what) + ".z #" + std::to_string(n)));
        };
        same(log.forcesAt[0].second, c["forceA"], "forceA");
        same(log.forcesAt[1].second, c["forceB"], "forceB");
        same(log.torques[0].second, c["torqueA"], "torqueA");
        same(log.torques[1].second, c["torqueRollA"], "torqueRollA");
        same(log.torques[2].second, c["torqueB"], "torqueB");
        same(log.torques[3].second, c["torqueRollB"], "torqueRollB");
        n++;
    }
    CHECK(n == 15);
}

// ============================================================================================== the FSM and the tree operations on two vessels

TEST_CASE("WP-25 docking two vessels: Ready -> Acquire -> Docked, Part.Couple parent / child flip, DockedVesselInfo, crossfeed, Undock restores the vessels") {
    if (!GameAvailable()) return;
    EnvGuard env;
    World25 w;
    LogCapture quiet;
    Poses poses;
    ForceLog log;
    DockingEnvironment::setNeutralControls = [] {};
    auto va = w.Ship("GDLV3", 10000);
    auto vb = w.Ship("GDLV3", 20000);
    const size_t na = va->parts.size(), nb = vb->parts.size();
    REQUIRE(na == nb);
    const std::string nameA = va->vesselName, nameB = vb->vesselName;
    va->vesselName = "Vessel A";
    vb->vesselName = "Vessel B";
    va->vesselType = VesselType::Station;
    vb->vesselType = VesselType::Lander;
    Part* pa = FirstDock(*va);
    Part* pb = FirstDock(*vb);
    REQUIRE(pa != nullptr);
    REQUIRE(pb != nullptr);
    ModuleDockingNode* a = DockOf(pa);
    ModuleDockingNode* b = DockOf(pb);
    REQUIRE(a != nullptr);
    REQUIRE(b != nullptr);
    // port B starts 0.4 m in front of port A: inside the acquire range (0.5 m) of the port cfg
    poses.byPart[pa] = Facing1();
    poses.byPart[pb] = Facing2(0.4f);
    va->UpdateCaches();
    vb->UpdateCaches();
    REQUIRE(va->dockingPorts.size() == 1);
    REQUIRE(vb->dockingPorts.size() == 1);
    for (Part* p : va->parts) p->ModulesOnStart(StartState::Flying);
    for (Part* p : vb->parts) p->ModulesOnStart(StartState::Flying);
    CHECK(a->Events()["Undock"]->active == false);
    CHECK(a->Events()["MakePrimary"]->active == false);
    CHECK(pa->fields.physicalSignificance == 0);
    // lateFSMStart runs one frame after OnStart (the coroutine's `yield return null`); the FSM starts in the saved state (Ready)
    CHECK(a->fsm == nullptr);
    ModuleDockingNode::TickAll();
    REQUIRE(a->fsm != nullptr);
    REQUIRE(b->fsm != nullptr);
    CHECK(a->fsm->Started());
    CHECK(a->fsm->currentStateName == "Ready");
    CHECK(b->fsm->currentStateName == "Ready");

    // Ready.OnFixedUpdate: otherNode = FindNodeApproaches (the nodes match: both dockingPort3-like size1 ports, in range and facing)
    a->FixedUpdate();
    b->FixedUpdate();
    CHECK(a->otherNode == b);
    CHECK(b->otherNode == a);
    // on_nodeApproach (UPDATE): the dominant vessel's port goes to Acquire (dockee), the other to Acquire
    a->Update();
    b->Update();
    const bool aIsDominant = Vessel::GetDominantVessel(va.get(), vb.get()) == va.get();
    ModuleDockingNode* dockee = aIsDominant ? a : b;
    ModuleDockingNode* docker = aIsDominant ? b : a;
    CHECK(dockee->state == "Acquire (dockee)");
    CHECK(docker->state == "Acquire");
    CHECK(dockee->dockedPartUId == docker->PartOf()->fields.flightID);
    CHECK(dockee->dockingNodeModuleIndex == static_cast<int>(docker->PartOf()->Modules().IndexOf(docker)));
    // outside the capture range (0.06 m) the Acquire state only pulls: forces on both parts, nothing captured
    log.forcesAt.clear();
    docker->FixedUpdate();
    CHECK(log.forcesAt.size() == 2);
    docker->Update();
    CHECK(docker->state == "Acquire");

    // --- capture: port B 0.01 m ahead, facing, no relative velocity
    poses.byPart[pb] = Facing2(0.01f);
    Vessel* dockerVessel = docker->PartOf()->vessel;
    Vessel* dockeeVessel = dockee->PartOf()->vessel;
    const std::string dockerName = dockerVessel->vesselName, dockeeName = dockeeVessel->vesselName;
    const VesselType dockerType = dockerVessel->vesselType, dockeeType = dockeeVessel->vesselType;
    const uint32_t dockerRootUId = dockerVessel->rootPart->fields.flightID, dockeeRootUId = dockeeVessel->rootPart->fields.flightID;
    Part* dockerRoot = dockerVessel->rootPart;
    Part* dockerPart = docker->PartOf();
    Part* dockeePart = dockee->PartOf();
    // the tree before: each port hangs in its own vessel's tree; the docker's path to its own root is reversed by Couple
    CHECK(dockerPart->vessel != dockeePart->vessel);
    struct Trace {
        std::vector<std::string> log;
        void Docking(uint32_t a, uint32_t b) { log.push_back("docking:" + std::to_string(a) + ">" + std::to_string(b)); }
        void Complete(GameEvents::FromToAction<Part*, Part*> d) { log.push_back("complete:" + d.from->gameObjectName + ">" + d.to->gameObjectName); }
    } trace;
    GameEvents::onVesselDocking.Add(&trace, &Trace::Docking);
    GameEvents::onDockingComplete.Add(&trace, &Trace::Complete);
    docker->Update();  // st_acquire: on_capture (UPDATE) -> CheckDockContact(captureRange ...) and the relative velocity test
    GameEvents::onVesselDocking.Remove(&trace, &Trace::Docking);
    GameEvents::onDockingComplete.Remove(&trace, &Trace::Complete);
    CHECK(docker->state == "Docked (docker)");
    CHECK(dockee->fsm->currentStateName == "Docked (dockee)");
    // parent / child flip: the docker's port became a child of the dockee's port (DockToVessel: base.part.Couple(node.part)); the old root of the
    // docker's vessel is now a leaf (Couple -> SetHierarchyRoot(this))
    CHECK(dockerPart->parent == dockeePart);
    CHECK(std::count(dockeePart->children.begin(), dockeePart->children.end(), dockerPart) == 1);
    CHECK(dockerPart->vessel == dockeePart->vessel);
    CHECK(dockerRoot->parent != nullptr);
    Vessel* merged = dockeePart->vessel;
    CHECK(merged->parts.size() == na + nb);
    CHECK(merged->rootPart == dockeeVessel->rootPart);
    CHECK(trace.log.size() == 2);
    CHECK(trace.log[0].rfind("docking:", 0) == 0);
    CHECK(trace.log[1] == "complete:" + dockerPart->gameObjectName + ">" + dockeePart->gameObjectName);
    // DockedVesselInfo of both ports: the names, types and root part ids the two vessels had before the merge
    REQUIRE(docker->vesselInfo.has_value());
    REQUIRE(dockee->vesselInfo.has_value());
    CHECK(docker->vesselInfo->name == dockerName);
    CHECK(docker->vesselInfo->vesselType == dockerType);
    CHECK(docker->vesselInfo->rootPartUId == dockerRootUId);
    CHECK(dockee->vesselInfo->name == dockeeName);
    CHECK(dockee->vesselInfo->vesselType == dockeeType);
    CHECK(dockee->vesselInfo->rootPartUId == dockeeRootUId);
    // crossfeed: Docked (docker).OnEnter adds the two ports to each other's fuelLookupTargets
    CHECK(std::count(dockerPart->fuelLookupTargets.begin(), dockerPart->fuelLookupTargets.end(), dockeePart) == 1);
    CHECK(std::count(dockeePart->fuelLookupTargets.begin(), dockeePart->fuelLookupTargets.end(), dockerPart) == 1);
    // events: Undock on the docker, MakePrimary on the dockee
    CHECK(docker->Events()["Undock"]->active);
    CHECK(dockee->Events()["MakePrimary"]->active);
    // the ports are saved with their state and the DOCKEDVESSEL node, and read back
    ConfigNode saved("MODULE");
    docker->OnSave(saved);
    CHECK(*saved.GetValue("state") == "Docked (docker)");
    CHECK(*saved.GetValue("dockUId") == std::to_string(dockeePart->fields.flightID));
    REQUIRE(saved.HasNode("DOCKEDVESSEL"));
    DockedVesselInfo reread;
    reread.Load(*saved.GetNode("DOCKEDVESSEL"));
    CHECK(reread.name == dockerName);
    CHECK(reread.rootPartUId == dockerRootUId);

    // --- MakePrimary swaps the roles (SwapPrimary / SwapSecondary events)
    dockee->MakePrimary();
    CHECK(dockee->fsm->currentStateName == "Docked (docker)");
    CHECK(docker->fsm->currentStateName == "Docked (dockee)");
    docker->MakePrimary();
    CHECK(docker->fsm->currentStateName == "Docked (docker)");
    CHECK(dockee->fsm->currentStateName == "Docked (dockee)");

    // --- Undock (docker): Part.Undock(vesselInfo) restores the docker's vessel (name, type, root), the ejection forces push the parts apart
    log.forces.clear();
    docker->Update();  // Docked (docker).OnUpdate: Undock active unless a Missions adjuster blocks it
    REQUIRE(docker->Events()["Undock"]->active);
    docker->Undock();
    REQUIRE(Vessel::SpawnedVessels().size() >= 1);
    Vessel* restored = dockerPart->vessel;
    CHECK(restored != dockeePart->vessel);
    CHECK(restored->vesselName == dockerName);
    CHECK(restored->vesselType == dockerType);
    CHECK(restored->rootPart == dockerRoot);
    CHECK(restored->parts.size() == (aIsDominant ? nb : na));
    CHECK(dockeePart->vessel->parts.size() == (aIsDominant ? na : nb));
    CHECK(dockerPart->parent != dockeePart);
    // ejection: -undockEjectionForce*0.5 along the node forward on the docker's part, +on the parent (Mono float chain: (0f - f) * 0.5f)
    REQUIRE(log.forces.size() == 2);
    const float ej = docker->Get<float>("undockEjectionForce");
    CHECK(log.forces[0].first == dockerPart);
    CHECK(log.forces[1].first == dockeePart);
    const float fz = poses.byPart[dockerPart].forward.z;  // the node forward the force acts along
    CHECK(log.forces[0].second.z == -ej * 0.5f * fz);
    CHECK(log.forces[1].second.z == ej * 0.5f * fz);
    CHECK(log.forces[0].second.x == 0.0f);
    // both ports left the docked states: Disengage (the other node is still known), vesselInfo kept until the distance event
    CHECK(docker->fsm->currentStateName == "Disengage");
    CHECK(dockee->fsm->currentStateName == "Disengage");
    CHECK_FALSE(docker->Events()["Undock"]->active);
    // crossfeed targets removed on leaving Docked (docker)
    CHECK(dockerPart->fuelLookupTargets.empty());
    CHECK(dockeePart->fuelLookupTargets.empty());
    // Disengage -> Ready only when the ports are more than minDistanceToReEngage (1 m) apart (on_nodeDistance, FIXEDUPDATE)
    poses.byPart[pb] = Facing2(0.5f);
    docker->FixedUpdate();
    CHECK(docker->fsm->currentStateName == "Disengage");
    poses.byPart[pb] = Facing2(1.5f);
    docker->FixedUpdate();
    dockee->FixedUpdate();
    docker->Update();
    dockee->Update();
    CHECK(docker->state == "Ready");
    CHECK(dockee->state == "Ready");
    CHECK(docker->otherNode == nullptr);
    CHECK_FALSE(docker->vesselInfo.has_value());
    CHECK_FALSE(dockee->vesselInfo.has_value());
    (void)nameA;
    (void)nameB;
}

// A pair of ports of one vessel that can dock with each other and are not attached to each other in the craft
static bool FindFreePair(Vessel& v, Part*& p0, Part*& p1) {
    for (size_t i = 0; i < v.parts.size(); i++)
        for (size_t j = i + 1; j < v.parts.size(); j++) {
            ModuleDockingNode* a = DockOf(v.parts[i]);
            ModuleDockingNode* b = DockOf(v.parts[j]);
            if (a == nullptr || b == nullptr || !a->CanDockWith(*b)) continue;
            auto attached = [](ModuleDockingNode* m) { return m->referenceNode != nullptr && m->referenceNode->attachedPart != nullptr; };
            if (attached(a) || attached(b)) continue;
            p0 = v.parts[i];
            p1 = v.parts[j];
            return true;
        }
    return false;
}

TEST_CASE("WP-25 docking two ports of one vessel: same-vessel capture, the joint seam, UndockSameVessel, events") {
    if (!GameAvailable()) return;
    EnvGuard env;
    World25 w;
    LogCapture quiet;
    Poses poses;
    ForceLog log;
    // the first stock craft with two free, compatible ports
    std::unique_ptr<Vessel> v;
    Part *p0 = nullptr, *p1 = nullptr;
    for (const char* rel : {"VAB/Learstar A1", "SPH/Crater Crawler", "VAB/Kerbal 1-5", "VAB/Orbital Scanner"}) {
        v = w.Craft(kHarness25 + "/inputs/Ships/" + rel + ".craft", 30000);
        if (FindFreePair(*v, p0, p1)) break;
        FlightRegistry::Clear();
        v.reset();
    }
    REQUIRE(v != nullptr);
    REQUIRE(p0 != nullptr);
    ModuleDockingNode* a = DockOf(p0);
    ModuleDockingNode* b = DockOf(p1);
    poses.byPart[p0] = Facing1();
    poses.byPart[p1] = Facing2(0.3f);
    for (Part* p : v->parts)
        if (p->isDockingPort() && p != p0 && p != p1) poses.byPart[p] = DockPose{Vector3f(100.0f, 0.0f, 0.0f), Vector3f(1.0f, 0.0f, 0.0f), Vector3f(0.0f, 1.0f, 0.0f), Vector3f(0.0f, 0.0f, -1.0f)};
    int joints = 0, destroyed = 0;
    DockingEnvironment::createSameVesselJoint = [&](ModuleDockingNode&, ModuleDockingNode&) { joints++; return std::make_shared<int>(joints); };
    DockingEnvironment::destroySameVesselJoint = [&](ModuleDockingNode&, std::shared_ptr<void>&) { destroyed++; };
    struct Ev {
        int dock = 0, undock = 0;
        void D(GameEvents::FromToAction<ModuleDockingNode*, ModuleDockingNode*>) { dock++; }
        void U(GameEvents::FromToAction<ModuleDockingNode*, ModuleDockingNode*>) { undock++; }
    } ev;
    GameEvents::onSameVesselDock.Add(&ev, &Ev::D);
    GameEvents::onSameVesselUndock.Add(&ev, &Ev::U);
    v->UpdateCaches();
    for (Part* p : v->parts) p->ModulesOnStart(StartState::Flying);
    ModuleDockingNode::TickAll();
    REQUIRE(a->fsm != nullptr);
    REQUIRE(b->fsm != nullptr);
    CHECK(a->state == "Ready");
    CHECK(b->state == "Ready");
    a->FixedUpdate();
    b->FixedUpdate();
    CHECK(a->otherNode == b);
    CHECK(b->otherNode == a);
    // GetDominantNode: the node whose part sits LATER in vessel.parts (ModuleDockingNode.cs:1685)
    ModuleDockingNode* later = a->GetDominantNode(a, b);
    const auto idx = [&](Part* p) { return std::find(v->parts.begin(), v->parts.end(), p) - v->parts.begin(); };
    CHECK(idx(later->PartOf()) == std::max(idx(p0), idx(p1)));
    a->Update();
    b->Update();
    // same vessel: the dominant node goes straight to Docked (dockee), the other to Acquire (on_nodeApproach, ModuleDockingNode.cs:991)
    ModuleDockingNode* dockee = later;
    ModuleDockingNode* other = later == a ? b : a;
    CHECK(dockee->state == "Docked (dockee)");
    CHECK(other->state == "Acquire");
    poses.byPart[p1] = Facing2(0.01f);
    other->Update();  // on_capture: same vessel: CheckDockContact only
    CHECK(other->state == "Docked (same vessel)");
    CHECK(dockee->fsm->currentStateName == "Docked (dockee)");
    CHECK(joints == 1);  // DockToSameVessel from the state's OnEnter (not packed)
    CHECK(other->sameVesselDockJoint != nullptr);
    CHECK(ev.dock == 1);
    CHECK(other->Events()["UndockSameVessel"]->active);
    // UndockSameVessel: the joint is destroyed when the state is left, the undock event fires, both go to Disengage
    other->UndockSameVessel();
    CHECK(other->fsm->currentStateName == "Disengage");
    CHECK(dockee->fsm->currentStateName == "Disengage");
    CHECK(destroyed == 1);
    CHECK(other->sameVesselDockJoint == nullptr);
    CHECK(ev.undock == 1);
    GameEvents::onSameVesselDock.Remove(&ev, &Ev::D);
    GameEvents::onSameVesselUndock.Remove(&ev, &Ev::U);
}

// Kerbal 1-5's first docking port has the craft's battery on its reference node (`top`): referenceNode.attachedPart != null starts the FSM in PreAttached
// (lateFSMStart), the Undock event is active, and Undock decouples that part (ModuleDockingNode.cs:1493 undockPreAttached -> Decouple -> Part.decouple),
// then on_undock sends the port to Ready (no other node).
TEST_CASE("WP-25 pre-attached docking port (craft file): PreAttached, Undock = Decouple splits the vessel") {
    if (!GameAvailable()) return;
    EnvGuard env;
    World25 w;
    LogCapture quiet;
    Poses poses;
    ForceLog log;
    auto v = w.Ship("Kerbal 1-5", 60000);
    Part* p0 = FirstDock(*v, 0);
    REQUIRE(p0 != nullptr);
    ModuleDockingNode* a = DockOf(p0);
    REQUIRE(a->referenceNode != nullptr);
    REQUIRE(a->referenceNode->attachedPart != nullptr);
    Part* held = a->referenceNode->attachedPart;
    poses.byPart[p0] = Facing1();
    for (Part* p : v->parts)
        if (p->isDockingPort() && p != p0) poses.byPart[p] = Facing2(50.0f);
    const size_t n = v->parts.size();
    v->UpdateCaches();
    for (Part* p : v->parts) p->ModulesOnStart(StartState::Flying);
    ModuleDockingNode::TickAll();
    CHECK(a->fsm->currentStateName == "PreAttached");
    CHECK(a->otherNode == nullptr);  // the held part is not a docking port
    CHECK(a->Events()["Undock"]->active);
    a->Update();
    CHECK(a->state == "PreAttached");
    a->Undock();
    REQUIRE(Vessel::SpawnedVessels().size() == 1);
    CHECK(p0->vessel != held->vessel);
    CHECK(p0->vessel->parts.size() + held->vessel->parts.size() == n);
    CHECK(a->fsm->currentStateName == "Ready");
    CHECK_FALSE(a->Events()["Undock"]->active);
    REQUIRE(log.forces.size() == 2);
    CHECK(log.forces[0].first == p0);
    CHECK(log.forces[1].first == held);
}

TEST_CASE("WP-25 ModuleDockingNode crossfeed events, staging (Decouple on activation) and the lock / rotation maths") {
    if (!GameAvailable()) return;
    EnvGuard env;
    Game15 g;
    Part* pa = g.Info("dockingPort2").partPrefab.get();
    ModuleDockingNode* m = DockOf(pa);
    // EnableXFeed / DisableXFeed: the part flag, the module field, the two event flags and onPartCrossfeedStateChange only on a change
    struct Cf {
        int n = 0;
        void On(Part*) { n++; }
    } cf;
    GameEvents::onPartCrossfeedStateChange.Add(&cf, &Cf::On);
    m->DisableXFeed();
    CHECK_FALSE(m->Get<bool>("crossfeed"));
    CHECK_FALSE(pa->fields.fuelCrossFeed);
    CHECK(m->Events()["EnableXFeed"]->active);
    CHECK_FALSE(m->Events()["DisableXFeed"]->active);
    CHECK(cf.n == 1);
    m->DisableXFeed();
    CHECK(cf.n == 1);  // no change, no event
    KSPActionParam param;
    m->ToggleXFeedAction(param);
    CHECK(m->Get<bool>("crossfeed"));
    CHECK(pa->fields.fuelCrossFeed);
    CHECK(cf.n == 2);
    m->ToggleXFeedAction(param);
    CHECK_FALSE(m->Get<bool>("crossfeed"));
    m->EnableXFeedAction(param);
    CHECK(m->Get<bool>("crossfeed"));
    m->DisableXFeedAction(param);
    CHECK_FALSE(m->Get<bool>("crossfeed"));
    CHECK(cf.n == 5);
    m->EnableXFeed();
    GameEvents::onPartCrossfeedStateChange.Remove(&cf, &Cf::On);
    CHECK(m->CrossfeedToggleableEditor());
    CHECK(m->CrossfeedToggleableFlight());
    CHECK(m->GetContractObjectiveType() == "Dock");
    // GetRotationAxis (ModuleDockingNode.cs:1639) / SetTargetRotation (:1624): AngleAxis(angle, axis), starting rotation first
    const char* axes[] = {"Z", "Z-", "Y", "Y-", "X", "X-", "bogus"};
    const Vector3f expect[] = {{0, 0, 1}, {0, 0, -1}, {0, 1, 0}, {0, -1, 0}, {1, 0, 0}, {-1, 0, 0}, {0, 0, 1}};
    for (size_t i = 0; i < 7; i++) {
        m->Set<std::string>("rotationAxis", axes[i]);
        const Vector3f ax = m->GetRotationAxis();
        CHECK(ax.x == expect[i].x);
        CHECK(ax.y == expect[i].y);
        CHECK(ax.z == expect[i].z);
    }
    m->Set<std::string>("rotationAxis", "Y");
    const Quaternionf q = m->SetTargetRotation(Quaternionf::identity(), 90.0f, true);
    CHECK(q.y == doctest::Approx(0.70710678f).epsilon(1e-6));
    CHECK(q.w == doctest::Approx(0.70710678f).epsilon(1e-6));
    // JointTargetAngle: targetAngle, inverted, and the other node's (ModuleDockingNode.cs:300)
    ModuleDockingNode* o = DockOf(g.Info("dockingPort1"));
    m->Set<float>("targetAngle", 30.0f);
    CHECK(m->JointTargetAngle() == 30.0f);
    m->Set<bool>("inverted", true);
    CHECK(m->JointTargetAngle() == -30.0f);
    m->otherNode = o;
    o->Set<float>("targetAngle", 10.0f);
    CHECK(m->JointTargetAngle() == -20.0f);
    o->Set<bool>("inverted", true);
    CHECK(m->JointTargetAngle() == -40.0f);
    m->otherNode = nullptr;
    m->Set<bool>("inverted", false);
    // the lock: EngageNodeLock refuses while rotating, DisengageNodeLock
    m->Fields()["targetAngle"];
    m->Set<bool>("nodeIsLocked", false);
    m->IsRotating = true;
    CHECK_FALSE(m->EngageNodeLock());
    m->IsRotating = false;
    SaveEnvironment::Current().loadedScene = GameScenes::MAINMENU;
    CHECK(m->EngageNodeLock());
    CHECK(m->Get<bool>("nodeIsLocked"));
    CHECK_FALSE(m->Fields()["targetAngle"]->guiInteractable);
    m->DisengageNodeLock();
    CHECK_FALSE(m->Get<bool>("nodeIsLocked"));
    CHECK(m->Fields()["targetAngle"]->guiInteractable);
}

// ============================================================================================== struts

namespace {
bool LoadShipWithParts(const fs::path& file, ShipConstruct& ship) {
    ConfigNode::NodePtr root = ConfigNode::Load(file.string());
    if (!root) return false;
    const std::string* sn = root->GetValuePtr("ship");
    ConfigNode::NodePtr upgraded;
    KSPUpgradePipeline::Process(root, sn ? *sn : std::string(), SaveUpgradePipeline::LoadContext::Craft, [&](const ConfigNode::NodePtr& n) { upgraded = n; },
                                [&](KSPUpgradePipeline::UpgradeFailOption, const ConfigNode::NodePtr&) {});
    if (!upgraded) return false;
    std::string err;
    return ship.LoadShipWithParts(*upgraded, 0, true, err);
}
}  // namespace

// CModuleStrut.cs:26: the strut is a CompoundPart; its target is resolved by CompoundPart.SetTarget (the craft's `tgt` craftID), then OnTargetSet builds the "Strut"
// AttachNode between the rigidbody owners of both ends. Every stock craft that carries struts.
TEST_CASE("WP-25 CModuleStrut target resolution over every stock craft with struts: rigidbody owners, the Strut AttachNode, same-body struts") {
    if (!GameAvailable()) return;
    EnvGuard env;
    Game15 g;
    LogCapture quiet;
    size_t struts = 0, joints = 0, sameBody = 0, crafts = 0, unresolved = 0;
    std::string unresolvedIn;
    std::vector<fs::path> files;
    for (const char* dir : {"VAB", "SPH"}) {
        const fs::path d = fs::path(KSPCORE_HARNESS_DIR) / "inputs" / "Ships" / dir;
        for (const auto& e : fs::directory_iterator(d))
            if (e.path().extension() == ".craft") files.push_back(e.path());
    }
    std::sort(files.begin(), files.end());
    int createdJoints = 0;
    StrutEnvironment::createJoint = [&](Part&, Part&, AttachNode&) {
        createdJoints++;
        return std::make_shared<int>(createdJoints);
    };
    for (const fs::path& f : files) {
        ShipConstruct ship;
        if (!LoadShipWithParts(f, ship)) continue;
        bool any = false;
        for (Part* p : ship.liveParts)
            if (p->FindModuleOfClass("CModuleStrut") != nullptr) any = true;
        if (!any) continue;
        crafts++;
        // the editor start: CompoundPart.onStartComplete resolves the targets through EditorLogic.fetch.ship.parts
        {
            SceneScope sc(GameScenes::EDITOR);
            CompoundPart::hooks.editorShipParts = [&ship]() { return ship.liveParts; };
            for (Part* p : ship.liveParts)
                if (auto* cp = dynamic_cast<CompoundPart*>(p)) cp->OnStartComplete();
            CompoundPart::hooks.editorShipParts = nullptr;
        }
        // flight: every part is a rigidbody owner except physics-less ones; give the tree its rigidbodies, then call OnTargetSet like SetTarget does
        SceneScope flight(GameScenes::FLIGHT);
        for (Part* p : ship.liveParts) p->hasRigidbody = p->fields.physicalSignificance == 0;
        for (Part* p : ship.liveParts) {
            auto* cp = dynamic_cast<CompoundPart*>(p);
            auto* strut = cp ? dynamic_cast<CModuleStrut*>(cp->FindModuleOfClass("CModuleStrut")) : nullptr;
            if (strut == nullptr) continue;
            struts++;
            if (cp->target == nullptr) {  // the craft's `tgt` did not resolve (the target part is not in the file's part list)
                unresolved++;
                unresolvedIn += f.filename().string() + " ";
                continue;
            }
            CHECK(cp->attachState() == CompoundPart::AttachState::Attached);
            strut->strutJoint = nullptr;
            strut->OnTargetSet(cp->target);
            Part* root = CModuleStrut::Rigidbody(cp);
            Part* tgt = CModuleStrut::Rigidbody(cp->target);
            if (root == nullptr) continue;  // `part.Rigidbody == null`: the strut hangs on nothing -> no joint (C# returns early)
            CHECK(strut->jointRoot == root);
            CHECK(strut->jointTarget == tgt);
            if (root != tgt) {
                joints++;
                REQUIRE(strut->TargetNode() != nullptr);
                const AttachNode& n = *strut->TargetNode();
                CHECK(n.id == "Strut");
                CHECK(n.attachedPart == tgt);
                CHECK(n.nodeType == AttachNodeType::Surface);
                CHECK(n.attachMethod == AttachNodeMethod::FIXED_JOINT);
                CHECK(n.breakingForce == strut->Get<float>("linearStrength"));
                CHECK(n.breakingTorque == strut->Get<float>("angularStrength"));
                CHECK(n.size == 1);
                CHECK_FALSE(n.ResourceXFeed);
                CHECK(n.owner == root);
                // position / orientation in the target's frame: InverseTransformPoint(TransformPoint(targetPosition)) of the C#
                const Vector3f world = cp->partTransform.TransformPoint(cp->compoundFields.targetPosition);
                const Vector3f pos = tgt->partTransform.InverseTransformPoint(world);
                CHECK(BitsOf(n.position.x) == BitsOf(pos.x));
                CHECK(BitsOf(n.position.y) == BitsOf(pos.y));
                CHECK(BitsOf(n.position.z) == BitsOf(pos.z));
                CHECK(strut->strutJoint != nullptr);
                // OnTargetLost destroys the joint and clears the node's owner
                int before = 0;
                StrutEnvironment::destroyJoint = [&](std::shared_ptr<void>&) { before++; };
                strut->OnTargetLost();
                CHECK(before == 1);
                CHECK(strut->TargetNode()->owner == nullptr);
                StrutEnvironment::destroyJoint = nullptr;
            } else {
                sameBody++;
                CHECK(strut->strutJoint == nullptr);
                CHECK(strut->TargetNode() == nullptr);
            }
            // GetInfo (CModuleStrut.cs:77): the localised line carries the strengths
            CHECK_FALSE(strut->GetInfo().empty());
        }
        // one rigid body: only the root keeps its rigidbody, so both ends of every strut resolve to the root (CModuleStrut.cs:48: no joint, a warning)
        for (Part* p : ship.liveParts) p->hasRigidbody = p == ship.liveParts[0];
        for (Part* p : ship.liveParts) {
            auto* cp = dynamic_cast<CompoundPart*>(p);
            auto* strut = cp ? dynamic_cast<CModuleStrut*>(cp->FindModuleOfClass("CModuleStrut")) : nullptr;
            if (strut == nullptr || cp->target == nullptr) continue;
            strut->OnTargetSet(cp->target);
            CHECK(strut->jointRoot == ship.liveParts[0]);
            CHECK(strut->jointTarget == ship.liveParts[0]);
            CHECK(strut->strutJoint == nullptr);
            CHECK(strut->TargetNode() == nullptr);
            sameBody++;
        }
    }
    CHECK(crafts >= 8);
    CHECK(struts >= 20);
    CHECK(joints + unresolved == struts);
    CHECK(sameBody == struts - unresolved);
    CHECK(joints >= 1);
    CHECK(CModuleStrut::Rigidbody(nullptr) == nullptr);
}

TEST_CASE("WP-25 CModuleStrut KSPFields of the stock strut part") {
    if (!GameAvailable()) return;
    EnvGuard env;
    Game15 g;
    const AvailablePart& ap = g.Info("strutConnector");
    auto* m = dynamic_cast<CModuleStrut*>(ap.partPrefab->FindModuleOfClass("CModuleStrut"));
    REQUIRE(m != nullptr);
    CHECK(m->Get<float>("linearStrength") > 0.0f);
    CHECK(m->Get<float>("angularStrength") > 0.0f);
    CHECK(m->GetModuleTitle() == "Strut Connector");
    CHECK(m->GetPrimaryField().empty());
}

// ============================================================================================== grapple

TEST_CASE("WP-25 ModuleGrappleNode OnLoad / OnSave of the stock claws, CheckGrappleContact, pivot mode events") {
    if (!GameAvailable()) return;
    EnvGuard env;
    Game15 g;
    for (const char* name : {"GrapplingDevice", "smallClaw"}) {
        const AvailablePart& ap = g.Info(name);
        auto* m = dynamic_cast<ModuleGrappleNode*>(ap.partPrefab->FindModuleOfClass("ModuleGrappleNode"));
        REQUIRE_MESSAGE(m != nullptr, name);
        CHECK(m->Get<float>("captureRange") > 0.0f);
        CHECK(m->Get<float>("pivotRange") > 0.0f);
        CHECK(m->GetContractObjectiveType() == "Grapple");
    }
    auto part = g.New("GrapplingDevice");
    auto* m = dynamic_cast<ModuleGrappleNode*>(part->FindModuleOfClass("ModuleGrappleNode"));
    REQUIRE(m != nullptr);
    SaveEnvironment::Current().loadedScene = GameScenes::FLIGHT;
    ConfigNode node("MODULE");
    node.AddValue("name", "ModuleGrappleNode");
    node.AddValue("state", "Grappled");
    node.AddValue("dockUId", "98765");
    node.AddValue("grapplePos", "0.5,1.5,-2.5");  // KSPUtil.WriteVector format
    node.AddValue("grappleOrt", "0,1,0");
    node.AddValue("grappleOrt2", "0,0,-1");
    node.AddValue("grappledSameVessel", "True");
    ConfigNode* a = node.AddNode("DOCKEDVESSEL");
    a->AddValue("vesselName", "Claw Ship");
    a->AddValue("rootUId", "11");
    a->AddValue("vesselType", "Ship");
    ConfigNode* b = node.AddNode("DOCKEDVESSEL_Other");
    b->AddValue("vesselName", "Asteroid XYZ");
    b->AddValue("rootUId", "22");
    b->AddValue("vesselType", "SpaceObject");
    m->Load(node);
    CHECK(m->state == "Grappled");
    CHECK(m->dockedPartUId == 98765u);
    CHECK(m->grabPos.x == 0.5f);
    CHECK(m->grabPos.y == 1.5f);
    CHECK(m->grabPos.z == -2.5f);
    CHECK(m->grappledSameVessel);
    REQUIRE(m->vesselInfo.has_value());
    REQUIRE(m->otherVesselInfo.has_value());
    CHECK(m->vesselInfo->name == "Claw Ship");
    CHECK(m->otherVesselInfo->name == "Asteroid XYZ");
    CHECK(m->otherVesselInfo->vesselType == VesselType::SpaceObject);
    CHECK(m->HasGrappleNode());  // OnLoad in flight: new AttachNode { size = 1, id = "grapple" }
    CHECK(m->GrappleNode()->id == "grapple");
    CHECK(m->GrappleNode()->size == 1);
    m->GrappleNode()->position = Vector3f(1.0f, 2.0f, 3.0f);
    ConfigNode out("MODULE");
    m->OnSave(out);
    CHECK(*out.GetValue("state") == "Ready");  // fsm == null
    CHECK(*out.GetValue("dockUId") == "98765");
    CHECK(out.HasValue("grapplePos"));
    CHECK(KSPUtil::ParseVector3(*out.GetValue("grapplePos")).z == 3.0f);
    CHECK(out.HasNode("DOCKEDVESSEL"));
    CHECK(out.HasNode("DOCKEDVESSEL_Other"));
    CHECK(*out.GetNode("DOCKEDVESSEL_Other")->GetValue("vesselName") == "Asteroid XYZ");

    // CheckGrappleContact (ModuleGrappleNode.cs:543): the hit point inside captureRange and the node facing the surface normal
    GrappleEnvironment::Hit hit;
    hit.point = Vector3f(0.0f, 0.0f, 0.05f);
    hit.normal = Vector3f(0.0f, 0.0f, -1.0f);
    DockPose node0 = Facing1();
    CHECK(m->CheckGrappleContact(node0, hit, 0.06f, 0.998f));
    hit.point = Vector3f(0.0f, 0.0f, 0.07f);
    CHECK_FALSE(m->CheckGrappleContact(node0, hit, 0.06f, 0.998f));
    hit.point = Vector3f(0.0f, 0.0f, 0.05f);
    hit.normal = Vector3f(0.0f, 0.6f, -0.8f);
    CHECK_FALSE(m->CheckGrappleContact(node0, hit, 0.06f, 0.998f));
    // pivot mode events (OnJointInit / OnDriveModeChanged), IsLoose / IsJointUnlocked without a pivot
    CHECK(m->IsLoose());
    CHECK_FALSE(m->IsJointUnlocked());
    m->OnDriveModeChanged(ActiveJointDriveMode::Neutral);
    CHECK_FALSE(m->Events()["SetLoose"]->active);
    CHECK(m->Events()["LockPivot"]->active);
    m->OnDriveModeChanged(ActiveJointDriveMode::Park);
    CHECK(m->Events()["SetLoose"]->active);
    CHECK_FALSE(m->Events()["LockPivot"]->active);
    m->OnDriveModeChanged(ActiveJointDriveMode::NoJoint);
    CHECK_FALSE(m->Events()["SetLoose"]->active);
    CHECK_FALSE(m->Events()["LockPivot"]->active);
}

namespace {
struct FakePivot : IGrapplePivot {
    ActiveJointDriveMode mode = ActiveJointDriveMode::Park;
    float limit = 0.0f;
    bool terminated = false;
    bool IsValid() override { return !terminated; }
    ActiveJointDriveMode DriveMode() override { return mode; }
    void SetDriveMode(ActiveJointDriveMode m) override { mode = m; }
    void SetPivotAngleLimit(float d) override { limit = d; }
    void Terminate() override { terminated = true; }
    Quaternionf GetLocalPivotRotation() override { return Quaternionf::identity(); }
};
}  // namespace

// MEV_Daring.craft carries a claw (ModuleGrappleNode). The claw grabs a second vessel (GDLV3): the FSM Ready -> Grappled, Part.Couple of the two trees, both
// DockedVesselInfo, then Release restores the grabbed vessel by name / type / root (Part.Undock(otherVesselInfo)).
TEST_CASE("WP-25 ModuleGrappleNode: grab a second vessel (Ready -> Grappled, Couple), pivot, Release restores both vessels") {
    if (!GameAvailable()) return;
    EnvGuard env;
    World25 w;
    LogCapture quiet;
    Poses poses;
    ForceLog log;
    DockingEnvironment::setNeutralControls = [] {};
    // a tank carrying the claw (stock parts, the claw's `top` node on the tank's `bottom`), and a second two-tank vessel to grab
    auto vc = w.Stack({"fuelTank", "GrapplingDevice"}, 40000);
    auto vt = w.Stack({"fuelTank", "fuelTank"}, 50000);
    vc->vesselName = "Claw Vessel";
    vc->vesselType = VesselType::Ship;
    vt->vesselName = "Target Rock";
    vt->vesselType = VesselType::SpaceObject;
    Part* claw = nullptr;
    for (Part* p : vc->parts)
        if (p->FindModuleOfClass("ModuleGrappleNode") != nullptr) claw = p;
    REQUIRE(claw != nullptr);
    auto* m = dynamic_cast<ModuleGrappleNode*>(claw->FindModuleOfClass("ModuleGrappleNode"));
    REQUIRE(m != nullptr);
    {
        // the hand-built stack skips the craft loader: PartModule.Load -> OnLoad creates the grapple AttachNode in flight
        ConfigNode empty("MODULE");
        m->OnLoad(empty);
    }
    Part* targetPart = vt->parts.back();
    const size_t nc = vc->parts.size(), nt = vt->parts.size();
    poses.byPart[claw] = Facing1();
    auto pivot = std::make_shared<FakePivot>();
    GrappleEnvironment::createPivot = [&](ModuleGrappleNode&, AttachNode*) { return std::static_pointer_cast<IGrapplePivot>(pivot); };
    GrappleEnvironment::raycast = [&](ModuleGrappleNode&, const Vector3f&, const Vector3f&, float) -> std::optional<GrappleEnvironment::Hit> {
        GrappleEnvironment::Hit h;
        h.part = targetPart;
        h.point = Vector3f(0.0f, 0.0f, 0.03f);
        h.normal = Vector3f(0.0f, 0.0f, -1.0f);
        return h;
    };
    for (Part* p : vc->parts) p->ModulesOnStart(StartState::Flying);
    ModuleGrappleNode::TickAll();
    REQUIRE(m->fsm != nullptr);
    CHECK(m->fsm->currentStateName == "Ready");
    CHECK_FALSE(m->Events()["Release"]->active);
    // the claw's arms are closed (the deploy animation is at 0): on_disable (UPDATE) -> Disabled; fully deployed -> on_enable -> Ready
    REQUIRE(m->deployAnimator != nullptr);
    m->Update();
    CHECK(m->state == "Disabled");
    CHECK_FALSE(m->Events()["Release"]->active);
    m->deployAnimator->Set<float>("animTime", 1.0f);
    m->Update();
    CHECK(m->state == "Ready");
    // Ready.OnFixedUpdate: FindContactParts (the ray hit the other vessel's part); on_contact (FIXEDUPDATE): within captureRange, facing
    m->FixedUpdate();
    CHECK(m->otherPart == targetPart);
    m->FixedUpdate();  // the event is checked inside updateFSM (FIXEDUPDATE) after the state's OnFixedUpdate
    m->Update();
    CHECK(m->state == "Grappled");
    CHECK(m->dockedPartUId == targetPart->fields.flightID);
    REQUIRE(m->vesselInfo.has_value());
    REQUIRE(m->otherVesselInfo.has_value());
    CHECK(m->vesselInfo->name == "Claw Vessel");
    CHECK(m->otherVesselInfo->name == "Target Rock");
    CHECK(m->otherVesselInfo->vesselType == VesselType::SpaceObject);
    CHECK(m->otherVesselInfo->rootPartUId == 50000u);
    // Part.Couple merged the two vessels; the grapple node joined the owner's attach nodes
    CHECK(claw->vessel == targetPart->vessel);
    CHECK(claw->vessel->parts.size() == nc + nt);
    CHECK(m->HasGrappleNode());
    CHECK(m->GrappleNode()->attachedPart != nullptr);
    CHECK(m->GrappleNode()->id == "grapple");
    CHECK(m->Events()["Release"]->active);
    REQUIRE(m->pivotJoint != nullptr);
    CHECK(m->pivotJoint->IsValid());
    CHECK(pivot->limit == m->Get<float>("pivotRange"));
    // SetLoose / LockPivot switch the pivot drive mode and the event visibility
    m->SetLoose();
    CHECK(pivot->mode == ActiveJointDriveMode::Neutral);
    CHECK(m->Events()["LockPivot"]->active);
    CHECK(m->IsLoose());
    m->LockPivot();
    CHECK(pivot->mode == ActiveJointDriveMode::Park);
    CHECK_FALSE(m->IsJointUnlocked());
    // Release (ModuleGrappleNode.cs:658): `parent != otherPart` -> otherPart.Undock(otherVesselInfo), else this part undocks with vesselInfo. The side that
    // hangs below leaves as a new vessel carrying the saved name, type and root.
    const bool otherHangsBelow = claw->parent != targetPart;
    Vessel* mergedVessel = claw->vessel;
    log.forces.clear();
    m->Release();
    CHECK(m->fsm->currentStateName == "Disengage");
    CHECK(claw->vessel != targetPart->vessel);
    Vessel* leaving = otherHangsBelow ? targetPart->vessel : claw->vessel;
    Vessel* staying = otherHangsBelow ? claw->vessel : targetPart->vessel;
    CHECK(staying == mergedVessel);
    CHECK(leaving->vesselName == (otherHangsBelow ? "Target Rock" : "Claw Vessel"));
    CHECK(leaving->vesselType == (otherHangsBelow ? VesselType::SpaceObject : VesselType::Ship));
    CHECK(leaving->parts.size() == (otherHangsBelow ? nt : nc));
    CHECK(staying->parts.size() == (otherHangsBelow ? nc : nt));
    CHECK(leaving->rootPart->fields.flightID == (otherHangsBelow ? 50000u : 40000u));
    // ejection: -undockEjectionForce * 0.5 on the claw part, + on the grabbed part, along the node forward (+Z here)
    const float ej = m->Get<float>("undockEjectionForce");
    REQUIRE(log.forces.size() == 2);
    CHECK(log.forces[0].first == claw);
    CHECK(log.forces[1].first == targetPart);
    CHECK(log.forces[0].second.z == -ej * 0.5f);
    CHECK(log.forces[1].second.z == ej * 0.5f);
    // the grapple node left the owner's attach nodes and was cleared (Release: attachNodes.Remove, attachedPart = owner = null)
    for (Part* p : {claw, targetPart})
        for (const AttachNode& n : p->attachNodes) CHECK(n.id != "grapple");
    CHECK_FALSE(m->Events()["Release"]->active);
    // Disengage -> Ready only once the grabbed part is more than minDistanceToReEngage from the grab point (on_nodeDistance, LATEUPDATE, after 2 frames)
    CHECK(m->otherPart == targetPart);
    CHECK_FALSE(m->GrappleNode()->attachedPart);
}
