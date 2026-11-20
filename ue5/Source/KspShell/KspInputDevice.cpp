#include "KspInputDevice.h"

#include "Input/Events.h"

namespace
{
	struct FKeyRow
	{
		const FKey* Key;
		int32 KeyCode;
		/** A second KeyCode for the same key, 0 for none. */
		int32 Also;
	};

	// UnityEngine.KeyCode values (KeyCode.cs). Unity's Mouse0..6 are KeyCodes too.
	const TArray<FKeyRow>& KeyRows()
	{
		static const TArray<FKeyRow> Rows = {
			{ &EKeys::BackSpace, 8, 0 }, { &EKeys::Tab, 9, 0 }, { &EKeys::Enter, 13, 0 }, { &EKeys::Pause, 19, 0 }, { &EKeys::Escape, 27, 0 },
			{ &EKeys::SpaceBar, 32, 0 }, { &EKeys::Delete, 127, 0 },
			{ &EKeys::Zero, 48, 0 }, { &EKeys::One, 49, 0 }, { &EKeys::Two, 50, 0 }, { &EKeys::Three, 51, 0 }, { &EKeys::Four, 52, 0 },
			{ &EKeys::Five, 53, 0 }, { &EKeys::Six, 54, 0 }, { &EKeys::Seven, 55, 0 }, { &EKeys::Eight, 56, 0 }, { &EKeys::Nine, 57, 0 },
			{ &EKeys::A, 97, 0 }, { &EKeys::B, 98, 0 }, { &EKeys::C, 99, 0 }, { &EKeys::D, 100, 0 }, { &EKeys::E, 101, 0 }, { &EKeys::F, 102, 0 },
			{ &EKeys::G, 103, 0 }, { &EKeys::H, 104, 0 }, { &EKeys::I, 105, 0 }, { &EKeys::J, 106, 0 }, { &EKeys::K, 107, 0 }, { &EKeys::L, 108, 0 },
			{ &EKeys::M, 109, 0 }, { &EKeys::N, 110, 0 }, { &EKeys::O, 111, 0 }, { &EKeys::P, 112, 0 }, { &EKeys::Q, 113, 0 }, { &EKeys::R, 114, 0 },
			{ &EKeys::S, 115, 0 }, { &EKeys::T, 116, 0 }, { &EKeys::U, 117, 0 }, { &EKeys::V, 118, 0 }, { &EKeys::W, 119, 0 }, { &EKeys::X, 120, 0 },
			{ &EKeys::Y, 121, 0 }, { &EKeys::Z, 122, 0 },
			// punctuation by the key's place on a US keyboard
			{ &EKeys::Apostrophe, 39, 0 }, { &EKeys::Comma, 44, 0 }, { &EKeys::Hyphen, 45, 0 }, { &EKeys::Period, 46, 0 }, { &EKeys::Slash, 47, 0 },
			{ &EKeys::Semicolon, 59, 0 }, { &EKeys::Equals, 61, 0 }, { &EKeys::LeftBracket, 91, 0 }, { &EKeys::Backslash, 92, 0 },
			{ &EKeys::RightBracket, 93, 0 }, { &EKeys::Tilde, 96, 0 },   // Unreal calls the ` key Tilde; Unity calls it BackQuote
			{ &EKeys::NumPadZero, 256, 0 }, { &EKeys::NumPadOne, 257, 0 }, { &EKeys::NumPadTwo, 258, 0 }, { &EKeys::NumPadThree, 259, 0 },
			{ &EKeys::NumPadFour, 260, 0 }, { &EKeys::NumPadFive, 261, 0 }, { &EKeys::NumPadSix, 262, 0 }, { &EKeys::NumPadSeven, 263, 0 },
			{ &EKeys::NumPadEight, 264, 0 }, { &EKeys::NumPadNine, 265, 0 }, { &EKeys::Decimal, 266, 0 }, { &EKeys::Divide, 267, 0 },
			{ &EKeys::Multiply, 268, 0 }, { &EKeys::Subtract, 269, 0 }, { &EKeys::Add, 270, 0 },
			{ &EKeys::Up, 273, 0 }, { &EKeys::Down, 274, 0 }, { &EKeys::Right, 275, 0 }, { &EKeys::Left, 276, 0 },
			{ &EKeys::Insert, 277, 0 }, { &EKeys::Home, 278, 0 }, { &EKeys::End, 279, 0 }, { &EKeys::PageUp, 280, 0 }, { &EKeys::PageDown, 281, 0 },
			{ &EKeys::F1, 282, 0 }, { &EKeys::F2, 283, 0 }, { &EKeys::F3, 284, 0 }, { &EKeys::F4, 285, 0 }, { &EKeys::F5, 286, 0 }, { &EKeys::F6, 287, 0 },
			{ &EKeys::F7, 288, 0 }, { &EKeys::F8, 289, 0 }, { &EKeys::F9, 290, 0 }, { &EKeys::F10, 291, 0 }, { &EKeys::F11, 292, 0 }, { &EKeys::F12, 293, 0 },
			{ &EKeys::NumLock, 300, 0 }, { &EKeys::CapsLock, 301, 0 }, { &EKeys::ScrollLock, 302, 0 },
			{ &EKeys::RightShift, 303, 0 }, { &EKeys::LeftShift, 304, 0 }, { &EKeys::RightControl, 305, 0 }, { &EKeys::LeftControl, 306, 0 },
			{ &EKeys::RightAlt, 307, 0 }, { &EKeys::LeftAlt, 308, 0 },
			// on Windows Unreal reports the Windows keys as the command keys; Unity calls them LeftWindows / RightWindows
			{ &EKeys::LeftCommand, 311, 0 }, { &EKeys::RightCommand, 312, 0 },
			{ &EKeys::LeftMouseButton, 323, 0 }, { &EKeys::RightMouseButton, 324, 0 }, { &EKeys::MiddleMouseButton, 325, 0 },
			{ &EKeys::ThumbMouseButton, 326, 0 }, { &EKeys::ThumbMouseButton2, 327, 0 },
			// an XInput pad in Unity's button order: JoystickButtonN (any joystick) and Joystick1ButtonN
			{ &EKeys::Gamepad_FaceButton_Bottom, 330, 350 }, { &EKeys::Gamepad_FaceButton_Right, 331, 351 }, { &EKeys::Gamepad_FaceButton_Left, 332, 352 },
			{ &EKeys::Gamepad_FaceButton_Top, 333, 353 }, { &EKeys::Gamepad_LeftShoulder, 334, 354 }, { &EKeys::Gamepad_RightShoulder, 335, 355 },
			{ &EKeys::Gamepad_Special_Left, 336, 356 }, { &EKeys::Gamepad_Special_Right, 337, 357 }, { &EKeys::Gamepad_LeftThumbstick, 338, 358 },
			{ &EKeys::Gamepad_RightThumbstick, 339, 359 },
		};
		return Rows;
	}

