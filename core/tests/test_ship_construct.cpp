// WP-18 unit tests: ShipConstruct over live parts (LoadShip/SaveShip, costs, masses, naming, resource sets), ShipTemplate,
// ShipConstruction (paths, sanitising, backups, templates, AssembleForLaunch, launch-site queries, craft size).
//
// The numerics that exist in a capture are gated by `capture.ship_construct` (C5 craft masses). Everything here is derived from the
// decompiled C# control flow or taken from original game data: the stock crafts of harness/inputs/Ships (57 load as in the game: the
// others contain mod parts) and analysis/physics/craft, the C5 capture (total mass = no-crew total + n * KerbalCrewMass), the C10
// structure table, the physprofile save, the stock part prefabs of the GameData mirror. No number is invented.
#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <map>
#include <random>
#include <set>
#include <sstream>

#include "ksp/kerbals/crew_manifest.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/celestial_system.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/modules/module_part_variants.hpp"
#include "ksp/modules/vessel_module.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/game_parameters.hpp"
#include "ksp/ship/ship_construction.hpp"
#include "ksp/ship/ship_template.hpp"
#include "ksp/util/ksp_util.hpp"
#include "ksp/vessel/vessel.hpp"
#include "ksp/vessel/vessel_assembly.hpp"
#include "nlohmann/json.hpp"
#include "resource_fixture.hpp"

using namespace ksp;
using namespace ksp::rtest;
using namespace ksp::support;
namespace fs = std::filesystem;
using GE = events::GameEvents;

namespace {

const std::string kHarness18 = KSPCORE_HARNESS_DIR;

struct CraftFile {
    std::string label;
    fs::path path;
};

// every .craft of harness/inputs/Ships (VAB, SPH) and analysis/physics/craft
std::vector<CraftFile> AllCrafts() {
    std::vector<CraftFile> out;
    for (const char* dir : {"VAB", "SPH"}) {
        const fs::path d = fs::path(KSPCORE_HARNESS_DIR) / "inputs" / "Ships" / dir;
        std::vector<fs::path> files;
        for (const auto& e : fs::directory_iterator(d))
            if (e.path().extension() == ".craft") files.push_back(e.path());
        std::sort(files.begin(), files.end());
        for (const auto& f : files) out.push_back({std::string(dir) + "/" + f.filename().string(), f});
    }
    const fs::path physDir = fs::path(KSPCORE_PROJECT_ROOT) / "analysis" / "physics" / "craft";
    if (fs::exists(physDir)) {
        std::vector<fs::path> files;
        for (const auto& e : fs::directory_iterator(physDir))
            if (e.path().extension() == ".craft") files.push_back(e.path());
        std::sort(files.begin(), files.end());
        for (const auto& f : files) out.push_back({"physics/" + f.filename().string(), f});
    }
    return out;
}

// KSPUpgradePipeline.Process as CreateConstructFromTemplate runs it; nullptr when the pipeline fails
ConfigNode::NodePtr Upgraded(const fs::path& file) {
    ConfigNode::NodePtr root = ConfigNode::Load(file.string());
    if (!root) return nullptr;
    const std::string* sn = root->GetValuePtr("ship");
    ConfigNode::NodePtr up;
    KSPUpgradePipeline::Process(root, sn ? *sn : std::string(), SaveUpgradePipeline::LoadContext::Craft,
                                [&](const ConfigNode::NodePtr& n) { up = n; }, [&](KSPUpgradePipeline::UpgradeFailOption, const ConfigNode::NodePtr&) {});
    return up;
}

std::string FirstDiff(const std::string& a, const std::string& b) {
    std::istringstream sa(a), sb(b);
    std::string la, lb;
    int n = 0;
    while (true) {
        const bool ga = static_cast<bool>(std::getline(sa, la));
        const bool gb = static_cast<bool>(std::getline(sb, lb));
        n++;
        if (!ga && !gb) return "";
        if (!ga || !gb || la != lb) return "line " + std::to_string(n) + ": [" + (ga ? la : "<eof>") + "] vs [" + (gb ? lb : "<eof>") + "]";
    }
}

// FlightGlobals' persistentId registries remember the ids of a load (a second load of the same ids re-issues them, as in the game);
// a scene change clears them
void ClearIds() {
    PersistentIdRegistry& ids = SaveEnvironment::Current().persistentIds;
    ids.persistentVesselIds.Clear();
    ids.persistentLoadedPartIds.Clear();
    ids.persistentUnloadedPartIds.Clear();
}

// numeric list "a,b,c" / "a|b|c" / "(a, b, c)" -> floats (float.Parse each)
std::vector<float> Floats(const std::string& v) {
    std::vector<float> out;
    std::string cur;
    auto flush = [&]() {
        if (cur.empty()) return;
        float f = 0;
        if (dotnet::TryParseSingle(cur, f)) out.push_back(f);
        else out.push_back(std::nanf(""));
        cur.clear();
    };
    for (char c : v) {
        if (c == ',' || c == '|' || c == '_') flush();
        else if (c != '(' && c != ')' && c != ' ') cur.push_back(c);
    }
    flush();
    return out;
}
bool SameFloats(const std::string& a, const std::string& b) {
    const auto fa = Floats(a), fb = Floats(b);
    if (fa.size() != fb.size()) return false;
    for (size_t i = 0; i < fa.size(); i++)
        if (!(fa[i] == fb[i])) return false;
    return true;
}
uint32_t CraftIdOf(const std::string& partValue) {  // "name_1234" -> 1234
    return static_cast<uint32_t>(std::stoull(partValue.substr(partValue.rfind('_') + 1)));
}
std::vector<uint32_t> LinkIds(const ConfigNode& part, const char* key) {
    std::vector<uint32_t> out;
    for (int k = 0; k < part.values().Count(); k++)
        if (part.values()[k].name == key) out.push_back(CraftIdOf(part.values()[k].value));
    return out;
}

// A stand-in for the editor's VesselCrewManifest: a crew count and no inventory (ShipConstruct.GetShipMass reads these)
struct StubManifest : VesselCrewManifest {
    int crew = 0;
    float inventoryPerKerbal = 0.0f;   // ProtoCrewMember.InventoryMass() of each crew member
    int assigned = 0;
    int CrewCount() const override { return crew; }
    float GetCrewInventoryCosts() override { return 0.0f; }
    // VesselCrewManifest.cs:131: num += allCrew[i].InventoryMass() per crew member (a float store each)
    float GetCrewInventoryMass() override {
        float num = 0.0f;
        for (int i = 0; i < crew; i++) num = f32store(fx(num) + fx(inventoryPerKerbal));
        return num;
    }
    float GetCrewResourceMass() override { return 0.0f; }
    void AssignCrewToVessel(ShipConstruct&) override { assigned++; }
};

// ProtoCrewMember.InventoryMass() (ProtoCrewMember.cs:775) of an editor kerbal = kerbalEVA's ModuleInventoryPart.GetModuleMass(0, CURRENT)
// (ModuleInventoryPart.cs:1928-1959) over the stored parts of its DEFAULTPARTS (kerbalEVA.cfg: evaChute, evaJetpack), one per slot, quantity 1:
//   num += snapshot.mass * (float)Math.Max(1, quantity);
//   num2 = sum over the snapshot's resources, last to first, of definition.density * (float)amount;  num += num2 * (float)Math.Max(1, quantity)
// All inputs are the part database: the prefab mass of each default part and its RESOURCEs with the resource library's density.
float KerbalInventoryMass(Game15& g) {
    ConfigNode::NodePtr eva = ConfigNode::Load((fs::path(ksp::testing::GameRoot()) / "GameData" / "Squad" / "Parts" / "Prebuilt" / "kerbalEVA.cfg").string());
    REQUIRE(eva);
    ConfigNode* part = eva->GetNode("PART");
    REQUIRE(part != nullptr);
    ConfigNode* inv = nullptr;
    for (ConfigNode* m : part->GetNodes("MODULE"))
        if (m->GetValuePtr("name") && *m->GetValuePtr("name") == "ModuleInventoryPart") inv = m;
    REQUIRE(inv != nullptr);
    ConfigNode* defaults = inv->GetNode("DEFAULTPARTS");
    REQUIRE(defaults != nullptr);
    float num = 0.0f;
    for (int i = 0; i < defaults->values().Count(); i++) {
        const AvailablePart& ap = g.Info(defaults->values()[i].value.c_str());
        const float quantity = 1.0f;   // StoredPart.quantity of a default part; Math.Max(1, quantity)
        num = f32store(fx(num) + fx(ap.partPrefab->fields.mass) * fx(quantity));
        float num2 = 0.0f;
        PartResourceList& res = ap.partPrefab->Resources;
        for (int r = res.Count(); r-- > 0;) {
            const PartResource& pr = res[static_cast<size_t>(r)];
            if (pr.info != nullptr) num2 = f32store(fx(num2) + fx(pr.info->density()) * fx(f32store(pr.amount)));
        }
        num = f32store(fx(num) + fx(num2) * fx(quantity));
    }
    return num;
}

// The stock game + PhysicsGlobals + Planetarium + the Kerbol system, as the flight scene has them (AssembleForLaunch builds a Vessel)
struct World18 {
    Game15 g;
    PhysicsGlobals pg;
    Planetarium planetarium;
    std::unique_ptr<CelestialSystem> sys;
    World18() {
        FlightRegistry::Clear();
        SaveEnvironment& env = SaveEnvironment::Current();
        pg.physicsDatabaseFilename = kHarness18 + "/inputs/Physics.cfg";
        pg.Awake("");
        REQUIRE(pg.LoadDatabase());
        planetarium.Awake();
        sys = CelestialSystem::Spawn(KerbolSystemPrefab(), planetarium);
        env.localBodies = sys->SpawnOrder();
        Vessel::env = VesselEnvironment{};
        VesselModuleManager::CompileModules();
    }
    ~World18() {
        FlightRegistry::Clear();
        sys.reset();
        Planetarium::Zup = Planetarium::AwakeZup(0.0);
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
        return [this](const std::string& n) { return g.data.parts.getPartInfoByName(n); };
    }
};

nlohmann::json LoadJson(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    REQUIRE_MESSAGE(f.good(), path);
    return nlohmann::json::parse(f);
}
const nlohmann::json& TableOf(const nlohmann::json& cap, const char* name) {
    for (const auto& t : cap.at("tables"))
        if (t.at("name") == name) return t;
    FAIL("table " << name);
    static nlohmann::json none;
    return none;
}
size_t ColOf(const nlohmann::json& t, const char* c) {
    size_t i = 0;
    for (const auto& col : t.at("columns")) {
        if (col.at("name") == c) return i;
        i++;
    }
    FAIL("column " << c);
    return 0;
}

// a unique scratch directory removed at the end of the scope
struct TempDir {
    fs::path path;
    TempDir() {
        std::random_device rd;
        path = fs::temp_directory_path() / ("kspcore_wp18_" + std::to_string(rd()));
        fs::create_directories(path);
    }
    ~TempDir() {
        std::error_code ec;
        fs::remove_all(path, ec);
    }
    std::string str() const { return path.string(); }
};

// Restores the process-wide ShipConstruction state a test changes
struct ShipConstructionScope {
    ShipConstructionEnvironment savedEnv;
    std::string savedData;
    ShipConstructionScope() : savedEnv(ShipConstruction::env), savedData(KSPUtil::ApplicationDataPath) {
        ShipConstruction::backups.clear();
        ShipConstruction::ShipConfig = nullptr;
        ShipConstruction::ShipManifest = nullptr;
        ShipConstruction::ShipType = EditorFacility::None;
    }
    ~ShipConstructionScope() {
        ShipConstruction::env = savedEnv;
        KSPUtil::ApplicationDataPath = savedData;
        ShipConstruction::backups.clear();
        ShipConstruction::ShipConfig = nullptr;
        ShipConstruction::ShipManifest = nullptr;
        ShipConstruct::editorRootPart = nullptr;
        ShipConstruct::partTechAvailable = nullptr;
        ShipConstruct::onEditorShipLoaded = nullptr;
        ShipTemplateEnvironment::isExperimentalPart = nullptr;
        ShipTemplateEnvironment::partTechAvailable = nullptr;
        ShipTemplateEnvironment::missionExcludedParts = nullptr;
    }
};

struct LoadedCraft {
    CraftFile file;
    ConfigNode::NodePtr upgraded;  // KSPUpgradePipeline output (the node LoadShip reads)
    std::unique_ptr<ShipConstruct> ship;
};

// Every craft that loads as in the game: the others contain mod parts and are refused by LoadShip
std::vector<LoadedCraft> LoadAllCrafts() {
    std::vector<LoadedCraft> out;
    for (const CraftFile& cf : AllCrafts()) {
        LoadedCraft lc;
        lc.file = cf;
        lc.upgraded = Upgraded(cf.path);
        if (!lc.upgraded) continue;
        lc.ship = std::make_unique<ShipConstruct>();
        std::string err;
        if (!lc.ship->LoadShipWithParts(*lc.upgraded, 0, true, err)) continue;
        out.push_back(std::move(lc));
    }
    return out;
}

}  // namespace

// ====================================================================================================================
// LoadShip over live parts
// ====================================================================================================================

TEST_CASE("WP-18 ShipConstruct.LoadShip creates the live parts: tree, links and ownership match the craft file for every loadable stock craft") {
    if (!GameAvailable()) return;
    Game15 g;
    LogCapture quiet;
    const std::vector<LoadedCraft> crafts = LoadAllCrafts();
    // 51 files of harness/inputs/Ships load (the others contain FAR / Pwings / MLP mod parts, WP-15 STATUS) + the 6 PhysProf crafts
    CHECK(crafts.size() == 57);
    size_t parts = 0, roots = 0;
    for (const LoadedCraft& lc : crafts) {
        CAPTURE(lc.file.label);
        const ShipConstruct& ship = *lc.ship;
        REQUIRE(ship.LiveCount() == static_cast<int>(ship.parts.size()));
        REQUIRE(ship.ownedParts.size() == ship.liveParts.size());
        CHECK(ship.Parts() == ship.liveParts);
        for (int i = 0; i < ship.LiveCount(); i++) {
            const ShipConstruct::PartData& d = ship.parts[static_cast<size_t>(i)];
            Part* p = ship.liveParts[static_cast<size_t>(i)];
            parts++;
            CHECK(p == ship.ownedParts[static_cast<size_t>(i)].get());
            CHECK(p->ship == &ship);                 // ShipConstruct.cs:443 part.ship = this
            CHECK(p->vessel == nullptr);
            CHECK(p->partInfo->name == d.partInfoName);
            CHECK(p->fields.craftID == d.craftID);
            CHECK(p->fields.persistentId == d.persistentId);
            CHECK(p->partTransform.position.Equals(d.position));
            CHECK(p->partTransform.rotation.Equals(d.rotation));
            CHECK(p->fields.packed);                 // :672 part.packed = true
            if (d.parent < 0) {
                CHECK(p->parent == nullptr);
                roots++;
            } else {
                CHECK(p->parent == ship.liveParts[static_cast<size_t>(d.parent)]);
            }
            REQUIRE(p->children.size() == d.children.size());
            for (size_t c = 0; c < d.children.size(); c++) CHECK(p->children[c] == ship.liveParts[static_cast<size_t>(d.children[c])]);
            REQUIRE(p->symmetryCounterparts.size() == d.symmetryCounterparts.size());
            for (size_t c = 0; c < d.symmetryCounterparts.size(); c++)
                CHECK(p->symmetryCounterparts[c] == (d.symmetryCounterparts[c] >= 0 ? ship.liveParts[static_cast<size_t>(d.symmetryCounterparts[c])] : nullptr));
            // the part's attach nodes know their owner and, when the file names a partner, the partner Part
            CHECK(p->srfAttachNode->owner == p);
            for (const AttachNode& an : p->attachNodes) {
                CHECK(an.owner == p);
                if (an.attachedPartId != 0) {
                    const int partner = ship.GetPartByCraftID(an.attachedPartId);
                    REQUIRE(partner >= 0);
                    CHECK(an.attachedPart == ship.liveParts[static_cast<size_t>(partner)]);
                }
            }
            // orgPos / orgRot are relative to the root transform (:670-678): the root part itself sits at the origin
            if (d.parent < 0 && i == 0) {
                CHECK(p->fields.orgPos.x == 0.0f);
                CHECK(p->fields.orgPos.y == 0.0f);
                CHECK(p->fields.orgPos.z == 0.0f);
            }
        }
        CHECK(ship.GetPartByCraftID(0) == -1);  // KSPUtil.cs:2076: id 0 is "none"
        CHECK(ship.AreAllPartsConnected());     // every craft is one tree (a single part with neither parent nor child would fail)
        bool anyControl = false;
        for (Part* p : ship.liveParts) anyControl = anyControl || p->fields.isControlSource > 0;
        CHECK(ship.isControllable() == anyControl);
        CHECK(ship.Contains(ship.liveParts[0]));
        CHECK_FALSE(ship.Contains(nullptr));
    }
    CHECK(roots == crafts.size());  // one root part per craft
    MESSAGE("crafts " << crafts.size() << " parts " << parts);
}

