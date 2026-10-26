// WP-19c: the flows of the flight-registry close-out on the physprofile save and on the Kerbol system: VesselRetrieval (request -> persistent
// -> Space Center -> 8 frames -> recovery -> persistent), the FlightState round trip, switching to an unloaded vessel (a new flight scene),
// QuickSaveLoad's dialogs and quickLoad branches, Game.Start, and FlightGlobals' debug setters. Expected values are the C# statements'
// (path:line in each case) and analytic values; nothing is taken from kspcore output.
#include <set>

#include "doctest/doctest.h"
#include "wp19c_fixture.hpp"

#include "ksp/core/log.hpp"
#include "ksp/input/game_settings.hpp"
#include "ksp/input/input_lock_manager.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/physics/collision_enhancer.hpp"
#include "ksp/vessel/vessel.hpp"
#include "ksp/vessel/vessel_retrieval.hpp"

using namespace ksp;
using namespace wp19c;
using GE = events::GameEvents;
using Localizer = KSP::Localization::Localizer;

namespace {
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
int CountVessels(const std::string& sfs) {
    auto root = ConfigNode::Parse(sfs);
    const ConfigNode* fs = root->GetNode("GAME")->GetNode("FLIGHTSTATE");
    return fs == nullptr ? -1 : static_cast<int>(fs->GetNodes("VESSEL").size());
}
std::string GameValue(const std::string& sfs, const char* name) {
    auto root = ConfigNode::Parse(sfs);
    const std::string* v = root->GetNode("GAME")->GetValuePtr(name);
    return v == nullptr ? "" : *v;
}
// the physprofile flight scene of the quicksave case: KSC game current, the persistent save resumed with the L2-AS active
void StartPhysprofileFlight(World& w, std::shared_ptr<Game>& ksc) {
    w.CopySave(kSaves + "/physprofile/persistent.sfs", "persistent");
    w.WriterEnvironment(w.SaveText("persistent"));
    SaveEnvironment::Current().getUpdatedProtoModules = [] { return HighLogic::CurrentGame()->scenarios; };
    ksc.reset(GamePersistence::LoadGame("persistent", HighLogic::SaveFolder(), true, true).release());
    REQUIRE(ksc != nullptr);
    HighLogic::CurrentGame(ksc.get());
    HighLogic::currentGameOwner = ksc;
    FlightDriver::StartAndFocusVessel("persistent", 1);
    w.NewScene(GameScenes::FLIGHT);
    w.StartFlight();
    REQUIRE(FlightGlobals::Vessels.size() == 2);
}
}  // namespace

TEST_CASE("WP-19c VesselRetrieval: OnVesselRecoveryRequested -> persistent -> Space Center -> 8 frames -> onVesselRecovered(pv, false) -> persistent without the vessel") {
    World w;
    std::shared_ptr<Game> ksc;
    StartPhysprofileFlight(w, ksc);
    VesselRetrieval vr;
    Vessel* ast = FlightGlobals::Vessels[0];
    REQUIRE(ast->vesselName == "Ast. YRJ-552");
    const Guid id = ast->id;
    GE::OnVesselRecoveryRequested.Fire(ast);  // VesselRetrieval.cs:51
    REQUIRE(vr.IDsOfVesselsToRecover().size() == 1);
    CHECK(vr.IDsOfVesselsToRecover()[0] == id);
    CHECK(w.loadedScenes.back() == GameScenes::SPACECENTER);
    const std::string saved = w.SaveText("persistent");
    CHECK(CountVessels(saved) == 2);
    CHECK(GameValue(saved, "scene") == "5");  // SaveGame(.., OVERWRITE) = Updated(LOADING): CanLeaveToSpaceCenter -> SPACECENTER
    GE::OnVesselRecoveryRequested.Fire(ast);  // a second request of the same vessel: the HashSet keeps one id
    CHECK(vr.IDsOfVesselsToRecover().size() == 1);
    // the Space Center scene: the persistent game is loaded (Game.Load: FlightState.Load in a scene with a Planetarium)
    w.NewScene(GameScenes::SPACECENTER);
    std::shared_ptr<Game> kscGame(GamePersistence::LoadGame("persistent", HighLogic::SaveFolder(), true, true).release());
    REQUIRE(kscGame != nullptr);
    kscGame->Load();
    REQUIRE(FlightGlobals::Vessels.size() == 2);
    struct L {
        std::vector<std::pair<std::string, bool>> got;
        void On(ProtoVessel* pv, bool quick) { got.emplace_back(pv->vesselName, quick); }
    } l;
    GE::onVesselRecovered.Add(&l, &L::On);
    vr.OnSceneLoaded(static_cast<int>(GameScenes::SPACECENTER));  // SceneManager.sceneLoaded
    CoroutineScheduler& sched = HighLogic::Scheduler();
    for (int frame = 1; frame <= 8; frame++) {
        CHECK(l.got.empty());  // KSCFrameDelay: `yield return null` 8 times first
        sched.AdvanceFrame(0.02f);
        sched.RunDelayedDynamicFrameRate();
    }
    GE::onVesselRecovered.Remove(&l, &L::On);
    REQUIRE(l.got.size() == 1);
    CHECK(l.got[0].first == "Ast. YRJ-552");
    CHECK_FALSE(l.got[0].second);  // recoverVessel: quick = false
    REQUIRE(FlightGlobals::Vessels.size() == 1);  // DestroyImmediate(v.gameObject)
    CHECK(FlightGlobals::Vessels[0]->vesselName == "PhysProf-L2-AS");
    CHECK(vr.IDsOfVesselsToRecover().empty());
    const std::string after = w.SaveText("persistent");
    CHECK(CountVessels(after) == 1);  // the new FlightState() of Game.Updated no longer holds it
    CHECK(after.find("Ast. YRJ-552") == std::string::npos);
    // a scene other than the Space Center does not recover
    GE::OnVesselRecoveryRequested.Fire(FlightGlobals::Vessels[0]);
    vr.OnSceneLoaded(static_cast<int>(GameScenes::FLIGHT));
    for (int frame = 0; frame < 10; frame++) {
        sched.AdvanceFrame(0.02f);
        sched.RunDelayedDynamicFrameRate();
    }
    CHECK(FlightGlobals::Vessels.size() == 1);
    CHECK(vr.IDsOfVesselsToRecover().size() == 1);
}

