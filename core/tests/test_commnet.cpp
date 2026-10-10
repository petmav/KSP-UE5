// WP-39 unit tests: CommNet and KerbNet (CommNet/*.cs, CommNet/Network/*.cs, CommNet/Occluders/*.cs, ModuleDataTransmitter.cs,
// ModuleKerbNetAccess.cs, ICommAntenna.cs, AntennaType.cs).
//
// No golden and no capture exists for CommNet (harness/golden/MANIFEST.json has no CommNet dataset; harness/captures/out and out2 carry no flight
// scene), so every expectation is derived: from the decompiled C# (cited in the code under test), from the game's own data (the antenna and KerbNet
// part cfgs under testdata/GameData-mirror, the Kerbol bodies of harness/golden/serialized/celestial_bodies.serialized.json) or from
// tools/wp39/commnet_reference.py, an independent Python implementation (tests/data/wp39_reference.json). The reference shares no code with kspcore;
// its Dijkstra-style searches include the .NET Dictionary enumeration order (a node's links), which decides every tie.
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <map>

#include "doctest/doctest.h"
#include "game_root.hpp"
#include "ksp/celestial/celestial_system.hpp"
#include "ksp/commnet/comm_net_vessel.hpp"
#include "ksp/commnet/comm_network.hpp"
#include "ksp/commnet/commnet_scene.hpp"
#include "ksp/core/coroutine.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/modules/module_command.hpp"
#include "ksp/modules/module_commnet_parts.hpp"
#include "ksp/modules/module_data_transmitter.hpp"
#include "ksp/modules/module_kerbnet_access.hpp"
#include "ksp/modules/scenario_runner.hpp"
#include "ksp/modules/vessel_module.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/parts/part_module_data.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/game_parameters.hpp"
#include "ksp/save/proto_vessel.hpp"
#include "ksp/vessel/vessel.hpp"
#include "nlohmann/json.hpp"
#include "resource_fixture.hpp"

using namespace ksp;
using namespace ksp::rtest;
using namespace ksp::CommNet;
using json = nlohmann::json;

namespace {

const json& Ref39() {
    static const json ref = [] {
        std::ifstream f(std::string(KSPCORE_TEST_DATA_DIR) + "/wp39_reference.json", std::ios::binary);
        REQUIRE(f.good());
        return json::parse(f);
    }();
    return ref;
}

int64_t UlpD(double a, double b) {
    int64_t x, y;
    std::memcpy(&x, &a, 8);
    std::memcpy(&y, &b, 8);
    if (x < 0) x = INT64_MIN - x;
    if (y < 0) y = INT64_MIN - y;
    return x > y ? x - y : y - x;
}

// the max ULP distance and the number of exact matches over a replay
struct Tally {
    int64_t maxUlp = 0;
    long values = 0, exact = 0;
    long reported = 0;
    void D(double got, double expected, const std::string& what = std::string()) {
        const int64_t u = UlpD(got, expected);
        maxUlp = std::max(maxUlp, u);
        values++;
        if (u == 0) exact++;
        else if (reported++ < 6) MESSAGE("mismatch " << what << ": got " << std::setprecision(17) << got << " expected " << expected << " (" << u << " ULP)");
    }
};

std::string GameDataDir39() { return ksp::testing::GameRoot() + "/GameData"; }

void LoadLocalizer39() {
    KSP::Localization::Localizer::Shutdown();
    KSP::Localization::Localizer::Init();
    ConfigNode::NodePtr root = ConfigNode::Load(GameDataDir39() + "/Squad/Localization/dictionary.cfg");
    REQUIRE(root);
    std::vector<ConfigNode*> nodes{root->GetNode("Localization")};
    KSP::Localization::Localizer::Instance()->AddTagValuesForLanguage(nodes, "en-us");
}

// A sandbox game with CommNet enabled and the stock difficulty (CommNetParams defaults); every static the package owns is reset
struct World39 {
    Game game;
    World39() {
        FlightRegistry::Clear();
        Vessel::env = VesselEnvironment{};
        game.Mode = Game_Modes::SANDBOX;
        game.Parameters->Difficulty.EnableCommNet = true;
        SaveEnvironment::Current().currentGame = &game;
        Reset();
    }
    ~World39() {
        Reset();
        SaveEnvironment::Current().currentGame = nullptr;
        FlightRegistry::Clear();
        TimeWarp::fetch = nullptr;
        Vessel::env = VesselEnvironment{};
    }
    static void Reset() {
        CommNetScenario::SetRangeModel(nullptr);
        CommNetEnvironment::Reset();
        CommandEnvironment::Reset();
        CommNetModuleEnvironment::Reset();
        TransmitterEnvironment::Reset();
        KerbNetEnvironment::Reset();
    }
    CommNetParams& Params() { return game.Parameters->CustomParams<CommNetParams>(); }
};

DoubleCurve DefaultRangeCurve() {
    DoubleCurve c;
    c.Add(0.0, 0.0, 0.0, 0.0);
    c.Add(1.0, 1.0, 0.0, 0.0);
    return c;
}

Vector3d V3(const json& a) { return Vector3d(a[0].get<double>(), a[1].get<double>(), a[2].get<double>()); }

// A bare vessel owning no parts (the parts belong to the test) plus the resource sets of its parts (as in test_command.cpp)
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

// A Vessel over parts owned by the test, optionally with the resource host
struct Ship {
    std::unique_ptr<Vessel> v = std::make_unique<Vessel>();
    std::unique_ptr<TestVesselHost> host;
    std::vector<Part*> parts;
    explicit Ship(std::vector<Part*> ps, bool withResources = false) : parts(std::move(ps)) {
        v->loaded = true;
        v->vesselType = VesselType::Ship;
        v->vesselName = "Test Ship";
        v->precalc.firstStatsRunComplete = true;  // GetWorldPos3D returns CoMD
        for (Part* p : parts) {
            v->parts.push_back(p);
            p->vessel = v.get();
        }
        if (withResources) {
            host = std::make_unique<TestVesselHost>(v.get());
            host->Update();
        }
    }
    ~Ship() {
        host.reset();
        for (Part* p : parts) p->vessel = nullptr;
        v->parts.clear();
    }
};

template <class T>
T* Find(Part& p, size_t index = 0) {
    for (size_t i = 0; i < p.Modules().Count(); i++)
        if (auto* m = dynamic_cast<T*>(p.Modules()[i]))
            if (index-- == 0) return m;
    return nullptr;
}

std::shared_ptr<CommNetVessel> NewCommNetVessel(Vessel& v) {
    auto m = std::static_pointer_cast<CommNetVessel>(VesselModuleFactory::Create(*GetVesselModuleClass("CommNetVessel")));
    m->vessel = &v;
    m->Awake();
    return m;
}

std::shared_ptr<PartModule> NewModule(const char* cls, PartPrefab* part = nullptr) {
    const ModuleClassInfo* c = GetPartModuleClass(cls);
    REQUIRE(c != nullptr);
    auto m = ModuleFactory::Create(*c);
    m->part = part;
    m->Awake();
    return m;
}

// an ICommNetControlSource with a scripted answer
struct FakeSource final : ICommNetControlSource {
    VesselControlState state;
    bool comm;
    int updates = 0;
    FakeSource(VesselControlState s, bool c) : state(s), comm(c) {}
    std::string ControlSourceName() const override { return "fake"; }
    void UpdateNetwork() override { updates++; }
    VesselControlState GetControlSourceState() const override { return state; }
    bool IsCommCapable() const override { return comm; }
};

}  // namespace

// ============================================================ range model and DSN

TEST_CASE("WP-39 CommRangeModel: InRange / GetNormalizedRange / GetMaximumRange over the stock antenna powers and DSN levels vs the reference") {
    const json& g = Ref39()["rangeGrid"];
    CommRangeModel rm;
    Tally t;
    int n = 0;
    for (const json& r : g["rows"]) {
        const double a = r["a"], b = r["b"], dist = r["distance"], sqr = r["sqr"], max = r["max"];
        CHECK(rm.InRange(a, b, sqr) == r["inRange"].get<bool>());
        t.D(rm.GetMaximumRange(a, b), max);
        t.D(rm.GetNormalizedRange(a, b, dist), r["norm"]);
        n++;
    }
    // the distinct stock antenna powers and the three DSN ranges, squared, x 9 distance fractions
    CHECK(n == static_cast<int>(g["powers"].size() * g["powers"].size() * 9));
    CHECK(g["powers"].size() == 8);
    INFO("max ULP " << t.maxUlp << " over " << t.values << " values");
    CHECK(t.maxUlp == 0);
}

TEST_CASE("WP-39 GameVariables.GetDSNRange / GetDSNRangeCurve and the CommNetParams presets") {
    World39 w;
    const json& ref = Ref39();
    for (const json& r : ref["dsn"]) {
        w.Params().DSNModifier = static_cast<float>(r["modifier"].get<double>());
        CHECK(DSN::GetDSNRange(static_cast<float>(r["level"].get<double>())) == r["range"].get<double>());
    }
    const DoubleCurve c = DSN::GetDSNRangeCurve();
    const DoubleCurve rc = DefaultRangeCurve();
    for (double t = -0.1; t < 1.2; t += 0.05) CHECK(c.Evaluate(t) == rc.Evaluate(t));
    // CommNetParams.SetDifficultyPreset (CommNetParams.cs:46)
    for (const char* name : {"Easy", "Moderate", "Hard"}) {
        CommNetParams p;
        p.SetDifficultyPreset(std::string(name) == "Easy" ? GameParameters::Preset::Easy
                                                          : (std::string(name) == "Moderate" ? GameParameters::Preset::Moderate : GameParameters::Preset::Hard));
        const json& pr = ref["presets"][name];
        CHECK(p.rangeModifier == static_cast<float>(pr["rangeModifier"].get<double>()));
        CHECK(p.occlusionMultiplierVac == static_cast<float>(pr["occVac"].get<double>()));
        CHECK(p.occlusionMultiplierAtm == static_cast<float>(pr["occAtm"].get<double>()));
    }
    // without a game: DSNModifier does not apply (HighLogic.CurrentGame == null)
    SaveEnvironment::Current().currentGame = nullptr;
    CHECK(DSN::GetDSNRange(0.0f) == 2000000000.0);
    CHECK(DSN::GetDSNRange(0.5f) == 50000000000.0);
    CHECK(DSN::GetDSNRange(0.50001f) == 250000000000.0);
    CHECK_FALSE(CommNetScenario::CommNetEnabled());
    SaveEnvironment::Current().currentGame = &w.game;
    CHECK(CommNetScenario::CommNetEnabled());
}

