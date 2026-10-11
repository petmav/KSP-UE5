// WP-50: key / axis bindings and their settings.cfg (de)serialisation.
// Spec: Assembly-CSharp/{KeyCodeExtended,InputBinding,InputBindingModes,KeyBinding,AxisBinding_Single,AxisBinding,AxisKeyBinding,
// AxisKeyBindingList,InputDevices}.cs. The ConfigNode text these write is what GameSettings puts in settings.cfg and what the
// shipped GameData/Squad/Controls/*.cfg keyboard layouts contain.
//
// Aliasing: C# classes are references. KeyBinding / AxisBinding hold their sub-objects by value here (KeyCodeExtended is only ever
// replaced, never mutated in place, so a shallow MemberwiseClone is unobservable), AxisKeyBinding keeps shared_ptr members because
// its Clone is a MemberwiseClone that shares the three binding objects with the original (Load through the clone changes both).
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/dotnet/collections.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/input/control_types.hpp"
#include "ksp/input/input_device.hpp"
#include "ksp/input/keycode.hpp"

namespace ksp {

// FlightUIMode.cs:1 (event type forward-declared in events/game_event_types.hpp)
enum class FlightUIMode : int { STAGING = 0, DOCKING = 1, MAPMODE = 2, MANEUVER_EDIT = 3, MANEUVER_INFO = 4 };

// InputBindingModes.cs:4 ([Flags])
enum class InputBindingModes : int {
    None = 0,
    Staging = 1,
    Docking_Translation = 2,
    Docking_Rotation = 4,
    RotationModes = 5,
    DockingModes = 6,
    Any = -1
};
constexpr InputBindingModes operator|(InputBindingModes a, InputBindingModes b) {
    return static_cast<InputBindingModes>(static_cast<int>(a) | static_cast<int>(b));
}
constexpr InputBindingModes operator&(InputBindingModes a, InputBindingModes b) {
    return static_cast<InputBindingModes>(static_cast<int>(a) & static_cast<int>(b));
}

// FlightUIModeController.Instance (null = not present) and its Mode, as InputBinding.CompareSwitchState reads them
struct FlightUIModeControllerSeam {
    static inline bool present = false;
    static inline FlightUIMode mode = FlightUIMode::STAGING;
};

// KeyCodeExtended.cs:4
class KeyCodeExtended {
public:
    KeyCode code = KeyCode::None;
    std::string name;  // "" for the C# null / empty string

    bool isNone() const;                                  // :10
    KeyCodeExtended();                                    // :22
    explicit KeyCodeExtended(KeyCode key);                // :28
    explicit KeyCodeExtended(const std::string& key);     // :34
    KeyCodeExtended(KeyCode key, const std::string& keyName);  // :48
    std::string ToString() const;                         // :54
};

// InputBinding.cs:3
class InputBinding {
public:
    InputBindingModes switchState = InputBindingModes::Any;
    static inline bool linRotState = false;
    bool useSwitchState = false;
    uint64_t inputLockMask = 0;

    virtual ~InputBinding() = default;
    virtual void Load(const ConfigNode& node);     // :13
    virtual void Save(ConfigNode& node) const;     // :25
    bool CompareSwitchState(InputBindingModes switchSt) const;  // :31
    bool IsLocked() const;                         // :47
    bool IsUnlocked() const { return !IsLocked(); }  // :52
    virtual bool IsNeutral() const { return true; }  // :57
};

// KeyBinding.cs:4
class KeyBinding : public InputBinding {
public:
    KeyCodeExtended primary;
    KeyCodeExtended secondary;
    InputBindingModes switchStateSecondary = InputBindingModes::Any;

    static constexpr float doubleTapInterval = 0.2f;  // :12

    std::string name() const;  // :26

