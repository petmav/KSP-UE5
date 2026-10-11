// WP-50: KSP/UI input-lock helpers.
#include "ksp/input/ui_input_locks.hpp"

#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/enum_parse.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/input/input_device.hpp"
#include "ksp/input/input_lock_manager.hpp"

namespace ksp::UI {

namespace {
std::string AsciiLower(std::string s) {
    for (char& c : s)
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    return s;
}
}  // namespace

// Mono Enum.cs:262 TryParseEnum with ignoreCase
ControlTypes ParseControlTypesIgnoreCase(const std::string& text) {
    const std::string value = dotnet::Trim(text);
    if (value.empty()) throw ksp::ArgumentException("Must specify valid information for parsing in the string.");
    if ((value[0] >= '0' && value[0] <= '9') || value[0] == '-' || value[0] == '+') {
        ControlTypes parsed{};
        if (dotnet::EnumTryParseFull<ControlTypes>(value, parsed)) return parsed;  // the numeric path (OverflowException propagates)
    }
    const auto& table = dotnet::detail::GetTable<ControlTypes>();
    uint64_t acc = 0;
    for (const std::string& part : dotnet::Split(value, ",", false)) {
        const std::string name = AsciiLower(dotnet::Trim(part));
        bool found = false;
        for (size_t j = 0; j < table.names.size(); ++j) {
            if (AsciiLower(table.names[j]) != name) continue;
            acc |= table.values[j];
            found = true;
            break;
        }
        if (!found) throw ksp::ArgumentException("Requested value '" + part + "' was not found.");
    }
    return static_cast<ControlTypes>(acc);
}

uint64_t ComputeLockMask(const std::vector<std::string>& names, const std::string& ownerName) {
    uint64_t mask = 0;
    for (const std::string& n : names) {
        try {
            mask |= static_cast<uint64_t>(ParseControlTypesIgnoreCase(n));
        } catch (const std::exception& ex) {
            log::Info(ownerName + ": " + ex.what());  // Debug.Log("CanvasGroupInputLock '<name>': " + ex.Message)
        }
    }
    return mask;
}

// CanvasGroupInputLock.cs:45
void CanvasGroupInputLock::UpdateLockMask() {
    lockMask_ = ComputeLockMask(inputLockMask_, gameObjectName);
    lockMaskDirty_ = false;
}

// CanvasGroupInputLock.cs:65
void CanvasGroupInputLock::Awake() {
    InitializeDefaultMask();
    events::GameEvents::onInputLocksModified.Add(this, &CanvasGroupInputLock::OnInputLocksModified);
}

// CanvasGroupInputLock.cs:72
void CanvasGroupInputLock::InitializeDefaultMask() {
    defaultMask_.clear();
    for (const std::string& s : inputLockMask_) defaultMask_.push_back(s);
}

// CanvasGroupInputLock.cs:81
void CanvasGroupInputLock::SetInputMaskToDefault() { SetInputMask(defaultMask_); }

// CanvasGroupInputLock.cs:86
void CanvasGroupInputLock::SetInputMask(const std::vector<std::string>& newMask) {
    const std::vector<std::string> copy = newMask;  // SetInputMask(defaultMask) passes our own list
    inputLockMask_.clear();
    for (const std::string& s : copy) inputLockMask_.push_back(s);
    UpdateLockMask();
}

// CanvasGroupInputLock.cs:96
void CanvasGroupInputLock::OnDestroy() { events::GameEvents::onInputLocksModified.Remove(this, &CanvasGroupInputLock::OnInputLocksModified); }

// CanvasGroupInputLock.cs:101
void CanvasGroupInputLock::Start() {
    if (!disableAllLocking) {
        UpdateLockMask();
        canvasGroup->blocksRaycasts = (InputLockManager::lockMask & lockMask_) == 0;
    }
}

// CanvasGroupInputLock.cs:110
void CanvasGroupInputLock::OnInputLocksModified(events::GameEvents::FromToAction<ControlTypes, ControlTypes> fromTo) {
    if (!disableAllLocking) {
        if (lockMaskDirty_) UpdateLockMask();
        canvasGroup->blocksRaycasts = (static_cast<uint64_t>(fromTo.to) & lockMask_) == 0;
    }
}

// CanvasGroupInputLock.cs:122
std::string CanvasGroupInputLock::ToBits(ControlTypes lockMask) { return ToBits(static_cast<uint64_t>(lockMask)); }

// CanvasGroupInputLock.cs:127
std::string CanvasGroupInputLock::ToBits(uint64_t lockMask) {
    std::string text;
    uint64_t v = lockMask;
    if (v == 0) text = "0";
    while (v != 0) {
        text.insert(text.begin(), (v & 1u) ? '1' : '0');
        v >>= 1;
    }
    if (text.size() < 64) text = std::string(64 - text.size(), '0') + text;
    return text;
}

// KSPGraphicRaycaster.cs:42
void KSPGraphicRaycaster::UpdateLockMask() {
    lockMask_ = ComputeLockMask(inputLockMask_, gameObjectName);
    lockMaskDirty_ = false;
}

// KSPGraphicRaycaster.cs:61
bool KSPGraphicRaycaster::Raycast() {
    if (lockMaskDirty_) UpdateLockMask();
    return lockMask_ == 0 || InputLockManager::IsUnlocked(static_cast<ControlTypes>(lockMask_));
}

// DialogMouseEnterControlLock.cs:12
void DialogMouseEnterControlLock::SetLockType(ControlTypes type) { locktype = static_cast<uint64_t>(type); }

// DialogMouseEnterControlLock.cs:17
void DialogMouseEnterControlLock::SetLockName(const std::string& name) { lockName = name; }

// DialogMouseEnterControlLock.cs:22
void DialogMouseEnterControlLock::Setup(ControlTypes type, const std::string& name) {
    SetLockType(type);
    SetLockName(name);
}

// DialogMouseEnterControlLock.cs:28
void DialogMouseEnterControlLock::OnPointerEnter() { InputLockManager::SetControlLock(static_cast<ControlTypes>(locktype), lockName); }

// DialogMouseEnterControlLock.cs:33
void DialogMouseEnterControlLock::OnPointerExit() { InputLockManager::RemoveControlLock(lockName); }

// DialogMouseEnterControlLock.cs:38
void DialogMouseEnterControlLock::OnDestroy() { InputLockManager::RemoveControlLock(lockName); }

// UIInputExtraMouseButtons.cs:41
void UIInputExtraMouseButtons::Update() {
    if (mouseEntered_) {
        const IInputDevice& input = InputEnvironment::Input();
        if (input.GetMouseButtonUp(0) && leftClick) leftClick();
        if (input.GetMouseButtonUp(1) && rightClick) rightClick();
        if (input.GetMouseButtonUp(2) && middleClick) middleClick();
    }
}

}  // namespace ksp::UI