TEST_CASE("WP-39 NodeUtilities.ConvertSignalStrength and CommLink.GetSignalStrength thresholds") {
    CHECK(NodeUtilities::ConvertSignalStrength(0.7500000001) == SignalStrength::Green);
    CHECK(NodeUtilities::ConvertSignalStrength(0.75) == SignalStrength::Yellow);
    CHECK(NodeUtilities::ConvertSignalStrength(0.5) == SignalStrength::Orange);
    CHECK(NodeUtilities::ConvertSignalStrength(0.25) == SignalStrength::Red);
    CHECK(NodeUtilities::ConvertSignalStrength(1e-9) == SignalStrength::None);
    CHECK(NodeUtilities::ConvertSignalStrength(0.0) == SignalStrength::None);
    CommLink l;
    l.strengthRR = 0.4;
    l.strengthAR = 0.0;
    l.strengthBR = 0.9;
    CHECK(l.GetSignalStrength(true, true) == 0.4);
    CHECK(l.GetSignalStrength(true, false) == 0.4);   // `!bMustRelay && RR <= AR` is false (0.4 > 0): RR
    CHECK(l.GetSignalStrength(false, true) == 0.9);   // RR > BR is false, BR != 0: BR
    CHECK(l.GetBestSignal() == 0.9);
    l.strengthAR = 0.7;
    CHECK(l.GetSignalStrength(true, false) == 0.7);   // RR <= AR: AR
    l.strengthBR = 0.0;
    CHECK(l.GetSignalStrength(false, false) == 0.4);  // RR (0.4) > BR (0): RR
    l.strengthRR = 0.0;
    CHECK(l.GetSignalStrength(false, false) == 0.7);  // RR > BR is false, BR == 0: AR
}

// ============================================================ link creation

TEST_CASE("WP-39 CommNetwork.SetNodeConnection / TryConnect: link strengths over the stock antennas and DSN levels (reference)") {
    World39 w;
    const json& rows = Ref39()["signals"];
    DoubleCurve rc = DefaultRangeCurve();
    Tally t;
    long connected = 0;
    for (const json& r : rows) {
        // a fresh network per pair: an existing link keeps its `cost` (CommLink.Update sets only the signal strength), a new one takes the distance
        CommNetwork net;
        CommNode a, b;
        a.set_name("a");
        b.set_name("b");
        net.Add(&a);
        net.Add(&b);
        a.antennaRelay.Update(r["aRelay"], &rc, false);
        a.antennaTransmit.Update(r["aTransmit"], &rc, false);
        b.antennaRelay.Update(r["bRelay"], &rc, false);
        b.antennaTransmit.Update(r["bTransmit"], &rc, false);
        a.distanceOffset = r["offA"];
        b.distanceOffset = r["offB"];
        a.set_precisePosition(Vector3d(0.0, 0.0, 0.0));
        b.set_precisePosition(Vector3d(r["distance"].get<double>(), 0.0, 0.0));
        net.Rebuild();
        Network::Node::LinkPtr lp;
        const bool conn = a.TryGetValue(&b, lp);
        REQUIRE(conn == r["connected"].get<bool>());
        if (!conn) continue;
        connected++;
        CommLink* l = static_cast<CommLink*>(lp.get());
        t.D(l->cost, r["cost"], "cost");
        t.D(l->signalStrength(), r["sS"], "sS");
        t.D(l->strengthRR, r["rr"], "rr");
        t.D(l->strengthAR, r["ar"], "ar");
        t.D(l->strengthBR, r["br"], "br");
        CHECK(l->aCanRelay == r["aCan"].get<bool>());
        CHECK(l->bCanRelay == r["bCan"].get<bool>());
        CHECK(l->bothRelay == r["both"].get<bool>());
        int k = 0;
        for (bool x : {false, true})
            for (bool y : {false, true}) t.D(l->GetSignalStrength(x, y), r["sig"][k++], "sig");
        t.D(l->GetBestSignal(), r["best"], "best");
        CHECK(static_cast<int>(l->signal) == r["signalEnum"].get<int>());
    }
    CHECK(connected > 1500);
    INFO("max ULP " << t.maxUlp << " over " << t.values << " values, " << connected << " links of " << rows.size() << " pairs");
    CHECK(t.maxUlp == 0);
}

TEST_CASE("WP-39 CommNode: two homes never link; the signal modifier scales every link (CommNode.GetSignalStrengthMultiplier)") {
    World39 w;
    DoubleCurve rc = DefaultRangeCurve();
    CommNetwork net;
    CommNode a, b;
    a.isHome = b.isHome = true;
    a.antennaRelay.Update(1e9, &rc, false);
    b.antennaRelay.Update(1e9, &rc, false);
    b.set_precisePosition(Vector3d(1e6, 0.0, 0.0));
    net.Add(&a);
    net.Add(&b);
    net.Rebuild();
    CHECK(a.Count() == 0);
    b.isHome = false;
    net.Rebuild();
    REQUIRE(a.Count() == 1);
    Network::Node::LinkPtr lp;
    REQUIRE(a.TryGetValue(&b, lp));
    const double full = static_cast<CommLink*>(lp.get())->strengthRR;
    CHECK(full > 0.0);
    a.OnLinkCreateSignalModifier = [](CommNode*) { return 0.5; };
    net.Rebuild();
    REQUIRE(a.TryGetValue(&b, lp));
    CHECK(static_cast<CommLink*>(lp.get())->strengthRR == full * 0.5);
    a.OnLinkCreateSignalModifier = [](CommNode*) { return 0.0; };
    net.Rebuild();
    CHECK(a.Count() == 0);  // strength 0: TryConnect disconnects
    CHECK(net.Links().empty());
}

// ============================================================ horizon occlusion over the Kerbol bodies

TEST_CASE("WP-39 CommNetBody.CreateOccluder / OccluderHorizonCulling over the 17 Kerbol bodies and the four difficulty presets (reference)") {
    World39 w;
    const json& ref = Ref39();
    const PSystemPrefab& sys = KerbolSystemPrefab();
    Tally t;
    long rays = 0, blocked = 0, nets = 0;
    for (const json& row : ref["occlusion"]) {
        const CelestialBodyPrefab* bp = nullptr;
        for (const CelestialBodyPrefab& p : sys.bodies)
            if (p.body.bodyName == row["body"].get<std::string>()) bp = &p;
        REQUIRE(bp != nullptr);
        CelestialBody& body = const_cast<CelestialBody&>(bp->body);
        const json& pr = ref["presets"][row["preset"].get<std::string>()];
        w.Params().occlusionMultiplierVac = static_cast<float>(pr["occVac"].get<double>());
        w.Params().occlusionMultiplierAtm = static_cast<float>(pr["occAtm"].get<double>());
        body.position = V3(row["bodyPos"]);
        CommNetBody cb;
        cb.body = &body;
        std::unique_ptr<Occluder> occ = cb.CreateOccluder();
        t.D(occ->radius, row["radius"]);
        occ->Update();  // OccluderHorizonCulling.Update: position = body.position, invRotation = Inverse(transform.rotation)
        CommNetwork net;
        net.Add(occ.get());
        DoubleCurve rc = DefaultRangeCurve();
        CommNode a, b;
        a.antennaRelay.Update(1e15, &rc, false);
        b.antennaRelay.Update(1e15, &rc, false);
        net.Add(&a);
        net.Add(&b);
        for (const json& geo : row["geo"]) {
            const Vector3d s = V3(geo["s"]), d = V3(geo["d"]);
            const double dist = geo["dist"];
            const bool bl = occ->Raycast(s, d);
            CHECK(bl == geo["blocked"].get<bool>());
            const bool inr = occ->InRange(s, dist);
            CHECK(inr == geo["inRange"].get<bool>());
            rays++;
            blocked += bl;
            // end to end: CommNetwork.TestOcclusion inside SetNodeConnection
            a.set_precisePosition(s);
            b.set_precisePosition(d);
            net.Rebuild();
            CHECK(a.ContainsKey(&b) == !(inr && bl));
            nets++;
        }
    }
    CHECK(rays == 68 * 24);
    CHECK(blocked > 100);
    INFO("max ULP " << t.maxUlp << " over " << t.values << " radii; " << rays << " rays (" << blocked << " blocked), " << nets << " end-to-end links");
    CHECK(t.maxUlp == 0);
}

TEST_CASE("WP-39 OccluderHorizonCulling: a zero radius never occludes; an ellipsoid scales the rays") {
    OccluderHorizonCulling none(nullptr, 0.0, 600000.0, 600000.0);
    CHECK_FALSE(none.InRange(Vector3d(1e6, 0, 0), 1e9));
    CHECK_FALSE(none.Raycast(Vector3d(1e6, 0, 0), Vector3d(-1e6, 0, 0)));
    UnityEngine::Transform tr;
    OccluderHorizonCulling ball(&tr, 600000.0, 600000.0, 600000.0);
    ball.Update();
    CHECK(ball.radius == 600000.0);
    CHECK(ball.Raycast(Vector3d(1e6, 0, 0), Vector3d(-1e6, 0, 0)));        // through the centre
    CHECK_FALSE(ball.Raycast(Vector3d(1e6, 0, 0), Vector3d(1e6, 2e6, 0)));  // a chord outside
    // a flattened pole axis (Y radius 0.5): a ray that grazes the sphere at the pole is blocked, one that misses the squashed body is not
    OccluderHorizonCulling flat(&tr, 600000.0, 300000.0, 600000.0);
    flat.Update();
    CHECK(flat.Raycast(Vector3d(0, 1e6, 0), Vector3d(0, -1e6, 0)));
    CHECK_FALSE(flat.Raycast(Vector3d(500000.0, 400000.0, 0), Vector3d(-500000.0, 400000.0, 0)));
}

