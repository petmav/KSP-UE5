#pragma once

#include "CoreMinimal.h"
#include "Framework/Application/IInputProcessor.h"
#include "InputCoreTypes.h"
#include "KspSim.h"

/**
 * The input device (WP-UE-08, ue5/DESIGN.md 5.1, decision D12): the shell reports raw device state, KSP's own bindings in core decide
 * what a key does. It collects Unreal's key and mouse events (as a Slate input pre-processor: before any widget, never consuming) and
 * hands core one frame of UnityEngine.Input-shaped state: keys that went down, keys that went up, keys held, mouse position, delta,
 * buttons and wheel.
 *
 * Keys are UnityEngine.KeyCode numbers (decompiled/KSP-Decompiled/Unity/UnityEngine.CoreModule/UnityEngine/KeyCode.cs). Unity names a
 * key by its place on a US keyboard, as Unreal's FKey does. The KeyCodes no Unreal key produces are listed with the reason
 * (UnmappedKeyCodes); the automation test KspUe5.Input.KeyCodeTable checks that table and list together cover the enum.
 *
 * Not reported yet: Input.inputString (typed characters reach Slate text widgets, not a pre-processor), joystick axes and Unity's
 * input-manager axes ("Mouse X" ...; their sensitivities are in the game's InputManager asset, not extracted).
 */
class KSPSHELL_API FKspInputDevice : public IInputProcessor
{
public:
	/** The KeyCodes of an Unreal key: none, one, or two (a gamepad button is JoystickButtonN and Joystick1ButtonN). */
	static void KeyCodesOf(const FKey& Key, TArray<int32, TInlineAllocator<2>>& Out);

	struct FUnmapped
	{
		int32 KeyCode;
		const TCHAR* Reason;
	};
	/** KeyCode values no Unreal key maps to, each with the reason. */
	static TConstArrayView<FUnmapped> UnmappedKeyCodes();
	/** Every KeyCode value some Unreal key maps to. */
	static TSet<int32> MappedKeyCodes();

	// ---- raw events (the Slate handlers below call these; tests inject through them) ----
	void KeyDown(const FKey& Key);
	void KeyUp(const FKey& Key);
	/** Cursor movement in pixels, Unreal's sense (y down). */
	void MouseMoved(const FVector2D& Delta);
	/** Wheel notches, positive away from the user. */
	void MouseWheel(float Delta);
	/** Forgets every held key and button (focus lost: no key-up will come). */
	void ReleaseAll();

	/**
	 * The frame since the previous call. MousePosition is in viewport pixels with Unreal's origin (top left); it is reported to core
	 * with Unity's origin (bottom left). Clears the per-frame lists; held keys stay.
	 */
	void TakeFrame(FKspInputFrame& Out, const FIntPoint& ScreenSize, const FVector2D& MousePosition, bool bHasFocus, bool bUiHoverLock, float RealTimeSinceStartup);

	// ---- IInputProcessor ----
	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override {}
	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override;
	virtual bool HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override;
	virtual bool HandleMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override;
	virtual bool HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override;
	virtual bool HandleMouseButtonUpEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override;
	virtual bool HandleMouseButtonDoubleClickEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override;
	virtual bool HandleMouseWheelOrGestureEvent(FSlateApplication& SlateApp, const FPointerEvent& InWheelEvent, const FPointerEvent* InGestureEvent) override;
	virtual const TCHAR* GetDebugName() const override { return TEXT("KspInputDevice"); }

private:
	/** In event order; a key is in Held from its down event to its up event. */
	TArray<int32> Down;
	TArray<int32> Up;
	TArray<int32> Held;
	FVector2D MouseDelta = FVector2D::ZeroVector;
	float Wheel = 0.0f;
};