	const TArray<FKspInputDevice::FUnmapped>& UnmappedRows()
	{
		static const TArray<FKspInputDevice::FUnmapped> Rows = [] {
			TArray<FKspInputDevice::FUnmapped> Out;
			Out.Add({ 0, TEXT("None: not a key") });
			Out.Add({ 12, TEXT("Clear (keypad 5 without NumLock): Unreal has no such key") });
			const TCHAR* Shifted = TEXT("a shifted character of a US keyboard: Unity reports the unshifted key, as Unreal does");
			for (int32 Code : { 33, 34, 35, 36, 37, 38, 40, 41, 42, 43, 58, 60, 62, 63, 64, 94, 95, 123, 124, 125, 126 })
			{
				Out.Add({ Code, Shifted });
			}
			Out.Add({ 271, TEXT("KeypadEnter: Unreal reports both Enter keys as Enter") });
			Out.Add({ 272, TEXT("KeypadEquals: a Mac keypad key") });
			for (int32 Code : { 294, 295, 296 })
			{
				Out.Add({ Code, TEXT("F13..F15: Unreal has no such keys") });
			}
			Out.Add({ 309, TEXT("RightCommand / RightApple: Mac") });
			Out.Add({ 310, TEXT("LeftCommand / LeftApple: Mac") });
			Out.Add({ 313, TEXT("AltGr: Unreal reports it as RightAlt") });
			for (int32 Code : { 315, 316, 317, 318, 319 })
			{
				Out.Add({ Code, TEXT("Help, Print, SysReq, Break, Menu: Unreal has no such keys") });
			}
			Out.Add({ 328, TEXT("Mouse5: Unreal has five mouse buttons") });
			Out.Add({ 329, TEXT("Mouse6: Unreal has five mouse buttons") });
			for (int32 Code = 340; Code <= 349; ++Code)
			{
				Out.Add({ Code, TEXT("JoystickButton10..19: beyond the ten buttons of an XInput pad") });
			}
			for (int32 Code = 360; Code <= 369; ++Code)
			{
				Out.Add({ Code, TEXT("Joystick1Button10..19: beyond the ten buttons of an XInput pad") });
			}
			for (int32 Code = 370; Code <= 509; ++Code)
			{
				Out.Add({ Code, TEXT("Joystick2..8 buttons: Unreal does not number game controllers as Unity does; one pad is reported as joystick 1") });
			}
			return Out;
		}();
		return Rows;
	}
}

