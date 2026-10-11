// WP-54 test helpers (shared by test_missions.cpp and test_missions_runtime.cpp)
#pragma once

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "doctest/doctest.h"
#include "game_root.hpp"
#include "ksp/config/config_node.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/missions/missions.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/space_center/high_logic.hpp"

namespace ksp::mtest {

using namespace ksp;
using namespace ksp::Expansions::Missions;
using Loc54 = KSP::Localization::Localizer;

inline uint32_t Bits54(float f) {
    uint32_t u;
    std::memcpy(&u, &f, 4);
    return u;
}

inline std::string GameData54() { return ksp::testing::GameRoot() + "/GameData"; }

inline void LoadLocalizer54() {
    Loc54::Shutdown();
    Loc54::Init();
    ConfigNode::NodePtr root = ConfigNode::Load(GameData54() + "/Squad/Localization/dictionary.cfg");
    REQUIRE(root);
    std::vector<ConfigNode*> nodes{root->GetNode("Localization")};
    Loc54::Instance()->AddTagValuesForLanguage(nodes, "en-us");
}

// canonical dump: name{k=v;k=v;child{...}} (values first, then the child nodes: the order ConfigNode prints them in)
inline std::string Dump(const ConfigNode& n) {
    std::string s = n.name + "{";
    for (const auto& v : n.values()) s += v.name + "=" + v.value + ";";
    for (int i = 0; i < n.nodes().Count(); ++i) s += Dump(*n.nodes()[i]);
    return s + "}";
}

struct Fixture54 {
    uint64_t guidCounter = 0;
    HighLogic hl;  // HighLogic.CurrentGame / LoadedScene need the singleton
    Fixture54() {
        HighLogic::ResetStatics();
        hl.Awake();
        hl.GameSaveFolder = "missions/test";  // Mission.Load (MISSION mode) strips the "missions/" prefix of HighLogic.SaveFolder
        MissionsEnvironment::Reset();
        AwardEnvironment::Reset();
        MissionsEnvironment::createSituationDefaults = false;
        MissionsEnvironment::makingHistoryInstalled = [] { return true; };
        MissionsEnvironment::newGuid = [this] {
            Guid g;
            ++guidCounter;
            g.bytes[0] = 0x54;
            for (int i = 0; i < 8; ++i) g.bytes[8 + i] = static_cast<uint8_t>(guidCounter >> (8 * (7 - i)));
            return g;
        };
        TestModuleRegistry::ResetForTests();
        ActionModuleRegistry::ResetForTests();
        DynamicModuleRegistry::ResetForTests();
        MissionScore::ResetGlobalScoreModules();
        DynamicModule::ResetObjectIds();
        RegisterScoreModules();
        RegisterAwardModules();
        MENode::ResetPool();
        SaveEnvironment::Reset();
        LoadLocalizer54();
    }
    ~Fixture54() {
        hl.OnDestroy();
        HighLogic::ResetStatics();
        MissionsEnvironment::Reset();
        AwardEnvironment::Reset();
        TestModuleRegistry::ResetForTests();
        ActionModuleRegistry::ResetForTests();
        DynamicModuleRegistry::ResetForTests();
        MissionScore::ResetGlobalScoreModules();
        MENode::ResetPool();
        Loc54::Shutdown();
        SaveEnvironment::Reset();
    }
};

inline std::string Esc(const std::string& s) {
    std::string t;
    for (char c : s) {
        if (c == '\n') t += "\\n";
        else if (c == '\t') t += "\\t";
        else t += c;
    }
    return t;
}

inline std::string GuidOf(int i) {
    char buf[64];
    std::snprintf(buf, sizeof buf, "6d650000-0000-0000-0000-%012d", i + 1);
    return buf;
}

}  // namespace ksp::mtest
