// WP-33 unit tests: inventory (StoredPart, ModuleCargoPart, ModuleInventoryPart, KerbalInventoryScenario) and EVA construction
// (EVAConstructionUtil / ModeController / ModeEditor, PartItemTransfer, the inventory UI models).
//
// The mass / volume / cost maths, the slot bookkeeping, the construction weight limit and the info-label text are checked against
// tools/wp33/inventory_reference.py (independent Python/numpy; tests/data/wp33_reference.json) at 0 ULP, on the stock parts of
// core/testdata/GameData-mirror. The state machines are checked against the C# line by line (the derivation is in the comment above
// each check).
#include <cmath>
#include <cstring>
#include <fstream>

#include "doctest/doctest.h"
#include "nlohmann/json.hpp"
#include "resource_fixture.hpp"

#include "ksp/core/unity_time.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/inventory/construction.hpp"
#include "ksp/inventory/inventory.hpp"
#include "ksp/kerbals/experience.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/modules/module_part_variants.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/vessel/vessel.hpp"

using namespace ksp;
using namespace ksp::rtest;
using Localizer = ksp::KSP::Localization::Localizer;

namespace {

nlohmann::json LoadRef() {
    std::ifstream f(std::string(KSPCORE_TEST_DATA_DIR) + "/wp33_reference.json", std::ios::binary);
    REQUIRE(f.good());
    return nlohmann::json::parse(f);
}

uint32_t Bits(float f) {
    uint32_t u;
    std::memcpy(&u, &f, 4);
    return u;
}
float FromBits(uint32_t u) {
    float f;
    std::memcpy(&f, &u, 4);
    return f;
}

struct Reset33 {
    Reset33() { Clear(); }
    ~Reset33() { Clear(); }
    static void Clear() {
        InventoryEnvironment::Reset();
        EVAConstructionEnvironment::Reset();
        KerbalInventoryScenario::ClearStatics();
        KerbalInventory::ClearAll();
        KerbalInventory::Uninstall();
        EVAConstructionModeController::Instance = nullptr;
        UIPartActionControllerInventory::Instance = nullptr;
        InventoryPanelController::Instance = nullptr;
        UIPartActionControllerInventory::heldPartIsStack = false;
        UIPartActionControllerInventory::stackSize = 0;
        UIPartActionControllerInventory::amountToStore = 0;
        PartItemTransfer::Instance = nullptr;
        FlightRegistry::Clear();
    }
};

template <class T>
T* Mod(Part& p, const char* cls) {
    auto* m = dynamic_cast<T*>(p.FindModuleOfClass(cls));
    REQUIRE(m != nullptr);
    return m;
}

ModuleInventoryPart* Inv(Part& p) { return Mod<ModuleInventoryPart>(p, "ModuleInventoryPart"); }

// the stock inventory part, started in the editor scene (OnStart: InitializeInventory, event subscriptions)
struct Host {
    std::unique_ptr<Part> part;
    ModuleInventoryPart* inv = nullptr;
    Host(Game15& g, const char* name) {
        part = g.New(name);
        inv = Inv(*part);
        inv->OnStart(StartState::Editor);
    }
};

const AvailablePart& InfoOf(Game15& g, const std::string& n) { return g.Info(n.c_str()); }
Part& Prefab(Game15& g, const std::string& n) { return *g.Info(n.c_str()).partPrefab; }

}  // namespace

// ================================================================ the Python reference