TEST_CASE("WP-18 LoadShip refuses what the game refuses: mod parts, incompatible versions, unknown parts") {
    if (!GameAvailable()) return;
    Game15 g;
    LogCapture quiet;
    // a craft with a part the database does not know (FAR craft: the part itself is stock but other crafts of the corpus use mod parts)
    ConfigNode::NodePtr root = ConfigNode::Load((fs::path(KSPCORE_HARNESS_DIR) / "inputs" / "Ships" / "VAB" / "Kerbal X.craft").string());
    REQUIRE(root);
    ConfigNode::NodePtr up;
    KSPUpgradePipeline::Process(root, "Kerbal X", SaveUpgradePipeline::LoadContext::Craft, [&](const ConfigNode::NodePtr& n) { up = n; },
                                [&](KSPUpgradePipeline::UpgradeFailOption, const ConfigNode::NodePtr&) {});
    REQUIRE(up);
    {   // an unknown part: LoadShip fails with the "craft contains missing parts" text and names the part
        ConfigNode::NodePtr broken = up->CreateCopy();
        broken->nodes()[3]->SetValue("part", "madeUpPart_4294967000");
        ShipConstruct ship;
        std::string err;
        CHECK_FALSE(ship.LoadShipWithParts(*broken, 0, true, err));
        CHECK(err.find("madeUpPart") != std::string::npos);
        CHECK(ship.LiveCount() == 0);
    }
    {   // a craft written by a KSP older than lastCompatible (0.18.0): the version check fails first
        ConfigNode::NodePtr old = up->CreateCopy();
        old->SetValue("version", "0.17.0");
        ShipConstruct ship;
        std::string err;
        CHECK_FALSE(ship.LoadShipWithParts(*old, 0, true, err));
        CHECK_FALSE(err.empty());
        CHECK(ship.LiveCount() == 0);
    }
    {   // a link to a craftID that no part has: "Contains Invalid reference", LoadShip returns false (the file-side half)
        ConfigNode::NodePtr bad = up->CreateCopy();
        bad->nodes()[0]->AddValue("link", "fuelTank_123456789");
        ShipConstruct ship;
        std::string err;
        CHECK_FALSE(ship.LoadShipWithParts(*bad, 0, true, err));
        CHECK(quiet.Has(log::Level::Error, "Contains Invalid reference to part 123456789"));
    }
}

TEST_CASE("WP-18 LoadShip bookkeeping: partTechAvailable, persistentId re-issue, editor hook, naming") {
    if (!GameAvailable()) return;
    Game15 g;
    LogCapture quiet;
    ShipConstructionScope scope;
    ConfigNode::NodePtr up = Upgraded(fs::path(KSPCORE_HARNESS_DIR) / "inputs" / "Ships" / "VAB" / "Kerbal X.craft");
    REQUIRE(up);
    {   // ResearchAndDevelopment.PartTechAvailable(availablePart) == false for one part: shipPartsUnlocked = false (ShipConstruct.cs:441)
        ShipConstruct a;
        std::string err;
        REQUIRE(a.LoadShipWithParts(*up, 0, true, err));
        CHECK(a.shipPartsUnlocked);
        ClearIds();
        ShipConstruct::partTechAvailable = [](const AvailablePart& ap) { return ap.name != "liquidEngineMainsail.v2"; };
        ShipConstruct b;
        REQUIRE(b.LoadShipWithParts(*up, 0, true, err));
        CHECK_FALSE(b.shipPartsUnlocked);
        ShipConstruct::partTechAvailable = nullptr;
    }
    {   // the same persistentIds loaded twice: the second ship gets new part ids (CheckPartpersistentId, :630) and the first ship's ids
        // are unchanged; the vessel persistentId is re-issued (CheckVesselpersistentId)
        ClearIds();
        ShipConstruct a, b;
        std::string err;
        REQUIRE(a.LoadShipWithParts(*up, 0, true, err));
        REQUIRE(b.LoadShipWithParts(*up, 0, true, err));
        CHECK(a.persistentId != b.persistentId);
        int differ = 0;
        for (int i = 0; i < a.LiveCount(); i++)
            if (a.liveParts[static_cast<size_t>(i)]->fields.persistentId != b.liveParts[static_cast<size_t>(i)]->fields.persistentId) differ++;
        CHECK(differ == a.LiveCount());
    }
    {   // the editor hook runs only in the editor scene (:690)
        ClearIds();
        int called = 0;
        ShipConstruct::onEditorShipLoaded = [&](ShipConstruct&) { called++; };
        ShipConstruct a;
        std::string err;
        REQUIRE(a.LoadShipWithParts(*up, 0, true, err));
        CHECK(called == 0);
        ClearIds();
        {
            SceneScope editor(GameScenes::EDITOR);
            ShipConstruct b;
            REQUIRE(b.LoadShipWithParts(*up, 0, true, err));
            CHECK(called == 1);
        }
        ShipConstruct::onEditorShipLoaded = nullptr;
    }
}

// ====================================================================================================================
// SaveShip over live parts
// ====================================================================================================================

TEST_CASE("WP-18 live SaveShip is idempotent: R1 = SaveShip(LoadShip(file)); LoadShip(R1) -> SaveShip == R1 for every loadable craft") {
    if (!GameAvailable()) return;
    Game15 g;
    LogCapture quiet;
    size_t crafts = 0, parts = 0;
    for (const CraftFile& cf : AllCrafts()) {
        ConfigNode::NodePtr up = Upgraded(cf.path);
        if (!up) continue;
        ClearIds();
        ShipConstruct ship;
        std::string err;
        if (!ship.LoadShipWithParts(*up, 0, true, err)) continue;
        crafts++;
        parts += static_cast<size_t>(ship.LiveCount());
        ConfigNode::NodePtr r1 = ship.SaveShip();
        // R2: the saved node goes through LoadShip again (the registries are cleared as a scene change does)
        ClearIds();
        ShipConstruct ship2;
        REQUIRE_MESSAGE(ship2.LoadShipWithParts(*r1, 0, true, err), cf.label << ": " << err);
        ConfigNode::NodePtr r2 = ship2.SaveShip();
        const std::string d = FirstDiff(r1->SaveToString(), r2->SaveToString());
        CHECK_MESSAGE(d.empty(), cf.label << " R1 != R2: " << d);
        // the file is written as KSP writes it: CRLF line ends, UTF-8 without BOM
        const std::string text = r1->SaveToString();
        CHECK(text.find("\r\n") != std::string::npos);
        CHECK(text.compare(0, 3, "\xEF\xBB\xBF") != 0);
    }
    CHECK(crafts == 57);
    CHECK(parts == 4282);
}

TEST_CASE("WP-18 live SaveShip agrees with the craft file on everything the live Part owns") {
    if (!GameAvailable()) return;
    Game15 g;
    LogCapture quiet;
    // Counters of checks per key; every one of them must be exact. What a craft file carries that the live Part does not (documented
    // in STATUS.md WP-18): the file's modules the prefab does not define (skipped by Part.LoadModule), module OnSave values of modules
    // that are not ported yet, FAR / fairing module state, the 1.6-era `partName`.
    std::map<std::string, size_t> checks;
    size_t farOrFairingParts = 0, farOrFairingDiffer = 0;
    for (const CraftFile& cf : AllCrafts()) {
        ConfigNode::NodePtr up = Upgraded(cf.path);
        if (!up) continue;
        ClearIds();
        ShipConstruct ship;
        std::string err;
        if (!ship.LoadShipWithParts(*up, 0, true, err)) continue;
        ConfigNode::NodePtr r1 = ship.SaveShip();
        REQUIRE(r1->nodes().Count() == up->nodes().Count());
        for (int i = 0; i < up->nodes().Count(); i++) {
            const ConfigNode& a = *up->nodes()[i];
            const ConfigNode& b = *r1->nodes()[i];
            const std::string where = cf.label + " part " + std::to_string(i);
            for (const char* k : {"pos", "rot", "attPos", "attPos0", "attRot", "attRot0", "mir"})
                if (a.HasValue(k)) {
                    CHECK_MESSAGE(SameFloats(*a.GetValuePtr(k), *b.GetValuePtr(k)), where << " " << k << " [" << *a.GetValuePtr(k) << "] vs [" << *b.GetValuePtr(k) << "]");
                    checks[k]++;
                }
            for (const char* k : {"istg", "dstg", "sidx", "sqor", "sepI", "attm", "resPri", "symMethod", "autostrutMode", "rigidAttachment", "sameVesselCollision", "cData", "persistentId"})
                if (a.HasValue(k)) {
                    CHECK_MESSAGE(*a.GetValuePtr(k) == *b.GetValuePtr(k), where << " " << k << " [" << *a.GetValuePtr(k) << "] vs [" << *b.GetValuePtr(k) << "]");
                    checks[k]++;
                }
            CHECK_MESSAGE(LinkIds(a, "link") == LinkIds(b, "link"), where << " link");
            CHECK_MESSAGE(LinkIds(a, "sym") == LinkIds(b, "sym"), where << " sym");
            checks["link/sym"]++;
            // the part name written is the AvailablePart name (the pipeline already renamed the upgraded parts)
            CHECK(a.GetValuePtr("part")->substr(0, a.GetValuePtr("part")->rfind('_')) == b.GetValuePtr("part")->substr(0, b.GetValuePtr("part")->rfind('_')));
            // srfN: node id, partner craftID and (in the 1.12 layout) the vectors
            const std::string* sa = a.GetValuePtr("srfN");
            const std::string* sb = b.GetValuePtr("srfN");
            CHECK_MESSAGE((sa != nullptr) == (sb != nullptr), where << " srfN presence");
            if (sa && sb) {
                const auto fa = dotnet::Split(*sa, ",", false), fb = dotnet::Split(*sb, ",", false);
                CHECK_MESSAGE((fa[0] == fb[0] && CraftIdOf(fa[1]) == CraftIdOf(fb[1])), where << " srfN [" << *sa << "] vs [" << *sb << "]");
                if (fa.size() >= 6) CHECK_MESSAGE(SameFloats(fa[3] + "," + fa[4] + "," + fa[5], fb[3] + "," + fb[4] + "," + fb[5]), where << " srfN vectors");
                checks["srfN"]++;
            }
            // attN: every node the file names exists on the live part (prefab nodes) with the same partner craftID
            std::map<std::string, std::string> live;
            for (int k = 0; k < b.values().Count(); k++)
                if (b.values()[k].name == "attN") live.emplace(b.values()[k].value.substr(0, b.values()[k].value.find(',')), b.values()[k].value);
            auto partnerId = [](const std::string& v) {  // "id,Name_craftID_vectors": the craftID between the first two '_' after the comma
                const size_t c = v.find(','), u1 = v.find('_', c), u2 = v.find('_', u1 + 1);
                return v.substr(u1 + 1, u2 == std::string::npos ? std::string::npos : u2 - u1 - 1);
            };
            for (int k = 0; k < a.values().Count(); k++) {
                if (a.values()[k].name != "attN") continue;
                const std::string& v = a.values()[k].value;
                const auto it = live.find(v.substr(0, v.find(',')));
                REQUIRE_MESSAGE(it != live.end(), where << " attN " << v);
                // the file's partner id is the first number after "Name_" (old layout: "Name_id|x|y": cut at '|')
                std::string fileId = partnerId(v);
                fileId = fileId.substr(0, fileId.find('|'));
                CHECK_MESSAGE(fileId == partnerId(it->second).substr(0, partnerId(it->second).find('|')), where << " attN [" << v << "] vs [" << it->second << "]");
                checks["attN"]++;
            }
            // RESOURCE: the file's resources exist on the live part with the same amounts (the live part keeps the prefab's other ones)
            std::map<std::string, std::pair<std::string, std::string>> rb;
            for (int k = 0; k < b.nodes().Count(); k++)
                if (b.nodes()[k]->name == "RESOURCE") rb[*b.nodes()[k]->GetValuePtr("name")] = {*b.nodes()[k]->GetValuePtr("amount"), *b.nodes()[k]->GetValuePtr("maxAmount")};
            for (int k = 0; k < a.nodes().Count(); k++) {
                if (a.nodes()[k]->name != "RESOURCE") continue;
                const ConfigNode& r = *a.nodes()[k];
                const std::string& rn = *r.GetValuePtr("name");
                REQUIRE_MESSAGE(rb.count(rn), where << " RESOURCE " << rn);
                CHECK_MESSAGE(SameFloats(*r.GetValuePtr("amount"), rb[rn].first), where << " amount " << rn);
                CHECK_MESSAGE(SameFloats(*r.GetValuePtr("maxAmount"), rb[rn].second), where << " maxAmount " << rn);
                checks["RESOURCE"]++;
            }
            // MODULE: the live part saves exactly the prefab's modules (Part.LoadModule skips a file module the prefab does not define)
            {
                std::vector<std::string> prefabNames, liveNames;
                const Part* p = ship.liveParts[static_cast<size_t>(i)];
                PartModuleList& mods = const_cast<Part*>(p->partInfo->partPrefab.get())->Modules();
                for (size_t m = 0; m < mods.Count(); m++) prefabNames.push_back(mods.GetModule(m)->moduleName);
                for (int k = 0; k < b.nodes().Count(); k++)
                    if (b.nodes()[k]->name == "MODULE") liveNames.push_back(*b.nodes()[k]->GetValuePtr("name"));
                CHECK_MESSAGE(prefabNames == liveNames, where << " MODULE names");
                checks["MODULE names"]++;
                // a file module value the live module also saves is the same value
                std::map<std::string, int> occA, occB;
                std::map<std::pair<std::string, int>, const ConfigNode*> liveMods;
                for (int k = 0; k < b.nodes().Count(); k++)
                    if (b.nodes()[k]->name == "MODULE") liveMods[{*b.nodes()[k]->GetValuePtr("name"), occB[*b.nodes()[k]->GetValuePtr("name")]++}] = b.nodes()[k];
                for (int k = 0; k < a.nodes().Count(); k++) {
                    if (a.nodes()[k]->name != "MODULE") continue;
                    const ConfigNode& fm = *a.nodes()[k];
                    const std::string mn = *fm.GetValuePtr("name");
                    const auto it = liveMods.find({mn, occA[mn]++});
                    if (it == liveMods.end()) continue;
                    for (int v = 0; v < fm.values().Count(); v++) {
                        const auto& fv = fm.values()[v];
                        const std::string* lv = it->second->GetValuePtr(fv.name);
                        if (lv == nullptr) continue;
                        // ModuleReactionWheel.OnLoad (ModuleReactionWheel.cs:124, WP-24): WheelState = Active sets stateString to its display
                        // description after the field load, so the live module saves "Active" where the craft saved the editor-time ""
                        if (mn == "ModuleReactionWheel" && fv.name == "stateString") continue;
                        CHECK_MESSAGE((fv.value == *lv || SameFloats(fv.value, *lv)), where << " " << mn << "." << fv.name << " [" << fv.value << "] vs [" << *lv << "]");
                        checks["module fields"]++;
                    }
                }
            }
            // modCost / modMass / modSize: exact for every part without a FAR module or a procedural fairing (their module state is
            // not in the prefab: FAR is a mod, ModuleProceduralFairing.panelMass is WP-26)
            bool exempt = false;
            for (int k = 0; k < a.nodes().Count(); k++)
                if (a.nodes()[k]->name == "MODULE") {
                    const std::string& mn = *a.nodes()[k]->GetValuePtr("name");
                    if (mn.rfind("FAR", 0) == 0 || mn == "ModuleProceduralFairing") exempt = true;
                }
            bool differ = false;
            for (const char* k : {"modCost", "modMass", "modSize"}) {
                if (!a.HasValue(k)) continue;
                const bool same = SameFloats(*a.GetValuePtr(k), *b.GetValuePtr(k));
                if (!exempt) CHECK_MESSAGE(same, where << " " << k << " [" << *a.GetValuePtr(k) << "] vs [" << *b.GetValuePtr(k) << "]");
                differ = differ || !same;
                checks[k]++;
            }
            if (exempt) {
                farOrFairingParts++;
                if (differ) farOrFairingDiffer++;
            }
        }
    }
    for (const char* k : {"pos", "rot", "attPos", "attRot", "istg", "link/sym", "srfN", "attN", "RESOURCE", "MODULE names", "module fields", "modCost", "modMass", "modSize"})
        CHECK_MESSAGE(checks[k] > 500, k << " " << checks[k]);
    CHECK(farOrFairingParts == 452);  // measured: parts of the corpus with a FAR* module or a ModuleProceduralFairing
    CHECK(farOrFairingDiffer > 0);    // and those are the only parts where the live mass / cost / size differ from the file's
}

// ====================================================================================================================
// ShipTemplate (what the craft browser reads without instantiating parts)
// ====================================================================================================================