TEST_CASE("WP-19c FlightState round trip: live capture -> Save -> FlightState(node) -> Save, Load -> live capture; MapViewFiltering's Serenity bit") {
    World w;
    std::shared_ptr<Game> ksc;
    StartPhysprofileFlight(w, ksc);
    Game* game = HighLogic::CurrentGame();
    FlightState a;
    CHECK(a.activeVesselIdx == 1);
    REQUIRE(a.protoVessels.size() == 2);
    CHECK(a.protoVessels[0].get() == FlightGlobals::Vessels[0]->protoVessel);  // unloaded: the snapshot the vessel was loaded from
    CHECK(a.protoVessels[1].get() == FlightGlobals::Vessels[1]->protoVessel);  // loaded: BackupVessel's new ProtoVessel(Vessel)
    CHECK(a.universalTime == Planetarium::GetUniversalTime());
    ConfigNode n1("FLIGHTSTATE");
    a.Save(n1);
    ConfigNode copy("FLIGHTSTATE");
    n1.CopyTo(copy);
    // a load starts with FlightGlobals.ClearpersistentIdDictionaries (GamePersistence.LoadGameCfg, GamePersistence.cs:229): the ids of the
    // live vessels would otherwise count as taken and be regenerated (FlightGlobals.CheckVesselpersistentId)
    SaveEnvironment::Current().persistentIds.ClearpersistentIdDictionaries();
    FlightState b(copy, game);
    ConfigNode n2("FLIGHTSTATE");
    b.Save(n2);
    if (n2.ToString() != n1.ToString()) {
        ConfigNode r1("root"), r2("root");
        n1.CopyTo(*r1.AddNode("GAME")->AddNode("FLIGHTSTATE"));
        n2.CopyTo(*r2.AddNode("GAME")->AddNode("FLIGHTSTATE"));
        for (const std::string& x : DiffLines(r1.ToString(), r2.ToString())) MESSAGE(x);
    }
    CHECK(n2.ToString() == n1.ToString());
    // FlightState.Load into a fresh scene: the vessels, the clock, the map filter
    w.NewScene(GameScenes::FLIGHT);
    w.planetarium.time = 1.0;
    b.Load();
    REQUIRE(FlightGlobals::Vessels.size() == 2);
    CHECK(Planetarium::GetUniversalTime() == b.universalTime);
    CHECK(FlightGlobals::Vessels[0]->protoVessel == b.protoVessels[0].get());
    CHECK(FlightGlobals::Vessels[1]->protoVessel == b.protoVessels[1].get());
    FlightGlobals::ActiveVessel = FlightGlobals::Vessels[static_cast<size_t>(b.activeVesselIdx)];
    FlightGlobals::ready = true;
    FlightState c;
    ConfigNode n3("FLIGHTSTATE");
    c.Save(n3);
    CHECK(n3.ToString() == n1.ToString());
    FlightGlobals::ready = false;
    // MapViewFiltering.LoadFilterState (MapViewFiltering.cs:138): the Serenity bit 0x4000 is dropped without Breaking Ground
    SaveEnvironment& env = SaveEnvironment::Current();
    FlightGlobals::Clear();
    Vessel::SpawnedVessels().clear();
    FlightState d;
    d.mapViewFilterState = 0x4000 | 0x12;
    env.SetStockExpansionInstalled("Serenity", false);
    d.Load();
    CHECK(env.mapViewFilteringVesselTypeFilter == 0x12);
    env.SetStockExpansionInstalled("Serenity", true);
    d.Load();
    CHECK(env.mapViewFilteringVesselTypeFilter == (0x4000 | 0x12));
    d.compatible = false;  // an incompatible flight state loads nothing
    env.mapViewFilteringVesselTypeFilter = 7;
    d.Load();
    CHECK(env.mapViewFilteringVesselTypeFilter == 7);
}