// ModuleInventoryPart.cs:762 / 429 / 847 / 1881 / 1429 / 1928 / 1966 / 1461 / 1508 / 1576 / 1130 against inventory_reference.py:
// the stock containers filled by a seeded random operation sequence, every number at 0 ULP
TEST_CASE("WP-33 ModuleInventoryPart: store / stack / remove sequences, mass / volume / cost, previews and limits equal the Python reference") {
    if (!GameAvailable()) return;
    Reset33 r;
    Game15 g;
    SceneScope scene(GameScenes::EDITOR);
    const nlohmann::json ref = LoadRef();
    int fills = 0, ops = 0, costChecked = 0, costSkipped = 0, previews = 0, caps = 0, reachedVol = 0, reachedMass = 0;
    for (const auto& s : ref["fills"]) {
        const std::string label = s["inventory"];
        Host host(g, label.c_str());
        ModuleInventoryPart* inv = host.inv;
        const auto& cfg = ref["inventories"][label];
        CHECK(inv->InventorySlots() == cfg["slots"].get<int>());
        CHECK(Bits(inv->packedVolumeLimit()) == Bits(cfg["volumeLimit"].get<float>()));
        CHECK(Bits(inv->massLimit()) == Bits(cfg["massLimit"].get<float>()));
        for (const auto& op : s["ops"]) {
            const std::string kind = op["op"];
            if (kind == "store") {
                CHECK(inv->StoreCargoPartAtSlot(&Prefab(g, op["part"]), op["slot"].get<int>()));
            } else if (kind == "setq") {
                CHECK(inv->UpdateStackAmountAtSlot(op["slot"].get<int>(), op["n"].get<int>()));
            } else if (kind == "removeN") {
                CHECK(inv->RemoveNPartsFromInventory(op["part"], op["n"].get<int>()) == op["ok"].get<bool>());
            } else if (kind == "clear") {
                CHECK(inv->ClearPartAtSlot(op["slot"].get<int>()));
            }
            ops++;
        }
        // the DictionaryValueList order, quantities and capacities
        const auto& vl = inv->storedParts.ValuesList();
        REQUIRE(vl.size() == s["stored"].size());
        for (size_t i = 0; i < vl.size(); i++) {
            CHECK(vl[i]->slotIndex == s["stored"][i]["slot"].get<int>());
            CHECK(vl[i]->partName == s["stored"][i]["part"].get<std::string>());
            CHECK(vl[i]->quantity == s["stored"][i]["quantity"].get<int>());
            CHECK(vl[i]->stackCapacity == s["stored"][i]["stackCapacity"].get<int>());
        }
        CHECK(Bits(inv->volumeOccupied) == s["volumeOccupied"].get<uint32_t>());
        CHECK(Bits(inv->massOccupied) == s["massOccupied"].get<uint32_t>());
        CHECK(Bits(inv->GetModuleMass(0.0f, ModifierStagingSituation::CURRENT)) == s["moduleMass"].get<uint32_t>());
        bool modCosts = false;
        for (const auto& sp : vl)
            if (sp->snapshot->moduleCosts != 0.0f) modCosts = true;
        if (!modCosts) {
            CHECK(Bits(inv->GetModuleCost(0.0f, ModifierStagingSituation::CURRENT)) == s["moduleCost"].get<uint32_t>());
            costChecked++;
        } else {
            costSkipped++;  // moduleCosts != 0 (a part-cost-modifier module): the reference assumes 0
        }
        CHECK(inv->volumeCapacityReached == s["volumeCapacityReached"].get<bool>());
        CHECK(inv->massCapacityReached == s["massCapacityReached"].get<bool>());
        reachedVol += inv->volumeCapacityReached;
        reachedMass += inv->massCapacityReached;
        CHECK(inv->TotalEmptySlots() == s["emptySlots"].get<int>());
        CHECK(inv->FirstEmptySlot() == s["firstEmpty"].get<int>());
        ConfigNode save("MODULE");
        inv->OnSave(save);
        std::string invString;
        save.TryGetValue("inventory", invString);
        CHECK(invString == s["inventoryString"].get<std::string>());
        for (const auto& h : s["hasCapacity"]) {
            CHECK(Bits(inv->GetPartMass(*host.part)) == cfg["hostMass_bits"].get<uint32_t>());
            CHECK(inv->HasCapacity(Prefab(g, h["part"])) == h["result"].get<bool>());
            caps++;
        }
        for (const auto& p : s["previews"]) {
            inv->volumeCapacity = inv->volumeOccupied;
            inv->massCapacity = inv->massOccupied;
            const int slot = p["slot"].get<int>();
            const std::string slotPart = p["slotPart"];
            const AvailablePart* sp = slotPart.empty() ? nullptr : &InfoOf(g, slotPart);
            CHECK((sp == nullptr) == inv->IsSlotEmpty(slot));
            inv->PreviewLimits(Prefab(g, p["part"]), p["amount"].get<int>(), sp, slot);
            CHECK(Bits(inv->volumeCapacity) == p["volumeCapacity"].get<uint32_t>());
            CHECK(Bits(inv->massCapacity) == p["massCapacity"].get<uint32_t>());
            CHECK(inv->volumeCapacityReached == p["volReached"].get<bool>());
            CHECK(inv->massCapacityReached == p["massReached"].get<bool>());
            previews++;
        }
        fills++;
    }
    MESSAGE("inventory reference: " << fills << " fills, " << ops << " operations, " << previews << " previews, " << caps << " HasCapacity answers, cost compared "
                                    << costChecked << " (skipped " << costSkipped << "), over-volume " << reachedVol << ", over-mass " << reachedMass);
    CHECK(fills >= 30);
    CHECK(costChecked > 0);
}

