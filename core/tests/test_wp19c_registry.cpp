// WP-19c: the rules of FlightState(), ProtoVessel.Clean / GetSituationString, ProtoPartSnapshot.LoadEVAInfoBySuitGender and QuickSaveLoad
// (QuickSaveClearToSave, Update) against tools/wp19c/registry_reference.py (tests/data/wp19c_reference.json), an independent Python reading of
// the decompiled C# (no kspcore code; the English texts come from the game's dictionary.cfg). Every comparison is exact (bits / strings).
#include <cstring>
#include <fstream>

#include "doctest/doctest.h"
#include "input_test_support.hpp"
#include "wp19c_fixture.hpp"

#include "ksp/career/funding.hpp"
#include "ksp/core/log.hpp"
#include "ksp/input/game_settings.hpp"
#include "ksp/input/input_lock_manager.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/science/science.hpp"
#include "ksp/space_center/launch_site.hpp"
#include "ksp/space_center/psystem_setup.hpp"
#include "ksp/vessel/vessel.hpp"

using namespace ksp;
using namespace wp19c;
using json = nlohmann::json;
using GE = events::GameEvents;
using Localizer = KSP::Localization::Localizer;

namespace {
const json& Ref19c() {
    static const json j = [] {
        std::ifstream f(std::string(KSPCORE_TEST_DATA_DIR) + "/wp19c_reference.json", std::ios::binary);
        REQUIRE(f.good());
        return json::parse(f);
    }();
    return j;
}
float HF(const json& j) {
    const uint32_t u = static_cast<uint32_t>(std::stoul(j.get<std::string>(), nullptr, 16));
    float f;
    std::memcpy(&f, &u, sizeof f);
    return f;
}
double HD(const json& j) {
    const uint64_t u = std::stoull(j.get<std::string>(), nullptr, 16);
    double d;
    std::memcpy(&d, &u, sizeof d);
    return d;
}
Vessel::Situations Sit(const std::string& s) {
    if (s == "LANDED") return Vessel::Situations::LANDED;
    if (s == "SPLASHED") return Vessel::Situations::SPLASHED;
    if (s == "PRELAUNCH") return Vessel::Situations::PRELAUNCH;
    if (s == "FLYING") return Vessel::Situations::FLYING;
    if (s == "ORBITING") return Vessel::Situations::ORBITING;
    FAIL("situation " << s);
    return Vessel::Situations::ORBITING;
}
struct Logs {
    std::vector<std::pair<log::Level, std::string>> lines;
    Logs() {
        log::SetSink([this](log::Level l, std::string_view m) { lines.emplace_back(l, std::string(m)); });
    }
    ~Logs() { log::SetSink(nullptr); }
};
}  // namespace

