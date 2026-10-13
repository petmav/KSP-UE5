// WP-54: MissionsEnvironment (the seams of the Expansions.Missions port; see mission_types.hpp).
#include "ksp/celestial/planetarium.hpp"
#include "ksp/missions/mission_types.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/space_center/high_logic.hpp"

namespace ksp::Expansions::Missions {

// Guid.NewGuid(): unset = a deterministic counter guid
Guid MissionsEnvironment::NewGuid() {
    if (newGuid) return newGuid();
    static uint64_t counter = 0;
    Guid g;
    ++counter;
    for (int i = 0; i < 8; ++i) g.bytes[8 + i] = static_cast<uint8_t>(counter >> (8 * (7 - i)));
    g.bytes[0] = 0x6d;
    g.bytes[1] = 0x65;
    return g;
}

int64_t MissionsEnvironment::NowTicks() { return nowTicks ? nowTicks() : 0; }

// Planetarium.GetUniversalTime()
double MissionsEnvironment::UniversalTime() {
    if (universalTime) return universalTime();
    if (Planetarium::fetch != nullptr) return Planetarium::GetUniversalTime();
    Game* g = HighLogic::CurrentGame();
    return g != nullptr ? g->UniversalTime() : 0.0;
}

// HighLogic.CurrentGame.UniversalTime
double MissionsEnvironment::GameUniversalTime() {
    if (gameUniversalTime) return gameUniversalTime();
    Game* g = HighLogic::CurrentGame();
    return g != nullptr ? g->UniversalTime() : 0.0;
}

// ExpansionsLoader.IsExpansionInstalled("MakingHistory")
bool MissionsEnvironment::MakingHistoryInstalled() {
    if (makingHistoryInstalled) return makingHistoryInstalled();
    return SaveEnvironment::Current().IsExpansionInstalled("MakingHistory");
}

// MissionSystem.Instance.StartCoroutine(routine)
void MissionsEnvironment::StartCoroutine(Enumerator routine) {
    if (startCoroutine) {
        startCoroutine(std::move(routine));
        return;
    }
    HighLogic::Scheduler().StartCoroutine(std::move(routine));
}

// ScreenMessages.PostScreenMessage(text, duration, UPPER_LEFT, persist: true)
void MissionsEnvironment::PostScreenMessage(const std::string& text, float duration) {
    if (postScreenMessage) {
        postScreenMessage(text, duration);
        return;
    }
    screenMessages.emplace_back(text, duration);
}

// MissionSystem.IsActive
bool MissionsEnvironment::MissionSystemActive() { return missionSystemActive ? missionSystemActive() : true; }

void MissionsEnvironment::Reset() {
    newGuid = nullptr;
    nowTicks = nullptr;
    universalTime = nullptr;
    gameUniversalTime = nullptr;
    makingHistoryInstalled = nullptr;
    makingHistorySignedMission = nullptr;
    buildMissionFileHash = nullptr;
    startCoroutine = nullptr;
    postScreenMessage = nullptr;
    screenMessages.clear();
    missionSystemActive = nullptr;
    editorMission = nullptr;
    setLockedCamera = nullptr;
    validatorSetting = Editor::ValidatorMode::AutoAfterRun;
    createSituationDefaults = true;
    analytics = nullptr;
    updateValidationLed = nullptr;
    basicNodeIconURL = nullptr;
    categoryColor = nullptr;
    addNodeLabel = nullptr;
    removeNodeLabel = nullptr;
    loadedPartNames = nullptr;
    craftFileBookkeeping = false;
    refreshMissionRequested = nullptr;
}

}  // namespace ksp::Expansions::Missions
