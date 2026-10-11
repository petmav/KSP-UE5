// WP-50 unit tests, part 2: GameSettings defaults / settings.cfg, axis maths, FlightInputHandler, Mouse, SpaceNavigator, InputSettings,
// ExtendedInput and the binding serialisation corner cases. References: tools/wp50/input_reference.py (see test_input.cpp) and, where noted,
// literal expectations read off the C# line by line.
#include "input_test_support.hpp"

using namespace ksp;
using json = nlohmann::json;
using namespace wp50test;

namespace {

const SettingsField* FieldByName(const std::string& n) {
    for (const SettingsField& f : GameSettings::Fields())
        if (n == f.name) return &f;
    return nullptr;
}

uint32_t Bits(float f) {
    uint32_t b;
    std::memcpy(&b, &f, 4);
    return b;
}

}  // namespace

TEST_CASE("WP-50 GameSettings: the public static field table has the reference order, SetDefaultValues the reference values") {
    InputFixture fixture;
    const json& ref = Ref();
    const auto& fields = GameSettings::Fields();
    REQUIRE(fields.size() == ref["public_fields"].size());
    for (size_t i = 0; i < fields.size(); ++i) CHECK(std::string(fields[i].name) == ref["public_fields"][i].get<std::string>());

    int scalars = 0;
    for (auto it = ref["defaults"]["scalars"].begin(); it != ref["defaults"]["scalars"].end(); ++it) {
        const SettingsField* f = FieldByName(it.key());
        REQUIRE(f != nullptr);
        const std::string ty = it.value()[0];
        const json& v = it.value()[1];
        if (ty == "bool") {
            CHECK(*static_cast<bool*>(f->address) == v.get<bool>());
        } else if (ty == "int") {
            CHECK(*static_cast<int*>(f->address) == v.get<int>());
        } else if (ty == "string") {
            CHECK(*static_cast<std::string*>(f->address) == v.get<std::string>());
        } else if (ty == "float") {
            CHECK(Bits(*static_cast<float*>(f->address)) == v.get<uint32_t>());
        } else if (ty == "double") {
            CHECK(*static_cast<double*>(f->address) == v.get<double>());
        } else {  // enum: ValidatorMode.Manual / TimeWarp.MaxRailsRateMode.PeAltitude
            const std::string e = v.get<std::string>();
            const std::string member = e.substr(e.rfind('.') + 1);
            if (f->kind == SettingsField::Kind::ValidatorMode)
                CHECK(dotnet::EnumToString(*static_cast<Expansions::Missions::Editor::ValidatorMode*>(f->address)) == member);
            else
                CHECK(dotnet::EnumToString(*static_cast<TimeWarp::MaxRailsRateMode*>(f->address)) == member);
        }
        ++scalars;
    }
    CHECK(scalars > 280);

    int kb = 0;
    for (auto it = ref["defaults"]["keybindings"].begin(); it != ref["defaults"]["keybindings"].end(); ++it) {
        const SettingsField* f = FieldByName(it.key());
        REQUIRE(f != nullptr);
        REQUIRE(f->kind == SettingsField::Kind::KeyBinding);
        const KeyBinding& b = **static_cast<std::shared_ptr<KeyBinding>*>(f->address);
        const json& e = it.value();
        CHECK(b.primary.ToString() == e[0].get<std::string>());
        CHECK(b.secondary.ToString() == e[1].get<std::string>());
        CHECK(std::to_string(b.inputLockMask) == e[2].get<std::string>());
        CHECK(std::to_string(static_cast<int>(b.switchState)) == e[3].get<std::string>());
        CHECK(std::to_string(static_cast<int>(b.switchStateSecondary)) == e[4].get<std::string>());
        ++kb;
    }
    CHECK(kb > 100);
    // the AxisBinding / AxisKeyBindingList / TrackIR / enum constructions of SetDefaultValues
    CHECK(GameSettings::AXIS_THROTTLE->primary.neutralPoint == -1.0f);
    CHECK(GameSettings::AXIS_THROTTLE->secondary.neutralPoint == -1.0f);
    CHECK(GameSettings::AXIS_PITCH->primary.neutralPoint == 0.0f);
    CHECK(GameSettings::AXIS_MOUSEWHEEL->primary.idTag == "Mouse ScrollWheel");
    CHECK(GameSettings::AXIS_MOUSEWHEEL->primary.deadzone == 0.0f);
    CHECK(GameSettings::AXIS_MOUSEWHEEL->secondary.idTag == "None");
    CHECK(GameSettings::AXIS_CUSTOM->Length() == 4);
    CHECK(static_cast<bool>(GameSettings::TRACKIR));
    CHECK(static_cast<bool>(GameSettings::AXIS_THROTTLE_INC));
    CHECK(ref["defaults"]["axes"].size() == 21);
}