TEST_CASE("WP-19c switching to an unloaded vessel: persistent (FLIGHT), FlightDriver.StartAndFocusVessel(HighLogic.CurrentGame.Updated(), index), the new flight scene") {
    World w;
    std::shared_ptr<Game> ksc;
    StartPhysprofileFlight(w, ksc);
    Vessel* active = FlightGlobals::ActiveVessel;
    Vessel* ast = FlightGlobals::Vessels[0];
    REQUIRE_FALSE(ast->loaded);
    // not forced: the asteroid is not Owned (FlightGlobals.cs:1010, #autoLOC_6002352)
    std::vector<std::string> msgs;
    FlightGlobals::env.postScreenMessage = [&](const std::string& m) { msgs.push_back(m); };
    CHECK_FALSE(FlightGlobals::SetActiveVessel(ast));
    REQUIRE(msgs.size() == 1);
    CHECK(msgs[0] == Localizer::Format("#autoLOC_6002352"));
    CHECK(FlightGlobals::ActiveVessel == active);
    int switching = 0;
    struct L {
        int* n;
        Vessel* from = nullptr;
        Vessel* to = nullptr;
        void On(Vessel* f, Vessel* t) {
            ++*n;
            from = f;
            to = t;
        }
    } l{&switching};
    GE::onVesselSwitchingToUnloaded.Add(&l, &L::On);
    const size_t scenes = w.loadedScenes.size();
    CHECK(FlightGlobals::ForceSetActiveVessel(ast));
    GE::onVesselSwitchingToUnloaded.Remove(&l, &L::On);
    CHECK(switching == 1);
    CHECK((l.from == active && l.to == ast));
    CHECK(FlightGlobals::ActiveVessel == active);  // the switch happens in the next scene
    // SaveGame(.., FLIGHT) = CurrentGame.Updated(FLIGHT): the override applies to mission games only; CanLeaveToSpaceCenter gives SPACECENTER
    CHECK(GameValue(w.SaveText("persistent"), "scene") == "5");
    REQUIRE(w.loadedScenes.size() == scenes + 1);
    CHECK(w.loadedScenes.back() == GameScenes::FLIGHT);
    CHECK(FlightDriver::StartupBehaviour == FlightDriver::StartupBehaviours::RESUME_SAVED_CACHE);
    CHECK(FlightDriver::FlightStateCache.get() == HighLogic::CurrentGame());
    CHECK(FlightDriver::FocusVesselAfterLoad == 0);
    // the new flight scene focuses the asteroid
    w.NewScene(GameScenes::FLIGHT);
    w.StartFlight();
    REQUIRE(FlightGlobals::ActiveVessel != nullptr);
    CHECK(FlightGlobals::ActiveVessel->vesselName == "Ast. YRJ-552");
    CHECK(FlightGlobals::ActiveVessel->loaded);
    CHECK_FALSE(FlightGlobals::Vessels[1]->loaded);
}

