// WP-50 unit tests, part 3: Mouse, SpaceNavigator, InputSettings listen session, TrackIR.Settings, axis speed multipliers.
#include "input_test_support.hpp"

using namespace ksp;
using json = nlohmann::json;
using namespace wp50test;

TEST_CASE("WP-50 Mouse: 400 frames of the tap / double-tap / drag state machine equal the reference (Python generator = Unity coroutine)") {
    InputFixture fixture;
    const json& r = Ref()["mouse"];
    Mouse::ResetStatics();
    GameSettings::DOUBLECLICK_MOUSESPEED = F(r["speed"]);
    InputEnvironment::screenHeight = 1080;
    int taps = 0, doubles = 0, drags = 0, clears = 0;
    int frame = 0;
    for (const auto& f : r["frames"]) {
        ScriptedDevice& d = fixture.dev;
        InputEnvironment::realtimeSinceStartup = F(f["t"]);
        d.mouseDown[0] = f["down"].get<bool>();
        d.mouseUp[0] = f["up"].get<bool>();
        d.mouseHeld[0] = f["held"].get<bool>();
        d.mouse = Vector3f(F(f["mx"]), F(f["my"]), 0.0f);
        if (f["clear"].get<bool>()) Mouse::Left.ClearMouseState();
        Mouse::Update();
        Mouse::ResumeCoroutines();
        CAPTURE(frame);
        CHECK(Mouse::Left.GetClick() == f["tap"].get<bool>());
        CHECK(Mouse::Left.GetDoubleClick(false) == f["doubleTap"].get<bool>());
        CHECK(Mouse::Left.isTapStarted() == f["isTapStarted"].get<bool>());
        CHECK(Mouse::Left.GetButtonDown() == f["bDown"].get<bool>());
        CHECK(Mouse::Left.GetButtonUp() == f["bUp"].get<bool>());
        CHECK(Mouse::Left.GetButton() == f["bButton"].get<bool>());
        CHECK(Mouse::Left.GetDoubleClick(true) ==
              ((InputEnvironment::realtimeSinceStartup < F(f["dclick"])) || f["doubleTap"].get<bool>()));
        CHECK(SameBits(Mouse::Left.GetDragDelta().x, F(f["dragX"])));
        CHECK(SameBits(Mouse::Left.GetDragDelta().y, F(f["dragY"])));
        CHECK(Mouse::Left.WasDragging() == f["wasDragging"].get<bool>());
        CHECK(SameBits(Mouse::screenPos.x, F(f["screenX"])));
        CHECK(SameBits(Mouse::screenPos.y, F(f["screenY"])));
        CHECK(SameBits(Mouse::delta.x, F(f["deltaX"])));
        CHECK(SameBits(Mouse::delta.y, F(f["deltaY"])));
        CHECK(Mouse::IsMoving() == f["isMoving"].get<bool>());
        taps += f["tap"].get<bool>();
        doubles += f["doubleTap"].get<bool>();
        drags += f["wasDragging"].get<bool>();
        clears += f["clear"].get<bool>();
        ++frame;
    }
    // the scripted run exercises every state
    CHECK(taps > 5);
    CHECK(doubles > 3);
    CHECK(drags > 5);
    CHECK(clears > 3);
    Mouse::ResetStatics();
}

