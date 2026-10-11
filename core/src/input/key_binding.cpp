// WP-50: KeyBinding.cs runtime half (GetKey / GetKeyDown / double taps) and InputDevices.cs.
#include "binding_util.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/input/extended_input.hpp"
#include "ksp/input/input_binding.hpp"

namespace ksp {

using namespace input_detail;

// KeyBinding.cs:144
bool KeyBinding::GetKey(bool ignoreInputLock) const {
    if (IsLocked() && !ignoreInputLock) return false;
    if (ExtendedInput::GetKey(primary) && CompareSwitchState(switchState)) return true;
    if (ExtendedInput::GetKey(secondary)) return CompareSwitchState(switchStateSecondary);
    return false;
}

// KeyBinding.cs:175
bool KeyBinding::GetKeyDown(bool ignoreInputLock) const {
    if (IsLocked() && !ignoreInputLock) return false;
    if (ExtendedInput::GetKeyDown(primary) && CompareSwitchState(switchState)) return true;
    if (ExtendedInput::GetKeyDown(secondary)) return CompareSwitchState(switchStateSecondary);
    return false;
}

// KeyBinding.cs:168
bool KeyBinding::GetDoubleTapDown(bool ignoreInputLock) {
    doubleDown_ = false;
    if (IsLocked() && !ignoreInputLock) return false;
    if ((ExtendedInput::GetKeyDown(primary) && CompareSwitchState(switchState)) ||
        (ExtendedInput::GetKeyDown(secondary) && CompareSwitchState(switchStateSecondary))) {
        doubleDown_ = fx(InputEnvironment::realtimeSinceStartup) < fx(lastKeyDownTime_) + fx(0.2f);
        if (lastKeyDownFrameCount_ != Time::frameCount) {
            lastKeyDownTime_ = InputEnvironment::realtimeSinceStartup;
            lastKeyDownFrameCount_ = Time::frameCount;
        }
    }
    return doubleDown_;
}

// KeyBinding.cs:211
bool KeyBinding::GetKeyUp(bool ignoreInputLock) const {
    if (IsLocked() && !ignoreInputLock) return false;
    if (ExtendedInput::GetKeyUp(primary) && CompareSwitchState(switchState)) return true;
    if (ExtendedInput::GetKeyUp(secondary)) return CompareSwitchState(switchStateSecondary);
    return false;
}

// KeyBinding.cs:204
bool KeyBinding::GetDoubleTapUp(bool ignoreInputLock) {
    doubleUp_ = false;
    if (IsLocked() && !ignoreInputLock) return false;
    if ((ExtendedInput::GetKeyUp(primary) && CompareSwitchState(switchState)) ||
        (ExtendedInput::GetKeyUp(secondary) && CompareSwitchState(switchStateSecondary))) {
        doubleUp_ = fx(InputEnvironment::realtimeSinceStartup) < fx(lastKeyUpTime_) + fx(0.2f);
        if (lastKeyUpFrameCount_ != Time::frameCount) {
            lastKeyUpTime_ = InputEnvironment::realtimeSinceStartup;
            lastKeyUpFrameCount_ = Time::frameCount;
        }
    }
    return doubleUp_;
}

// KeyBinding.cs:223
bool KeyBinding::IsNeutral() const { return !GetKey(); }

// InputDevices.cs:8
InputDevices::InputDevices() {
    if (InputEnvironment::applicationIsPlaying) InitializeDevices();
}

// InputDevices.cs:17
void InputDevices::InitializeDevices() {
    const std::vector<std::string> joystickNames = InputEnvironment::Input().GetJoystickNames();
    dotnet::Dictionary<std::string, int> dictionary;
    int num = static_cast<int>(joystickNames.size());
    while (num-- > 0) {
        std::string text = TrimDeviceName(joystickNames[static_cast<size_t>(num)]);
        if (text.empty()) {
            text = "Joystick " + dotnet::FormatInt64(num);  // + Debug.LogWarning("... renaming as Joystick n")
        }
        dictionary.Set(text, num);
    }
    devices_ = std::move(dictionary);
    // Debug.Log("[Input Devices]: " + PrintCollection(...)): host log only
}

// InputDevices.cs:36
void InputDevices::Load(const ConfigNode& node) {
    devices_.Clear();
    for (const ConfigNode::Value& value : node.values()) {
        devices_.Add(dotnet::Trim(value.name), ParseInt(value.value));
    }
    InitializeDevices();
}

// InputDevices.cs:46
void InputDevices::Save(ConfigNode& node) const {
    for (const auto& kv : devices_) node.AddValue(kv.first, static_cast<int32_t>(kv.second));
}

// InputDevices.cs:54
int InputDevices::GetDeviceIndex(const std::string& deviceName) const {
    int value = 0;
    if (devices_.TryGetValue(deviceName, value)) return value;
    return -1;
}

// InputDevices.cs:63
int InputDevices::GetDeviceListLength() const { return devices_.Count(); }

// InputDevices.cs:73
std::string InputDevices::TrimDeviceName(const std::string& src) { return dotnet::Trim(src); }

}  // namespace ksp