TEST_CASE("WP-19c FlightState(): the live capture (DEAD, DECLUTTER_KSC, MAX_VESSELS_BUDGET, order, activeVesselIdx) vs registry_reference.py (400 scenes)") {
    flighttest::FlightEnv w;
    CelestialBody* kerbin = w.Body("Kerbin");
    PSystemSetup setup;
    setup.Awake();
    for (const json& n : Ref19c().at("stockSites"))
        setup.stocklaunchsites.push_back(std::make_unique<LaunchSite>(n.get<std::string>(), "Kerbin", n.get<std::string>(),
                                                                       std::vector<LaunchSite::SpawnPoint>{}, n.get<std::string>(), EditorFacility::None));
    const bool declutter0 = GameSettings::DECLUTTER_KSC;
    const int budget0 = GameSettings::MAX_VESSELS_BUDGET;
    int scenes = 0, captured = 0, cleaned = 0, cut = 0;
    for (const json& sc : Ref19c().at("capture")) {
        FlightGlobals::Clear();
        w.vessels.clear();
        std::vector<Vessel*> vs;
        for (const json& jv : sc["vessels"]) {
            Vessel* v = w.MakeVessel("v" + std::to_string(vs.size()), kerbin, 700000.0 + 1000.0 * static_cast<double>(vs.size()));
            v->vesselType = static_cast<VesselType>(jv["type"].get<int>());
            v->set_isPersistent(jv["persistent"].get<bool>());
            v->situation = Sit(jv["situation"].get<std::string>());
            v->landedAt = jv["landedAt"].get<std::string>();
            v->protoVessel->landedAt = v->landedAt;  // LandedInKSC of an unloaded vessel reads protoVessel.landedAt
            if (jv["dead"].get<bool>()) v->state = Vessel::State::DEAD;
            if (jv["autoClean"].get<bool>()) v->SetAutoClean("old reason");
            vs.push_back(v);
        }
        GameSettings::DECLUTTER_KSC = sc["declutter"].get<bool>();
        GameSettings::MAX_VESSELS_BUDGET = sc["budget"].get<int>();
        FlightGlobals::ready = sc["ready"].get<bool>();
        const int active = sc["active"].get<int>();
        FlightGlobals::ActiveVessel = active >= 0 ? vs[static_cast<size_t>(active)] : nullptr;
        if (FlightGlobals::ready && FlightGlobals::ActiveVessel == nullptr) FlightGlobals::ready = false;  // the reference never has that case
        Logs logs;
        FlightState fs;
        INFO("scene " << sc.dump().substr(0, 400));
        const json& want = sc["captured"];
        REQUIRE(fs.protoVessels.size() == want.size());
        for (size_t k = 0; k < want.size(); k++) {
            Vessel* v = vs[want[k].get<size_t>()];
            CHECK(fs.protoVessels[k].get() == v->protoVessel);  // the vessel's own snapshot (BackupVessel), shared
        }
        for (size_t i = 0; i < vs.size(); i++) {
            CHECK(vs[i]->AutoClean() == sc["autoClean"][i][0].get<bool>());
            CHECK(vs[i]->AutoCleanReason() == sc["autoClean"][i][1].get<std::string>());
        }
        if (sc["ready"].get<bool>() && active >= 0) CHECK(fs.activeVesselIdx == sc["activeVesselIdx"].get<int>());
        if (!sc["ready"].get<bool>()) CHECK(fs.activeVesselIdx == 0);
        scenes++;
        captured += static_cast<int>(want.size());
        for (const json& c : sc["autoClean"]) cleaned += c[1].get<std::string>().rfind("it was debris", 0) == 0 ? 1 : 0;
        for (const auto& [lv, m] : logs.lines) cut += m.find("Too many vessels in scene") != std::string::npos ? 1 : 0;
        FlightGlobals::ActiveVessel = nullptr;
        FlightGlobals::ready = false;
    }
    MESSAGE(scenes << " scenes, " << captured << " vessels captured, " << cleaned << " auto-cleaned by DECLUTTER_KSC, " << cut << " cut by the budget");
    CHECK(cleaned > 50);
    CHECK(cut > 50);
    GameSettings::DECLUTTER_KSC = declutter0;
    GameSettings::MAX_VESSELS_BUDGET = budget0;
    setup.OnDestroy();
}

