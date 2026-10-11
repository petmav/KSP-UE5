// WP-50 unit tests: input, bindings, input locks (include/ksp/input/*, src/input/*).
//
// Gate: tools/wp50/input_reference.py -> tests/data/wp50_reference.json, an independent Python implementation (no kspcore imports) of
// ControlTypes / KeyCode enum text, InputLockManager (.NET Dictionary order, events), KeyCodeExtended / KeyBinding / AxisBinding
// parsing and serialisation, the shipped keyboard layouts (GameData/Squad/Controls/*.cfg, original game data), GameSettings defaults,
// AxisBinding_Single.GetAxis and FlightInputHandler.FixedUpdate (Mono float rule), Mouse tap routine (Python generator = the
// Unity coroutine), SpaceNavigator scaling. Float comparisons are bit-exact (0 ULP).
#include <cmath>
#include <cstring>
#include <fstream>
#include <map>
#include <set>
#include <string>

#include "doctest/doctest.h"
#include "nlohmann/json.hpp"
#include "resource_fixture.hpp"

#include "ksp/core/unity_time.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/dotnet/enum_parse.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/input/control_types.hpp"
#include "ksp/input/extended_input.hpp"
#include "ksp/input/flight_input_handler.hpp"
#include "ksp/input/game_settings.hpp"
#include "ksp/input/input_binding.hpp"
#include "ksp/input/input_lock_manager.hpp"
#include "ksp/input/input_settings.hpp"
#include "ksp/input/mouse.hpp"
#include "ksp/input/space_navigator.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/space_center/high_logic.hpp"
#include "ksp/space_center/scene_drivers.hpp"
#include "ksp/unity/quaternion_native.hpp"
#include "ksp/vessel/vessel.hpp"

using namespace ksp;
using json = nlohmann::json;

namespace {

const json& Ref() {
    static const json j = [] {
        std::ifstream f(std::string(KSPCORE_TEST_DATA_DIR) + "/wp50_reference.json", std::ios::binary);
        REQUIRE(f.good());
        return json::parse(f);
    }();
    return j;
}

uint64_t U64(const json& s) { return std::stoull(s.get<std::string>()); }

bool SameBits(float a, float b) {
    uint32_t x, y;
    std::memcpy(&x, &a, 4);
    std::memcpy(&y, &b, 4);
    return x == y;
}

float F(const json& j) { return static_cast<float>(j.get<double>()); }

// UnityEngine.Input driven by the test: held / down / up key names (KeyCode.ToString), analogue axes, the mouse
class ScriptedDevice final : public IInputDevice {
public:
    std::set<std::string> held, down, up;
    std::map<std::string, float> axes;
    bool mouseHeld[5] = {false, false, false, false, false}, mouseDown[5] = {false, false, false, false, false},
         mouseUp[5] = {false, false, false, false, false};
    Vector3f mouse{0.0f, 0.0f, 0.0f};
    std::u16string typed;
    std::vector<std::string> joysticks;