TEST_CASE("WP-50 Mouse: button masks and CheckButtons") {
    InputFixture fixture;
    fixture.dev.mouseHeld[0] = true;
    fixture.dev.mouseHeld[2] = true;
    fixture.dev.mouseHeld[4] = true;
    fixture.dev.mouseDown[1] = true;
    fixture.dev.mouseUp[3] = true;
    const auto held = Mouse::GetAllMouseButtons();
    CHECK(held == (Mouse::Buttons::Left | Mouse::Buttons::Middle | Mouse::Buttons::Btn5));
    CHECK(Mouse::GetAllMouseButtonsDown() == Mouse::Buttons::Right);
    CHECK(Mouse::GetAllMouseButtonsUp() == Mouse::Buttons::Btn4);
    CHECK(Mouse::CheckButtons(held, Mouse::Buttons::Left | Mouse::Buttons::Middle));
    CHECK_FALSE(Mouse::CheckButtons(held, Mouse::Buttons::Left | Mouse::Buttons::Right));
    CHECK(Mouse::CheckButtons(held, Mouse::Buttons::Left | Mouse::Buttons::Right, false));
    CHECK_FALSE(Mouse::CheckButtons(held, Mouse::Buttons::Right, false));
    CHECK(Mouse::CheckButtons(held, Mouse::Buttons::None));
    CHECK_FALSE(Mouse::CheckButtons(held, Mouse::Buttons::None, false));
    // Update() asks the host for the part under the cursor
    Mouse::ResetStatics();
    Mouse::hoveredPartProvider = [] { return reinterpret_cast<Part*>(0x1234); };
    Mouse::Update();
    CHECK(Mouse::HoveredPart == reinterpret_cast<Part*>(0x1234));
    Mouse::ResetStatics();
}

TEST_CASE("WP-50 SpaceNavigator: QuaternionExtensions Pitch / Yaw / Roll and the sensor scaling equal the reference at 0 ULP") {
    InputFixture fixture;
    int defined = 0, undefined = 0;
    for (const auto& c : Ref()["quat"]) {
        const Quaternionf q(F(c["q"][0]), F(c["q"][1]), F(c["q"][2]), F(c["q"][3]));
        if (c["pitch"].is_null()) {
            CHECK(std::isnan(QuaternionPitch(q)));
        } else {
            CHECK(SameBits(QuaternionPitch(q), F(c["pitch"])));
            ++defined;
        }
        if (c["yaw"].is_null()) {
            CHECK(std::isnan(QuaternionYaw(q)));
            ++undefined;
        } else {
            CHECK(SameBits(QuaternionYaw(q), F(c["yaw"])));
        }
        if (c["roll"].is_null()) CHECK(std::isnan(QuaternionRoll(q)));
        else CHECK(SameBits(QuaternionRoll(q), F(c["roll"])));
    }
    CHECK(defined > 300);

    struct Sensor final : ISpaceNavSensor {
        SpaceNavSensorReading r;
        SpaceNavSensorReading Read() const override { return r; }
    };
    auto sensor = std::make_shared<Sensor>();
    SpaceNavigator::ResetStatics();
    SpaceNavigatorWindows::sensorFactory = [sensor] { return sensor; };
    CHECK(SpaceNavigator::Instance() == SpaceNavigatorWindows::SubInstance());
    CHECK(SpaceNavigator::Available());
    InputEnvironment::applicationIsPlaying = false;  // the lock flags only act outside play mode
    for (const auto& c : Ref()["spacenav"]) {
        SpaceNavigator* nav = SpaceNavigator::Instance();
        nav->TransSens[static_cast<size_t>(nav->CurrentGear)] = F(c["num"]);
        nav->RotSens = F(c["rnum"]);
        SpaceNavigator::SetLockTranslationX(c["locks"][0].get<bool>());
        SpaceNavigator::SetLockTranslationY(c["locks"][1].get<bool>());
        SpaceNavigator::SetLockTranslationZ(c["locks"][2].get<bool>());
        SpaceNavigator::SetLockTranslationAll(c["locks"][3].get<bool>());
        SpaceNavigator::SetLockRotationX(c["rlocks"][0].get<bool>());
        SpaceNavigator::SetLockRotationY(c["rlocks"][1].get<bool>());
        SpaceNavigator::SetLockRotationZ(c["rlocks"][2].get<bool>());
        SpaceNavigator::SetLockRotationAll(c["rlocks"][3].get<bool>());
        sensor->r.translationX = c["t"][0].get<double>();
        sensor->r.translationY = c["t"][1].get<double>();
        sensor->r.translationZ = c["t"][2].get<double>();
        sensor->r.rotationX = c["rot"][0].get<double>();
        sensor->r.rotationY = c["rot"][1].get<double>();
        sensor->r.rotationZ = c["rot"][2].get<double>();
        sensor->r.rotationAngle = c["angle"].get<double>();
        const Vector3f t = SpaceNavigator::Translation();
        CHECK(SameBits(t.x, F(c["expect"][0])));
        CHECK(SameBits(t.y, F(c["expect"][1])));
        CHECK(SameBits(t.z, F(c["expect"][2])));
        const Quaternionf q = SpaceNavigator::Rotation();
        const Quaternionf e = UnityNative::AngleAxis(F(c["expect_angle"]), Vector3f(F(c["expect_axis"][0]), F(c["expect_axis"][1]), F(c["expect_axis"][2])));
        CHECK(SameBits(q.x, e.x));
        CHECK(SameBits(q.y, e.y));
        CHECK(SameBits(q.z, e.z));
        CHECK(SameBits(q.w, e.w));
    }
    // play mode: the locks are ignored and PlayTransSens (1.0) is used
    InputEnvironment::applicationIsPlaying = true;
    SpaceNavigator::SetLockTranslationAll(true);
    sensor->r.translationX = 100.0;
    sensor->r.translationZ = 50.0;
    const Vector3f p = SpaceNavigator::Translation();
    CHECK(p.x == f32store(fx(f32store(100.0 * 1.0)) * fx(0.0001f)));
    CHECK(SameBits(p.z, f32store(fx(f32store(-50.0)) * 1.0 * fx(0.0001f))));
    SpaceNavigator::SetTranslationSensitivity(2.0f);
    CHECK(SpaceNavigator::Instance()->PlayTransSens == 2.0f);
    // no sensor: zero translation, identity rotation
    SpaceNavigator::ResetStatics();
    SpaceNavigatorWindows::sensorFactory = nullptr;
    SpaceNavigatorNoDevice* nd = dynamic_cast<SpaceNavigatorNoDevice*>(SpaceNavigator::Instance());
    REQUIRE(nd != nullptr);
    CHECK_FALSE(SpaceNavigator::Available());
    CHECK(nd->GetTranslation().x == 0.0f);
    const Quaternionf id = nd->GetRotation();
    CHECK(id.w == 1.0f);
    CHECK(id.x == 0.0f);
    SpaceNavigator::ResetStatics();
}

