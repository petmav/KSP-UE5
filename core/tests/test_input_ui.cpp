// WP-50 unit tests, part 4: FlightInputHandler.Update key toggles, and the KSP.UI input-lock helpers. Literal expectations read off
// FlightInputHandler.cs / CanvasGroupInputLock.cs / KSPGraphicRaycaster.cs / DialogMouseEnterControlLock.cs.
#include "input_test_support.hpp"

#include "ksp/celestial/planetarium.hpp"
#include "ksp/input/menu_nav_input.hpp"
#include "ksp/modules/vessel_module.hpp"
#include "ksp/parts/part_module_data.hpp"
#include "ksp/vessel/axis_groups_module.hpp"
#include "ksp/input/ui_input_locks.hpp"

using namespace ksp;
using json = nlohmann::json;
using namespace wp50test;

namespace {

void LoadLocalizerWp50() {
    KSP::Localization::Localizer::Shutdown();
    KSP::Localization::Localizer::Init();
    ConfigNode::NodePtr root = ConfigNode::Load(ksp::testing::GameRoot() + "/GameData/Squad/Localization/dictionary.cfg");
    REQUIRE(root);
    std::vector<ConfigNode*> nodes{root->GetNode("Localization")};
    KSP::Localization::Localizer::Instance()->AddTagValuesForLanguage(nodes, "en-us");
}

struct FlightSetup {
    InputFixture fixture;
    Planetarium planetarium;  // ActionGroupList.ToggleGroup reads the universal time for the cooldown
    Vessel active;
    FlightInputHandler h;
    std::vector<std::pair<std::string, float>> messages;
    int stages = 0;
    FlightSetup() {
        planetarium.Awake();
        Planetarium::SetUniversalTime(100.0);
        GameSettings::layoutConfigNodes.Clear();
        GameSettings::ParseLayoutsCfg({LayoutDir() + "Qwerty.cfg"});
        GameSettings::LoadLayoutKeyBindings("Qwerty.cfg");
        FlightRegistry::VesselsLoaded = {&active};
        FlightRegistry::ActiveVessel = &active;
        FlightRegistry::ready = true;
        HighLogic::LoadedSceneIsFlight = true;
        FlightDriver::pause = false;
        FlightInputHandler::host = {};
        FlightInputHandler::host.postScreenMessage = [this](const std::string& t, float d) { messages.emplace_back(t, d); };
        FlightInputHandler::host.activateNextStage = [this] { ++stages; };
        h.Awake();
        h.Start();
    }
    ~FlightSetup() {
        h.OnDestroy();
        FlightRegistry::VesselsLoaded.clear();
        FlightRegistry::ActiveVessel = nullptr;
        FlightRegistry::ready = false;
        FlightInputHandler::host = {};
        SpaceNavigator::ResetStatics();
        SpaceNavigatorWindows::sensorFactory = nullptr;
        InputLockManager::Reset();
    }
    void Press(std::initializer_list<const char*> down, std::initializer_list<const char*> held = {}) {
        fixture.dev.down.clear();
        fixture.dev.held.clear();
        fixture.dev.up.clear();
        for (const char* d : down) {
            fixture.dev.down.insert(d);
            fixture.dev.held.insert(d);
        }
        for (const char* d : held) fixture.dev.held.insert(d);
    }
    void Release(std::initializer_list<const char*> up) {
        fixture.dev.down.clear();
        fixture.dev.held.clear();
        fixture.dev.up.clear();
        for (const char* d : up) fixture.dev.up.insert(d);
    }
};

}  // namespace