TEST_CASE("WP-19c ProtoVessel.Clean: the recovery deltas (Mathf.Round of float32 differences), the removal and the log text vs registry_reference.py (300 scenes)") {
    World w;  // the game's dictionary (Funds / Science / Reputation, the list connectors)
    CelestialBody* kerbin = w.Body("Kerbin");
    CelestialBody* mun = w.Body("Mun");
    int fired = 0, refunds = 0;
    for (const json& sc : Ref19c().at("clean")) {
        FlightGlobals::Clear();
        w.vessels.clear();
        Game game;
        game.Parameters = std::make_shared<GameParameters>();
        game.Parameters->CustomParams<GameParameters::AdvancedParams>().AllowNegativeCurrency = true;
        const std::string mode = sc["mode"].get<std::string>();
        game.Mode = mode == "CAREER" ? Game::Modes::CAREER : mode == "SCIENCE_SANDBOX" ? Game::Modes::SCIENCE_SANDBOX : Game::Modes::SANDBOX;
        game.flightState = std::make_shared<FlightState>();
        HighLogic::CurrentGame(&game);
        Funding funds;
        Reputation rep;
        ResearchAndDevelopment rnd;
        Funding::Instance = sc["has"]["funding"].get<bool>() ? &funds : nullptr;
        Reputation::Instance = sc["has"]["rep"].get<bool>() ? &rep : nullptr;
        ResearchAndDevelopment::Instance = sc["has"]["rnd"].get<bool>() ? &rnd : nullptr;
        funds.SetFunds(HD(sc["funds0"]), TransactionReasons::Cheating);
        rnd.SetScience(HF(sc["sci0"]), TransactionReasons::Cheating);
        rep.SetReputation(HF(sc["rep0"]), TransactionReasons::Cheating);
        // the unloaded vessel and its snapshot in the current game's flight state
        Vessel* v = w.MakeVessel(sc["name"].get<std::string>(), sc["home"].get<bool>() ? kerbin : mun, 700000.0);
        v->vesselName = sc["name"].get<std::string>();
        v->protoVessel->vesselName = v->vesselName;
        std::shared_ptr<ProtoVessel> pv;
        {
            ConfigNode node("VESSEL");
            v->protoVessel->Save(node);
            pv = std::make_shared<ProtoVessel>(node, &game);
        }
        pv->vesselRef = v;
        v->StartFromBackup(pv.get());
        game.flightState->protoVessels.push_back(pv);
        // the listener (in the game: VesselRecovery -> Funding / R&D / Reputation) adds the scene's amounts
        struct L {
            const json* sc;
            Funding* f;
            ResearchAndDevelopment* r;
            Reputation* p;
            int n = 0;
            bool quick = false;
            ProtoVessel* pv = nullptr;
            void On(ProtoVessel* x, bool q) {
                ++n;
                quick = q;
                pv = x;
                f->SetFunds(f->Funds() + HD((*sc)["dFunds"]), TransactionReasons::Cheating);
                r->SetScience(r->Science() + HF((*sc)["dSci"]), TransactionReasons::Cheating);
                p->SetReputation(p->reputation() + HF((*sc)["dRep"]), TransactionReasons::Cheating);
            }
        } l{&sc, &funds, &rnd, &rep};
        GE::onVesselRecovered.Add(&l, &L::On);
        Logs logs;
        ProtoVessel* raw = pv.get();
        pv.reset();  // the flight state's entry is the only owner now (and the vessel's own shared reference)
        raw->Clean(sc["reason"].get<std::string>());
        GE::onVesselRecovered.Remove(&l, &L::On);
        INFO("scene " << sc.dump());
        CHECK(l.n == (sc["fired"].get<bool>() ? 1 : 0));
        if (l.n == 1) {
            CHECK(l.quick);
            CHECK(l.pv == raw);
            fired++;
        }
        CHECK(game.flightState->protoVessels.empty());  // protoVessels.Remove(this)
        CHECK(std::find(FlightGlobals::Vessels.begin(), FlightGlobals::Vessels.end(), v) == FlightGlobals::Vessels.end());  // DestroyImmediate
        REQUIRE_FALSE(logs.lines.empty());
        CHECK(logs.lines.back().first == log::Level::Warning);
        CHECK(logs.lines.back().second == sc["text"].get<std::string>());
        refunds += sc["text"].get<std::string>().find("refund") != std::string::npos ? 1 : 0;
        Funding::Instance = nullptr;
        Reputation::Instance = nullptr;
        ResearchAndDevelopment::Instance = nullptr;
        HighLogic::CurrentGame(nullptr);
    }
    MESSAGE(fired << " recoveries fired, " << refunds << " refund texts");
    CHECK(refunds > 100);
}

