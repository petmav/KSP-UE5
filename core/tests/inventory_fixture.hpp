// WP-33 test fixture: the helpers shared by test_inventory.cpp (Python reference gates), test_inventory_rules.cpp and test_construction.cpp.
#pragma once

#include <cmath>
#include <cstring>
#include <fstream>

#include "doctest/doctest.h"
#include "nlohmann/json.hpp"
#include "resource_fixture.hpp"

#include "ksp/core/exceptions.hpp"
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

namespace ksp::inv33 {

using namespace ksp::rtest;
using Localizer = ksp::KSP::Localization::Localizer;

inline nlohmann::json LoadRef() {
    std::ifstream f(std::string(KSPCORE_TEST_DATA_DIR) + "/wp33_reference.json", std::ios::binary);
    REQUIRE(f.good());
    return nlohmann::json::parse(f);
}

inline uint32_t Bits(float f) {
    uint32_t u;
    std::memcpy(&u, &f, 4);
    return u;
}
inline float FromBits(uint32_t u) {
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

inline ModuleInventoryPart* Inv(Part& p) { return Mod<ModuleInventoryPart>(p, "ModuleInventoryPart"); }

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

inline const AvailablePart& InfoOf(Game15& g, const std::string& n) { return g.Info(n.c_str()); }
inline Part& Prefab(Game15& g, const std::string& n) { return *g.Info(n.c_str()).partPrefab; }


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
inline std::unique_ptr<Vessel> BareVessel(VesselType type, const Vector3f& pos) {
    auto v = std::make_unique<Vessel>();
    v->vesselType = type;
    v->loaded = true;
    v->vesselTransform.position = pos;
    return v;
}


// the English tag table of the stock game (Squad/Localization/dictionary.cfg)
inline void LoadLocalizer() {
    Localizer::Shutdown();
    Localizer::Init();
    ConfigNode::NodePtr root = ConfigNode::Load(ksp::testing::GameRoot() + "/GameData/Squad/Localization/dictionary.cfg");
    REQUIRE(root);
    std::vector<ConfigNode*> nodes{root->GetNode("Localization")};
    Localizer::Instance()->AddTagValuesForLanguage(nodes, "en-us");
}

struct Counts {
    int changed = 0;
    std::vector<int> slots;
    void C(ModuleInventoryPart*) { changed++; }
    void S(ModuleInventoryPart*, int s) { slots.push_back(s); }
    Counts() {
        GameEvents::onModuleInventoryChanged.Add(this, &Counts::C);
        GameEvents::onModuleInventorySlotChanged.Add(this, &Counts::S);
    }
    ~Counts() {
        GameEvents::onModuleInventoryChanged.Remove(this, &Counts::C);
        GameEvents::onModuleInventorySlotChanged.Remove(this, &Counts::S);
    }
};

inline std::string Msg(const char* tag, const std::string& arg) { return Localizer::Format(tag, {arg}); }

inline bool Has(const std::vector<std::string>& v, const std::string& s) { return std::find(v.begin(), v.end(), s) != v.end(); }

// the EVA kerbal vessel the construction code asks (seated / engineer answers by pointer)
struct Kerbals {
    std::vector<std::unique_ptr<Vessel>> vessels;
    std::vector<std::unique_ptr<FakeEva>> controllers;
    std::vector<std::unique_ptr<Part>> rootParts;  // the kerbal's own part
    std::map<Vessel*, bool> seated;
    std::map<Vessel*, int> engineers;
    Vessel* Add(const Vector3f& pos, bool isSeated = false, int engineer = 0) {
        vessels.push_back(BareVessel(VesselType::EVA, pos));
        controllers.push_back(std::make_unique<FakeEva>());
        Vessel* v = vessels.back().get();
        v->evaController = controllers.back().get();
        rootParts.push_back(std::make_unique<Part>());
        v->rootPart = rootParts.back().get();
        v->parts.push_back(v->rootPart);
        seated[v] = isSeated;
        engineers[v] = engineer;
        FlightRegistry::VesselsLoaded.push_back(v);
        return v;
    }
    Kerbals() {
        EVAConstructionEnvironment::isSeated = [this](Vessel& v) { return seated[&v]; };
        EVAConstructionEnvironment::engineerCount = [this](Vessel& v) { return engineers[&v]; };
    }
};

struct Weight {
    int fired = 0;
    void F() { fired++; }
    Weight() { GameEvents::OnCombinedConstructionWeightLimitChanged.Add(this, &Weight::F); }
    ~Weight() { GameEvents::OnCombinedConstructionWeightLimitChanged.Remove(this, &Weight::F); }
};

struct PartEvents {
    std::vector<std::pair<ConstructionEventType, Part*>> events;
    void E(ConstructionEventType t, Part* p) { events.emplace_back(t, p); }
    PartEvents() { GameEvents::onEditorPartEvent.Add(this, &PartEvents::E); }
    ~PartEvents() { GameEvents::onEditorPartEvent.Remove(this, &PartEvents::E); }
    int Count(ConstructionEventType t) const {
        int n = 0;
        for (const auto& e : events) n += e.first == t;
        return n;
    }
};


}  // namespace ksp::inv33