TEST_CASE("WP-50 FlightInputHandler.Update: stage lock, precision mode, RCS, gear, lights, throttle keys, staging, SAS") {
    FlightSetup f;
    FlightCtrlState& st = FlightInputHandler::state;
    InputEnvironment::realtimeSinceStartup = 100.0f;  // a first press is never a double tap
    // MODIFIER + L: manualStageLock toggles
    f.Press({"L"}, {"LeftAlt"});
    f.h.Update();
    CHECK(f.h.stageLock);
    CHECK(InputLockManager::GetControlLock("manualStageLock") == ControlTypes::STAGING);
    f.h.Update();
    CHECK_FALSE(f.h.stageLock);
    CHECK(InputLockManager::GetControlLock("manualStageLock") == ControlTypes::None);
    // MISC locked: neither the stage lock nor the precision toggle
    InputLockManager::SetControlLock(ControlTypes::MISC, "misc");
    f.Press({"L"}, {"LeftAlt"});
    f.h.Update();
    CHECK_FALSE(f.h.stageLock);
    f.Press({"CapsLock"});
    f.h.Update();
    CHECK_FALSE(f.h.precisionMode);
    InputLockManager::RemoveControlLock("misc");
    // precision mode + event
    struct P { std::vector<bool> seen; void On(bool b) { seen.push_back(b); } } p;
    events::GameEvents::Input::OnPrecisionModeToggle.Add(&p, &P::On);
    f.h.Update();
    CHECK(f.h.precisionMode);
    f.h.Update();
    CHECK_FALSE(f.h.precisionMode);
    CHECK(p.seen == std::vector<bool>{true, false});
    events::GameEvents::Input::OnPrecisionModeToggle.Remove(&p, &P::On);
    // RCS: the action group toggles and the lock flips
    f.Press({"R"});
    CHECK(f.h.rcslock);
    const bool rcsBefore = f.active.ActionGroups()[KSPActionGroup::RCS];
    f.h.Update();
    CHECK(f.active.ActionGroups()[KSPActionGroup::RCS] == !rcsBefore);
    CHECK_FALSE(f.h.rcslock);
    InputLockManager::SetControlLock(ControlTypes::RCS, "rcs");
    f.h.Update();
    CHECK_FALSE(f.h.rcslock);  // locked: nothing
    InputLockManager::RemoveControlLock("rcs");
    // gear and lights set the one-frame flags
    f.Press({"G", "U"});
    f.h.Update();
    CHECK(st.gearDown);
    CHECK(st.gearUp);
    CHECK(st.headlight);
    f.Press({});
    f.h.Update();
    CHECK_FALSE(st.gearDown);
    CHECK_FALSE(st.headlight);
    // throttle cut / full
    st.mainThrottle = 0.4f;
    f.Press({"X"});
    f.h.Update();
    CHECK(st.mainThrottle == 0.0f);
    f.Press({"Z"});
    f.h.Update();
    CHECK(st.mainThrottle == 1.0f);
    st.mainThrottle = 0.4f;
    f.Press({"X"}, {"LeftAlt"});  // with the modifier the cut-off is ignored
    f.h.Update();
    CHECK(st.mainThrottle == 0.4f);
    InputLockManager::SetControlLock(ControlTypes::THROTTLE_CUT_MAX, "cut");
    f.Press({"Z"});
    f.h.Update();
    CHECK(st.mainThrottle == 0.4f);
    InputLockManager::RemoveControlLock("cut");
    // staging (Space in the staging UI mode only)
    FlightUIModeControllerSeam::present = true;
    FlightUIModeControllerSeam::mode = FlightUIMode::STAGING;
    f.Press({"Space"});
    f.h.Update();
    CHECK(f.stages == 1);
    InputLockManager::SetControlLock(ControlTypes::STAGING, "stg");
    f.h.Update();
    CHECK(f.stages == 1);
    InputLockManager::RemoveControlLock("stg");
    FlightUIModeControllerSeam::mode = FlightUIMode::DOCKING;  // Space then means the rotation / translation toggle, not staging
    f.h.Update();
    CHECK(f.stages == 1);
    // SAS: toggle on T, hold on F (down toggles, up toggles back)
    FlightUIModeControllerSeam::present = false;
    f.Press({"T"});
    f.h.Update();
    CHECK(st.killRot);
    f.h.Update();
    CHECK_FALSE(st.killRot);
    f.Press({"F"});
    f.h.Update();
    CHECK(st.killRot);
    f.Release({"F"});
    f.h.Update();
    CHECK_FALSE(st.killRot);
    // custom action groups and abort / brakes are routed to the vessel's action group list
    f.Press({"Alpha1", "Alpha0", "Backspace", "B"});
    const bool c1 = f.active.ActionGroups()[KSPActionGroup::Custom01];
    const bool c10 = f.active.ActionGroups()[KSPActionGroup::Custom10];
    const bool ab = f.active.ActionGroups()[KSPActionGroup::Abort];
    f.h.Update();
    CHECK(f.active.ActionGroups()[KSPActionGroup::Custom01] == !c1);
    CHECK(f.active.ActionGroups()[KSPActionGroup::Custom10] == !c10);
    CHECK(f.active.ActionGroups()[KSPActionGroup::Abort] == !ab);
    CHECK(f.active.ActionGroups()[KSPActionGroup::Brakes]);
    f.Release({"B"});
    f.h.Update();
    CHECK_FALSE(f.active.ActionGroups()[KSPActionGroup::Brakes]);
    InputLockManager::SetControlLock(ControlTypes::CUSTOM_ACTION_GROUPS, "cag");
    f.Press({"Alpha1"});
    f.h.Update();
    CHECK(f.active.ActionGroups()[KSPActionGroup::Custom01] == !c1);  // still the previous toggle
    InputLockManager::RemoveControlLock("cag");
}