TEST_CASE("WP-19c ProtoVessel.GetSituationString vs registry_reference.py (300 scenes, the game's en-us texts)") {
    World w;
    ConfigNode::NodePtr root = ConfigNode::Load(kSaves + "/physprofile/persistent.sfs");
    REQUIRE(root);
    const ConfigNode* vn = root->GetNode("GAME")->GetNode("FLIGHTSTATE")->GetNodes("VESSEL")[1];
    ProtoVessel pv(*vn, nullptr);
    std::vector<std::unique_ptr<CelestialBody>> owned;
    int checked = 0;
    for (const json& sc : Ref19c().at("situation")) {
        if (sc["text"].is_null()) continue;  // the minibiome display name (R&D): a separate case below
        pv.situation = static_cast<Vessel::Situations>(sc["situation"].get<int>());
        pv.landedAt = sc["landedAt"].get<std::string>();
        pv.displaylandedAt = sc["displaylandedAt"].get<std::string>();
        pv.orbitSnapShot->ReferenceBodyIndex = sc["ref"].get<int>();
        owned.clear();
        std::vector<CelestialBody*> bodies;
        const int n = sc["nbodies"].get<int>();
        for (int i = 0; i < n; i++) {
            owned.push_back(std::make_unique<CelestialBody>());
            owned.back()->bodyDisplayName = sc["bodyNames"][static_cast<size_t>(i)].get<std::string>();
            bodies.push_back(owned.back().get());
        }
        INFO("scene " << sc.dump());
        CHECK(ProtoVessel::GetSituationString(pv, n < 0 ? nullptr : &bodies) == sc["text"].get<std::string>());
        checked++;
    }
    CHECK(checked > 250);
    // landed with landedAt but no display name: the R&D minibiome name of the landed-at tag (ProtoVessel.cs:1014 / :1040)
    pv.situation = Vessel::Situations::LANDED;
    pv.landedAt = "KSC_LaunchPad_Platform";
    pv.displaylandedAt = "";
    const std::string biome = ResearchAndDevelopment::GetMiniBiomedisplayNameByScienceID(Vessel::GetLandedAtString(pv.landedAt), true);
    CHECK(ProtoVessel::GetSituationString(pv, nullptr) == Localizer::Format("#autoLOC_6100016", {biome}));
}

TEST_CASE("WP-19c ProtoPartSnapshot.LoadEVAInfoBySuitGender: suit x gender x DLC vs registry_reference.py") {
    SaveEnvironment::Reset();
    SaveEnvironment& env = SaveEnvironment::Current();
    std::map<std::string, std::unique_ptr<AvailablePart>> parts;
    env.getPartInfoByName = [&](std::string_view n) -> const AvailablePart* {
        auto& p = parts[std::string(n)];
        if (!p) {
            p = std::make_unique<AvailablePart>();
            p->name = std::string(n);
        }
        return p.get();
    };
    int rows = 0;
    for (const json& row : Ref19c().at("eva")) {
        // the stored suit was loaded while its DLC was installed
        env.SetStockExpansionInstalled("MakingHistory", true);
        env.SetStockExpansionInstalled("Serenity", true);
        ProtoCrewMember pcm(ProtoCrewMember::KerbalType::Crew, "Test Kerman");
        pcm.set_gender(row["gender"].get<std::string>() == "Female" ? ProtoCrewMember::Gender::Female : ProtoCrewMember::Gender::Male);
        const std::string s = row["suit"].get<std::string>();
        pcm.set_suit(s == "Vintage" ? ProtoCrewMember::KerbalSuit::Vintage : s == "Future" ? ProtoCrewMember::KerbalSuit::Future
                     : s == "Slim"  ? ProtoCrewMember::KerbalSuit::Slim
                                    : ProtoCrewMember::KerbalSuit::Default);
        env.SetStockExpansionInstalled("MakingHistory", row["makingHistory"].get<bool>());
        env.SetStockExpansionInstalled("Serenity", row["serenity"].get<bool>());
        const AvailablePart* ap = ProtoPartSnapshot::LoadEVAInfoBySuitGender(pcm);
        INFO("row " << row.dump());
        if (row["part"].is_null()) CHECK(ap == nullptr);
        else {
            REQUIRE(ap != nullptr);
            CHECK(ap->name == row["part"].get<std::string>());
        }
        rows++;
    }
    CHECK(rows == 32);
    SaveEnvironment::Reset();
}