    bool GetKey(KeyCode k) const override { return held.count(dotnet::EnumToString(k)) != 0; }
    bool GetKeyDown(KeyCode k) const override { return down.count(dotnet::EnumToString(k)) != 0; }
    bool GetKeyUp(KeyCode k) const override { return up.count(dotnet::EnumToString(k)) != 0; }
    float GetAxis(const std::string& n) const override {
        auto it = axes.find(n);
        return it == axes.end() ? 0.0f : it->second;
    }
    bool GetMouseButton(int b) const override { return mouseHeld[b]; }
    bool GetMouseButtonDown(int b) const override { return mouseDown[b]; }
    bool GetMouseButtonUp(int b) const override { return mouseUp[b]; }
    Vector3f mousePosition() const override { return mouse; }
    std::u16string inputString() const override { return typed; }
    std::vector<std::string> GetJoystickNames() const override { return joysticks; }
};

// restores the process-wide input statics
struct InputFixture {
    ScriptedDevice dev;
    InputFixture() {
        InputLockManager::Reset();
        ExtendedInput::Reset();
        InputEnvironment::device = &dev;
        InputEnvironment::realtimeSinceStartup = 0.0f;
        InputEnvironment::applicationIsPlaying = true;
        InputBinding::linRotState = false;
        HighLogic::LoadedSceneIsFlight = false;
        FlightUIModeControllerSeam::present = false;
        GameSettings::SetDefaultValues();
    }
    ~InputFixture() {
        InputEnvironment::device = nullptr;
        InputLockManager::Reset();
        ExtendedInput::Reset();
        HighLogic::LoadedSceneIsFlight = false;
        FlightUIModeControllerSeam::present = false;
        InputBinding::linRotState = false;
    }
};

std::string LayoutDir() { return ksp::testing::GameRoot() + "/GameData/Squad/Controls/"; }

std::string Str(const std::optional<std::string>& s) { return s ? *s : std::string("<null>"); }

struct LockListener {
    std::vector<std::vector<std::string>> cur;
    void OnMod(events::GameEvents::FromToAction<ControlTypes, ControlTypes> a) {
        cur.push_back({"mod", std::to_string(static_cast<uint64_t>(a.from)), std::to_string(static_cast<uint64_t>(a.to))});
    }
    void OnGuiLock() { cur.push_back({"guilock"}); }
    void OnTooltip() { cur.push_back({"tooltip"}); }
    void OnGuiUnlock() { cur.push_back({"guiunlock"}); }
};

}  // namespace

TEST_CASE("WP-50 ControlTypes: every member, and Enum.ToString of 288 flag combinations, equal the reference") {
    const json& r = Ref()["control_types"];
    using Traits = dotnet::EnumTraits<ControlTypes>;
    CHECK(r["members"].size() == sizeof(Traits::members) / sizeof(Traits::members[0]));
    for (size_t i = 0; i < r["members"].size(); ++i) {
        CHECK(std::string(Traits::members[i].name) == r["members"][i][0].get<std::string>());
        CHECK(static_cast<uint64_t>(Traits::members[i].value) == U64(r["members"][i][1]));
    }
    // two members have the value 0 (None, STAGE_IN_MAP_CONTROLS); Mono's Array.Sort tie order picks which one is table index 0 / 1, so the
    // reference holds <Z0> / <Z1> and either assignment is accepted -- the same one for every value
    const std::string zA = r["zero_names"][0], zB = r["zero_names"][1];
    auto expand = [&](std::string e, const std::string& z0, const std::string& z1) {
        for (const auto& [tok, name] : {std::pair<std::string, std::string>{"<Z0>", z0}, {"<Z1>", z1}}) {
            for (size_t p = e.find(tok); p != std::string::npos; p = e.find(tok, p + name.size())) e.replace(p, tok.size(), name);
        }
        return e;
    };
    bool orderAB = true, orderBA = true;
    int n = 0;
    for (const auto& t : r["tostring"]) {
        const std::string got = dotnet::EnumToString(static_cast<ControlTypes>(U64(t[0])));
        orderAB = orderAB && got == expand(t[1].get<std::string>(), zA, zB);
        orderBA = orderBA && got == expand(t[1].get<std::string>(), zB, zA);
        ++n;
    }
    CHECK((orderAB || orderBA));
    CHECK(n > 250);
}

TEST_CASE("WP-50 KeyCode members and KeyCodeExtended(string): Enum.Parse rules, name fallback, ToString") {
    const json& r = Ref()["keycode"];
    using Traits = dotnet::EnumTraits<KeyCode>;
    REQUIRE(r["members"].size() == sizeof(Traits::members) / sizeof(Traits::members[0]));
    for (size_t i = 0; i < r["members"].size(); ++i) {
        CHECK(std::string(Traits::members[i].name) == r["members"][i][0].get<std::string>());
        CHECK(static_cast<int>(Traits::members[i].value) == r["members"][i][1].get<int>());
    }
    for (const auto& c : r["kce"]) {
        const std::string tok = c[0].get<std::string>();
        if (c[1].is_null()) {
            CHECK_THROWS_AS(KeyCodeExtended{tok}, dotnet::OverflowException);
            continue;
        }
        KeyCodeExtended k{tok};
        CHECK(static_cast<int>(k.code) == c[1]["code"].get<int>());
        CHECK(k.name == c[1]["name"].get<std::string>());
        CHECK(k.ToString() == c[1]["tostring"].get<std::string>());
        CHECK(k.isNone() == (c[1]["code"].get<int>() == 0 && c[1]["name"].get<std::string>().empty()));
    }
}