TEST_CASE("WP-18 ShipTemplate reads the craft file: name, description, type, counts, stages, controllability, root part") {
    if (!GameAvailable()) return;
    Game15 g;
    LogCapture quiet;
    size_t templates = 0, controllable = 0, withKnownRoot = 0, moddedCrafts = 0;
    for (const CraftFile& cf : AllCrafts()) {
        ConfigNode::NodePtr up = Upgraded(cf.path);
        if (!up) continue;
        {   // a craft with parts the database does not know (mods): the template still reads, but is locked
            bool allKnown = true;
            for (int i = 0; i < up->nodes().Count(); i++)
                allKnown = allKnown && g.data.parts.getPartInfoByName(ShipTemplate::GetPartName(*up->nodes()[i])) != nullptr;
            if (!allKnown) {
                ShipTemplate m;
                m.LoadShip(cf.path.string(), up);
                CHECK(m.partCount == up->CountNodes());
                CHECK(m.shipName == *up->GetValuePtr("ship"));
                CHECK_FALSE(m.shipPartsUnlocked);
                moddedCrafts++;
                continue;
            }
        }
        ShipTemplate t;
        t.LoadShip(cf.path.string(), up);
        templates++;
        CAPTURE(cf.label);
        CHECK(t.config == up);
        CHECK(t.filename == cf.path.string());
        CHECK(t.partCount == up->CountNodes());
        CHECK(t.shipName == *up->GetValuePtr("ship"));
        // description: U+00A8 is a line break (the later "description" value wins: the loop runs backwards)
        std::string d = *up->GetValuePtr("description");
        std::string expectedDesc;
        for (size_t i = 0; i < d.size();) {
            if (static_cast<unsigned char>(d[i]) == 0xC2 && i + 1 < d.size() && static_cast<unsigned char>(d[i + 1]) == 0xA8) {
                expectedDesc += '\n';
                i += 2;
            } else {
                expectedDesc += d[i++];
            }
        }
        CHECK(t.shipDescription == expectedDesc);
        CHECK(t.shipType == (*up->GetValuePtr("type") == "SPH" ? 1 : 0));
        if (up->HasValue("size")) CHECK(t.shipSize.Equals(KSPUtil::ParseVector3(*up->GetValuePtr("size"))));
        // stageCount = 1 + max "istg" over the PART nodes (every part of an upgraded craft resolves)
        int maxStage = 0;
        for (int i = 0; i < up->nodes().Count(); i++)
            if (up->nodes()[i]->HasValue("istg")) maxStage = std::max(maxStage, dotnet::ParseInt32(*up->nodes()[i]->GetValuePtr("istg")));
        CHECK(t.stageCount == maxStage + 1);
        // the root part is the one PART no other part links to
        std::set<std::string> linked;
        for (int i = 0; i < up->nodes().Count(); i++)
            for (int k = 0; k < up->nodes()[i]->values().Count(); k++)
                if (up->nodes()[i]->values()[k].name == "link") linked.insert(up->nodes()[i]->values()[k].value);
        std::vector<ConfigNode*> unlinked;
        for (int i = 0; i < up->nodes().Count(); i++)
            if (!linked.count(*up->nodes()[i]->GetValuePtr("part"))) unlinked.push_back(up->nodes()[i]);
        REQUIRE(unlinked.size() == 1);
        CHECK(t.rootPartNode.get() == unlinked[0]);
        withKnownRoot++;
        // controllable = the first PART (the root) has a ModuleCommand
        bool cmd = false;
        for (ConfigNode* m : up->GetNode("PART")->GetNodes("MODULE")) cmd = cmd || *m->GetValuePtr("name") == "ModuleCommand";
        CHECK(t.isControllable == cmd);
        controllable += cmd;
        CHECK(t.shipPartsUnlocked);
        CHECK_FALSE(t.shipPartsExperimental);
        CHECK_FALSE(t.duplicatedParts);
        CHECK(ShipTemplate::GetPartName(*up->nodes()[0]) == up->nodes()[0]->GetValuePtr("part")->substr(0, up->nodes()[0]->GetValuePtr("part")->find('_')));
        CHECK(ShipTemplate::GetPartId(*up->nodes()[0]) == up->nodes()[0]->GetValuePtr("part")->substr(up->nodes()[0]->GetValuePtr("part")->find('_') + 1));
    }
    CHECK(templates == 57);
    CHECK(moddedCrafts == 35);   // 92 craft files (86 of harness/inputs/Ships + 6 of analysis/physics/craft) minus the 57 that load
    CHECK(withKnownRoot == 57);
    CHECK(controllable > 20);
}

TEST_CASE("WP-18 ShipTemplate: duplicate parts, unknown parts, tech availability, experimental parts, mission part filter, template without a root") {
    if (!GameAvailable()) return;
    Game15 g;
    LogCapture quiet;
    ShipConstructionScope scope;
    ConfigNode::NodePtr up = Upgraded(fs::path(KSPCORE_HARNESS_DIR) / "inputs" / "Ships" / "VAB" / "Kerbal 2.craft");
    REQUIRE(up);
    {   // the same "part" value twice: duplicatedParts, shipPartsUnlocked = false, the loop stops (nothing else is read)
        ConfigNode::NodePtr dup = up->CreateCopy();
        dup->nodes()[1]->SetValue("part", *dup->nodes()[0]->GetValuePtr("part"));
        ShipTemplate t;
        t.LoadShip(dup);
        CHECK(t.duplicatedParts);
        CHECK_FALSE(t.shipPartsUnlocked);
        CHECK(t.shipDescription == "");  // description is read after the duplicate test
        CHECK(t.stageCount == 0);
        CHECK(t.rootPartNode == nullptr);
    }
    {   // an unknown part name: logged, shipPartsUnlocked = false, the part contributes nothing to the totals
        ConfigNode::NodePtr unk = up->CreateCopy();
        const std::string orig = *unk->nodes()[2]->GetValuePtr("part");
        unk->nodes()[2]->SetValue("part", "madeUpPart_" + orig.substr(orig.rfind('_') + 1));
        ShipTemplate t, ref;
        t.LoadShip(unk);
        ref.LoadShip(up);
        CHECK_FALSE(t.shipPartsUnlocked);
        CHECK(quiet.Has(log::Level::Error, "No AvailablePart found for madeUpPart"));
        CHECK(t.totalCost < ref.totalCost);
        CHECK(t.totalMass < ref.totalMass);
        CHECK(t.partCount == ref.partCount);
    }
    {   // R&D hooks: IsExperimentalPart -> shipPartsExperimental (the tech test is skipped for it); PartTechAvailable false -> locked
        const std::string rootName = ShipTemplate::GetPartName(*up->nodes()[0]);
        const std::string otherName = ShipTemplate::GetPartName(*up->nodes()[2]);
        ShipTemplateEnvironment::isExperimentalPart = [&](const AvailablePart& ap) { return ap.name == otherName; };
        ShipTemplateEnvironment::partTechAvailable = [&](const AvailablePart& ap) { return ap.name != otherName; };
        ShipTemplate t;
        t.LoadShip(up);
        CHECK(t.shipPartsExperimental);   // IsExperimentalPart(part) -> the else branch (PartTechAvailable) is not evaluated for it
        CHECK(t.shipPartsUnlocked);
        ShipTemplateEnvironment::isExperimentalPart = nullptr;
        ShipTemplate t2;
        t2.LoadShip(up);
        CHECK_FALSE(t2.shipPartsExperimental);
        CHECK_FALSE(t2.shipPartsUnlocked); // not experimental and the tech is not available
        ShipTemplateEnvironment::partTechAvailable = [&](const AvailablePart& ap) { return ap.name != rootName; };
        ShipTemplate t3;
        t3.LoadShip(up);
        CHECK_FALSE(t3.shipPartsUnlocked);
        ShipTemplateEnvironment::partTechAvailable = nullptr;
    }
    {   // Making History mission filter: a part in the excluded list locks the ship; an empty list does not
        ShipTemplateEnvironment::missionExcludedParts = []() -> std::optional<std::vector<std::string>> { return std::vector<std::string>{"fuelTank"}; };
        ShipTemplate t;
        t.LoadShip(up);
        bool has = false;
        for (int i = 0; i < up->nodes().Count(); i++) has = has || up->nodes()[i]->GetValuePtr("part")->rfind("fuelTank_", 0) == 0;
        CHECK(t.shipPartsUnlocked == !has);
        ShipTemplateEnvironment::missionExcludedParts = []() -> std::optional<std::vector<std::string>> { return std::vector<std::string>{}; };
        ShipTemplate t2;
        t2.LoadShip(up);
        CHECK(t2.shipPartsUnlocked);
        ShipTemplateEnvironment::missionExcludedParts = nullptr;
    }
    {   // a PART node without "part": the root-part dictionary does ContainsKey(null) first (ArgumentNullException)
        ConfigNode::NodePtr nop = up->CreateCopy();
        nop->nodes()[4]->RemoveValues("part");
        ShipTemplate t;
        CHECK_THROWS_AS(t.LoadShip(nop), ArgumentNullException);
    }
    {   // a non-PART node without "part" (e.g. a stray node): partCount = 0 (sic) and the node is skipped
        ConfigNode::NodePtr extra = up->CreateCopy();
        extra->AddNode("STRAY");
        ShipTemplate t, ref;
        t.LoadShip(extra);
        ref.LoadShip(up);
        CHECK(t.partCount == 0);
        CHECK(t.totalCost == ref.totalCost);
        CHECK(t.totalMass == ref.totalMass);
    }
    {   // type: the values are visited from the end, so the FIRST "type" value of the file wins
        ConfigNode::NodePtr ty = up->CreateCopy();
        ty->AddValue("type", "SPH");
        ShipTemplate t;
        t.LoadShip(ty);
        CHECK(t.shipType == (*up->GetValuePtr("type") == "SPH" ? 1 : 0));  // the appended "SPH" is visited first, the file's own value last
    }
    {   // GetShipSize returns the stored size (ShipConstruction.CalculateCraftSize(ShipTemplate))
        ShipTemplate t;
        t.LoadShip(up);
        CHECK(t.GetShipSize().Equals(t.shipSize));
        ShipTemplate empty;
        CHECK(empty.GetShipSize().Equals(Vector3f::zero()));
    }
}

// ====================================================================================================================
// Costs and masses
// ====================================================================================================================

namespace {
// the per-part dry / fuel cost and mass exactly as ShipConstruct.GetShipCosts / GetShipMass accumulate them (Mono float rule)
struct PartTotals {
    float dryCost = 0, fuelCost = 0, dryMass = 0, fuelMass = 0;
};
PartTotals LiveTotals(Part& part) {
    PartTotals t;
    const AvailablePart* ap = part.partInfo;
    float c = f32store(fx(ap->cost) + fx(part.GetModuleCosts(ap->cost)));
    float fc = 0.0f, fm = 0.0f;
    for (int i = part.Resources.Count(); i-- > 0;) {
        PartResource& r = part.Resources[static_cast<size_t>(i)];
        c = f32store(fx(c) - fx(r.info->unitCost()) * fx(f32store(r.maxAmount)));
        fc = f32store(fx(fc) + fx(r.info->unitCost()) * fx(f32store(r.amount)));
        fm = f32store(fx(fm) + fx(r.info->density()) * fx(f32store(r.amount)));
    }
    part.UpdateMass();
    t.dryCost = c;
    t.fuelCost = fc;
    t.dryMass = part.fields.mass;
    t.fuelMass = fm;
    return t;
}
}  // namespace

TEST_CASE("WP-18 GetShipCosts / GetShipMass: part sums in the C# order, dry + fuel, template totals, crew mass") {
    if (!GameAvailable()) return;
    World18 w;
    LogCapture quiet;
    ShipConstructionScope scope;
    size_t crafts = 0, exactCost = 0, exactMass = 0;
    for (const CraftFile& cf : AllCrafts()) {
        ConfigNode::NodePtr up = Upgraded(cf.path);
        if (!up) continue;
        ClearIds();
        ShipConstruct ship;
        std::string err;
        if (!ship.LoadShipWithParts(*up, 0, true, err)) continue;
        crafts++;
        CAPTURE(cf.label);
        // GetShipCosts: parts from the last to the first, dry and fuel accumulated separately, total = 0 + (dry + fuel)
        float dry = 0, fuel = 0, mdry = 0, mfuel = 0;
        float eDry = 0, eFuel = 0, eMDry = 0, eMFuel = 0;
        for (int i = ship.LiveCount(); i-- > 0;) {
            const PartTotals t = LiveTotals(*ship.liveParts[static_cast<size_t>(i)]);
            eDry = f32store(fx(eDry) + fx(t.dryCost));
            eFuel = f32store(fx(eFuel) + fx(t.fuelCost));
            eMDry = f32store(fx(eMDry) + fx(t.dryMass));
            eMFuel = f32store(fx(eMFuel) + fx(t.fuelMass));
        }
        const float total = ship.GetShipCosts(dry, fuel);
        CHECK(dry == eDry);
        CHECK(fuel == eFuel);
        CHECK(total == f32store(0.0 + (fx(eDry) + fx(eFuel))));
        const float mtotal = ship.GetShipMass(mdry, mfuel);
        CHECK(mdry == eMDry);
        CHECK(mfuel == eMFuel);
        CHECK(mtotal == f32store(0.0 + (fx(eMDry) + fx(eMFuel))));
        // fuel mass is the resource mass of the parts (Part.GetResourceMass sums density * amount in double)
        double resMass = 0.0;
        for (Part* p : ship.liveParts) resMass += p->GetResourceMass();
        CHECK(std::fabs(static_cast<double>(mfuel) - resMass) <= 1e-5 * std::max(1.0, resMass));
        // the cost of a ship is never negative before resources: dry cost + fuel cost = sum of part costs at current fill
        CHECK(total > 0.0f);
        // a crew manifest adds (float)CrewCount * KerbalCrewMass (+ inventory + resource mass: zero here)
        StubManifest vcm;
        vcm.crew = 3;
        float d2, f2;
        const float withCrew = ship.GetShipMass(d2, f2, &vcm);
        CHECK(d2 == mdry);
        CHECK(f2 == mfuel);
        CHECK(withCrew == f32store(fx(mtotal) + fx(3.0f) * fx(PhysicsGlobals::KerbalCrewMass())));
        // GetTotalMass() uses ShipConstruction.ShipManifest: none -> the no-crew total; with a manifest -> the crew total
        ShipConstruction::ShipManifest = nullptr;
        CHECK(ship.GetTotalMass() == mtotal);
        ShipConstruction::ShipManifest = &vcm;
        CHECK(ship.GetTotalMass() == withCrew);
        ShipConstruction::ShipManifest = nullptr;
        // GetShipCosts with a manifest adds the crew inventory costs (zero for the stub)
        CHECK(ship.GetShipCosts(d2, f2, &vcm) == total);

        // ShipTemplate over the saved node (modCost / modMass / RESOURCE as KSP wrote them): the same ship, summed per part forward
        ConfigNode::NodePtr r1 = ship.SaveShip();
        ShipTemplate t;
        t.LoadShip(r1);
        float fTotalCost = 0, fFuelCost = 0, fDryCost = 0, fTotalMass = 0;
        // per part, GetPartCostsAndMass(PART node) equals the live computation exactly: same inputs (modCost / modMass / amounts)
        for (int i = 0; i < ship.LiveCount(); i++) {
            float dc, fc, dm, fm;
            const AvailablePart* ap = ship.liveParts[static_cast<size_t>(i)]->partInfo;
            const float sum = ShipConstruction::GetPartCostsAndMass(*r1->nodes()[i], ap, dc, fc, dm, fm);
            const PartTotals lt = LiveTotals(*ship.liveParts[static_cast<size_t>(i)]);
            CHECK_MESSAGE(dc == lt.dryCost, cf.label << " part " << i);
            CHECK_MESSAGE(fc == lt.fuelCost, cf.label << " part " << i);
            CHECK_MESSAGE(dm == lt.dryMass, cf.label << " part " << i);
            CHECK_MESSAGE(fm == lt.fuelMass, cf.label << " part " << i);
            CHECK(sum == f32store(fx(dc) + fx(fc)));
            // ShipTemplate.cs:129-143: totalCost += num; fuelCost += num2; dryCost += num3; totalMass += dryMass + fuelMass, forward over the
            // parts, each a float store (Mono rule); the ship adds dry and fuel separately from the last part to the first, so the two
            // totals differ only by association order, and the template's must be reproduced exactly by this forward sum
            fTotalCost = f32store(fx(fTotalCost) + fx(sum));
            fFuelCost = f32store(fx(fFuelCost) + fx(fc));
            fDryCost = f32store(fx(fDryCost) + fx(dc));
            fTotalMass = f32store(fx(fTotalMass) + (fx(dm) + fx(fm)));
        }
        CHECK_MESSAGE(t.totalCost == fTotalCost, cf.label << " " << t.totalCost << " vs " << fTotalCost);
        CHECK_MESSAGE(t.fuelCost == fFuelCost, cf.label);
        CHECK_MESSAGE(t.dryCost == fDryCost, cf.label);
        CHECK_MESSAGE(t.totalMass == fTotalMass, cf.label << " " << t.totalMass << " vs " << fTotalMass);
        // the ship's own association order gives (almost always) the same bits; count the exact ones for the record
        exactCost += (t.totalCost == total);
        exactMass += (t.totalMass == mtotal);
    }
    MESSAGE("crafts " << crafts << " (ship backwards sum == template forward sum: totalCost " << exactCost << ", totalMass " << exactMass << ")");
    CHECK(crafts == 57);
}