TEST_CASE("WP-19c QuickSaveLoad.QuickSaveClearToSave and quickSave's refusals vs registry_reference.py") {
    flighttest::FlightEnv w;
    Vessel* v = w.MakeVessel("active", w.Body("Kerbin"), 700000.0, 0.0, true, false, OrbitDriver::UpdateMode::TRACK_Phys);
    FlightGlobals::ActiveVessel = v;
    const bool always0 = GameSettings::CAN_ALWAYS_QUICKSAVE;
    for (const json& row : Ref19c().at("quicksaveClear")) {
        // each status from the active vessel's state (FlightGlobals.ClearToSave, FlightGlobals.cs:1501)
        v->situation = Vessel::Situations::ORBITING;
        v->heightFromTerrain = -1.0f;
        v->geeForce = 0.0;
        v->Landed = false;
        v->srf_velocity = Vector3d();
        const int status = row["status"].get<int>();
        if (status == 1) v->situation = Vessel::Situations::FLYING;
        if (status == 2) v->geeForce = 1.0;
        if (status == 3) {
            v->Landed = true;
            v->srf_velocity = Vector3d(1.0, 0.0, 0.0);
        }
        if (status == 4) {
            v->situation = Vessel::Situations::FLYING;
            v->heightFromTerrain = 0.2f;
        }
        if (status == 5 || status >= 6) continue;  // the ladder (EVA controller) and the two statuses ClearToSave never returns
        GameSettings::CAN_ALWAYS_QUICKSAVE = row["canAlways"].get<bool>();
        REQUIRE(static_cast<int>(FlightGlobals::ClearToSave(false)) == status);
        INFO("row " << row.dump());
        CHECK(QuickSaveLoad::QuickSaveClearToSave() == row["clear"].get<bool>());
    }
    GameSettings::CAN_ALWAYS_QUICKSAVE = always0;
    // quickSave's refusals: CanQuickSave off (#autoLOC_174631), not clear (GetNotClearToSaveStatusReason(.., #autoLOC_174654)), DEAD (#autoLOC_174659)
    HighLogic hl;
    hl.Awake();
    Game game;
    game.Parameters = std::make_shared<GameParameters>();
    HighLogic::CurrentGame(&game);
    QuickSaveLoad qsl;
    std::vector<std::pair<std::string, float>> m;
    QuickSaveLoad::env.postScreenMessage = [&](const std::string& s, float d) { m.emplace_back(s, d); };
    game.Parameters->Flight.CanQuickSave = false;
    qsl.quickSave(false);
    game.Parameters->Flight.CanQuickSave = true;
    v->situation = Vessel::Situations::FLYING;
    v->heightFromTerrain = 0.2f;  // about to crash
    qsl.quickSave(false);
    v->state = Vessel::State::DEAD;
    qsl.quickSave(false);
    REQUIRE(m.size() == 3);
    CHECK(m[0] == std::make_pair(Localizer::Format("#autoLOC_174631"), 2.0f));
    CHECK(m[1] == std::make_pair(FlightGlobals::GetNotClearToSaveStatusReason(ClearToSaveStatus::NOT_WHILE_ABOUT_TO_CRASH, Localizer::Format("#autoLOC_174654")), 2.0f));
    CHECK(m[2] == std::make_pair(Localizer::Format("#autoLOC_174659"), 2.0f));
    QuickSaveLoad::ResetStatics();
    HighLogic::CurrentGame(nullptr);
    hl.OnDestroy();
    HighLogic::ResetStatics();
}

