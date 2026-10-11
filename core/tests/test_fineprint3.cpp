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
