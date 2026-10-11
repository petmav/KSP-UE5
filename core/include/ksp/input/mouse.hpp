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

class Mouse {
public:
    // Mouse.cs:8
    class MouseButton {
    public:
        MouseButton(int button) : buttonIndex_(button) {}   // :33 (the owner is the global Mouse)
        void Update();                                      // :38
        void ClearMouseState();                             // :104
        bool GetButtonDown() const { return down_; }        // :116
        bool GetButtonUp() const { return up_; }            // :121
        bool GetButton() const { return button_; }          // :126
        bool GetClick() const { return tap_; }              // :131
        bool GetDoubleClick(bool isDelegate = false) const; // :140
        Vector2f GetDragDelta() const { return dragDelta_; }   // :148
        bool WasDragging(float delta = 400.0f) const;       // :153
        void ResumeTapRoutine();                            // :61 (the coroutine, after a `yield return null`)
        bool isTapStarted() const { return isTapStarted_; }

    private:
        enum class Phase { Done, WaitUp, TapYield, WaitDown };
        void StartTapRoutine();                             // :61 (StartCoroutine: runs the first segment)
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

    // Mouse.cs:164 ([Flags])
    enum class Buttons : int { None = 0, Left = 1, Right = 2, Middle = 4, Btn4 = 8, Btn5 = 0x10, Any = -1 };

    static inline MouseButton Left{0};     // :186
    static inline MouseButton Right{1};    // :188
    static inline MouseButton Middle{2};   // :190
    static inline Vector2f screenPos = Vector2f::zero();   // :192
    static inline Vector2f delta = Vector2f::zero();       // :196
    static inline Part* HoveredPart = nullptr;             // :202
    static inline std::function<Part*()> hoveredPartProvider;   // Mouse.CheckHoveredPart (:322): unset = no part under the cursor

    static bool IsMoving();                                // :204
    static void Update();                                  // :228
    static void ResumeCoroutines();                        // Unity's coroutine pass for the three TapRoutines
    static Buttons GetAllMouseButtons();                   // :242
    static Buttons GetAllMouseButtonsUp();                 // :266
    static Buttons GetAllMouseButtonsDown();               // :290
    static bool CheckButtons(Buttons buttons, Buttons buttonsToTest, bool strict = true);  // :314
    static void ResetStatics();

private:
    static inline Vector2f lastPos_ = Vector2f::zero();    // :194
};

constexpr Mouse::Buttons operator|(Mouse::Buttons a, Mouse::Buttons b) {
    return static_cast<Mouse::Buttons>(static_cast<int>(a) | static_cast<int>(b));
}
constexpr Mouse::Buttons operator&(Mouse::Buttons a, Mouse::Buttons b) {
    return static_cast<Mouse::Buttons>(static_cast<int>(a) & static_cast<int>(b));
}

}  // namespace ksp
