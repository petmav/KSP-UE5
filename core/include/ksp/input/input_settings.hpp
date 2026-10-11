// WP-50: the binding-edit ("listen") session of InputSettings (Assembly-CSharp/InputSettings.cs): which keys may be bound, and the
// transaction ListenToKey / ListenToAxis -> SetKey / SetAxis (from device polling in OnUpdate) -> ApplyKeyBinding / ApplyAxisBinding,
// which copies the edited binding into the live one through ConfigNode Save / Load exactly as the settings dialog does. The
// IMGUI / DialogGUI layout (drawInputPage, DrawKeyBinding, DrawMiniSettings, ...) and the 150 field copies of GetSettings /
// ApplySettings (reference aliasing of GameSettings bindings, which this port holds as shared_ptr anyway) are UI and not ported.
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "ksp/input/game_settings.hpp"
#include "ksp/input/input_binding.hpp"

namespace ksp {

class InputSettings {
public:
    // InputSettings.cs:9
    enum class ListenModes : int { OFF = 0, KEY_PRIMARY = 1, KEY_SECONDARY = 2, AXIS_PRIMARY = 3, AXIS_SECONDARY = 4 };

    static constexpr int deviceCount = 11;   // :39x
    static constexpr int axisCount = 20;

    // :383 unbindableKeys, :392 keyValues = GetValidKeys()
    static const std::vector<KeyCode>& unbindableKeys();
    static const std::vector<KeyCode>& keyValues();
    static std::vector<KeyCode> GetValidKeys();    // :430

    ListenModes listenMode = ListenModes::OFF;
    std::shared_ptr<KeyBinding> keyTarget, potentialKey;
    std::shared_ptr<AxisBinding> axisTarget, potentialAxis;
    InputBindingModes targetSwitchState = InputBindingModes::None;
    std::string actionTargetName;
    std::vector<std::string> joynames;
    std::vector<float> initialAxisValues;
    bool has6DOFDevice = false;

    void GetSettings();                                                    // :456 (the device / SpaceNavigator part)
    void ListenToKey(std::shared_ptr<KeyBinding> target, bool isPrimary, const std::string& actionName);   // :1106
    void ListenToAxis(std::shared_ptr<AxisBinding> target, bool primary, const std::string& actionName);   // :1119
    void ResetAxisValues();                                                // :1133
    void SetKey(const KeyCodeExtended& key);                               // :1142
    void SetAxis(const std::string& axis, const std::string& name, int axisIdx, int deviceIdx, ListenModes mode);   // :1160
    void ApplyAxisBinding();                                               // :1173
    void ApplyKeyBinding();                                                // :1183
    void OnUpdate();                                                       // :1193 (polling half)
};

}  // namespace ksp
