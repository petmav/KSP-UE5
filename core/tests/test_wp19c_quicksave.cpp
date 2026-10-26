// WP-19c: QuickSaveLoad, FlightState.Load / the live FlightState(), Game.Load and FlightDriver.Start over the physprofile save (the only
// unmodded original save in testdata): save -> load -> save byte-identical through the real flight scene (FlightDriver.Start, FlightState.Load,
// FlightGlobals.SetActiveVessel loading the active vessel's parts, BackupVessel, QuickSaveLoad.doSave / quickLoad), as WP-04's corpus did for
// the data model. Expected values come from the C# statements (QuickSaveLoad.cs, FlightState.cs, Game.cs, FlightDriver.cs) and the original save.
#include <set>

#include "doctest/doctest.h"
#include "wp19c_fixture.hpp"

#include "ksp/input/game_settings.hpp"
#include "ksp/input/input_lock_manager.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/vessel/vessel.hpp"

using namespace ksp;
using namespace wp19c;

namespace {
// the values of every node, as "path/name = value" lines (a structural diff that names what changed)
void Flatten(const ConfigNode& n, const std::string& path, std::vector<std::string>& out) {
    for (int i = 0; i < n.values().Count(); i++) out.push_back(path + "/" + n.values()[i].name + " = " + n.values()[i].value);
    std::map<std::string, int> seen;
    for (int i = 0; i < n.nodes().Count(); i++) {
        const ConfigNode& c = *n.nodes()[i];
        Flatten(c, path + "/" + c.name + "[" + std::to_string(seen[c.name]++) + "]", out);
    }
}
std::vector<std::string> DiffLines(const std::string& a, const std::string& b) {
    auto na = ConfigNode::Parse(a);
    auto nb = ConfigNode::Parse(b);
    std::vector<std::string> la, lb, d;
    Flatten(*na, "", la);
    Flatten(*nb, "", lb);
    std::multiset<std::string> sa(la.begin(), la.end()), sb(lb.begin(), lb.end());
    for (const auto& x : la)
        if (sb.count(x) == 0) d.push_back("- " + x);
    for (const auto& x : lb)
        if (sa.count(x) == 0) d.push_back("+ " + x);
    return d;
}
}  // namespace