TEST_CASE("WP-50 InputLockManager: 400 random lock operations, masks, events, queries and Dictionary order equal the reference") {
    InputFixture fx;
    KSP::Localization::Localizer::Shutdown();
    const json& r = Ref()["locks"];
    LockListener l;
    events::GameEvents::onInputLocksModified.Add(&l, &LockListener::OnMod);
    events::GameEvents::onGUILock.Add(&l, &LockListener::OnGuiLock);
    events::GameEvents::onTooltipDestroyRequested.Add(&l, &LockListener::OnTooltip);
    events::GameEvents::onGUIUnlock.Add(&l, &LockListener::OnGuiUnlock);
    for (size_t i = 0; i < r["ops"].size(); ++i) {
        const json& op = r["ops"][i];
        const json& e = r["results"][i];
        l.cur.clear();
        const std::string o = op["op"];
        if (o == "set") InputLockManager::SetControlLock(static_cast<ControlTypes>(U64(op["mask"])), op["id"]);
        else if (o == "setall") InputLockManager::SetControlLock(op["id"].get<std::string>());
        else if (o == "remove") InputLockManager::RemoveControlLock(op["id"]);
        else InputLockManager::ClearControlLocks();
        // events
        REQUIRE(l.cur.size() == e["events"].size());
        for (size_t k = 0; k < l.cur.size(); ++k) {
            for (size_t m = 0; m < l.cur[k].size(); ++m) CHECK(l.cur[k][m] == e["events"][k][m].get<std::string>());
        }
        CHECK(InputLockManager::LockMask() == U64(e["mask"]));
        // Dictionary order and values
        std::vector<std::string> order;
        size_t idx = 0;
        for (const auto& kv : InputLockManager::lockStack) {
            order.push_back(kv.first);
            REQUIRE(idx < e["values"].size());
            CHECK(kv.second == U64(e["values"][idx]));
            ++idx;
        }
        CHECK(order.size() == e["order"].size());
        for (size_t k = 0; k < order.size() && k < e["order"].size(); ++k) CHECK(order[k] == e["order"][k].get<std::string>());
        // queries
        for (auto it = e["queries"].begin(); it != e["queries"].end(); ++it) {
            const ControlTypes probe = static_cast<ControlTypes>(std::stoull(it.key()));
            CHECK(InputLockManager::IsLocked(probe) == (it.value()[0].get<int>() != 0));
            CHECK(InputLockManager::IsUnlocked(probe) == (it.value()[1].get<int>() != 0));
            CHECK(InputLockManager::IsAllLocked(probe) == (it.value()[2].get<int>() != 0));
            CHECK(InputLockManager::IsAnyUnlocked(probe) == (it.value()[3].get<int>() != 0));
        }
        // PrintLockStack
        std::string expect;
        if (order.empty()) expect = KSP::Localization::Localizer::Format("#autoLOC_176849");
        else {
            expect = KSP::Localization::Localizer::Format("#autoLOC_176851");
            for (const auto& k : order) expect += k + "\n";
            expect += "-- \nbitmask: " + e["bits"].get<std::string>();
        }
        CHECK(InputLockManager::PrintLockStack() == expect);
    }
    events::GameEvents::onInputLocksModified.Remove(&l, &LockListener::OnMod);
    events::GameEvents::onGUILock.Remove(&l, &LockListener::OnGuiLock);
    events::GameEvents::onTooltipDestroyRequested.Remove(&l, &LockListener::OnTooltip);
    events::GameEvents::onGUIUnlock.Remove(&l, &LockListener::OnGuiUnlock);
}