TEST_CASE("WP-18 GetPartCostsAndMass / SanitizePartCosts on a hand-built PART node") {
    if (!GameAvailable()) return;
    Game15 g;
    LogCapture quiet;
    const AvailablePart& tank = g.Info("fuelTank");
    const PartResourceDefinition* lf = PartResourceLibrary::Instance()->GetDefinition("LiquidFuel");
    const PartResourceDefinition* ox = PartResourceLibrary::Instance()->GetDefinition("Oxidizer");
    REQUIRE(lf != nullptr);
    REQUIRE(ox != nullptr);
    ConfigNode node("PART");
    node.AddValue("part", "fuelTank_4294967000");
    node.AddValue("modCost", "10.5");
    node.AddValue("modMass", "0.25");
    ConfigNode* r1 = node.AddNode("RESOURCE");
    r1->AddValue("name", "LiquidFuel");
    r1->AddValue("amount", "100");
    r1->AddValue("maxAmount", "360");
    ConfigNode* r2 = node.AddNode("RESOURCE");
    r2->AddValue("name", "Oxidizer");
    r2->AddValue("amount", "40");
    r2->AddValue("maxAmount", "440");
    float dc, fc, dm, fm;
    const float total = ShipConstruction::GetPartCostsAndMass(node, &tank, dc, fc, dm, fm);
    // the C# order: dryCost = cost; += modCost; the RESOURCE nodes from the last to the first
    float eDry = tank.cost;
    eDry = f32store(fx(eDry) + fx(10.5f));
    eDry = f32store(fx(eDry) - fx(ox->unitCost()) * fx(440.0f));
    eDry = f32store(fx(eDry) - fx(lf->unitCost()) * fx(360.0f));
    float eFuel = 0.0f;
    eFuel = f32store(fx(eFuel) + fx(ox->unitCost()) * fx(40.0f));
    eFuel = f32store(fx(eFuel) + fx(lf->unitCost()) * fx(100.0f));
    float eFuelMass = 0.0f;
    eFuelMass = f32store(fx(eFuelMass) + fx(ox->density()) * fx(40.0f));
    eFuelMass = f32store(fx(eFuelMass) + fx(lf->density()) * fx(100.0f));
    CHECK(dc == eDry);
    CHECK(fc == eFuel);
    CHECK(dm == f32store(fx(tank.partPrefab->fields.mass) + fx(0.25f)));
    CHECK(fm == eFuelMass);
    CHECK(total == f32store(fx(eDry) + fx(eFuel)));
    // aP == null: the part is looked up by the name before the first '_'
    float dc2, fc2, dm2, fm2;
    CHECK(ShipConstruction::GetPartCostsAndMass(node, nullptr, dc2, fc2, dm2, fm2) == total);
    CHECK(dc2 == dc);
    // an unknown part: error logged, 0 returned, all out values zero
    ConfigNode unknown("PART");
    unknown.AddValue("part", "noSuchPart_1");
    CHECK(ShipConstruction::GetPartCostsAndMass(unknown, nullptr, dc2, fc2, dm2, fm2) == 0.0f);
    CHECK(quiet.Has(log::Level::Error, "No AvailablePart found for noSuchPart"));
    CHECK(dc2 == 0.0f);
    CHECK(dm2 == 0.0f);
    // a RESOURCE the library does not define: logged, ignored
    ConfigNode unk2("PART");
    unk2.AddValue("part", "fuelTank_1");
    ConfigNode* ur = unk2.AddNode("RESOURCE");
    ur->AddValue("name", "Unobtainium");
    ur->AddValue("amount", "1");
    ur->AddValue("maxAmount", "1");
    ShipConstruction::GetPartCostsAndMass(unk2, &tank, dc2, fc2, dm2, fm2);
    CHECK(quiet.Has(log::Level::Error, "No Resource definition found for RESOURCE"));
    CHECK(dc2 == tank.cost);
    CHECK(fc2 == 0.0f);
    // SanitizePartCosts: a part cheaper than its resources gets cost = fuelCost (ShipConstruction.cs:1292-1301)
    AvailablePart cheap;
    cheap.name = "fuelTank";
    cheap.cost = 1.0f;
    ShipConstruction::SanitizePartCosts(cheap, node);
    // dryCost = 1 + 10.5 - (440 ox + 360 lf units) < 0 for the stock unit costs
    CHECK(cheap.cost == fc);
    CHECK(quiet.Has(log::Level::Warning, "is less than the cost of its resources"));
    AvailablePart rich;
    rich.name = "fuelTank";
    rich.cost = 100000.0f;
    ShipConstruction::SanitizePartCosts(rich, node);
    CHECK(rich.cost == 100000.0f);
}

TEST_CASE("WP-18 GetPartCosts(ProtoPartSnapshot): double arithmetic on the snapshot, dry / fuel split, includeModuleCosts") {
    if (!GameAvailable()) return;
    Game15 g;
    LogCapture quiet;
    GameParameters::GetDefaultParameters(Game_Modes::CAREER, GameParameters::Preset::Normal);
    ConfigNode::NodePtr root = ConfigNode::Load(std::string(KSPCORE_TESTDATA_DIR) + "/saves/physprofile/persistent.sfs");
    REQUIRE(root);
    Game game(root.get());
    REQUIRE(game.flightState);
    size_t parts = 0, withResources = 0;
    PartResourceLibrary* lib = PartResourceLibrary::Instance();
    for (auto& pv : game.flightState->protoVessels) {
        for (auto& pps : pv->protoPartSnapshots) {
            const AvailablePart* ap = pps->partInfo;
            if (ap == nullptr) continue;
            parts++;
            float dry = -1, fuel = -1;
            const float total = ShipConstruction::GetPartCosts(*pps, ap, dry, fuel);
            // the C# in double: dryCost = float(cost + moduleCosts) (a float store), then num -= (double)unitCost * maxAmount...
            const float start = f32store(fx(ap->cost) + fx(pps->moduleCosts));
            double num = start, num2 = 0.0;
            for (auto& r : pps->resources) {
                const PartResourceDefinition* d = lib->GetDefinition(*r->resourceName);
                REQUIRE(d != nullptr);
                num -= static_cast<double>(d->unitCost()) * r->maxAmount;
                num2 += static_cast<double>(d->unitCost()) * r->amount;
            }
            CHECK(dry == static_cast<float>(num));
            CHECK(fuel == static_cast<float>(num2));
            CHECK(total == static_cast<float>(num + num2));
            // without module costs the dry cost starts at the part's cost
            float dry2, fuel2;
            const float total2 = ShipConstruction::GetPartCosts(*pps, false, ap, dry2, fuel2);
            double n2 = ap->cost;
            for (auto& r : pps->resources) n2 -= static_cast<double>(lib->GetDefinition(*r->resourceName)->unitCost()) * r->maxAmount;
            CHECK(dry2 == static_cast<float>(n2));
            CHECK(fuel2 == fuel);
            CHECK(total2 == static_cast<float>(n2 + num2));
            if (!pps->resources.empty()) withResources++;
        }
    }
    CHECK(parts == 458);   // the save's 458 parts (WP-13 / WP-14)
    CHECK(withResources > 100);
    // a null AvailablePart: aP.cost is read before the null test (NullReferenceException)
    auto& first = *game.flightState->protoVessels[1]->protoPartSnapshots[0];
    float d, f;
    CHECK_THROWS_AS(ShipConstruction::GetPartCosts(first, nullptr, d, f), NullReferenceException);
    // an unknown resource: logged and ignored (the totals are the ones without it)
    float d0, f0;
    const float t0 = ShipConstruction::GetPartCosts(first, first.partInfo, d0, f0);
    ConfigNode rn("RESOURCE");
    rn.AddValue("name", "Unobtainium");
    rn.AddValue("amount", "1");
    rn.AddValue("maxAmount", "2");
    first.resources.push_back(std::make_unique<ProtoPartResourceSnapshot>(rn));
    float d1, f1;
    CHECK(ShipConstruction::GetPartCosts(first, first.partInfo, d1, f1) == t0);
    CHECK(d1 == d0);
    CHECK(f1 == f0);
    CHECK(quiet.Has(log::Level::Error, "No Resource definition found for Unobtainium"));
}

// ====================================================================================================================
// Paths and names
// ====================================================================================================================

TEST_CASE("WP-18 craft file names: KSPUtil.SanitizeString / SanitizeFilename rules as ShipConstruction applies them") {
    // Expected values are derived from KSPUtil.SanitizeString (KSPUtil.cs:1505): the 12 characters \ / . : | * ? { } < > " become the
    // replacement character, then Trim(), then "Unnamed" for an empty string when replaceEmpty, else a DOS device name gets the
    // replacement character appended (con prn aux nul clock$ lst lpt lpt0-4 com0-4, case-insensitive)
    struct Case {
        const char* in;
        const char* out;
    };
    const Case cases[] = {
        {"Kerbal X", "Kerbal X"},
        {"Kerbal 1-5", "Kerbal 1-5"},
        {"a/b\\c.d:e|f*g?h{i}j<k>l\"m", "a_b_c_d_e_f_g_h_i_j_k_l_m"},
        {"  padded name  ", "padded name"},
        {"  /x/  ", "_x_"},                  // the replacement happens before the trim: '/' is not whitespace
        {"", "Unnamed"},
        {"   ", "Unnamed"},                  // trims to empty
        {"con", "con_"},
        {"CON", "CON_"},
        {"Clock$", "Clock$_"},
        {"lpt1", "lpt1_"},
        {"lpt5", "lpt5"},                    // not in the list
        {"com4", "com4_"},
        {"com5", "com5"},
        {"Mun.Lander v2", "Mun_Lander v2"},
        {"MUN LANDER", "MUN LANDER"},
        {"x.craft", "x_craft"},
        {".", "_"},
        {"\t tab \t", "tab"},                // Trim() removes all white space
        {"ünïcode ship", "ünïcode ship"},
    };
    for (const Case& c : cases) CHECK_MESSAGE(KSPUtil::SanitizeString(c.in, '_', true) == c.out, c.in);
    // replaceEmpty == false keeps an empty name empty
    CHECK(KSPUtil::SanitizeString("", '_', false) == "");
    CHECK(KSPUtil::SanitizeString("  ", '_', false) == "");
    CHECK(KSPUtil::SanitizeString("con", '_', false) == "con_");
    // another replacement character is used for the DOS names too
    CHECK(KSPUtil::SanitizeString("nul", '-', true) == "nul-");
    CHECK(KSPUtil::SanitizeString("a/b", '-', true) == "a-b");
}

TEST_CASE("WP-18 ShipConstruction paths: GetSavePath, GetShipsPathFor, GetShipsSubfolderFor, GetRootCraftSavePath") {
    ShipConstructionScope scope;
    KSPUtil::ApplicationDataPath = "/game/KSP_x64_Data";
    ShipConstruction::env.SaveFolder = "career1";
    ShipConstruction::env.DefaultCraftSavePath = "/game/saves/career1/Ships/VAB";
    ShipConstruction::env.editorFacility = EditorFacility::VAB;
    CHECK(ShipConstruction::GetRootCraftSavePath() == "/game/KSP_x64_Data/../saves");   // ApplicationRootPath = dataPath + "/../"
    CHECK(ShipConstruction::GetShipsPathFor("career1") == "/game/KSP_x64_Data/../saves/career1/");
    CHECK(ShipConstruction::GetShipsPathFor("career1", EditorFacility::SPH) == "/game/KSP_x64_Data/../saves/career1/Ships/SPH/");
    CHECK(ShipConstruction::GetShipsPathFor("career1", EditorFacility::None) == "/game/KSP_x64_Data/../saves/career1/Ships//");
    CHECK(ShipConstruction::GetCurrentGameShipsPathFor(EditorFacility::VAB) == "/game/KSP_x64_Data/../saves/career1/Ships/VAB/");
    CHECK(ShipConstruction::GetShipsSubfolderFor(EditorFacility::VAB) == "VAB");
    CHECK(ShipConstruction::GetShipsSubfolderFor(EditorFacility::SPH) == "SPH");
    CHECK(ShipConstruction::GetShipsSubfolderFor(EditorFacility::None) == "");
    // GetSavePath: empty -> "", sanitised name, DefaultCraftSavePath outside the mission modes
    CHECK(ShipConstruction::GetSavePath("") == "");
    CHECK(ShipConstruction::GetSavePath("Kerbal X") == "/game/saves/career1/Ships/VAB/Kerbal X.craft");
    CHECK(ShipConstruction::GetSavePath("a/b.c") == "/game/saves/career1/Ships/VAB/a_b_c.craft");
    CHECK(ShipConstruction::GetSavePath("   ") == "/game/saves/career1/Ships/VAB/Unnamed.craft");   // not empty: Trim happens in SanitizeString
    CHECK(ShipConstruction::GetSavePath("con") == "/game/saves/career1/Ships/VAB/con_.craft");
    // mission modes need Making History installed AND a mission
    Game game;
    game.Mode = Game_Modes::MISSION;
    ShipConstruction::env.CurrentGame = &game;
    ShipConstruction::env.mission = ShipConstructionEnvironment::MissionInfo{"MyMission", "/game/missions/MyMission/"};
    SaveEnvironment::Current().SetStockExpansionInstalled("MakingHistory", false);
    CHECK(ShipConstruction::GetSavePath("m") == "/game/saves/career1/Ships/VAB/m.craft");   // MH not installed: the default path
    SaveEnvironment::Current().SetStockExpansionInstalled("MakingHistory", true);
    ShipConstruction::env.editorFacility = EditorFacility::SPH;
    CHECK(ShipConstruction::GetSavePath("m") == "/game/KSP_x64_Data/../saves/career1/Ships/SPH/m.craft");   // MISSION: ApplicationRootPath + saves/<folder>/Ships/<fac>
    game.Mode = Game_Modes::MISSION_BUILDER;
    CHECK(ShipConstruction::GetSavePath("m") == "/game/missions/MyMission/Ships/SPH/m.craft");
    ShipConstruction::env.mission.reset();   // MissionSystem.missions.Count == 0
    CHECK(ShipConstruction::GetSavePath("m") == "/game/saves/career1/Ships/VAB/m.craft");
    game.Mode = Game_Modes::CAREER;
    CHECK(ShipConstruction::GetSavePath("m") == "/game/saves/career1/Ships/VAB/m.craft");
    SaveEnvironment::Current().SetStockExpansionInstalled("MakingHistory", false);
}

TEST_CASE("WP-18 GetPlayerCraftThumbnailName: thumbnail file names from the game / sub-directory / ship name") {
    if (!GameAvailable()) return;
    Game15 g;
    ShipConstructionScope scope;
    // the 3-argument form is pure string work (ShipConstruction.cs:208): SanitizeFilename(game) + SanitizeFilename(sub) + "_" + SanitizeString(ship)
    CHECK(ShipConstruction::GetPlayerCraftThumbnailName("career1", "/VAB", "Kerbal X") == "career1_VAB_Kerbal X");
    CHECK(ShipConstruction::GetPlayerCraftThumbnailName("career1", "/VAB/sub dir", "x") == "career1_VAB_sub dir_x");
    // a literal "\n" or a real newline and tabs inside the directory become "/n" / "/t" before sanitising
    CHECK(ShipConstruction::GetPlayerCraftThumbnailName("g", "/VAB/a\nb", "x") == "g_VAB_a_nb_x");
    CHECK(ShipConstruction::GetPlayerCraftThumbnailName("g", "/VAB/a\\nb", "x") == "g_VAB_a_nb_x");
    CHECK(ShipConstruction::GetPlayerCraftThumbnailName("g", "/VAB/a\tb", "x") == "g_VAB_a_tb_x");
    CHECK(ShipConstruction::GetPlayerCraftThumbnailName("g", "/VAB/a\\tb", "x") == "g_VAB_a_tb_x");
    CHECK(ShipConstruction::GetPlayerCraftThumbnailName("my game", "", "con") == "my gameUnnamed_con_");   // SanitizeFilename("") = "Unnamed"
    CHECK(ShipConstruction::GetPlayerCraftThumbnailName("g.h", "/SPH", "a:b") == "g_h_SPH_a_b");
    // the 2-argument form works on the file system: <root>/saves/<game>/Ships/<VAB|SPH>/<sub>/<ship>.craft (the facility is read from the file)
    TempDir tmp;
    KSPUtil::ApplicationDataPath = tmp.str() + "/KSP_x64_Data";
    fs::create_directories(tmp.path / "KSP_x64_Data");
    Game game;
    game.Mode = Game_Modes::SANDBOX;
    ShipConstruction::env.CurrentGame = &game;
    const fs::path vab = tmp.path / "saves" / "career1" / "Ships" / "VAB";
    fs::create_directories(vab / "sub");
    fs::copy_file(fs::path(KSPCORE_HARNESS_DIR) / "inputs" / "Ships" / "VAB" / "Kerbal X.craft", vab / "sub" / "Kerbal X.craft");
    fs::copy_file(fs::path(KSPCORE_HARNESS_DIR) / "inputs" / "Ships" / "SPH" / "Gull.craft", vab / "Gull.craft");   // an SPH craft in the VAB folder
    CHECK(ShipConstruction::GetPlayerCraftThumbnailName((vab / "sub").string(), "Kerbal X") == "career1_VAB_sub_Kerbal X");
    // outside the mission modes the facility only decides which Ships/<facility> folder the root is measured from, so an SPH craft in
    // the VAB folder is named after the folder it is in
    CHECK(ShipConstruction::GetPlayerCraftThumbnailName(vab.string(), "Gull") == "career1_VAB_Gull");
    // a path outside <root>/saves: the walk up to the root reaches the file-system root (DirectoryInfo.Parent == null) -> "" (caught)
    CHECK(ShipConstruction::GetPlayerCraftThumbnailName(tmp.str(), "Kerbal X") == "");
    // mission mode with no missions -> ""
    game.Mode = Game_Modes::MISSION;
    CHECK(ShipConstruction::GetPlayerCraftThumbnailName((vab / "sub").string(), "Kerbal X") == "");
    ShipConstruction::env.mission = ShipConstructionEnvironment::MissionInfo{"MyMission", ""};
    CHECK(ShipConstruction::GetPlayerCraftThumbnailName((vab / "sub").string(), "Kerbal X") == "MyMission_VAB_Kerbal X");
    CHECK(ShipConstruction::GetPlayerCraftThumbnailName(vab.string(), "Gull") == "MyMission_SPH_Gull");   // mission mode: "/" + the file's facility
    // no HighLogic.CurrentGame: NullReferenceException inside the try -> ""
    ShipConstruction::env.CurrentGame = nullptr;
    CHECK(ShipConstruction::GetPlayerCraftThumbnailName((vab / "sub").string(), "Kerbal X") == "");
    // CheckCraftFileType reads "type"
    CHECK(ShipConstruction::CheckCraftFileType((vab / "Gull.craft").string()) == EditorFacility::SPH);
    CHECK(ShipConstruction::CheckCraftFileType((vab / "sub" / "Kerbal X.craft").string()) == EditorFacility::VAB);
    CHECK(ShipConstruction::CheckCraftFileType((vab / "missing.craft").string()) == EditorFacility::None);
    {   // a craft with no "type" value -> None; any other type value -> VAB
        ConfigNode::NodePtr c = ConfigNode::Load((vab / "Gull.craft").string());
        c->RemoveValues("type");
        c->Save((vab / "notype.craft").string());
        CHECK(ShipConstruction::CheckCraftFileType((vab / "notype.craft").string()) == EditorFacility::None);
        c->AddValue("type", "Whatever");
        c->Save((vab / "other.craft").string());
        CHECK(ShipConstruction::CheckCraftFileType((vab / "other.craft").string()) == EditorFacility::VAB);
    }
}