TEST_CASE("WP-19c QuickSaveLoad save-as dialog: naming, escaping, overwrite confirmation, invalid names, Escape, Return with blockedSubmitOnStart") {
    World w;
    std::shared_ptr<Game> ksc;
    StartPhysprofileFlight(w, ksc);
    QuickSaveLoad qsl;
    std::vector<std::string> ev;
    QuickSaveLoad::env.spawnSaveDialog = [&](const std::string& n) { ev.push_back("spawn:" + n); };
    QuickSaveLoad::env.dismissSaveDialog = [&] { ev.push_back("dismiss"); };
    QuickSaveLoad::env.setSaveDialogActive = [&](bool a) { ev.push_back(a ? "active" : "inactive"); };
    QuickSaveLoad::env.spawnConfirmDialog = [&](const std::string& m) { ev.push_back("confirm:" + m); };
    QuickSaveLoad::env.dismissConfirmDialog = [&] { ev.push_back("dismissConfirm"); };
    int saves = 0;
    struct L {
        int* n;
        void On() { ++*n; }
    } l{&saves};
    GE::onGameAboutToQuicksave.Add(&l, &L::On);
    // QuickSaveLoad.cs:346-351: "quicksave", then "quicksave #1" while the file exists
    std::ofstream(w.folder / "quicksave.sfs") << "x";
    qsl.SpawnSaveAsDialog();
    CHECK(qsl.quicksaveFilename == Localizer::Format("#autoLOC_6002266") + " #1");
    CHECK(FlightDriver::Pause());
    CHECK(InputLockManager::IsLocked(ControlTypes::QUICKLOAD));  // SetControlLock("SaveAsDialog"): every control type
    REQUIRE(ev.size() == 1);
    CHECK(ev[0] == "spawn:" + qsl.quicksaveFilename);
    // the text field: EscapeString (CR / LF -> ' ', '"' -> '\'', KSPUtil.SanitizeFilename)
    CHECK(qsl.OnSaveAsTextInput("a\"b\nc") == "a'b c");
    // an existing file: OverwriteSave, then "Overwrite" saves under that name (doSave(quicksaveFilename))
    qsl.OnSaveAsTextInput("quicksave");
    std::string closedWith = "-";
    qsl.ConfirmDialog(true, [&](const std::string& n) { closedWith = n; });
    CHECK(ev[1] == "inactive");
    CHECK(ev[2] == "confirm:" + Localizer::Format("#autoLOC_417256", {std::string("quicksave")}));
    CHECK(saves == 0);
    qsl.ConfirmOverwrite();
    CHECK(saves == 1);
    CHECK(closedWith == "quicksave");
    CHECK_FALSE(FlightDriver::Pause());
    CHECK_FALSE(InputLockManager::IsLocked(ControlTypes::QUICKLOAD));
    CHECK_FALSE(qsl.SaveDialogOpen());
    CHECK(w.SaveText("quicksave").find("PhysProf-L2-AS") != std::string::npos);
    // an invalid name: InvalidSaveName, its button shows the save dialog again; Escape then closes it without saving
    ev.clear();
    qsl.SpawnSaveAsDialog();
    qsl.quicksaveFilename = "bad/name";
    qsl.ConfirmDialog();
    REQUIRE(ev.size() == 2);
    CHECK(ev[1] == "confirm:" + Localizer::Format("#autoLOC_417273"));
    qsl.InvalidNameOk();
    CHECK(ev.back() == "active");
    struct Keys final : IInputDevice {
        std::set<KeyCode> up, held;
        bool GetKey(KeyCode k) const override { return held.count(k) != 0; }
        bool GetKeyDown(KeyCode) const override { return false; }
        bool GetKeyUp(KeyCode k) const override { return up.count(k) != 0; }
        float GetAxis(const std::string&) const override { return 0.0f; }
        bool GetMouseButton(int) const override { return false; }
        bool GetMouseButtonDown(int) const override { return false; }
        bool GetMouseButtonUp(int) const override { return false; }
        Vector3f mousePosition() const override { return {}; }
        std::u16string inputString() const override { return {}; }
        std::vector<std::string> GetJoystickNames() const override { return {}; }
    } keys;
    InputEnvironment::device = &keys;
    keys.up.insert(KeyCode::Escape);
    qsl.Update();
    keys.up.clear();
    CHECK_FALSE(qsl.SaveDialogOpen());
    CHECK(saves == 1);
    // Return while the dialog opened with Return held: the first Return is swallowed (blockedSubmitOnStart), the second confirms
    keys.held.insert(KeyCode::Return);
    qsl.SpawnSaveAsDialog();
    keys.held.clear();
    qsl.OnSaveAsTextInput("named save");
    keys.up.insert(KeyCode::Return);
    qsl.Update();
    CHECK(qsl.SaveDialogOpen());
    CHECK(saves == 1);
    qsl.Update();
    CHECK_FALSE(qsl.SaveDialogOpen());
    CHECK(saves == 2);
    CHECK(fs::exists(w.folder / "named save.sfs"));
    InputEnvironment::device = nullptr;
    GE::onGameAboutToQuicksave.Remove(&l, &L::On);
}

