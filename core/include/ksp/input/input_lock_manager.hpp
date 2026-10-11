// WP-50: InputLockManager (Assembly-CSharp/InputLockManager.cs): the lock stack (lockID -> ControlTypes mask), the OR-ed
// lockMask and the onInputLocksModified / onGUILock / onGUIUnlock events. Static like the C# class. The lock stack is a
// ksp::dotnet::Dictionary, so PrintLockStack lists the IDs in .NET Dictionary enumeration order (insertion order until a removal
// leaves a hole that the next insert reuses).
#pragma once

#include <cstdint>
#include <string>

#include "ksp/dotnet/collections.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/input/control_types.hpp"

namespace ksp {

struct InputLockManager {
    using FromToAction = events::GameEvents::FromToAction<ControlTypes, ControlTypes>;

    static inline dotnet::Dictionary<std::string, uint64_t> lockStack;  // :8
    static inline uint64_t lockMask = 0;                               // :10

    static uint64_t LockMask() { return lockMask; }                    // :12

    static ControlTypes SetControlLock(ControlTypes locks, const std::string& lockID);  // :24
    static ControlTypes SetControlLock(const std::string& lockID);                      // :38
    static void RemoveControlLock(const std::string& lockID);                           // :43
    static void ClearControlLocks();                                                    // :58
    static ControlTypes GetControlLock(const std::string& lockID);                      // :66
    static bool IsLocked(ControlTypes controlType);                                     // :75
    static bool IsUnlocked(ControlTypes controlType);                                   // :80
    static bool IsAllLocked(ControlTypes mask);                                         // :85
    static bool IsAnyUnlocked(ControlTypes mask);                                       // :90
    static bool IsLocked(ControlTypes controlType, ControlTypes refMask);               // :95
    static bool IsUnlocked(ControlTypes controlType, ControlTypes refMask);             // :100
    static bool IsLocking(ControlTypes controlType, const FromToAction& refMasks);      // :105
    static bool IsUnlocking(ControlTypes controlType, const FromToAction& refMasks);    // :114
    static std::string PrintLockStack();                                                // :123
    static void DebugLockStack();                                                       // :137
    static uint64_t ControlLocks(int lower, int upper);                                 // :142

    // tests / host: forget every lock without firing events (a fresh process)
    static void Reset();

private:
    static void RecalcMask();                                                           // :14
};

}  // namespace ksp