// ====================================================================================================================
// Loading, saving, backups, templates, subassemblies on files
// ====================================================================================================================

TEST_CASE("WP-18 ShipConstruction.LoadShip / SaveShip / backups / SaveShipToPath / subassemblies on the file system") {
    if (!GameAvailable()) return;
    Game15 g;
    LogCapture quiet;
    ShipConstructionScope scope;
    TempDir tmp;
    KSPUtil::ApplicationDataPath = tmp.str() + "/KSP_x64_Data";
    fs::create_directories(tmp.path / "KSP_x64_Data");   // "<data>/../saves" resolves through it
    ShipConstruction::env.SaveFolder = "career1";
    ShipConstruction::env.editorFacility = EditorFacility::VAB;
    const fs::path vabDir = tmp.path / "saves" / "career1" / "Ships" / "VAB";
    fs::create_directories(vabDir);
    ShipConstruction::env.DefaultCraftSavePath = vabDir.string();
    Game game;
    ShipConstruction::env.CurrentGame = &game;
    std::vector<std::string> thumbs;
    ShipConstruction::env.captureThumbnail = [&](ShipConstruct*, const std::string& baseFolder, const std::string& url) { thumbs.push_back(baseFolder + "|" + url); };
    ShipConstruction::env.steamIdFromSteamFolder = [](const std::string&) -> uint64_t { return 123456789012ull; };

    // a craft already in the current layout (the PhysProf crafts are 1.12.5): ShipConstruction.LoadShip(path) reads it without the pipeline
    const fs::path src = fs::path(KSPCORE_PROJECT_ROOT) / "analysis" / "physics" / "craft" / "PhysProf-S.craft";
    fs::copy_file(src, vabDir / "PhysProf-S.craft");
    ClearIds();
    std::unique_ptr<ShipConstruct> ship = ShipConstruction::LoadShip((vabDir / "PhysProf-S.craft").string());
    REQUIRE(ship != nullptr);
    CHECK(ship->LiveCount() == 21);
    CHECK(ship->shipName == "PhysProf-S");
    CHECK(ship->steamPublishedFileId == 123456789012ull);       // steamPublishedFileId == 0 -> KSPSteamUtils.GetSteamIDFromSteamFolder
    REQUIRE(ShipConstruction::ShipConfig != nullptr);           // the loaded ConfigNode stays as ShipConfig
    CHECK(ShipConstruction::ShipConfig->CountNodes() == 21);
    // missing file / unreadable ship
    CHECK(ShipConstruction::LoadShip((vabDir / "nothere.craft").string()) == nullptr);
    CHECK(quiet.Has(log::Level::Error, "nothere.craft' not found!"));
    {
        ConfigNode::NodePtr bad = ConfigNode::Load((vabDir / "PhysProf-S.craft").string());
        bad->SetValue("version", "0.1.0");
        bad->Save((vabDir / "tooold.craft").string());
        ClearIds();
        CHECK(ShipConstruction::LoadShip((vabDir / "tooold.craft").string()) == nullptr);
        CHECK(quiet.Has(log::Level::Error, "Ship file error!"));
    }

    // SaveShip(ship, filename): <root>/saves/<game>/Ships/<VAB>/<filename>.craft, thumbnail "<game>_<VAB>_<filename>"
    const std::string savedPath = ShipConstruction::SaveShip(*ship, "Round Trip");
    CHECK(fs::exists(dotnet::PathFromUtf8(savedPath)));
    CHECK(savedPath.size() > 6);
    CHECK(savedPath.substr(savedPath.size() - 17) == "/Round Trip.craft");
    REQUIRE(thumbs.size() == 1);
    CHECK(thumbs[0] == "thumbs|career1_VAB_Round Trip");
    // the file is exactly the node SaveShip builds, saved with CRLF
    {
        std::ifstream f(dotnet::PathFromUtf8(savedPath), std::ios::binary);
        std::stringstream ss;
        ss << f.rdbuf();
        ConfigNode::NodePtr again = ship->SaveShip();
        CHECK(ss.str() == again->SaveToString());
    }

    // backups: CreateBackup stores SaveShip()'s node and sets ShipConfig / ShipType
    ShipConstruction::ClearBackups();
    CHECK(ShipConstruction::backups.empty());
    ShipConstruction::CreateBackup(*ship);
    ShipConstruction::CreateBackup(*ship);
    CHECK(ShipConstruction::backups.size() == 2);
    CHECK(ShipConstruction::ShipConfig == ShipConstruction::backups[1]);
    CHECK(ShipConstruction::ShipType == ship->shipFacility);
    // ShiftAndCreateBackup shifts the list down by one (the first entry is dropped) and replaces the last
    ConfigNode::NodePtr second = ShipConstruction::backups[1];
    ShipConstruction::ShiftAndCreateBackup(*ship);
    CHECK(ShipConstruction::backups.size() == 2);
    CHECK(ShipConstruction::backups[0] == second);
    CHECK(ShipConstruction::backups[1] == ShipConstruction::ShipConfig);
    CHECK(ShipConstruction::backups[1] != second);
    // RestoreBackup reloads the backup into a new ship; LoadShip() reloads ShipConfig
    ClearIds();
    std::unique_ptr<ShipConstruct> restored = ShipConstruction::RestoreBackup(1);
    REQUIRE(restored != nullptr);
    CHECK(restored->LiveCount() == 21);
    CHECK(ShipConstruction::ShipConfig == ShipConstruction::backups[1]);
    ClearIds();
    std::unique_ptr<ShipConstruct> reloaded = ShipConstruction::LoadShip();
    REQUIRE(reloaded != nullptr);
    CHECK(reloaded->LiveCount() == 21);
    CHECK_THROWS(ShipConstruction::RestoreBackup(5));
    ShipConstruction::DebugBackup();
    ShipConstruction::ClearBackups();
    CHECK_THROWS(ShipConstruction::ShiftAndCreateBackup(*ship) /* backups[Count - 1] with Count == 0 */);
    ShipConstruction::ShipConfig = nullptr;
    CHECK(ShipConstruction::LoadShip() == nullptr);
    CHECK(quiet.Has(log::Level::Error, "Backup ship config not found!"));
    CHECK(ShipConstruction::SaveShip("x") == "");          // ShipConfig == null: nothing saved
    CHECK(ShipConstruction::SaveShipToPath("x", tmp.str()) == "");

    // SaveShip(filename) writes ShipConfig to GetSavePath(filename); SaveShipToPath sanitises the name
    thumbs.clear();
    ShipConstruction::ShipConfig = ship->SaveShip();
    const std::string p1 = ShipConstruction::SaveShip("A/B.C");
    CHECK(p1 == (vabDir.string() + "/A_B_C.craft"));
    CHECK(fs::exists(vabDir / "A_B_C.craft"));
    REQUIRE(thumbs.size() == 1);
    CHECK(thumbs[0] == "thumbs|career1_VAB_A/B.C");   // the thumbnail name uses the raw file name argument (ShipConstruction.cs:126)
    thumbs.clear();
    fs::create_directories(vabDir / "folder");
    game.Mode = Game_Modes::SANDBOX;
    const std::string p2 = ShipConstruction::SaveShipToPath("con", (vabDir / "folder").string());
    CHECK(p2 == (vabDir / "folder").string());
    CHECK(fs::exists(vabDir / "folder" / "con_.craft"));
    REQUIRE(thumbs.size() == 1);
    CHECK(thumbs[0] == "thumbs|career1_VAB_folder_con_");
    thumbs.clear();
    game.Mode = Game_Modes::MISSION_BUILDER;
    ShipConstruction::SaveShipToPath("m", (vabDir / "folder").string());
    REQUIRE(thumbs.size() == 1);
    CHECK(thumbs[0].substr(0, 20) == "thumbs/test_missions");
    game.Mode = Game_Modes::SANDBOX;
    thumbs.clear();
    // the 4-argument form: <root>/saves/<game>/Ships/<facility>/<local path>
    const std::string p3 = ShipConstruction::SaveShipToPath("career1", EditorFacility::VAB, "folder", "direct");
    CHECK(p3.find("Ships/VAB//folder") != std::string::npos);   // GetShipsPathFor ends with "/", then "/" + localPath
    CHECK(fs::exists(vabDir / "folder" / "direct.craft"));

    // subassemblies: <root>/saves/<game>/Subassemblies/<name>.craft
    CHECK_FALSE(ShipConstruction::SubassemblyExists("sa1"));
    ClearIds();
    std::unique_ptr<ShipConstruct> sa = ShipConstruction::LoadShip((vabDir / "PhysProf-S.craft").string());
    REQUIRE(sa != nullptr);
    const std::string saPath = ShipConstruction::SaveSubassembly(*sa, "sa1");
    CHECK(fs::exists(dotnet::PathFromUtf8(saPath)));
    CHECK(ShipConstruction::SubassemblyExists("sa1"));
    ClearIds();
    std::unique_ptr<ShipConstruct> sa2 = ShipConstruction::LoadSubassembly(saPath);
    REQUIRE(sa2 != nullptr);
    CHECK(sa2->LiveCount() == 21);
    CHECK(ShipConstruction::LoadSubassembly((vabDir / "nope.craft").string()) == nullptr);

    // the thumbnail hooks (CraftThumbnail is UI): CaptureStockThumbnail / CaptureThumbnail only reach the environment's callbacks
    {
        std::vector<std::string> stock;
        ShipConstruction::env.captureStockThumbnail = [&](ShipConstruct*, const std::string& path, bool expansion) { stock.push_back(path + (expansion ? "|exp" : "|stock")); };
        ShipConstruction::CaptureStockThumbnail(ship.get(), "a.png", true);
        ShipConstruction::CaptureStockThumbnail(ship.get(), "b.png", false);
        CHECK(stock == std::vector<std::string>{"a.png|exp", "b.png|stock"});
        thumbs.clear();
        ShipConstruction::CaptureThumbnail(ship.get(), "thumbs", "x");
        CHECK(thumbs == std::vector<std::string>{"thumbs|x"});
    }
    // IEnumerable<Part>: range-for over the live parts
    {
        size_t n = 0;
        for (Part* p : *ship) {
            CHECK(p == ship->liveParts[n]);
            n++;
        }
        CHECK(n == 21);
    }

    // LoadTemplate: the template of a file (no parts instantiated)
    std::unique_ptr<ShipTemplate> t = ShipConstruction::LoadTemplate((vabDir / "PhysProf-S.craft").string());
    REQUIRE(t != nullptr);
    CHECK(t->partCount == 21);
    CHECK(t->filename == (vabDir / "PhysProf-S.craft").string());
    CHECK(ShipConstruction::LoadTemplate((vabDir / "nope.craft").string()) == nullptr);
}

TEST_CASE("WP-18 CreateConstructFromTemplate: the template config goes through the upgrade pipeline; the file is rewritten and the original kept") {
    if (!GameAvailable()) return;
    Game15 g;
    LogCapture quiet;
    ShipConstructionScope scope;
    TempDir tmp;
    // a 1.6.0 craft with parts the pipeline renames (Kerbal X: liquidEngine1-2 -> liquidEngineMainsail.v2, v180 control surfaces...)
    const fs::path file = tmp.path / "Kerbal X.craft";
    fs::copy_file(fs::path(KSPCORE_HARNESS_DIR) / "inputs" / "Ships" / "VAB" / "Kerbal X.craft", file);
    const std::string originalText = [&] {
        std::ifstream f(file, std::ios::binary);
        std::stringstream ss;
        ss << f.rdbuf();
        return ss.str();
    }();
    std::unique_ptr<ShipTemplate> t = ShipConstruction::LoadTemplate(file.string());
    REQUIRE(t != nullptr);
    ConfigNode::NodePtr rawConfig = t->config;
    std::unique_ptr<ShipConstruct> built;
    int calls = 0;
    ClearIds();
    ShipConstruction::CreateConstructFromTemplate(*t, [&](std::unique_ptr<ShipConstruct> s) {
        calls++;
        built = std::move(s);
    });
    CHECK(calls == 1);
    REQUIRE(built != nullptr);
    CHECK(built->LiveCount() == 73);
    CHECK(t->config != rawConfig);   // the pipeline produced a new node and the template now holds it
    // the upgraded file replaced the original and the original was kept as <file>.original
    CHECK(fs::exists(tmp.path / "Kerbal X.craft.original"));
    {
        std::ifstream f(tmp.path / "Kerbal X.craft.original", std::ios::binary);
        std::stringstream ss;
        ss << f.rdbuf();
        CHECK(ss.str() == rawConfig->SaveToString());
    }
    {
        std::ifstream f(file, std::ios::binary);
        std::stringstream ss;
        ss << f.rdbuf();
        CHECK(ss.str() == t->config->SaveToString());
        CHECK(ss.str() != originalText);
    }
    // every part's craftID is its instance id ((uint)part.GetInstanceID()), as OnPipelineFinished sets it
    for (Part* p : built->liveParts) CHECK(p->fields.craftID == p->instanceID);
    // a second pass over the rewritten file: the pipeline has nothing to do, no new .original, the node is unchanged
    std::unique_ptr<ShipTemplate> t2 = ShipConstruction::LoadTemplate(file.string());
    ClearIds();
    std::unique_ptr<ShipConstruct> built2;
    ShipConstruction::CreateConstructFromTemplate(*t2, [&](std::unique_ptr<ShipConstruct> s) { built2 = std::move(s); });
    REQUIRE(built2 != nullptr);
    CHECK(built2->LiveCount() == 73);
    // a template whose parts are unknown: onComplete(null)
    ConfigNode::NodePtr unknown = t2->config->CreateCopy();
    unknown->nodes()[0]->SetValue("part", "madeUpPart_1");
    ShipTemplate t3;
    t3.shipName = "x";
    t3.filename = (tmp.path / "x.craft").string();
    t3.config = unknown;
    int calls3 = 0;
    ShipConstruction::CreateConstructFromTemplate(t3, [&](std::unique_ptr<ShipConstruct> s) {
        calls3++;
        CHECK(s == nullptr);
    });
    CHECK(calls3 == 1);
}

