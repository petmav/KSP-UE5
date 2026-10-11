// WP-50: AxisBinding_Single.cs, AxisBinding.cs, AxisKeyBinding.cs, AxisKeyBindingList.cs.
#include "binding_util.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/input/game_settings.hpp"
#include "ksp/input/input_binding.hpp"
#include "ksp/unity/mathf.hpp"

namespace ksp {

using namespace input_detail;

// AxisBinding_Single.cs:35
AxisBinding_Single::AxisBinding_Single(ControlTypes lockMask) { inputLockMask = static_cast<uint64_t>(lockMask); }

// AxisBinding_Single.cs:40
AxisBinding_Single::AxisBinding_Single(float neutral) { neutralPoint = neutral; }

// AxisBinding_Single.cs:45
AxisBinding_Single::AxisBinding_Single(float neutral, ControlTypes lockMask) {
    neutralPoint = neutral;
    inputLockMask = static_cast<uint64_t>(lockMask);
}

// AxisBinding_Single.cs:51
AxisBinding_Single::AxisBinding_Single(InputBindingModes useSwitch) { switchState = useSwitch; }

// AxisBinding_Single.cs:56
AxisBinding_Single::AxisBinding_Single(InputBindingModes useSwitch, ControlTypes lockMask) {
    switchState = useSwitch;
    inputLockMask = static_cast<uint64_t>(lockMask);
}

// AxisBinding_Single.cs:62
AxisBinding_Single::AxisBinding_Single(InputBindingModes useSwitch, float neutral) {
    switchState = useSwitch;
    neutralPoint = neutral;
}

// AxisBinding_Single.cs:68
AxisBinding_Single::AxisBinding_Single(InputBindingModes useSwitch, float neutral, ControlTypes lockMask) {
    switchState = useSwitch;
    neutralPoint = neutral;
    inputLockMask = static_cast<uint64_t>(lockMask);
}

// AxisBinding_Single.cs:75
AxisBinding_Single::AxisBinding_Single(const std::string& Id, const std::string& Name, bool isInverted) {
    idTag = Id;
    name = Name;
    inverted = isInverted;
}

// AxisBinding_Single.cs:82
AxisBinding_Single::AxisBinding_Single(const std::string& Id, const std::string& Name, bool isInverted, ControlTypes lockMask) {
    idTag = Id;
    name = Name;
    inverted = isInverted;
    inputLockMask = static_cast<uint64_t>(lockMask);
}

// AxisBinding_Single.cs:90
AxisBinding_Single::AxisBinding_Single(const std::string& Id, const std::string& Name, bool isInverted, float sens, float dead_zone,
                                       float axisScale) {
    idTag = Id;
    name = Name;
    inverted = isInverted;
    sensitivity = sens;
    deadzone = dead_zone;
    scale = axisScale;
}

// AxisBinding_Single.cs:100
AxisBinding_Single::AxisBinding_Single(const std::string& Id, const std::string& Name, bool isInverted, float sens, float dead_zone,
                                       float axisScale, ControlTypes lockMask) {
    idTag = Id;
    name = Name;
    inverted = isInverted;
    sensitivity = sens;
    deadzone = dead_zone;
    scale = axisScale;
    inputLockMask = static_cast<uint64_t>(lockMask);
}

// AxisBinding_Single.cs:111
void AxisBinding_Single::Save(ConfigNode& node) const {
    node.AddValue("name", name);
    node.AddValue("axis", static_cast<int32_t>(axisIdx));
    node.AddValue("inv", inverted);
    node.AddValue("sensitivity", sensitivity);
    node.AddValue("deadzone", deadzone);
    node.AddValue("scale", scale);
    node.AddValue("group", dotnet::FormatUInt64(inputLockMask));
    InputBinding::Save(node);
}

// AxisBinding_Single.cs:123
void AxisBinding_Single::Load(const ConfigNode& node) {
    name = "NULL";
    if (node.HasValue("name")) name = GetValueOrEmpty(node, "name");
    if (node.HasValue("axis")) {
        deviceIdx = -1;
        if (node.HasValue("axis")) axisIdx = ParseInt(GetValueOrEmpty(node, "axis"));
        else axisIdx = -1;
    } else if (node.HasValue("id")) {
        idTag = GetValueOrEmpty(node, "id");
        if (dotnet::StartsWith(idTag, "joy")) {
            const std::vector<std::string> parts = dotnet::Split(idTag, ".", false);
            deviceIdx = ParseInt(dotnet::Replace(parts.at(0), "joy", ""));
            if (parts.size() < 2) throw ksp::Exception("Index was outside the bounds of the array.");
            axisIdx = ParseInt(parts[1]);
        } else {
            deviceIdx = -1;
            axisIdx = -1;
        }
    } else {
        deviceIdx = -1;
        axisIdx = -1;
    }
    if (node.HasValue("inv")) inverted = ParseBool(GetValueOrEmpty(node, "inv"));
    if (node.HasValue("sensitivity")) sensitivity = ParseFloatInvariant(GetValueOrEmpty(node, "sensitivity"));
    if (node.HasValue("deadzone")) deadzone = ParseFloatInvariant(GetValueOrEmpty(node, "deadzone"));
    if (node.HasValue("scale")) scale = ParseFloatInvariant(GetValueOrEmpty(node, "scale"));
    if (node.HasValue("group")) inputLockMask = ParseULong(GetValueOrEmpty(node, "group"));
    InputBinding::Load(node);
    if (axisIdx != -1) {
        if (!GameSettings::INPUT_DEVICES) throw NullReferenceException("GameSettings.INPUT_DEVICES is null");
        deviceIdx = GameSettings::INPUT_DEVICES->GetDeviceIndex(name);
        if (deviceIdx != -1) {
            idTag = "joy" + dotnet::FormatInt64(deviceIdx) + "." + dotnet::FormatInt64(axisIdx);
            title = name + "Axis " + dotnet::FormatInt64(axisIdx);
        } else {
            idTag = "None";
            title = "Not Found";
        }
    } else {
        idTag = name;
        title = name;
    }
}

// AxisBinding_Single.cs:203 (Mono float rule: every float expression in double, rounded at each store)
float AxisBinding_Single::GetAxis() {
    if (idTag == "None") return neutralPoint;
    if (!CompareSwitchState(switchState)) return neutralPoint;
    if (IsLocked()) return neutralPoint;
    value_ = f32store(fx(InputEnvironment::Input().GetAxis(idTag)) * fx(scale));
    if (inverted != preinvertAxis) value_ = f32store(fx(value_) * -1.0);
    if (Mathf::Abs(value_) < deadzone) {
        value_ = 0.0f;
    } else {
        if (value_ > 0.0f) value_ = f32store((fx(value_) - fx(deadzone)) * (1.0 / (1.0 - fx(deadzone))));
        if (value_ < 0.0f) value_ = f32store((fx(value_) + fx(deadzone)) * (1.0 / (1.0 - fx(deadzone))));
    }
    if (value_ > 0.0f) value_ = Mathf::Pow(Mathf::Clamp01(value_), sensitivity);
    if (value_ < 0.0f) value_ = f32store(0.0 - fx(Mathf::Pow(Mathf::Clamp01(Mathf::Abs(value_)), sensitivity)));
    return value_;
}

// AxisBinding_Single.cs:248
bool AxisBinding_Single::IsNeutral() const { return const_cast<AxisBinding_Single*>(this)->GetAxis() == 0.0f; }

// AxisBinding.cs:14
float AxisBinding::deadzone() const { return Mathf::Max(primary.deadzone, secondary.deadzone); }

// AxisBinding.cs:24
AxisBinding::AxisBinding(ControlTypes lockMask) : primary(lockMask), secondary(lockMask) {}

// AxisBinding.cs:30
AxisBinding::AxisBinding(float neutral) : primary(neutral), secondary(neutral) {}

// AxisBinding.cs:36
AxisBinding::AxisBinding(float neutral, ControlTypes lockMask) : primary(neutral, lockMask), secondary(neutral, lockMask) {}

// AxisBinding.cs:42
AxisBinding::AxisBinding(InputBindingModes useSwitch) : primary(useSwitch), secondary(useSwitch) {}

// AxisBinding.cs:48
AxisBinding::AxisBinding(InputBindingModes useSwitch, ControlTypes lockMask) : primary(useSwitch, lockMask), secondary(useSwitch, lockMask) {}

// AxisBinding.cs:54
AxisBinding::AxisBinding(InputBindingModes useSwitch, float neutral) : primary(useSwitch, neutral), secondary(useSwitch, neutral) {}

// AxisBinding.cs:60
AxisBinding::AxisBinding(InputBindingModes useSwitch, float neutral, ControlTypes lockMask)
    : primary(useSwitch, neutral, lockMask), secondary(useSwitch, neutral, lockMask) {}

// AxisBinding.cs:66
AxisBinding::AxisBinding(const std::string& Id, const std::string& Name, bool isInverted) : primary(Id, Name, isInverted), secondary() {}

// AxisBinding.cs:72
AxisBinding::AxisBinding(const std::string& Id, const std::string& Name, bool isInverted, ControlTypes lockMask)
    : primary(Id, Name, isInverted, lockMask), secondary(lockMask) {}

// AxisBinding.cs:78
AxisBinding::AxisBinding(const std::string& Id, const std::string& Name, bool isInverted, float sens, float dead_zone, float axisScale)
    : primary(Id, Name, isInverted, sens, dead_zone, axisScale), secondary() {}

// AxisBinding.cs:84
AxisBinding::AxisBinding(const std::string& Id, const std::string& Name, bool isInverted, float sens, float dead_zone, float axisScale,
                         ControlTypes lockMask)
    : primary(Id, Name, isInverted, sens, dead_zone, axisScale, lockMask), secondary(lockMask) {}

// AxisBinding.cs:90
void AxisBinding::Load(const ConfigNode& node) {
    if (node.HasNode("PRIMARY")) primary.Load(*node.GetNode("PRIMARY"));
    if (node.HasNode("SECONDARY")) secondary.Load(*node.GetNode("SECONDARY"));
}

// AxisBinding.cs:102
void AxisBinding::Save(ConfigNode& node) const {
    primary.Save(*node.AddNode("PRIMARY"));
    secondary.Save(*node.AddNode("SECONDARY"));
}

// AxisBinding.cs:108
float AxisBinding::GetAxis() {
    pAxis_ = primary.GetAxis();
    sAxis_ = secondary.GetAxis();
    if (Mathf::Abs(f32store(fx(pAxis_) - fx(primary.neutralPoint))) >= Mathf::Abs(f32store(fx(sAxis_) - fx(secondary.neutralPoint)))) {
        return pAxis_;
    }
    return sAxis_;
}

// AxisBinding.cs:119
bool AxisBinding::IsNeutral() const {
    if (primary.IsNeutral()) return secondary.IsNeutral();
    return false;
}

// AxisKeyBinding.cs:11
AxisKeyBinding::AxisKeyBinding()
    : axisBinding(std::make_shared<AxisBinding>()),
      plusKeyBinding(std::make_shared<KeyBinding>()),
      minusKeyBinding(std::make_shared<KeyBinding>()) {}

// AxisKeyBinding.cs:18
void AxisKeyBinding::Save(ConfigNode& node) const {
    axisBinding->Save(*node.AddNode("AXIS_BINDING"));
    plusKeyBinding->Save(*node.AddNode("PLUS_KEY_BINDING"));
    minusKeyBinding->Save(*node.AddNode("MINUS_KEY_BINDING"));
}

// AxisKeyBinding.cs:25
void AxisKeyBinding::Load(const ConfigNode& node) {
    ConfigNode* node2 = node.GetNode("AXIS_BINDING");
    if (node2 != nullptr) axisBinding->Load(*node2);
    node2 = node.GetNode("PLUS_KEY_BINDING");
    if (node2 != nullptr) plusKeyBinding->Load(*node2);
    node2 = node.GetNode("MINUS_KEY_BINDING");
    if (node2 != nullptr) minusKeyBinding->Load(*node2);
}

// AxisKeyBindingList.cs:25
AxisKeyBindingList::AxisKeyBindingList(int count) { axisKeyBindings_.resize(static_cast<size_t>(count)); }

// AxisKeyBindingList.cs:34
void AxisKeyBindingList::Save(ConfigNode& node) const {
    for (size_t i = 0; i < axisKeyBindings_.size(); ++i) {
        ConfigNode* node2 = node.AddNode("AXIS_KEY_BINDING");
        axisKeyBindings_[i].Save(*node2);
    }
}

// AxisKeyBindingList.cs:43
void AxisKeyBindingList::Load(const ConfigNode& node) {
    std::vector<ConfigNode*> nodes = node.GetNodes("AXIS_KEY_BINDING");
    size_t num = nodes.size();
    if (num > axisKeyBindings_.size()) num = axisKeyBindings_.size();
    for (size_t i = 0; i < num; ++i) axisKeyBindings_[i].Load(*nodes[i]);
}

// AxisKeyBindingList.cs:57
AxisKeyBindingList AxisKeyBindingList::Clone() const {
    AxisKeyBindingList list;
    list.axisKeyBindings_.reserve(axisKeyBindings_.size());
    for (const AxisKeyBinding& b : axisKeyBindings_) list.axisKeyBindings_.push_back(b.Clone());
    return list;
}

}  // namespace ksp