TEST_CASE("WP-50 GameSettings.WriteCfg: Convert.ToString text of every default (Mono float G7 / double G15) equals the reference") {
    InputFixture fixture;
    const ConfigNode::NodePtr cfg = GameSettings::BuildCfg();
    int n = 0;
    for (auto it = Ref()["default_text"].begin(); it != Ref()["default_text"].end(); ++it) {
        const std::string expect = it.value();
        const std::string got = Str(cfg->GetValue(it.key()));
        // ConfigNode.AddValue trims the stored value (e.g. the COLOR_LIGHT_PRESET_2.. defaults end in a space)
        CHECK(dotnet::Trim(got) == dotnet::Trim(expect));
        ++n;
    }
    CHECK(n > 280);
    // node fields come out as nodes, in field order, and everything else as values
    CHECK(cfg->HasNode("PITCH_DOWN"));
    CHECK(cfg->HasNode("AXIS_PITCH"));
    CHECK(cfg->HasNode("AXIS_CUSTOM"));
    CHECK(cfg->GetNode("AXIS_CUSTOM")->nodes().Count() == 4);
    CHECK(cfg->HasNode("INPUT_DEVICES") == false);  // INPUT_DEVICES is null until GameSettings.Awake creates it: an empty value
    CHECK(Str(cfg->GetValue("INPUT_DEVICES")) == "");
    CHECK(Str(cfg->GetValue("AXIS_INCREMENTAL_SPEED_MULTIPLIER_VALUES")) == "");
    CHECK(Str(cfg->GetValue("ORBIT_WARP_MAXRATE_MODE")) == "PeAltitude");
    CHECK(Str(cfg->GetValue("MISSION_VALIDATOR_MODE")) == "Manual");
    GameSettings::InitializeDefaultValues();
    const ConfigNode::NodePtr cfg2 = GameSettings::BuildCfg();
    CHECK(Str(cfg2->GetValue("AXIS_INCREMENTAL_SPEED_MULTIPLIER_VALUES")) == "System.Collections.Generic.List`1[System.Single]");
}

TEST_CASE("WP-50 GameSettings.ParseCfg: typed values, nodes, version gates, errors") {
    InputFixture fixture;
    GameSettings::host = {};
    auto cfg = ConfigNode::Create();
    cfg->AddValue("SETTINGS_FILE_VERSION", "1.3.0");
    cfg->AddValue("UI_SCALE", "1.25");
    cfg->AddValue("MAX_VESSELS_BUDGET", "77");
    cfg->AddValue("LANGUAGE", "de-de");
    cfg->AddValue("FULLSCREEN", "True");
    cfg->AddValue("EVA_LADDER_JOINT_BREAK_VELOCITY", "123.5");
    cfg->AddValue("MISSION_VALIDATOR_MODE", "FullAuto");
    cfg->AddValue("ORBIT_WARP_MAXRATE_MODE", "VesselAltitude");  // no branch in ParseCfg: unchanged
    cfg->AddValue("NOT_A_FIELD", "5");
    cfg->AddValue("PAW_COLLAPSED_GROUP_NAMES", "A,,B");
    ConfigNode* pd = cfg->AddNode("PITCH_DOWN");
    pd->AddValue("primary", "T");
    pd->AddValue("secondary", "ö");
    pd->AddValue("group", "5");
    pd->AddValue("modeMask", "2");
    pd->AddValue("modeMaskSec", "4");
    ConfigNode* tr = cfg->AddNode("TRACKIR");  // not a field name: ignored
    tr->AddValue("x", "1");
    ConfigNode* trk = cfg->AddNode("TRACKIR_NOT");
    (void)trk;
    CHECK(GameSettings::ParseCfg(cfg) == GameSettings::ParseResult::Ok);
    CHECK(GameSettings::SETTINGS_FILE_VERSION == "1.3.0");
    CHECK(GameSettings::UI_SCALE == 1.25f);
    CHECK(GameSettings::MAX_VESSELS_BUDGET == 77);
    CHECK(GameSettings::LANGUAGE == "de-de");
    CHECK(GameSettings::FULLSCREEN == true);
    CHECK(GameSettings::EVA_LADDER_JOINT_BREAK_VELOCITY == 123.5);
    CHECK(GameSettings::MISSION_VALIDATOR_MODE == Expansions::Missions::Editor::ValidatorMode::FullAuto);
    CHECK(GameSettings::ORBIT_WARP_MAXRATE_MODE == TimeWarp::MaxRailsRateMode::PeAltitude);
    CHECK(GameSettings::PITCH_DOWN->primary.ToString() == "T");
    CHECK(GameSettings::PITCH_DOWN->secondary.name == "ö");
    CHECK(GameSettings::PITCH_DOWN->secondary.code == KeyCode::None);
    CHECK(GameSettings::PITCH_DOWN->inputLockMask == 5);
    CHECK(GameSettings::PITCH_DOWN->switchState == InputBindingModes::Docking_Translation);
    CHECK(GameSettings::PITCH_DOWN->switchStateSecondary == InputBindingModes::Docking_Rotation);
    CHECK(GameSettings::SHOW_ANALYTICS_DIALOG_SettingExists == false);
    CHECK(GameSettings::collpasedPAWGroups == std::vector<std::string>{"A", "B"});  // InitializeDefaultValues -> SetCollapsedPawGroupNames
    CHECK(GameSettings::GetAxisIncrementalSpeedMultiplierValues() != nullptr);
    CHECK(GameSettings::GetLightPresetColors().size() == 5);
    CHECK(GameSettings::GetFireworkPresetColors().size() == 5);

    // older file versions: shadows projection reset (< 1.2.0) and vessel naming (< 1.3.0), settings rewritten via OnGameSettingsWritten
    struct W { int n = 0; void On() { ++n; } } w;
    events::GameEvents::OnGameSettingsWritten.Add(&w, &W::On);
    auto old = ConfigNode::Create();
    old->AddValue("SETTINGS_FILE_VERSION", "1.1.0");
    old->AddValue("SHADOWS_FLIGHT_PROJECTION", "0");
    old->AddValue("SHOW_VESSEL_NAMING_IN_FLIGHT", "False");
    old->AddValue("SHOW_ANALYTICS_DIALOG", "False");
    GameSettings::ParseCfg(old);
    CHECK(GameSettings::SHADOWS_FLIGHT_PROJECTION == 1);
    CHECK(GameSettings::SHADOWS_DEFAULT_PROJECTION == 1);
    CHECK(GameSettings::SHOW_VESSEL_NAMING_IN_FLIGHT == true);
    CHECK(GameSettings::SHOW_ANALYTICS_DIALOG_SettingExists == true);
    CHECK(GameSettings::SETTINGS_FILE_VERSION == "1.3.0");
    CHECK(GameSettings::SHOW_VERSION_WATERMARK == false);
    CHECK(w.n == 1);  // WriteCfg ran once
    // up to date file: no rewrite
    GameSettings::ParseCfg(cfg);
    CHECK(w.n == 1);
    events::GameEvents::OnGameSettingsWritten.Remove(&w, &W::On);

    // incompatible / missing
    auto bad = ConfigNode::Create();
    bad->AddValue("SETTINGS_FILE_VERSION", "0.9.0");
    CHECK(GameSettings::ParseCfg(bad) == GameSettings::ParseResult::Incompatible);
    auto none = ConfigNode::Create();
    CHECK(GameSettings::ParseCfg(none) == GameSettings::ParseResult::Incompatible);
    CHECK(GameSettings::ParseCfg(nullptr) == GameSettings::ParseResult::NoFile);
    // a malformed typed value throws out of ParseCfg (FormatException of int.Parse)
    auto fmt = ConfigNode::Create();
    fmt->AddValue("SETTINGS_FILE_VERSION", "1.3.0");
    fmt->AddValue("MAX_VESSELS_BUDGET", "many");
    CHECK_THROWS_AS(GameSettings::ParseCfg(fmt), dotnet::FormatException);
}

