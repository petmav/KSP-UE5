// WP-38 slice 3 unit tests: FinePrint.ContractDefs (full), Sentinel, CompatibilityUtilities, the pre-built craft model and the WP-37 deferred
// templates (core/include/ksp/fineprint/{contract_defs,sentinel,compatibility,prebuilt}.hpp).
//
// Expectations come from tools/wp38c/wp38c_reference.py (tests/data/wp38c_reference.json): an independent Python model that reads the decompiled C#
// and the mirror cfgs and does not import kspcore.
#include <filesystem>

#include "world38b.hpp"
#include "ksp/fineprint/contract_defs.hpp"
#include "ksp/fineprint/sentinel.hpp"

namespace FP3 = ksp::FinePrint;
namespace SM = ksp::SentinelMission;

namespace {

const json& Ref3() {
    static const json j = [] {
        std::ifstream f(std::string(KSPCORE_TEST_DATA_DIR) + "/wp38c_reference.json", std::ios::binary);
        REQUIRE(f.good());
        return json::parse(f);
    }();
    return j;
}

std::string ContractsCfg3() { return ksp::testing::GameRoot() + "/GameData/Squad/Contracts/Contracts.cfg"; }

// the encoded form the reference uses: float "%08x" of the bits, double "%016x", bool 0/1, int, string
std::string HexOf(uint64_t v, int digits) {
    char b[32];
    std::snprintf(b, sizeof b, "%0*llx", digits, static_cast<unsigned long long>(v));
    return b;
}
json Enc(const FP3::ContractDefs::Ptr& p) {
    if (auto* f = std::get_if<float*>(&p)) {
        uint32_t u;
        std::memcpy(&u, *f, 4);
        return HexOf(u, 8);
    }
    if (auto* d = std::get_if<double*>(&p)) {
        uint64_t u;
        std::memcpy(&u, *d, 8);
        return HexOf(u, 16);
    }
    if (auto* i = std::get_if<int*>(&p)) return **i;
    if (auto* b = std::get_if<bool*>(&p)) return static_cast<int>(**b);
    return *std::get<std::string*>(p);
}

// every member against one reference table ("defaults", "loadedShipped", ...); returns the number compared
size_t CompareAll(const json& table) {
    size_t n = 0;
    for (const auto& m : FP3::ContractDefs::AllMembers()) {
        REQUIRE(table.contains(m.path));
        CHECK_MESSAGE(Enc(m.member) == table.at(m.path), m.path, " got ", Enc(m.member).dump(), " want ", table.at(m.path).dump());
        ++n;
    }
    return n;
}

struct ContractDefsFixture {
    ContractDefsFixture() {
        SaveEnvironment::Current().SetStockExpansionInstalled("Serenity", false);
        Contracts::ContractDefsLite::Clear();
    }
    ~ContractDefsFixture() { Contracts::ContractDefsLite::Clear(); }
};

}  // namespace

TEST_CASE("WP-38c ContractDefs: the static initialisers are the C# defaults (460 members)") {
    ContractDefsFixture fx;
    const json& r = Ref3()["contractDefs"];
    FP3::ContractDefs::ResetToDefaults();
    CHECK(CompareAll(r["defaults"]) == 460);
    CHECK(FP3::ContractDefs::AllMembers().size() == r["defaults"].size());
}

TEST_CASE("WP-38c ContractDefs: LoadConfig over the shipped Contracts.cfg (Serenity off and on)") {
    ContractDefsFixture fx;
    const json& r = Ref3()["contractDefs"];
    Contracts::ContractDefsLite::Load(ContractsCfg3());
    CHECK(CompareAll(r["loadedShipped"]) == 460);
    SaveEnvironment::Current().SetStockExpansionInstalled("Serenity", true);
    Contracts::ContractDefsLite::Load(ContractsCfg3());
    CHECK(CompareAll(r["loadedShippedSerenity"]) == 460);
    SaveEnvironment::Current().SetStockExpansionInstalled("Serenity", false);
}

TEST_CASE("WP-38c ContractDefs: the (section, sub, key) registry covers what LoadConfig reads, and ContractDefsLite answers from it") {
    ContractDefsFixture fx;
    const json& r = Ref3()["contractDefs"];
    Contracts::ContractDefsLite::Load(ContractsCfg3());
    size_t n = 0;
    for (const json& e : r["registry"]) {
        const std::string sec = e[0], sub = e[1], key = e[2], path = e[3];
        const bool ser = e[4];
        if (ser) continue;
        const FP3::ContractDefs::Entry* en = FP3::ContractDefs::Find(sec, sub, key);
        REQUIRE_MESSAGE(en != nullptr, path);
        CHECK_MESSAGE(Enc(en->member) == r["loadedShipped"].at(path), path);
        // ContractDefsLite answers with the member's own type
        if (std::holds_alternative<float*>(en->member)) {
            uint32_t a, b;
            const float v = Contracts::ContractDefsLite::F(sec, sub, key);
            std::memcpy(&a, &v, 4);
            std::memcpy(&b, std::get<float*>(en->member), 4);
            CHECK(a == b);
        } else if (std::holds_alternative<int*>(en->member)) {
            CHECK(Contracts::ContractDefsLite::I(sec, sub, key) == *std::get<int*>(en->member));
        } else if (std::holds_alternative<bool*>(en->member)) {
            CHECK(Contracts::ContractDefsLite::B(sec, sub, key) == *std::get<bool*>(en->member));
        } else if (std::holds_alternative<double*>(en->member)) {
            CHECK(Contracts::ContractDefsLite::D(sec, sub, key) == *std::get<double*>(en->member));
        } else {
            CHECK(Contracts::ContractDefsLite::S(sec, sub, key) == *std::get<std::string*>(en->member));
        }
        ++n;
    }
    CHECK(n == 391);
}