void FKspInputDevice::KeyCodesOf(const FKey& Key, TArray<int32, TInlineAllocator<2>>& Out)
{
	Out.Reset();
	static const TMap<FKey, FKeyRow> ByKey = [] {
		TMap<FKey, FKeyRow> Map;
		for (const FKeyRow& Row : KeyRows())
		{
			Map.Add(*Row.Key, Row);
		}
		return Map;
	}();
	if (const FKeyRow* Row = ByKey.Find(Key))
	{
		Out.Add(Row->KeyCode);
		if (Row->Also != 0)
		{
			Out.Add(Row->Also);
		}
	}
}

TConstArrayView<FKspInputDevice::FUnmapped> FKspInputDevice::UnmappedKeyCodes()
{
	return UnmappedRows();
}

TSet<int32> FKspInputDevice::MappedKeyCodes()
{
	TSet<int32> Out;
	for (const FKeyRow& Row : KeyRows())
	{
		Out.Add(Row.KeyCode);
		if (Row.Also != 0)
		{
			Out.Add(Row.Also);
		}
	}
	return Out;
}

void FKspInputDevice::KeyDown(const FKey& Key)
{
	TArray<int32, TInlineAllocator<2>> Codes;
	KeyCodesOf(Key, Codes);
	for (int32 Code : Codes)
	{
		if (!Held.Contains(Code))   // a repeat of a held key is not a new press
		{
			Held.Add(Code);
			Down.Add(Code);
		}
	}
}

void FKspInputDevice::KeyUp(const FKey& Key)
{
	TArray<int32, TInlineAllocator<2>> Codes;
	KeyCodesOf(Key, Codes);
	for (int32 Code : Codes)
	{
		if (Held.Remove(Code) > 0)
		{
			Up.Add(Code);
		}
	}
}

void FKspInputDevice::MouseMoved(const FVector2D& Delta)
{
	MouseDelta += Delta;
}

void FKspInputDevice::MouseWheel(float Delta)
{
	Wheel += Delta;
}

void FKspInputDevice::ReleaseAll()
{
	Up.Append(Held);
	Held.Reset();
}

void FKspInputDevice::TakeFrame(FKspInputFrame& Out, const FIntPoint& ScreenSize, const FVector2D& MousePosition, bool bHasFocus, bool bUiHoverLock,
	float RealTimeSinceStartup)
{
	Out.KeysDown = Down;
	Out.KeysUp = Up;
	Out.KeysHeld = Held;
	// Unity: origin bottom left, y up
	Out.MousePosition = FVector2D(MousePosition.X, double(ScreenSize.Y) - MousePosition.Y);
	Out.MouseDelta = FVector2f(float(MouseDelta.X), float(-MouseDelta.Y));
	Out.MouseWheel = Wheel;
	// Input.GetMouseButton(n) = bit n: Mouse0 (323) is the left button
	auto Buttons = [](const TArray<int32>& Codes)
	{
		uint32 Bits = 0;
		for (int32 Code : Codes)
		{
			if (Code >= 323 && Code <= 329)
			{
				Bits |= 1u << (Code - 323);
			}
		}
		return Bits;
	};
	Out.MouseButtonsDown = Buttons(Down);
	Out.MouseButtonsUp = Buttons(Up);
	Out.MouseButtonsHeld = Buttons(Held);
	Out.ScreenWidth = ScreenSize.X;
	Out.ScreenHeight = ScreenSize.Y;
	Out.bHasFocus = bHasFocus;
	Out.bUiHoverLock = bUiHoverLock;
	Out.RealTimeSinceStartup = RealTimeSinceStartup;
	Down.Reset();
	Up.Reset();
	MouseDelta = FVector2D::ZeroVector;
	Wheel = 0.0f;
}

// The handlers never consume: the event goes on to Slate and the viewport as if the device were not there.

bool FKspInputDevice::HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent)
{
	if (!InKeyEvent.IsRepeat())
	{
		KeyDown(InKeyEvent.GetKey());
	}
	return false;
}

bool FKspInputDevice::HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent)
{
	KeyUp(InKeyEvent.GetKey());
	return false;
}

bool FKspInputDevice::HandleMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	MouseMoved(MouseEvent.GetCursorDelta());
	return false;
}

bool FKspInputDevice::HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	KeyDown(MouseEvent.GetEffectingButton());
	return false;
}

bool FKspInputDevice::HandleMouseButtonUpEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	KeyUp(MouseEvent.GetEffectingButton());
	return false;
}

bool FKspInputDevice::HandleMouseButtonDoubleClickEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	// the second press of a double click arrives as this event instead of a button-down
	KeyDown(MouseEvent.GetEffectingButton());
	return false;
}

bool FKspInputDevice::HandleMouseWheelOrGestureEvent(FSlateApplication& SlateApp, const FPointerEvent& InWheelEvent, const FPointerEvent* InGestureEvent)
{
	MouseWheel(InWheelEvent.GetWheelDelta());
	return false;
}