TEST_CASE("WP-50 GameSettings.WriteCfg -> ParseCfg round trip keeps bindings, strings, bools, ints") {
    InputFixture fixture;
    GameSettings::host = {};
    GameSettings::LANGUAGE = "fr-fr";
    GameSettings::CONSOLE_BUFFER_SIZE = 4096;
    GameSettings::LOG_INSTANT_FLUSH = true;
    GameSettings::SAS_HOLD->primary = KeyCodeExtended(KeyCode::G);
    GameSettings::AXIS_ROLL->primary.name = "stick";
    GameSettings::AXIS_ROLL->primary.sensitivity = 2.5f;
    GameSettings::AXIS_CUSTOM->operator[](1).plusKeyBinding->primary = KeyCodeExtended(KeyCode::F8);
    GameSettings::INPUT_DEVICES = std::make_shared<InputDevices>();
    const ConfigNode::NodePtr cfg = GameSettings::BuildCfg();
    const std::string text = cfg->SaveToString(std::string("KSP Game Settings"));
    GameSettings::SetDefaultValues();
    GameSettings::INPUT_DEVICES = std::make_shared<InputDevices>();
    const ConfigNode::NodePtr reloaded = ConfigNode::Parse(text);
    REQUIRE(reloaded);
    CHECK(GameSettings::ParseCfg(reloaded) == GameSettings::ParseResult::Ok);
    CHECK(GameSettings::LANGUAGE == "fr-fr");
    CHECK(GameSettings::CONSOLE_BUFFER_SIZE == 4096);
    CHECK(GameSettings::LOG_INSTANT_FLUSH == true);
    CHECK(GameSettings::SAS_HOLD->primary.ToString() == "G");
    CHECK(GameSettings::AXIS_ROLL->primary.name == "stick");
    CHECK(GameSettings::AXIS_ROLL->primary.idTag == "stick");
    CHECK(GameSettings::AXIS_ROLL->primary.sensitivity == 2.5f);
    CHECK((*GameSettings::AXIS_CUSTOM)[1].plusKeyBinding->primary.ToString() == "F8");
    // the whole config is a fixed point of write -> parse -> write
    const ConfigNode::NodePtr again = GameSettings::BuildCfg();
    {
        const std::string text2 = again->SaveToString(std::string("KSP Game Settings"));
        const auto l1 = dotnet::Split(text, "\n", false), l2 = dotnet::Split(text2, "\n", false);
        CHECK(l1.size() == l2.size());
        for (size_t i = 0; i < l1.size() && i < l2.size(); ++i) {
            if (l1[i].rfind("AXIS_INCREMENTAL_SPEED_MULTIPLIER_VALUES", 0) == 0) continue;  // null before ParseCfg, a List after it
            if (dotnet::Trim(l1[i]) != dotnet::Trim(l2[i])) {  // the first write keeps the trailing space of COLOR_LIGHT_PRESET_2.., Parse trims it
                CAPTURE(i);
                CHECK(l2[i] == l1[i]);
                break;
            }
        }
    }
}