// single parts: ModuleInventoryPart.cs:1429 / 1928 / 1966 for each stock cargo part at quantity 1 and at a full stack
TEST_CASE("WP-33 ModuleInventoryPart: one stored part per stock cargo part (quantity 1 and full stack) equals the Python reference") {
    if (!GameAvailable()) return;
    Reset33 r;
    Game15 g;
    SceneScope scene(GameScenes::EDITOR);
    const nlohmann::json ref = LoadRef();
    int n = 0, withResources = 0;
    for (const auto& s : ref["singles"]) {
        Host host(g, "cargoContainer");
        ModuleInventoryPart* inv = host.inv;
        const std::string name = s["part"];
        REQUIRE(inv->StoreCargoPartAtSlot(&Prefab(g, name), 0));
        if (s["quantity"].get<int>() > 1) {
            CHECK(inv->UpdateStackAmountAtSlot(0, s["quantity"].get<int>()));
        }
        CHECK(Bits(inv->volumeOccupied) == s["volumeOccupied"].get<uint32_t>());
        CHECK(Bits(inv->massOccupied) == s["massOccupied"].get<uint32_t>());
        CHECK(Bits(inv->GetModuleMass(0.0f, ModifierStagingSituation::CURRENT)) == s["moduleMass"].get<uint32_t>());
        if (inv->storedParts[0]->snapshot->moduleCosts == 0.0f)
            CHECK(Bits(inv->GetModuleCost(0.0f, ModifierStagingSituation::CURRENT)) == s["moduleCost"].get<uint32_t>());
        withResources += Prefab(g, name).Resources.Count() > 0;
        n++;
    }
    // the part table: GetPartMass (:1571) of every cargo part with its resource mass (Part.GetResourceMass, reverse accumulation)
    for (const auto& p : ref["parts"]) {
        Part& part = Prefab(g, p["name"]);
        CHECK(Bits(part.fields.mass) == p["mass_bits"].get<uint32_t>());
        CHECK(Bits(part.GetResourceMass()) == p["resourceMass_bits"].get<uint32_t>());
        Host host(g, "cargoContainer");
        CHECK(Bits(host.inv->GetPartMass(part)) == p["partMass_bits"].get<uint32_t>());
        auto* cargo = Mod<ModuleCargoPart>(part, "ModuleCargoPart");
        CHECK(Bits(cargo->packedVolume()) == p["packedVolume_bits"].get<uint32_t>());
        CHECK(cargo->stackableQuantity() == p["stackable"].get<int>());
    }
    MESSAGE("single-part reference: " << n << " rows, " << withResources << " with resources, " << ref["parts"].size() << " parts");
    CHECK(n > 150);
}

// ================================================================ EVA construction weight limit and info labels

namespace {

struct FakeEva : IEvaController {
    Part* LadderPart() override { return nullptr; }
};

// PhysicsGlobals from the game's Physics.cfg (constructionWeightLimit 588.399)
struct PhysicsWorld {
    PhysicsGlobals pg;
    PhysicsWorld() {
        pg.physicsDatabaseFilename = ksp::testing::GameRoot() + "/Physics.cfg";
        pg.Awake("");
        REQUIRE(pg.LoadDatabase());
    }
};

// a bare vessel: type, situation flag and the gravity the construction code reads
std::unique_ptr<Vessel> BareVessel(VesselType type, const Vector3f& pos) {
    auto v = std::make_unique<Vessel>();
    v->vesselType = type;
    v->vesselTransform.position = pos;
    return v;
}

}  // namespace