TEST_CASE("WP-50 SpaceNavigator: PlayerPrefs ReadSettings / WriteSettings round trip and the defaults") {
    struct Prefs final : IPlayerPrefs {
        std::map<std::string, float> f;
        std::map<std::string, int> i;
        float GetFloat(const std::string& k, float d) const override { auto it = f.find(k); return it == f.end() ? d : it->second; }
        int GetInt(const std::string& k, int d) const override { auto it = i.find(k); return it == i.end() ? d : it->second; }
        void SetFloat(const std::string& k, float v) override { f[k] = v; }
        void SetInt(const std::string& k, int v) override { i[k] = v; }
    } prefs;
    SpaceNavigator::ResetStatics();
    SpaceNavigatorNoDevice* nd = SpaceNavigatorNoDevice::SubInstance();
    CHECK(nd->PlayTransSens == 1.0f);
    CHECK(nd->TransSens == std::vector<float>{50.0f, 1.0f, 0.05f});
    nd->ReadSettings(prefs);  // all defaults
    CHECK(nd->TransSens == std::vector<float>{50.0f, 1.0f, 0.05f});
    CHECK(nd->RotSens == 1.0f);
    CHECK(nd->RotSensMax == 5.0f);
    nd->TransSens[2] = 0.5f;
    nd->RotSens = 2.0f;
    SpaceNavigator::SetLockRotationY(true);
    nd->WriteSettings(prefs);
    CHECK(prefs.f["Translation sensitivity2"] == 0.5f);
    CHECK(prefs.f["Translation sensitivity minimum0"] == 1.0f);
    CHECK(prefs.f["Translation sensitivity maximum1"] == 10.0f);
    CHECK(prefs.f["Rotation sensitivity"] == 2.0f);
    CHECK(prefs.i["Rotation lock Y"] == 1);
    CHECK(prefs.i["Rotation lock X"] == 0);
    SpaceNavigator::SetLockRotationY(false);
    nd->TransSens[2] = 7.0f;
    nd->ReadSettings(prefs);
    CHECK(nd->TransSens[2] == 0.5f);
    InputEnvironment::applicationIsPlaying = false;
    CHECK(SpaceNavigator::LockRotationY());
    InputEnvironment::applicationIsPlaying = true;
    // RotationInLocalCoordSys(identity) is the sensor rotation itself
    const Quaternionf q = SpaceNavigator::RotationInLocalCoordSys(Quaternionf::identity());
    CHECK(q.w == 1.0f);
    SpaceNavigator::ResetStatics();
}