TEST_CASE("WP-19c QuickSaveLoad.quickLoad branches: no file, a Space Center save, an incompatible save, a pre-0.24 save, hold-to-load, the load dialog, a mission quicksave") {
    World w;
    std::shared_ptr<Game> ksc;
    StartPhysprofileFlight(w, ksc);
    QuickSaveLoad qsl;
    std::vector<std::string> msgs;
    QuickSaveLoad::env.postScreenMessage = [&](const std::string& m, float) { msgs.push_back(m); };
    // no file (QuickSaveLoad.cs:258-264)
    qsl.quickLoad("nothing", HighLogic::SaveFolder());
    REQUIRE(msgs.size() == 1);
    CHECK(msgs[0] == Localizer::Format("#autoLOC_174686", {std::string("#autoLOC_6002266")}));
    // the physprofile file itself (scene = SPACECENTER, 1.12.5): persistent is saved and the Space Center loaded (:315-319)
    w.CopySave(kSaves + "/physprofile/persistent.sfs", "kscsave");
    std::error_code ec;
    fs::remove(w.folder / "persistent.sfs", ec);
    size_t scenes = w.loadedScenes.size();
    qsl.quickLoad("kscsave", HighLogic::SaveFolder());
    REQUIRE(w.loadedScenes.size() == scenes + 1);
    CHECK(w.loadedScenes.back() == GameScenes::SPACECENTER);
    CHECK(fs::exists(w.folder / "persistent.sfs"));
    // a 0.20.0 file: every upgrade script is TooEarly, the pipeline fails; Cancel (the default answer) unpauses (:299-301), LoadAnyway reaches
    // LoadGameCfg, which refuses the incompatible game (0.20 < 0.21): unpause again (:327)
    std::string old = ReadText(fs::path(kSaves) / "physprofile" / "persistent.sfs");
    const std::string ver = "\tversion = 1.12.5";
    old.replace(old.find(ver), ver.size(), "\tversion = 0.20.0");
    std::ofstream(w.folder / "old020.sfs", std::ios::binary) << old;
    FlightDriver::pause = true;
    scenes = w.loadedScenes.size();
    qsl.quickLoad("old020", HighLogic::SaveFolder());
    CHECK_FALSE(FlightDriver::Pause());
    CHECK(w.loadedScenes.size() == scenes);
    QuickSaveLoad::env.upgradeFailDialog = [](const std::string&) { return KSPUpgradePipeline::UpgradeFailOption::LoadAnyway; };
    FlightDriver::pause = true;
    qsl.quickLoad("old020", HighLogic::SaveFolder());
    CHECK_FALSE(FlightDriver::Pause());
    CHECK(w.loadedScenes.size() == scenes);
    // a 0.23.5 file (compatible, older than 0.24): the flight scene even though its start scene is the Space Center (:315 / :322)
    std::string v0235 = ReadText(fs::path(kSaves) / "physprofile" / "persistent.sfs");
    v0235.replace(v0235.find(ver), ver.size(), "\tversion = 0.23.5");
    std::ofstream(w.folder / "old0235.sfs", std::ios::binary) << v0235;
    qsl.quickLoad("old0235", HighLogic::SaveFolder());
    REQUIRE(w.loadedScenes.size() == scenes + 1);
    CHECK(w.loadedScenes.back() == GameScenes::FLIGHT);
    CHECK(FlightDriver::StartupBehaviour == FlightDriver::StartupBehaviours::RESUME_SAVED_CACHE);
    CHECK(FlightDriver::FocusVesselAfterLoad == 1);
    QuickSaveLoad::env.upgradeFailDialog = nullptr;
    // hold-to-load: Invoke("onHoldComplete", holdTime) at Time.time + 1 s
    msgs.clear();
    qsl.holdToLoad = true;
    struct F9 final : IInputDevice {
        bool down = true;
        bool GetKey(KeyCode) const override { return false; }
        bool GetKeyDown(KeyCode k) const override { return down && k == KeyCode::F9; }
        bool GetKeyUp(KeyCode) const override { return false; }
        float GetAxis(const std::string&) const override { return 0.0f; }
        bool GetMouseButton(int) const override { return false; }
        bool GetMouseButtonDown(int) const override { return false; }
        bool GetMouseButtonUp(int) const override { return false; }
        Vector3f mousePosition() const override { return {}; }
        std::u16string inputString() const override { return {}; }
        std::vector<std::string> GetJoystickNames() const override { return {}; }
    } f9;
    InputEnvironment::device = &f9;
    Time::time = 10.0f;
    qsl.Update();
    f9.down = false;
    REQUIRE(msgs.size() == 1);
    CHECK(msgs[0] == Localizer::Format("#autoLOC_174589", {GameSettings::QUICKLOAD->name()}));
    CHECK(qsl.IsInvokingHoldComplete());
    fs::remove(w.folder / "quicksave.sfs", ec);
    qsl.RunDelayedCalls(10.5f);
    CHECK(msgs.size() == 1);
    qsl.RunDelayedCalls(11.0f);
    CHECK_FALSE(qsl.IsInvokingHoldComplete());
    REQUIRE(msgs.size() == 2);  // onHoldComplete -> quickLoad("quicksave"): no file
    CHECK(msgs[1] == Localizer::Format("#autoLOC_174686", {std::string("#autoLOC_6002266")}));
    InputEnvironment::device = nullptr;
    // the load dialog: OnLoadClose(path) loads the file without its extension, unpauses, calls back
    std::string loadFolder;
    QuickSaveLoad::env.spawnLoadBrowser = [&](const std::string& f) { loadFolder = f; };
    std::string back = "-";
    qsl.SpawnQuickLoadDialog(true, [&](const std::string& n) { back = n; });
    CHECK(loadFolder == HighLogic::SaveFolder());
    CHECK(FlightDriver::Pause());
    scenes = w.loadedScenes.size();
    qsl.OnLoadClose("C:/somewhere/kscsave.sfs");
    CHECK(w.loadedScenes.size() == scenes + 1);
    CHECK(back == "C:/somewhere/kscsave.sfs");
    CHECK_FALSE(FlightDriver::Pause());
    // a mission quicksave of a mission game (:265-277): LoadGameCfg becomes HighLogic.CurrentGame, InitMission, persistent, Game.Start
    std::string mission = ReadText(fs::path(kSaves) / "physprofile" / "persistent.sfs");
    mission.replace(mission.find("\tMode = SANDBOX"), std::string("\tMode = SANDBOX").size(), "\tMode = MISSION");
    const size_t at = mission.find("\tPARAMETERS");
    mission.insert(at, "\tMISSIONTOSTART\n\t{\n\t\tname = test\n\t}\n");
    std::ofstream(w.folder / "mission.sfs", std::ios::binary) << mission;
    SaveEnvironment::Current().SetStockExpansionInstalled("MakingHistory", true);
    HighLogic::CurrentGame()->Mode = Game::Modes::MISSION;
    int inits = 0;
    QuickSaveLoad::env.initMission = [&](Game& g) {
        ++inits;
        CHECK(&g == HighLogic::CurrentGame());
    };
    fs::remove(w.folder / "persistent.sfs", ec);
    scenes = w.loadedScenes.size();
    Game* before = HighLogic::CurrentGame();
    qsl.quickLoad("mission", HighLogic::SaveFolder());
    CHECK(inits == 1);
    CHECK(HighLogic::CurrentGame() != before);
    CHECK(HighLogic::CurrentGame()->Mode == Game::Modes::MISSION);
    CHECK(HighLogic::CurrentGame()->Status == Game::GameStatus::ONGOING);
    CHECK(fs::exists(w.folder / "persistent.sfs"));
    REQUIRE(w.loadedScenes.size() == scenes + 1);
    CHECK(w.loadedScenes.back() == GameScenes::SPACECENTER);  // Game.Start: HighLogic.LoadScene(startScene)
    SaveEnvironment::Current().SetStockExpansionInstalled("MakingHistory", false);
}