TEST_CASE("WP-50 FlightInputHandler.Update: translation / rotation switch (double tap hold mode), EVA, paused, ActionGroupsApp, SpaceNav toggle") {
    FlightSetup f;
    LoadLocalizerWp50();
    FlightUIModeControllerSeam::present = true;
    FlightUIModeControllerSeam::mode = FlightUIMode::DOCKING;
    // single press toggles the rotation / translation state
    InputEnvironment::realtimeSinceStartup = 1.0f;
    Time::frameCount = 1;
    f.Press({"Space"});
    f.h.Update();
    CHECK(InputBinding::linRotState);
    CHECK(f.messages.empty());
    // (the state flips the Docking_toggleRotLin switch compare: Docking_Rotation & DockingModes still matches)
    InputEnvironment::realtimeSinceStartup = 1.5f;
    Time::frameCount = 2;
    f.h.Update();
    CHECK_FALSE(InputBinding::linRotState);
    // a third press within 0.2 s of the second: hold mode, with the screen message
    InputEnvironment::realtimeSinceStartup = 1.6f;
    Time::frameCount = 3;
    f.h.Update();  // 1.6 < 1.5 + 0.2 -> double tap
    REQUIRE(f.messages.size() == 1);
    CHECK(f.h.linRotSwitchHold);
    CHECK(f.messages[0].second == 2.0f);
    CHECK(f.messages[0].first == KSP::Localization::Localizer::Format("#autoLOC_7003261", {KSP::Localization::Localizer::Format("#autoLOC_7003262")}));
    // hold mode: down sets, up clears
    InputBinding::linRotState = false;
    InputEnvironment::realtimeSinceStartup = 5.0f;
    Time::frameCount = 5;
    f.Press({"Space"});
    f.h.Update();
    CHECK(InputBinding::linRotState);
    f.Release({"Space"});
    f.h.Update();
    CHECK_FALSE(InputBinding::linRotState);
    FlightUIModeControllerSeam::present = false;

    // EVA: only the EVA-rotate toggle on SAS_TOGGLE (T) when UI_DIALOGS is unlocked
    f.active.vesselType = VesselType::EVA;
    f.Press({"T"});
    const bool rot = GameSettings::EVA_ROTATE_ON_MOVE;
    f.h.Update();
    CHECK(GameSettings::EVA_ROTATE_ON_MOVE == !rot);
    InputLockManager::SetControlLock(ControlTypes::UI_DIALOGS, "dlg");
    f.h.Update();
    CHECK(GameSettings::EVA_ROTATE_ON_MOVE == !rot);
    InputLockManager::RemoveControlLock("dlg");
    f.active.vesselType = VesselType::Debris;

    // paused / not ready: Update does nothing
    FlightDriver::pause = true;
    f.Press({"G"});
    f.h.Update();
    CHECK_FALSE(FlightInputHandler::state.gearDown);
    FlightDriver::pause = false;
    FlightRegistry::ready = false;
    f.h.Update();
    CHECK_FALSE(FlightInputHandler::state.gearDown);
    FlightRegistry::ready = true;

    // ActionGroupsApp select keys
    int next = 0, prev = 0;
    FlightInputHandler::host.actionGroupsAppPresent = true;
    FlightInputHandler::host.actionGroupsSelectNext = [&] { ++next; };
    FlightInputHandler::host.actionGroupsSelectPrev = [&] { ++prev; };
    f.Press({"F7"});
    f.h.Update();
    f.Press({"F6"});
    f.h.Update();
    CHECK(next == 1);
    CHECK(prev == 1);

    // SpaceNavigator toggle: needs a SpaceNavigator device when the handler starts
    struct Sensor final : ISpaceNavSensor { SpaceNavSensorReading Read() const override { return {}; } };
    SpaceNavigator::ResetStatics();
    SpaceNavigatorWindows::sensorFactory = [] { return std::make_shared<Sensor>(); };
    f.h.Start();
    CHECK(f.h.hasSpaceNavDevice);
    f.messages.clear();
    f.Release({"ScrollLock"});
    CHECK_FALSE(FlightInputHandler::SPACENAV_USE_AS_FLIGHT_CONTROL);
    f.h.Update();
    CHECK(FlightInputHandler::SPACENAV_USE_AS_FLIGHT_CONTROL);
    REQUIRE(f.messages.size() == 1);
    CHECK(f.messages[0].first == KSP::Localization::Localizer::Format("#autoLOC_175419"));
    CHECK(f.messages[0].second == 3.0f);
    f.h.Update();
    CHECK_FALSE(FlightInputHandler::SPACENAV_USE_AS_FLIGHT_CONTROL);
    CHECK(f.messages[1].first == KSP::Localization::Localizer::Format("#autoLOC_175414"));
    // the SpaceNav contribution to FixedUpdate: translation added, rotation subtracted / added
    FlightInputHandler::SPACENAV_USE_AS_FLIGHT_CONTROL = true;
    struct Moving final : ISpaceNavSensor { SpaceNavSensorReading Read() const override { SpaceNavSensorReading r; r.translationX = 5000.0; r.rotationAngle = 100.0; r.rotationX = 1.0; return r; } };
    SpaceNavigator::ResetStatics();
    SpaceNavigatorWindows::sensorFactory = [] { return std::make_shared<Moving>(); };
    f.Press({});
    Time::deltaTime = 0.02f;
    f.h.refocusThrottle = false;
    f.h.FixedUpdate();
    CHECK(FlightInputHandler::state.X != 0.0f);
    CHECK(FlightInputHandler::state.pitch != 0.0f);
    FlightInputHandler::SPACENAV_USE_AS_FLIGHT_CONTROL = false;
}