TEST_CASE("WP-38c ContractDefs: CreateDefaultConfig writes the node the reference builds, and LoadConfig reads it back") {
    ContractDefsFixture fx;
    const json& r = Ref3()["contractDefs"];
    namespace fs = std::filesystem;
    const fs::path dir = fs::temp_directory_path() / "kspcore_wp38c_contractdefs";
    fs::remove_all(dir);
    fs::create_directories(dir / "GameData" / "Squad" / "Contracts");
    FP3::ContractDefs::ResetToDefaults();
    FP3::ContractDefs::applicationRootPath = dir.string() + "/";
    FP3::ContractDefs::CreateDefaultConfig();
    FP3::ContractDefs::applicationRootPath.clear();
    REQUIRE(FP3::ContractDefs::config);
    // the file Save wrote (the Contracts node and its children; UTF-8 without BOM, CRLF)
    std::string text;
    {
        std::ifstream f(dir / "GameData" / "Squad" / "Contracts" / "Contracts.cfg", std::ios::binary);
        REQUIRE(f.good());
        text.assign((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    }
    const std::string want = r["writerText"];
    CHECK(text.size() == want.size());
    CHECK(text == want);
    // the writer / reader round trip
    FP3::ContractDefs::ResetToDefaults();
    FP3::ContractDefs::LoadConfig();
    CHECK(CompareAll(r["roundTrip"]) == 460);
    fs::remove_all(dir);
}

TEST_CASE("WP-38c ContractDefs: Awake merges the Contracts nodes, loads, and LoadDefinitions localises the orbit names") {
    ContractDefsFixture fx;
    using L = KSP::Localization::Localizer;
    L::Shutdown();
    L::Init();
    ConfigNode::NodePtr root = ConfigNode::Load(ksp::testing::GameRoot() + "/GameData/Squad/Localization/dictionary.cfg");
    REQUIRE(root);
    std::vector<ConfigNode*> nodes{root->GetNode("Localization")};
    L::Instance()->AddTagValuesForLanguage(nodes, "en-us");
    ConfigNode::NodePtr file = ConfigNode::Load(ContractsCfg3());
    REQUIRE(file);
    FP3::ContractDefs::ResetToDefaults();
    FP3::ContractDefs::Awake({file->GetNode("Contracts")}, true);
    CHECK(FP3::ContractDefs::MolniyaName == L::Format("#autoLOC_7001203"));
    CHECK(FP3::ContractDefs::SunSynchronousName == L::Format("#autoLOC_7001208"));
    CHECK(FP3::ContractDefs::PolarOrbitName == L::Format("#autoLOC_7001202"));
    CHECK(!FP3::ContractDefs::MolniyaName.empty());
    // the numeric values are the cfg values
    CHECK(FP3::ContractDefs::WeightDefault == 30);
    CHECK(FP3::ContractDefs::Flag::Expire::DeadlineDays == 2130);
    L::Shutdown();
}

// ================================================================================================================================================
// Sentinel (SentinelMission/*.cs)
// ================================================================================================================================================
namespace {

const json& RefS() { return Ref3()["sentinel"]; }

// Mono float rule helpers are in world38b.hpp (CHECK_F32b / CHECK_F64b)
#define CHECK_F64n(got, ref) CHECK_MESSAGE(BitsD38b(static_cast<double>(got)) == BitsD38b(static_cast<double>(ref)), "got ", (got), " want ", (ref))

struct SentinelWorld : World38b {
    SentinelWorld() {
        SM::SentinelUtilities::Reset();
        SM::SentinelEnvironment::Reset();
        SM::SentinelModule::Register();
    }
    ~SentinelWorld() {
        SM::SentinelUtilities::Reset();
        SM::SentinelEnvironment::Reset();
        SM::SentinelScenario::Instance = nullptr;
    }
};

}  // namespace

TEST_CASE("WP-38c SentinelUtilities: inner / outer bodies, view angle, escape velocity, minimum orbital speed, solar orbit, read duration, random helpers vs the reference") {
    SentinelWorld w;
    const json& u = RefS()["util"];
    CelestialBody* inner = nullptr;
    CelestialBody* outer = nullptr;
    int errors = 0;
    for (const json& e : u["findInner"]) {
        const double sma = e["sma"].get<double>();
        if (e["result"].is_null()) {
            CHECK_THROWS_AS(SM::SentinelUtilities::FindInnerAndOuterBodies(sma, inner, outer), std::out_of_range);
            ++errors;
            continue;
        }
        INFO("sma " << sma);
        CHECK(SM::SentinelUtilities::FindInnerAndOuterBodies(sma, inner, outer) == e["result"][2].get<bool>());
        CHECK(inner->name() == e["result"][0].get<std::string>());
        CHECK(outer->name() == e["result"][1].get<std::string>());
    }
    CHECK(errors > 0);  // an SMA at or beyond the outermost planet indexes past the list (ArgumentOutOfRangeException in C#)
    int nan = 0;
    for (const json& e : u["viewAngle"]) {
        const double got = SM::SentinelUtilities::AdjustedSentinelViewAngle(*BodyOrbit(&w.Body(e["inner"].get<std::string>())), *BodyOrbit(&w.Body(e["outer"].get<std::string>())));
        if (e["angle"].is_null()) {
            CHECK(std::isnan(got));
            ++nan;
        } else {
            CHECK_F64n(got, e["angle"].get<double>());
        }
    }
    CHECK(nan > 0);
    for (const json& e : u["escape"]) CHECK_F64n(SM::SentinelUtilities::GetEscapeVelocity(&w.Body(e["body"].get<std::string>()), e["altitude"].get<double>()), e["v"].get<double>());
    for (const json& e : u["minSpeed"]) {
        if (e["body"] == "Sun" || e["v"].is_null()) continue;
        CHECK_F64n(SM::SentinelUtilities::GetMinimumOrbitalSpeed(&w.Body(e["body"].get<std::string>())), e["v"].get<double>());
    }
    CHECK_F64n(SM::SentinelUtilities::GetMinimumOrbitalSpeed(&w.Body("Sun")), [&] {
        for (const json& e : u["minSpeed"]) if (e["body"] == "Sun") return e["v"].get<double>();
        return 0.0;
    }());
    for (const json& e : u["solarOrbit"]) CHECK(SM::SentinelUtilities::IsOnSolarOrbit(&w.Body(e["body"].get<std::string>())) == e["v"].get<bool>());
    for (const json& e : u["readDuration"]) CHECK_F32b(SM::SentinelUtilities::CalculateReadDuration(e["text"].get<std::string>()), F38b(e["v"]));
    // the random helpers consume the generator exactly like the reference model
    for (const json& r : u["rng"]) {
        KSPRandom g(r["seed"].get<int>());
        for (const json& x : r["scan"]) CHECK(static_cast<int>(SM::SentinelUtilities::RandomScanType(&g)) == x.get<int>());
        for (const json& x : r["class"]) CHECK(static_cast<int>(SM::SentinelUtilities::WeightedAsteroidClass(&g)) == x.get<int>());
        SM::SentinelEnvironment::discoverableObjects = SM::SentinelEnvironment::DiscoverableObjects{UntrackedObjectClass::C, UntrackedObjectClass::F, 1.0f, 2.0f, 0};
        for (const json& x : r["classLimited"]) CHECK(static_cast<int>(SM::SentinelUtilities::WeightedAsteroidClass(&g)) == x.get<int>());
        SM::SentinelEnvironment::discoverableObjects.reset();
        for (const json& x : r["wr"]) CHECK_F64n(SM::SentinelUtilities::WeightedRandom(&g), x.get<double>());
        for (const json& x : r["wrRange"]) CHECK_F64n(SM::SentinelUtilities::WeightedRandom(&g, 0.05, 0.4), x.get<double>());
        for (const json& x : r["wrMax"]) CHECK_F64n(SM::SentinelUtilities::WeightedRandom(&g, 12.5), x.get<double>());
        for (const json& x : r["range"]) CHECK_F64n(SM::SentinelUtilities::RandomRange(&g, 1.05, 1.2), x.get<double>());
        for (const json& x : r["rangeSwapped"]) CHECK_F64n(SM::SentinelUtilities::RandomRange(&g, 7.0, -3.0), x.get<double>());
    }
    // a null generator is a fresh KSPRandom seeded by the host seam (ContractsEnvironment::newRandomSeed = 4242 in the test world)
    {
        KSPRandom fresh(4242);
        CHECK_F64n(SM::SentinelUtilities::RandomRange(nullptr, 0.0, 10.0), fresh.NextDouble() * 10.0);
    }
    // the public settings
    CHECK(SM::SentinelUtilities::SentinelPartName == "InfraredTelescope");
    CHECK(SM::SentinelUtilities::SentinelModuleName == "SentinelModule");
    CHECK(SM::SentinelUtilities::perSentinelObjectLimit == 10);
    CHECK_F32b(SM::SentinelUtilities::SpawnChance, 0.25f);
    CHECK(SM::SentinelUtilities::SentinelPartTitle() == RefS()["partTitle"].get<std::string>());
}

TEST_CASE("WP-38c SentinelParameter: titles for every scan type / focus body / size / eccentricity / inclination, notes, node format, discoveries") {
    SentinelWorld w;
    const json& t = RefS()["paramTexts"];
    auto holder = std::make_unique<C38::ARMContract>();
    holder->targetBody = Planetarium::fetch->Home;
    holder->agent = w.agents->GetAgent("Kerbodyne");
    size_t n = 0;
    for (const json& e : t["rows"]) {
        auto p = std::make_unique<SM::SentinelParameter>(&w.Body(e["focus"].get<std::string>()), static_cast<SM::SentinelScanType>(e["scan"].get<int>()),
                                                         static_cast<UntrackedObjectClass>(e["size"].get<int>()), e["ecc"].get<double>(), e["inc"].get<double>(),
                                                         e["count"].get<int>());
        CHECK_MESSAGE(p->GetTitle() == e["title"].get<std::string>(), e.dump());
        ++n;
    }
    CHECK(n == 5 * 4 * 3 * 3 * 5);
    SM::SentinelParameter p;
    {
        SceneScope sc(GameScenes::FLIGHT);
        CHECK(p.GetNotes() == t["notes"].get<std::string>());
    }
    {
        SceneScope sc(GameScenes::SPACECENTER);
        CHECK(p.GetNotes() == t["notesSpaceCenter"].get<std::string>());
    }
    // the default constructor (SentinelParameter.cs:27): home focus, no scan type, the largest class, 3 discoveries
    CHECK(p.FocusBody == Planetarium::fetch->Home);
    CHECK(p.ScanType == SM::SentinelScanType::NONE);
    CHECK(p.TargetSize == UntrackedObjectClass::I);
    CHECK(p.TotalDiscoveries == 3);
    CHECK(p.RemainingDiscoveries == 3);
    SM::SentinelEnvironment::discoverableObjects = SM::SentinelEnvironment::DiscoverableObjects{UntrackedObjectClass::B, UntrackedObjectClass::E, 1.0f, 2.0f, 0};
    SM::SentinelParameter limited;
    CHECK(limited.TargetSize == UntrackedObjectClass::E);
    SM::SentinelEnvironment::discoverableObjects.reset();
    // node format and round trip (SentinelParameter.cs:85 / :96): enums by name, doubles "G17"
    SM::SentinelParameter q(&w.Body("Duna"), SM::SentinelScanType::ECCENTRICITY, UntrackedObjectClass::D, 0.25, 12.5, 7);
    q.RemainingDiscoveries = 4;
    ConfigNode a("PARAM");
    q.OnSave(a);
    CHECK(*a.GetValue("FocusBody") == dotnet::FormatInt64(w.Body("Duna").flightGlobalsIndex));
    CHECK(*a.GetValue("ScanType") == "ECCENTRICITY");
    CHECK(*a.GetValue("TargetSize") == "D");
    CHECK(*a.GetValue("MinimumEccentricity") == "0.25");
    CHECK(*a.GetValue("MinimumInclination") == "12.5");
    CHECK(*a.GetValue("TotalDiscoveries") == "7");
    CHECK(*a.GetValue("RemainingDiscoveries") == "4");
    RoundTrip(q, "SentinelParameter");
    // loading from an empty node keeps the C# defaults of OnLoad (largest class, 3 discoveries, home focus)
    SM::SentinelParameter r(&w.Body("Eve"), SM::SentinelScanType::CLASS, UntrackedObjectClass::A, 1.0, 2.0, 9);
    ConfigNode empty("PARAM");
    r.OnLoad(empty);
    CHECK(r.FocusBody == Planetarium::fetch->Home);
    CHECK(r.ScanType == SM::SentinelScanType::NONE);
    CHECK(r.TargetSize == UntrackedObjectClass::I);
    CHECK(r.TotalDiscoveries == 3);
    CHECK(r.RemainingDiscoveries == 3);
    CHECK_F64n(r.MinimumEccentricity, 0.0);
}

namespace {

void CheckSentinelOrbitParam(const json& pr, ContractParameter* p) {
    auto* so = dynamic_cast<P38::SpecificOrbitParameter*>(p);
    REQUIRE(so != nullptr);
    const json& el = pr["orbit"];
    CHECK(so->orbitType == FP::OrbitType::RANDOM);
    CHECK_F64n(so->sma, el["sma"].get<double>());
    CHECK_F64n(so->eccentricity, el["ecc"].get<double>());
    CHECK_F64n(so->inclination, el["inc"].get<double>());
    CHECK_F64n(so->lan, el["lan"].get<double>());
    CHECK_F64n(so->argumentOfPeriapsis, el["argpe"].get<double>());
    CHECK_F64n(so->meanAnomalyAtEpoch, el["mep"].get<double>());
    CHECK_F64n(so->epoch, el["epoch"].get<double>());
    CHECK_F64n(so->deviationWindow, pr["deviation"].get<double>());
    CHECK(so->TargetBody == Planetarium::fetch->Sun);
}

}  // namespace

TEST_CASE("WP-38c SentinelContract / CometDetectionContract: generated trees (focus bodies, scan type, orbit draws, parameters, texts, rewards, agent, id) vs the reference (0 ULP, byte-exact)") {
    const json& ref = RefS();
    std::map<std::string, std::pair<int, int>> counts;
    std::map<int, int> scanTypes;
    int paramsChecked = 0;
    for (size_t si = 0; si < ref["scenarios"].size(); ++si) {
        SentinelWorld w;
        ContractSystem cs;
        cs.Awake();
        ConfigNode weights = WeightsNode38b(ref["weights"]);
        cs.LoadNow(weights);
        LoadScenario38b(w, ref["scenarios"][si]);
        int curMult = -1;
        for (const json& e : ref["contracts"]) {
            if (e["scenario"].get<size_t>() != si) continue;
            if (e["mult"].get<int>() != curMult) {
                curMult = e["mult"].get<int>();
                w.SetMult(Ref38b()["mult_sets"][static_cast<size_t>(curMult)]);
            }
            w.nextGuid = GuidOf38b(e["guid"]);
            const std::string type = e["type"].get<std::string>();
            INFO(type << " scenario " << ref["scenarios"][si]["name"] << " prestige " << e["prestige"] << " seed " << e["seed"] << " mult " << e["mult"]);
            std::unique_ptr<Contract> c = cs.GenerateContract(e["seed"].get<int>(), PrestigeOf38b(e["prestige"].get<int>()), type);
            const json& r = e["result"];
            if (r.is_null()) {
                INFO("unexpected contract: " << (c ? c->Title() : std::string()));
                CHECK(c == nullptr);
                ++counts[type].second;
                continue;
            }
            REQUIRE(c != nullptr);
            ++counts[type].first;
            CHECK(c->Agent()->Name() == r["agent"].get<std::string>());
            CHECK(c->Agent()->Title() == r["agentTitle"].get<std::string>());
            CHECK(c->Keywords() == r["keywords"].get<std::vector<std::string>>());
            CHECK(c->Title() == r["title"].get<std::string>());
            CHECK(c->Synopsys() == r["synopsis"].get<std::string>());
            CHECK(c->Description() == r["description"].get<std::string>());
            CHECK(c->MessageCompleted() == r["completed"].get<std::string>());
            CHECK(c->ContractID() == r["contractID"].get<long long>());
            CHECK(dotnet::EnumToString(c->Prestige()) == r["prestige"].get<std::string>());
            CHECK_F64b(c->FundsAdvance, r["fa"].get<double>());
            CHECK_F64b(c->FundsCompletion, r["fc"].get<double>());
            CHECK_F64b(c->FundsFailure, r["ff"].get<double>());
            CHECK_F32b(c->ReputationCompletion, F38b(r["rc"]));
            CHECK_F32b(c->ReputationFailure, F38b(r["rf"]));
            CHECK_F32b(c->ScienceCompletion, F38b(r["sc"]));
            CHECK_F64b(c->TimeExpiry, r["expiry"].get<double>());
            CHECK_F64b(c->TimeDeadline, r["deadline"].get<double>());
            CHECK(c->CanBeCancelled());
            CHECK(c->CanBeDeclined());
            CHECK(c->MissionSeed() == e["seed"].get<int>());
            REQUIRE(c->ParameterCount() == static_cast<int>(r["params"].size()));
            for (int i = 0; i < c->ParameterCount(); ++i) {
                ContractParameter* p = c->GetParameter(i);
                const json& pr = r["params"][static_cast<size_t>(i)];
                CHECK(p->TypeName() == pr["type"].get<std::string>());
                CHECK(p->HashString() == pr["hash"].get<std::string>());
                if (pr.contains("title")) CHECK(p->Title() == pr["title"].get<std::string>());
                if (pr["type"] == "SpecificOrbitParameter") CheckSentinelOrbitParam(pr, p);
                if (pr["type"] == "SentinelParameter") {
                    auto* sp = dynamic_cast<SM::SentinelParameter*>(p);
                    REQUIRE(sp != nullptr);
                    CHECK(sp->FocusBody->name() == r["outerBody"].get<std::string>());
                    CHECK(static_cast<int>(sp->ScanType) == r["scanType"].get<int>());
                    CHECK(static_cast<int>(sp->TargetSize) == r["targetSize"].get<int>());
                    CHECK_F64n(sp->MinimumEccentricity, r["minEcc"].get<double>());
                    CHECK_F64n(sp->MinimumInclination, r["minInc"].get<double>());
                    CHECK(sp->TotalDiscoveries == r["asteroidCount"].get<int>());
                    CHECK(sp->RemainingDiscoveries == r["asteroidCount"].get<int>());
                    ++scanTypes[r["scanType"].get<int>()];
                }
                if (pr["type"] == "EnterOrbit") CHECK(dynamic_cast<ksp::Contracts::Parameters::EnterOrbit*>(p) != nullptr);
                ++paramsChecked;
            }
            if (type == "SentinelContract") {
                auto* s = dynamic_cast<SM::SentinelContract*>(c.get());
                REQUIRE(s != nullptr);
                CHECK(s->innerBody->name() == r["innerBody"].get<std::string>());
                CHECK(s->outerBody->name() == r["outerBody"].get<std::string>());
                CHECK(s->asteroidCount == r["asteroidCount"].get<int>());
            } else {
                auto* s = dynamic_cast<SM::CometDetectionContract*>(c.get());
                REQUIRE(s != nullptr);
                CHECK(s->cometCount == r["cometCount"].get<int>());
            }
        }
    }
    MESSAGE("generated / refused: Sentinel " << counts["SentinelContract"].first << "/" << counts["SentinelContract"].second << ", CometDetection "
                                              << counts["CometDetectionContract"].first << "/" << counts["CometDetectionContract"].second << "; parameters checked " << paramsChecked);
    CHECK(counts["SentinelContract"].first > 0);
    CHECK(counts["SentinelContract"].second > 0);
    CHECK(counts["CometDetectionContract"].first > 0);
    CHECK(counts["CometDetectionContract"].second > 0);
    for (int st = 0; st < 4; ++st) CHECK(scanTypes[st] > 0);
}

// ---- behaviour: derived from the decompiled C# (each check names the member it follows) --------------------------------------------------------
namespace {

struct SentinelRig {
    SentinelWorld& w;
    VRig r;
    SM::SentinelModule* module = nullptr;
    explicit SentinelRig(SentinelWorld& world, std::initializer_list<const char*> parts = {"probeCoreOcto.v2", "InfraredTelescope", "HighGainAntenna", "solarPanels1"})
        : w(world), r(world, world.Body("Sun"), parts, Vessel::Situations::ORBITING) {
        r.v.id = Guid::Parse("11111111-2222-3333-4444-555555555555");
        FlightRegistry::Vessels = {&r.v};
        for (auto& p : r.parts) {
            PartModule* m = p->FindModuleImplementing([](PartModule& pm) { return dynamic_cast<SM::SentinelModule*>(&pm) != nullptr; });
            if (m != nullptr) module = dynamic_cast<SM::SentinelModule*>(m);
        }
    }
    ~SentinelRig() { FlightRegistry::Vessels.clear(); }
    Orbit& O() { return *r.v.orbitDriver->orbit; }
    void SetSolarOrbit(double inc, double ecc, double sma, double lan) { O().SetOrbit(inc, ecc, sma, lan, 0.0, 0.0, 0.0, &w.Body("Sun")); }
};

}  // namespace

TEST_CASE("WP-38c SentinelUtilities on vessels: FindInnerAndOuterBodies(Vessel / Orbit), SentinelCanScan (loaded and unloaded), GetVesselClass") {
    SentinelWorld w;
    SentinelRig s(w);
    CelestialBody* inner = nullptr;
    CelestialBody* outer = nullptr;
    // between Eve (sma 9.83e9) and Kerbin (1.36e10)
    s.SetSolarOrbit(0.0, 0.0, 1.2e10, 0.0);
    CHECK(SM::SentinelUtilities::FindInnerAndOuterBodies(s.r.v, inner, outer));
    CHECK(inner->name() == "Eve");
    CHECK(outer->name() == "Kerbin");
    CHECK(SM::SentinelUtilities::FindInnerAndOuterBodies(s.O(), inner, outer));
    CHECK(outer->name() == "Kerbin");
    // an orbit around Mun climbs to the planet it belongs to (Mun -> Kerbin): the SMA searched is Kerbin's own
    Orbit munOrbit(0.0, 0.0, 300000.0, 0.0, 0.0, 0.0, 0.0, &w.Body("Mun"));
    CHECK(SM::SentinelUtilities::FindInnerAndOuterBodies(munOrbit, inner, outer));
    CHECK(inner->name() == "Kerbin");  // SMA == Kerbin's: list[k] <= SMA and the next planet (Duna) is beyond
    CHECK(outer->name() == "Duna");
    // SentinelCanScan: Kerbin's orbit is circular and not inclined, so a circular equatorial heliocentric orbit passes
    CHECK(SM::SentinelUtilities::SentinelCanScan(s.r.v));
    CHECK(SM::SentinelUtilities::SentinelCanScan(s.r.v, inner, outer));
    s.SetSolarOrbit(0.0, 0.25, 1.2e10, 0.0);  // eccentricity differs by more than 0.2
    CHECK_FALSE(SM::SentinelUtilities::SentinelCanScan(s.r.v));
    s.SetSolarOrbit(0.0, 0.2, 1.2e10, 0.0);
    CHECK(SM::SentinelUtilities::SentinelCanScan(s.r.v));
    s.SetSolarOrbit(0.0, 0.0, 1.2e10, 0.0);
    s.r.v.situation = Vessel::Situations::SUB_ORBITAL;
    CHECK_FALSE(SM::SentinelUtilities::SentinelCanScan(s.r.v));
    s.r.v.situation = Vessel::Situations::ORBITING;
    // the inclination window: Sentinel.SignificantDeviation percent of 90 degrees (Kerbin is not inclined: no LAN test)
    const double window = static_cast<double>(FP3::ContractDefs::Sentinel::SignificantDeviation) / 100.0 * 90.0;
    s.SetSolarOrbit(window - 0.01, 0.0, 1.2e10, 0.0);
    CHECK(SM::SentinelUtilities::SentinelCanScan(s.r.v));
    s.SetSolarOrbit(window + 0.01, 0.0, 1.2e10, 0.0);
    CHECK_FALSE(SM::SentinelUtilities::SentinelCanScan(s.r.v));
    // a vessel around Kerbin never scans
    s.O().SetOrbit(0.0, 0.0, 700000.0, 0.0, 0.0, 0.0, 0.0, &w.Body("Kerbin"));
    CHECK_FALSE(SM::SentinelUtilities::SentinelCanScan(s.r.v));
    // Moho is inclined (7 degrees, LAN 70): the LAN of the vessel must match within Satellite.SignificantDeviation percent of 360 degrees
    const Orbit& moho = *BodyOrbit(&w.Body("Moho"));
    const double lanWindow = FP3::ContractDefs::Satellite::SignificantDeviation / 100.0 * 360.0;
    s.SetSolarOrbit(moho.inclination, moho.eccentricity, moho.semiMajorAxis - 1000.0, moho.LAN);
    CHECK(SM::SentinelUtilities::FindInnerAndOuterBodies(s.r.v, inner, outer));
    CHECK(inner->name() == "Sun");
    CHECK(outer->name() == "Moho");
    CHECK(SM::SentinelUtilities::SentinelCanScan(s.r.v));
    s.SetSolarOrbit(moho.inclination, moho.eccentricity, moho.semiMajorAxis - 1000.0, moho.LAN + lanWindow - 0.5);
    CHECK(SM::SentinelUtilities::SentinelCanScan(s.r.v));
    s.SetSolarOrbit(moho.inclination, moho.eccentricity, moho.semiMajorAxis - 1000.0, moho.LAN + lanWindow + 0.5);
    CHECK_FALSE(SM::SentinelUtilities::SentinelCanScan(s.r.v));
    // the difference wraps: LAN - 360 + small is close to Moho's LAN again
    s.SetSolarOrbit(moho.inclination, moho.eccentricity, moho.semiMajorAxis - 1000.0, moho.LAN + 360.0 - 1.0);
    CHECK(SM::SentinelUtilities::SentinelCanScan(s.r.v));
    // a retrograde vessel (negative inclination): its LAN is shifted by 180 degrees before the comparison
    s.SetSolarOrbit(-moho.inclination, moho.eccentricity, moho.semiMajorAxis - 1000.0, moho.LAN + 180.0);
    CHECK(SM::SentinelUtilities::SentinelCanScan(s.r.v));
    // an SMA beyond the outermost planet indexes past the list (the C# throws ArgumentOutOfRangeException)
    s.SetSolarOrbit(0.0, 0.0, 1.0e12, 0.0);
    CHECK_THROWS_AS(SM::SentinelUtilities::FindInnerAndOuterBodies(s.r.v, inner, outer), std::out_of_range);
    // GetVesselClass: the loaded vessel reads DiscoveryInfo.objectSize
    CHECK(SM::SentinelUtilities::GetVesselClass(s.r.v) == s.r.v.GetDiscoveryInfo().objectSize());
}

TEST_CASE("WP-38c SentinelUtilities on unloaded vessels: orbit snapshot, discovery size") {
    SentinelWorld w;
    SentinelRig s(w);
    s.SetSolarOrbit(0.0, 0.0, 1.2e10, 0.0);
    auto pv = std::make_shared<ProtoVessel>(s.r.v);
    pv->situation = VesselSituations::ORBITING;
    pv->orbitSnapShot.emplace(s.O(), FlightRegistry::Bodies());
    pv->discoveryInfo = std::make_shared<ConfigNode>("DISCOVERY");
    s.r.v.protoVessel = pv.get();
    s.r.v.loaded = false;
    CelestialBody* inner = nullptr;
    CelestialBody* outer = nullptr;
    CHECK(SM::SentinelUtilities::FindInnerAndOuterBodies(s.r.v, inner, outer));
    CHECK(inner->name() == "Eve");
    CHECK(outer->name() == "Kerbin");
    CHECK(SM::SentinelUtilities::SentinelCanScan(s.r.v));
    pv->situation = VesselSituations::SUB_ORBITAL;
    CHECK_FALSE(SM::SentinelUtilities::SentinelCanScan(s.r.v));
    pv->situation = VesselSituations::ORBITING;
    pv->orbitSnapShot->eccentricity = 0.5;
    CHECK_FALSE(SM::SentinelUtilities::SentinelCanScan(s.r.v));
    // the unloaded vessel's discovery size comes from the node (default class C)
    CHECK(SM::SentinelUtilities::GetVesselClass(s.r.v) == UntrackedObjectClass::C);
    pv->discoveryInfo->AddValue("size", "5");
    CHECK(SM::SentinelUtilities::GetVesselClass(s.r.v) == UntrackedObjectClass::F);
    s.r.v.protoVessel = nullptr;
    s.r.v.loaded = true;
}

TEST_CASE("WP-38c SentinelModule: start / stop tracking, activation requirements, status text") {
    SentinelWorld w;
    SentinelRig s(w);
    REQUIRE(s.module != nullptr);
    // the vessel is a probe with antenna and generator: the objective modules answer from the real prefabs
    s.SetSolarOrbit(0.0, 0.0, 1.2e10, 0.0);
    L38::FinePrintEnvironment::postScreenMessage = nullptr;
    L38::FinePrintEnvironment::screenMessages.clear();
    SM::SentinelModule::CacheLocalStrings();
    s.module->OnAwake();
    s.module->OnStart(StartState::Orbital);
    CHECK(s.module->Events()["StartTracking"]->active);
    CHECK_FALSE(s.module->Events()["StopTracking"]->active);
    CHECK(s.module->Events()["StartTracking"]->guiActiveUnfocused);
    s.module->FixedUpdate();
    CHECK(s.module->status() == SM::SentinelModule::cacheAutoLOC_6002296);
    CHECK(SM::SentinelModule::cacheAutoLOC_6002296 == KSP::Localization::Localizer::Format("#autoLOC_6002296"));
    // the event handlers are bound (invoking the KSPEvent runs StartTracking)
    s.module->Events()["StartTracking"]->Invoke();
    CHECK(s.module->isTracking());
    CHECK_FALSE(s.module->Events()["StartTracking"]->active);
    CHECK(s.module->Events()["StopTracking"]->active);
    REQUIRE(L38::FinePrintEnvironment::screenMessages.size() == 1);
    CHECK(L38::FinePrintEnvironment::screenMessages[0] ==
          KSP::Localization::Localizer::Format("#autoLOC_6002285", {SM::SentinelUtilities::SentinelPartTitle(), s.module->FocusName()}));
    CHECK(s.module->FocusName() == KSP::Localization::Localizer::Format("#autoLOC_7001301", {w.Body("Kerbin").displayName()}));
    // OnStart honours the persisted flag
    s.module->OnStart(StartState::Orbital);
    CHECK_FALSE(s.module->Events()["StartTracking"]->active);
    CHECK(s.module->Events()["StopTracking"]->active);
    // status: scanning (in range) / not scanning (out of range)
    s.module->FixedUpdate();
    CHECK(s.module->status() == KSP::Localization::Localizer::Format("#autoLOC_6002291", {w.Body("Kerbin").displayName()}));
    s.SetSolarOrbit(0.0, 0.4, 1.2e10, 0.0);
    s.module->FixedUpdate();
    CHECK(s.module->status() == KSP::Localization::Localizer::Format("#autoLOC_6002292", {w.Body("Kerbin").displayName()}));
    // SMA beyond the outermost planet: FindInnerAndOuterBodies is false only when below the list; beyond it the C# throws inside FixedUpdate
    // stopping
    L38::FinePrintEnvironment::screenMessages.clear();
    s.module->Events()["StopTracking"]->Invoke();
    CHECK_FALSE(s.module->isTracking());
    CHECK(s.module->Events()["StartTracking"]->active);
    CHECK_FALSE(s.module->Events()["StopTracking"]->active);
    REQUIRE(L38::FinePrintEnvironment::screenMessages.size() == 1);
    CHECK(L38::FinePrintEnvironment::screenMessages[0] ==
          KSP::Localization::Localizer::Format("#autoLOC_6002287", {SM::SentinelUtilities::SentinelPartTitle(), s.module->FocusName()}));
    // activation requirements: antenna and generator on the vessel, and a heliocentric orbit
    L38::FinePrintEnvironment::screenMessages.clear();
    s.O().SetOrbit(0.0, 0.0, 700000.0, 0.0, 0.0, 0.0, 0.0, &w.Body("Kerbin"));
    s.module->StartTracking();
    CHECK_FALSE(s.module->isTracking());
    REQUIRE(L38::FinePrintEnvironment::screenMessages.size() == 1);
    CHECK(L38::FinePrintEnvironment::screenMessages[0] == KSP::Localization::Localizer::Format("#autoLOC_6002295", {SM::SentinelUtilities::SentinelPartTitle()}));
    // a vessel without a generator / an antenna
    SentinelRig bare(w, {"probeCoreOcto.v2", "InfraredTelescope"});
    REQUIRE(bare.module != nullptr);
    bare.module->OnAwake();
    bare.SetSolarOrbit(0.0, 0.0, 1.2e10, 0.0);
    L38::FinePrintEnvironment::screenMessages.clear();
    bare.module->StartTracking();
    CHECK_FALSE(bare.module->isTracking());
    REQUIRE(L38::FinePrintEnvironment::screenMessages.size() == 1);
    CHECK(L38::FinePrintEnvironment::screenMessages[0] == KSP::Localization::Localizer::Format("#autoLOC_6002293", {SM::SentinelUtilities::SentinelPartTitle()}));
}

TEST_CASE("WP-38c SentinelScenario: deployed sentinels, matching orbit, reset, game database limits, asteroid spawn routine") {
    SentinelWorld w;
    SentinelRig s(w);
    REQUIRE(s.module != nullptr);
    s.module->OnAwake();
    s.SetSolarOrbit(0.0, 0.0, 1.2e10, 0.0);
    SM::SentinelScenario sc;
    sc.Awake();
    CHECK(SM::SentinelScenario::Instance == &sc);
    CHECK(sc.GetDeployedSentinels().empty());  // not tracking
    s.module->setIsTracking(true);
    CHECK(sc.GetDeployedSentinels() == std::vector<Vessel*>{&s.r.v});
    s.SetSolarOrbit(0.0, 0.4, 1.2e10, 0.0);  // tracking but unable to scan
    CHECK(sc.GetDeployedSentinels().empty());
    s.SetSolarOrbit(0.0, 0.0, 1.2e10, 0.0);
    // ActiveSentinelMatchingOrbit
    Orbit same(0.0, 0.0, 1.2e10, 0.0, 0.0, 0.0, 0.0, &w.Body("Sun"));
    CHECK(sc.ActiveSentinelMatchingOrbit(&same, 5.0));
    Orbit other(0.0, 0.0, 1.3e10, 0.0, 0.0, 0.0, 0.0, &w.Body("Sun"));
    CHECK_FALSE(sc.ActiveSentinelMatchingOrbit(&other, 5.0));
    sc.deployedSentinels = sc.GetDeployedSentinels();  // Start() / the routine refresh the list
    CHECK(sc.MaxSentinelUntrackedSpaceObjects() == 10 * 1);
    // NextSpawnTime defaults to double.MinValue (persistent field)
    CHECK_F64n(sc.NextSpawnTime(), -1.7976931348623157e308);
    // ResetSentinels clears the flag of the loaded module
    sc.ResetSentinels();
    CHECK_FALSE(s.module->isTracking());
    s.module->setIsTracking(true);
    // LoadGameDBSettings: the first SPACEOBJECTS_LIMITS node sets the per-sentinel limit
    ConfigNode limits("SPACEOBJECTS_LIMITS");
    limits.AddValue("perSentinelSpawnGroupMaxLimit", static_cast<int32_t>(7));
    sc.LoadGameDBSettings({&limits});
    CHECK(SM::SentinelUtilities::perSentinelObjectLimit == 7);
    CHECK(sc.MaxSentinelUntrackedSpaceObjects() == 7);
    sc.LoadGameDBSettings({});
    CHECK(SM::SentinelUtilities::perSentinelObjectLimit == 7);  // no node: unchanged
    SM::SentinelUtilities::perSentinelObjectLimit = 10;
    // a second instance replaces the first (OnAwake destroys the old one)
    {
        SM::SentinelScenario second;
        second.Awake();
        CHECK(SM::SentinelScenario::Instance == &second);
        CHECK(sc.destroyed_);
        SM::SentinelScenario::Instance = &sc;
        sc.destroyed_ = false;
    }
    // the spawn routine: find a seed whose first draw passes the SpawnChance gate
    int seed = 1;
    while (KSPRandom(seed).NextDouble() > static_cast<double>(SM::SentinelUtilities::SpawnChance)) ++seed;
    ContractsEnvironment::Current().newRandomSeed = [seed] { return seed; };
    struct Spawned {
        std::string name;
        Orbit orbit;
        uint32_t seed;
        UntrackedObjectClass cls;
        double life, lifeMax;
    };
    std::vector<Spawned> spawned;
    SM::SentinelEnvironment::generateAsteroidName = [] { return std::string("Ast. XYZ-1"); };
    SM::SentinelEnvironment::spawnAsteroid = [&](const std::string& name, Orbit& o, uint32_t sd, UntrackedObjectClass cls, double life, double lifeMax) {
        spawned.push_back({name, o, sd, cls, life, lifeMax});
    };
    Planetarium::fetch->time = 1000.0;
    CoroutineScheduler& sched = CareerEnvironment::Current().Scheduler();
    sc.Start();
    // replicate the generator: SentinelRoutine -> ProcessSentinelScan's first segment (gate, processTime) -> NextSpawnTime
    KSPRandom g(seed);
    const int day = KSPUtil::dateTimeFormatter().Day();
    const double gate = g.NextDouble();
    CHECK(gate <= 0.25);
    const double processTime = 1000.0 + (0.0 + g.NextDouble() * (static_cast<double>(day) / 3.0 - 0.0));
    const double nextSpawn = 1000.0 + static_cast<double>(day) * (0.5 + g.NextDouble() * (1.5 - 0.5));
    CHECK_F64n(sc.NextSpawnTime(), nextSpawn);
    CHECK(spawned.empty());
    // time passes: the scan runs when UT reaches processTime
    Planetarium::fetch->time = processTime - 1.0;
    sched.AdvanceFrame(0.02f);
    sched.RunDelayedDynamicFrameRate();
    CHECK(spawned.empty());
    Planetarium::fetch->time = processTime;
    sched.AdvanceFrame(0.02f);
    sched.RunDelayedDynamicFrameRate();
    REQUIRE(spawned.size() == 1);
    CHECK(spawned[0].name == "Ast. XYZ-1");
    CHECK(spawned[0].orbit.referenceBody == &w.Body("Sun"));
    CHECK(spawned[0].life >= static_cast<double>(day * 4));
    CHECK(spawned[0].life <= static_cast<double>(day * 80));
    CHECK_F64n(spawned[0].lifeMax, static_cast<double>(day * 80));
    CHECK(static_cast<int>(spawned[0].cls) >= 0);
    CHECK(static_cast<int>(spawned[0].cls) <= 8);
    // before NextSpawnTime nothing else happens; a second sweep spawns again (the gate is a new draw)
    Planetarium::fetch->time = nextSpawn - 1.0;
    sched.AdvanceFrame(0.02f);
    sched.RunDelayedDynamicFrameRate();
    CHECK(spawned.size() == 1);
    CHECK_F64n(sc.NextSpawnTime(), nextSpawn);
    Planetarium::fetch->time = nextSpawn;
    sched.AdvanceFrame(0.02f);
    sched.RunDelayedDynamicFrameRate();
    CHECK(sc.NextSpawnTime() > nextSpawn);
    sc.OnDestroy();
    SM::SentinelScenario::Instance = nullptr;
}

TEST_CASE("WP-38c SentinelScenario.SentinelAsteroidOrbit: deterministic, heliocentric, near the outer body") {
    SentinelWorld w;
    SentinelRig s(w);
    s.SetSolarOrbit(0.0, 0.0, 1.2e10, 0.0);
    Planetarium::fetch->time = 5000.0;
    s.O().UpdateFromUT(5000.0);
    SM::SentinelScenario a, b;
    a.generator = std::make_unique<KSPRandom>(77);
    b.generator = std::make_unique<KSPRandom>(77);
    Orbit oa = a.SentinelAsteroidOrbit(s.O());
    Orbit ob = b.SentinelAsteroidOrbit(s.O());
    CHECK_F64n(oa.semiMajorAxis, ob.semiMajorAxis);
    CHECK_F64n(oa.eccentricity, ob.eccentricity);
    CHECK_F64n(oa.inclination, ob.inclination);
    CHECK_F64n(oa.meanAnomalyAtEpoch, ob.meanAnomalyAtEpoch);
    CHECK(oa.referenceBody == &w.Body("Sun"));
    CHECK(std::isfinite(oa.semiMajorAxis));
    // the semi-major axis is at least the outer body's (Math.Max with outer.sma +- 10 percent)
    CHECK(oa.semiMajorAxis > 0.0);
    // another seed gives another orbit
    SM::SentinelScenario c;
    c.generator = std::make_unique<KSPRandom>(78);
    Orbit oc = c.SentinelAsteroidOrbit(s.O());
    CHECK((oc.inclination != oa.inclination || oc.meanAnomalyAtEpoch != oa.meanAnomalyAtEpoch));
    // the inclination offset stays inside the adjusted view angle
    const double halfView = SM::SentinelUtilities::AdjustedSentinelViewAngle(s.O(), *BodyOrbit(&w.Body("Kerbin"))) / 2.0;
    CHECK(std::fabs(oa.inclination - s.O().inclination) <= halfView + 1e-6);
}

TEST_CASE("WP-38c SentinelParameter.DiscoverAsteroid: scan filters, the orbit match, completion and messages") {
    SentinelWorld w;
    Career38b career(w);
    LoadScenario38b(w, RefS()["scenarios"][9]);  // "everything"
    SentinelRig s(w);
    s.SetSolarOrbit(0.0, 0.0, 1.2e10, 0.0);
    s.module->OnAwake();
    s.module->setIsTracking(true);
    SM::SentinelScenario sc;
    sc.Awake();
    sc.generator = std::make_unique<KSPRandom>(1);
    sc.deployedSentinels = sc.GetDeployedSentinels();
    // an active contract with the sentinel parameter (a CLASS scan for D asteroids around Kerbin) and an orbit parameter the vessel matches
    auto c = std::make_unique<SM::SentinelContract>();
    c->agent = w.agents->GetAgent("Kerbodyne");
    c->innerBody = &w.Body("Eve");
    c->outerBody = &w.Body("Kerbin");
    c->asteroidCount = 3;
    auto orbitParam = std::make_unique<P38::SpecificOrbitParameter>(FP::OrbitType::RANDOM, 0.0, 0.0, 1.2e10, 0.0, 0.0, 0.0, 0.0, &w.Body("Sun"), 5.0);
    P38::SpecificOrbitParameter* op = c->AddParameter(std::move(orbitParam)) ? c->GetParameterT<P38::SpecificOrbitParameter>() : nullptr;
    REQUIRE(op != nullptr);
    auto sp = std::make_unique<SM::SentinelParameter>(&w.Body("Kerbin"), SM::SentinelScanType::CLASS, UntrackedObjectClass::D, 0.0, 0.0, 3);
    SM::SentinelParameter* p = static_cast<SM::SentinelParameter*>(c->AddParameter(std::move(sp)));
    c->state = Contract::State::Active;
    c->Register();
    L38::FinePrintEnvironment::screenMessages.clear();
    // OnRegister: DisableOnStateChange
    p->DiscoverAsteroid(UntrackedObjectClass::C, 0.1, 5.0, &w.Body("Kerbin"));   // wrong class
    CHECK(p->RemainingDiscoveries == 3);
    p->DiscoverAsteroid(UntrackedObjectClass::D, 0.1, 5.0, &w.Body("Duna"));     // wrong body
    CHECK(p->RemainingDiscoveries == 3);
    p->DiscoverAsteroid(UntrackedObjectClass::D, 0.1, 5.0, &w.Body("Kerbin"));
    CHECK(p->RemainingDiscoveries == 2);
    REQUIRE(L38::FinePrintEnvironment::screenMessages.size() == 1);
    CHECK(L38::FinePrintEnvironment::screenMessages[0] ==
          KSP::Localization::Localizer::Format("#autoLOC_6002310", {"1", "3", w.Body("Kerbin").displayName(), c->Agent()->Title()}));
    // the orbit parameter is not complete: a scan only counts while a deployed sentinel matches the orbit; move the vessel away
    s.SetSolarOrbit(0.0, 0.0, 1.6e10, 0.0);
    p->DiscoverAsteroid(UntrackedObjectClass::D, 0.1, 5.0, &w.Body("Kerbin"));
    CHECK(p->RemainingDiscoveries == 2);
    s.SetSolarOrbit(0.0, 0.0, 1.2e10, 0.0);
    p->DiscoverAsteroid(UntrackedObjectClass::D, 0.1, 5.0, &w.Body("Kerbin"));
    CHECK(p->RemainingDiscoveries == 1);
    L38::FinePrintEnvironment::screenMessages.clear();
    p->DiscoverAsteroid(UntrackedObjectClass::D, 0.1, 5.0, &w.Body("Kerbin"));
    CHECK(p->RemainingDiscoveries == 0);
    REQUIRE(!L38::FinePrintEnvironment::screenMessages.empty());
    CHECK(L38::FinePrintEnvironment::screenMessages[0] == KSP::Localization::Localizer::Format("#autoLOC_6002311", {w.Body("Kerbin").displayName(), c->Agent()->Title()}));
    CHECK(p->State() == ksp::Contracts::ParameterState::Complete);
    // eccentricity and inclination scans filter on the minimum
    c->state = Contract::State::Active;
    // (a parameter outside a contract has no Root: attach them to the contract)
    auto* ep = static_cast<SM::SentinelParameter*>(c->AddParameter(std::make_unique<SM::SentinelParameter>(&w.Body("Kerbin"), SM::SentinelScanType::ECCENTRICITY, UntrackedObjectClass::A, 0.2, 0.0, 2)));
    auto* ip = static_cast<SM::SentinelParameter*>(c->AddParameter(std::make_unique<SM::SentinelParameter>(&w.Body("Kerbin"), SM::SentinelScanType::INCLINATION, UntrackedObjectClass::A, 0.0, 20.0, 2)));
    op->SetComplete();  // no orbit requirement
    ep->DiscoverAsteroid(UntrackedObjectClass::A, 0.1, 30.0, &w.Body("Kerbin"));
    CHECK(ep->RemainingDiscoveries == 2);
    ep->DiscoverAsteroid(UntrackedObjectClass::A, 0.2, 0.0, &w.Body("Kerbin"));
    CHECK(ep->RemainingDiscoveries == 1);
    ip->DiscoverAsteroid(UntrackedObjectClass::A, 0.5, 19.0, &w.Body("Kerbin"));
    CHECK(ip->RemainingDiscoveries == 2);
    ip->DiscoverAsteroid(UntrackedObjectClass::A, 0.0, 20.0, &w.Body("Kerbin"));
    CHECK(ip->RemainingDiscoveries == 1);
    // an inactive contract ignores discoveries
    c->state = Contract::State::Offered;
    ip->DiscoverAsteroid(UntrackedObjectClass::A, 0.0, 25.0, &w.Body("Kerbin"));
    CHECK(ip->RemainingDiscoveries == 1);
    c->Unregister();
    sc.OnDestroy();
    SM::SentinelScenario::Instance = nullptr;
}

// ================================================================================================================================================
// CompatibilityUtilities (FinePrint/Utilities/CompatibilityUtilities.cs)
// ================================================================================================================================================
#include "ksp/fineprint/compatibility.hpp"

namespace {

const json& RefC() { return Ref3()["compat"]; }

ConfigNode::NodePtr NodeFromJson(const json& j) {
    auto n = std::make_shared<ConfigNode>(j["name"].get<std::string>());
    for (const json& v : j["values"]) n->AddValue(v[0].get<std::string>(), v[1].get<std::string>());
    for (const json& c : j["nodes"]) n->AddNode(NodeFromJson(c));
    return n;
}

namespace LC = ksp::Contracts::Lite;

}  // namespace

TEST_CASE("WP-38c CompatibilityUtilities: OldCareerSave, SavePriorTo and UpgradeVesselSituation vs the reference") {
    World38b w;
    const json& r = RefC();
    for (const json& e : r["oldTable"]) {
        INFO(e.dump());
        const Game::Modes mode = e["mode"] == "CAREER" ? Game::Modes::CAREER : (e["mode"] == "SANDBOX" ? Game::Modes::SANDBOX : Game::Modes::SCIENCE_SANDBOX);
        LC::CompatibilityUtilities::majorVersion = LC::CompatibilityUtilities::minorVersion = LC::CompatibilityUtilities::revisionVersion = 2147483647;
        CHECK(LC::CompatibilityUtilities::OldCareerSave(mode, e["version"].get<std::string>()) == e["old"].get<bool>());
        // the saved version is remembered (also for a non career game); an unparsable string leaves it alone
        if (e["ints"][0].get<int>() != 2147483647 || e["old"].get<bool>() == false) {
            CHECK(LC::CompatibilityUtilities::majorVersion == e["ints"][0].get<int>());
            CHECK(LC::CompatibilityUtilities::minorVersion == e["ints"][1].get<int>());
            CHECK(LC::CompatibilityUtilities::revisionVersion == e["ints"][2].get<int>());
        }
    }
    for (const json& e : r["priorTo"]) {
        const std::string ver = e["version"];
        int a = 0, b = 0, c = 0;
        std::sscanf(ver.c_str(), "%d.%d.%d", &a, &b, &c);
        LC::CompatibilityUtilities::OldCareerSave(Game::Modes::CAREER, a, b, c);
        for (const json& q : e["p"]) CHECK_MESSAGE(LC::CompatibilityUtilities::SavePriorTo(q[0], q[1], q[2]) == q[3].get<bool>(), ver, " ", q.dump());
    }
    size_t n = 0;
    for (const json& e : r["situations"]) {
        const std::string ver = e["version"];
        int a = 0, b = 0, c = 0;
        std::sscanf(ver.c_str(), "%d.%d.%d", &a, &b, &c);
        LC::CompatibilityUtilities::OldCareerSave(Game::Modes::CAREER, a, b, c);
        CHECK_MESSAGE(LC::CompatibilityUtilities::UpgradeVesselSituation(e["in"].get<std::string>()) == e["out"].get<std::string>(), ver, " ", e["in"]);
        ++n;
    }
    CHECK(n > 600);
}

TEST_CASE("WP-38c CompatibilityUtilities.UpdateCareerSave: every branch over the synthetic old-format contract nodes (byte-exact vs the reference)") {
    World38b w;
    const json& r = RefC();
    int changed = 0, unchanged = 0;
    for (const json& e : r["cases"]) {
        INFO(e["label"] << " @ " << e["version"]);
        ConfigNode::NodePtr node = NodeFromJson(e["input"]);
        const std::string before = node->SaveToString();
        const bool old = LC::CompatibilityUtilities::OldCareerSave(Game::Modes::CAREER, e["version"].get<std::string>());
        if (e["output"].is_null()) {
            CHECK_THROWS_AS(LC::CompatibilityUtilities::UpdateCareerSave(*node), NullReferenceException);
            continue;
        }
        if (old) LC::CompatibilityUtilities::UpdateCareerSave(*node);
        const std::string got = node->SaveToString();
        CHECK(got == e["output"].get<std::string>());
        (got == before ? unchanged : changed)++;
    }
    MESSAGE("cases changed " << changed << ", unchanged " << unchanged);
    CHECK(changed > 1000);
    CHECK(unchanged > 100);
}

TEST_CASE("WP-38c CompatibilityUtilities.CleanUpUnsanitaryEVAKerbals + FindSurveyDefinition + the ContractsEnvironment hooks") {
    World38b w;
    const json& r = RefC();
    for (const json& e : r["eva"]) {
        INFO(e["label"]);
        ConfigNode::NodePtr node = NodeFromJson(e["input"]);
        LC::CompatibilityUtilities::CleanUpUnsanitaryEVAKerbals(*node);
        CHECK(node->SaveToString() == e["output"].get<std::string>());
    }
    // FindSurveyDefinition: the Contracts.cfg survey definitions (via ContractDefsLite when no GameDatabase answers)
    auto def = LC::CompatibilityUtilities::FindSurveyDefinition("evaReport");
    REQUIRE(def != nullptr);
    CHECK(def->DataName == "observational");
    CHECK(def->ResultName == "reports");
    CHECK(LC::CompatibilityUtilities::FindSurveyDefinition("noSuchExperiment") == nullptr);
    // the seam answers instead
    LC::CompatibilityEnvironment::contractsConfigNodes = [] { return std::vector<ConfigNode*>{}; };
    CHECK(LC::CompatibilityUtilities::FindSurveyDefinition("evaReport") == nullptr);
    LC::CompatibilityEnvironment::contractsConfigNodes = nullptr;
    // FinePrint::InstallAll connects the hooks ContractSystem.OnLoad uses
    const ContractsEnvironment& env = ContractsEnvironment::Current();
    REQUIRE(static_cast<bool>(env.oldCareerSave));
    REQUIRE(static_cast<bool>(env.updateCareerSave));
    CHECK(env.oldCareerSave("1.0.4"));
    CHECK_FALSE(env.oldCareerSave("1.12.5"));
    ConfigNode::NodePtr node = NodeFromJson(RefC()["cases"][0]["input"]);
    env.updateCareerSave(*node);
    CHECK(node->SaveToString() != std::string());
    // ContractSystem.LoadNow runs the upgrade for an old career save (the gameNode is changed in place) and records the current version
    {
        ContractSystem cs;
        cs.Awake();
        ConfigNode save("SCENARIO");
        save.AddValue("name", "ContractSystem");
        save.AddValue("version", "1.0.4");
        ConfigNode* contracts = save.AddNode("CONTRACTS");
        ConfigNode* c = contracts->AddNode("CONTRACT");
        c->AddValue("type", "RecordTrackContract");
        ConfigNode* keep = contracts->AddNode("CONTRACT");
        keep->AddValue("type", "SatelliteContract");
        cs.LoadNow(save);
        CHECK(cs.Version() == ContractsEnvironment::Current().versionString);
        CHECK(save.GetNode("CONTRACTS")->GetNodes("CONTRACT").size() == 1);  // the RecordTrackContract was removed
        CHECK(save.GetNode("CONTRACTS")->GetNode("CONTRACT")->HasValue("placementMode"));
        // a current save is not touched
        ConfigNode save2("SCENARIO");
        save2.AddValue("name", "ContractSystem");
        save2.AddValue("version", "1.12.5");
        save2.AddNode("CONTRACTS")->AddNode("CONTRACT")->AddValue("type", "SatelliteContract");
        cs.LoadNow(save2);
        CHECK_FALSE(save2.GetNode("CONTRACTS")->GetNode("CONTRACT")->HasValue("placementMode"));
    }
}

// ================================================================================================================================================
// The pre-built craft model and the three WP-37 deferred templates (VesselRepair, OrbitalConstruction, RoverConstruction)
// ================================================================================================================================================
#include "ksp/fineprint/construction_templates.hpp"
#include "ksp/fineprint/prebuilt.hpp"

namespace {

const json& RefT() { return Ref3()["templates"]; }

// Waypoint.RandomizeNear runs through sin / cos / asin / atan2: the reference model uses the host libm, kspcore the game runtime's (ksp::dotnet::Math), so the
// result may differ in the last bits. The distance in units in the last place is recorded and bounded (STATUS.md).
int64_t UlpDistance(double a, double b) {
    int64_t x, y;
    std::memcpy(&x, &a, 8);
    std::memcpy(&y, &b, 8);
    if (x < 0) x = INT64_MIN - x;
    if (y < 0) y = INT64_MIN - y;
    return x > y ? x - y : y - x;
}
void CheckUlp(double got, double want, int64_t& maxUlp) {
    const int64_t d = UlpDistance(got, want);
    if (d > maxUlp) maxUlp = d;
    CHECK_MESSAGE(d <= 16, "got ", got, " want ", want, " (", d, " ulp)");
}

// The synthetic craft files and PREBUILTCRAFT definitions of the reference (the stock .craft files are empty files in the mirror) are written below a temp
// folder; the PREBUILTPOSITIONS and CONSTRUCTION_PART nodes are the real Squad cfgs.
struct PreBuiltFixture {
    std::filesystem::path dir;
    ConfigNode::NodePtr craftCfg, positions, constructionParts;
    PreBuiltFixture() {
        namespace fs = std::filesystem;
        dir = fs::temp_directory_path() / "kspcore_wp38c_prebuilt";
        fs::remove_all(dir);
        for (auto it = RefT()["crafts"].begin(); it != RefT()["crafts"].end(); ++it) {
            const fs::path p = dir / it.key();
            fs::create_directories(p.parent_path());
            std::ofstream f(p, std::ios::binary);
            f << it.value().get<std::string>();
        }
        craftCfg = ConfigNode::Parse(RefT()["cfg"].get<std::string>());
        const std::string root = ksp::testing::GameRoot() + "/GameData/Squad/Contracts/";
        ConfigNode::NodePtr pos = ConfigNode::Load(root + "PrebuiltCraft/prebuiltData.cfg");
        REQUIRE(pos);
        positions = pos;
        ConfigNode::NodePtr parts = ConfigNode::Load(root + "constructionContractParts.cfg");
        REQUIRE(parts);
        constructionParts = parts;
        FP3::ContractDefs::applicationRootPath = dir.generic_string() + "/";
        FP3::ContractDefs::gameDatabaseConfigNodes = [this](const std::string& type) {
            std::vector<ConfigNode*> out;
            if (type == "CONTRACTCRAFT") {
                // GameDatabase.GetConfigNodes returns the nodes of the files (the CONTRACTCRAFT node itself, not the file wrapper)
                for (ConfigNode* n : craftCfg->GetNodes("CONTRACTCRAFT")) out.push_back(n);
                for (ConfigNode* n : positions->GetNodes("CONTRACTCRAFT")) out.push_back(n);
            } else if (type == "CONTRACT_CONSTRUCTION_PARTS") {
                for (ConfigNode* n : constructionParts->GetNodes("CONTRACT_CONSTRUCTION_PARTS")) out.push_back(n);
            }
            return out;
        };
        Reload();
    }
    ~PreBuiltFixture() {
        FP3::ContractDefs::gameDatabaseConfigNodes = nullptr;
        FP3::ContractDefs::applicationRootPath.clear();
        FP3::ContractDefs::loadedContractCraftDefs = false;
        FP3::ContractDefs::loadedConstructionPartsList = false;
        FP3::ContractDefs::PreBuiltCraftDefs.reset();
        FP3::ContractDefs::constructionParts.reset();
        FP3::ContractDefs::PreBuiltCraftPositions.items.clear();
        std::filesystem::remove_all(dir);
    }
    // a fresh load: the per-type lists are cached by reference and the contracts remove entries from them
    void Reload() {
        FP3::ContractDefs::loadedContractCraftDefs = false;
        FP3::ContractDefs::loadedConstructionPartsList = false;
        FP3::ContractDefs::LoadContractCraftDefs();
        FP3::ContractDefs::LoadConstructionPartsList();
    }
};

std::string GameDataTail(const std::string& url) {
    const size_t i = url.rfind("GameData");
    return i == std::string::npos ? url : url.substr(i);
}

}  // namespace

TEST_CASE("WP-38c pre-built craft model: PREBUILTCRAFT definitions validated against the part database, positions, construction parts (vs the reference)") {
    World38b w;
    PreBuiltFixture fx;
    const json& r = RefT();
    REQUIRE(FP3::ContractDefs::PreBuiltCraftDefs != nullptr);
    // the definitions in cfg order: every contract type's list
    std::map<std::string, std::vector<const json*>> expected;
    for (const json& d : r["definitions"])
        for (const json& t : d["types"]) expected[t.get<std::string>()].push_back(&d);
    CHECK(expected.size() == 3);
    for (const auto& kv : expected) {
        auto& list = FP3::ContractDefs::PreBuiltCraftDefs->CraftForContractType(kv.first);
        REQUIRE(list.size() == kv.second.size());
        for (size_t i = 0; i < list.size(); ++i) {
            const json& d = *kv.second[i];
            CHECK(GameDataTail(list[i]->craftURL) == d["url"].get<std::string>());
            CHECK(list[i]->allowGround == d["allowGround"].get<bool>());
            CHECK(list[i]->allowOrbit == d["allowOrbit"].get<bool>());
            CHECK(list[i]->allowWater == d["allowWater"].get<bool>());
            CHECK(list[i]->usePreBuiltPositions == d["usePos"].get<bool>());
            CHECK(list[i]->brokenPartNames() == d["broken"].get<std::vector<std::string>>());
            CHECK(list[i]->craftInfo.compatibility == VersionCompareResult::COMPATIBLE);
            CHECK_FALSE(list[i]->craftInfo.CraftHasErrors());
            CHECK_FALSE(list[i]->craftNode == nullptr);
        }
        // CraftForContractType caches by reference
        CHECK(&FP3::ContractDefs::PreBuiltCraftDefs->CraftForContractType(kv.first) == &list);
    }
    // CraftByURL compares from "GameData" on
    auto byUrl = FP3::ContractDefs::PreBuiltCraftDefs->CraftByURL("C:/elsewhere/" + r["definitions"][0]["url"].get<std::string>());
    REQUIRE(byUrl != nullptr);
    CHECK(GameDataTail(byUrl->craftURL) == r["definitions"][0]["url"].get<std::string>());
    CHECK(FP3::ContractDefs::PreBuiltCraftDefs->CraftByURL("GameData/none.craft") == nullptr);
    CHECK_THROWS_AS(FP3::ContractDefs::PreBuiltCraftDefs->CraftByURL("no marker.craft"), std::out_of_range);
    // the positions: insertion ordered per body
    const json& pos = r["positions"];
    REQUIRE(FP3::ContractDefs::PreBuiltCraftPositions.items.size() == pos.size());
    for (size_t i = 0; i < pos.size(); ++i) {
        CHECK(FP3::ContractDefs::PreBuiltCraftPositions.items[i].first == pos[i][0].get<std::string>());
        const auto& lst = FP3::ContractDefs::PreBuiltCraftPositions.items[i].second;
        REQUIRE(lst.size() == pos[i][1].size());
        for (size_t j = 0; j < lst.size(); ++j) {
            CHECK_F64n(static_cast<double>(lst[j].CenterLatitude), pos[i][1][j][0].get<double>());
            CHECK_F64n(static_cast<double>(lst[j].CenterLongitude), pos[i][1][j][1].get<double>());
            CHECK_F64n(static_cast<double>(lst[j].SearchRadius), pos[i][1][j][2].get<double>());
        }
    }
    CHECK(FP3::ContractDefs::PreBuiltCraftPositions.ContainsKey("Mun"));
    CHECK_FALSE(FP3::ContractDefs::PreBuiltCraftPositions.ContainsKey("Kerbin"));
    // the construction parts (parts whose ModuleCargoPart has a packedVolume >= 0)
    REQUIRE(FP3::ContractDefs::constructionParts != nullptr);
    auto& cp = FP3::ContractDefs::constructionParts->PartsForContractType("OrbitalConstruction");
    const json& ecp = r["constructionParts"];
    size_t n = 0;
    for (const json& e : ecp) {
        if (std::find(e["types"].begin(), e["types"].end(), "OrbitalConstruction") == e["types"].end()) continue;
        REQUIRE(n < cp.size());
        CHECK(cp[n]->partName == e["name"].get<std::string>());
        CHECK(cp[n]->availablePart != nullptr);
        ++n;
    }
    CHECK(n == cp.size());
    CHECK(n > 0);
    CHECK(Contracts::Lite::ContainsPart(cp, cp[0]->partName));
    CHECK_FALSE(Contracts::Lite::ContainsPart(cp, "noSuchPart"));
    // the stock craft files are empty files in the mirror: every stock PREBUILTCRAFT definition is invalid (reported, not hand-authored)
    ConfigNode::NodePtr stock = ConfigNode::Load(ksp::testing::GameRoot() + "/GameData/Squad/Contracts/PrebuiltCraft/RoverContract/roverContractCraft.cfg");
    REQUIRE(stock);
    auto none = FP3::ContractDefs::GetPreBuiltCraft("test", *stock->GetNode("CONTRACTCRAFT"));
    CHECK(none->CraftForContractType("RoverConstruction").empty());
}

TEST_CASE("WP-38c RoverConstruction / OrbitalConstruction / VesselRepair contracts: generated trees (targets, craft, positions, orbits, parameters, texts, rewards, agent, id) vs the reference") {
    const json& ref = RefT();
    std::map<std::string, std::pair<int, int>> counts;
    int thrown = 0, paramsChecked = 0, orbitCases = 0, groundCases = 0;
    int64_t maxUlpEnd = 0;
    for (size_t si = 0; si < ref["scenarios"].size(); ++si) {
        World38b w;
        PreBuiltFixture fx;
        ContractSystem cs;
        cs.Awake();
        ConfigNode weights = WeightsNode38b(ref["weights"]);
        cs.LoadNow(weights);
        LoadScenario38b(w, ref["scenarios"][si]);
        fx.Reload();
        int curMult = -1;
        for (const json& e : ref["contracts"]) {
            if (e["scenario"].get<size_t>() != si) continue;
            if (e["mult"].get<int>() != curMult) {
                curMult = e["mult"].get<int>();
                w.SetMult(Ref38b()["mult_sets"][static_cast<size_t>(curMult)]);
            }
            w.nextGuid = GuidOf38b(e["guid"]);
            const int rseed = e["rseed"].get<int>();  // what `new KSPRandom()` takes in this case
            ContractsEnvironment::Current().newRandomSeed = [rseed] { return rseed; };
            const std::string type = e["type"].get<std::string>();
            INFO(type << " scenario " << ref["scenarios"][si]["name"] << " prestige " << e["prestige"] << " seed " << e["seed"] << " mult " << e["mult"]);
            const json& r = e["result"];
            if (!r.is_null() && r.contains("exception")) {
                CHECK_THROWS_AS(cs.GenerateContract(e["seed"].get<int>(), PrestigeOf38b(e["prestige"].get<int>()), type), NullReferenceException);
                ++thrown;
                continue;
            }
            std::unique_ptr<Contract> c = cs.GenerateContract(e["seed"].get<int>(), PrestigeOf38b(e["prestige"].get<int>()), type);
            if (r.is_null()) {
                INFO("unexpected contract: " << (c ? c->Title() : std::string()));
                CHECK(c == nullptr);
                ++counts[type].second;
                continue;
            }
            REQUIRE(c != nullptr);
            ++counts[type].first;
            CHECK(c->Agent()->Name() == r["agent"].get<std::string>());
            CHECK(c->Agent()->Title() == r["agentTitle"].get<std::string>());
            CHECK(c->Keywords() == r["keywords"].get<std::vector<std::string>>());
            CHECK(c->Title() == r["title"].get<std::string>());
            CHECK(c->Synopsys() == r["synopsis"].get<std::string>());
            CHECK(c->Description() == r["description"].get<std::string>());
            CHECK(c->MessageCompleted() == r["completed"].get<std::string>());
            CHECK(c->ContractID() == r["contractID"].get<long long>());
            CHECK(dotnet::EnumToString(c->Prestige()) == r["prestige"].get<std::string>());
            CHECK_F64b(c->FundsAdvance, r["fa"].get<double>());
            CHECK_F64b(c->FundsCompletion, r["fc"].get<double>());
            CHECK_F64b(c->FundsFailure, r["ff"].get<double>());
            CHECK_F32b(c->ReputationCompletion, F38b(r["rc"]));
            CHECK_F32b(c->ReputationFailure, F38b(r["rf"]));
            CHECK_F32b(c->ScienceCompletion, F38b(r["sc"]));
            CHECK_F64b(c->TimeExpiry, r["expiry"].get<double>());
            CHECK_F64b(c->TimeDeadline, r["deadline"].get<double>());
            CHECK(c->CanBeCancelled());
            CHECK(c->CanBeDeclined());
            REQUIRE(c->ParameterCount() == static_cast<int>(r["params"].size()));
            for (int i = 0; i < c->ParameterCount(); ++i) {
                ContractParameter* p = c->GetParameter(i);
                const json& pr = r["params"][static_cast<size_t>(i)];
                CHECK(p->TypeName() == pr["type"].get<std::string>());
                CHECK(p->HashString() == pr["hash"].get<std::string>());
                CHECK(p->Title() == pr["title"].get<std::string>());
                if (pr["type"] == "RoverWayPointParameter") {
                    auto* rp = dynamic_cast<P38::RoverWayPointParameter*>(p);
                    REQUIRE(rp != nullptr);
                    CHECK_F64n(rp->craftStartLatitude, pr["start"][0].get<double>());
                    CHECK_F64n(rp->craftStartLongitude, pr["start"][1].get<double>());
                    CheckUlp(rp->craftEndLatitude, pr["end"][0].get<double>(), maxUlpEnd);
                    CheckUlp(rp->craftEndLongitude, pr["end"][1].get<double>(), maxUlpEnd);
                }
                ++paramsChecked;
            }
            auto checkOrbit = [&](const Orbit* o, const json& el) {
                REQUIRE(o != nullptr);
                // the draw level elements; the final orbit went through Orbit.UpdateFromStateVectors (see tests/test_fineprint.cpp, GenerateOrbit)
                CHECK(std::fabs(o->semiMajorAxis - el["sma"].get<double>()) <= 1e-12 * std::fabs(el["sma"].get<double>()));
                CHECK(std::fabs(std::fabs(o->eccentricity) - std::fabs(el["ecc"].get<double>())) <= 1e-9);
                CHECK(std::fabs(o->inclination - el["inc"].get<double>()) <= 1e-9);
            };
            if (type == "RoverConstructionContract") {
                auto* t = dynamic_cast<Templates::RoverConstructionContract*>(c.get());
                REQUIRE(t != nullptr);
                CHECK(t->TargetBody()->name() == r["targetBody"].get<std::string>());
                CHECK(GameDataTail(t->roverCraftDefinition->craftURL) == r["craftURL"].get<std::string>());
                CHECK_F64n(t->CraftStartLatitude(), r["start"][0].get<double>());
                CHECK_F64n(t->CraftStartLongitude(), r["start"][1].get<double>());
                CheckUlp(t->CraftEndLatitude(), r["end"][0].get<double>(), maxUlpEnd);
                CheckUlp(t->CraftEndLongitude(), r["end"][1].get<double>(), maxUlpEnd);
                ++groundCases;
            } else if (type == "OrbitalConstructionContract") {
                auto* t = dynamic_cast<Templates::OrbitalConstructionContract*>(c.get());
                REQUIRE(t != nullptr);
                CHECK(t->TargetBody()->name() == r["targetBody"].get<std::string>());
                CHECK(GameDataTail(t->craftDefinition->craftURL) == r["craftURL"].get<std::string>());
                CHECK(t->ConstructionPartName() == r["partName"].get<std::string>());
                CHECK(t->VesselName() == r["vesselName"].get<std::string>());
                CHECK_F64n(t->OrbitEccentricity(), r["eccentricity"].get<double>());
                CHECK_F64n(t->OrbitAltitudeFactor(), r["altitudeFactor"].get<double>());
                CHECK_F64n(t->OrbitInclinationFactor(), r["inclinationFactor"].get<double>());
                checkOrbit(t->orbit.get(), r["orbit"]);
                ++orbitCases;
            } else {
                auto* t = dynamic_cast<Templates::VesselRepairContract*>(c.get());
                REQUIRE(t != nullptr);
                CHECK(t->TargetBody()->name() == r["targetBody"].get<std::string>());
                CHECK(GameDataTail(t->repairCraftDefinition->craftURL) == r["craftURL"].get<std::string>());
                CHECK(t->RepairPartName() == r["partName"].get<std::string>());
                CHECK(t->VesselName() == r["vesselName"].get<std::string>());
                CHECK(t->spawnOnGround == r["spawnOnGround"].get<bool>());
                CHECK(t->spawnInOrbit == r["spawnInOrbit"].get<bool>());
                CHECK_F64n(t->CraftStartLatitude(), r["start"][0].get<double>());
                CHECK_F64n(t->CraftStartLongitude(), r["start"][1].get<double>());
                CHECK_F64n(t->OrbitEccentricity(), r["eccentricity"].get<double>());
                CHECK_F64n(t->OrbitAltitudeFactor(), r["altitudeFactor"].get<double>());
                CHECK_F64n(t->OrbitInclinationFactor(), r["inclinationFactor"].get<double>());
                if (t->spawnInOrbit) {
                    checkOrbit(t->orbit.get(), r["orbit"]);
                    ++orbitCases;
                } else {
                    ++groundCases;
                }
            }
        }
    }
    MESSAGE("generated / refused: Rover " << counts["RoverConstructionContract"].first << "/" << counts["RoverConstructionContract"].second << ", Orbital "
                                          << counts["OrbitalConstructionContract"].first << "/" << counts["OrbitalConstructionContract"].second << ", Repair "
                                          << counts["VesselRepairContract"].first << "/" << counts["VesselRepairContract"].second << "; NullReferenceException " << thrown << "; max ulp of the end coordinates " << maxUlpEnd
                                          << "; parameters checked " << paramsChecked << "; orbit cases " << orbitCases << ", ground cases " << groundCases);
    for (const char* t : {"RoverConstructionContract", "OrbitalConstructionContract", "VesselRepairContract"}) {
        CHECK(counts[t].first > 0);
        CHECK(counts[t].second > 0);
    }
    CHECK(thrown > 0);
    CHECK(orbitCases > 0);
    CHECK(groundCases > 0);
}

// ---- behaviour: derived from the decompiled C# (each check names the member it follows) --------------------------------------------------------
namespace {

// the first reference case of a contract type in the scenario "everything" that was generated and is not an exception
const json* FirstGenerated(const std::string& type, int prestige, std::function<bool(const json&)> pred = nullptr) {
    for (const json& e : RefT()["contracts"]) {
        if (e["scenario"].get<size_t>() != 9 || e["type"] != type || e["prestige"].get<int>() != prestige || e["mult"].get<int>() != 0) continue;
        const json& r = e["result"];
        if (r.is_null() || r.contains("exception")) continue;
        if (pred && !pred(r)) continue;
        return &e;
    }
    return nullptr;
}

struct TemplateRig {
    World38b w;
    PreBuiltFixture fx;
    ContractSystem cs;
    explicit TemplateRig(size_t scenario = 9) {
        cs.Awake();
        ConfigNode weights = WeightsNode38b(RefT()["weights"]);
        cs.LoadNow(weights);
        LoadScenario38b(w, RefT()["scenarios"][scenario]);
        fx.Reload();
    }
    std::unique_ptr<Contract> Make(const json& e) {
        w.SetMult(Ref38b()["mult_sets"][static_cast<size_t>(e["mult"].get<int>())]);
        w.nextGuid = GuidOf38b(e["guid"]);
        const int rseed = e["rseed"].get<int>();
        ContractsEnvironment::Current().newRandomSeed = [rseed] { return rseed; };
        std::unique_ptr<Contract> c = cs.GenerateContract(e["seed"].get<int>(), PrestigeOf38b(e["prestige"].get<int>()), e["type"].get<std::string>());
        REQUIRE(c != nullptr);
        return c;
    }
};

}  // namespace

TEST_CASE("WP-38c RoverConstructionContract: OnAccepted spawn request, vessel events, node round trip") {
    TemplateRig t;
    const json* e = FirstGenerated("RoverConstructionContract", 1);
    REQUIRE(e != nullptr);
    std::unique_ptr<Contract> c = t.Make(*e);
    auto* rc = dynamic_cast<Templates::RoverConstructionContract*>(c.get());
    REQUIRE(rc != nullptr);
    // OnAccepted: a landed pre-built rover at the start position, named after the contract seed
    std::vector<Contracts::Lite::PreBuiltVesselSituation> requests;
    Contracts::Lite::PreBuiltEnvironment::spawnPreBuiltShip = [&](Contracts::Lite::PreBuiltCraftDefinition& def, const Contracts::Lite::PreBuiltVesselSituation& s) {
        CHECK(def.craftURL == s.craftFile);
        requests.push_back(s);
    };
    c->state = Contract::State::Active;
    c->Register();
    rc->OnAccepted();
    REQUIRE(requests.size() == 1);
    CHECK(requests[0].targetBody == rc->targetBody);
    CHECK_F64n(requests[0].latitude, rc->craftStartLatitude);
    CHECK_F64n(requests[0].longitude, rc->craftStartLongitude);
    CHECK(requests[0].situation == Contracts::Lite::PreBuiltVesselSituation::StartSituation::LANDED);
    CHECK_FALSE(requests[0].playerCreated);
    CHECK_FALSE(requests[0].focusOnSpawn);
    CHECK(requests[0].vesselName == rc->CreateVesselName(rc->targetBody->bodyDisplayName));
    CHECK(rc->waitingForVslSpawn);
    // the spawned vessel arrives through GameEvents.Contract.onContractPreBuiltVesselSpawned
    VRig vrig(t.w, *rc->targetBody, {"mk1pod.v2"}, Vessel::Situations::LANDED);
    vrig.v.persistentId = 777;
    vrig.v.vesselName = requests[0].vesselName;
    ksp::events::GameEvents::Contract::onContractPreBuiltVesselSpawned.Fire(&vrig.v);
    CHECK(rc->RoverVesselId() == 777u);
    CHECK_FALSE(rc->waitingForVslSpawn);
    auto* wp = rc->GetParameterT<P38::RoverWayPointParameter>();
    REQUIRE(wp != nullptr);
    CHECK(wp->RoverVslId() == 777u);
    CHECK(wp->RoverVessel() == &vrig.v);
    // a second spawn event is ignored (not waiting any more)
    vrig.v.persistentId = 778;
    ksp::events::GameEvents::Contract::onContractPreBuiltVesselSpawned.Fire(&vrig.v);
    CHECK(rc->RoverVesselId() == 777u);
    // persistentId changes are followed
    ksp::events::GameEvents::onVesselPersistentIdChanged.Fire(777u, 888u);
    CHECK(rc->RoverVesselId() == 888u);
    ksp::events::GameEvents::onVesselPersistentIdChanged.Fire(1u, 2u);
    CHECK(rc->RoverVesselId() == 888u);
    // node round trip: OnSave -> a fresh contract of the same type loaded through the ContractSystem
    ConfigNode node("CONTRACT");
    c->Save(node);
    CHECK(node.HasValue("bodyName"));
    CHECK(node.HasValue("roverCraftDef"));
    CHECK(*node.GetValue("roverVslId") == "888");
    {
        ConfigNode save("SCENARIO");
        save.AddValue("name", "ContractSystem");
        ConfigNode* contracts = save.AddNode("CONTRACTS");
        contracts->AddNode(node.CreateCopy());
        ContractSystem cs2;
        cs2.Awake();
        cs2.LoadNow(save);
        REQUIRE(cs2.Contracts().size() == 1);
        auto* back = dynamic_cast<Templates::RoverConstructionContract*>(cs2.Contracts()[0].get());
        REQUIRE(back != nullptr);
        CHECK(back->targetBody == rc->targetBody);
        CHECK(back->roverCraftDefinition == rc->roverCraftDefinition);
        CHECK_F64n(back->craftStartLatitude, rc->craftStartLatitude);
        CHECK_F64n(back->craftEndLongitude, rc->craftEndLongitude);
        CHECK(back->RoverVesselId() == 888u);
        ConfigNode again("CONTRACT");
        cs2.Contracts()[0]->Save(again);
        CHECK(again.ToString() == node.ToString());
        cs2.Contracts()[0]->Unregister();
    }
    // OnVesselDestroy of the contract's rover fails the contract
    vrig.v.persistentId = 888;
    ksp::events::GameEvents::onVesselDestroy.Fire(&vrig.v);
    CHECK(c->ContractState() == Contract::State::Failed);
    Contracts::Lite::PreBuiltEnvironment::spawnPreBuiltShip = nullptr;
}

TEST_CASE("WP-38c OrbitalConstructionContract: OnAccepted spawn request (orbit), vessel events, node round trip, requirements") {
    TemplateRig t;
    const json* e = FirstGenerated("OrbitalConstructionContract", 0);
    REQUIRE(e != nullptr);
    std::unique_ptr<Contract> c = t.Make(*e);
    auto* oc = dynamic_cast<Templates::OrbitalConstructionContract*>(c.get());
    REQUIRE(oc != nullptr);
    std::vector<Contracts::Lite::PreBuiltVesselSituation> requests;
    Contracts::Lite::PreBuiltEnvironment::spawnPreBuiltShip = [&](Contracts::Lite::PreBuiltCraftDefinition&, const Contracts::Lite::PreBuiltVesselSituation& s) { requests.push_back(s); };
    c->state = Contract::State::Active;
    c->Register();
    oc->OnAccepted();
    REQUIRE(requests.size() == 1);
    CHECK(requests[0].hasOrbit);
    CHECK(requests[0].situation == Contracts::Lite::PreBuiltVesselSituation::StartSituation::ORBITING);
    CHECK(requests[0].orbit.referenceBody == oc->targetBody);
    CHECK_F64n(requests[0].orbit.semiMajorAxis, oc->orbit->semiMajorAxis);
    CHECK(requests[0].vesselName == oc->VesselName());
    CHECK(requests[0].craftFile == oc->craftDefinition->craftURL);
    CHECK(oc->waitingForVslSpawn);
    VRig vrig(t.w, *oc->targetBody, {"mk1pod.v2"}, Vessel::Situations::ORBITING);
    vrig.v.persistentId = 4711;
    ksp::events::GameEvents::Contract::onContractPreBuiltVesselSpawned.Fire(&vrig.v);
    CHECK(oc->ConstructionVesselId() == 4711u);
    auto* cp = oc->GetParameterT<P38::ConstructionParameter>();
    REQUIRE(cp != nullptr);
    CHECK(cp->VesselPersistentId() == 4711u);
    CHECK(cp->PartName() == oc->ConstructionPartName());
    CHECK(cp->VesselName() == oc->VesselName());
    ksp::events::GameEvents::onVesselPersistentIdChanged.Fire(4711u, 4712u);
    CHECK(oc->ConstructionVesselId() == 4712u);
    // OnSave / OnLoad
    ConfigNode node("CONTRACT");
    c->Save(node);
    CHECK(*node.GetValue("constructionPartName") == oc->ConstructionPartName());
    CHECK(*node.GetValue("vesselName") == oc->VesselName());
    CHECK(node.HasValue("constructionCraftDef"));
    {
        ConfigNode save("SCENARIO");
        save.AddValue("name", "ContractSystem");
        save.AddNode("CONTRACTS")->AddNode(node.CreateCopy());
        ContractSystem cs2;
        cs2.Awake();
        cs2.LoadNow(save);
        REQUIRE(cs2.Contracts().size() == 1);
        auto* back = dynamic_cast<Templates::OrbitalConstructionContract*>(cs2.Contracts()[0].get());
        REQUIRE(back != nullptr);
        CHECK(back->craftDefinition == oc->craftDefinition);
        CHECK(back->constructionPart != nullptr);  // OnLoad: PartLoader.getPartInfoByName(constructionPartName)
        REQUIRE(back->orbit != nullptr);           // OnLoad regenerates the orbit from the factors
        CHECK_F64n(back->orbit->semiMajorAxis, oc->orbit->semiMajorAxis);
        ConfigNode again("CONTRACT");
        cs2.Contracts()[0]->Save(again);
        CHECK(again.ToString() == node.ToString());
        cs2.Contracts()[0]->Unregister();
    }
    // the map renderer is set up in the tracking station for an offered contract (ContractDefs.DisplayOfferedOrbits)
    int setups = 0;
    L38::FinePrintEnvironment::setupOrbitRenderer = [&](Contract&, Orbit&) { ++setups; };
    L38::FinePrintEnvironment::cleanupOrbitRenderer = [] {};
    {
        SceneScope sc(GameScenes::TRACKSTATION);
        ConfigNode save("SCENARIO");
        save.AddValue("name", "ContractSystem");
        ConfigNode* cn = save.AddNode("CONTRACTS")->AddNode(node.CreateCopy());
        cn->SetValue("state", "Offered", true);
        ContractSystem cs3;
        cs3.Awake();
        cs3.LoadNow(save);
        CHECK(setups == 1);
        for (auto& x : cs3.Contracts()) x->Unregister();
    }
    L38::FinePrintEnvironment::setupOrbitRenderer = nullptr;
    L38::FinePrintEnvironment::cleanupOrbitRenderer = nullptr;
    // vessel destruction fails the contract
    vrig.v.persistentId = 4712;
    ksp::events::GameEvents::onVesselDestroy.Fire(&vrig.v);
    CHECK(c->ContractState() == Contract::State::Failed);
    Contracts::Lite::PreBuiltEnvironment::spawnPreBuiltShip = nullptr;
}

TEST_CASE("WP-38c VesselRepairContract: OnAccepted (orbit / landed), the broken part of the spawned vessel, node round trip") {
    TemplateRig t;
    // one orbit contract and one landed contract
    const json* eo = FirstGenerated("VesselRepairContract", 0, [](const json& r) { return r["spawnInOrbit"].get<bool>(); });
    const json* eg = FirstGenerated("VesselRepairContract", 1, [](const json& r) { return r["spawnOnGround"].get<bool>(); });
    REQUIRE(eo != nullptr);
    REQUIRE(eg != nullptr);
    std::vector<Contracts::Lite::PreBuiltVesselSituation> requests;
    Contracts::Lite::PreBuiltEnvironment::spawnPreBuiltShip = [&](Contracts::Lite::PreBuiltCraftDefinition&, const Contracts::Lite::PreBuiltVesselSituation& s) { requests.push_back(s); };
    {
        std::unique_ptr<Contract> c = t.Make(*eo);
        auto* vr = dynamic_cast<Templates::VesselRepairContract*>(c.get());
        REQUIRE(vr != nullptr);
        c->state = Contract::State::Active;
        c->Register();
        vr->OnAccepted();
        REQUIRE(requests.size() == 1);
        CHECK(requests[0].hasOrbit);
        CHECK(requests[0].situation == Contracts::Lite::PreBuiltVesselSituation::StartSituation::ORBITING);
        CHECK(requests[0].vesselName == vr->VesselName());
        CHECK(vr->waitingForVslSpawn);
        // the spawned vessel: a ProtoVessel whose solar panel part has a BROKEN module (deployState)
        VRig vrig(t.w, *vr->targetBody, {"mk1pod.v2", "solarPanels1"}, Vessel::Situations::ORBITING);
        auto pv = std::make_shared<ProtoVessel>(vrig.v);
        pv->persistentId = 9100;
        vrig.v.protoVessel = pv.get();
        ProtoPartSnapshot* panel = nullptr;
        for (auto& ps : pv->protoPartSnapshots)
            if (ps->partName.value_or("") == vr->RepairPartName()) panel = ps.get();
        REQUIRE(panel != nullptr);
        {
            panel->persistentId = 6001;
            for (auto& m : panel->modules) m->moduleValues->SetValue("deployState", "BROKEN", true);
        }
        const AvailablePart* info = t.w.g.data.parts.getPartInfoByName(vr->RepairPartName());
        REQUIRE(info != nullptr);
        ksp::events::GameEvents::Contract::onContractPreBuiltVesselSpawned.Fire(&vrig.v);
        CHECK(vr->RepairVesselId() == 9100u);
        CHECK_FALSE(vr->waitingForVslSpawn);
        auto* rp = vr->GetParameterT<P38::RepairPartParameter>();
        REQUIRE(rp != nullptr);
        CHECK(rp->VesselPersistentId() == 9100u);
        CHECK(rp->PartName() == vr->RepairPartName());
        CHECK(rp->VesselName() == vr->VesselName());
        CHECK(rp->PartPersistentId() == 6001u);
        // OnSave / OnLoad (spawnOrbit is only written for a landed craft)
        ConfigNode node("CONTRACT");
        c->Save(node);
        CHECK_FALSE(node.HasValue("spawnOrbit"));
        CHECK(*node.GetValue("repairVslId") == "9100");
        CHECK(*node.GetValue("repairPartName") == vr->RepairPartName());
        ksp::events::GameEvents::onVesselPersistentIdChanged.Fire(9100u, 9200u);
        CHECK(vr->RepairVesselId() == 9200u);
        vrig.v.protoVessel = nullptr;
        vrig.v.persistentId = 9200;
        ksp::events::GameEvents::onVesselDestroy.Fire(&vrig.v);
        CHECK(c->ContractState() == Contract::State::Failed);
    }
    {
        requests.clear();
        std::unique_ptr<Contract> c = t.Make(*eg);
        auto* vr = dynamic_cast<Templates::VesselRepairContract*>(c.get());
        REQUIRE(vr != nullptr);
        c->state = Contract::State::Active;
        c->Register();
        vr->OnAccepted();
        REQUIRE(requests.size() == 1);
        CHECK_FALSE(requests[0].hasOrbit);
        CHECK(requests[0].situation == Contracts::Lite::PreBuiltVesselSituation::StartSituation::LANDED);
        CHECK_F64n(requests[0].latitude, vr->craftStartLatitude);
        CHECK_F64n(requests[0].longitude, vr->craftStartLongitude);
        ConfigNode node("CONTRACT");
        c->Save(node);
        CHECK(*node.GetValue("spawnOrbit") == "Landed");
        // OnLoad restores the landed flags
        ConfigNode save("SCENARIO");
        save.AddValue("name", "ContractSystem");
        save.AddNode("CONTRACTS")->AddNode(node.CreateCopy());
        ContractSystem cs2;
        cs2.Awake();
        cs2.LoadNow(save);
        REQUIRE(cs2.Contracts().size() == 1);
        auto* back = dynamic_cast<Templates::VesselRepairContract*>(cs2.Contracts()[0].get());
        REQUIRE(back != nullptr);
        CHECK(back->spawnOnGround);
        CHECK_FALSE(back->spawnInOrbit);
        CHECK(back->orbit == nullptr);
        CHECK(back->repairCraftDefinition == vr->repairCraftDefinition);
        CHECK_F64n(back->craftStartLatitude, vr->craftStartLatitude);
        ConfigNode again("CONTRACT");
        cs2.Contracts()[0]->Save(again);
        CHECK(again.ToString() == node.ToString());
        cs2.Contracts()[0]->Unregister();
        c->Unregister();
    }
    Contracts::Lite::PreBuiltEnvironment::spawnPreBuiltShip = nullptr;
}

TEST_CASE("WP-38c construction templates: requirements (R&D / part purchase seams), list mutation of the cached craft lists, NullReferenceException without loaded craft defs") {
    TemplateRig t;
    // MeetRequirements
    Templates::VesselRepairContract vr;
    CHECK(vr.MeetRequirements());  // evaRepairKit exists and PartModelPurchased is true by default
    ContractsEnvironment::Current().partModelPurchased = [](const std::string&) { return false; };
    CHECK_FALSE(vr.MeetRequirements());
    ContractsEnvironment::Current().partModelPurchased = nullptr;
    Templates::OrbitalConstructionContract oc;
    CHECK(oc.MeetRequirements());  // no R&D: every technology node is Available
    Templates::RoverConstructionContract rc;
    CHECK(rc.MeetRequirements());  // an orbited body besides the home world exists in the scenario "everything"
}

TEST_CASE("WP-38c construction templates: VesselRepair removes crafts without exactly one broken part from the cached list, Generate fails without craft defs") {
    TemplateRig t;
    auto& list = FP3::ContractDefs::PreBuiltCraftDefs->CraftForContractType("VesselRepair");
    const size_t before = list.size();
    const json* e = FirstGenerated("VesselRepairContract", 0);
    REQUIRE(e != nullptr);
    t.Make(*e);
    // the 2-broken-part craft (P1) was removed from the cache by Generate (sic: the C# edits the cached list)
    CHECK(list.size() < before);
    for (auto& d : list) CHECK(d->brokenPartNames().size() == 1);
    // without ContractDefs.PreBuiltCraftDefs the C# throws a NullReferenceException
    FP3::ContractDefs::PreBuiltCraftDefs.reset();
    Templates::RoverConstructionContract rc;
    CHECK_THROWS_AS(t.cs.GenerateContract(5, ContractPrestige::Trivial, "RoverConstructionContract"), NullReferenceException);
}