TEST_CASE("WP-50 InputLockManager: ControlLocks(lower, upper) shifts, GetControlLock, IsLocking / IsUnlocking") {
    InputFixture fx;
    for (const auto& c : Ref()["control_locks"])
        CHECK(InputLockManager::ControlLocks(c[0].get<int>(), c[1].get<int>()) == U64(c[2]));
    InputLockManager::SetControlLock(ControlTypes::PITCH | ControlTypes::YAW, "a");
    CHECK(InputLockManager::GetControlLock("a") == (ControlTypes::PITCH | ControlTypes::YAW));
    CHECK(InputLockManager::GetControlLock("missing") == ControlTypes::None);
    using FT = InputLockManager::FromToAction;
    const FT ft(ControlTypes::PITCH, ControlTypes::PITCH | ControlTypes::ROLL);
    CHECK(InputLockManager::IsLocking(ControlTypes::ROLL, ft));
    CHECK_FALSE(InputLockManager::IsLocking(ControlTypes::PITCH, ft));
    CHECK_FALSE(InputLockManager::IsUnlocking(ControlTypes::PITCH, ft));
    const FT unlock(ControlTypes::PITCH | ControlTypes::ROLL, ControlTypes::ROLL);
    CHECK(InputLockManager::IsUnlocking(ControlTypes::PITCH, unlock));
    CHECK_FALSE(InputLockManager::IsUnlocking(ControlTypes::ROLL, unlock));
    // RemoveControlLock of an unknown ID fires nothing and changes nothing
    LockListener l;
    events::GameEvents::onInputLocksModified.Add(&l, &LockListener::OnMod);
    InputLockManager::RemoveControlLock("nope");
    CHECK(l.cur.empty());
    events::GameEvents::onInputLocksModified.Remove(&l, &LockListener::OnMod);
}

TEST_CASE("WP-50 shipped keyboard layouts: dependency + KEY_MAP merge, Load, and the Save round trip equal the reference (9 layouts)") {
    InputFixture fx;
    const json& r = Ref()["layouts"];
    std::vector<std::string> files;
    for (const auto& n : r["order"]) files.push_back(LayoutDir() + n.get<std::string>());
    GameSettings::layoutConfigNodes.Clear();
    GameSettings::ParseLayoutsCfg(files);
    CHECK(GameSettings::layoutConfigNodes.Count() == static_cast<int>(r["order"].size()));
    int bindings = 0;
    for (const auto& nameJ : r["order"]) {
        const std::string name = nameJ;
        const json& lay = r["layouts"][name];
        // merged tree
        const ConfigNode::NodePtr merged = GameSettings::layoutConfigNodes.At(name);
        ConfigNode* kl = merged->GetNode("KEYBOARD_LAYOUT");
        REQUIRE(kl != nullptr);
        CHECK(Str(kl->GetValue("layout")) == lay["layout_name"][0].get<std::string>());
        ConfigNode* km = merged->GetNode("KEY_MAP");
        CHECK((km ? km->values().Count() : 0) == lay["keymap_count"].get<int>());
        // load into the defaults and save again
        GameSettings::SetDefaultValues();
        GameSettings::LoadLayoutKeyBindings(name, true);
        CHECK(GameSettings::CURRENT_LAYOUT_SETTINGS == name);
        const ConfigNode::NodePtr cfg = GameSettings::BuildCfg();
        for (auto it = lay["bindings"].begin(); it != lay["bindings"].end(); ++it) {
            ConfigNode* node = cfg->GetNode(it.key());
            REQUIRE(node != nullptr);
            const json& e = it.value();
            const char* keys[5] = {"primary", "secondary", "group", "modeMask", "modeMaskSec"};
            for (int k = 0; k < 5; ++k) CHECK(Str(node->GetValue(keys[k])) == e[static_cast<size_t>(k)].get<std::string>());
            CHECK(node->values().Count() == 5);
            ++bindings;
        }
    }
    CHECK(bindings > 500);
}

