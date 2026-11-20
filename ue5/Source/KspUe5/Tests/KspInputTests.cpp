// Headless automation tests of the input device (WP-UE-08). KspUe5.Input.DeviceParity is in KspSimTests.cpp (it shares the parity run). Run:
//   Scripts\test.ps1 -Filter KspUe5.Input
#include "CoreMinimal.h"
#include "KspInputDevice.h"
#include "KspTestPaths.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#if WITH_DEV_AUTOMATION_TESTS

// The key table against UnityEngine.KeyCode itself: every value of the enum (read from the decompiled KeyCode.cs) is produced by
// some Unreal key or is listed as unmapped with a reason, never both, and nothing else is.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKspInputKeyCodeTableTest, "KspUe5.Input.KeyCodeTable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)
bool FKspInputKeyCodeTableTest::RunTest(const FString& Parameters)
{
	const FString File = FPaths::Combine(KspTestPaths::RepoRoot(), TEXT("decompiled"), TEXT("KSP-Decompiled"), TEXT("Unity"), TEXT("UnityEngine.CoreModule"),
		TEXT("UnityEngine"), TEXT("KeyCode.cs"));
	TArray<FString> Lines;
	if (!FFileHelper::LoadFileToStringArray(Lines, *File))
	{
		AddError(FString::Printf(TEXT("cannot read %s"), *File));
		return false;
	}
	TMap<int32, FString> Enum;   // value -> the first name with that value
	int32 Names = 0;
	for (const FString& Line : Lines)
	{
		FString Name, Value;
		if (Line.Split(TEXT(" = "), &Name, &Value))
		{
			Name.TrimStartAndEndInline();
			Value.TrimStartAndEndInline();
			Value.RemoveFromEnd(TEXT(","));
			if (Value.IsNumeric() && !Name.Contains(TEXT(" ")))
			{
				++Names;
				if (!Enum.Contains(FCString::Atoi(*Value)))
				{
					Enum.Add(FCString::Atoi(*Value), Name);
				}
			}
		}
	}
	TestEqual(TEXT("names in UnityEngine.KeyCode"), Names, 326);

	const TSet<int32> Mapped = FKspInputDevice::MappedKeyCodes();
	TSet<int32> Unmapped;
	for (const FKspInputDevice::FUnmapped& Row : FKspInputDevice::UnmappedKeyCodes())
	{
		TestFalse(*FString::Printf(TEXT("KeyCode %d is listed as unmapped once"), Row.KeyCode), Unmapped.Contains(Row.KeyCode));
		Unmapped.Add(Row.KeyCode);
		TestTrue(*FString::Printf(TEXT("unmapped KeyCode %d has a reason"), Row.KeyCode), Row.Reason && Row.Reason[0]);
	}
	int32 Neither = 0, Both = 0, NotInEnum = 0;
	for (const TPair<int32, FString>& Pair : Enum)
	{
		const bool bMapped = Mapped.Contains(Pair.Key), bUnmapped = Unmapped.Contains(Pair.Key);
		if (!bMapped && !bUnmapped)
		{
			++Neither;
			AddError(FString::Printf(TEXT("KeyCode %s = %d is neither mapped nor listed as unmapped"), *Pair.Value, Pair.Key));
		}
		if (bMapped && bUnmapped)
		{
			++Both;
			AddError(FString::Printf(TEXT("KeyCode %s = %d is mapped and listed as unmapped"), *Pair.Value, Pair.Key));
		}
	}
	for (int32 Code : Mapped.Union(Unmapped))
	{
		if (!Enum.Contains(Code))
		{
			++NotInEnum;
			AddError(FString::Printf(TEXT("%d is in the table but not a KeyCode"), Code));
		}
	}
	AddInfo(FString::Printf(TEXT("UnityEngine.KeyCode: %d names, %d values; %d mapped from Unreal keys, %d listed as unmapped"), Names, Enum.Num(), Mapped.Num(),
		Unmapped.Num()));
	TestEqual(TEXT("values neither mapped nor listed"), Neither, 0);
	TestEqual(TEXT("values both mapped and listed"), Both, 0);
	TestEqual(TEXT("table entries that are no KeyCode"), NotInEnum, 0);
	TestEqual(TEXT("mapped + unmapped = every value"), Mapped.Num() + Unmapped.Num(), Enum.Num());

	// KSP's default flight keys (GameSettings.cs defaults): the names Unity and Unreal give them
	auto Code = [](const FKey& Key)
	{
		TArray<int32, TInlineAllocator<2>> Codes;
		FKspInputDevice::KeyCodesOf(Key, Codes);
		return Codes.Num() > 0 ? Codes[0] : 0;
	};
	auto Named = [&Enum](int32 Value) { return Enum.FindRef(Value); };
	TestEqual(TEXT("W"), Named(Code(EKeys::W)), FString(TEXT("W")));
	TestEqual(TEXT("Z"), Named(Code(EKeys::Z)), FString(TEXT("Z")));
	TestEqual(TEXT("T"), Named(Code(EKeys::T)), FString(TEXT("T")));
	TestEqual(TEXT("space"), Named(Code(EKeys::SpaceBar)), FString(TEXT("Space")));
	TestEqual(TEXT("left shift"), Named(Code(EKeys::LeftShift)), FString(TEXT("LeftShift")));
	TestEqual(TEXT("left control"), Named(Code(EKeys::LeftControl)), FString(TEXT("LeftControl")));
	TestEqual(TEXT("comma"), Named(Code(EKeys::Comma)), FString(TEXT("Comma")));
	TestEqual(TEXT("period"), Named(Code(EKeys::Period)), FString(TEXT("Period")));
	TestEqual(TEXT("F5"), Named(Code(EKeys::F5)), FString(TEXT("F5")));
	TestEqual(TEXT("backquote"), Named(Code(EKeys::Tilde)), FString(TEXT("BackQuote")));
	TestEqual(TEXT("keypad 5"), Named(Code(EKeys::NumPadFive)), FString(TEXT("Keypad5")));
	TestEqual(TEXT("left mouse button"), Named(Code(EKeys::LeftMouseButton)), FString(TEXT("Mouse0")));
	TestEqual(TEXT("a key without a KeyCode"), Code(EKeys::Gamepad_LeftTrigger), 0);
	return true;
}