// Part.cs:4060 + EVAConstructionUtil.cs:3 + EVAConstructionModeEditor.cs:1265 against inventory_reference.py: partWeight, the combined limit for
// 0..4 assisting kerbals and the verdict for every stock cargo part at six gravities
TEST_CASE("WP-33 EVA construction weight limit: partWeight and IsUnderConstructionWeightLimit for every stock cargo part, 0..4 assistants, equal the Python reference") {
    if (!GameAvailable()) return;
    Reset33 r;
    Game15 g;
    PhysicsWorld phys;
    SceneScope scene(GameScenes::FLIGHT);
    const nlohmann::json ref = LoadRef();
    CHECK(PhysicsGlobals::ConstructionWeightLimit() == ref["physics"]["constructionWeightLimit"].get<double>());
    CHECK(PhysicsGlobals::ConstructionWeightLimitPerKerbalCombine() == ref["physics"]["constructionWeightLimitPerKerbalCombine"].get<double>());
    int rows = 0, under = 0, over = 0;
    for (int assisting = 0; assisting <= 4; assisting++) {
        // the active EVA kerbal plus `assisting` others in range (all non-seated, any trait: COMBINE_NONENGINEERS default true)
        FlightRegistry::Clear();
        std::vector<std::unique_ptr<Vessel>> eva;
        std::vector<std::unique_ptr<FakeEva>> ctl;
        for (int i = 0; i <= assisting; i++) {
            eva.push_back(BareVessel(VesselType::EVA, Vector3f(float(i), 0.0f, 0.0f)));
            ctl.push_back(std::make_unique<FakeEva>());
            eva.back()->evaController = ctl.back().get();
            FlightRegistry::VesselsLoaded.push_back(eva.back().get());
        }
        FlightRegistry::ActiveVessel = eva[0].get();
        EVAConstructionModeController controller;
        EVAConstructionModeEditor editor;
        controller.evaEditor = &editor;
        EVAConstructionModeController::Instance = &controller;
        InventoryEnvironment::EVA_CONSTRUCTION_COMBINE_ENABLED = assisting > 0;
        editor.UpdateAssistingKerbals();
        CHECK(editor.AssistingKerbals() == assisting);
        if (assisting > 0) CHECK(editor.CombinedConstructionWeightLimit() == doctest::Approx(588.399 * (assisting + 1)));
        auto landed = BareVessel(VesselType::Debris, Vector3f(50.0f, 0.0f, 0.0f));
        landed->Landed = true;
        for (const auto& w : ref["weights"]) {
            Part& part = Prefab(g, w["part"]);
            const double gee = w["gee"].get<double>();
            landed->gravityForPos = Vector3d(0.0, 0.0, gee);
            Vessel* saved = part.vessel;
            part.vessel = landed.get();
            const bool verdict = EVAConstructionUtil::IsUnderConstructionWeightLimit(part);
            part.vessel = saved;
            CHECK(part.partWeight == w["partWeight"].get<double>());
            CHECK(verdict == w["under" + std::to_string(assisting)].get<bool>());
            verdict ? under++ : over++;
            rows++;
        }
    }
    MESSAGE("construction weight reference: " << rows << " rows (" << under << " under, " << over << " over the limit)");
    CHECK(under > 0);
    CHECK(over > 0);
}

// EVAConstructionModeController.cs:604: the "{0:F2}t" text and the assisting-kerbal count, 35 (limit, gravity) pairs
TEST_CASE("WP-33 EVAConstructionModeController.UpdateInfoLabels: max-mass text equals the Python reference") {
    Reset33 r;
    const nlohmann::json ref = LoadRef();
    Localizer::Shutdown();
    Localizer::Init();
    EVAConstructionModeController controller;
    EVAConstructionModeEditor editor;
    controller.evaEditor = &editor;
    int far = 0;
    for (const auto& l : ref["labels"]) {
        editor.assistingKerbals = l["assisting"].get<int>();
        editor.combinedConstructionWeightLimit = l["limit"].get<double>();
        controller.constructionGravity = l["gee"].get<double>();
        controller.UpdateInfoLabels();
        const std::string expect = l["text"].get<std::string>();
        if (expect.rfind("#autoLOC_", 0) == 0) {
            CHECK(controller.MaxMassLimitNumberText == Localizer::Format(expect));  // >= 100000: "unlimited"
            far++;
        } else {
            CHECK(controller.MaxMassLimitNumberText == expect);
        }
        CHECK(controller.AssistingKerbalsNumberText == std::to_string(l["assisting"].get<int>()));
    }
    CHECK(far > 0);
    // the label tag follows COMBINE_NONENGINEERS
    InventoryEnvironment::EVA_CONSTRUCTION_COMBINE_NONENGINEERS = false;
    controller.UpdateInfoLabels();
    CHECK(controller.AssistingKerbalsLabelText == Localizer::Format("#autoLOC_8014164"));
    InventoryEnvironment::EVA_CONSTRUCTION_COMBINE_NONENGINEERS = true;
    controller.UpdateInfoLabels();
    CHECK(controller.AssistingKerbalsLabelText == Localizer::Format("#autoLOC_8014163"));
}
