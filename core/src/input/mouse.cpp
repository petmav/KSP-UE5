// WP-50: Mouse.cs.
#include "ksp/input/mouse.hpp"

#include "ksp/dotnet/mono_float.hpp"
#include "ksp/input/game_settings.hpp"
#include "ksp/input/input_device.hpp"

namespace ksp {

// Mouse.cs:41
void Mouse::MouseButton::Update() {
    if (abort_) {
        if (!isTapStarted_) abort_ = false;
        return;
    }
    const IInputDevice& input = InputEnvironment::Input();
    down_ = input.GetMouseButtonDown(buttonIndex_);
    up_ = input.GetMouseButtonUp(buttonIndex_);
    button_ = input.GetMouseButton(buttonIndex_);
    if (down_) {
        pAtBtnDown_ = Mouse::screenPos;
    } else if (button_) {
        dragDelta_ = Mouse::screenPos - pAtBtnDown_;
    }
    if (!isTapStarted_) {
        tap_ = false;
        doubleTap_ = false;
        if (down_) StartTapRoutine();
    }
}

// kspcore state machine for Mouse.cs:61 MouseButton.TapRoutine (the first segment, up to the first `yield return null`)
void Mouse::MouseButton::StartTapRoutine() {
    isTapStarted_ = true;
    endTime2_ = f32store(fx(InputEnvironment::realtimeSinceStartup) + fx(GameSettings::DOUBLECLICK_MOUSESPEED));
    tapped_ = false;
    phase_ = Phase::WaitUp;
    justStarted_ = true;
    RunTapRoutine();
}

// kspcore state machine for Mouse.cs:61 MouseButton.TapRoutine (a resume after `yield return null`)
void Mouse::MouseButton::ResumeTapRoutine() {
    if (phase_ == Phase::Done) return;
    if (justStarted_) {  // `yield return null` resumes in the NEXT frame's coroutine pass
        justStarted_ = false;
        return;
    }
    RunTapRoutine();
}

// kspcore state machine for Mouse.cs:61 MouseButton.TapRoutine: while (!abort && !(now >= endTime)) { if (up) { tap = tapped = true; yield; tap = false; break; } yield; }
// then, when tapped, the same wait for a second button down (doubleTap).
void Mouse::MouseButton::RunTapRoutine() {
    const IInputDevice& input = InputEnvironment::Input();
    for (;;) {
        switch (phase_) {
            case Phase::WaitUp: {
                if (!abort_ && !(InputEnvironment::realtimeSinceStartup >= endTime2_)) {
                    if (input.GetMouseButtonUp(buttonIndex_)) {
                        tap_ = true;
                        tapped_ = true;
                        phase_ = Phase::TapYield;
                        return;  // yield return null
                    }
                    return;  // yield return null
                }
                // the first loop is over
                if (tapped_) {
                    endTime2_ = f32store(fx(InputEnvironment::realtimeSinceStartup) + fx(GameSettings::DOUBLECLICK_MOUSESPEED));
                    phase_ = Phase::WaitDown;
                    continue;
                }
                abort_ = false;
                isTapStarted_ = false;
                phase_ = Phase::Done;
                return;
            }
            case Phase::TapYield: {
                tap_ = false;  // after the yield; break out of the first loop
                if (tapped_) {
                    endTime2_ = f32store(fx(InputEnvironment::realtimeSinceStartup) + fx(GameSettings::DOUBLECLICK_MOUSESPEED));
                    phase_ = Phase::WaitDown;
                    continue;
                }
                abort_ = false;
                isTapStarted_ = false;
                phase_ = Phase::Done;
                return;
            }
            case Phase::WaitDown: {
                if (!abort_ && !(InputEnvironment::realtimeSinceStartup >= endTime2_)) {
                    if (input.GetMouseButtonDown(buttonIndex_)) {
                        doubleTap_ = true;
                        tap_ = false;
                        doubleClickTime_ = f32store(fx(InputEnvironment::realtimeSinceStartup) + fx(GameSettings::DOUBLECLICK_MOUSESPEED));
                        // break
                    } else {
                        return;  // yield return null
                    }
                }
                abort_ = false;
                isTapStarted_ = false;
                phase_ = Phase::Done;
                return;
            }
            case Phase::Done:
                return;
        }
    }
}

// Mouse.cs:109
void Mouse::MouseButton::ClearMouseState() {
    doubleClickTime_ = InputEnvironment::realtimeSinceStartup;
    tap_ = false;
    doubleTap_ = false;
    down_ = false;
    up_ = false;
    button_ = false;
    abort_ = true;
}

// Mouse.cs:140
bool Mouse::MouseButton::GetDoubleClick(bool isDelegate) const {
    if (isDelegate && InputEnvironment::realtimeSinceStartup < doubleClickTime_) return true;
    return doubleTap_;
}

// Mouse.cs:154
bool Mouse::MouseButton::WasDragging(float delta) const {
    if (!button_ && !up_) return false;
    return dragDelta_.sqrMagnitude() > delta;
}

// Mouse.cs:194
bool Mouse::IsMoving() { return delta.sqrMagnitude() > 0.01f; }

// Mouse.cs:223
void Mouse::Update() {
    Left.Update();
    Right.Update();
    Middle.Update();
    const Vector3f mp = InputEnvironment::Input().mousePosition();
    screenPos = Vector2f(mp.x, mp.y);
    screenPos.y = f32store(fx(static_cast<float>(InputEnvironment::screenHeight)) - fx(screenPos.y));
    delta = screenPos - lastPos_;
    lastPos_ = screenPos;
    HoveredPart = hoveredPartProvider ? hoveredPartProvider() : nullptr;
}

void Mouse::ResumeCoroutines() {
    Left.ResumeTapRoutine();
    Right.ResumeTapRoutine();
    Middle.ResumeTapRoutine();
}

// Mouse.cs:235
Mouse::Buttons Mouse::GetAllMouseButtons() {
    const IInputDevice& input = InputEnvironment::Input();
    Buttons buttons = Buttons::None;
    if (input.GetMouseButton(0)) buttons = buttons | Buttons::Left;
    if (input.GetMouseButton(1)) buttons = buttons | Buttons::Right;
    if (input.GetMouseButton(2)) buttons = buttons | Buttons::Middle;
    if (input.GetMouseButton(3)) buttons = buttons | Buttons::Btn4;
    if (input.GetMouseButton(4)) buttons = buttons | Buttons::Btn5;
    return buttons;
}

// Mouse.cs:261
Mouse::Buttons Mouse::GetAllMouseButtonsUp() {
    const IInputDevice& input = InputEnvironment::Input();
    Buttons buttons = Buttons::None;
    if (input.GetMouseButtonUp(0)) buttons = buttons | Buttons::Left;
    if (input.GetMouseButtonUp(1)) buttons = buttons | Buttons::Right;
    if (input.GetMouseButtonUp(2)) buttons = buttons | Buttons::Middle;
    if (input.GetMouseButtonUp(3)) buttons = buttons | Buttons::Btn4;
    if (input.GetMouseButtonUp(4)) buttons = buttons | Buttons::Btn5;
    return buttons;
}

// Mouse.cs:287
Mouse::Buttons Mouse::GetAllMouseButtonsDown() {
    const IInputDevice& input = InputEnvironment::Input();
    Buttons buttons = Buttons::None;
    if (input.GetMouseButtonDown(0)) buttons = buttons | Buttons::Left;
    if (input.GetMouseButtonDown(1)) buttons = buttons | Buttons::Right;
    if (input.GetMouseButtonDown(2)) buttons = buttons | Buttons::Middle;
    if (input.GetMouseButtonDown(3)) buttons = buttons | Buttons::Btn4;
    if (input.GetMouseButtonDown(4)) buttons = buttons | Buttons::Btn5;
    return buttons;
}

// Mouse.cs:313
bool Mouse::CheckButtons(Buttons buttons, Buttons buttonsToTest, bool strict) {
    if (strict) return (buttons & buttonsToTest) == buttonsToTest;
    return (buttons & buttonsToTest) != Buttons::None;
}

void Mouse::ResetStatics() {
    Left = MouseButton(0);
    Right = MouseButton(1);
    Middle = MouseButton(2);
    screenPos = Vector2f::zero();
    lastPos_ = Vector2f::zero();
    delta = Vector2f::zero();
    HoveredPart = nullptr;
    hoveredPartProvider = nullptr;
}

}  // namespace ksp
