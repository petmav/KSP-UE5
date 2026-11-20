// Headless automation tests of the sim subsystem (WP-UE-07). Run:
//   Scripts\test.ps1 -Filter KspUe5.Sim
//   Scripts\test.ps1 -Filter KspUe5.Slice     (runs the ctest twin capi_flight_slice first so its hash file is fresh)
#include "CoreMinimal.h"
#include "KspInputDevice.h"
#include "KspRenderAnchor.h"
#include "KspSim.h"
#include "KspSimSubsystem.h"
#include "KspTestPaths.h"
#include "KspTestWorld.h"
#include "Math/RandomStream.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	bool Near(const FVector& A, const FVector& B, double Tolerance) { return (A - B).GetAbsMax() <= Tolerance; }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKspSimAxisConversionTest, "KspUe5.Sim.AxisConversion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)
bool FKspSimAxisConversionTest::RunTest(const FString& Parameters)
{
	// Unity: right = +X, up = +Y, forward = +Z, metres. Unreal: forward = +X, right = +Y, up = +Z, centimetres.
	const FVector UnityRight(1, 0, 0), UnityUp(0, 1, 0), UnityForward(0, 0, 1);
	TestEqual(TEXT("Unity right is Unreal right"), FKspAxis::DirectionToUe(UnityRight), FVector::RightVector);
	TestEqual(TEXT("Unity up is Unreal up"), FKspAxis::DirectionToUe(UnityUp), FVector::UpVector);
	TestEqual(TEXT("Unity forward is Unreal forward"), FKspAxis::DirectionToUe(UnityForward), FVector::ForwardVector);
	TestEqual(TEXT("metres to centimetres"), FKspAxis::PositionToUe(FVector(1, 2, 3)), FVector(300, 100, 200));

	// Handedness: the axis map has determinant +1 (both systems are left-handed), so no mirror and no change of a rotation's sense.
	const double Det = FVector::DotProduct(
		FVector::CrossProduct(FKspAxis::DirectionToUe(UnityRight), FKspAxis::DirectionToUe(UnityUp)), FKspAxis::DirectionToUe(UnityForward));
	TestEqual(TEXT("determinant of the axis map"), Det, 1.0);

	// Unity's Quaternion.AngleAxis(90, Vector3.up) = (0, sin 45, 0, cos 45) turns forward to right; so must its conversion in Unreal.
	const double S = FMath::Sqrt(0.5);
	const FQuat UeYaw = FKspAxis::RotationToUe(FQuat(0.0, S, 0.0, S));
	TestTrue(TEXT("90 degrees about up turns forward to right"), Near(UeYaw.RotateVector(FVector::ForwardVector), FVector::RightVector, 1e-15));
	TestTrue(TEXT("the converted rotation is Unreal's yaw of +90 degrees"), UeYaw.Equals(FQuat(FVector::UpVector, UE_DOUBLE_HALF_PI), 1e-15));

	FRandomStream Random(20261118);
	double MaxPositionRoundTrip = 0.0, MaxRotate = 0.0, MaxCompose = 0.0;
	bool bExactRoundTrips = true;
	for (int32 I = 0; I < 1000; ++I)
	{
		// positions at planetary scale (Kerbin's radius is 600 km), in metres
		const FVector P(Random.FRandRange(-7.0e5, 7.0e5), Random.FRandRange(-7.0e5, 7.0e5), Random.FRandRange(-7.0e5, 7.0e5));
		const FVector D = Random.GetUnitVector();
		const FQuat A = FQuat(Random.GetUnitVector(), Random.FRandRange(-UE_DOUBLE_PI, UE_DOUBLE_PI));
		const FQuat B = FQuat(Random.GetUnitVector(), Random.FRandRange(-UE_DOUBLE_PI, UE_DOUBLE_PI));

		// the axis map alone is a permutation: exact both ways
		bExactRoundTrips &= FKspAxis::DirectionToUnity(FKspAxis::DirectionToUe(D)) == D && FKspAxis::DirectionToUe(FKspAxis::DirectionToUnity(D)) == D;
		const FQuat BackA = FKspAxis::RotationToUnity(FKspAxis::RotationToUe(A));
		bExactRoundTrips &= BackA.X == A.X && BackA.Y == A.Y && BackA.Z == A.Z && BackA.W == A.W;
		// with the unit scale: x100 then /100 may round once
		MaxPositionRoundTrip = FMath::Max(MaxPositionRoundTrip, ((FKspAxis::PositionToUnity(FKspAxis::PositionToUe(P)) - P) / P).GetAbsMax());

		// conversion commutes with rotating a vector and with composing rotations (Unity and Unreal both apply the right factor first)
		MaxRotate = FMath::Max(MaxRotate, (FKspAxis::DirectionToUe(A.RotateVector(D)) - FKspAxis::RotationToUe(A).RotateVector(FKspAxis::DirectionToUe(D))).GetAbsMax());
		const FQuat Lhs = FKspAxis::RotationToUe(A * B), Rhs = FKspAxis::RotationToUe(A) * FKspAxis::RotationToUe(B);
		MaxCompose = FMath::Max(MaxCompose, FMath::Max(FMath::Max(FMath::Abs(Lhs.X - Rhs.X), FMath::Abs(Lhs.Y - Rhs.Y)), FMath::Max(FMath::Abs(Lhs.Z - Rhs.Z), FMath::Abs(Lhs.W - Rhs.W))));
	}
	AddInfo(FString::Printf(TEXT("1000 samples: position round trip max relative error %.3g, rotate-vector max difference %.3g, compose max difference %.3g"),
		MaxPositionRoundTrip, MaxRotate, MaxCompose));
	TestTrue(TEXT("direction and rotation round trips are exact"), bExactRoundTrips);
	TestTrue(TEXT("position round trip within one rounding"), MaxPositionRoundTrip <= 2.5e-16);
	TestTrue(TEXT("conversion commutes with rotating a vector"), MaxRotate <= 1e-15);
	TestTrue(TEXT("conversion commutes with composing rotations"), MaxCompose <= 1e-15);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKspSimRenderAnchorTest, "KspUe5.Sim.RenderAnchor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)
bool FKspSimRenderAnchorTest::RunTest(const FString& Parameters)
{
	const double Km = 1000.0 * 100.0;
	// the KSC pad in body-fixed axes is about 600 km from Kerbin's centre
	const FVector Pad(1.6e7, -5.8e7, -1.0e5);
	FKspRenderAnchor Anchor;
	TestTrue(TEXT("the first focus places the anchor"), Anchor.Update(Pad, 0));
	TestEqual(TEXT("anchor at the focus"), Anchor.Origin, Pad);
	TestEqual(TEXT("the focus renders at the world origin"), Anchor.ToWorld(Pad), FVector::ZeroVector);

	TestFalse(TEXT("99 km away: stays"), Anchor.Update(Pad + FVector(99.0 * Km, 0, 0), 0));
	TestFalse(TEXT("exactly 100 km away: stays"), Anchor.Update(Pad + FVector(0, 100.0 * Km, 0), 0));
	TestEqual(TEXT("a position 99 km away renders 99 km from the origin"), Anchor.ToWorld(Pad + FVector(99.0 * Km, 0, 0)), FVector(99.0 * Km, 0, 0));
	TestEqual(TEXT("one move so far"), Anchor.MoveCount, 1);

	const FVector Far = Pad + FVector(0, 0, 100.001 * Km);
	TestTrue(TEXT("beyond 100 km: moves to the focus"), Anchor.Update(Far, 0));
	TestEqual(TEXT("anchor at the new focus"), Anchor.Origin, Far);

	TestTrue(TEXT("a frame change of core moves it whatever the distance"), Anchor.Update(Far + FVector(1, 2, 3), 1));
	TestEqual(TEXT("anchor at the focus after the frame change"), Anchor.Origin, Far + FVector(1, 2, 3));
	TestFalse(TEXT("the same serial again: stays"), Anchor.Update(Far, 1));
	TestEqual(TEXT("three moves"), Anchor.MoveCount, 3);

	Anchor.Reset();
	TestFalse(TEXT("reset"), Anchor.bValid);
	return true;
}

// The parity gate of ue5/DESIGN.md 1.5 point 5. The same run as ctest capi.flight_slice (core/tests/capi/capi_flight_slice.c): PhysProf-M2
// launched from the KSC LaunchPad on the repository's GameData mirror, 100 settle frames, then full throttle (Z), SAS (T) and the first
// stage (Space) as capture C10 flew it, 4,560 more frames, every frame 0.02 s. Here the frames run through the Unreal world tick and
// UKspSimSubsystem; the final snapshot hash must equal the one the pure-C program wrote.
namespace
{
bool RunSliceParity(FAutomationTestBase& T, bool bThroughDevice)
{
	const int32 KeySpace = 32, KeyT = 116, KeyZ = 122;   // UnityEngine.KeyCode
	const int32 Settle = 100, Window = 4560;

	const FString Repo = KspTestPaths::RepoRoot();
	const FString HashFile = FPaths::Combine(KspTestPaths::CoreBuildDir(), TEXT("tests"), TEXT("capi_flight_slice_work"), TEXT("final.hash"));
	FString Expected;
	if (!FFileHelper::LoadFileToString(Expected, *HashFile))
	{
		T.AddError(FString::Printf(TEXT("no hash file of the ctest twin at %s: run ctest -R capi.flight_slice in the core build (Scripts\\test.ps1 does)"), *HashFile));
		return false;
	}
	Expected.TrimStartAndEndInline();

	// the arguments of the ctest (core/tests/CMakeLists.txt, capi.flight_slice)
	const FKspGameConfig Config = KspTestPaths::MirrorGameConfig(TEXT("parity"));
	const FString Craft = FPaths::Combine(Repo, TEXT("analysis"), TEXT("physics"), TEXT("craft"), TEXT("PhysProf-M2.craft"));

	if (FKspGame::IsInitialised() && !(FKspGame::GetConfig() == Config) && !FKspGame::Shutdown())
	{
		T.AddError(TEXT("core is initialised with another configuration and a sim is running"));
		return false;
	}

	{
		FKspTestWorld TestWorld;
		UKspSimSubsystem* Subsystem = TestWorld.World->GetSubsystem<UKspSimSubsystem>();
		if (!T.TestNotNull(TEXT("the game world has the sim subsystem"), Subsystem))
		{
			return false;
		}
		FString Error;
		if (!Subsystem->StartSandboxFlight(Config, TEXT("flight_slice"), Craft, TEXT("LaunchPad"), Error))
		{
			T.AddError(FString::Printf(TEXT("StartSandboxFlight failed: %s"), *Error));
			return false;
		}
		Subsystem->FixedFrameDt = 0.02;
		// the twin's input: nothing before the flight scene runs, then an idle 1280 x 720 device and the three keys at frame `Settle`
		int32 FlightFrame = 0;
		Subsystem->InputSource = [&FlightFrame, Subsystem, KeySpace, KeyT, KeyZ, Settle, bThroughDevice](FKspInputFrame& Input)
		{
			if (Subsystem->GetSim()->CurrentScene() != KspScene::Flight)
			{
				return false;
			}
			const float RealTime = float(FlightFrame) * 0.02f;
			if (bThroughDevice)
			{
				// The same keys as key events through the input device: Z, T and Space go down in the frame `Settle`; T and Space are
				// tapped (released within the frame) and Z is released in the next frame, because the twin lists Z as the only held key
				// of that frame and none afterwards. The cursor rests at Unity's (0, 0), the bottom left corner.
				FKspInputDevice* Device = Subsystem->GetInputDevice();
				if (FlightFrame == Settle)
				{
					Device->KeyDown(EKeys::Z);
					Device->KeyDown(EKeys::T);
					Device->KeyDown(EKeys::SpaceBar);
					Device->KeyUp(EKeys::T);
					Device->KeyUp(EKeys::SpaceBar);
				}
				else if (FlightFrame == Settle + 1)
				{
					Device->KeyUp(EKeys::Z);
				}
				Device->TakeFrame(Input, FIntPoint(1280, 720), FVector2D(0.0, 720.0), true, false, RealTime);
			}
			else
			{
				Input.ScreenWidth = 1280;
				Input.ScreenHeight = 720;
				Input.bHasFocus = true;
				Input.RealTimeSinceStartup = RealTime;
				if (FlightFrame == Settle)
				{
					Input.KeysDown = { KeyZ, KeyT, KeySpace };
					Input.KeysHeld = { KeyZ };
				}
			}
			++FlightFrame;
			return true;
		};

		// frame 1: core reports the requested FLIGHT scene and the subsystem answers; then the twin's 4,660 frames
		TestWorld.Tick(0.02f);
		const FKspSim* Sim = Subsystem->GetSim();
		if (!T.TestNotNull(TEXT("the sim runs after the first frame"), Sim))
		{
			return false;
		}
		T.TestEqual(TEXT("core is in the FLIGHT scene after the first world tick"), Sim->CurrentScene(), KspScene::Flight);
		const FKspVesselState* Vessel = Sim->ActiveVessel();
		if (!T.TestNotNull(TEXT("M2 is the active vessel"), Vessel))
		{
			return false;
		}
		T.TestEqual(TEXT("M2 part count"), Vessel->PartCount, 153u);
		const double PadAltitude = Vessel->Altitude;

		const double T0 = FPlatformTime::Seconds();
		for (int32 Frame = 0; Frame < Settle + Window && Subsystem->GetSim(); ++Frame)
		{
			TestWorld.Tick(0.02f);
		}
		const double LoopSeconds = FPlatformTime::Seconds() - T0;
		Sim = Subsystem->GetSim();
		if (!T.TestNotNull(TEXT("the sim ran every frame"), Sim))
		{
			return false;
		}
		T.TestEqual(TEXT("frames the tick function ran"), Subsystem->GetFrameCount(), int64(1 + Settle + Window));
		T.TestEqual(TEXT("input frames taken"), FlightFrame, Settle + Window);
		T.TestEqual(TEXT("frames core counts in the flight scene"), Sim->FrameState().FrameCount, uint64(Settle + Window));

		Vessel = Sim->ActiveVessel();
		const FString Hash = FString::Printf(TEXT("%016llx"), Sim->SnapshotHash());
		T.AddInfo(FString::Printf(TEXT("pad altitude %.3f m; after %d frames: altitude %.1f m, surface speed %.1f m/s, Mach %.3f, UT %.2f"), PadAltitude,
			Settle + Window, Vessel ? Vessel->Altitude : 0.0, Vessel ? Vessel->SurfaceSpeed : 0.0f, Vessel ? Vessel->Mach : 0.0f, Sim->FrameState().Ut));
		T.AddInfo(FString::Printf(TEXT("state hash through Unreal %s, ctest twin %s"), *Hash, *Expected));
		T.AddInfo(FString::Printf(TEXT("world tick %.3f ms per frame, of which core %.3f ms; render anchor moved %d time(s), %.3f ms in the frames it moved"),
			1000.0 * LoopSeconds / double(Settle + Window), 1000.0 * Subsystem->GetCoreSeconds() / double(Subsystem->GetFrameCount()),
			Subsystem->GetAnchor().MoveCount, 1000.0 * Subsystem->GetReanchorSeconds()));
		T.TestEqual(TEXT("final state hash equals the ctest twin's"), Hash, Expected);
		// the ascent ends above 30 km: the vessel left the pad's neighbourhood, the anchor did not need to move (threshold 100 km)
		T.TestTrue(TEXT("the vessel climbed"), Vessel && Vessel->Altitude > 30000.0);
		T.TestEqual(TEXT("render anchor placed once"), Subsystem->GetAnchor().MoveCount, 1);
		Subsystem->StopSim();
	}
	T.TestTrue(TEXT("core shuts down once the sim is gone"), FKspGame::Shutdown());
	return true;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKspSliceHeadlessParityTest, "KspUe5.Slice.HeadlessParity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)
bool FKspSliceHeadlessParityTest::RunTest(const FString& Parameters)
{
	return RunSliceParity(*this, false);
}

// The same run with the three keys as key events through FKspInputDevice (WP-UE-08) instead of a hand-made input frame: the key
// table and the device's frame bookkeeping give core the same flight, bit for bit.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKspInputDeviceParityTest, "KspUe5.Input.DeviceParity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)
bool FKspInputDeviceParityTest::RunTest(const FString& Parameters)
{
	return RunSliceParity(*this, true);
}

#endif // WITH_DEV_AUTOMATION_TESTS