// ============================================================ best path search, dictionary order

namespace {

struct GraphWorld {
    std::vector<std::unique_ptr<CommNode>> storage;
    std::map<std::string, CommNode*> nodes;
    DoubleCurve rc = DefaultRangeCurve();
    CommNetwork net;
    explicit GraphWorld(const json& spec) {
        for (const json& s : spec["nodes"]) {
            auto n = std::make_unique<CommNode>();
            n->set_name(s["name"]);
            n->antennaRelay.Update(s["relay"], &rc, false);
            n->antennaTransmit.Update(s["transmit"], &rc, false);
            n->isHome = s["home"];
            n->isControlSource = s["control"];
            n->isControlSourceMultiHop = s["multihop"];
            n->set_precisePosition(V3(s["pos"]));
            nodes[n->name()] = n.get();
            storage.push_back(std::move(n));
        }
    }
};

json LinkOrder(CommNetwork& net) {
    json j = json::object();
    for (int i = 0; i < net.Count(); i++) {
        CommNode* n = net.NodeAt(i);
        json names = json::array();
        for (const auto& kv : n->links()) names.push_back(kv.first->name());
        j[n->name()] = names;
    }
    return j;
}

// the observable result of a path query as the reference reports it
bool CheckQuery(CommNetwork& net, std::map<std::string, CommNode*>& nodes, const json& res, Tally& t) {
    const json& q = res["q"];
    CommNode* from = nodes.at(q["from"].get<std::string>());
    CommPath path;
    bool found = false;
    const std::string kind = q["kind"];
    if (kind == "home") found = net.FindHome(from, &path);
    else if (kind == "control") found = net.FindClosestControlSource(from, &path);
    else found = net.FindPath(from, &path, nodes.at(q["to"].get<std::string>()));
    CHECK(found == res["found"].get<bool>());
    if (!found || !res["found"].get<bool>()) return found == res["found"].get<bool>();
    const json& p = res["path"];
    REQUIRE(path.Count() == static_cast<int>(p["costs"].size()));
    CHECK(path.LinkAt(0)->A()->name() == p["nodes"][0].get<std::string>());
    for (int i = 0; i < path.Count(); i++) {
        CHECK(path.LinkAt(i)->B()->name() == p["nodes"][static_cast<size_t>(i) + 1].get<std::string>());
        t.D(path.LinkAt(i)->cost, p["costs"][static_cast<size_t>(i)]);
        t.D(path.LinkAt(i)->signalStrength(), p["hopStrength"][static_cast<size_t>(i)]);
    }
    t.D(path.signalStrength(), p["signalStrength"]);
    CHECK(static_cast<int>(path.signal()) == p["signal"].get<int>());
    CHECK(path.IsLastHopHome() == p["lastHopHome"].get<bool>());
    CHECK(path.LastLink()->hopType == (p["lastHopHome"].get<bool>() ? HopType::Home : HopType::ControlPoint));
    if (kind != "path") CHECK(path.LastLink()->B()->name() == res["target"].get<std::string>());
    return true;
}

}  // namespace

TEST_CASE("WP-39 CommNetwork best path: FindHome / FindClosestControlSource / FindPath on hand-built graphs, node removal and movement (reference)") {
    World39 w;
    Tally t;
    long queries = 0, foundTotal = 0;
    for (const json& g : Ref39()["graphs"]) {
        INFO("graph " << g["name"].get<std::string>());
        GraphWorld gw(g["spec"]);
        for (const json& s : g["spec"]["nodes"]) gw.net.Add(gw.nodes.at(s["name"].get<std::string>()));
        gw.net.Rebuild();
        for (const json& ph : g["phases"]) {
            INFO("phase " << ph["label"].get<std::string>());
            for (const json& op : ph["ops"]) {
                CommNode* n = gw.nodes.at(op["node"].get<std::string>());
                if (op["op"] == "remove") {
                    CHECK(gw.net.Remove(n));
                    gw.net.Rebuild();
                } else if (op["op"] == "add") {
                    gw.net.Add(n);
                    gw.net.Rebuild();
                } else {
                    n->set_precisePosition(V3(op["pos"]));
                    gw.net.Rebuild();
                }
            }
            CHECK(LinkOrder(gw.net) == ph["links"]);  // each node's links enumerate in .NET Dictionary order (free-list reuse after Remove)
            for (const json& res : ph["results"]) {
                queries++;
                foundTotal += res["found"].get<bool>();
                CheckQuery(gw.net, gw.nodes, res, t);
            }
        }
    }
    CHECK(queries > 800);
    CHECK(foundTotal > 400);
    INFO(queries << " queries, " << foundTotal << " found, max ULP " << t.maxUlp << " over " << t.values << " values");
    CHECK(t.maxUlp == 0);
}

TEST_CASE("WP-39 generic Net: Dijkstra FindClosestWhere / FindPath on a node graph with unit link costs") {
    // the abstract Net.FindPath / FindClosestWhere (distance based, not the CommNetwork override): a chain a - b - c - d plus a shortcut a - d
    struct Plain : Network::Net {
        std::shared_ptr<Network::Link> NewLink() override { return std::make_shared<Network::Link>(); }
        bool SetNodeConnection(Network::Node*, Network::Node*) override { return false; }
        void LinkNodes(Network::Node* a, Network::Node* b, double d) { Connect(a, b, d); }
        void Unlink(Network::Node* a, Network::Node* b) { Disconnect(a, b); }
    };
    struct N : Network::Node {
        std::string n;
        explicit N(std::string s) : n(std::move(s)) {}
        std::string name() const override { return n; }
    };
    Plain net;
    N a("a"), b("b"), c("c"), d("d");
    for (N* n : {&a, &b, &c, &d}) net.Add(n);
    net.LinkNodes(&a, &b, 1.0);
    net.LinkNodes(&b, &c, 1.0);
    net.LinkNodes(&c, &d, 1.0);
    net.LinkNodes(&a, &d, 5.0);
    Network::Path p;
    // Net.cs:202 FindPath: CreateShortestPathTree relaxes with the node's own bestCost (Net.cs:315, no link cost) and the path is built end first
    // as {a = end} links; the final `path[0].Contains(start)` test then fails for every start != end (path[0] is the END link), so the generic
    // FindPath clears the path and returns false (CommNetwork overrides FindPath; the generic one is dead code in KSP)
    CHECK_FALSE(net.FindPath(&a, &p, &d));
    CHECK(p.Count() == 0);
    CHECK(d.bestLinkNode == &a);   // the search itself reached d over the direct link (discovered first from a)
    // FindClosestWhere adds the link costs: the closest node satisfying the predicate is b (cost 1), not d. With a path the same
    // `path[0].Contains(start)` test returns null (Net.cs:304); without a path the node is returned.
    Network::Node* hit = net.FindClosestWhere(&a, nullptr, [&](Network::Node*, Network::Node* x) { return x == &b || x == &d; });
    CHECK(hit == &b);
    Network::Path q;
    CHECK(net.FindClosestWhere(&a, &q, [&](Network::Node*, Network::Node* x) { return x == &b; }) == nullptr);
    CHECK(q.Count() == 0);
    CHECK(net.Count() == 4);
    CHECK(net.Links().size() == 4);
    net.Unlink(&a, &d);
    CHECK(net.Links().size() == 3);
    CHECK(net.Remove(&b));
    CHECK(net.Count() == 3);
    CHECK(a.Count() == 0);
    CHECK(c.Count() == 1);
}

// ============================================================ the scenario, network and ground stations

TEST_CASE("WP-39 CommNetScenario: the physprofile SCENARIO node round-trips; Awake / Start / OnDestroy lifecycle") {
    World39 w;
    auto root = ConfigNode::Load(std::string(KSPCORE_TESTDATA_DIR) + "/saves/physprofile/persistent.sfs");
    REQUIRE(root);
    ConfigNode* game = root->GetNode("GAME");
    REQUIRE(game != nullptr);
    ConfigNode* sc = nullptr;
    for (ConfigNode* n : game->GetNodes("SCENARIO"))
        if (n->GetValue("name") == std::string("CommNetScenario")) sc = n;
    REQUIRE(sc != nullptr);
    auto in = ConfigNode::Create();
    sc->CopyTo(*in);
    ScenarioModuleFactory::Register("CommNetScenario", [](const std::string&) { return std::unique_ptr<ScenarioModule>(std::make_unique<CommNetScenario>()); });
    // ProtoScenarioModule.Load (ScenarioRunner host): Awake, targetScenes from `scene`, ScenarioModule.Load; then a snapshot saves it again
    ProtoScenarioModule proto(*in);
    ScenarioRunner runner;
    ScenarioModule* mod = runner.LoadProto(proto);
    REQUIRE(mod != nullptr);
    auto* cns0 = dynamic_cast<CommNetScenario*>(mod);
    REQUIRE(cns0 != nullptr);
    CHECK(CommNetScenario::Instance() == cns0);
    CHECK(mod->ClassName() == "CommNetScenario");
    ProtoScenarioModule snap(*mod);
    auto out = ConfigNode::Create();
    snap.Save(*out);
    CHECK(out->ToString() == in->ToString());
    auto m = ScenarioModuleFactory::Create("CommNetScenario");
    REQUIRE(m != nullptr);
    m->Awake();   // a second scenario: "Instance already exists" -> the first one is destroyed
    CHECK(cns0->destroyed());
    CHECK(CommNetScenario::Instance() == dynamic_cast<CommNetScenario*>(m.get()));
    auto* cns = dynamic_cast<CommNetScenario*>(m.get());
    CHECK(cns->Network() == nullptr);
    cns->Start();
    REQUIRE(cns->Network() != nullptr);
    CHECK(CommNetNetwork::Initialized());
    CHECK(CommNetNetwork::Instance() == cns->Network());
    CHECK(cns->Network()->CommNet() != nullptr);
    cns->OnDestroy();
    CHECK_FALSE(CommNetNetwork::Initialized());
    CHECK(CommNetScenario::Instance() == nullptr);
    // CommNet off in the difficulty: the module destroys itself (OnAwake)
    w.game.Parameters->Difficulty.EnableCommNet = false;
    auto m2 = ScenarioModuleFactory::Create("CommNetScenario");
    m2->Awake();
    CHECK(dynamic_cast<CommNetScenario*>(m2.get())->destroyed());
    CHECK(CommNetScenario::Instance() == nullptr);
}

