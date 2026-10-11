// WP-50: InputLockManager.cs.
#include "ksp/input/input_lock_manager.hpp"

#include "ksp/localization/localizer.hpp"

namespace ksp {

using GE = events::GameEvents;

// InputLockManager.cs:14
void InputLockManager::RecalcMask() {
    lockMask = 0;
    for (const auto& kv : lockStack) lockMask |= kv.second;
}

// InputLockManager.cs:24
ControlTypes InputLockManager::SetControlLock(ControlTypes locks, const std::string& lockID) {
    ControlTypes controlTypes = static_cast<ControlTypes>(lockMask);
    lockStack.Set(lockID, static_cast<uint64_t>(locks));
    RecalcMask();
    GE::onInputLocksModified.Fire(FromToAction(controlTypes, static_cast<ControlTypes>(lockMask)));
    if ((controlTypes & ControlTypes::GUI) != ControlTypes::None && (lockMask & 0x8000000000000ULL) == 0) {
        GE::onGUILock.Fire();
        GE::onTooltipDestroyRequested.Fire();
    }
    return locks;
}

// InputLockManager.cs:38
ControlTypes InputLockManager::SetControlLock(const std::string& lockID) { return SetControlLock(ControlTypes::All, lockID); }

// InputLockManager.cs:43
void InputLockManager::RemoveControlLock(const std::string& lockID) {
    if (lockStack.ContainsKey(lockID)) {
        ControlTypes controlTypes = static_cast<ControlTypes>(lockMask);
        lockStack.Remove(lockID);
        RecalcMask();
        GE::onInputLocksModified.Fire(FromToAction(controlTypes, static_cast<ControlTypes>(lockMask)));
        if ((controlTypes & ControlTypes::GUI) == ControlTypes::None && (lockMask & 0x8000000000000ULL) != 0) {
            GE::onGUIUnlock.Fire();
        }
    }
}

// InputLockManager.cs:58
void InputLockManager::ClearControlLocks() {
    ControlTypes from = static_cast<ControlTypes>(lockMask);
    lockStack.Clear();
    lockMask = 0;
    GE::onInputLocksModified.Fire(FromToAction(from, static_cast<ControlTypes>(lockMask)));
}

// InputLockManager.cs:66
ControlTypes InputLockManager::GetControlLock(const std::string& lockID) {
    if (lockStack.ContainsKey(lockID)) return static_cast<ControlTypes>(lockStack.At(lockID));
    return ControlTypes::None;
}

// InputLockManager.cs:75
bool InputLockManager::IsLocked(ControlTypes controlType) { return (lockMask & static_cast<uint64_t>(controlType)) > 0; }

// InputLockManager.cs:80
bool InputLockManager::IsUnlocked(ControlTypes controlType) { return (lockMask & static_cast<uint64_t>(controlType)) == 0; }

// InputLockManager.cs:85
bool InputLockManager::IsAllLocked(ControlTypes mask) { return (lockMask & static_cast<uint64_t>(mask)) == static_cast<uint64_t>(mask); }

// InputLockManager.cs:90
bool InputLockManager::IsAnyUnlocked(ControlTypes mask) { return (lockMask & static_cast<uint64_t>(mask)) != static_cast<uint64_t>(mask); }

// InputLockManager.cs:95
bool InputLockManager::IsLocked(ControlTypes controlType, ControlTypes refMask) {
    return static_cast<uint64_t>(refMask & controlType) > 0;
}

// InputLockManager.cs:100
bool InputLockManager::IsUnlocked(ControlTypes controlType, ControlTypes refMask) { return (refMask & controlType) == ControlTypes::None; }

// InputLockManager.cs:105
bool InputLockManager::IsLocking(ControlTypes controlType, const FromToAction& refMasks) {
    if (IsUnlocked(controlType, refMasks.from)) return IsLocked(controlType, refMasks.to);
    return false;
}

// InputLockManager.cs:114
bool InputLockManager::IsUnlocking(ControlTypes controlType, const FromToAction& refMasks) {
    if (IsLocked(controlType, refMasks.from)) return IsUnlocked(controlType, refMasks.to);
    return false;
}

// InputLockManager.cs:123
std::string InputLockManager::PrintLockStack() {
    if (lockStack.Count() == 0) return Localizer::Format("#autoLOC_176849");
    std::string text = Localizer::Format("#autoLOC_176851");
    for (const auto& kv : lockStack) text = text + kv.first + "\n";
    // Convert.ToString((long)lockMask, 2): two's complement digits, no leading zeros ("0" for 0)
    std::string bits;
    uint64_t v = lockMask;
    if (v == 0) bits = "0";
    while (v != 0) {
        bits.insert(bits.begin(), (v & 1u) ? '1' : '0');
        v >>= 1;
    }
    return text + "-- \nbitmask: " + bits;
}

// InputLockManager.cs:137 (Debug.Log of the text: the log sink is the host's, nothing to do here)
void InputLockManager::DebugLockStack() { (void)PrintLockStack(); }

// InputLockManager.cs:142
uint64_t InputLockManager::ControlLocks(int lower, int upper) {
    if (lower > upper) return 0;
    int num = upper - lower + 1;
    if (num >= 64) return 0;
    uint64_t num2 = (uint64_t{1} << num) - uint64_t{1};  // (ulong)((1L << num) - 1L), unchecked
    // C# masks the shift count of a ulong shift to 6 bits; `-lower` is an int negation
    const int shl = lower & 63;
    const int shr = static_cast<int>(0u - static_cast<uint32_t>(lower)) & 63;
    return (num2 << shl) | (num2 >> shr);
}

void InputLockManager::Reset() {
    lockStack.Clear();
    lockMask = 0;
}

}  // namespace ksp