TEST_CASE("WP-18 the C5 capture's total_mass is the no-crew total plus an integral number of KerbalCrewMass; part masses and dry / fuel masses are gated by capture.ship_construct") {
    if (!GameAvailable()) return;
    World18 w;
    LogCapture quiet;
    ShipConstructionScope scope;
    const nlohmann::json cap = LoadJson(kHarness18 + "/captures/out2/deltav_readout.json");
    const nlohmann::json& crafts = TableOf(cap, "craft");
    const size_t cId = ColOf(crafts, "craft_id"), cPath = ColOf(crafts, "path"), cTotal = ColOf(crafts, "total_mass"), cParts = ColOf(crafts, "parts");
    size_t rows = 0, crewed = 0, uncrewed = 0;
    for (const auto& r : crafts.at("rows")) {
        // craft_ComSat_Lx has a fairing whose mass (ModuleProceduralFairing.panelMass, WP-26) is in the capture's totals only
        if (r[cId].get<std::string>() == "craft_ComSat_Lx") continue;
        const std::string path = r[cPath].get<std::string>();   // "Ships/VAB/PhysProf-S.craft" (the game's Ships folder)
        const std::string name = path.substr(path.rfind('/') + 1);
        fs::path file = fs::path(KSPCORE_HARNESS_DIR) / "inputs" / "Ships" / (path.find("/SPH/") != std::string::npos ? "SPH" : "VAB") / name;
        if (!fs::exists(file)) file = fs::path(KSPCORE_PROJECT_ROOT) / "analysis" / "physics" / "craft" / name;
        REQUIRE_MESSAGE(fs::exists(file), path);
        ConfigNode::NodePtr up = Upgraded(file);
        REQUIRE(up);
        ClearIds();
        ShipConstruct ship;
        std::string err;
        REQUIRE_MESSAGE(ship.LoadShipWithParts(*up, 0, true, err), err);
        CHECK(ship.LiveCount() == r[cParts].get<int>());
        float dry, fuel;
        const float noCrew = ship.GetShipMass(dry, fuel);
        const float capture = r[cTotal].get<float>();
        const float perKerbal = KerbalInventoryMass(w.g);   // evaChute 0.004 + evaJetpack (0.02 + 5 EVA Propellant * 0.005)
        CHECK(perKerbal > 0.0f);
        // the editor crew adds (float)CrewCount * KerbalCrewMass (0.045), then the crew's inventory mass, then its resource mass to the no-crew
        // total (ShipConstruct.GetShipMass); the per-kerbal inventory mass is computed from the part database (KerbalInventoryMass: 0.049 t),
        // and every craft's total_mass is reproduced bit for bit by one crew count
        int n = -1;
        int capacity = 0;
        for (Part* p : ship.liveParts) capacity += p->fields.CrewCapacity;
        for (int k = 0; k <= capacity; k++) {
            StubManifest m;
            m.crew = k;
            m.inventoryPerKerbal = perKerbal;
            float d2, f2;
            if (ship.GetShipMass(d2, f2, &m) == capture) {
                n = k;
                break;
            }
        }
        CHECK_MESSAGE(n >= 0, r[cId].get<std::string>() << " total_mass " << capture << " no crew " << noCrew << " crewMass " << PhysicsGlobals::KerbalCrewMass() << " capacity " << capacity);
        if (n > 0) crewed++;
        else uncrewed++;
        rows++;
    }
    CHECK(rows == 16);       // the 17 crafts of the capture minus ComSat Lx
    CHECK(crewed == 11);     // measured: crafts with a crew in the capture's editor session
    CHECK(uncrewed == 5);
}

// ====================================================================================================================
// Ship list operations, naming, resource sets, craft size
// ====================================================================================================================

TEST_CASE("WP-18 ShipConstruct list operations and the part constructors") {
    if (!GameAvailable()) return;
    Game15 g;
    LogCapture quiet;
    // a three-part stack built from stock prefabs: pod <- tank <- engine
    auto a = g.New("mk1pod.v2");
    auto b = g.New("fuelTank");
    auto c = g.New("liquidEngine");
    Attach(*a, "bottom", *b, "top");
    Attach(*b, "bottom", *c, "top");
    {
        ShipConstruct ship("Stack", "three parts", a.get());   // the (name, description, Part root) constructor adds the tree recursively
        CHECK(ship.shipName == "Stack");
        CHECK(ship.shipDescription == "three parts");
        REQUIRE(ship.LiveCount() == 3);
        CHECK(ship.liveParts[0] == a.get());   // depth first: root, then its children
        CHECK(ship.liveParts[1] == b.get());
        CHECK(ship.liveParts[2] == c.get());
        CHECK(a->ship == &ship);
        CHECK(c->ship == &ship);
        CHECK(ship.persistentId != 0);
        CHECK(ship.OverrideDefault.size() == 4);
        CHECK(ship.OverrideActionControl.size() == 4);
        CHECK(ship.OverrideAxisControl.size() == 4);
        CHECK(ship.OverrideGroupNames.size() == 4);
        CHECK(ship.shipSize.Equals(Vector3f::zero()));   // no renderer bounds without a model reader
        CHECK(ship.Contains(b.get()));
        ship.Remove(b.get());
        CHECK_FALSE(ship.Contains(b.get()));
        CHECK(ship.LiveCount() == 2);
        ship.Add(b.get());
        CHECK(ship.liveParts.back() == b.get());   // Add appends
        ship.Remove(b.get());
        ship.Remove(b.get());                      // List.Remove of an absent item is a no-op
        CHECK(ship.LiveCount() == 2);
        ship.AddToConstruct(a.get());              // a is already in: only the children are visited; b is added, c is already in
        CHECK(ship.LiveCount() == 3);
        CHECK(ship.liveParts[2] == b.get());
        CHECK(ship.AreAllPartsConnected());
        ship.Clear();
        CHECK(ship.LiveCount() == 0);
        CHECK(ship.AreAllPartsConnected());        // the loop never runs
        for (Part* p : {a.get(), b.get(), c.get()}) p->ship = nullptr;
    }
    {   // a lone part with neither parent nor children breaks "all parts connected"
        auto lone = g.New("fuelTank");
        ShipConstruct ship("x", "y", std::vector<Part*>{a.get(), lone.get()});
        CHECK_FALSE(ship.AreAllPartsConnected());
        CHECK(a->ship == &ship);
        a->ship = nullptr;
        lone->ship = nullptr;
    }
    {   // (name, facility, parts) and (facility)
        ShipConstruct ship("Plane", EditorFacility::SPH, std::vector<Part*>{a.get(), b.get(), c.get()});
        CHECK(ship.shipFacility == EditorFacility::SPH);
        CHECK(ship.LiveCount() == 3);
        CHECK(ship[1] == b.get());
        for (Part* p : {a.get(), b.get(), c.get()}) p->ship = nullptr;
        ShipConstruct s2(EditorFacility::VAB);
        CHECK(s2.shipFacility == EditorFacility::VAB);
        CHECK(s2.LiveCount() == 0);
        CHECK(s2.persistentId != 0);
        // the constructor with a facility does not set the rotation to identity (Quaternion default)
        CHECK(s2.rotation.Equals(Quaternionf{0.0f, 0.0f, 0.0f, 0.0f}));
        ShipConstruct s3;
        CHECK(s3.rotation.Equals(Quaternionf::identity()));   // the default constructor does
    }
}

TEST_CASE("WP-18 ShipConstruct vessel naming: the VESSELNAMING part with the highest priority names the ship") {
    if (!GameAvailable()) return;
    Game15 g;
    LogCapture quiet;
    ShipConstructionScope scope;
    auto a = g.New("mk1pod.v2");
    auto b = g.New("fuelTank");
    Attach(*a, "bottom", *b, "top");
    ShipConstruct ship("Stack", "", a.get());
    struct Listener {
        std::vector<std::string> changes;
        void OnChange(GE::HostedFromToAction<ShipConstruct*, std::string> d) { changes.push_back(d.from + ">" + d.to); }
    } listener;
    std::vector<std::string>& changes = listener.changes;
    GE::onEditorVesselNamingChanged.Add(&listener, &Listener::OnChange);
    std::vector<std::string> messages;
    ShipConstruct::postScreenMessage = [&](const std::string& m) { messages.push_back(m); };
    // no part has a VesselNaming: no name, no event
    CHECK_FALSE(ship.UpdateVesselNaming());
    CHECK(ship.vesselNamedBy == nullptr);
    // part b: priority 5, "Probe One", type Probe
    b->vesselNaming = std::make_shared<VesselNaming>();
    b->vesselNaming->vesselName = "Probe One";
    b->vesselNaming->vesselType = VesselType::Probe;
    b->vesselNaming->namingPriority = 5;
    CHECK(ship.UpdateVesselNaming());
    CHECK(ship.vesselNamedBy == b.get());
    CHECK(ship.highestPriorityName == "Probe One");
    CHECK(ship.vesselType == VesselType::Probe);
    REQUIRE(messages.size() == 1);
    CHECK(messages[0] == "Probe One");
    REQUIRE(changes.size() == 1);
    CHECK(changes[0] == "Stack>Stack");      // the event carries shipName before and after (shipName itself is not renamed here)
    // nothing changed: no event
    CHECK(ship.UpdateVesselNaming());
    CHECK(changes.size() == 1);
    // the same part renamed: the highestPriorityName differs -> update + event
    b->vesselNaming->vesselName = "Probe Two";
    CHECK(ship.UpdateVesselNaming());
    CHECK(ship.highestPriorityName == "Probe Two");
    CHECK(changes.size() == 2);
    // the type changes only
    b->vesselNaming->vesselType = VesselType::Lander;
    CHECK(ship.UpdateVesselNaming());
    CHECK(ship.vesselType == VesselType::Lander);
    CHECK(changes.size() == 3);
    // a higher-priority part takes over; noGameEvent suppresses the event and the message
    a->vesselNaming = std::make_shared<VesselNaming>();
    a->vesselNaming->vesselName = "Pod";
    a->vesselNaming->vesselType = VesselType::Ship;
    a->vesselNaming->namingPriority = 9;
    a->fields.persistentId = b->fields.persistentId + 1;   // different parts have different ids
    CHECK(ship.UpdateVesselNaming(true));
    CHECK(ship.vesselNamedBy == a.get());
    CHECK(ship.highestPriorityName == "Pod");
    CHECK(changes.size() == 3);
    CHECK(messages.size() == 3);
    // in the editor an unset vesselNamedBy starts at EditorLogic.RootPart
    ship.vesselNamedBy = nullptr;
    {
        SceneScope editor(GameScenes::EDITOR);
        ShipConstruct::editorRootPart = [&]() -> Part* { return b.get(); };
        a->vesselNaming->vesselName = "Pod";
        CHECK(ship.UpdateVesselNaming(true));   // root = b, the new namer is a (different persistentId) -> named by a
        CHECK(ship.vesselNamedBy == a.get());
    }
    GE::onEditorVesselNamingChanged.Remove(&listener, &Listener::OnChange);
    ShipConstruct::postScreenMessage = nullptr;
    for (Part* p : {a.get(), b.get()}) p->ship = nullptr;
}

TEST_CASE("WP-18 ShipConstruct resource sets: UpdateResourceSets builds the ship's PartSet; GetConnectedResourceTotals / RequestResource through Part.ship") {
    if (!GameAvailable()) return;
    Game15 g;
    LogCapture quiet;
    SceneScope editor(GameScenes::EDITOR);
    ConfigNode::NodePtr up = Upgraded(fs::path(KSPCORE_HARNESS_DIR) / "inputs" / "Ships" / "VAB" / "Kerbal 2.craft");
    REQUIRE(up);
    ShipConstruct ship;
    std::string err;
    REQUIRE(ship.LoadShipWithParts(*up, 0, true, err));
    CHECK(ship.resourcePartSet() == nullptr);
    double amount = -1, maxAmount = -1;
    const int lfId = PartResourceLibrary::Instance()->GetDefinition("LiquidFuel")->id();
    ship.GetConnectedResourceTotals(lfId, true, amount, maxAmount);   // no PartSet yet: zeros
    CHECK(amount == 0.0);
    CHECK(maxAmount == 0.0);
    CHECK(ship.RequestResource(ship.liveParts[0], lfId, 1.0, true, true) == 0.0);
    ship.UpdateResourceSets();
    REQUIRE(ship.resourcePartSet() != nullptr);
    // the simulation totals of the whole ship (ALL_VESSEL flow: every part with a flowing LiquidFuel tank)
    ship.GetConnectedResourceTotals(lfId, true, amount, maxAmount);
    double sumAmount = 0.0, sumMax = 0.0;
    for (Part* p : ship.liveParts) {
        PartResource* r = p->SimulationResources().Get(lfId);
        if (r != nullptr && r->flowState) {
            sumAmount += r->amount;
            sumMax += r->maxAmount;
        }
    }
    CHECK(maxAmount > 0.0);
    CHECK(amount == sumAmount);
    CHECK(maxAmount == sumMax);
    // Part.GetConnectedResourceTotals reaches the same numbers through Part.ship (editor, simulate)
    double pa = -1, pm = -1;
    ship.liveParts[0]->GetConnectedResourceTotals(lfId, ResourceFlowMode::ALL_VESSEL, true, pa, pm);
    CHECK(pa == amount);
    CHECK(pm == maxAmount);
    // the ShipConstruct is the IResourceVessel the host maps Part.ship to
    CHECK(ResourceFlowEnvironment::Host(&ship) == static_cast<IResourceVessel*>(&ship));
    // asking again with the same parts keeps the PartSet (the cache of ShipConstruct.cs:948 compares the part sets)
    const std::shared_ptr<PartSet> first = ship.resourcePartSet();
    ship.UpdateResourceSets();
    CHECK(ship.resourcePartSet() == first);
    // the threshold variant exists on Vessel only
    double ta = -1, tm = -1;
    ship.GetConnectedResourceTotals(lfId, true, ta, tm, 0.1, true);
    CHECK(ta == 0.0);
    CHECK(tm == 0.0);
    // an empty ship does nothing (inputParts.Count == 0)
    ShipConstruct empty;
    empty.UpdateResourceSets();
    CHECK(empty.resourcePartSet() == nullptr);
}

TEST_CASE("WP-18 CalculateCraftSize / FindCraftCenter / FindCraftMOI over given renderer bounds; PutShipToGround over given collider bounds") {
    if (!GameAvailable()) return;
    Game15 g;
    LogCapture quiet;
    ShipConstructionScope scope;
    // pod <- tank <- engine, 1 m apart on the y axis; the model reader's job (renderer / collider bounds) is replaced by given bounds
    auto a = g.New("mk1pod.v2");
    auto b = g.New("fuelTank");
    auto c = g.New("liquidEngine");
    a->partTransform.position = Vector3f{0.0f, 3.0f, 0.0f};
    b->partTransform.position = Vector3f{0.0f, 2.0f, 0.0f};
    c->partTransform.position = Vector3f{0.0f, 1.0f, 0.0f};
    Attach(*a, "bottom", *b, "top");
    Attach(*b, "bottom", *c, "top");
    PartModelRenderer ra, rb, rc;
    ra.name = rb.name = rc.name = "model";
    ra.bounds = Bounds(Vector3f{0.0f, 3.0f, 0.0f}, Vector3f{1.0f, 1.0f, 1.0f});
    rb.bounds = Bounds(Vector3f{0.0f, 2.0f, 0.0f}, Vector3f{1.5f, 1.0f, 1.5f});
    rc.bounds = Bounds(Vector3f{0.0f, 1.0f, 0.0f}, Vector3f{1.0f, 1.0f, 1.0f});
    a->modelRenderers = {ra};
    b->modelRenderers = {rb};
    c->modelRenderers = {rc};
    ShipConstruct ship("Stack", "", a.get());
    // CalculateCraftSize: each part's bounds * boundsMultiplier + the module size (zero here), merged centred on the root transform:
    // x 1.5 (the tank), y from 0.5 to 3.5 = 3, z 1.5
    const Vector3f size = ShipConstruction::CalculateCraftSize(ship);
    CHECK(size.Equals(Vector3f{1.5f, 3.0f, 1.5f}));
    CHECK(ShipConstruction::CalculateCraftSize(ship.liveParts, ship.liveParts[0]).Equals(size));
    CHECK(ShipConstruction::CalculateCraftSize(std::vector<Part*>{}, a.get()).Equals(Vector3f::zero()));
    CHECK(ShipConstruction::CalculateCraftSize(ship.liveParts, nullptr).Equals(Vector3f::zero()));
    // boundsMultiplier scales the bounds of that part
    b->fields.boundsMultiplier = 2.0f;
    CHECK(ShipConstruction::CalculateCraftSize(ship).Equals(Vector3f{3.0f, 3.0f, 3.0f}));   // the tank becomes 3 x 2 x 3 around y = 2 (y 1 to 3): still inside the y range 0.5 to 3.5
    b->fields.boundsMultiplier = 1.0f;
    // the same through SaveShip: with renderer bounds the size is CalculateCraftSize's, written with G9
    ConfigNode::NodePtr saved = ship.SaveShip();
    CHECK(*saved->GetValuePtr("size") == KSPUtil::WriteVector(size));
    // ShipTemplate.GetShipSize reads the stored size; CalculateCraftSize(ShipTemplate) returns the stored size or zero
    ShipTemplate tmpl;
    tmpl.shipSize = Vector3f{1.0f, 2.0f, 3.0f};
    CHECK(ShipConstruction::CalculateCraftSize(tmpl).Equals(Vector3f{1.0f, 2.0f, 3.0f}));
    // FindCraftCenter: the centroid of all renderer bounds centres (PartGeometryUtil.FindBoundsCentroid)
    const Vector3f centre = ShipConstruction::FindCraftCenter(ship);
    CHECK(centre.x == 0.0f);
    CHECK(centre.y == 2.0f);
    CHECK(centre.z == 0.0f);
    // without renderers at all the first part's (empty) list is used
    {
        PartModelRenderer none;
        (void)none;
        auto lone = g.New("fuelTank");
        ShipConstruct s2("lone", "", lone.get());
        CHECK(ShipConstruction::CalculateCraftSize(s2).Equals(Vector3f::zero()));
        lone->ship = nullptr;
    }
    // FindCraftMOI: sum of m * (y^2 + z^2, x^2 + z^2, x^2 + y^2) about the CoM, children that are attached and not DEAD included
    for (Part* p : ship.liveParts) {
        p->partTransform.rotation = Quaternionf::identity();
        p->fields.CoMOffset = Vector3f::zero();
        p->UpdateMass();
    }
    const Vector3f com{0.0f, 2.0f, 0.0f};
    for (Part* p : ship.liveParts) p->set_isAttached(true);
    const Vector3f moi = ShipConstruction::FindCraftMOI(*a, com);
    double ex = 0, ey = 0, ez = 0;
    for (Part* p : ship.liveParts) {
        const double m = static_cast<double>(p->fields.mass) + p->GetResourceMass();
        const double dy = p->partTransform.position.y - 2.0;
        ex += m * dy * dy;
        ey += 0.0;
        ez += m * dy * dy;
    }
    CHECK(std::fabs(moi.x - ex) <= 1e-4 * std::max(1.0, ex));
    CHECK(moi.y == 0.0f);
    CHECK(std::fabs(moi.z - ez) <= 1e-4 * std::max(1.0, ez));
    (void)ey;
    // a DEAD child is skipped
    c->set_State(PartStates::DEAD);
    const Vector3f moi2 = ShipConstruction::FindCraftMOI(*a, com);
    CHECK(moi2.x < moi.x);
    c->set_State(PartStates::IDLE);

    // PutShipToGround: collider bounds in world space; the whole ship moves down by the lowest collider point, then to the spawn point
    a->colliderBounds = {Bounds(Vector3f{0.0f, 3.0f, 0.0f}, Vector3f{1.0f, 1.0f, 1.0f})};   // min.y 2.5
    b->colliderBounds = {Bounds(Vector3f{0.0f, 2.0f, 0.0f}, Vector3f{1.5f, 1.0f, 1.5f})};   // 1.5
    c->colliderBounds = {Bounds(Vector3f{0.0f, 1.0f, 0.0f}, Vector3f{1.0f, 1.0f, 1.0f})};   // 0.5 (the lowest)
    int sent = 0;
    ShipConstruction::env.onPutToGround = [&](Part&, PartHeightQuery& q) {
        sent++;
        CHECK(q.lowestPoint == 0.5f);
    };
    SpawnPoint spawn;
    spawn.position = Vector3f{10.0f, 70.0f, -5.0f};
    spawn.rotation = Quaternionf::identity();
    ShipConstruction::PutShipToGround(ship, spawn);
    CHECK(sent == 3);
    // translation = (0, -lowest, 0) + spawn.position = (10, 69.5, -5); identity rotation: no rotation about the spawn point
    CHECK(a->partTransform.position.Equals(Vector3f{10.0f, 72.5f, -5.0f}));
    CHECK(b->partTransform.position.Equals(Vector3f{10.0f, 71.5f, -5.0f}));
    CHECK(c->partTransform.position.Equals(Vector3f{10.0f, 70.5f, -5.0f}));
    for (Part* p : ship.liveParts) CHECK(p->partTransform.rotation.Equals(Quaternionf::identity()));
    for (Part* p : {a.get(), b.get(), c.get()}) p->ship = nullptr;
}