TEST_CASE("WP-39 CommNetNetwork.Update: rebuild timing (QueueRebuild, dirty graph, focused vessel)") {
    World39 w;
    CommNetNetwork net;
    net.Awake();
    REQUIRE(net.CommNet() != nullptr);
    int rebuilds = 0;
    auto hook = [&] { rebuilds++; };
    net.CommNet()->OnNetworkPreUpdate = hook;
    float now = 0.0f;
    CommNetEnvironment::timeSinceLevelLoad = [&] { return now; };
    net.Update();  // never rebuilt: prevUpdate + interval <= now
    CHECK(rebuilds == 1);
    // no focused vessel: `prevUpdate + 0 > now` is false for now >= prevUpdate, so every Update rebuilds (CommNetNetwork.cs:99 as decompiled)
    now = 0.1f;
    net.Update();
    CHECK(rebuilds == 2);
    // a focused vessel limits the rate to packedInterval (0.5 s: no vessel is active so the packed interval applies)
    Vessel focus;
    net.OnMapFocusChange(&focus);
    now = 0.3f;
    net.Update();
    CHECK(rebuilds == 2);
    CHECK(net.GraphDirty());
    now = 0.7f;
    net.Update();
    CHECK(rebuilds == 3);
    CHECK_FALSE(net.GraphDirty());
    now = 0.8f;
    net.QueueRebuild();
    net.Update();
    CHECK(rebuilds == 4);
    // a dirty graph (a node was added) rebuilds at once
    CommNode n;
    now = 0.9f;
    CommNetNetwork::Add(&n);
    CHECK(net.CommNet()->IsDirty());
    net.Update();
    CHECK(rebuilds == 5);
    CommNetNetwork::Remove(&n);
    net.OnDestroy();
}

TEST_CASE("WP-39 CommNetHome / CommNetBody: ground station nodes and planet occluders join the network on OnNetworkInitialized") {
    World39 w;
    const PSystemPrefab& sys = KerbolSystemPrefab();
    const CelestialBodyPrefab* kerbin = nullptr;
    for (const CelestialBodyPrefab& p : sys.bodies)
        if (p.body.bodyName == "Kerbin") kerbin = &p;
    REQUIRE(kerbin != nullptr);
    CelestialBody& body = const_cast<CelestialBody&>(kerbin->body);
    body.position = Vector3d(0.0, 0.0, 0.0);
    UnityEngine::Transform ksc;
    ksc.position = Vector3f(0.0f, 0.0f, 0.0f);
    {
        Vector3d p = body.GetWorldSurfacePosition(-0.0972, -74.5575, 70.0);
        ksc.position = p.ToVector3f();
    }
    CommNetScenario scenario;
    scenario.Awake();
    scenario.Start();
    REQUIRE(scenario.Network() != nullptr);
    CommNetwork* net = scenario.Network()->CommNet();
    CommNetHome home;
    home.nodeName = "KSC";
    home.displaynodeName = "Kerbal Space Center";
    home.isKSC = true;
    home.body = &body;
    home.transform = &ksc;
    home.Start();
    REQUIRE(home.Comm() != nullptr);
    CHECK(net->Count() == 1);
    CHECK(home.Comm()->isHome);
    CHECK(home.Comm()->isControlSource);
    CHECK(home.Comm()->isControlSourceMultiHop);
    CHECK(home.Comm()->name() == "KSC");
    CHECK(home.Comm()->displayName() == "Kerbal Space Center");
    // tracking station level 0 (unset hook): GameVariables.GetDSNRange(0) = 2 Gm
    CHECK(home.Comm()->antennaRelay.power == 2000000000.0);
    CHECK(home.Comm()->antennaRelay.rangeCurve != nullptr);
    CommNetEnvironment::trackingStationLevel = [] { return 1.0f; };
    home.OnNetworkInitialized();  // the event fires again (settings applied): the node is rebuilt in place
    CHECK(net->Count() == 1);
    CHECK(home.Comm()->antennaRelay.power == 250000000000.0);
    // the node follows the surface position of the lat / lon / alt it was created at
    home.Comm()->NetworkPreUpdate();
    const Vector3d surf = home.Comm()->precisePosition();
    CHECK(std::abs(surf.x - static_cast<double>(ksc.position.x)) < 1e-2);
    CHECK(std::abs(surf.y - static_cast<double>(ksc.position.y)) < 1e-2);
    CHECK(std::abs(surf.z - static_cast<double>(ksc.position.z)) < 1e-2);
    // a permanent (other launch site) station uses its own antennaPower
    CommNetHome site;
    site.nodeName = "Island";
    site.isPermanent = true;
    site.antennaPower = 123456.0;
    site.body = &body;
    site.transform = &ksc;
    site.Start();
    REQUIRE(site.Comm() != nullptr);
    CHECK(site.Comm()->antennaRelay.power == 123456.0);
    CHECK(net->Count() == 2);
    // enableGroundStations off: only the KSC stays (not isKSC && permanent && !AllowOtherLaunchSites)
    w.Params().enableGroundStations = false;
    site.OnNetworkInitialized();
    CHECK(site.Comm() == nullptr);
    CHECK(net->Count() == 1);
    home.OnNetworkInitialized();
    REQUIRE(home.Comm() != nullptr);
    // the planet occluder
    CommNetBody cb;
    cb.body = &body;
    cb.Start();
    REQUIRE(cb.GetOccluder() != nullptr);
    CHECK(cb.GetOccluder()->radius == 600000.0 * 1.0 * static_cast<double>(w.Params().occlusionMultiplierAtm));  // Kerbin has an atmosphere
    home.OnDestroy();
    CHECK(net->Count() == 0);
    cb.OnDestroy();
    scenario.OnDestroy();
}

// ============================================================ the vessel module

TEST_CASE("WP-39 CommNetVessel.UpdateComm: antenna aggregation over random sets of stock antennas, relay enabler and range modifier (reference)") {
    if (!GameAvailable()) return;
    Game15 g;
    World39 w;
    LoadLocalizer39();
    const json& rows = Ref39()["updateComm"];
    Tally t;
    long compared = 0;
    for (const json& row : rows) {
        w.Params().rangeModifier = static_cast<float>(row["rangeModifier"].get<double>());
        std::vector<std::unique_ptr<Part>> owned;
        std::vector<Part*> ptrs;
        for (const json& pn : row["parts"]) {
            REQUIRE_MESSAGE(g.data.parts.getPartInfoByName(pn.get<std::string>()) != nullptr, pn.get<std::string>());
        owned.push_back(g.New(pn.get<std::string>().c_str()));
            ptrs.push_back(owned.back().get());
        }
        std::shared_ptr<PartModule> relay;
        if (row["relayEnabler"].get<bool>()) {
            relay = NewModule("ModuleRelayEnabler", ptrs.back());
            static_cast<ModuleRelayEnabler*>(relay.get())->isRelaying = true;
            ptrs.back()->Modules().Add(relay);
        }
        Ship ship(ptrs);
        auto m = NewCommNetVessel(*ship.v);
        m->Start();
        REQUIRE(m->Comm() != nullptr);
        CHECK(ship.v->connection == m.get());
        m->OnNetworkPreUpdate();
        const json& e = row["expect"];
        t.D(m->Comm()->antennaTransmit.power, e["transmit"]);
        t.D(m->Comm()->antennaRelay.power, e["relay"]);
        if (e["transmit"].get<double>() > 0.0) CHECK(m->Comm()->antennaTransmit.combined == e["tcomb"].get<bool>());
        if (e["relay"].get<double>() > 0.0) CHECK(m->Comm()->antennaRelay.combined == e["rcomb"].get<bool>());
        m->IsConnectedHome = true;
        CHECK(m->CanScience() == e["hasScience"].get<bool>());
        CHECK(m->Comm()->name() == "Test Ship");
        compared++;
        if (relay) ptrs.back()->Modules().Remove(relay.get());
        m->OnDestroy();
        CHECK(ship.v->connection == nullptr);
    }
    CHECK(compared == 240);
    INFO("max ULP " << t.maxUlp << " over " << t.values << " values (" << t.exact << " exact; the combined-antenna powers go through Math.Pow)");
    CHECK(t.maxUlp == 0);
}

