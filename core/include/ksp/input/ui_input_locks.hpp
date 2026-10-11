// WP-50: the KSP.UI input-lock helpers (Assembly-CSharp/KSP/UI/{CanvasGroupInputLock,KSPGraphicRaycaster,DialogMouseEnterControlLock,
// UIInputExtraMouseButtons}.cs): UI components whose raycast blocking follows InputLockManager. The Unity UI objects they drive
// (CanvasGroup.blocksRaycasts, GraphicRaycaster.Raycast, EventTrigger) are seams: a `bool blocksRaycasts` / the caller of Raycast
// decides on the returned bool / std::function callbacks. MouseEventsHandlerBase (a GameObject-parent search) and
// StandaloneInputModuleCustom (a StandaloneInputModule accessor) have no logic outside the engine and are not ported.
#pragma once

#include <functional>
#include <string>
#include <vector>

#include "ksp/events/game_events.hpp"
#include "ksp/input/control_types.hpp"

namespace ksp::UI {

// Enum.Parse(typeof(ControlTypes), text, ignoreCase: true) (Mono Enum.cs:262 TryParseEnum): throws ksp::ArgumentException /
// dotnet::OverflowException like the runtime
ControlTypes ParseControlTypesIgnoreCase(const std::string& text);

// shared by CanvasGroupInputLock and KSPGraphicRaycaster: the OR of the parsable names, failures logged and skipped
uint64_t ComputeLockMask(const std::vector<std::string>& names, const std::string& ownerName);

// UnityEngine.CanvasGroup (blocksRaycasts only)
struct CanvasGroup {
    bool blocksRaycasts = true;
};

// CanvasGroupInputLock.cs:7
class CanvasGroupInputLock {
public:
    static inline bool disableAllLocking = false;      // :9
    std::string gameObjectName;                        // base.gameObject.name (log text only)
    CanvasGroup* canvasGroup = nullptr;                // :19 (GetComponent<CanvasGroup>() in Awake)

    std::vector<std::string>& InputLockMask() { lockMaskDirty_ = true; return inputLockMask_; }   // :21
    uint64_t LockMask() const { return lockMask_; }                                              // :31
    void SetLockMask(uint64_t value) { lockMask_ = value; lockMaskDirty_ = false; }              // :35

    void Awake();                                      // :64
    void Start();                                      // :91
    void OnDestroy();                                  // :84
    void SetInputMaskToDefault();                      // :78
    void SetInputMask(const std::vector<std::string>& newMask);   // :83
    void OnInputLocksModified(events::GameEvents::FromToAction<ControlTypes, ControlTypes> fromTo);   // :100
    static std::string ToBits(ControlTypes lockMask);  // :111
    static std::string ToBits(uint64_t lockMask);      // :116

private:
    void UpdateLockMask();                             // :47
    void InitializeDefaultMask();                      // :71

    std::vector<std::string> inputLockMask_;
    std::vector<std::string> defaultMask_;
    uint64_t lockMask_ = 0;
    bool lockMaskDirty_ = true;
};

// KSPGraphicRaycaster.cs:9
class KSPGraphicRaycaster {
public:
    std::string gameObjectName;
    std::vector<std::string>& InputLockMask() { lockMaskDirty_ = true; return inputLockMask_; }   // :14
    uint64_t LockMask() const { return lockMask_; }                                              // :24
    void SetLockMask(uint64_t value) { lockMask_ = value; lockMaskDirty_ = false; }              // :28
    // :53 Raycast: true when base.Raycast(eventData, results) must run (no lock mask, or its controls are all unlocked)
    bool Raycast();

private:
    void UpdateLockMask();                             // :36
    std::vector<std::string> inputLockMask_;
    uint64_t lockMask_ = 0;
    bool lockMaskDirty_ = true;
};

// DialogMouseEnterControlLock.cs:6
class DialogMouseEnterControlLock {
public:
    std::string lockName = "DialogMouseEnterControlLock";   // :8
    uint64_t locktype = 1152921504606846975ULL;              // :10

    void SetLockType(ControlTypes type);                     // :12
    void SetLockName(const std::string& name);               // :17
    void Setup(ControlTypes type, const std::string& name);  // :22
    void OnPointerEnter();                                   // :28
    void OnPointerExit();                                    // :33
    void OnDestroy();                                        // :38
};

// UIInputExtraMouseButtons.cs:8
class UIInputExtraMouseButtons {
public:
    std::function<void()> leftClick, rightClick, middleClick;   // :14-18 (Button.ButtonClickedEvent.Invoke)
    void OnMouseEnter() { mouseEntered_ = true; }                // :33
    void OnMouseExit() { mouseEntered_ = false; }                // :38
    void Update();                                               // :43

private:
    bool mouseEntered_ = false;
};

}  // namespace ksp::UI