TEST_CASE("WP-50 FlightInputHandler: SetNeutralControls / ResumeVesselCtrlState / OverrideGroupChanged / OnApplicationFocus") {
    FlightSetup f;
    Vessel other;
    FlightInputHandler::state.pitch = 0.9f;
    f.active.ctrlState.pitch = 0.5f;
    FlightInputHandler::SetNeutralControls();
    CHECK(FlightInputHandler::state.pitch == 0.0f);
    CHECK(f.active.ctrlState.pitch == 0.0f);
    CHECK(f.h.throttle == -1.0f);
    // ResumeVesselCtrlState(v): the vessel's control state becomes the input state; the throttle mirrors it
    other.ctrlState.mainThrottle = 0.75f;
    other.ctrlState.roll = 0.25f;
    other.ActionGroups().Set(KSPActionGroup::Gear, true);
    FlightInputHandler::ResumeVesselCtrlState(&other);
    CHECK(FlightInputHandler::state.mainThrottle == 0.75f);
    CHECK(FlightInputHandler::state.roll == 0.25f);
    CHECK(f.h.throttle == 0.5f);  // 0.75 * 2 - 1
    CHECK(f.h.refocusThrottle);
    CHECK(f.active.ActionGroups()[KSPActionGroup::Gear]);
    // the override-group event resumes only the active vessel
    f.h.refocusThrottle = false;
    f.h.OnVesselOverrideGroupChanged(&other);
    CHECK_FALSE(f.h.refocusThrottle);
    f.h.OnVesselOverrideGroupChanged(&f.active);
    CHECK(f.h.refocusThrottle);
    f.h.OnApplicationFocus(false);
    CHECK_FALSE(f.h.hasFocus);
    f.h.refocusThrottle = false;
    f.h.OnApplicationFocus(true);
    CHECK(f.h.hasFocus);
    CHECK(f.h.refocusThrottle);
    // SetLaunchCtrlState uses the PRELAUNCH_DEFAULT_THROTTLE
    GameSettings::PRELAUNCH_DEFAULT_THROTTLE = 0.3f;
    FlightInputHandler::SetLaunchCtrlState();
    CHECK(FlightInputHandler::state.mainThrottle == 0.3f);
}