TEST_CASE("WP-39 CommNetVessel: control state evaluation, GetControlLevel and Vessel.GetControlLevel (reference table)") {
    World39 w;
    const json& c = Ref39()["control"];
    Vessel v;
    v.loaded = true;
    v.vesselType = VesselType::Ship;
    auto m = NewCommNetVessel(v);
    for (const json& l : c["levels"]) {
        FakeSource s(static_cast<VesselControlState>(l["state"].get<int>()), true);
        m->CommandSources().clear();
        m->RegisterCommandSource(&s);
        m->RegisterCommandSource(&s);  // AddUnique
        CHECK(m->CommandSources().size() == 1);
        m->UpdateControlState();
        CHECK(static_cast<int>(m->ControlState()) == l["state"].get<int>());
        CHECK(static_cast<int>(m->GetControlLevel()) == l["level"].get<int>());
    }
    for (const json& combo : c["combos"]) {
        std::vector<std::unique_ptr<FakeSource>> srcs;
        m->CommandSources().clear();
        for (const json& s : combo["sources"]) {
            srcs.push_back(std::make_unique<FakeSource>(static_cast<VesselControlState>(s["state"].get<int>()), s["comm"].get<bool>()));
            m->RegisterCommandSource(srcs.back().get());
        }
        m->UpdateControlState();
        CHECK(static_cast<int>(m->ControlState()) == combo["controlState"].get<int>());
        CHECK(m->CanComm() == combo["canComm"].get<bool>());
        CHECK(static_cast<int>(m->GetControlLevel()) == combo["level"].get<int>());
        // Vessel.GetControlLevel: connection.GetControlLevel() when CommNet is on and the scenario exists, else the parts' isControlSource
        v.connection = m.get();
        CHECK(static_cast<int>(v.GetControlLevel()) == 0);  // no CommNetScenario.Instance yet: the (empty) parts decide
    }
    CommNetScenario scenario;
    scenario.Awake();
    FakeSource full(VesselControlState::ProbeFull, true);
    m->CommandSources().clear();
    m->RegisterCommandSource(&full);
    m->UpdateControlState();
    v.connection = m.get();
    CHECK(v.GetControlLevel() == ControlLevel::FULL);
    w.game.Parameters->Difficulty.EnableCommNet = false;
    CHECK(v.GetControlLevel() == ControlLevel::NONE);
    w.game.Parameters->Difficulty.EnableCommNet = true;
    // the save round trip of the two persistent fields
    ConfigNode node;
    m->Save(node);
    CHECK(node.GetValue("controlState") == std::string("ProbeFull"));
    CHECK(node.GetValue("canComm") == std::string("True"));
    scenario.OnDestroy();
    m->OnDestroy();
    v.connection = nullptr;
}

TEST_CASE("WP-39 CommNetVessel: control sources of the vessel parts (ModuleCommand is an ICommNetControlSource), connection events and plasma blackout") {
    if (!GameAvailable()) return;
    Game15 g;
    World39 w;
    LoadLocalizer39();
    auto pod = g.New("probeCoreOcto.v2");
    REQUIRE(pod != nullptr);
    ModuleCommand* cmd = Find<ModuleCommand>(*pod);
    REQUIRE(cmd != nullptr);
    CHECK(dynamic_cast<ICommNetControlSource*>(cmd) != nullptr);
    Ship ship({pod.get()});
    auto m = NewCommNetVessel(*ship.v);
    m->Start();
    m->Update();  // part count changed: FindCommandSources
    REQUIRE(m->CommandSources().size() == 1);
    CHECK(m->CommandSources()[0] == static_cast<ICommNetControlSource*>(cmd));
    // ModuleCommand.Connection: the real vessel.connection (no hook)
    CommNode home;
    DoubleCurve rc = DefaultRangeCurve();
    home.isHome = true;
    home.isControlSource = true;
    home.antennaRelay.Update(2e9, &rc, false);
    home.set_name("KSC");
    CommNetwork net;
    net.Add(&home);
    net.Add(m->Comm());
    m->Comm()->antennaTransmit.Update(5e6, &rc, false);
    ship.v->CoMD = Vector3d(1e6, 0, 0);   // the vessel node follows vessel.GetWorldPos3D() (CoMD) in OnNetworkPreUpdate
    m->Comm()->set_name("Test Ship");
    // events: OnCommStatusChange / OnCommHomeStatusChange fire on the transitions of CreateControlConnection
    std::vector<std::pair<Vessel*, bool>> status, homeStatus;
    struct Sub {
        std::vector<std::pair<Vessel*, bool>>* s;
        std::vector<std::pair<Vessel*, bool>>* h;
        void S(Vessel* v, bool b) { s->push_back({v, b}); }
        void H(Vessel* v, bool b) { h->push_back({v, b}); }
    } sub{&status, &homeStatus};
    GameEvents::CommNet::OnCommStatusChange.Add(&sub, &Sub::S);
    GameEvents::CommNet::OnCommHomeStatusChange.Add(&sub, &Sub::H);
    net.Rebuild();   // the vessel node's OnNetworkPostUpdate runs CreateControlConnection
    CHECK(m->IsConnected);
    CHECK(m->IsConnectedHome);
    CHECK(m->Signal() != SignalStrength::None);
    CHECK(m->SignalStrengthValue() > 0.0);
    CHECK(m->ControlPath().IsLastHopHome());
    REQUIRE(status.size() == 1);
    CHECK(status[0].second);
    REQUIRE(homeStatus.size() == 1);
    CHECK_FALSE(m->CreateControlConnection());  // unchanged: no event
    CHECK(status.size() == 1);
    // out of range: disconnected, the events fire with false
    ship.v->CoMD = Vector3d(1e15, 0, 0);
    net.Rebuild();
    CHECK_FALSE(m->IsConnected);
    CHECK(status.size() == 2);
    CHECK_FALSE(status[1].second);
    CHECK_FALSE(homeStatus[1].second);
    GameEvents::CommNet::OnCommStatusChange.Remove(&sub, &Sub::S);
    GameEvents::CommNet::OnCommHomeStatusChange.Remove(&sub, &Sub::H);

    // plasma blackout: CalculatePlasmaMult and GetSignalStrengthModifier
    w.Params().plasmaBlackout = true;
    ship.v->packed = false;
    ship.v->externalTemperature = PhysicsGlobals::CommNetTempForBlackout() + 1.0;
    ship.v->atmDensity = PhysicsGlobals::CommNetDensityForBlackout() + 1.0;
    const double qMin = PhysicsGlobals::CommNetQTimesVelForBlackoutMin(), qMax = PhysicsGlobals::CommNetQTimesVelForBlackoutMax();
    ship.v->srfSpeed = 1.0;
    ship.v->dynamicPressurekPa = (qMin + qMax) * 0.5;
    ship.v->srf_vel_direction = Vector3d(1.0, 0.0, 0.0);
    ship.v->CoMD = Vector3d(0.0, 0.0, 0.0);
    m->OnNetworkPreUpdate();
    CHECK(m->InPlasma());
    CommNode other;
    other.set_precisePosition(Vector3d(1e6, 0.0, 0.0));  // straight ahead: dot = 1
    const double mult = UtilMath::InverseLerp(qMin, qMax, (qMin + qMax) * 0.5);
    double expect = 1.0 - mult;  // num2 = 1 * plasmaMult; num2 = 1 - num2
    const double th = PhysicsGlobals::CommNetBlackoutThreshold();
    expect = expect < th ? 0.0 : (expect - th) / (1.0 - th);
    CHECK(m->GetSignalStrengthModifier(&other) == expect);
    other.set_precisePosition(Vector3d(-1e6, 0.0, 0.0));  // behind: dot -1 < DotMin -> num2 = 0 -> 1
    CHECK(m->GetSignalStrengthModifier(&other) == 1.0);
    // canComm false: the link modifier is 0
    FakeSource none(VesselControlState::None, false);
    m->CommandSources().clear();
    m->RegisterCommandSource(&none);
    m->UpdateControlState();
    CHECK(m->GetSignalStrengthModifier(&other) == 0.0);
    m->OnDestroy();
}

TEST_CASE("WP-39 CommNetVessel.GetBestTransmitter picks the lowest ScienceUtil.GetTransmitterScore among the antennas that reach the first hop") {
    if (!GameAvailable()) return;
    Game15 g;
    World39 w;
    LoadLocalizer39();
    std::vector<std::string> names;
    for (const json& a : Ref39()["antennas"])
        if (a["type"] != "INTERNAL" && names.size() < 4) names.push_back(a["part"].get<std::string>());
    REQUIRE(names.size() == 4);
    std::vector<std::unique_ptr<Part>> owned;
    std::vector<Part*> ptrs;
    for (const std::string& n : names) {
        owned.push_back(g.New(n.c_str()));
        ptrs.push_back(owned.back().get());
    }
    Ship ship(ptrs);
    auto m = NewCommNetVessel(*ship.v);
    m->Start();
    CHECK(m->GetBestTransmitter() == nullptr);   // not connected
    DoubleCurve rc = DefaultRangeCurve();
    CommNode home;
    home.isHome = true;
    home.isControlSource = true;
    home.antennaRelay.Update(250000000000.0, &rc, false);
    home.set_name("KSC");
    CommNetwork net;
    net.Add(&home);
    net.Add(m->Comm());
    ship.v->CoMD = Vector3d(2.0e6, 0, 0);
    for (Part* p : ptrs) Find<ModuleDataTransmitter>(*p)->deployFxModules = std::vector<IScalarModule*>{};
    net.Rebuild();
    REQUIRE(m->IsConnectedHome);
    IScienceDataTransmitter* best = m->GetBestTransmitter();
    REQUIRE(best != nullptr);
    // the expectation: ScienceUtil.GetTransmitterScore (ScienceUtil.cs:327) over every transmitter that can transmit and reach the home relay power
    Network::Link* first = m->ControlPath().First().get();
    double cost = first->start()->At(first->end())->cost;
    cost *= cost;
    const double power = home.antennaRelay.power;
    const bool combined = (m->Comm()->antennaRelay.power > m->Comm()->antennaTransmit.power) ? m->Comm()->antennaRelay.combined : m->Comm()->antennaTransmit.combined;
    float bestScore = 3.40282347E+38f;
    IScienceDataTransmitter* expected = nullptr;
    for (int i = static_cast<int>(ptrs.size()) - 1; i >= 0; i--) {
        ModuleDataTransmitter* tx = Find<ModuleDataTransmitter>(*ptrs[static_cast<size_t>(i)]);
        if (!tx->CanTransmit() || !tx->CanScienceTo(combined, power, cost)) continue;
        const float score = static_cast<float>(1.0 / static_cast<double>(tx->DataRate()) * (1.0 + static_cast<double>(static_cast<float>(tx->DataResourceCost()))) *
                                                   (tx->IsBusy() ? 100.0 : 1.0) * 1.0);
        if (score < bestScore) {
            bestScore = score;
            expected = tx;
        }
    }
    CHECK(best == expected);
    // a busy transmitter scores 100x worse
    auto* bestTx = static_cast<ModuleDataTransmitter*>(best);
    TransmitterEnvironment::scheduler = nullptr;
    bestTx->transmissionQueue.clear();
    m->OnDestroy();
}