TEST_CASE("WP-19c Game.Start: FLIGHT -> FlightDriver.StartAndFocusVessel(this), EDITOR -> EditorDriver, other scenes -> HighLogic.LoadScene; mission games without Making History") {
    World w;
    w.CopySave(kSaves + "/physprofile/persistent.sfs", "persistent");
    std::shared_ptr<Game> g(GamePersistence::LoadGame("persistent", HighLogic::SaveFolder(), true, true).release());
    REQUIRE(g != nullptr);
    HighLogic::CurrentGame(g.get());
    g->Status = Game::GameStatus::UNSTARTED;
    g->startScene = GameScenes::FLIGHT;
    g->Start();
    CHECK(g->Status == Game::GameStatus::ONGOING);
    CHECK(FlightDriver::FlightStateCache == g);  // shared: the cache keeps the game
    CHECK(FlightDriver::FocusVesselAfterLoad == 1);
    CHECK(w.loadedScenes.back() == GameScenes::FLIGHT);
    g->startScene = GameScenes::EDITOR;
    g->editorFacility = EditorFacility::SPH;
    g->Parameters->Editor.startUpMode = 2;
    g->Parameters->Editor.craftFileToLoad = "Ships/SPH/x.craft";
    g->Start();
    CHECK(EditorDriver::StartupBehaviour == EditorDriver::StartupBehaviours::LOAD_FROM_FILE);
    CHECK(EditorDriver::filePathToLoad == KSPUtil::ApplicationRootPath() + "Ships/SPH/x.craft");
    CHECK(EditorDriver::editorFacility == EditorFacility::SPH);
    CHECK(w.loadedScenes.back() == GameScenes::EDITOR);
    g->startScene = GameScenes::TRACKSTATION;
    g->Start();
    CHECK(w.loadedScenes.back() == GameScenes::TRACKSTATION);
    // a mission game without Making History: the signature check fails (no verifier), nothing happens (Game.cs:502-518)
    g->Mode = Game::Modes::MISSION;
    g->Status = Game::GameStatus::UNSTARTED;
    const size_t scenes = w.loadedScenes.size();
    g->Start();
    CHECK(g->Status == Game::GameStatus::UNSTARTED);
    CHECK(w.loadedScenes.size() == scenes);
    Game::missionSignatureVerified = [](const Game&) { return true; };
    g->Start();
    CHECK(g->Status == Game::GameStatus::ONGOING);
    Game::missionSignatureVerified = nullptr;
    EditorDriver::ResetStatics();
}

