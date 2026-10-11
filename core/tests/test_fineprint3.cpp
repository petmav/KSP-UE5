// WP-38 slice 3 unit tests: FinePrint.ContractDefs (full), Sentinel, CompatibilityUtilities, the pre-built craft model and the WP-37 deferred
// templates (core/include/ksp/fineprint/{contract_defs,sentinel,compatibility,prebuilt}.hpp).
//
// Expectations come from tools/wp38c/wp38c_reference.py (tests/data/wp38c_reference.json): an independent Python model that reads the decompiled C#
// and the mirror cfgs and does not import kspcore.
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>

#include "doctest/doctest.h"
#include "game_root.hpp"
#include "ksp/contracts/contract_environment.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/fineprint/contract_defs.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/save/save_environment.hpp"
#include "nlohmann/json.hpp"

using namespace ksp;
using json = nlohmann::json;
namespace FP3 = ksp::FinePrint;

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
