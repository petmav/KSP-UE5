// WP-38 slice 3: FinePrint.Utilities.CompatibilityUtilities (Assembly-CSharp/FinePrint/Utilities/CompatibilityUtilities.cs): the save upgrade of the
// contract nodes of older KSP versions (UpdateCareerSave, installed as ContractsEnvironment::oldCareerSave / updateCareerSave by FinePrint::InstallAll),
// the version tests and CleanUpUnsanitaryEVAKerbals.
//
// Seam: FindSurveyDefinition reads GameDatabase.GetConfigNodes("Contracts"); CompatibilityEnvironment::contractsConfigNodes answers it (unset: the
// GameDatabase singleton when it exists, else the Contracts node loaded into ContractDefsLite).
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/fineprint/utilities.hpp"
#include "ksp/save/game.hpp"

namespace ksp::Contracts::Lite {

struct CompatibilityEnvironment {
    // GameDatabase.Instance.GetConfigNodes("Contracts")
    static inline std::function<std::vector<ConfigNode*>()> contractsConfigNodes;
};

// CompatibilityUtilities.cs:7
struct CompatibilityUtilities {
    static inline int majorVersion = 2147483647;      // :9
    static inline int minorVersion = 2147483647;      // :11
    static inline int revisionVersion = 2147483647;   // :13
    static bool OldCareerSave(Game::Modes mode, int saveMajor, int saveMinor, int saveRevision);   // :15
    static bool OldCareerSave(Game::Modes mode, const std::string& versionString);                 // :42
    static void UpdateCareerSave(ConfigNode& node);                                                // :64
    static std::unique_ptr<SurveyDefinition> FindSurveyDefinition(const std::string& experiment);  // :479
    static bool SavePriorTo(int major, int minor, int revision);                                   // :519
    static std::string UpgradeVesselSituation(const std::string& situation);                       // :524
    static void CleanUpUnsanitaryEVAKerbals(ConfigNode& flightState);                              // :588
};

}  // namespace ksp::Contracts::Lite