TEST_CASE("WP-50 AxisBinding_Single.GetAxis / AxisBinding.GetAxis: 600 randomised cases equal the reference at 0 ULP") {
    InputFixture fixture;
    int n = 0;
    for (const auto& c : Ref()["axis"]) {
        AxisBinding_Single s("axisA", "axisA", c["inv"].get<bool>(), F(c["sens"]), F(c["dead"]), F(c["scale"]));
        s.preinvertAxis = c["pre"].get<bool>();
        s.neutralPoint = F(c["neutral"]);
        fixture.dev.axes["axisA"] = F(c["raw"]);
        fixture.dev.axes["axisB"] = F(c["raw2"]);
        CHECK(SameBits(s.GetAxis(), F(c["expect"])));
        CHECK((s.IsNeutral() == (F(c["expect"]) == 0.0f)));
        AxisBinding ab;
        ab.primary = s;
        ab.secondary = s;
        ab.secondary.idTag = "axisB";
        CHECK(SameBits(ab.GetAxis(), F(c["combined"])));
        CHECK(ab.deadzone() == F(c["dead"]));
        ++n;
    }
    CHECK(n == 600);
}

TEST_CASE("WP-50 AxisBinding_Single: None tag, switch state, input lock and neutral") {
    InputFixture fixture;
    fixture.dev.axes["a"] = 0.8f;
    AxisBinding_Single none;  // idTag None
    none.neutralPoint = -1.0f;
    CHECK(none.GetAxis() == -1.0f);
    AxisBinding_Single s("a", "a", false);
    s.inputLockMask = static_cast<uint64_t>(ControlTypes::PITCH);
    CHECK(s.GetAxis() > 0.0f);
    InputLockManager::SetControlLock(ControlTypes::PITCH, "p");
    CHECK(s.GetAxis() == 0.0f);  // neutralPoint
    s.neutralPoint = 0.25f;
    CHECK(s.GetAxis() == 0.25f);
    InputLockManager::RemoveControlLock("p");
    // CompareSwitchState: only in the flight scene with a FlightUIModeController
    s.switchState = InputBindingModes::Staging;
    HighLogic::LoadedSceneIsFlight = true;
    FlightUIModeControllerSeam::present = true;
    FlightUIModeControllerSeam::mode = FlightUIMode::DOCKING;
    InputBinding::linRotState = false;
    CHECK(s.GetAxis() == 0.25f);  // Docking_Translation & Staging == 0
    s.switchState = InputBindingModes::Docking_Translation;
    CHECK(s.GetAxis() > 0.25f);
    InputBinding::linRotState = true;  // Docking_Rotation
    CHECK(s.GetAxis() == 0.25f);
    FlightUIModeControllerSeam::mode = FlightUIMode::MAPMODE;  // anything but DOCKING is Staging
    s.switchState = InputBindingModes::Staging;
    CHECK(s.GetAxis() > 0.25f);
    FlightUIModeControllerSeam::present = false;  // no controller: always true
    s.switchState = InputBindingModes::Docking_Rotation;
    CHECK(s.CompareSwitchState(InputBindingModes::Docking_Translation));
}

