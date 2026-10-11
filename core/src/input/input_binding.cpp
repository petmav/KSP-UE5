// WP-50: KeyCodeExtended / InputBinding / KeyBinding / AxisBinding_Single / AxisBinding / AxisKeyBinding(List) / InputDevices.
#include "ksp/input/input_binding.hpp"

#include <cstdlib>

#include "binding_util.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/dotnet/enum_parse.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/input/extended_input.hpp"
#include "ksp/input/game_settings.hpp"
#include "ksp/input/input_lock_manager.hpp"
#include "ksp/space_center/high_logic.hpp"
#include "ksp/unity/mathf.hpp"

namespace ksp {

using namespace input_detail;

// ------------------------------------------------------------------------------------------------ KeyCodeExtended

// KeyCodeExtended.cs:10
bool KeyCodeExtended::isNone() const { return code == KeyCode::None && name.empty(); }

// KeyCodeExtended.cs:22
KeyCodeExtended::KeyCodeExtended() : code(KeyCode::None), name() {}

// KeyCodeExtended.cs:28
KeyCodeExtended::KeyCodeExtended(KeyCode key) : code(key), name() {}

// KeyCodeExtended.cs:34 (try { Enum.Parse } catch (ArgumentException): the text becomes the key name; a null string is
// ArgumentNullException, which is an ArgumentException)
KeyCodeExtended::KeyCodeExtended(const std::string& key) {
    try {
        code = dotnet::EnumParseFull<KeyCode>(key);
        name = "";
    } catch (const ksp::ArgumentException&) {
        code = KeyCode::None;
        name = key;
    }
}

// KeyCodeExtended.cs:48
KeyCodeExtended::KeyCodeExtended(KeyCode key, const std::string& keyName) : code(key), name(keyName) {}

// KeyCodeExtended.cs:54
std::string KeyCodeExtended::ToString() const {
    if (!name.empty()) return name;
    return dotnet::EnumToString(code);
}

// ------------------------------------------------------------------------------------------------ InputBinding

// InputBinding.cs:13
void InputBinding::Load(const ConfigNode& node) {
    if (node.HasValue("switchState")) {
        switchState = dotnet::EnumParseFull<InputBindingModes>(GetValueOrEmpty(node, "switchState"));
    }
    if (node.HasValue("modeMask")) {
        switchState = static_cast<InputBindingModes>(ParseInt(GetValueOrEmpty(node, "modeMask")));
    }
}

// InputBinding.cs:25
void InputBinding::Save(ConfigNode& node) const {
    int num = static_cast<int>(switchState);
    node.AddValue("modeMask", dotnet::FormatInt64(num));
}

// InputBinding.cs:31
bool InputBinding::CompareSwitchState(InputBindingModes switchSt) const {
    if (HighLogic::LoadedSceneIsFlight) {
        if (FlightUIModeControllerSeam::present) {
            InputBindingModes inputBindingModes = InputBindingModes::None;
            FlightUIMode mode = FlightUIModeControllerSeam::mode;
            inputBindingModes = ((mode == FlightUIMode::STAGING || mode != FlightUIMode::DOCKING)
                                     ? InputBindingModes::Staging
                                     : (linRotState ? InputBindingModes::Docking_Rotation : InputBindingModes::Docking_Translation));
            return (static_cast<int>(inputBindingModes) & static_cast<int>(switchSt)) != 0;
        }
        return true;
    }
    return true;
}

// InputBinding.cs:47
bool InputBinding::IsLocked() const { return (InputLockManager::LockMask() & inputLockMask) > 0; }

// ------------------------------------------------------------------------------------------------ KeyBinding

// KeyBinding.cs:26
std::string KeyBinding::name() const {
    if (primary.isNone()) return secondary.ToString();
    return primary.ToString();
}

// KeyBinding.cs:100
KeyBinding::KeyBinding() {}

// KeyBinding.cs:100
KeyBinding::KeyBinding(ControlTypes lockMask) { inputLockMask = static_cast<uint64_t>(lockMask); }

// KeyBinding.cs:100
KeyBinding::KeyBinding(KeyCode main) : primary(main) {}

// KeyBinding.cs:100
KeyBinding::KeyBinding(KeyCode main, ControlTypes lockMask) : primary(main) { inputLockMask = static_cast<uint64_t>(lockMask); }

// KeyBinding.cs:100
KeyBinding::KeyBinding(KeyCode main, InputBindingModes useSwitch) : primary(main) { switchState = useSwitch; }

// KeyBinding.cs:100
KeyBinding::KeyBinding(KeyCode main, InputBindingModes useSwitch, ControlTypes lockMask) : primary(main) {
    switchState = useSwitch;
    inputLockMask = static_cast<uint64_t>(lockMask);
}

// KeyBinding.cs:100
KeyBinding::KeyBinding(KeyCode main, KeyCode alt) : primary(main), secondary(alt) {}

// KeyBinding.cs:100
KeyBinding::KeyBinding(KeyCode main, KeyCode alt, ControlTypes lockMask) : primary(main), secondary(alt) {
    inputLockMask = static_cast<uint64_t>(lockMask);
}

// KeyBinding.cs:100
KeyBinding::KeyBinding(KeyCode main, KeyCode alt, InputBindingModes useSwitch, InputBindingModes useSwitchSecondary)
    : primary(main), secondary(alt) {
    switchState = useSwitch;
    switchStateSecondary = useSwitchSecondary;
}

// KeyBinding.cs:100
KeyBinding::KeyBinding(KeyCode main, KeyCode alt, InputBindingModes useSwitch, InputBindingModes useSwitchSecondary,
                       ControlTypes lockMask)
    : primary(main), secondary(alt) {
    switchState = useSwitch;
    switchStateSecondary = useSwitchSecondary;
    inputLockMask = static_cast<uint64_t>(lockMask);
}

// KeyBinding.cs:109
void KeyBinding::Save(ConfigNode& node) const {
    node.AddValue("primary", primary.ToString());
    node.AddValue("secondary", secondary.ToString());
    node.AddValue("group", dotnet::FormatUInt64(inputLockMask));
    InputBinding::Save(node);
    int num = static_cast<int>(switchStateSecondary);
    node.AddValue("modeMaskSec", dotnet::FormatInt64(num));
}

// KeyBinding.cs:119
void KeyBinding::Load(const ConfigNode& node) {
    primary = KeyCodeExtended(GetValueOrEmpty(node, "primary"));
    secondary = KeyCodeExtended(GetValueOrEmpty(node, "secondary"));
    if (node.HasValue("group")) inputLockMask = ParseULong(GetValueOrEmpty(node, "group"));
    InputBinding::Load(node);
    if (node.HasValue("modeMaskSec")) switchStateSecondary = static_cast<InputBindingModes>(ParseInt(GetValueOrEmpty(node, "modeMaskSec")));
}

}  // namespace ksp