TEST_CASE("WP-50 CanvasGroupInputLock / KSPGraphicRaycaster / DialogMouseEnterControlLock / UIInputExtraMouseButtons") {
    InputFixture fixture;
    // Enum.Parse(ignoreCase): names in any case, comma lists, numbers; failures skip the entry
    CHECK(UI::ParseControlTypesIgnoreCase("pitch") == ControlTypes::PITCH);
    CHECK(UI::ParseControlTypesIgnoreCase("  ThRoTtLe ") == ControlTypes::THROTTLE);
    CHECK(UI::ParseControlTypesIgnoreCase("PITCH, yaw") == (ControlTypes::PITCH | ControlTypes::YAW));
    CHECK(UI::ParseControlTypesIgnoreCase("128") == ControlTypes::STAGING);
    CHECK_THROWS_AS(UI::ParseControlTypesIgnoreCase(""), ksp::ArgumentException);
    CHECK_THROWS_AS(UI::ParseControlTypesIgnoreCase("nope"), ksp::ArgumentException);
    CHECK_THROWS(UI::ParseControlTypesIgnoreCase("-1"));  // .NET: OverflowException from Convert.ChangeType; here the shared numeric parser reports failure
    CHECK(UI::ComputeLockMask({"pitch", "bogus", "YAW", ""}, "go") == (static_cast<uint64_t>(ControlTypes::PITCH) | static_cast<uint64_t>(ControlTypes::YAW)));

    UI::CanvasGroup group;
    UI::CanvasGroupInputLock c;
    c.canvasGroup = &group;
    c.gameObjectName = "Panel";
    c.InputLockMask() = {"PITCH", "stAging"};
    c.Awake();  // the default mask is what the inspector configured
    c.Start();
    CHECK(c.LockMask() == (static_cast<uint64_t>(ControlTypes::PITCH) | static_cast<uint64_t>(ControlTypes::STAGING)));
    CHECK(group.blocksRaycasts);  // nothing locked
    InputLockManager::SetControlLock(ControlTypes::STAGING, "s");   // the event: blocksRaycasts = (to & mask) == 0
    CHECK_FALSE(group.blocksRaycasts);
    InputLockManager::RemoveControlLock("s");
    CHECK(group.blocksRaycasts);
    // SetInputMask rebuilds the mask; SetInputMaskToDefault restores the Awake list
    c.SetInputMask({"ROLL"});
    CHECK(c.LockMask() == static_cast<uint64_t>(ControlTypes::ROLL));
    InputLockManager::SetControlLock(ControlTypes::PITCH, "p");
    CHECK(group.blocksRaycasts);  // PITCH is no longer in the mask
    c.SetInputMaskToDefault();
    CHECK(c.LockMask() == (static_cast<uint64_t>(ControlTypes::PITCH) | static_cast<uint64_t>(ControlTypes::STAGING)));
    InputLockManager::SetControlLock(ControlTypes::PITCH, "p2");
    CHECK_FALSE(group.blocksRaycasts);
    InputLockManager::Reset();
    // an edit through InputLockMask() marks the mask dirty: recomputed on the next lock event
    c.InputLockMask().push_back("YAW");
    InputLockManager::SetControlLock(ControlTypes::YAW, "y");
    CHECK_FALSE(group.blocksRaycasts);
    // disableAllLocking: nothing changes
    UI::CanvasGroupInputLock::disableAllLocking = true;
    group.blocksRaycasts = true;
    InputLockManager::SetControlLock(ControlTypes::PITCH, "q");
    CHECK(group.blocksRaycasts);
    UI::CanvasGroupInputLock::disableAllLocking = false;
    c.OnDestroy();
    InputLockManager::Reset();
    group.blocksRaycasts = true;
    InputLockManager::SetControlLock(ControlTypes::PITCH, "after destroy");
    CHECK(group.blocksRaycasts);  // unsubscribed
    InputLockManager::Reset();

    CHECK(UI::CanvasGroupInputLock::ToBits(ControlTypes::None) == std::string(64, '0'));
    CHECK(UI::CanvasGroupInputLock::ToBits(ControlTypes::PITCH) == std::string(63, '0') + "1");
    CHECK(UI::CanvasGroupInputLock::ToBits(static_cast<uint64_t>(1) << 63) == "1" + std::string(63, '0'));
    CHECK(UI::CanvasGroupInputLock::ToBits(ControlTypes::All) == "0000" + std::string(60, '1'));

    // KSPGraphicRaycaster: raycast only when the mask is empty or all its controls are unlocked
    UI::KSPGraphicRaycaster r;
    CHECK(r.Raycast());  // empty mask
    r.InputLockMask() = {"camerAcontrols"};
    CHECK(r.Raycast());
    CHECK(r.LockMask() == static_cast<uint64_t>(ControlTypes::CAMERACONTROLS));
    InputLockManager::SetControlLock(ControlTypes::CAMERACONTROLS, "cam");
    CHECK_FALSE(r.Raycast());
    InputLockManager::Reset();
    r.SetLockMask(0);
    CHECK(r.Raycast());

    // DialogMouseEnterControlLock: the default lock is the All mask under its default name
    UI::DialogMouseEnterControlLock d;
    CHECK(d.locktype == static_cast<uint64_t>(ControlTypes::All));
    d.OnPointerEnter();
    CHECK(InputLockManager::GetControlLock("DialogMouseEnterControlLock") == ControlTypes::All);
    d.OnPointerExit();
    CHECK(InputLockManager::GetControlLock("DialogMouseEnterControlLock") == ControlTypes::None);
    d.Setup(ControlTypes::EDITOR_LOCK, "dlg");
    d.OnPointerEnter();
    CHECK(InputLockManager::GetControlLock("dlg") == ControlTypes::EDITOR_LOCK);
    d.OnDestroy();
    CHECK(InputLockManager::lockStack.Count() == 0);

    // UIInputExtraMouseButtons: clicks only while the pointer is inside
    UI::UIInputExtraMouseButtons m;
    int l = 0, rr = 0, mid = 0;
    m.leftClick = [&] { ++l; };
    m.rightClick = [&] { ++rr; };
    m.middleClick = [&] { ++mid; };
    fixture.dev.mouseUp[0] = fixture.dev.mouseUp[1] = fixture.dev.mouseUp[2] = true;
    m.Update();
    CHECK(l + rr + mid == 0);
    m.OnMouseEnter();
    m.Update();
    CHECK((l == 1 && rr == 1 && mid == 1));
    m.OnMouseExit();
    m.Update();
    CHECK(l == 1);

    // MenuNavInput
    CHECK(static_cast<int>(MenuNavInput::None) == 7);
    CHECK(static_cast<int>(MenuNavInput::Tab) == 6);
}