    KeyBinding();                                                              // :38
    explicit KeyBinding(ControlTypes lockMask);                                // :44
    explicit KeyBinding(KeyCode main);                                         // :51
    KeyBinding(KeyCode main, ControlTypes lockMask);                           // :57
    KeyBinding(KeyCode main, InputBindingModes useSwitch);                     // :64
    KeyBinding(KeyCode main, InputBindingModes useSwitch, ControlTypes lockMask);  // :71
    KeyBinding(KeyCode main, KeyCode alt);                                     // :79
    KeyBinding(KeyCode main, KeyCode alt, ControlTypes lockMask);              // :85
    KeyBinding(KeyCode main, KeyCode alt, InputBindingModes useSwitch, InputBindingModes useSwitchSecondary);  // :92
    KeyBinding(KeyCode main, KeyCode alt, InputBindingModes useSwitch, InputBindingModes useSwitchSecondary,
               ControlTypes lockMask);                                         // :162

    void Save(ConfigNode& node) const override;     // :109
    void Load(const ConfigNode& node) override;     // :119
    bool GetKey(bool ignoreInputLock = false) const;        // :134
    bool GetKeyDown(bool ignoreInputLock = false) const;    // :151
    bool GetDoubleTapDown(bool ignoreInputLock = false);    // :168
    bool GetKeyUp(bool ignoreInputLock = false) const;      // :187
    bool GetDoubleTapUp(bool ignoreInputLock = false);      // :204
    bool IsNeutral() const override;                        // :223

private:
    int lastKeyDownFrameCount_ = 0;
    float lastKeyDownTime_ = 0.0f;
    bool doubleDown_ = false;
    int lastKeyUpFrameCount_ = 0;
    float lastKeyUpTime_ = 0.0f;
    bool doubleUp_ = false;
};

// InputDevices.cs:4
class InputDevices {
public:
    InputDevices();                                    // :8
    void InitializeDevices();                          // :17
    void Load(const ConfigNode& node);                 // :36
    void Save(ConfigNode& node) const;                 // :46
    int GetDeviceIndex(const std::string& deviceName) const;  // :54
    int GetDeviceListLength() const;                   // :63
    static std::string TrimDeviceName(const std::string& src);  // :73
    const dotnet::Dictionary<std::string, int>& Devices() const { return devices_; }

private:
    dotnet::Dictionary<std::string, int> devices_;
};

// AxisBinding_Single.cs:5
class AxisBinding_Single : public InputBinding {
public:
    std::string idTag = "None";
    std::string name = "None";
    std::string title = "None";
    int deviceIdx = -1;
    int axisIdx = -1;
    bool preinvertAxis = false;
    bool inverted = false;
    float sensitivity = 1.0f;
    float deadzone = 0.05f;
    float scale = 1.0f;
    float neutralPoint = 0.0f;

    AxisBinding_Single() = default;                                             // :31
    explicit AxisBinding_Single(ControlTypes lockMask);                         // :31
    explicit AxisBinding_Single(float neutral);                                 // :40
    AxisBinding_Single(float neutral, ControlTypes lockMask);                   // :45
    explicit AxisBinding_Single(InputBindingModes useSwitch);                   // :51
    AxisBinding_Single(InputBindingModes useSwitch, ControlTypes lockMask);     // :56
    AxisBinding_Single(InputBindingModes useSwitch, float neutral);             // :62
    AxisBinding_Single(InputBindingModes useSwitch, float neutral, ControlTypes lockMask);  // :68
    AxisBinding_Single(const std::string& Id, const std::string& Name, bool isInverted);   // :75
    AxisBinding_Single(const std::string& Id, const std::string& Name, bool isInverted, ControlTypes lockMask);  // :82
    AxisBinding_Single(const std::string& Id, const std::string& Name, bool isInverted, float sens, float dead_zone,
                       float axisScale);                                        // :86
    AxisBinding_Single(const std::string& Id, const std::string& Name, bool isInverted, float sens, float dead_zone,
                       float axisScale, ControlTypes lockMask);                 // :96

