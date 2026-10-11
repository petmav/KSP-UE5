// WP-50: InputSettings.cs (the listen session; see input_settings.hpp for what is not ported).
#include "ksp/input/input_settings.hpp"

#include "ksp/dotnet/enum.hpp"
#include "ksp/input/extended_input.hpp"
#include "ksp/input/input_device.hpp"
#include "ksp/input/space_navigator.hpp"
#include "ksp/unity/mathf.hpp"

namespace ksp {

// InputSettings.cs:383
const std::vector<KeyCode>& InputSettings::unbindableKeys() {
    static const std::vector<KeyCode> keys = {KeyCode::Mouse0, KeyCode::Mouse1, KeyCode::Mouse2};
    return keys;
}

// InputSettings.cs:392
const std::vector<KeyCode>& InputSettings::keyValues() {
    static const std::vector<KeyCode> keys = GetValidKeys();
    return keys;
}

// InputSettings.cs:430 (Enum.GetValues(typeof(KeyCode)): ascending by value, aliases included)
std::vector<KeyCode> InputSettings::GetValidKeys() {
    std::vector<KeyCode> list;
    for (uint64_t v : dotnet::detail::GetTable<KeyCode>().values) list.push_back(static_cast<KeyCode>(static_cast<int>(static_cast<int64_t>(v))));
    int num = static_cast<int>(unbindableKeys().size()) - 1;
    int count = static_cast<int>(list.size());
    while (count-- > 0) {
        int num2 = num;
        while (num2 >= 0) {
            if (unbindableKeys()[static_cast<size_t>(num2)] != list[static_cast<size_t>(count)]) {
                num2--;
                continue;
            }
            list.erase(list.begin() + count);
            break;
        }
    }
    for (int i = 0; i < 127; i++) list.push_back(static_cast<KeyCode>(i + 128));
    return list;
}

// InputSettings.cs:456 (the joystick-name table, 6DOF flag and axis baseline; the GameSettings -> field copies are aliasing no-ops here)
void InputSettings::GetSettings() {
    has6DOFDevice = dynamic_cast<SpaceNavigatorNoDevice*>(SpaceNavigator::Instance()) == nullptr;
    joynames = InputEnvironment::Input().GetJoystickNames();
    for (size_t i = 0; i < joynames.size(); ++i) {
        if (!joynames[i].empty()) joynames[i] = InputDevices::TrimDeviceName(joynames[i]);
        else joynames[i] = "Joystick " + std::to_string(i);
    }
    initialAxisValues.assign(static_cast<size_t>(deviceCount * axisCount), 0.0f);
}

// InputSettings.cs:1106
void InputSettings::ListenToKey(std::shared_ptr<KeyBinding> target, bool isPrimary, const std::string& actionName) {
    listenMode = isPrimary ? ListenModes::KEY_PRIMARY : ListenModes::KEY_SECONDARY;
    targetSwitchState = isPrimary ? target->switchState : target->switchStateSecondary;
    keyTarget = target;
    potentialKey = std::make_shared<KeyBinding>();
    ConfigNode node;
    keyTarget->Save(node);
    potentialKey->Load(node);
    actionTargetName = actionName;
    // SpawnListenModeDialog(): UI
}

// InputSettings.cs:1119
void InputSettings::ListenToAxis(std::shared_ptr<AxisBinding> target, bool primary, const std::string& actionName) {
    listenMode = primary ? ListenModes::AXIS_PRIMARY : ListenModes::AXIS_SECONDARY;
    axisTarget = target;
    potentialAxis = std::make_shared<AxisBinding>();
    ConfigNode node;
    axisTarget->Save(node);
    potentialAxis->Load(node);
    ResetAxisValues();
    targetSwitchState = primary ? target->primary.switchState : target->secondary.switchState;
    actionTargetName = actionName;
}

// InputSettings.cs:1133
void InputSettings::ResetAxisValues() {
    if (initialAxisValues.size() < static_cast<size_t>(deviceCount * axisCount)) initialAxisValues.resize(static_cast<size_t>(deviceCount * axisCount), 0.0f);
    for (size_t i = 0; i < initialAxisValues.size(); ++i) {
        initialAxisValues[i] = InputEnvironment::Input().GetAxis("joy" + std::to_string(i / axisCount) + "." + std::to_string(i % axisCount));
    }
}

// InputSettings.cs:1142
void InputSettings::SetKey(const KeyCodeExtended& key) {
    if (listenMode != ListenModes::OFF && listenMode != ListenModes::AXIS_PRIMARY && listenMode != ListenModes::AXIS_SECONDARY) {
        if (listenMode == ListenModes::KEY_PRIMARY) {
            potentialKey->primary = key;
            potentialKey->switchState = targetSwitchState;
        }
        if (listenMode == ListenModes::KEY_SECONDARY) {
            potentialKey->secondary = key;
            potentialKey->switchStateSecondary = targetSwitchState;
        }
    }
}

// InputSettings.cs:1160
void InputSettings::SetAxis(const std::string& axis, const std::string& name, int axisIdx, int deviceIdx, ListenModes mode) {
    AxisBinding_Single& axisBinding_Single = (mode == ListenModes::AXIS_PRIMARY || mode != ListenModes::AXIS_SECONDARY) ? potentialAxis->primary
                                                                                                                       : potentialAxis->secondary;
    axisBinding_Single.idTag = axis;
    axisBinding_Single.name = name;
    axisBinding_Single.title = name + ((axisIdx != -1) ? (" Axis " + std::to_string(axisIdx)) : "");
    axisBinding_Single.deviceIdx = deviceIdx;
    axisBinding_Single.axisIdx = axisIdx;
    axisBinding_Single.switchState = targetSwitchState;
    ResetAxisValues();
}

// InputSettings.cs:1173
void InputSettings::ApplyAxisBinding() {
    ConfigNode node;
    potentialAxis->Save(node);
    axisTarget->Load(node);
    potentialAxis.reset();
    listenMode = ListenModes::OFF;
}

// InputSettings.cs:1183
void InputSettings::ApplyKeyBinding() {
    ConfigNode node;
    potentialKey->Save(node);
    keyTarget->Load(node);
    potentialKey.reset();
    listenMode = ListenModes::OFF;
}

// InputSettings.cs:1193
void InputSettings::OnUpdate() {
    if (listenMode == ListenModes::OFF) return;
    const IInputDevice& input = InputEnvironment::Input();
    if (listenMode == ListenModes::AXIS_PRIMARY || listenMode == ListenModes::AXIS_SECONDARY) {
        for (int i = 0; i < deviceCount; i++) {
            for (int j = 0; j < axisCount; j++) {
                const float cur = input.GetAxis("joy" + std::to_string(i) + "." + std::to_string(j));
                if (Mathf::Abs(f32store(fx(cur) - fx(initialAxisValues[static_cast<size_t>(i * axisCount + j)]))) > 0.5f) {
                    // joynames[i]: IndexOutOfRangeException for a device without a name entry, as in C#
                    SetAxis("joy" + std::to_string(i) + "." + std::to_string(j), joynames.at(static_cast<size_t>(i)), j, i, listenMode);
                }
            }
        }
    }
    KeyCodeExtended key;
    if ((listenMode == ListenModes::KEY_PRIMARY || listenMode == ListenModes::KEY_SECONDARY) &&
        ExtendedInput::DetectKeyDown(&keyValues(), key)) {
        SetKey(key);
    }
}

}  // namespace ksp