TEST_CASE("WP-50 GameSettings.DetectKeyboardLayout: the file-name matching rules, and KeyboardLayout.GetKeyboardLayoutType") {
    InputFixture fx;
    GameSettings::layoutConfigNodes.Clear();
    std::vector<std::string> files;
    for (const auto& n : Ref()["layouts"]["order"]) files.push_back(LayoutDir() + n.get<std::string>());
    GameSettings::ParseLayoutsCfg(files);
    struct Case { KeyboardLayoutType t; const char* locale; const char* iso2; const char* expect; };
    const Case cases[] = {
        {KeyboardLayoutType::QWERTY, "en-US", "en", "Qwerty.cfg"},       {KeyboardLayoutType::AZERTY, "fr-FR", "fr", "Azerty.cfg"},
        {KeyboardLayoutType::QWERTZ, "de-DE", "de", "Qwertz.cfg"},       {KeyboardLayoutType::QWERTY, "en-GB", "en", "Qwerty_en-GB.cfg"},
        {KeyboardLayoutType::QWERTY, "it-IT", "it", "Qwerty_it-IT.cfg"}, {KeyboardLayoutType::Dvorak, "en-US", "en", "Dvorak.cfg"},
        {KeyboardLayoutType::TurkishF, "tr-TR", "tr", "Qwerty.cfg"},     {KeyboardLayoutType::AZERTY, "fr-BE", "fr", "Azerty.cfg"},
        {KeyboardLayoutType::QWERTY, "ja-JP", "ja", "Qwerty_ja-JP.cfg"}, {KeyboardLayoutType::QWERTY, "pt-BR", "pt", "Qwerty_pt-PT.cfg"},
    };
    for (const Case& c : cases) {
        KeyboardLayout::provider = [c] {
            KeyboardLayout k;
            k.Type = c.t;
            k.LocaleName = c.locale;
            k.TwoLetterISOLanguageName = c.iso2;
            return k;
        };
        CHECK(GameSettings::DetectKeyboardLayout() == c.expect);
    }
    KeyboardLayout::provider = nullptr;
    CHECK(GameSettings::DetectKeyboardLayout() == "Qwerty.cfg");
    // GetKeyboardLayoutType: the high word of the HKL
    auto code = [](int hi) { return (static_cast<int64_t>(hi) << 16) | 0x0409; };
    CHECK(KeyboardLayout::GetKeyboardLayoutType(code(1062)) == KeyboardLayoutType::Latvian);
    CHECK(KeyboardLayout::GetKeyboardLayoutType(code(61460)) == KeyboardLayoutType::TurkishF);
    CHECK(KeyboardLayout::GetKeyboardLayoutType(code(66591)) == KeyboardLayoutType::TurkishF);
    CHECK(KeyboardLayout::GetKeyboardLayoutType(code(61442)) == KeyboardLayoutType::Dvorak);
    CHECK(KeyboardLayout::GetKeyboardLayoutType(code(66569)) == KeyboardLayoutType::Dvorak);
    CHECK(KeyboardLayout::GetKeyboardLayoutType(code(1036)) == KeyboardLayoutType::AZERTY);
    CHECK(KeyboardLayout::GetKeyboardLayoutType(code(2060)) == KeyboardLayoutType::AZERTY);
    CHECK(KeyboardLayout::GetKeyboardLayoutType(code(1031)) == KeyboardLayoutType::QWERTZ);
    CHECK(KeyboardLayout::GetKeyboardLayoutType(code(2055)) == KeyboardLayoutType::QWERTZ);
    CHECK(KeyboardLayout::GetKeyboardLayoutType(code(1033)) == KeyboardLayoutType::QWERTY);
    CHECK(KeyboardLayout::GetKeyboardLayoutType(code(0)) == KeyboardLayoutType::QWERTY);
}