    void Save(ConfigNode& node) const override;     // :111
    void Load(const ConfigNode& node) override;     // :123
    float GetAxis();                                // :203
    bool IsNeutral() const override;                // :248

private:
    float value_ = 0.0f;
};

// AxisBinding.cs:4
class AxisBinding {
public:
    AxisBinding_Single primary;
    AxisBinding_Single secondary;

    float deadzone() const;                      // :14
    float scale() const { return primary.scale; }  // :16

    AxisBinding() = default;                     // :18
    explicit AxisBinding(ControlTypes lockMask);  // :24
    explicit AxisBinding(float neutral);          // :30
    AxisBinding(float neutral, ControlTypes lockMask);  // :36
    explicit AxisBinding(InputBindingModes useSwitch);  // :42
    AxisBinding(InputBindingModes useSwitch, ControlTypes lockMask);  // :48
    AxisBinding(InputBindingModes useSwitch, float neutral);          // :54
    AxisBinding(InputBindingModes useSwitch, float neutral, ControlTypes lockMask);  // :60
    AxisBinding(const std::string& Id, const std::string& Name, bool isInverted);    // :66
    AxisBinding(const std::string& Id, const std::string& Name, bool isInverted, ControlTypes lockMask);  // :72
    AxisBinding(const std::string& Id, const std::string& Name, bool isInverted, float sens, float dead_zone,
                float axisScale);                // :78
    AxisBinding(const std::string& Id, const std::string& Name, bool isInverted, float sens, float dead_zone, float axisScale,
                ControlTypes lockMask);          // :84

    void Load(const ConfigNode& node);           // :90
    void Save(ConfigNode& node) const;           // :102
    float GetAxis();                             // :108
    bool IsNeutral() const;                      // :119

private:
    float pAxis_ = 0.0f;
    float sAxis_ = 0.0f;
};

// AxisKeyBinding.cs:3
class AxisKeyBinding {
public:
    std::shared_ptr<AxisBinding> axisBinding;
    std::shared_ptr<KeyBinding> plusKeyBinding;
    std::shared_ptr<KeyBinding> minusKeyBinding;

    AxisKeyBinding();                    // :11
    void Save(ConfigNode& node) const;   // :18
    void Load(const ConfigNode& node);   // :25
    // :42 MemberwiseClone: the copy shares the three binding objects
    AxisKeyBinding Clone() const { return *this; }
};

// AxisKeyBindingList.cs:3
class AxisKeyBindingList {
public:
    explicit AxisKeyBindingList(int count);              // :25
    AxisKeyBinding& operator[](int index) { return axisKeyBindings_[static_cast<size_t>(index)]; }  // :11
    const AxisKeyBinding& operator[](int index) const { return axisKeyBindings_[static_cast<size_t>(index)]; }
    int Length() const { return static_cast<int>(axisKeyBindings_.size()); }  // :19
    void Save(ConfigNode& node) const;                   // :34
    void Load(const ConfigNode& node);                   // :43
    AxisKeyBindingList Clone() const;                    // :57

private:
    AxisKeyBindingList() = default;                      // :21
    std::vector<AxisKeyBinding> axisKeyBindings_;
};

}  // namespace ksp

template <>
struct ksp::dotnet::EnumTraits<ksp::InputBindingModes> {
    static constexpr bool flags = true;
    static constexpr EnumMember<ksp::InputBindingModes> members[] = {
        {ksp::InputBindingModes::None, "None"},
        {ksp::InputBindingModes::Staging, "Staging"},
        {ksp::InputBindingModes::Docking_Translation, "Docking_Translation"},
        {ksp::InputBindingModes::Docking_Rotation, "Docking_Rotation"},
        {ksp::InputBindingModes::RotationModes, "RotationModes"},
        {ksp::InputBindingModes::DockingModes, "DockingModes"},
        {ksp::InputBindingModes::Any, "Any"}};
};