TEST_CASE("WP-50 AxisBinding_Single / KeyBinding Save and Load: the settings.cfg text and the Load branches") {
    InputFixture fixture;
    GameSettings::INPUT_DEVICES = std::make_shared<InputDevices>();
    // Save: field order and number text
    AxisBinding_Single s("joy0.1", "Pad", true, 2.5f, 0.1f, 1.5f);
    s.axisIdx = 3;
    s.inputLockMask = 7;
    s.switchState = InputBindingModes::RotationModes;
    ConfigNode n;
    s.Save(n);
    const char* expectKeys[] = {"name", "axis", "inv", "sensitivity", "deadzone", "scale", "group", "modeMask"};
    const char* expectVals[] = {"Pad", "3", "True", "2.5", "0.100000001", "1.5", "7", "5"};  // ConfigNode.AddValue(float) is "G9"
    REQUIRE(n.values().Count() == 8);
    for (int i = 0; i < 8; ++i) {
        CHECK(n.values()[i].name == expectKeys[i]);
        CHECK(n.values()[i].value == expectVals[i]);
    }
    // Load: "axis" present -> the device table decides; unknown device -> None / Not Found
    AxisBinding_Single t;
    t.Load(n);
    CHECK(t.idTag == "None");
    CHECK(t.title == "Not Found");
    CHECK(t.axisIdx == 3);
    CHECK(t.inverted == true);
    CHECK(t.sensitivity == 2.5f);
    CHECK(t.deadzone == 0.1f);
    CHECK(t.scale == 1.5f);
    CHECK(t.inputLockMask == 7);
    CHECK(t.switchState == InputBindingModes::RotationModes);
    fixture.dev.joysticks = {"Gamepad X", "Pad"};
    GameSettings::INPUT_DEVICES = std::make_shared<InputDevices>();
    AxisBinding_Single u;
    u.Load(n);
    CHECK(u.deviceIdx == 1);
    CHECK(u.idTag == "joy1.3");
    CHECK(u.title == "PadAxis 3");
    // the legacy "id = joyN.M" form and no axis at all
    ConfigNode legacy;
    legacy.AddValue("name", "Old");
    legacy.AddValue("id", "joy2.5");
    AxisBinding_Single v;
    v.Load(legacy);
    CHECK(v.axisIdx == 5);
    CHECK(v.deviceIdx == -1);  // overwritten by GetDeviceIndex("Old") == -1 -> "Not Found"
    CHECK(v.idTag == "None");
    ConfigNode plain;
    plain.AddValue("inv", "False");
    AxisBinding_Single w;
    w.Load(plain);
    CHECK(w.name == "NULL");
    CHECK(w.idTag == "NULL");
    CHECK(w.title == "NULL");
    CHECK(w.axisIdx == -1);
    ConfigNode badId;
    badId.AddValue("id", "joy7");
    AxisBinding_Single x;
    CHECK_THROWS(x.Load(badId));
    ConfigNode nonjoy;
    nonjoy.AddValue("id", "Mouse X");
    AxisBinding_Single y;
    y.Load(nonjoy);
    CHECK(y.axisIdx == -1);
    CHECK(y.idTag == "NULL");  // name defaults to "NULL" and the final else assigns idTag = name
    // switchState by name (legacy) and modeMask winning over it
    ConfigNode sw;
    sw.AddValue("switchState", "Docking_Rotation");
    InputBinding b;
    b.Load(sw);
    CHECK(b.switchState == InputBindingModes::Docking_Rotation);
    sw.AddValue("modeMask", "1");
    b.Load(sw);
    CHECK(b.switchState == InputBindingModes::Staging);
    ConfigNode badSw;
    badSw.AddValue("switchState", "Nope");
    CHECK_THROWS_AS(b.Load(badSw), ksp::ArgumentException);

    // KeyBinding
    KeyBinding k(KeyCode::W, KeyCode::UpArrow, InputBindingModes::Staging, InputBindingModes::Any, ControlTypes::EVA_INPUT);
    ConfigNode kn;
    k.Save(kn);
    const char* kk[] = {"primary", "secondary", "group", "modeMask", "modeMaskSec"};
    const char* kv[] = {"W", "UpArrow", "268435456", "1", "-1"};
    REQUIRE(kn.values().Count() == 5);
    for (int i = 0; i < 5; ++i) {
        CHECK(kn.values()[i].name == kk[i]);
        CHECK(kn.values()[i].value == kv[i]);
    }
    KeyBinding k2;
    k2.Load(kn);
    CHECK(k2.name() == "W");
    CHECK(k2.inputLockMask == 268435456ULL);
    CHECK(k2.switchState == InputBindingModes::Staging);
    CHECK(k2.switchStateSecondary == InputBindingModes::Any);
    // a binding with an empty primary names itself by the secondary
    KeyBinding e;
    e.secondary = KeyCodeExtended(KeyCode::B);
    CHECK(e.name() == "B");
    CHECK(KeyBinding().name() == "None");
    // missing keys: primary / secondary from null -> None; group / modeMaskSec keep their values
    ConfigNode empty;
    KeyBinding k3(KeyCode::A, KeyCode::B, InputBindingModes::Staging, InputBindingModes::Docking_Rotation, ControlTypes::SAS);
    k3.Load(empty);
    CHECK(k3.primary.isNone());
    CHECK(k3.secondary.isNone());
    CHECK(k3.inputLockMask == static_cast<uint64_t>(ControlTypes::SAS));
    CHECK(k3.switchStateSecondary == InputBindingModes::Docking_Rotation);
    // ulong.Parse errors
    ConfigNode ov;
    ov.AddValue("group", "99999999999999999999");
    CHECK_THROWS_AS(k3.Load(ov), dotnet::OverflowException);
    ConfigNode fe;
    fe.AddValue("group", "x");
    CHECK_THROWS_AS(k3.Load(fe), dotnet::FormatException);
}

TEST_CASE("WP-50 AxisKeyBinding / AxisKeyBindingList: serialisation, MemberwiseClone aliasing, Load limits") {
    InputFixture fixture;
    AxisKeyBindingList list(3);
    (*list.operator[](0).plusKeyBinding).primary = KeyCodeExtended(KeyCode::F1);
    list[2].minusKeyBinding->secondary = KeyCodeExtended(KeyCode::F2);
    list[1].axisBinding->primary.name = "ax";
    ConfigNode n;
    list.Save(n);
    REQUIRE(n.nodes().Count() == 3);
    CHECK(n.nodes()[0]->name == "AXIS_KEY_BINDING");
    REQUIRE(n.nodes()[0]->nodes().Count() == 3);
    CHECK(n.nodes()[0]->nodes()[0]->name == "AXIS_BINDING");
    CHECK(n.nodes()[0]->nodes()[1]->name == "PLUS_KEY_BINDING");
    CHECK(n.nodes()[0]->nodes()[2]->name == "MINUS_KEY_BINDING");
    CHECK(Str(n.nodes()[0]->nodes()[1]->GetValue("primary")) == "F1");
    // Load into a shorter list takes only the first nodes; a longer source is ignored beyond the length
    AxisKeyBindingList shorter(2);
    shorter.Load(n);
    CHECK(shorter[0].plusKeyBinding->primary.ToString() == "F1");
    CHECK(shorter[1].axisBinding->primary.name == "ax");
    AxisKeyBindingList longer(5);
    longer.Load(n);
    CHECK(longer[2].minusKeyBinding->secondary.ToString() == "F2");
    CHECK(longer[3].plusKeyBinding->primary.isNone());
    // Clone: the list is copied element by element, each element MemberwiseClone: the bindings are shared with the original
    AxisKeyBindingList copy = list.Clone();
    CHECK(copy[0].plusKeyBinding == list[0].plusKeyBinding);
    copy[0].plusKeyBinding->primary = KeyCodeExtended(KeyCode::F9);
    CHECK(list[0].plusKeyBinding->primary.ToString() == "F9");
    // AxisBinding.Clone semantic: C++ copy is deep for AxisBinding (primary / secondary are values)
    AxisBinding a;
    a.primary.name = "n1";
    AxisBinding b = a;
    b.primary.name = "n2";
    CHECK(a.primary.name == "n1");
    // Load of a node without the sub-nodes changes nothing
    AxisBinding c;
    c.primary.name = "keep";
    ConfigNode emptyNode;
    c.Load(emptyNode);
    CHECK(c.primary.name == "keep");
}

