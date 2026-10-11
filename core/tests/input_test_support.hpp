// WP-50 test support: the reference JSON, a scripted UnityEngine.Input, and the fixture that restores the input statics.
#pragma once

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

namespace wp50test {

inline const json& Ref() {
    static const json j = [] {
        std::ifstream f(std::string(KSPCORE_TEST_DATA_DIR) + "/wp50_reference.json", std::ios::binary);
        REQUIRE(f.good());
        return json::parse(f);
    }();
    return j;
}

inline uint64_t U64(const json& s) { return std::stoull(s.get<std::string>()); }

inline bool SameBits(float a, float b) {
    uint32_t x, y;
    std::memcpy(&x, &a, 4);
    std::memcpy(&y, &b, 4);
    return x == y;
}

inline float F(const json& j) { return static_cast<float>(j.get<double>()); }

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
        GameSettings::AXIS_INCREMENTAL_SPEED_MULTIPLIER_VALUES.reset();  // never assigned by SetDefaultValues (null in a fresh process)
        GameSettings::INPUT_DEVICES.reset();                             // created by GameSettings.Awake
        Time::deltaTime = 0.02f;
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

inline std::string LayoutDir() { return ksp::testing::GameRoot() + "/GameData/Squad/Controls/"; }

inline std::string Str(const std::optional<std::string>& s) { return s ? *s : std::string("<null>"); }

struct LockListener {
    std::vector<std::vector<std::string>> cur;
    void OnMod(events::GameEvents::FromToAction<ControlTypes, ControlTypes> a) {
        cur.push_back({"mod", std::to_string(static_cast<uint64_t>(a.from)), std::to_string(static_cast<uint64_t>(a.to))});
    }
    void OnGuiLock() { cur.push_back({"guilock"}); }
    void OnTooltip() { cur.push_back({"tooltip"}); }
    void OnGuiUnlock() { cur.push_back({"guiunlock"}); }
};

}  // namespace wp50test