TEST_CASE("WP-19c quicksave -> quickload -> quicksave of the physprofile save through the flight scene is byte-identical") {
    World w;
    w.CopySave(kSaves + "/physprofile/persistent.sfs", "persistent");
    const std::string original = w.SaveText("persistent");
    w.WriterEnvironment(original);
    // the Space Center runs the loaded game (HighLogic.CurrentGame: Game.Load reads its Mode), then asks for the flight scene of the saved
    // game: FlightDriver.Start with RESUME_SAVED_FILE (SpaceCenter's StartAndFocusVessel("persistent", index))
    std::shared_ptr<Game> ksc(GamePersistence::LoadGame("persistent", HighLogic::SaveFolder(), true, true));
    REQUIRE(ksc != nullptr);
    HighLogic::CurrentGame(ksc.get());
    HighLogic::currentGameOwner = ksc;
    FlightDriver::StartAndFocusVessel("persistent", 1);
    REQUIRE(w.loadedScenes.back() == GameScenes::FLIGHT);
    // the scenario modules' own Save is their packages' (several are partial ports): the runner hands back the loaded snapshots
    SaveEnvironment::Current().getUpdatedProtoModules = [] { return HighLogic::CurrentGame()->scenarios; };
    // FlightCamera.CamHdg / CamPitch / CamMode: a stand-in camera (values unlike the file's 0.3 / 0.2 / 0)
    Vessel::env.flightCamera = [](float& h, float& p, int& m) {
        h = 0.75f;
        p = -0.25f;
        m = 1;
    };
    w.NewScene(GameScenes::FLIGHT);
    w.StartFlight();
    w.Frames(3);
    Game* game = HighLogic::CurrentGame();
    REQUIRE(game != nullptr);
    REQUIRE(HighLogic::currentGameOwner.get() == game);  // Game.Load: HighLogic.CurrentGame = this (the loaded game, kept alive)
    REQUIRE(FlightGlobals::Vessels.size() == 2);         // FlightState.Load: both proto vessels became Vessels
    REQUIRE(FlightGlobals::ActiveVessel == FlightGlobals::Vessels[1]);
    CHECK(FlightGlobals::ActiveVessel->vesselName == "PhysProf-L2-AS");
    CHECK(FlightGlobals::ActiveVessel->loaded);  // SetActiveVessel -> MakeActive -> Load (the parts from the snapshots)
    CHECK(FlightGlobals::ActiveVessel->parts.size() == game->flightState->protoVessels[1]->protoPartSnapshots.size());
    CHECK_FALSE(FlightGlobals::Vessels[0]->loaded);
    CHECK(FlightGlobals::Vessels[0]->protoVessel == game->flightState->protoVessels[0].get());  // the shared snapshot (Vessel.StartFromBackup)
    CHECK(FlightGlobals::ready);
    CHECK(Planetarium::GetUniversalTime() == game->flightState->universalTime);  // FlightDriver.Start + FlightState.Load set the clock

    // F5: QuickSaveLoad.Update with the QUICKSAVE key down
    QuickSaveLoad qsl;
    REQUIRE(qsl.Alive());
    std::vector<std::pair<std::string, float>> messages;
    QuickSaveLoad::env.postScreenMessage = [&](const std::string& m, float d) { messages.emplace_back(m, d); };
    int aboutToQuicksave = 0;
    struct L {
        int* n;
        void On() { ++*n; }
    } l{&aboutToQuicksave};
    events::GameEvents::onGameAboutToQuicksave.Add(&l, &L::On);
    qsl.quickSave(false);
    events::GameEvents::onGameAboutToQuicksave.Remove(&l, &L::On);
    CHECK(aboutToQuicksave == 1);
    REQUIRE(fs::exists(w.folder / "quicksave.sfs"));
    REQUIRE(messages.size() == 1);
    CHECK(messages[0].first == KSP::Localization::Localizer::Format("#autoLOC_174669"));
    CHECK(messages[0].second == 2.0f);
    CHECK(game->startScene == GameScenes::SPACECENTER);  // doSave: FLIGHT for the quicksave, SPACECENTER afterwards
    const std::string q1 = w.SaveText("quicksave");
    CHECK(w.SaveText("persistent") == original);  // AutoSaveOnQuickSave = false: persistent untouched

    // what the first quicksave changed against the original file: exactly these lines, each with its cause (anything else fails)
    const std::set<std::string> expected = {
        // Game.startScene = FLIGHT for the quicksave (QuickSaveLoad.cs:240; the file was written from the Space Center)
        "- /GAME[0]/scene = 5", "+ /GAME[0]/scene = 7",
        // Vessel.OnSaveFlightState of the active vessel takes FlightCamera's angles and mode (Vessel.cs:3617): the stand-in camera's values
        "- /GAME[0]/FLIGHTSTATE[0]/VESSEL[1]/cPch = 0.2", "- /GAME[0]/FLIGHTSTATE[0]/VESSEL[1]/cHdg = 0.3", "- /GAME[0]/FLIGHTSTATE[0]/VESSEL[1]/cMod = 0",
        "+ /GAME[0]/FLIGHTSTATE[0]/VESSEL[1]/cPch = -0.25", "+ /GAME[0]/FLIGHTSTATE[0]/VESSEL[1]/cHdg = 0.75", "+ /GAME[0]/FLIGHTSTATE[0]/VESSEL[1]/cMod = 1",
        // live state that the physics tick of the bubble (not run here: no rigid bodies) produces: ModuleReactionWheel's FixedUpdate status text
        "- /GAME[0]/FLIGHTSTATE[0]/VESSEL[1]/PART[0]/MODULE[2]/stateString = Idle", "+ /GAME[0]/FLIGHTSTATE[0]/VESSEL[1]/PART[0]/MODULE[2]/stateString = Active",
        // the asteroid's CommNetVessel (a SpaceObject: no connection) writes its two persistent fields in kspcore's CommNetVessel save (WP-39);
        // the file has none (the CommNet network decides them in the game)
        "+ /GAME[0]/FLIGHTSTATE[0]/VESSEL[0]/VESSELMODULES[0]/CommNetVessel[0]/controlState = None",
        "+ /GAME[0]/FLIGHTSTATE[0]/VESSEL[0]/VESSELMODULES[0]/CommNetVessel[0]/canComm = True"};
    const std::vector<std::string> d = DiffLines(original, q1);
    for (const std::string& x : d)
        if (expected.count(x) == 0) MESSAGE("unexplained: " << x);
    for (const std::string& x : expected)
        if (std::find(d.begin(), d.end(), x) == d.end()) MESSAGE("expected but absent: " << x);
    CHECK(std::set<std::string>(d.begin(), d.end()) == expected);
    CHECK(d.size() == expected.size());

    // F9: quickLoad("quicksave") -> the upgrade pipeline -> onQuickloadPipelineFinished -> FlightDriver.StartAndFocusVessel(game, activeVesselIdx)
    const size_t scenesBefore = w.loadedScenes.size();
    qsl.quickLoad("quicksave", HighLogic::SaveFolder());
    REQUIRE(w.loadedScenes.size() == scenesBefore + 1);
    CHECK(w.loadedScenes.back() == GameScenes::FLIGHT);
    CHECK(FlightDriver::StartupBehaviour == FlightDriver::StartupBehaviours::RESUME_SAVED_CACHE);
    REQUIRE(FlightDriver::FlightStateCache != nullptr);
    CHECK(FlightDriver::FocusVesselAfterLoad == 1);
    CHECK(FlightDriver::FlightStateCache->startScene == GameScenes::FLIGHT);
    // the new flight scene
    w.NewScene(GameScenes::FLIGHT);
    w.StartFlight();
    w.Frames(3);
    REQUIRE(HighLogic::CurrentGame() == FlightDriver::FlightStateCache.get());
    REQUIRE(FlightGlobals::Vessels.size() == 2);
    REQUIRE(FlightGlobals::ActiveVessel == FlightGlobals::Vessels[1]);
    CHECK(FlightDriver::resumeCacheUsed);
    QuickSaveLoad::env.postScreenMessage = nullptr;
    qsl.quickSave(false);
    const std::string q2 = w.SaveText("quicksave");
    for (const std::string& x : DiffLines(q1, q2)) MESSAGE(x);
    CHECK(q2 == q1);
    // SaveMode.BACKUP: the first quicksave moved to saves/wp19c/Backup/quicksave (<date>).sfs
    int backups = 0;
    for (const auto& e : fs::directory_iterator(w.folder / "Backup")) backups += e.path().filename().string().rfind("quicksave (", 0) == 0 ? 1 : 0;
    CHECK(backups == 1);
}