// ====================================================================================================================
// AssembleForLaunch (ship -> Vessel), launch-site queries
// ====================================================================================================================

TEST_CASE("WP-18 AssembleForLaunch: the Vessel of a ship equals WP-14's craft-file assembly and the C10 structure table") {
    if (!GameAvailable()) return;
    World18 w;
    LogCapture quiet;
    ShipConstructionScope scope;
    const nlohmann::json cap = LoadJson(kHarness18 + "/captures/out2/aero_forces_vessel.json");
    const nlohmann::json& st = TableOf(cap, "structure");
    const size_t cCraft = ColOf(st, "craft"), cIdx = ColOf(st, "part_index"), cPart = ColOf(st, "part"), cParent = ColOf(st, "parent_index"),
                 cMode = ColOf(st, "attachMode");
    std::map<std::string, std::vector<const nlohmann::json*>> rowsOf;
    for (const auto& r : st.at("rows")) rowsOf[r[cCraft].get<std::string>()].push_back(&r);
    REQUIRE(rowsOf.size() == 3);
    Game game;
    ShipConstruction::env.CurrentGame = &game;
    for (const auto& [name, rows] : rowsOf) {
        CAPTURE(name);
        const fs::path file = fs::path(KSPCORE_PROJECT_ROOT) / "analysis" / "physics" / "craft" / (name + ".craft");
        // WP-14's route: the craft file straight to a Vessel
        ConfigNode::NodePtr root = ConfigNode::Load(file.string());
        REQUIRE(root);
        ShipConstruct dataShip;
        std::string err;
        ClearIds();
        REQUIRE(dataShip.LoadShip(*root, 0, true, err));
        AssembleOptions ao;
        ao.rootPosition = w.PlaceKerbinAtPad();
        std::unique_ptr<Vessel> viaCraft = AssembleVesselFromCraft(dataShip, w.Lookup(), ao);
        REQUIRE(viaCraft != nullptr);
        FlightRegistry::Clear();
        // WP-18's route: ShipConstruction.LoadShip(path) (live parts), AssembleForLaunch
        ClearIds();
        std::unique_ptr<ShipConstruct> ship = ShipConstruction::LoadShip(file.string());
        REQUIRE(ship != nullptr);
        StubManifest manifest;
        const uint32_t launchIdBefore = game.launchID;
        LaunchPlacement lp;
        lp.rootPosition = w.PlaceKerbinAtPad();
        std::unique_ptr<Vessel> v = ShipConstruction::AssembleForLaunch(*ship, "LaunchPad", "Launch Pad", "Squad/Flags/default", game, &manifest, lp);
        REQUIRE(v != nullptr);
        CHECK(manifest.assigned == 1);                          // crewManifest.AssignCrewToVessel(ship)
        CHECK(game.launchID == launchIdBefore + 1);             // launchID = HighLogic.CurrentGame.launchID++
        CHECK(ship->ownedParts.empty());                        // the Vessel owns the parts now
        for (Part* p : ship->liveParts) CHECK(p->ship == nullptr);
        CHECK(v->vesselName == ship->shipName);
        CHECK(v->persistentId == ship->persistentId);
        CHECK(v->parts.size() == rows.size());
        CHECK(v->parts.size() == viaCraft->parts.size());
        CHECK(v->rootPart == v->parts[0]);
        CHECK(v->Landed);
        CHECK(v->landedAt == "LaunchPad");
        CHECK(v->displaylandedAt == "Launch Pad");
        CHECK(v->launchedFrom == "LaunchPad");
        CHECK(v->vesselSpawning);
        CHECK(v->loaded);
        CHECK(v->packed);
        CHECK(v->situation == Vessel::Situations::PRELAUNCH);
        CHECK(v->situation == viaCraft->situation);
        CHECK(v->mainBody() == &w.Kerbin());
        CHECK_FALSE(v->skipGroundPositioning);
        std::set<uint32_t> flightIds;
        for (Part* p : v->parts) {
            CHECK(p->fields.launchID == launchIdBefore);
            CHECK(p->fields.flagURL.value_or("") == "Squad/Flags/default");
            CHECK(p->fields.missionID == v->parts[0]->fields.missionID);
            CHECK(p->fields.flightID != 0);
            flightIds.insert(p->fields.flightID);
            CHECK(p->vessel == v.get());
        }
        CHECK(flightIds.size() == v->parts.size());   // GetUniqueFlightID: unique per part
        CHECK(v->parts[0]->fields.missionID != 0);
        // the C10 structure table (DFS order of findVesselParts, parent, attach mode) and WP-14's assembly
        for (const nlohmann::json* r : rows) {
            const size_t i = (*r)[cIdx].get<size_t>();
            Part* p = v->parts.at(i);
            Part* q = viaCraft->parts.at(i);
            CHECK(p->partInfo->name == (*r)[cPart].get<std::string>());
            CHECK(p->partInfo->name == q->partInfo->name);
            if ((*r)[cParent].is_null()) {
                CHECK(p == v->rootPart);
            } else {
                CHECK(v->parts.at((*r)[cParent].get<size_t>()) == p->parent);
            }
            CHECK((p->fields.attachMode == static_cast<int>(AttachModes::SRF_ATTACH) ? "SRF_ATTACH" : "STACK") == (*r)[cMode].get<std::string>());
            CHECK(p->fields.attachMode == q->fields.attachMode);
            CHECK(p->partTransform.position.Equals(q->partTransform.position));
            CHECK(p->partTransform.rotation.Equals(q->partTransform.rotation));
        }
        FlightRegistry::Clear();
    }
    // the manifest is required (crewManifest.AssignCrewToVessel(ship) on null)
    {
        const fs::path file = fs::path(KSPCORE_PROJECT_ROOT) / "analysis" / "physics" / "craft" / "PhysProf-S.craft";
        ClearIds();
        std::unique_ptr<ShipConstruct> ship = ShipConstruction::LoadShip(file.string());
        REQUIRE(ship != nullptr);
        CHECK_THROWS_AS(ShipConstruction::AssembleForLaunch(*ship, "LaunchPad", "", "", game, nullptr), NullReferenceException);
    }
    // a ship whose parts the ship does not own cannot be handed to a Vessel
    {
        auto a = w.g.New("mk1pod.v2");
        ShipConstruct borrowed("b", "", a.get());
        StubManifest manifest;
        CHECK_THROWS_AS(ShipConstruction::AssembleForLaunch(borrowed, "LaunchPad", "", "", game, &manifest), dotnet::InvalidOperationException);
        a->ship = nullptr;
    }
}

TEST_CASE("WP-18 AssembleForLaunch variants: splashed, not landed, orbit, preCreate, active vessel; findFirst* helpers") {
    if (!GameAvailable()) return;
    World18 w;
    LogCapture quiet;
    ShipConstructionScope scope;
    Game game;
    ShipConstruction::env.CurrentGame = &game;
    const fs::path file = fs::path(KSPCORE_PROJECT_ROOT) / "analysis" / "physics" / "craft" / "PhysProf-S.craft";
    auto load = [&]() {
        ClearIds();
        std::unique_ptr<ShipConstruct> s = ShipConstruction::LoadShip(file.string());
        REQUIRE(s != nullptr);
        return s;
    };
    LaunchPlacement lp;
    lp.rootPosition = w.PlaceKerbinAtPad();
    StubManifest manifest;
    {   // splashed: Landed = false, Splashed = true, skipGroundPositioning = true
        auto s = load();
        auto v = ShipConstruction::AssembleForLaunch(*s, "Water", "", "flag", game, &manifest, true, false, true, false, nullptr, false, true, lp);
        CHECK_FALSE(v->Landed);
        CHECK(v->Splashed);
        CHECK(v->skipGroundPositioning);
        FlightRegistry::Clear();
    }
    {   // not landed: Landed = false, no SetLandedAt
        auto s = load();
        auto v = ShipConstruction::AssembleForLaunch(*s, "LaunchPad", "", "flag", game, &manifest, true, false, false, false, nullptr, false, false, lp);
        CHECK_FALSE(v->Landed);
        CHECK_FALSE(v->Splashed);
        FlightRegistry::Clear();
    }
    {   // empty landedAt: no launchedFrom, vesselSpawning keeps its default
        auto s = load();
        auto v = ShipConstruction::AssembleForLaunch(*s, "", "", "flag", game, &manifest, true, false, true, false, nullptr, false, false, lp);
        CHECK(v->launchedFrom == "");
        FlightRegistry::Clear();
    }
    {   // setActiveVessel
        auto s = load();
        auto v = ShipConstruction::AssembleForLaunch(*s, "LaunchPad", "", "flag", game, &manifest, true, true, true, false, nullptr, false, false, lp);
        CHECK(FlightRegistry::ActiveVessel == v.get());
        FlightRegistry::Clear();
    }
    {   // an orbit: the OrbitDriver exists before Initialize and keeps the orbit object
        auto s = load();
        auto orbit = std::make_shared<Orbit>();
        auto v = ShipConstruction::AssembleForLaunch(*s, "LaunchPad", "", "flag", game, &manifest, true, false, true, false, orbit, false, false, lp);
        CHECK(v->orbitDriver->orbit == orbit);
        FlightRegistry::Clear();
    }
    {   // the default overload: fromShipAssembly, setActiveVessel and isLanded all true
        auto s = load();
        auto v = ShipConstruction::AssembleForLaunch(*s, "LaunchPad", "LP", "flag", game, &manifest, lp);
        CHECK(FlightRegistry::ActiveVessel == v.get());
        CHECK(v->Landed);
        CHECK(v->landedAt == "LaunchPad");
        FlightRegistry::Clear();
    }
    // findFirstCrewablePart: the first part (depth first) with seats, crew aboard and a control source; findFirstPod_Placeholder ignores crew
    {
        auto s = load();
        Part* root = s->liveParts[0];
        CHECK(ShipConstruction::findFirstCrewablePart(root) == nullptr);   // PhysProf-S is a probe: no seats
        auto pod = w.g.New("mk1pod.v2");
        CHECK(pod->fields.CrewCapacity == 1);
        pod->fields.isControlSource = 1;   // ModuleCommand.UpdateControlSourceState sets it in the game (WP-24)
        CHECK(ShipConstruction::findFirstCrewablePart(pod.get()) == nullptr);        // nobody aboard
        CHECK(ShipConstruction::findFirstPod_Placeholder(pod.get()) == pod.get());   // seats + control source is enough
        auto crew = std::make_shared<ProtoCrewMember>(ProtoCrewMember_KerbalType::Crew);
        pod->protoModuleCrew.push_back(crew);
        CHECK(ShipConstruction::findFirstCrewablePart(pod.get()) == pod.get());
        auto tank = w.g.New("fuelTank");
        Attach(*tank, "bottom", *pod, "top");   // the pod below the tank: found through the children
        CHECK(ShipConstruction::findFirstCrewablePart(tank.get()) == pod.get());
        CHECK(ShipConstruction::findFirstPod_Placeholder(tank.get()) == pod.get());
        CHECK(ShipConstruction::findFirstPod_Placeholder(root) == nullptr);
        pod->protoModuleCrew.clear();
        tank->ship = nullptr;
        for (Part* p : s->liveParts) p->ship = nullptr;
    }
    {   // findFirstControlSource: the first part of the vessel with a control level
        auto s = load();
        auto v = ShipConstruction::AssembleForLaunch(*s, "LaunchPad", "", "flag", game, &manifest, true, false, true, false, nullptr, false, false, lp);
        // ModuleCommand.OnAwake (ModuleCommand.cs:366, WP-24) made every command part a FULL control source: the first one is found
        Part* firstCmd = nullptr;
        for (Part* p : v->parts)
            if (firstCmd == nullptr && p->fields.isControlSource > 0) firstCmd = p;
        REQUIRE(firstCmd != nullptr);
        CHECK(ShipConstruction::findFirstControlSource(*v) == firstCmd);
        for (Part* p : v->parts) p->fields.isControlSource = 0;   // no part has a control level yet
        CHECK(ShipConstruction::findFirstControlSource(*v) == nullptr);
        v->parts[3]->fields.isControlSource = 1;
        v->parts[5]->fields.isControlSource = 2;
        CHECK(ShipConstruction::findFirstControlSource(*v) == v->parts[3]);
        FlightRegistry::Clear();
    }
    // GetUniqueFlightID never returns an id already in the flight state, or 0
    {
        FlightState fs;
        const uint32_t id = ShipConstruction::GetUniqueFlightID(&fs);
        CHECK(id != 0);
        CHECK(ShipConstruction::GetUniqueFlightID(nullptr) != 0);
    }
}