TEST_CASE("WP-39 CommNetVessel unloaded vessel: UpdateComm over the ProtoVessel snapshots and the proto module flags") {
    if (!GameAvailable()) return;
    Game15 g;
    World39 w;
    LoadLocalizer39();
    // an unloaded vessel owns a ProtoVessel whose part snapshots point at the prefabs (partInfo.partPrefab)
    ConfigNode vesselNode;
    ProtoVessel pv(vesselNode, &w.game);
    auto dishPart = g.New("mediumDishAntenna");
    pv.protoPartSnapshots.push_back(std::make_unique<ProtoPartSnapshot>(*dishPart, &pv));
    REQUIRE(pv.protoPartSnapshots[0]->partInfo != nullptr);
    Vessel v;
    v.loaded = false;
    v.vesselType = VesselType::Ship;
    v.protoVessel = &pv;
    v.vesselName = "Unloaded";
    auto m = NewCommNetVessel(v);
    m->Start();
    CHECK(m->unloadedDoOnce);
    m->unloadedDoOnce = false;
    m->OnNetworkPreUpdate();   // unloaded, nothing forced: the node keeps its (zero) powers
    CHECK(m->Comm()->antennaTransmit.power == 0.0);
    CHECK_FALSE(m->InPlasma());
    m->overridePreUpdate = true;
    m->OnNetworkPreUpdate();
    const AvailablePart& dish = g.Info("mediumDishAntenna");
    ModuleDataTransmitter* tx = nullptr;
    for (const PartModuleData& d : dish.partPrefab->ModuleData)
        if (auto* x = dynamic_cast<ModuleDataTransmitter*>(d.module.get())) tx = x;
    REQUIRE(tx != nullptr);
    const double p = tx->antennaPower();
    const bool relay = tx->antennaType() == AntennaType::RELAY;
    CHECK((relay ? m->Comm()->antennaRelay.power : m->Comm()->antennaTransmit.power) == p * static_cast<double>(w.Params().rangeModifier));
    m->OnDestroy();
}

// ============================================================ modules

TEST_CASE("WP-39 every stock ModuleDataTransmitter is the real class with its cfg values (reference parsed from the same cfgs)") {
    if (!GameAvailable()) return;
    Game15 g;
    World39 w;
    LoadLocalizer39();
    std::map<std::string, std::vector<json>> byPart;
    for (const json& a : Ref39()["antennas"]) byPart[a["part"].get<std::string>()].push_back(a);
    int found = 0;
    for (const auto& apu : g.data.parts.loadedParts) {
        const AvailablePart* ap = apu.get();
        if (!ap->partPrefab) continue;
        size_t idx = 0;
        for (const PartModuleData& d : ap->partPrefab->ModuleData) {
            auto* tx = dynamic_cast<ModuleDataTransmitter*>(d.module.get());
            if (tx == nullptr) continue;
            INFO(ap->name);
            REQUIRE(byPart.count(ap->name) == 1);
            const json& a = byPart[ap->name].at(idx++);
            found++;
            const std::string type = a["type"];
            CHECK(tx->antennaType() == (type == "INTERNAL" ? AntennaType::INTERNAL : (type == "DIRECT" ? AntennaType::DIRECT : AntennaType::RELAY)));
            CHECK(tx->antennaPower() == a["power"].get<double>());
            CHECK(tx->antennaCombinable() == a["combinable"].get<bool>());
            CHECK(tx->antennaCombinableExponent() == a["exponent"].get<double>());
            CHECK(tx->packetInterval() == static_cast<float>(a["packetInterval"].get<double>()));
            CHECK(tx->packetSize() == static_cast<float>(a["packetSize"].get<double>()));
            CHECK(tx->packetResourceCost() == a["packetResourceCost"].get<double>());
            CHECK(tx->DeployFxModuleIndices.has_value() == a["hasDeployFx"].get<bool>());
            // OnAwake curves: range (0,0)-(1,1) and the science curve
            REQUIRE(tx->CommRangeCurve() != nullptr);
            CHECK(tx->CommRangeCurve()->Evaluate(0.5) == DefaultRangeCurve().Evaluate(0.5));
            REQUIRE(tx->CommScienceCurve() != nullptr);
            CHECK(tx->CommScienceCurve()->keys.size() == 4);
            CHECK(tx->CommPower() == tx->antennaPower());
            CHECK(tx->EvaluateScienceMultiplier(1.0) == 0.4000000059604645);
            CHECK(tx->EvaluateScienceMultiplier(0.0) == 0.0);
            // the instance keeps the serialized member lists
            auto inst = g.New(ap->name.c_str());
            ModuleDataTransmitter* it = Find<ModuleDataTransmitter>(*inst, idx - 1);
            REQUIRE(it != nullptr);
            CHECK(it->DeployFxModuleIndices == tx->DeployFxModuleIndices);
            CHECK(it->CommPower() == tx->antennaPower());
            CHECK(!it->Get<std::string>("powerText").empty());
        }
    }
    CHECK(found == static_cast<int>(Ref39()["antennas"].size()));
    CHECK(found == 36);
}

TEST_CASE("WP-39 ModuleDataTransmitter: DataRate, DataResourceCost, CanTransmit, CanComm and the unloaded flags") {
    if (!GameAvailable()) return;
    Game15 g;
    World39 w;
    LoadLocalizer39();
    std::map<std::string, json> trans;
    for (const json& r : Ref39()["transmit"])
        if (!trans.count(r["part"].get<std::string>())) trans[r["part"].get<std::string>()] = r;
    Tally t;
    for (const auto& kv : trans) {
        auto part = g.New(kv.first.c_str());
        ModuleDataTransmitter* tx = Find<ModuleDataTransmitter>(*part);
        REQUIRE(tx != nullptr);
        t.D(static_cast<double>(tx->DataRate()), kv.second["rate"]);
        t.D(tx->DataResourceCost(), kv.second["dataResourceCost"]);
        // no CommNet vessel: CanTransmit follows the module state; an INTERNAL type never transmits
        CHECK(tx->CanTransmit() == (tx->antennaType() != AntennaType::INTERNAL));
        tx->isEnabled = false;
        CHECK_FALSE(tx->CanTransmit());
        tx->isEnabled = true;
    }
    CHECK(t.maxUlp == 0);
    // CanScienceTo: combinable antennas that are combined always reach; otherwise the range model decides (sqrDistance <= power * bPower)
    auto part = g.New("longAntenna");
    ModuleDataTransmitter* tx = Find<ModuleDataTransmitter>(*part);
    REQUIRE(tx != nullptr);
    const double power = tx->CommPower();
    CHECK(tx->CanScienceTo(false, 1e9, power * 1e9));
    CHECK_FALSE(tx->CanScienceTo(false, 1e9, power * 1e9 * 1.0000001));
    CHECK(tx->CanScienceTo(tx->antennaCombinable(), 1e9, power * 1e9 * 2.0) == tx->antennaCombinable());
    // CanComm / CanCommUnloaded
    CHECK(tx->CanComm());
    ConfigNode::NodePtr nodeOff = ConfigNode::Parse("MODULE\n{\n name = ModuleDataTransmitter\n canComm = False\n}\n")->nodes().Shared(0);
    ConfigNode::NodePtr nodeOn = ConfigNode::Parse("MODULE\n{\n name = ModuleDataTransmitter\n canComm = True\n}\n")->nodes().Shared(0);
    ProtoPartModuleSnapshot off(*nodeOff), on(*nodeOn);
    CHECK_FALSE(tx->CanCommUnloaded(&off));
    CHECK(tx->CanCommUnloaded(&on));
    CHECK(tx->CanCommUnloaded(nullptr));
    CHECK(tx->CommPowerUnloaded(&on) == power);
    // adjusters (Missions): a broken transmitter does not communicate; the power is adjusted in order
    struct Broken final : IDataTransmitterAdjuster {
        bool broken;
        double scale;
        Broken(bool b, double s) : broken(b), scale(s) {}
        double ApplyPowerAdjustment(double p) override { return p * scale; }
        bool IsDataTransmitterBroken() override { return broken; }
    } half(false, 0.5), bad(true, 1.0);
    tx->OnModuleAdjusterAdded(&half);
    CHECK(tx->CommPower() == power * 0.5);
    CHECK(tx->CanComm());
    tx->OnModuleAdjusterAdded(&bad);
    CHECK_FALSE(tx->CanComm());
    tx->OnModuleAdjusterRemoved(&bad);
    tx->OnModuleAdjusterRemoved(&half);
    CHECK(tx->CommPower() == power);
    // OnSave in the flight scene writes canComm
    ConfigNode n;
    SaveEnvironment::Current().loadedScene = GameScenes::FLIGHT;
    tx->OnSave(n);
    CHECK(n.GetValue("canComm") == std::string("True"));
    SaveEnvironment::Current().loadedScene = GameScenes::MAINMENU;
}