TEST_CASE("WP-50 InputSettings: valid keys equal the reference, the listen session edits a copy and applies it through ConfigNode") {
    InputFixture fixture;
    const std::vector<KeyCode>& keys = InputSettings::keyValues();
    REQUIRE(keys.size() == Ref()["valid_keys"].size());
    for (size_t i = 0; i < keys.size(); ++i) CHECK(static_cast<int>(keys[i]) == Ref()["valid_keys"][i].get<int>());
    for (KeyCode k : InputSettings::unbindableKeys()) CHECK(std::find(keys.begin(), keys.end(), k) == keys.end());

    InputSettings s;
    fixture.dev.joysticks = {"Pad", ""};
    s.GetSettings();
    CHECK(s.joynames == std::vector<std::string>{"Pad", "Joystick 1"});
    CHECK(s.initialAxisValues.size() == static_cast<size_t>(InputSettings::deviceCount * InputSettings::axisCount));

    // key: primary
    auto target = std::make_shared<KeyBinding>(KeyCode::W, KeyCode::UpArrow, InputBindingModes::Staging, InputBindingModes::Docking_Rotation);
    s.ListenToKey(target, true, "Pitch down");
    CHECK(s.listenMode == InputSettings::ListenModes::KEY_PRIMARY);
    CHECK(s.targetSwitchState == InputBindingModes::Staging);
    CHECK(s.potentialKey != target);
    CHECK(s.potentialKey->primary.ToString() == "W");
    // nothing pressed: nothing detected
    s.OnUpdate();
    CHECK(s.potentialKey->primary.ToString() == "W");
    fixture.dev.down.insert("F");
    s.OnUpdate();
    CHECK(s.potentialKey->primary.ToString() == "F");
    CHECK(target->primary.ToString() == "W");  // the live binding is untouched until Apply
    s.ApplyKeyBinding();
    CHECK(target->primary.ToString() == "F");
    CHECK(target->secondary.ToString() == "UpArrow");
    CHECK(s.listenMode == InputSettings::ListenModes::OFF);
    CHECK_FALSE(static_cast<bool>(s.potentialKey));
    // secondary slot takes the secondary switch state
    s.ListenToKey(target, false, "x");
    CHECK(s.targetSwitchState == InputBindingModes::Docking_Rotation);
    fixture.dev.down.clear();
    fixture.dev.down.insert("G");
    s.OnUpdate();
    CHECK(s.potentialKey->secondary.ToString() == "G");
    CHECK(s.potentialKey->switchStateSecondary == InputBindingModes::Docking_Rotation);
    s.ApplyKeyBinding();
    CHECK(target->secondary.ToString() == "G");
    // SetKey is ignored in axis mode
    auto ax = std::make_shared<AxisBinding>();
    s.ListenToAxis(ax, true, "Pitch");
    s.SetKey(KeyCodeExtended(KeyCode::A));
    CHECK(s.potentialAxis->primary.idTag == "None");
    // an axis moving by more than 0.5 from its baseline is picked up
    fixture.dev.axes["joy0.4"] = 0.9f;
    s.OnUpdate();
    CHECK(s.potentialAxis->primary.idTag == "joy0.4");
    CHECK(s.potentialAxis->primary.name == "Pad");
    CHECK(s.potentialAxis->primary.axisIdx == 4);
    CHECK(s.potentialAxis->primary.deviceIdx == 0);
    CHECK(s.potentialAxis->primary.title == "Pad Axis 4");
    GameSettings::INPUT_DEVICES = std::make_shared<InputDevices>();
    s.ApplyAxisBinding();  // Save + Load: the live binding resolves the device through the InputDevices table
    CHECK(ax->primary.name == "Pad");
    CHECK(ax->primary.axisIdx == 4);
    CHECK(ax->primary.idTag == "joy0.4");
    CHECK(s.listenMode == InputSettings::ListenModes::OFF);
}