TEST_CASE("WP-50 InputDevices: name table order, empty names, Save / Load") {
    InputFixture fixture;
    fixture.dev.joysticks = {"Pad A", "", "  Pad B ", "Pad A"};
    InputDevices d;
    CHECK(d.GetDeviceListLength() == 3);
    CHECK(d.GetDeviceIndex("Pad A") == 0);  // the loop runs from the last device down, so the lowest index wins
    CHECK(d.GetDeviceIndex("Pad B") == 2);
    CHECK(d.GetDeviceIndex("Joystick 1") == 1);
    CHECK(d.GetDeviceIndex("none") == -1);
    ConfigNode n;
    d.Save(n);
    REQUIRE(n.values().Count() == 3);
    CHECK(n.values()[0].name == "Pad A");   // Dictionary insertion order: index 3's name was added first, then overwritten in place
    CHECK(n.values()[0].value == "0");
    CHECK(n.values()[1].name == "Pad B");
    CHECK(n.values()[1].value == "2");
    CHECK(n.values()[2].name == "Joystick 1");
    CHECK(n.values()[2].value == "1");
    CHECK(InputDevices::TrimDeviceName("  x \t") == "x");
    // Load: duplicates throw (Dictionary.Add) and the table is rebuilt from the live joysticks afterwards
    ConfigNode dup;
    dup.AddValue("A", "1");
    dup.AddValue("A ", "2");
    CHECK_THROWS_AS(d.Load(dup), ksp::ArgumentException);
    ConfigNode ok;
    ok.AddValue("Old pad", "4");
    d.Load(ok);
    CHECK(d.GetDeviceIndex("Old pad") == -1);
    CHECK(d.GetDeviceIndex("Pad B") == 2);
    // not playing: the constructor does not touch the devices
    InputEnvironment::applicationIsPlaying = false;
    InputDevices e;
    CHECK(e.GetDeviceListLength() == 0);
}

TEST_CASE("WP-50 ExtendedInput: typed-character bindings, frame thresholds, DetectKeyDown") {
    InputFixture fixture;
    Time::deltaTime = 0.01f;
    ExtendedInput ei;
    KeyBinding semi;
    semi.primary = KeyCodeExtended("ö");
    CHECK(semi.primary.code == KeyCode::None);
    CHECK(semi.primary.name == "ö");
    // nothing typed
    ei.LateUpdate();
    CHECK_FALSE(semi.GetKey());
    // the character is typed this frame (upper-cased by ToUpper): GetKey and GetKeyDown, then LateUpdate moves it to LastInputString
    fixture.dev.typed = u"ö";
    ei.LateUpdate();
    CHECK(semi.GetKey());
    CHECK(semi.GetKeyDown());   // IsPressed && !WasPressed
    CHECK_FALSE(semi.GetKeyUp());
    fixture.dev.typed = u"ö";
    ei.LateUpdate();            // Last = Current ("Ö"), Current = "Ö" again
    CHECK(semi.GetKey());
    CHECK_FALSE(semi.GetKeyDown());
    // the key is released: empty inputString, but EmtpyFrames (0) <= FrameThreshold (0.48 as the string changed -> 0.02 as it did not): the
    // old string is kept for up to that many seconds
    fixture.dev.typed = u"";
    Time::deltaTime = 0.01f;
    ei.LateUpdate();            // Last = "Ö"; empty and 0 <= 0.02: EmtpyFrames = 0.01, return (Current stays)
    CHECK(semi.GetKey());
    ei.LateUpdate();            // 0.01 <= 0.02: EmtpyFrames = 0.02
    ei.LateUpdate();            // 0.02 <= 0.02: EmtpyFrames = 0.03
    CHECK(semi.GetKey());
    ei.LateUpdate();            // 0.03 > 0.02: Current = "" (ToUpper of empty), threshold 0.02 (equal to Last? no: Last = "Ö")
    CHECK_FALSE(semi.GetKey());
    CHECK(semi.GetKeyUp());     // WasPressed (Last still "Ö") && !IsPressed
    // a KeyCode binding combines with the typed path through Input.GetKey
    KeyBinding w(KeyCode::W);
    fixture.dev.held.insert("W");
    CHECK(w.GetKey());
    fixture.dev.down.insert("W");
    CHECK(w.GetKeyDown());
    // DetectKeyDown: key codes first, then the typed character
    ExtendedInput::Reset();
    ScriptedDevice& d = fixture.dev;
    d.held.clear();
    d.down.clear();
    d.down.insert("A");
    KeyCodeExtended k;
    std::vector<KeyCode> candidates = {KeyCode::Z, KeyCode::A};
    CHECK(ExtendedInput::DetectKeyDown(&candidates, k));
    CHECK(k.code == KeyCode::A);
    CHECK(ExtendedInput::LastKeyDetected == KeyCode::A);
    CHECK_FALSE(ExtendedInput::DetectKeyDown(nullptr, k));
    CHECK(k.isNone());
    d.down.clear();
    d.held.insert("A");  // the last detected key is still held: typed characters are ignored
    ExtendedInput::CurrentInputString = u"ß";
    CHECK_FALSE(ExtendedInput::DetectKeyDown(&candidates, k));
    d.held.clear();
    CHECK(ExtendedInput::DetectKeyDown(&candidates, k));  // new typed char, empty Last -> true
    CHECK(k.name == "\xc3\x9f");
    ExtendedInput::LastInputString = u"ß";
    ExtendedInput::CurrentInputString = u"xß";
    CHECK_FALSE(ExtendedInput::DetectKeyDown(&candidates, k));  // same last char as before: name set but false
    CHECK(k.name == "\xc3\x9f");
    ExtendedInput::CurrentInputString = u"x\n";
    CHECK_FALSE(ExtendedInput::DetectKeyDown(&candidates, k));  // newline / backspace are not characters
}

