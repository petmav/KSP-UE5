// WP-15 test fixture: the stock game loaded through GameData (mirror or KSP_GAMEDATA_ROOT), parts instantiated like
// ShipConstruction.CreatePart, helpers for attaching parts and counting events.
#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include "craft_tree.hpp"
#include "doctest/doctest.h"
#include "game_root.hpp"
#include "ksp/core/log.hpp"
#include "ksp/database/game_data.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/parts/part_loader.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/resources/planetary_resources.hpp"
#include "ksp/resources/resource_broker.hpp"
#include "ksp/save/save_environment.hpp"

namespace ksp::rtest {

inline bool GameAvailable() { return std::filesystem::exists(std::filesystem::path(ksp::testing::GameRoot()) / "GameData" / "Squad"); }

// The stock game. One per test case: the singletons the loaders install belong to the most recent one.
struct Game15 {
    GameData data;
    Game15() {
        SaveEnvironment::Reset();
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
        ResourceSetup::database = &data.database;
        ResourceSetup::ResetConfig();
        ResourceCache::getConfigNodes = [this](const std::string& t) { return data.database.GetConfigNodes(t); };
    }
    ~Game15() {
        ResourceSetup::database = nullptr;
        ResourceSetup::ResetConfig();
        ResourceEnvironment::bodies = nullptr;
        SaveEnvironment::Reset();
    }
    const AvailablePart& Info(const char* name) {
        const AvailablePart* p = data.parts.getPartInfoByName(name);
        REQUIRE_MESSAGE(p != nullptr, name);
        return *p;
    }
    // ShipConstruction.CreatePart-like: Instantiate(prefab) + partInfo + name
    std::unique_ptr<Part> New(const char* name) {
        const AvailablePart& ap = Info(name);
        auto p = Part::Instantiate(*ap.partPrefab);
        p->partInfo = &ap;
        p->gameObjectName = ap.name;
        return p;
    }
};

// parent.nodeA <-> child.nodeB, child parented to parent
inline void Attach(Part& parent, const char* parentNode, Part& child, const char* childNode) {
    AttachNode* pn = parent.FindAttachNode(parentNode);
    AttachNode* cn = child.FindAttachNode(childNode);
    REQUIRE(pn != nullptr);
    REQUIRE(cn != nullptr);
    pn->attachedPart = &child;
    cn->attachedPart = &parent;
    child.setParent(&parent);
}

// Counts the part-resource events (PartResource flow/amount transitions, list changes)
struct EventLog {
    std::vector<std::string> events;
    EventLog() {
        GameEvents::onPartResourceEmptyNonempty.Add(this, &EventLog::EN);
        GameEvents::onPartResourceNonemptyEmpty.Add(this, &EventLog::NE);
        GameEvents::onPartResourceEmptyFull.Add(this, &EventLog::EF);
        GameEvents::onPartResourceFullEmpty.Add(this, &EventLog::FE);
        GameEvents::onPartResourceFullNonempty.Add(this, &EventLog::FN);
        GameEvents::onPartResourceNonemptyFull.Add(this, &EventLog::NF);
        GameEvents::onPartResourceListChange.Add(this, &EventLog::LC);
        GameEvents::onPartResourceFlowStateChange.Add(this, &EventLog::FS);
        GameEvents::onPartResourceFlowModeChange.Add(this, &EventLog::FM);
    }
    ~EventLog() {
        GameEvents::onPartResourceEmptyNonempty.Remove(this, &EventLog::EN);
        GameEvents::onPartResourceNonemptyEmpty.Remove(this, &EventLog::NE);
        GameEvents::onPartResourceEmptyFull.Remove(this, &EventLog::EF);
        GameEvents::onPartResourceFullEmpty.Remove(this, &EventLog::FE);
        GameEvents::onPartResourceFullNonempty.Remove(this, &EventLog::FN);
        GameEvents::onPartResourceNonemptyFull.Remove(this, &EventLog::NF);
        GameEvents::onPartResourceListChange.Remove(this, &EventLog::LC);
        GameEvents::onPartResourceFlowStateChange.Remove(this, &EventLog::FS);
        GameEvents::onPartResourceFlowModeChange.Remove(this, &EventLog::FM);
    }
    void EN(PartResource*) { events.push_back("EmptyNonempty"); }
    void NE(PartResource*) { events.push_back("NonemptyEmpty"); }
    void EF(PartResource*) { events.push_back("EmptyFull"); }
    void FE(PartResource*) { events.push_back("FullEmpty"); }
    void FN(PartResource*) { events.push_back("FullNonempty"); }
    void NF(PartResource*) { events.push_back("NonemptyFull"); }
    void LC(Part*) { events.push_back("ListChange"); }
    void FS(GameEvents::HostedFromToAction<PartResource*, bool> d) {
        events.push_back(std::string("FlowState:") + (d.from ? "1" : "0") + ">" + (d.to ? "1" : "0"));
    }
    void FM(GameEvents::HostedFromToAction<PartResource*, PartResource_FlowMode> d) {
        events.push_back("FlowMode:" + std::to_string(static_cast<int>(d.from)) + ">" + std::to_string(static_cast<int>(d.to)));
    }
    void Clear() { events.clear(); }
};

struct LogCapture {
    std::vector<std::pair<log::Level, std::string>> lines;
    LogCapture() {
        log::SetSink([this](log::Level l, std::string_view m) { lines.emplace_back(l, std::string(m)); });
    }
    ~LogCapture() { log::SetSink(nullptr); }
    bool Has(log::Level l, const std::string& needle) const {
        for (const auto& [lv, m] : lines)
            if (lv == l && m.find(needle) != std::string::npos) return true;
        return false;
    }
};

// SaveEnvironment::loadedScene for the duration of a scope (HighLogic.LoadedScene)
struct SceneScope {
    GameScenes saved;
    explicit SceneScope(GameScenes s) : saved(SaveEnvironment::Current().loadedScene) { SaveEnvironment::Current().loadedScene = s; }
    ~SceneScope() { SaveEnvironment::Current().loadedScene = saved; }
};

inline int Id(const char* resource) { return MonoStringHash(resource); }

// Three FL-T400 tanks stacked A (top) - B - C (bottom) as a vessel. inverseStage A=0, B=1, C=2 => resource priorities 0, 10, 20
// (Part.GetResourcePriority = inverseStage * 10 + resourcePriorityOffset). LiquidFuel amount/max set to a/b/c and 400.
struct Stack3 {
    std::unique_ptr<Part> A, B, C;
    support::TestVessel vessel;
    int lf = Id("LiquidFuel");
    Stack3(Game15& g, double a = 100.0, double b = 200.0, double c = 300.0) {
        A = g.New("fuelTank");
        B = g.New("fuelTank");
        C = g.New("fuelTank");
        A->fields.persistentId = 101;  // ListExtension.ContainsId compares persistentId: real parts have distinct ones
        B->fields.persistentId = 102;
        C->fields.persistentId = 103;
        Attach(*A, "bottom", *B, "top");
        Attach(*B, "bottom", *C, "top");
        A->fields.inverseStage = 0;
        B->fields.inverseStage = 1;
        C->fields.inverseStage = 2;
        Set(a, b, c);
        vessel.Adopt({A.get(), B.get(), C.get()});
    }
    static void SetOne(Part& p, double amount) {
        PartResource* r = p.GetResource("LiquidFuel");
        r->maxAmount = 400.0;
        r->amount = amount;
        p.ResetSimulationResources();
    }
    void Set(double a, double b, double c) {
        SetOne(*A, a);
        SetOne(*B, b);
        SetOne(*C, c);
    }
    double Amount(Part& p) { return p.GetResource("LiquidFuel")->amount; }
    double Sim(Part& p) { return p.SimulationResources().Get("LiquidFuel")->amount; }
};

}  // namespace ksp::rtest