TEST_CASE("WP-39 ModuleDataTransmitter.transmitQueuedData: packet count, packet sizes, EC drawn and steps per packet over the stock antennas (reference)") {
    if (!GameAvailable()) return;
    Game15 g;
    World39 w;
    LoadLocalizer39();
    struct Stream final : ICommsStream {
        std::vector<float>* sent;
        std::vector<int>* frames;
        int* frame;
        void StreamData(float amount, ProtoVessel*) override {
            sent->push_back(amount);
            frames->push_back(*frame);
        }
    };
    long runs = 0, packetsTotal = 0;
    Time::fixedDeltaTime = 0.02f;
    TimeWarp::fetch = nullptr;
    for (const json& r : Ref39()["transmit"]) {
        auto antenna = g.New(r["part"].get<std::string>().c_str());
        auto battery = g.New("batteryBank");
        PartResource* ec = battery->GetResource("ElectricCharge");
        REQUIRE(ec != nullptr);
        ec->amount = ec->maxAmount;
        battery->ResetSimulationResources();
        Ship ship({antenna.get(), battery.get()}, true);
        ModuleDataTransmitter* tx = Find<ModuleDataTransmitter>(*antenna);
        REQUIRE(tx != nullptr);
        tx->deployFxModules = std::vector<IScalarModule*>{};   // no animation (engine): CanSetFXModules is true, nothing to move
        CoroutineScheduler sched;
        TransmitterEnvironment::scheduler = &sched;
        TransmitterEnvironment::screenMessages.clear();
        std::vector<float> sent;
        std::vector<int> frames;
        int frame = 0;
        TransmitterEnvironment::createCommsStream = [&](const TransmissionItem&, float, float, bool) {
            auto s = std::make_unique<Stream>();
            s->sent = &sent;
            s->frames = &frames;
            s->frame = &frame;
            return std::unique_ptr<ICommsStream>(std::move(s));
        };
        TransmissionItem item;
        item.dataAmount = static_cast<float>(r["amount"].get<double>());
        item.subjectID = "crewReport@KerbinSrfLanded";
        auto totalEC = [&] {
            double sum = 0.0;
            for (Part* p : ship.parts)
                if (PartResource* r = p->GetResource("ElectricCharge")) sum += r->amount;
            return sum;
        };
        const double before = totalEC();
        tx->TransmitData({item});
        CHECK(tx->IsBusy());
        while (tx->IsBusy() && frame < 200000) {
            sched.AdvanceFrame(0.02f);
            sched.RunDelayedFixedFrameRate();
            sched.RunDelayedDynamicFrameRate();
            frame++;
        }
        REQUIRE(!tx->IsBusy());
        const json& expSent = r["sent"];
        REQUIRE(sent.size() == expSent.size());
        for (size_t i = 0; i < sent.size(); i++) CHECK(sent[i] == static_cast<float>(expSent[i].get<double>()));
        CHECK(static_cast<int>(sent.size()) == r["packets"].get<int>());
        // EC: packets x packetResourceCost (the capacitor is charged once per packet)
        const double used = before - totalEC();
        const double expectEc = r["ec"];
        CHECK(std::abs(used - expectEc) <= 1e-9 * expectEc);
        // the gap between packets: WaitForFixedSeconds(packetInterval) with the float curTime += fixedDeltaTime accumulation. The first gap of a run is
        // one step shorter: the scheduler resumes the first WaitForFixedUpdate in the pass that started it (coroutine.hpp:16, the routine was started
        // from outside a fixed step); every later gap is exactly the reference count.
        for (size_t i = 1; i < frames.size(); i++) CHECK(frames[i] - frames[i - 1] == r["stepsPerPacket"].get<int>() - (i == 1 ? 1 : 0));
        CHECK_FALSE(tx->xmitAborted());
        CHECK(tx->Get<std::string>("statusText") == Localizer::Format("#autoLOC_236147"));
        // the first screen message is the "Transmitting" status; the last status is the completion message
        REQUIRE(TransmitterEnvironment::screenMessages.size() >= 2);
        runs++;
        packetsTotal += static_cast<long>(sent.size());
        TransmitterEnvironment::scheduler = nullptr;
        TransmitterEnvironment::createCommsStream = nullptr;
    }
    CHECK(runs == static_cast<long>(Ref39()["transmit"].size()));
    CHECK(runs == 88);
    INFO(runs << " transmissions, " << packetsTotal << " packets");
    CHECK(packetsTotal > 300);
}

TEST_CASE("WP-39 ModuleDataTransmitter: abort (StopTransmission), insufficient charge with xmitIncomplete, TransmitIncompleteToggle") {
    if (!GameAvailable()) return;
    Game15 g;
    World39 w;
    LoadLocalizer39();
    Time::fixedDeltaTime = 0.02f;
    TimeWarp::fetch = nullptr;
    auto antenna = g.New("HighGainAntenna5.v2");
    auto battery = g.New("batteryBank");
    PartResource* ec = battery->GetResource("ElectricCharge");
    ec->amount = 0.0;
    battery->ResetSimulationResources();
    Ship ship({antenna.get(), battery.get()}, true);
    ModuleDataTransmitter* tx = Find<ModuleDataTransmitter>(*antenna);
    REQUIRE(tx != nullptr);
    tx->deployFxModules = std::vector<IScalarModule*>{};
    CoroutineScheduler sched;
    TransmitterEnvironment::scheduler = &sched;
    std::vector<TransmissionItem> returned;
    TransmitterEnvironment::returnData = [&](ModuleDataTransmitter&, const TransmissionItem& d) { returned.push_back(d); };
    TransmissionItem item;
    item.dataAmount = 4.0f;
    // no charge, xmitIncomplete false: the first failed packet aborts the transmission and the data goes back to its container
    tx->TransmitData({item});
    int frame = 0;
    while (tx->IsBusy() && frame < 5000) {
        sched.AdvanceFrame(0.02f);
        sched.RunDelayedFixedFrameRate();
        sched.RunDelayedDynamicFrameRate();
        frame++;
    }
    REQUIRE(!tx->IsBusy());
    REQUIRE(returned.size() == 1);
    CHECK(returned[0].dataAmount == 4.0f);
    CHECK_FALSE(tx->xmitAborted());   // cleared at the end of the coroutine
    // TransmitIncompleteToggle flips the persistent flag and the event label
    CHECK_FALSE(tx->xmitIncomplete());
    tx->TransmitIncompleteToggle();
    CHECK(tx->xmitIncomplete());
    CHECK(tx->Events()["TransmitIncompleteToggle"]->guiName() == Localizer::Format("#autoLOC_7001224"));
    tx->TransmitIncompleteToggle();
    CHECK_FALSE(tx->xmitIncomplete());
    CHECK(tx->Events()["TransmitIncompleteToggle"]->guiName() == Localizer::Format("#autoLOC_7001225"));
    // StopTransmission mid-flight: charge present, abort after the first packet
    ec->amount = ec->maxAmount;
    battery->ResetSimulationResources();
    returned.clear();
    tx->TransmitData({item});
    sched.AdvanceFrame(0.02f);
    sched.RunDelayedFixedFrameRate();
    tx->StopTransmission();
    CHECK(tx->xmitAborted());
    frame = 0;
    while (tx->IsBusy() && frame < 5000) {
        sched.AdvanceFrame(0.02f);
        sched.RunDelayedFixedFrameRate();
        sched.RunDelayedDynamicFrameRate();
        frame++;
    }
    REQUIRE(!tx->IsBusy());
    CHECK(returned.size() == 1);
    TransmitterEnvironment::scheduler = nullptr;
}

TEST_CASE("WP-39 ModuleRelayEnabler / ModuleProbeControlPoint: unloaded flags, adjusters, the stock probe control points") {
    if (!GameAvailable()) return;
    Game15 g;
    World39 w;
    LoadLocalizer39();
    auto re = NewModule("ModuleRelayEnabler");
    auto* relay = static_cast<ModuleRelayEnabler*>(re.get());
    CHECK(relay->CanRelay());
    ConfigNode::NodePtr off = ConfigNode::Parse("MODULE\n{\n name = ModuleRelayEnabler\n canRelay = False\n}\n")->nodes().Shared(0);
    ConfigNode::NodePtr on = ConfigNode::Parse("MODULE\n{\n name = ModuleRelayEnabler\n canRelay = True\n}\n")->nodes().Shared(0);
    ConfigNode::NodePtr none = ConfigNode::Parse("MODULE\n{\n name = ModuleRelayEnabler\n}\n")->nodes().Shared(0);
    ProtoPartModuleSnapshot sOff(*off), sOn(*on), sNone(*none);
    CHECK_FALSE(relay->CanRelayUnloaded(&sOff));
    CHECK(relay->CanRelayUnloaded(&sOn));
    CHECK(relay->CanRelayUnloaded(&sNone));
    CHECK(relay->CanRelayUnloaded(nullptr));
    relay->isRelaying = false;
    ConfigNode saved;
    relay->OnSave(saved);
    CHECK(saved.GetValue("canRelay") == std::string("False"));
    // the stock probe control points
    int cores = 0;
    for (const auto& apu : g.data.parts.loadedParts) {
        const AvailablePart* ap = apu.get();
        if (!ap->partPrefab) continue;
        for (const PartModuleData& d : ap->partPrefab->ModuleData) {
            auto* pcp = dynamic_cast<ModuleProbeControlPoint*>(d.module.get());
            if (pcp == nullptr) continue;
            cores++;
            INFO(ap->name);
            const std::optional<std::string> mc = d.config->GetValue("minimumCrew");
            CHECK(pcp->minimumCrew() == (mc ? std::stoi(*mc) : 1));
            const std::optional<std::string> mh = d.config->GetValue("multiHop");
            CHECK(pcp->multiHop() == (mh && (*mh == "True" || *mh == "true")));
            CHECK(pcp->CanControl());
            CHECK(!pcp->GetInfo().empty());
        }
    }
    CHECK(cores == 5);
    struct Bad final : IProbeControlPointAdjuster {
        bool IsProbeControlPointBroken() override { return true; }
    } bad;
    auto pm = NewModule("ModuleProbeControlPoint");
    auto* pcp = static_cast<ModuleProbeControlPoint*>(pm.get());
    CHECK(pcp->CanControl());
    pcp->OnModuleAdjusterAdded(&bad);
    CHECK_FALSE(pcp->CanControl());
    // the unloaded path indexes the module's own cache (sic): one adjuster in the list, one in the cache
    CommNetModuleEnvironment::protoProbeAdjusters = [&](ProtoPartModuleSnapshot&) { return std::vector<IProbeControlPointAdjuster*>{&bad}; };
    ProtoPartModuleSnapshot snap(*none);
    CHECK_FALSE(pcp->CanControlUnloaded(&snap));
    pcp->OnModuleAdjusterRemoved(&bad);
    CHECK(pcp->CanControl());
    CHECK(pcp->CanControlUnloaded(nullptr));
}