TEST_CASE("WP-50 KeyBinding double taps use Time.realtimeSinceStartup and the frame count") {
    InputFixture fixture;
    KeyBinding k(KeyCode::Space);
    fixture.dev.down.insert("Space");
    Time::frameCount = 10;
    InputEnvironment::realtimeSinceStartup = 1.0f;
    CHECK_FALSE(k.GetDoubleTapDown());  // first press: lastKeyDownTime = 0, 1.0 < 0.2 false; records time 1.0
    Time::frameCount = 11;
    InputEnvironment::realtimeSinceStartup = 1.1f;
    CHECK(k.GetDoubleTapDown());        // 1.1 < 1.0 + 0.2f
    Time::frameCount = 12;
    InputEnvironment::realtimeSinceStartup = 1.5f;
    CHECK_FALSE(k.GetDoubleTapDown());  // 1.5 < 1.1 + 0.2 false
    // same frame: the time stamp is not refreshed
    Time::frameCount = 12;
    InputEnvironment::realtimeSinceStartup = 1.55f;
    CHECK(k.GetDoubleTapDown());        // 1.55 < 1.5 + 0.2 (lastKeyDownFrameCount == frameCount, so the stamp stays 1.5)
}

TEST_CASE("WP-50 FlightInputHandler.FixedUpdate: 360 scripted frames of the shipped Qwerty bindings equal the reference at 0 ULP") {
    InputFixture fixture;
    const json& r = Ref()["flight"];
    GameSettings::layoutConfigNodes.Clear();
    GameSettings::ParseLayoutsCfg({LayoutDir() + "Qwerty.cfg"});
    GameSettings::LoadLayoutKeyBindings("Qwerty.cfg");
    GameSettings::INPUT_DEVICES = std::make_shared<InputDevices>();
    for (auto it = r["axis_cfg"].begin(); it != r["axis_cfg"].end(); ++it) {
        const SettingsField* f = FieldByName(it.key());
        REQUIRE(f != nullptr);
        ConfigNode n;
        ConfigNode* p = n.AddNode("PRIMARY");
        p->AddValue("name", it.value()["name"].get<std::string>());
        p->AddValue("inv", it.value()["inv"].get<bool>());
        p->AddValue("sensitivity", F(it.value()["sens"]));
        p->AddValue("deadzone", F(it.value()["dz"]));
        p->AddValue("scale", F(it.value()["scale"]));
        p->AddValue("group", "0");
        (*static_cast<std::shared_ptr<AxisBinding>*>(f->address))->Load(n);
    }
    Vessel active, other;
    FlightRegistry::VesselsLoaded = {&active, &other};
    FlightRegistry::ActiveVessel = &active;
    FlightRegistry::ready = true;
    HighLogic::LoadedSceneIsFlight = true;
    FlightDriver::pause = false;
    int feeds = 0;
    FlightInputHandler::host = {};
    FlightInputHandler::host.feedInputFeed = [&](Vessel*) { ++feeds; };
    FlightInputHandler h;
    h.Awake();
    h.Start();
    h.refocusThrottle = true;

    const char* names[] = {"mainThrottle", "roll", "yaw", "pitch", "rollTrim", "yawTrim", "pitchTrim", "wheelSteer", "wheelSteerTrim",
                           "wheelThrottle", "wheelThrottleTrim", "X", "Y", "Z"};
    int frame = 0;
    for (size_t i = 0; i < r["inputs"].size(); ++i) {
        const json& in = r["inputs"][i];
        const json& ex = r["states"][i];
        fixture.dev.held.clear();
        fixture.dev.down.clear();
        for (const auto& k : in["pressed"]) fixture.dev.held.insert(k.get<std::string>());
        for (const auto& k : in["keydown"]) fixture.dev.down.insert(k.get<std::string>());
        for (auto a = in["axes"].begin(); a != in["axes"].end(); ++a) fixture.dev.axes[a.key()] = F(a.value());
        Time::deltaTime = F(in["dt"]);
        InputLockManager::Reset();
        const uint64_t mask = U64(in["lock"]);
        if (mask != 0) InputLockManager::SetControlLock(static_cast<ControlTypes>(mask), "wp50");
        h.precisionMode = in["precision"].get<bool>();
        if (in["refocus"].get<bool>()) h.OnApplicationFocus(true);
        h.FixedUpdate();
        const FlightCtrlState& s = FlightInputHandler::state;
        const float got[] = {s.mainThrottle, s.roll, s.yaw, s.pitch, s.rollTrim, s.yawTrim, s.pitchTrim, s.wheelSteer, s.wheelSteerTrim,
                             s.wheelThrottle, s.wheelThrottleTrim, s.X, s.Y, s.Z};
        for (int k = 0; k < 14; ++k) {
            CAPTURE(frame);
            CAPTURE(names[k]);
            CHECK(SameBits(got[k], F(ex[names[k]])));
        }
        // the active vessel got the state, the other loaded vessel only its trims and zero translation
        FlightCtrlState vs;
        active.GetControlState(vs);
        CHECK(SameBits(vs.pitch, s.pitch));
        CHECK(SameBits(vs.mainThrottle, s.mainThrottle));
        CHECK(SameBits(other.ctrlState.pitch, other.ctrlState.pitchTrim));
        CHECK(other.ctrlState.X == 0.0f);
        ++frame;
    }
    CHECK(feeds == 2 * frame);
    CHECK(frame == 360);
    h.OnDestroy();
    CHECK(FlightInputHandler::fetch == nullptr);
    FlightRegistry::VesselsLoaded.clear();
    FlightRegistry::ActiveVessel = nullptr;
    FlightRegistry::ready = false;
    FlightInputHandler::host = {};
    SpaceNavigator::ResetStatics();
}

