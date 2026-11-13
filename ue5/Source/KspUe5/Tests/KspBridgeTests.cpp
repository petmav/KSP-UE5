// Headless automation tests of the kspcore bridge (WP-UE-01). Run:
//   Scripts\test.ps1 -Filter KspUe5.Bridge      (runs the ctest twin first so kspcore_jolt_step.txt is fresh)
#include "CoreMinimal.h"
#include "KspCoreBridge.h"
#include "HAL/PlatformMisc.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// <KSPCORE_ROOT or <project>/../core>/build/RelWithDebInfo, where the core build puts kspcore_link.txt and where the ctest
	// twin capi.jolt_step writes its hash.
	FString CoreBuildDir()
	{
		FString Root = FPlatformMisc::GetEnvironmentVariable(TEXT("KSPCORE_ROOT"));
		if (Root.IsEmpty())
		{
			Root = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), TEXT(".."), TEXT("core")));
		}
		return FPaths::Combine(Root, TEXT("build"), TEXT("RelWithDebInfo"));
	}

	/** Restores MXCSR on scope exit, so a failing test cannot leave FTZ / DAZ set on the editor's thread. */
	struct FMxcsrRestore
	{
		const uint32 Saved = FKspFpGuard::ReadMxcsr();
		~FMxcsrRestore() { FKspFpGuard::WriteMxcsr(Saved); }
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKspBridgeLinksCoreTest, "KspUe5.Bridge.LinksCore",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)
bool FKspBridgeLinksCoreTest::RunTest(const FString& Parameters)
{
	const FString Info = FKspCoreBridge::GetCoreBuildInfo();
	AddInfo(FString::Printf(TEXT("kspcore build info: %s (stub=%d, manifest libraries=%d)"), *Info, FKspCoreBridge::IsStub() ? 1 : 0,
		FKspCoreBridge::GetManifestLibraryCount()));
	TestFalse(TEXT("build info is non-empty"), Info.IsEmpty());
	TestTrue(TEXT("core built against the release CRT DLL (/MD)"), Info.Contains(TEXT("crt=MD ")));
	TestTrue(TEXT("core built with _ITERATOR_DEBUG_LEVEL=0"), Info.Contains(TEXT("idl=0")));
	TestTrue(TEXT("core built as C++20"), Info.Contains(TEXT("lang=202002")));
	TestEqual(TEXT("C ABI version the bridge compiled against"), FKspCoreBridge::GetCompiledAbiVersion(), 1);
	TestTrue(TEXT("build info carries the ABI version the bridge compiled against"),
		Info.Contains(FString::Printf(TEXT("abi=%d "), FKspCoreBridge::GetCompiledAbiVersion())));

	// The real core, not the stub, and the manifest listed kspcore.lib and Jolt.lib.
	TestFalse(TEXT("linked the real core (not ue5/Stub)"), FKspCoreBridge::IsStub());
	if (!FKspCoreBridge::IsStub())
	{
		TestEqual(TEXT("C ABI version the library reports"), FKspCoreBridge::GetCoreAbiVersion(), FKspCoreBridge::GetCompiledAbiVersion());
		TestTrue(TEXT("link manifest lists kspcore.lib and Jolt.lib"), FKspCoreBridge::GetManifestLibraryCount() >= 2);
		// The definitions in the manifest (JPH_*) describe the Jolt the core was built with; the library says what it is.
		const FString Linked = FKspCoreBridge::GetCoreJoltConfig();
		AddInfo(FString::Printf(TEXT("Jolt as linked: %s; as declared by the manifest: %s"), *Linked.TrimEnd().Replace(TEXT("\n"), TEXT(" ")),
			*FKspCoreBridge::GetManifestJoltConfig().TrimEnd().Replace(TEXT("\n"), TEXT(" "))));
		TestEqual(TEXT("Jolt configuration of the linked library equals the manifest's definitions"), Linked, FKspCoreBridge::GetManifestJoltConfig());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKspBridgeFpGuardTest, "KspUe5.Bridge.FpGuard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)
bool FKspBridgeFpGuardTest::RunTest(const FString& Parameters)
{
	if (FKspCoreBridge::IsStub() || FKspFpGuard::ReadMxcsr() == 0)
	{
		AddInfo(TEXT("stub core or no MXCSR on this CPU: nothing to check"));
		return true;
	}
	FMxcsrRestore Restore;

	// Hostile caller state: flush-to-zero (bit 15) and denormals-are-zero (bit 6) on, all exceptions masked.
	constexpr uint32 Hostile = 0x1F80u | 0x8000u | 0x0040u;
	FKspFpGuard::WriteMxcsr(Hostile);
	TestEqual(TEXT("the hostile state took effect"), FKspFpGuard::ReadMxcsr(), Hostile);

	// Through the bridge, core code sees exactly the state its own tests run in.
	// control bits only (0xFFC0): the low six bits are sticky exception flags that core code legitimately sets
	TestEqual(TEXT("MXCSR control bits inside core, called through the bridge"), FKspCoreBridge::ReadCoreMxcsr() & 0xFFC0u, FKspFpGuard::CoreMxcsr);
	TestEqual(TEXT("caller MXCSR restored after the call"), FKspFpGuard::ReadMxcsr(), Hostile);

	// Nested guards restore in order.
	{
		FKspFpGuard Outer;
		TestEqual(TEXT("inside the guard"), FKspFpGuard::ReadMxcsr(), FKspFpGuard::CoreMxcsr);
		{
			FKspFpGuard Inner;
			TestEqual(TEXT("inside the nested guard"), FKspFpGuard::ReadMxcsr(), FKspFpGuard::CoreMxcsr);
		}
		TestEqual(TEXT("after the nested guard"), FKspFpGuard::ReadMxcsr(), FKspFpGuard::CoreMxcsr);
	}
	TestEqual(TEXT("after the outer guard"), FKspFpGuard::ReadMxcsr(), Hostile);

	// Without the guard core would see the hostile state: the probe is the proof that the guard is what fixes it.
	// (kspcore_test_mxcsr called raw is not exposed by the facade on purpose; the guard is the only way in.)
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKspBridgeJoltStepTest, "KspUe5.Bridge.JoltStep",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)
bool FKspBridgeJoltStepTest::RunTest(const FString& Parameters)
{
	if (FKspCoreBridge::IsStub())
	{
		AddError(TEXT("the stub core has no Jolt scenario: build with bUseRealCore"));
		return false;
	}

	// The ctest twin (core/tests/capi/capi_jolt_step.c, ctest capi.jolt_step) stepped the same scenario through the same test ABI
	// entry and wrote its hash; both processes run the same kspcore.lib + Jolt.lib objects.
	const FString TwinFile = FPaths::Combine(CoreBuildDir(), TEXT("kspcore_jolt_step.txt"));
	TArray<FString> Lines;
	if (!FFileHelper::LoadFileToStringArray(Lines, *TwinFile))
	{
		AddError(FString::Printf(TEXT("ctest twin output missing: %s (run: ctest --preset relwithdebinfo -R capi.jolt_step in core, or Scripts\\test.ps1)"), *TwinFile));
		return false;
	}
	int32 Steps = 0;
	uint64 CtestHash = 0;
	FString TwinInfo;
	for (const FString& Line : Lines)
	{
		FString Key, Value;
		if (!Line.Split(TEXT("="), &Key, &Value))
		{
			continue;
		}
		if (Key == TEXT("pile_steps"))
		{
			Steps = FCString::Atoi(*Value);
		}
		else if (Key == TEXT("pile_hash"))
		{
			CtestHash = FCString::Strtoui64(*Value, nullptr, 16);
		}
		else if (Key == TEXT("build_info"))
		{
			TwinInfo = Value;
		}
	}
	TestTrue(TEXT("twin file has a step count and a hash"), Steps > 0 && CtestHash != 0);
	TestEqual(TEXT("twin and bridge report the same core build"), TwinInfo, FKspCoreBridge::GetCoreBuildInfo());

	uint64 Hash1 = 0, Hash1b = 0, Hash4 = 0, HashHostile = 0;
	TestTrue(TEXT("scenario ran (1 thread)"), FKspCoreBridge::RunJoltPileScenario(1, Steps, Hash1));
	TestTrue(TEXT("scenario ran again (1 thread)"), FKspCoreBridge::RunJoltPileScenario(1, Steps, Hash1b));
	TestTrue(TEXT("scenario ran (4 threads)"), FKspCoreBridge::RunJoltPileScenario(4, Steps, Hash4));
	AddInfo(FString::Printf(TEXT("pile, %d steps: UE hash 0x%016llx, ctest hash 0x%016llx"), Steps, Hash1, CtestHash));
	TestEqual(TEXT("state hash equals the ctest twin's"), Hash1, CtestHash);
	TestEqual(TEXT("state hash run to run"), Hash1b, Hash1);
	TestEqual(TEXT("state hash at 4 threads equals 1 thread"), Hash4, Hash1);

	// A hostile caller floating-point state (FTZ / DAZ on) must not change the result: the bridge's guard restores core's own.
	if (FKspFpGuard::ReadMxcsr() != 0)
	{
		FMxcsrRestore Restore;
		FKspFpGuard::WriteMxcsr(0x1F80u | 0x8000u | 0x0040u);
		TestTrue(TEXT("scenario ran under FTZ / DAZ"), FKspCoreBridge::RunJoltPileScenario(1, Steps, HashHostile));
		TestEqual(TEXT("state hash is independent of the caller's MXCSR"), HashHostile, Hash1);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
