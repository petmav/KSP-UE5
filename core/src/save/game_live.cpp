// WP-19c: the live half of Game (Game.cs:500 Start, :539 Load): what a loaded game does to the running scene. The data half (constructors,
// Updated, Save, clones, scenario lists) is WP-04's src/save/game.cpp.
#include "ksp/core/exceptions.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/modules/scenario_runner.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/ship/ship_construction.hpp"
#include "ksp/space_center/high_logic.hpp"
#include "ksp/space_center/scene_drivers.hpp"
#include "ksp/util/ksp_util.hpp"

namespace ksp {

using GE = events::GameEvents;

namespace {
// Game.cs:502-519 / :541-558: a mission game without Making History runs only when ExpansionsLoader.IsExpansionInstalled("MakingHistory",
// hashBytes, signature) accepts the mission's signature (ExpansionsLoader.cs:337: expansionsInfo has no MakingHistory entry, so the
// signature check of VerifyHashSignature decides)
bool MissionAllowed(const Game& game) {
    Game* current = HighLogic::CurrentGame();
    if (current == nullptr) throw NullReferenceException("HighLogic.CurrentGame is null");
    if ((current->Mode == Game::Modes::MISSION_BUILDER || current->Mode == Game::Modes::MISSION) &&
        !SaveEnvironment::Current().IsExpansionInstalled("MakingHistory")) {
        if (!(Game::missionSignatureVerified && Game::missionSignatureVerified(game))) return false;
    }
    return true;
}
}  // namespace

// Game.cs:500
void Game::Start() {
    if (!MissionAllowed(*this)) return;
    Status = GameStatus::ONGOING;
    ShipConstruction::ShipConfig = nullptr;
    if (!CrewRoster) throw NullReferenceException("Game.CrewRoster is null");
    CrewRoster->Init(*this);
    switch (startScene) {
        case GameScenes::FLIGHT: {
            if (!flightState) throw NullReferenceException("Game.flightState is null");
            // FlightDriver.StartAndFocusVessel(this, ...): the cache holds this game (shared when it is shared-owned, else referenced)
            std::shared_ptr<Game> self = weak_from_this().lock();
            if (!self) self = std::shared_ptr<Game>(this, [](Game*) {});
            FlightDriver::StartAndFocusVessel(self, flightState->activeVesselIdx);
            break;
        }
        default:
            HighLogic::LoadScene(startScene);
            break;
        case GameScenes::EDITOR:
            if (!Parameters) throw NullReferenceException("Game.Parameters is null");
            EditorDriver::StartupBehaviour = static_cast<EditorDriver::StartupBehaviours>(Parameters->Editor.startUpMode);
            EditorDriver::filePathToLoad = KSPUtil::ApplicationRootPath() + Parameters->Editor.craftFileToLoad;
            EditorDriver::StartEditor(editorFacility);
            break;
    }
}

// Game.cs:539
void Game::Load() {
    if (!MissionAllowed(*this)) return;
    // HighLogic.CurrentGame = this (the setter does nothing without a HighLogic instance); the owner keeps a shared-owned game alive
    HighLogic::CurrentGame(this);
    if (HighLogic::fetch != nullptr) HighLogic::currentGameOwner = weak_from_this().lock();
    if (ScenarioRunner::Instance() == nullptr) throw NullReferenceException("ScenarioRunner.Instance is null");  // static SetProtoModules
    ScenarioRunner::SetProtoModules(scenarios);
    if (!CrewRoster) throw NullReferenceException("Game.CrewRoster is null");
    CrewRoster->Init(*this);
    if (HighLogic::LoadedSceneHasPlanetarium() && flightState != nullptr) flightState->Load();
    if (config != nullptr) GE::onGameStateLoad.Fire(config.get());
}

}  // namespace ksp