TEST_CASE("WP-50 FlightInputHandler: EVA / pause / packed vessels, SPACENAV, raw axis callbacks, static helpers") {
    InputFixture fixture;
    GameSettings::layoutConfigNodes.Clear();
    GameSettings::ParseLayoutsCfg({LayoutDir() + "Qwerty.cfg"});
    GameSettings::LoadLayoutKeyBindings("Qwerty.cfg");
    Vessel active, packed;
    packed.packed = true;
    FlightRegistry::VesselsLoaded = {&active, &packed};
    FlightRegistry::ActiveVessel = &active;
    FlightRegistry::ready = true;
    HighLogic::LoadedSceneIsFlight = true;
    FlightDriver::pause = false;
    FlightInputHandler h;
    h.Awake();
    Time::deltaTime = 0.02f;
    // not in the flight scene / not ready: nothing happens
    FlightInputHandler::state.pitch = 0.75f;
    HighLogic::LoadedSceneIsFlight = false;
    h.FixedUpdate();
    CHECK(FlightInputHandler::state.pitch == 0.75f);
    HighLogic::LoadedSceneIsFlight = true;
    // paused: pitch / yaw / roll / translation zero, the vessels still get the state
    FlightDriver::pause = true;
    h.FixedUpdate();
    CHECK(FlightInputHandler::state.pitch == 0.0f);
    FlightDriver::pause = false;
    // the raw-axis callback sees the state before the trim / throttle post-processing
    float seenThrottle = -5.0f;
    struct Spy { float* t; void On(FlightCtrlState& s) { *t = s.mainThrottle; } } spy{&seenThrottle};
    FlightInputHandler::OnRawAxisInput = FlightInputHandler::OnRawAxisInput + FlightInputCallback::Bind(&spy, &Spy::On);
    FlightInputHandler::state.mainThrottle = 0.5f;
    h.refocusThrottle = false;
    h.FixedUpdate();
    // state.mainThrottle 0.5 -> throttle 0; the first frame with an unbound throttle axis (neutral -1, lastAxisThrottle 0, no focus)
    // jumps to the axis value -1 (|axis - last| > 0.25); that is what OnRawAxisInput sees, then (-1 + 1) * 0.5
    CHECK(seenThrottle == -1.0f);
    CHECK(FlightInputHandler::state.mainThrottle == 0.0f);
    FlightInputHandler::OnRawAxisInput = FlightInputHandler::OnRawAxisInput - FlightInputCallback::Bind(&spy, &Spy::On);
    // THROTTLE locked: the throttle value is not touched by keys but still re-mapped
    fixture.dev.held.insert("LeftShift");
    InputLockManager::SetControlLock(ControlTypes::THROTTLE, "t");
    FlightInputHandler::state.mainThrottle = 0.25f;
    h.FixedUpdate();
    CHECK(FlightInputHandler::state.mainThrottle == 0.25f);
    InputLockManager::RemoveControlLock("t");
    // THROTTLE_UP (LeftShift): +1 * dt per frame
    FlightInputHandler::state.mainThrottle = 0.0f;
    h.FixedUpdate();
    {
        const float t = f32store(-1.0 + 1.0 * fx(0.02f));
        CHECK(SameBits(FlightInputHandler::state.mainThrottle, f32store((fx(t) + 1.0) * fx(0.5f))));
    }
    // static helpers
    FlightInputHandler::SetLaunchCtrlState();
    CHECK(FlightInputHandler::state.mainThrottle == GameSettings::PRELAUNCH_DEFAULT_THROTTLE);
    CHECK(h.refocusThrottle);
    CHECK(h.throttle == -1.0f);
    CHECK(FlightInputHandler::RCSLock());
    h.rcslock = false;
    CHECK_FALSE(FlightInputHandler::RCSLock());
    h.OnDestroy();
    CHECK_FALSE(FlightInputHandler::RCSLock());
    FlightRegistry::VesselsLoaded.clear();
    FlightRegistry::ActiveVessel = nullptr;
    FlightRegistry::ready = false;
    SpaceNavigator::ResetStatics();
}