TEST_CASE("WP-19c FlightGlobals debug setters: SetShipOrbit, SetShipOrbitRendezvous, SetVesselPosition (clamps, surface state vector, rotation, GoOffRails one frame later or on unpause)") {
    flighttest::FlightEnv w;
    HighLogic hl;
    hl.Awake();
    CelestialBody* kerbin = w.Body("Kerbin");
    CelestialBody* mun = w.Body("Mun");
    Vessel* v = w.MakeVessel("active", kerbin, 700000.0, 0.0, true, false, OrbitDriver::UpdateMode::TRACK_Phys);
    Vessel* other = w.MakeVessel("other", kerbin, 800000.0, 0.0, true, false, OrbitDriver::UpdateMode::TRACK_Phys);
    v->situation = Vessel::Situations::LANDED;
    v->Landed = true;
    v->landedAt = "KSC";
    FlightGlobals::ActiveVessel = v;
    FlightGlobals::Start();
    const double ut = Planetarium::GetUniversalTime();
    int soi = 0;
    struct L {
        int* n;
        GE::HostedFromToAction<Vessel*, CelestialBody*> last;
        void On(GE::HostedFromToAction<Vessel*, CelestialBody*> a) {
            ++*n;
            last = a;
        }
    } l{&soi};
    GE::onVesselSOIChanged.Add(&l, &L::On);
    // SetShipOrbit (FlightGlobals.cs:1090): sma capped at 0.99 SOI, SetOrbit at the current UT, every unpacked vessel on rails
    const int munIndex = FlightGlobals::GetBodyIndex(mun);
    physics::CollisionEnhancer::bypass = false;
    Time::time = 100.0f;
    FlightGlobals::SetShipOrbit(munIndex, 0.1, 1.0e12, 5.0, 30.0, 0.5, 45.0, 0.0);
    const Orbit& o = v->orbit();
    CHECK(o.referenceBody == mun);
    CHECK(o.semiMajorAxis == mun->sphereOfInfluence * 0.99);
    CHECK(o.eccentricity == 0.1);
    CHECK(o.inclination == 5.0);
    CHECK(o.LAN == 30.0);
    CHECK(o.argumentOfPeriapsis == 45.0);
    CHECK(o.meanAnomalyAtEpoch == 0.5);
    CHECK(o.epoch == ut);
    CHECK(FlightGlobals::currentMainBody == mun);
    CHECK_FALSE(v->Landed);
    CHECK(v->landedAt.empty());
    CHECK(v->packed);
    CHECK(other->packed);
    CHECK(physics::CollisionEnhancer::bypass);
    CHECK(FlightGlobals::overrideOrbit);
    REQUIRE(soi == 1);
    CHECK((l.last.host == v && l.last.from == kerbin && l.last.to == mun));
    CHECK(v->IgnoreSpeedActive());
    FlightGlobals::RunDelayedCalls(101.9f);  // Start's 1 s Invoke has run, PrepForOrbitSet's 2 s one is pending
    CHECK_FALSE(FlightGlobals::overrideOrbit);
    FlightGlobals::overrideOrbit = true;
    FlightGlobals::RunDelayedCalls(102.0f);
    CHECK_FALSE(FlightGlobals::overrideOrbit);
    CHECK_FALSE(physics::CollisionEnhancer::bypass);
    // SetShipOrbitRendezvous (:1104): at rel = 0 the target's state; rel.y along the target's prograde direction
    Vessel* target = w.MakeVessel("target", kerbin, 900000.0, 0.05);
    FlightGlobals::SetShipOrbitRendezvous(target, Vector3d(), Vector3d());
    CHECK(v->orbit().referenceBody == kerbin);
    const Orbit* to = target->GetCurrentOrbit();
    CHECK((v->orbit().getRelativePositionAtUT(ut) - to->getRelativePositionAtUT(ut)).magnitude() < 1e-6);
    CHECK((v->orbit().getOrbitalVelocityAtUT(ut) - to->getOrbitalVelocityAtUT(ut)).magnitude() < 1e-9);
    FlightGlobals::SetShipOrbitRendezvous(target, Vector3d(0.0, 100.0, 0.0), Vector3d(0.0, 0.0, 0.0));
    const Vector3d d = v->orbit().getRelativePositionAtUT(ut) - to->getRelativePositionAtUT(ut);
    CHECK((d - to->vel.normalized() * 100.0).magnitude() < 1e-6);
    // SetVesselPosition (:1166): clamped latitude / longitude, |altitude| capped at DEBUG_MAX_SETPOSITION_ALTITUDE, no PQS: no terrain height,
    // the co-rotating surface state vector
    const double maxAlt0 = GameSettings::DEBUG_MAX_SETPOSITION_ALTITUDE;
    GameSettings::DEBUG_MAX_SETPOSITION_ALTITUDE = 1000.0;
    const int kerbinIndex = FlightGlobals::GetBodyIndex(kerbin);
    FlightDriver::pause = false;
    FlightGlobals::SetVesselPosition(kerbinIndex, 95.0, -200.0, -5000.0, 10.0, 90.0, true);
    const Vector3d rel = kerbin->GetWorldSurfacePosition(89.999, -180.0, 1000.0) - kerbin->position;
    const Vector3d vel = Vector3d::Cross(kerbin->angularVelocity, rel);
    // the vessel's orbit is UpdateFromStateVectors of exactly that state vector (every element bit for bit; the near-polar co-rotating
    // state is an almost radial orbit whose elements do not reproduce the position to the metre, which is the Orbit representation's, not this)
    Orbit expect;
    expect.UpdateFromStateVectors(rel.xzy(), vel.xzy(), kerbin, ut);
    const Orbit& got = v->orbit();
    CHECK(got.referenceBody == kerbin);
    CHECK(flighttest::SameBits(got.semiMajorAxis, expect.semiMajorAxis));
    CHECK(flighttest::SameBits(got.eccentricity, expect.eccentricity));
    CHECK(flighttest::SameBits(got.inclination, expect.inclination));
    CHECK(flighttest::SameBits(got.LAN, expect.LAN));
    CHECK(flighttest::SameBits(got.argumentOfPeriapsis, expect.argumentOfPeriapsis));
    CHECK(flighttest::SameBits(got.meanAnomalyAtEpoch, expect.meanAnomalyAtEpoch));
    CHECK(flighttest::SameBits(got.epoch, expect.epoch));
    CHECK_FALSE(FlightGlobals::setPositionInProgress);
    GameSettings::DEBUG_MAX_SETPOSITION_ALTITUDE = maxAlt0;
    // paused: the GoOffRails waits for onGameUnpause (:1219-1225, :1240)
    FlightDriver::pause = true;
    FlightGlobals::SetVesselPosition(kerbinIndex, 0.0, 0.0, 500.0, 0.0, 0.0, false);
    CHECK(FlightGlobals::setPositionInProgress);
    CHECK(FlightGlobals::setPositionInProgressVesselID == v->persistentId);
    FlightDriver::pause = false;
    GE::onGameUnpause.Fire();
    CHECK_FALSE(FlightGlobals::setPositionInProgress);
    GE::onVesselSOIChanged.Remove(&l, &L::On);
    FlightGlobals::Clear();
    hl.OnDestroy();
    HighLogic::ResetStatics();
    FlightDriver::ResetStatics();
}