// UnityEngine.Input's per-frame view of the events: GetKeyDown / GetKeyUp lists, GetKey set, mouse buttons as bits, the cursor with
// Unity's origin.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKspInputDeviceFramesTest, "KspUe5.Input.DeviceFrames",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)
bool FKspInputDeviceFramesTest::RunTest(const FString& Parameters)
{
	const int32 KeyW = 119, KeyA = 97, KeySpace = 32, Mouse0 = 323, Mouse1 = 324;
	FKspInputDevice Device;
	FKspInputFrame Frame;
	auto Take = [&Device, &Frame](const FVector2D& Mouse = FVector2D(0.0, 720.0))
	{
		Frame = FKspInputFrame();
		Device.TakeFrame(Frame, FIntPoint(1280, 720), Mouse, true, false, 1.5f);
	};

	// frame 1: W and A go down in that order; the key repeat of W is not a second press
	Device.KeyDown(EKeys::W);
	Device.KeyDown(EKeys::A);
	Device.KeyDown(EKeys::W);
	Take();
	TestEqual(TEXT("frame 1 down"), Frame.KeysDown, TArray<int32>({ KeyW, KeyA }));
	TestEqual(TEXT("frame 1 held"), Frame.KeysHeld, TArray<int32>({ KeyW, KeyA }));
	TestEqual(TEXT("frame 1 up"), Frame.KeysUp.Num(), 0);
	TestEqual(TEXT("screen width"), Frame.ScreenWidth, 1280);
	TestEqual(TEXT("screen height"), Frame.ScreenHeight, 720);
	TestEqual(TEXT("real time"), Frame.RealTimeSinceStartup, 1.5f);
	TestTrue(TEXT("focus"), Frame.bHasFocus);

	// frame 2: nothing happens; the keys are still held, the edge lists are empty
	Take();
	TestEqual(TEXT("frame 2 down"), Frame.KeysDown.Num(), 0);
	TestEqual(TEXT("frame 2 held"), Frame.KeysHeld, TArray<int32>({ KeyW, KeyA }));

	// frame 3: W is released; Space is tapped within the frame: down and up, never held
	Device.KeyUp(EKeys::W);
	Device.KeyDown(EKeys::SpaceBar);
	Device.KeyUp(EKeys::SpaceBar);
	Device.KeyUp(EKeys::Q);   // a key that was never down: no key-up
	Take();
	TestEqual(TEXT("frame 3 down"), Frame.KeysDown, TArray<int32>({ KeySpace }));
	TestEqual(TEXT("frame 3 up"), Frame.KeysUp, TArray<int32>({ KeyW, KeySpace }));
	TestEqual(TEXT("frame 3 held"), Frame.KeysHeld, TArray<int32>({ KeyA }));

	// frame 4: mouse. Buttons are KeyCodes and bits (bit 0 left, bit 1 right); the delta's y and the cursor's y turn over; the wheel adds up
	Device.KeyDown(EKeys::LeftMouseButton);
	Device.KeyDown(EKeys::RightMouseButton);
	Device.KeyUp(EKeys::RightMouseButton);
	Device.MouseMoved(FVector2D(3.0, 4.0));
	Device.MouseMoved(FVector2D(-1.0, 2.0));
	Device.MouseWheel(1.0f);
	Device.MouseWheel(0.5f);
	Take(FVector2D(100.0, 20.0));
	TestEqual(TEXT("frame 4 down"), Frame.KeysDown, TArray<int32>({ Mouse0, Mouse1 }));
	TestEqual(TEXT("frame 4 buttons down"), Frame.MouseButtonsDown, 3u);
	TestEqual(TEXT("frame 4 buttons up"), Frame.MouseButtonsUp, 2u);
	TestEqual(TEXT("frame 4 buttons held"), Frame.MouseButtonsHeld, 1u);
	TestEqual(TEXT("frame 4 held keys"), Frame.KeysHeld, TArray<int32>({ KeyA, Mouse0 }));
	TestEqual(TEXT("mouse delta x"), Frame.MouseDelta.X, 2.0f);
	TestEqual(TEXT("mouse delta y (up is positive)"), Frame.MouseDelta.Y, -6.0f);
	TestEqual(TEXT("wheel"), Frame.MouseWheel, 1.5f);
	TestEqual(TEXT("cursor x"), Frame.MousePosition.X, 100.0);
	TestEqual(TEXT("cursor y from the bottom"), Frame.MousePosition.Y, 700.0);

	// frame 5: the per-frame mouse values are gone
	Take();
	TestEqual(TEXT("frame 5 delta"), Frame.MouseDelta, FVector2f::ZeroVector);
	TestEqual(TEXT("frame 5 wheel"), Frame.MouseWheel, 0.0f);
	TestEqual(TEXT("frame 5 buttons down"), Frame.MouseButtonsDown, 0u);

	// focus lost: everything held is released
	Device.ReleaseAll();
	Take();
	TestEqual(TEXT("release all: up"), Frame.KeysUp, TArray<int32>({ KeyA, Mouse0 }));
	TestEqual(TEXT("release all: held"), Frame.KeysHeld.Num(), 0);
	TestEqual(TEXT("release all: buttons"), Frame.MouseButtonsHeld, 0u);

	// a gamepad button is two KeyCodes: JoystickButton0 and Joystick1Button0
	Device.KeyDown(EKeys::Gamepad_FaceButton_Bottom);
	Take();
	TestEqual(TEXT("gamepad A"), Frame.KeysDown, TArray<int32>({ 330, 350 }));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