TEST_CASE("WP-50 AxisGroupsInputHandler: subscribes to OnRawAxisInput and onVesselChange, picks the last AxisGroupsModule of the vessel") {
    InputFixture fixture;
    Vessel vessel;
    std::shared_ptr<VesselModule> mod = VesselModuleFactory::Create(*GetVesselModuleClass("AxisGroupsModule"));
    auto* axes = dynamic_cast<AxisGroupsModule*>(mod.get());
    REQUIRE(axes != nullptr);
    std::shared_ptr<VesselModule> other = VesselModuleFactory::Create(*GetVesselModuleClass("FlightIntegrator"));
    vessel.vesselModules = {mod, other};
    const size_t before = FlightInputHandler::OnRawAxisInput.GetInvocationList().size();
    AxisGroupsInputHandler h;
    h.Awake();
    CHECK(FlightInputHandler::OnRawAxisInput.GetInvocationList().size() == before + 1);
    CHECK(h.VesselAxisGroups == nullptr);
    events::GameEvents::onVesselChange.Fire(&vessel);
    CHECK(h.VesselAxisGroups == axes);  // the FlightIntegrator at the end is skipped
    FlightCtrlState st;
    st.pitch = 0.5f;
    CHECK_NOTHROW(FlightInputHandler::OnRawAxisInput(st));  // no axis groups built: UpdateAxisGroup returns at once
    vessel.vesselModules = {other};
    events::GameEvents::onVesselChange.Fire(&vessel);
    CHECK(h.VesselAxisGroups == nullptr);
    h.OnDestroy();
    CHECK(FlightInputHandler::OnRawAxisInput.GetInvocationList().size() == before);
    events::GameEvents::onVesselChange.Fire(&vessel);
    CHECK(h.VesselAxisGroups == nullptr);
}