TEST_CASE("WP-50 TrackIR.Settings: Load keeps the node, Save copies it back when no TrackIR component exists") {
    InputFixture fixture;
    TrackIRSettings s;
    TrackIRSettings::Instance = nullptr;
    ConfigNode out;
    s.Save(out);
    CHECK(out.CountValues() == 0);
    CHECK(out.CountNodes() == 0);
    // a node named TRACKIR is copied into the target
    auto node = ConfigNode::Create("TRACKIR");
    node->AddValue("activeFlight", "False");
    s.Load(node);
    CHECK(s.Node() == node);
    ConfigNode out2;
    s.Save(out2);
    CHECK(Str(out2.GetValue("activeFlight")) == "False");
    // a wrapper node containing TRACKIR: the child is set (created) under the target
    auto wrap = ConfigNode::Create("wrapper");
    ConfigNode* child = wrap->AddNode("TRACKIR");
    child->AddValue("verbose", "True");
    s.Load(wrap);
    ConfigNode out3;
    s.Save(out3);
    REQUIRE(out3.HasNode("TRACKIR"));
    CHECK(Str(out3.GetNode("TRACKIR")->GetValue("verbose")) == "True");
    // with a component the component writes
    struct Inst final : ITrackIRInstance {
        int loads = 0;
        void Load(const ConfigNode&) override { ++loads; }
        void Save(ConfigNode& n) const override { n.AddValue("fromInstance", "1"); }
    } inst;
    TrackIRSettings::Instance = &inst;
    s.Load(node);
    CHECK(inst.loads == 1);
    ConfigNode out4;
    s.Save(out4);
    CHECK(Str(out4.GetValue("fromInstance")) == "1");
    CHECK_FALSE(out4.HasValue("activeFlight"));
    TrackIRSettings::Instance = nullptr;
}