TEST_CASE("WP-19c QuickSaveLoad.Update: the F5 / F9 / modifier branches, the hold-to-load Invoke, CancelInvoke vs registry_reference.py (400 frames)") {
    World w;
    wp50test::InputFixture input;
    // a real current game (the physprofile save): quickSave writes it, quickLoad reads it back (LoadGameCfg needs a complete GAME)
    w.CopySave(kSaves + "/physprofile/persistent.sfs", "persistent");
    std::shared_ptr<Game> current(GamePersistence::LoadGame("persistent", HighLogic::SaveFolder(), true, true));
    REQUIRE(current != nullptr);
    Game& game = *current;
    HighLogic::CurrentGame(&game);
    Vessel* v = w.MakeVessel("active", w.Body("Kerbin"), 700000.0, 0.0, true, false, OrbitDriver::UpdateMode::TRACK_Phys);
    v->situation = Vessel::Situations::ORBITING;
    FlightGlobals::ActiveVessel = v;
    std::vector<std::string> acts;
    QuickSaveLoad::env.postScreenMessage = [&](const std::string& s, float) {
        if (s == Localizer::Format("#autoLOC_174575")) acts.push_back("msg:#autoLOC_174575");
        else if (s == Localizer::Format("#autoLOC_174610")) acts.push_back("msg:#autoLOC_174610");
        else if (s == Localizer::Format("#autoLOC_174589", {GameSettings::QUICKLOAD->name()})) acts.push_back("msg:#autoLOC_174589");
        else if (s == Localizer::Format("#autoLOC_174686", {std::string("#autoLOC_6002266")})) acts.push_back("quickLoad");  // no quicksave file
        else if (s == Localizer::Format("#autoLOC_174669")) {
        }  // doSave's "Quicksaved" (recorded by onGameAboutToQuicksave)
        else acts.push_back("other:" + s);
    };
    QuickSaveLoad::env.spawnSaveDialog = [&](const std::string&) { acts.push_back("quickSave(saveAs)"); };
    QuickSaveLoad::env.spawnLoadBrowser = [&](const std::string&) { acts.push_back("loadDialog"); };
    struct L {
        std::vector<std::string>* a;
        void On() { a->push_back("quickSave(save)"); }
    } l{&acts};
    GE::onGameAboutToQuicksave.Add(&l, &L::On);
    // a quickLoad that finds the quicksave of the same frame ends in FlightDriver.StartAndFocusVessel -> HighLogic.LoadScene(FLIGHT)
    HighLogic::sceneLoader = [&](int, bool) { acts.push_back("quickLoad"); };
    int n = 0;
    for (const json& sc : Ref19c().at("update")) {
        QuickSaveLoad qsl;
        REQUIRE(qsl.Alive());
        qsl.holdToLoad = sc["holdToLoad"].get<bool>();
        game.Parameters->Flight.CanQuickLoad = true;
        input.dev.held.clear();
        input.dev.down.clear();
        input.dev.up.clear();
        if (sc["invoking"].get<bool>()) {  // a pending Invoke("onHoldComplete") from an earlier frame
            const bool hold = qsl.holdToLoad;
            qsl.holdToLoad = true;
            input.dev.down.insert("F9");
            qsl.Update();
            input.dev.down.clear();
            qsl.holdToLoad = hold;
            REQUIRE(qsl.IsInvokingHoldComplete());
        }
        acts.clear();
        QuickSaveLoad::env.cameraMode = [&] { return sc["cameraMode"].get<bool>(); };
        if (sc["saveLocked"].get<bool>()) InputLockManager::SetControlLock(ControlTypes::QUICKSAVE, "wp19c_s");
        if (sc["loadLocked"].get<bool>()) InputLockManager::SetControlLock(ControlTypes::QUICKLOAD, "wp19c_l");
        if (sc["saveDown"].get<bool>()) input.dev.down.insert("F5");
        if (sc["loadDown"].get<bool>()) input.dev.down.insert("F9");
        if (sc["loadUp"].get<bool>()) input.dev.up.insert("F9");
        if (sc["modifier"].get<bool>()) input.dev.held.insert("LeftAlt");
        game.Parameters->Flight.CanQuickLoad = sc["canQuickLoad"].get<bool>();
        FlightDriver::pause = sc["paused"].get<bool>();
        Logs logs;
        qsl.Update();
        std::string logText;
        for (const auto& [lv, m] : logs.lines) logText += m.substr(0, 160) + " || ";
        INFO("log " << logText);
        INFO("frame " << sc.dump());
        std::vector<std::string> want;  // Invoke / CancelInvoke leave no trace but the pending state (checked below)
        for (const json& a : sc["actions"])
            if (a.get<std::string>() != "invoke" && a.get<std::string>() != "cancelInvoke") want.push_back(a.get<std::string>());
        std::string got;
        for (const std::string& a : acts) got += a + " | ";
        INFO("got " << got);
        // the save-as and load dialogs are closed again before the next frame (their own buttons)
        CHECK(acts == want);
        CHECK(qsl.IsInvokingHoldComplete() == sc["invokingAfter"].get<bool>());
        if (qsl.SaveDialogOpen()) qsl.OnSaveAsClose("", true, nullptr);
        if (qsl.LoadBrowserOpen()) qsl.OnLoadClose("");
        InputLockManager::RemoveControlLock("wp19c_s");
        InputLockManager::RemoveControlLock("wp19c_l");
        std::error_code ec;
        fs::remove(w.folder / "quicksave.sfs", ec);  // a quickLoad of a later frame finds no file (its message records the call)
        FlightDriver::pause = false;
        QuickSaveLoad::env.cameraMode = nullptr;
        n++;
    }
    GE::onGameAboutToQuicksave.Remove(&l, &L::On);
    CHECK(n == 400);
    HighLogic::CurrentGame(nullptr);
}