TEST_CASE("WP-18 launch-site queries on the physprofile save: FindVesselsLandedAt, CheckLaunchSiteClear, RecoverVesselFromFlight, FindVesselsAtLaunchSite") {
    if (!GameAvailable()) return;
    Game15 g;
    LogCapture quiet;
    GameParameters::GetDefaultParameters(Game_Modes::CAREER, GameParameters::Preset::Normal);
    ConfigNode::NodePtr root = ConfigNode::Load(std::string(KSPCORE_TESTDATA_DIR) + "/saves/physprofile/persistent.sfs");
    REQUIRE(root);
    Game game(root.get());
    REQUIRE(game.flightState);
    FlightState& fs = *game.flightState;
    REQUIRE(fs.protoVessels.size() == 2);
    // the save's VESSEL nodes: an asteroid (landedAt empty) and a probe on the LaunchPad
    ConfigNode* flightState = root->GetNode("GAME")->GetNode("FLIGHTSTATE");
    REQUIRE(flightState != nullptr);
    std::vector<ConfigNode*> vesselNodes = flightState->GetNodes("VESSEL");
    REQUIRE(vesselNodes.size() == 2);
    auto landedOf = [&](size_t i) { return *vesselNodes[i]->GetValuePtr("landedAt"); };
    int pad = -1;
    for (size_t i = 0; i < 2; i++)
        if (landedOf(i) == "LaunchPad") pad = static_cast<int>(i);
    REQUIRE(pad >= 0);
    const ProtoVessel& probe = *fs.protoVessels[static_cast<size_t>(pad)];
    // FindVesselsLandedAt (list form): String.Contains
    {
        auto onPad = ShipConstruction::FindVesselsLandedAt(&fs, "LaunchPad");
        REQUIRE(onPad.size() == 1);
        CHECK(onPad[0] == &probe);
        CHECK(ShipConstruction::FindVesselsLandedAt(&fs, "Pad").size() == 1);          // substring
        CHECK(ShipConstruction::FindVesselsLandedAt(&fs, "Runway").empty());
        CHECK(ShipConstruction::FindVesselsLandedAt(&fs, "").size() == 2);              // "x".Contains("") is true
        CHECK(ShipConstruction::FindVesselsLandedAt(nullptr, "LaunchPad").empty());     // null flight state: empty list
    }
    // the out-parameter form: count, the name / index / type of the first match, then a later vessel only replaces it when it is not Debris
    {
        int count = -1, idx = -1;
        std::string name = "unset";
        VesselType vt = VesselType::Base;
        ShipConstruction::FindVesselsLandedAt(&fs, "LaunchPad", count, name, idx, vt);
        CHECK(count == 1);
        CHECK(name == probe.vesselName);
        CHECK(idx == pad);
        CHECK(vt == probe.vesselType);
        ShipConstruction::FindVesselsLandedAt(&fs, "Runway", count, name, idx, vt);
        CHECK(count == 0);
        CHECK(name == "");
        CHECK(idx == 0);
        CHECK(vt == VesselType::Debris);
        ShipConstruction::FindVesselsLandedAt(nullptr, "LaunchPad", count, name, idx, vt);
        CHECK(count == 0);
        // both vessels match "": the first is taken (flag), the second only replaces it when its type is above Debris
        ShipConstruction::FindVesselsLandedAt(&fs, "", count, name, idx, vt);
        CHECK(count == 2);
        const ProtoVessel& first = *fs.protoVessels[0];
        const ProtoVessel& second = *fs.protoVessels[1];
        const bool secondWins = static_cast<int>(second.vesselType) > static_cast<int>(VesselType::Debris);
        CHECK(idx == (secondWins ? 1 : 0));
        CHECK(name == (secondWins ? second.vesselName : first.vesselName));
    }
    // [Obsolete] FindVesselsAtLaunchSite calls itself: a StackOverflowException in the game
    CHECK_THROWS_WITH_AS(ShipConstruction::FindVesselsAtLaunchSite(&fs, "LaunchPad"), doctest::Contains("StackOverflow"), Exception);
    // CheckLaunchSiteClear: false (a vessel is on the pad); the index of the highest-type vessel; doCleanup removes it and fires onVesselRecovered
    {
        int firstIdx = -2;
        CHECK_FALSE(ShipConstruction::CheckLaunchSiteClear(&fs, "LaunchPad", false, firstIdx));
        CHECK(firstIdx == pad);
        CHECK(fs.protoVessels.size() == 2);
        CHECK(ShipConstruction::CheckLaunchSiteClear(&fs, "Runway", true, firstIdx));
        CHECK(firstIdx == -1);
        CHECK(ShipConstruction::CheckLaunchSiteClear(nullptr, "LaunchPad", true, firstIdx));
        CHECK(ShipConstruction::CheckLaunchSiteClear(&fs, "Runway", false));
        struct Rec {
            std::vector<std::pair<std::string, bool>> events;
            void On(ProtoVessel* pv, bool quick) { events.emplace_back(pv->vesselName, quick); }
        } rec;
        GE::onVesselRecovered.Add(&rec, &Rec::On);
        const std::string probeName = probe.vesselName;
        CHECK_FALSE(ShipConstruction::CheckLaunchSiteClear(&fs, "LaunchPad", true, firstIdx));
        GE::onVesselRecovered.Remove(&rec, &Rec::On);
        REQUIRE(rec.events.size() == 1);
        CHECK(rec.events[0].first == probeName);
        CHECK(rec.events[0].second == false);
        CHECK(fs.protoVessels.size() == 1);
        CHECK(ShipConstruction::FindVesselsLandedAt(&fs, "LaunchPad").empty());
        CHECK(quiet.Has(log::Level::Info, "has vessels on it. - Removing before something stupid happens."));
    }
    // RecoverVesselFromFlight: fires onVesselRecovered(vessel, quick), removes the ProtoVessel from the state; no Vessel -> only the log
    {
        struct Rec {
            std::vector<std::pair<std::string, bool>> events;
            void On(ProtoVessel* pv, bool quick) { events.emplace_back(pv->vesselName, quick); }
        } rec;
        GE::onVesselRecovered.Add(&rec, &Rec::On);
        ProtoVessel& left = *fs.protoVessels[0];
        const std::string leftName = left.vesselName;
        ShipConstruction::RecoverVesselFromFlight(left, fs, true);
        GE::onVesselRecovered.Remove(&rec, &Rec::On);
        REQUIRE(rec.events.size() == 1);
        CHECK(rec.events[0].first == leftName);
        CHECK(rec.events[0].second == true);
        CHECK(fs.protoVessels.empty());
        CHECK(quiet.Has(log::Level::Info, "recovered"));
    }
}

// ====================================================================================================================
// Compatibility checks, craft id sanitising, id tracking modules, recovery of a loaded vessel
// ====================================================================================================================

TEST_CASE("WP-18 AllPartsFound / IsCompatible / FindPartWithCraftID / SanitizeCraftIDs") {
    if (!GameAvailable()) return;
    Game15 g;
    LogCapture quiet;
    ShipConstructionScope scope;
    ConfigNode::NodePtr up = Upgraded(fs::path(KSPCORE_HARNESS_DIR) / "inputs" / "Ships" / "VAB" / "Kerbal 2.craft");
    REQUIRE(up);
    // AllPartsFound: needs a root and a non-null error string
    std::string error = "";
    CHECK(ShipConstruction::AllPartsFound(up.get(), &error));
    CHECK(error.empty());
    CHECK_FALSE(ShipConstruction::AllPartsFound(nullptr, &error));
    CHECK_FALSE(ShipConstruction::AllPartsFound(up.get(), nullptr));
    {   // an unknown part appends the "missing part" text per part and fails
        ConfigNode::NodePtr bad = up->CreateCopy();
        bad->nodes()[0]->SetValue("part", "noSuchPart_1");
        bad->nodes()[1]->SetValue("part", "otherMissing_2");
        std::string e = "prefix:";
        CHECK_FALSE(ShipConstruction::AllPartsFound(bad.get(), &e));
        CHECK(e.rfind("prefix:", 0) == 0);
        CHECK(e.find("noSuchPart") != std::string::npos);
        CHECK(e.find("otherMissing") != std::string::npos);
        // a part with a registered replacement (PartLoader.AddPartReplacement) counts as found: DoesPartHaveReplacement is enough, even
        // when the replacement itself does not exist
        SaveEnvironment::Current().partReplacements.emplace_back("noSuchPart", "fuelTank");
        e = "";
        CHECK_FALSE(ShipConstruction::AllPartsFound(bad.get(), &e));
        CHECK(e.find("noSuchPart") == std::string::npos);
        CHECK(e.find("otherMissing") != std::string::npos);
        SaveEnvironment::Current().partReplacements.emplace_back("otherMissing", "doesNotExistEither");
        e = "";
        CHECK(ShipConstruction::AllPartsFound(bad.get(), &e));
        CHECK(e.empty());
        SaveEnvironment::Current().partReplacements.clear();
    }
    // IsCompatible: the craft version against lastCompatible (0.18.0)
    error = "";
    CHECK(ShipConstruction::IsCompatible(up.get(), &error));
    CHECK(error.empty());
    {
        ConfigNode::NodePtr old = up->CreateCopy();
        old->SetValue("version", "0.17.9");
        std::string e = "";
        CHECK_FALSE(ShipConstruction::IsCompatible(old.get(), &e));
        CHECK_FALSE(e.empty());
        old->SetValue("version", "0.18.0");
        e = "";
        CHECK(ShipConstruction::IsCompatible(old.get(), &e));
        CHECK(e.empty());
        CHECK_FALSE(ShipConstruction::IsCompatible(nullptr, &e));
        CHECK_FALSE(ShipConstruction::IsCompatible(old.get(), nullptr));
    }
    // FindPartWithCraftID looks in EditorLogic.fetch.ship
    ShipConstruct ship;
    std::string err;
    REQUIRE(ship.LoadShipWithParts(*up, 0, true, err));
    CHECK_THROWS_AS(ShipConstruction::FindPartWithCraftID(1), NullReferenceException);   // no editor ship
    ShipConstruction::env.editorShip = &ship;
    for (Part* p : ship.liveParts) CHECK(ShipConstruction::FindPartWithCraftID(p->fields.craftID) == p);
    CHECK(ShipConstruction::FindPartWithCraftID(7) == nullptr);
    // SanitizeCraftIDs: a part of the given list whose craftID another live part also has gets a new id (the given part, or the other
    // one when preserveIDsOnGivenParts); Part.allParts is the scene's part list
    ClearIds();
    ShipConstruct ship2;
    REQUIRE(ship2.LoadShipWithParts(*up, 0, true, err));   // the same craftIDs as `ship`
    Part* mine = ship2.liveParts[2];
    Part* other = ship.liveParts[2];
    REQUIRE(mine->fields.craftID == other->fields.craftID);
    const uint32_t shared = mine->fields.craftID;
    ShipConstruction::SanitizeCraftIDs({mine}, false);
    CHECK(mine->fields.craftID == mine->instanceID);          // (uint)part.GetInstanceID()
    CHECK(other->fields.craftID == shared);
    mine->fields.craftID = shared;
    ShipConstruction::SanitizeCraftIDs({mine}, true);
    CHECK(mine->fields.craftID == shared);
    CHECK(other->fields.craftID == other->instanceID);
    ShipConstruction::env.editorShip = nullptr;
}

namespace {
// ModuleRoboticController (Serenity) implements IShipConstructIDChanges; this stand-in records what LoadShip tells it
class TrackerModule : public PartModule, public IShipConstructIDChanges {
public:
    explicit TrackerModule(const ModuleClassInfo& c) : PartModule(c) {}
    void UpdatePersistentIDs(const std::map<uint32_t, uint32_t>& changed) override {
        seen = changed;
        calls++;
    }
    std::map<uint32_t, uint32_t> seen;
    int calls = 0;
};
}  // namespace

TEST_CASE("WP-18 LoadShip re-issues duplicate part persistentIds and tells IShipConstructIDChanges modules; Part.ship is cleared with the ship") {
    if (!GameAvailable()) return;
    Game15 g;
    LogCapture quiet;
    ShipConstructionScope scope;
    ConfigNode::NodePtr up = Upgraded(fs::path(KSPCORE_HARNESS_DIR) / "inputs" / "Ships" / "VAB" / "Kerbal 2.craft");
    REQUIRE(up);
    // two parts with the same persistentId: the second is re-issued (CheckPartpersistentId), the pair is recorded once, a warning is
    // logged for a third part with the same old id
    ConfigNode::NodePtr dup = up->CreateCopy();
    const std::string id0 = *dup->nodes()[0]->GetValuePtr("persistentId");
    dup->nodes()[1]->SetValue("persistentId", id0);
    dup->nodes()[2]->SetValue("persistentId", id0);
    uint32_t oldId = 0;
    REQUIRE(dotnet::TryParseUInt32(id0, oldId));
    ShipConstruct a;
    TrackerModule* tracker = nullptr;
    std::shared_ptr<TrackerModule> keep;
    ShipConstruct::partTechAvailable = [&](const AvailablePart&) {
        // called by InstantiateParts for each part before the id hand-over: attach the tracker to the root part once
        if (keep == nullptr && !a.ownedParts.empty()) {
            keep = std::make_shared<TrackerModule>(*GetPartModuleClass("PartModule"));
            tracker = keep.get();
            a.ownedParts[0]->Modules().Add(keep);
        }
        return true;
    };
    std::string err;
    REQUIRE(a.LoadShipWithParts(*dup, 0, true, err));
    ShipConstruct::partTechAvailable = nullptr;
    REQUIRE(tracker != nullptr);
    CHECK(a.liveParts[0]->fields.persistentId == oldId);                       // the first part keeps its id
    CHECK(a.liveParts[1]->fields.persistentId != oldId);                       // the second is re-issued
    CHECK(a.liveParts[2]->fields.persistentId != oldId);
    std::set<uint32_t> ids;
    for (Part* p : a.liveParts) ids.insert(p->fields.persistentId);
    CHECK(ids.size() == a.liveParts.size());                                   // all part ids of a ship are distinct
    CHECK(tracker->calls == 1);
    REQUIRE(tracker->seen.size() == 1);                                        // the old id appears once (the dictionary keeps the first pair)
    CHECK(tracker->seen.begin()->first == oldId);
    CHECK(tracker->seen.begin()->second == a.liveParts[1]->fields.persistentId);
    CHECK(quiet.Has(log::Level::Warning, "The same persistentID existed on two parts in this ship"));
    // the registry knows the live parts: loading the same ids again (without clearing) re-issues them all, the first ship keeps its ids
    const uint32_t firstShipId = a.liveParts[3]->fields.persistentId;
    ShipConstruct b;
    REQUIRE(b.LoadShipWithParts(*up, 0, true, err));
    CHECK(a.liveParts[3]->fields.persistentId == firstShipId);
    int clashes = 0;
    std::set<uint32_t> aIds(ids);
    for (Part* p : b.liveParts) clashes += static_cast<int>(aIds.count(p->fields.persistentId));
    CHECK(clashes == 0);
    // Part.ship is cleared when the ship dies (the parts the ship owns do not outlive it here; the destructor must not touch freed parts)
    {
        ShipConstruct tmp;
        ClearIds();
        REQUIRE(tmp.LoadShipWithParts(*up, 0, true, err));
        CHECK(tmp.liveParts[0]->ship == &tmp);
    }
}

TEST_CASE("WP-18 ship costs equal the C9 part database: AvailablePart.cost, module cost and resource cost of every part of every loadable craft") {
    if (!GameAvailable()) return;
    Game15 g;
    LogCapture quiet;
    // harness/captures/out2/part_database.json (C9) table `parts`: cost, module_cost (IPartCostModifier sum of the prefab), resource_cost
    const nlohmann::json cap = LoadJson(kHarness18 + "/captures/out2/part_database.json");
    const nlohmann::json& pt = TableOf(cap, "parts");
    const size_t cName = ColOf(pt, "name"), cCost = ColOf(pt, "cost"), cMod = ColOf(pt, "module_cost"), cRes = ColOf(pt, "resource_cost");
    std::map<std::string, std::array<double, 3>> c9;
    for (const auto& r : pt.at("rows"))
        if (!r[cCost].is_null())
            c9[r[cName].get<std::string>()] = {r[cCost].get<double>(), r[cMod].is_null() ? 0.0 : r[cMod].get<double>(), r[cRes].is_null() ? 0.0 : r[cRes].get<double>()};
    PartResourceLibrary* lib = PartResourceLibrary::Instance();
    size_t parts = 0, resourceCostMatches = 0, defaultFill = 0;
    for (LoadedCraft& lc : LoadAllCrafts()) {
        ShipConstruct& ship = *lc.ship;
        CAPTURE(lc.file.label);
        double expectedDry = 0.0, expectedFuel = 0.0;
        for (Part* p : ship.liveParts) {
            const auto it = c9.find(p->partInfo->name);
            REQUIRE_MESSAGE(it != c9.end(), p->partInfo->name);
            parts++;
            // the loader's cost and the part's module cost are the capture's, bit for bit (float32 values; the JSON text is the shortest round trip)
            CHECK_MESSAGE(p->partInfo->cost == static_cast<float>(it->second[0]), p->partInfo->name);
            CHECK_MESSAGE(p->GetModuleCosts(p->partInfo->cost) == static_cast<float>(it->second[1]), p->partInfo->name);
            // the capture's resource_cost is the prefab's sum of unitCost * maxAmount; the part's own resources give the same sum when the
            // craft did not change the tank sizes (a tweaked max amount changes the dry cost, not the part's price)
            double maxSum = 0.0, amountSum = 0.0;
            for (int i = 0; i < p->Resources.Count(); i++) {
                const PartResource& r = p->Resources[static_cast<size_t>(i)];
                maxSum += static_cast<double>(r.info->unitCost()) * static_cast<float>(r.maxAmount);
                amountSum += static_cast<double>(r.info->unitCost()) * static_cast<float>(r.amount);
            }
            if (std::fabs(maxSum - it->second[2]) <= 1e-4 * std::max(1.0, std::fabs(it->second[2]))) resourceCostMatches++;
            if (std::fabs(maxSum - amountSum) < 1e-9) defaultFill++;
            expectedDry += it->second[0] + it->second[1] - maxSum;
            expectedFuel += amountSum;
        }
        float dry = 0, fuel = 0;
        const float total = ship.GetShipCosts(dry, fuel);
        auto near = [](double a, double b) { return std::fabs(a - b) <= 1e-4 * std::max(1.0, std::fabs(b)); };
        CHECK_MESSAGE(near(dry, expectedDry), dry << " vs " << expectedDry);
        CHECK_MESSAGE(near(fuel, expectedFuel), fuel << " vs " << expectedFuel);
        CHECK_MESSAGE(near(total, expectedDry + expectedFuel), total << " vs " << expectedDry + expectedFuel);
    }
    MESSAGE("parts " << parts << " resource cost == C9 " << resourceCostMatches << " tanks at the default fill " << defaultFill);
    CHECK(parts == 4282);
    CHECK(resourceCostMatches == 4204);   // measured: the other 78 parts have a different resource set than the prefab (tank sizes changed in the craft)
    CHECK(defaultFill > 0);
}