TEST_CASE("WP-39 ModuleKerbNetAccess: FoV bounds, enhanced situations, anomaly chance and required effects on every stock KerbNet part (reference)") {
    if (!GameAvailable()) return;
    Game15 g;
    World39 w;
    LoadLocalizer39();
    std::map<std::string, std::vector<json>> byPart;
    for (const json& a : Ref39()["kerbnet"]) byPart[a["part"].get<std::string>()].push_back(a);
    int found = 0;
    long rows = 0;
    for (const auto& apu : g.data.parts.loadedParts) {
        const AvailablePart* ap = apu.get();
        if (!ap->partPrefab) continue;
        size_t idx = 0;
        for (const PartModuleData& d : ap->partPrefab->ModuleData) {
            if (dynamic_cast<ModuleKerbNetAccess*>(d.module.get()) == nullptr) continue;
            INFO(ap->name);
            REQUIRE(byPart.count(ap->name) == 1);
            const json& ref = byPart[ap->name].at(idx);
            found++;
            auto part = g.New(ap->name.c_str());
            ModuleKerbNetAccess* kn = Find<ModuleKerbNetAccess>(*part, idx);
            idx++;
            REQUIRE(kn != nullptr);
            CHECK(kn->MinimumFoV() == static_cast<float>(ref["MinimumFoV"].get<double>()));
            CHECK(kn->MaximumFoV() == static_cast<float>(ref["MaximumFoV"].get<double>()));
            CHECK(kn->EnhancedMinimumFoV() == static_cast<float>(ref["EnhancedMinimumFoV"].get<double>()));
            CHECK(kn->EnhancedMaximumFoV() == static_cast<float>(ref["EnhancedMaximumFoV"].get<double>()));
            CHECK(kn->EnhancedSituationMask() == ref["EnhancedSituationMask"].get<uint32_t>());
            CHECK(kn->AnomalyDetection() == static_cast<float>(ref["AnomalyDetection"].get<double>()));
            CHECK(kn->GetKerbNetAnomalyChance() == kn->AnomalyDetection());
            CHECK(kn->RequiresAnimation() == ref["RequiresAnimation"].get<bool>());
            CHECK(kn->GetKerbNetDisplayModes() == ref["modes"].get<std::vector<std::string>>());
            CHECK(kn->effects == ref["effects"].get<std::vector<std::string>>());
            CHECK(kn->isCheckingEffects == !ref["effects"].empty());
            // the enhanced situation list (Enum.GetValues order)
            std::vector<int> es;
            for (VesselSituations s : kn->GetEnhancedSituations()) es.push_back(static_cast<int>(s));
            CHECK(es == ref["enhancedSituations"].get<std::vector<int>>());
            CHECK(!kn->GetInfo().empty());
            for (const json& row : ref["rows"]) {
                bool pushed = false;
                float pmin = 0.0f, pmax = 0.0f;
                KerbNetEnvironment::hasThisDialogOpen = [](ModuleKerbNetAccess&) { return true; };
                KerbNetEnvironment::setFoVBounds = [&](float mn, float mx) {
                    pushed = true;
                    pmin = mn;
                    pmax = mx;
                };
                kn->isEnhanced = row["prevEnhanced"];
                kn->CheckVesselSituationFoV(static_cast<VesselSituations>(row["situation"].get<int>()), row["firstRun"]);
                CHECK(kn->isEnhanced == row["enhanced"].get<bool>());
                CHECK(pushed == row["boundsSet"].get<bool>());
                if (pushed) {
                    CHECK(pmin == static_cast<float>(row["min"].get<double>()));
                    CHECK(pmax == static_cast<float>(row["max"].get<double>()));
                }
                CHECK(kn->GetKerbNetMinimumFoV() == static_cast<float>(row["getMin"].get<double>()));
                CHECK(kn->GetKerbNetMaximumFoV() == static_cast<float>(row["getMax"].get<double>()));
                rows++;
            }
            KerbNetEnvironment::Reset();
            // error state: no crew with the required effect, or a part that is not animating
            if (kn->isCheckingEffects) {
                kn->partHasEffects = false;
                CHECK(!kn->GetKerbNetErrorState().empty());
                kn->partHasEffects = true;
                CHECK(kn->GetKerbNetErrorState().empty());
            }
        }
    }
    CHECK(found == static_cast<int>(Ref39()["kerbnet"].size()));
    CHECK(found == 17);
    CHECK(rows == 17 * 32);
}

TEST_CASE("WP-39 ModuleKerbNetAccess: OnStart subscribes situation changes, crew effects and the animation group") {
    if (!GameAvailable()) return;
    Game15 g;
    World39 w;
    LoadLocalizer39();
    // the first stock KerbNet part with an enhanced situation mask
    std::unique_ptr<Part> part;
    for (const auto& apu : g.data.parts.loadedParts) {
        if (!apu->partPrefab) continue;
        for (const PartModuleData& d : apu->partPrefab->ModuleData)
            if (auto* k = dynamic_cast<ModuleKerbNetAccess*>(d.module.get()))
                if (k->EnhancedSituationMask() != 0 && !part) part = g.New(apu->name.c_str());
        if (part) break;
    }
    REQUIRE(part != nullptr);
    ModuleKerbNetAccess* kn = Find<ModuleKerbNetAccess>(*part);
    REQUIRE(kn != nullptr);
    Ship ship({part.get()});
    ship.v->situation = VesselSituations::LANDED;
    kn->OnStart(StartState::Landed);
    CHECK_FALSE(kn->isEnhanced);
    // the mask names the enhanced situations; a change event for this vessel re-evaluates the FoV
    const std::vector<VesselSituations> enh = kn->GetEnhancedSituations();
    REQUIRE(!enh.empty());
    ship.v->situation = enh[0];
    GameEvents::onVesselSituationChange.Fire(GameEvents::HostedFromToAction<Vessel*, Vessel_Situations>(ship.v.get(), VesselSituations::LANDED, enh[0]));
    CHECK(kn->isEnhanced);
    CHECK(kn->GetKerbNetMinimumFoV() == kn->EnhancedMinimumFoV());
    // another vessel's change does not matter
    Vessel other;
    GameEvents::onVesselSituationChange.Fire(GameEvents::HostedFromToAction<Vessel*, Vessel_Situations>(&other, enh[0], VesselSituations::LANDED));
    CHECK(kn->isEnhanced);
    ship.v->situation = VesselSituations::LANDED;
    GameEvents::onVesselSituationChange.Fire(GameEvents::HostedFromToAction<Vessel*, Vessel_Situations>(ship.v.get(), enh[0], VesselSituations::LANDED));
    CHECK(kn->isEnhanced == kn->SituationIsEnhanced(VesselSituations::LANDED));
    // the Invoke timer of the crew effect check
    if (kn->isCheckingEffects) {
        GameEvents::onVesselCrewWasModified.Fire(ship.v.get());
        CHECK(kn->IsInvoking());
        kn->RunInvoke();
        CHECK_FALSE(kn->IsInvoking());
        CHECK_FALSE(kn->partHasEffects);  // nobody aboard
    }
    kn->OnDestroy();
    // after OnDestroy the module no longer listens
    ship.v->situation = enh[0];
    const bool before = kn->isEnhanced;
    GameEvents::onVesselSituationChange.Fire(GameEvents::HostedFromToAction<Vessel*, Vessel_Situations>(ship.v.get(), VesselSituations::LANDED, enh[0]));
    CHECK(kn->isEnhanced == before);
}

TEST_CASE("WP-39 CommNet registration: the real classes replace the stand-ins and ModuleCommand registers its command source") {
    if (!GameAvailable()) return;
    Game15 g;
    World39 w;
    LoadLocalizer39();
    auto pod = g.New("probeCoreOcto.v2");
    ModuleCommand* cmd = Find<ModuleCommand>(*pod);
    REQUIRE(cmd != nullptr);
    Ship ship({pod.get()});
    auto m = NewCommNetVessel(*ship.v);
    m->Start();
    // ModuleCommand.Start in the flight scene: registers on OnNetworkInitialized (vessel.connection exists)
    SaveEnvironment::Current().loadedScene = GameScenes::FLIGHT;
    SaveEnvironment::Current().loadedSceneIsGame = true;
    CommNetScenario scenario;
    scenario.Awake();
    scenario.Start();
    cmd->Start();
    CHECK(cmd->networkInitialised());
    REQUIRE(m->CommandSources().size() == 1);
    cmd->OnDestroy();
    CHECK(m->CommandSources().empty());
    scenario.OnDestroy();
    m->OnDestroy();
    SaveEnvironment::Current().loadedSceneIsGame = false;
    SaveEnvironment::Current().loadedScene = GameScenes::MAINMENU;
    // the module classes are the new implementations (the generic PartModule would not cast)
    for (const char* name : {"ModuleDataTransmitter", "ModuleKerbNetAccess", "ModuleRelayEnabler", "ModuleProbeControlPoint"}) {
        auto mod = NewModule(name);
        INFO(name);
        CHECK(std::string(typeid(*mod).name()).find("Generic") == std::string::npos);
    }
    CHECK(dynamic_cast<ModuleDataTransmitter*>(NewModule("ModuleDataTransmitter").get()) != nullptr);
    CHECK(dynamic_cast<ModuleKerbNetAccess*>(NewModule("ModuleKerbNetAccess").get()) != nullptr);
    CHECK(dynamic_cast<ModuleRelayEnabler*>(NewModule("ModuleRelayEnabler").get()) != nullptr);
    CHECK(dynamic_cast<ModuleProbeControlPoint*>(NewModule("ModuleProbeControlPoint").get()) != nullptr);
    CHECK(dynamic_cast<CommNetVessel*>(VesselModuleFactory::Create(*GetVesselModuleClass("CommNetVessel")).get()) != nullptr);
}
