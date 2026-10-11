// WP-50: Mouse (Assembly-CSharp/Mouse.cs): the three tracked buttons with tap / double-tap / drag state and the button-mask helpers.
// MouseButton.TapRoutine is a Unity coroutine; here it is an explicit state machine whose first segment runs when the button goes
// down (StartCoroutine runs to the first `yield return null`) and which the host resumes once per frame AFTER every Update
// (Mouse::ResumeCoroutines(), Unity's coroutine pass). Hover-part raycasting (Mouse.CheckHoveredPart: Physics.Raycast over
// Part.layerMask) is the host seam `Mouse::hoveredPartProvider`.
#pragma once

#include <cstdint>
#include <functional>

#include "ksp/unity/vector.hpp"

namespace ksp {

class Part;

// Mouse.cs:8
class MouseButton {
public:
    MouseButton(int button) : buttonIndex_(button) {}   // :35 (the owner is the global Mouse)
    void Update();                                      // :41
    void ClearMouseState();                             // :109
    bool GetButtonDown() const { return down_; }        // :120
    bool GetButtonUp() const { return up_; }            // :125
    bool GetButton() const { return button_; }          // :130
    bool GetClick() const { return tap_; }              // :135
    bool GetDoubleClick(bool isDelegate = false) const; // :140
    Vector2f GetDragDelta() const { return dragDelta_; }   // :149
    bool WasDragging(float delta = 400.0f) const;       // :154
    void ResumeTapRoutine();                            // kspcore: the TapRoutine coroutine (Mouse.cs:61), after a `yield return null`
    bool isTapStarted() const { return isTapStarted_; }

private:
    enum class Phase { Done, WaitUp, TapYield, WaitDown };
    void StartTapRoutine();                             // kspcore: the TapRoutine coroutine (Mouse.cs:61), StartCoroutine runs the first segment
    void RunTapRoutine();

    bool button_ = false, down_ = false, up_ = false, tap_ = false, doubleTap_ = false, abort_ = false;
    float doubleClickTime_ = 0.0f;
    Vector2f dragDelta_ = Vector2f::zero();
    Vector2f pAtBtnDown_ = Vector2f::zero();
    int buttonIndex_ = 0;
    bool isTapStarted_ = false;
    Phase phase_ = Phase::Done;
    float endTime2_ = 0.0f;
    bool tapped_ = false;
    bool justStarted_ = false;   // started in this frame's Update: first resumed in the next frame's coroutine pass
};

class Mouse {
public:
    using MouseButton = ksp::MouseButton;   // Mouse.cs:7 (a namespace-level class here: its default member initializers must be complete where Left / Right / Middle are defined)

    // Mouse.cs:164 ([Flags])
    enum class Buttons : int { None = 0, Left = 1, Right = 2, Middle = 4, Btn4 = 8, Btn5 = 0x10, Any = -1 };

    static inline MouseButton Left{0};     // :186
    static inline MouseButton Right{1};    // :188
    static inline MouseButton Middle{2};   // :190
    static inline Vector2f screenPos = Vector2f::zero();   // :56
    static inline Vector2f delta = Vector2f::zero();       // :154
    static inline Part* HoveredPart = nullptr;             // :192
    static inline std::function<Part*()> hoveredPartProvider;   // Mouse.CheckHoveredPart (:322): unset = no part under the cursor

    static bool IsMoving();                                // :194
    static void Update();                                  // :223
    static void ResumeCoroutines();                        // Unity's coroutine pass for the three TapRoutines
    static Buttons GetAllMouseButtons();                   // :235
    static Buttons GetAllMouseButtonsUp();                 // :261
    static Buttons GetAllMouseButtonsDown();               // :287
    static bool CheckButtons(Buttons buttons, Buttons buttonsToTest, bool strict = true);  // :313
    static void ResetStatics();

private:
    static inline Vector2f lastPos_ = Vector2f::zero();    // :186
};

constexpr Mouse::Buttons operator|(Mouse::Buttons a, Mouse::Buttons b) {
    return static_cast<Mouse::Buttons>(static_cast<int>(a) | static_cast<int>(b));
}
constexpr Mouse::Buttons operator&(Mouse::Buttons a, Mouse::Buttons b) {
    return static_cast<Mouse::Buttons>(static_cast<int>(a) & static_cast<int>(b));
}

}  // namespace ksp