TEST_CASE("WP-50 GameSettings: axis incremental speed multipliers, collapsed PAW groups, colour presets") {
    InputFixture fixture;
    CHECK(GameSettings::GetAxisIncrementalSpeedMultiplierValues() == nullptr);  // C# null until InitializeAxisGroupbindings
    CHECK(GameSettings::GetAxisSpeedMultiplier(3) == GameSettings::AXIS_INCREMENTAL_SPEED_MULTIPLIER_DEFAULT);
    GameSettings::InitializeAxisGroupbindings();
    const std::vector<float>& v = *GameSettings::GetAxisIncrementalSpeedMultiplierValues();
    const float expected[] = {0.01f, 0.02f, 0.0333f, 0.05f, 0.0666f, 0.08f, 0.10f, 0.125f, 0.15f, 0.2f, 0.25f, 0.333f, 0.4f, 0.5f, 0.666f, 1.0f, 2.0f, 3.0f};
    REQUIRE(v.size() == 18);
    for (size_t i = 0; i < 18; ++i) CHECK(v[i] == expected[i]);
    CHECK(GameSettings::GetAxisSpeedMultiplier(0) == 0.01f);
    CHECK(GameSettings::GetAxisSpeedMultiplier(17) == 3.0f);
    CHECK(GameSettings::GetAxisSpeedMultiplier(18) == 0.2f);  // out of range -> the default
    CHECK(GameSettings::GetAxiSpeedMultiplierIndex(0.2f) == 9);
    CHECK(GameSettings::GetAxiSpeedMultiplierIndex(0.3f) == -1);  // not in the list; the default is already there
    // a list without the default gets it inserted and sorted
    GameSettings::AXIS_INCREMENTAL_SPEED_MULTIPLIER_STORAGE = "0.5,junk,0.1,,3";
    GameSettings::InitializeAxisGroupbindings();
    CHECK(*GameSettings::GetAxisIncrementalSpeedMultiplierValues() == std::vector<float>{0.5f, 0.1f, 3.0f});
    CHECK(GameSettings::GetAxiSpeedMultiplierIndex(0.5f) == 0);  // found: the stored position in the unsorted list
    GameSettings::AddIncrementalSpeedMultiplierValue(7.0f);
    CHECK(GameSettings::GetAxisIncrementalSpeedMultiplierValues()->back() == 7.0f);
    // the unsorted list: found values report their stored position
    CHECK(GameSettings::GetAxiSpeedMultiplierIndex(0.1f) == 1);
    // not found and no default: default 0.2 appended and the list sorted
    CHECK(GameSettings::GetAxiSpeedMultiplierIndex(0.3f) == -1);
    CHECK(*GameSettings::GetAxisIncrementalSpeedMultiplierValues() == std::vector<float>{0.1f, 0.2f, 0.5f, 3.0f, 7.0f});

    // collapsed PAW groups: split on ',' dropping empties; the string is rebuilt by WriteCfg
    GameSettings::PAW_COLLAPSED_GROUP_NAMES = "Engine,,Resources,";
    GameSettings::SetCollapsedPawGroupNames();
    CHECK(GameSettings::collpasedPAWGroups == std::vector<std::string>{"Engine", "Resources"});
    GameSettings::collpasedPAWGroups.push_back("Misc");
    GameSettings::SetCollapsedPawGroupString();
    CHECK(GameSettings::PAW_COLLAPSED_GROUP_NAMES == "Engine,Resources,Misc");

    // colour presets from the default strings
    GameSettings::InitializeDefaultValues();
    const auto& lights = GameSettings::GetLightPresetColors();
    REQUIRE(lights.size() == 5);
    CHECK(lights[0].r == 1.0f);
    CHECK(lights[1].r == 0.9333f);
    CHECK(lights[1].g == 0.0039f);
    CHECK(lights[4].b == 1.0f);
    CHECK(GameSettings::highlighterColors.colorPartHighlightDefault.g == 1.0f);
    CHECK(GameSettings::highlighterColors.colorPartEditorDetached.r == 1.0f);
    CHECK(GameSettings::highlighterColors.HighlighterLimit == GameSettings::PART_HIGHLIGHTER_BRIGHTNESSFACTOR);
    // an invalid colour string leaves the target unchanged (SetHighlighterColor) / the parsed default (LoadColor)
    GameSettings::COLOR_PART_HIGHLIGHT = "nonsense";
    GameSettings::highlighterColors.colorPartHighlightDefault = Color(0.3f, 0.3f, 0.3f, 1.0f);
    GameSettings::SetHighlighterColors();
    CHECK(GameSettings::highlighterColors.colorPartHighlightDefault.r == 0.3f);
    CHECK(GameSettings::COLOR_CONTROLPOINT_COLOR_FORWARD.b == 0.86f);
}

TEST_CASE("WP-50 GameSettings: ResetSettings / SaveSettingsOnNextGameSave / ApplySettings host hooks") {
    InputFixture fixture;
    GameSettings::host = {};
    int applied = 0, presets = 0;
    GameSettings::host.applySettings = [&] { ++applied; };
    GameSettings::host.createDefaultPresetList = [&] { ++presets; };
    int written = 0;
    struct W { int* n; void On() { ++*n; } } w{&written};
    events::GameEvents::OnGameSettingsWritten.Add(&w, &W::On);
    GameSettings::UI_SCALE = 3.0f;
    GameSettings::layoutConfigNodes.Clear();
    GameSettings::ParseLayoutsCfg({LayoutDir() + "Qwerty.cfg"});
    GameSettings::ResetSettings();
    CHECK(GameSettings::UI_SCALE == 1.0f);
    CHECK(GameSettings::CURRENT_LAYOUT_SETTINGS == "Qwerty.cfg");
    CHECK(applied == 1);
    CHECK(presets == 1);
    CHECK(written == 1);
    // SaveSettingsOnNextGameSave: the next OnGameSave writes once
    GameSettings::OnGameSave();
    CHECK(written == 1);
    GameSettings::SaveSettingsOnNextGameSave();
    GameSettings::OnGameSave();
    CHECK(written == 2);
    GameSettings::OnGameSave();
    CHECK(written == 2);
    events::GameEvents::OnGameSettingsWritten.Remove(&w, &W::On);
    GameSettings::host = {};
}
