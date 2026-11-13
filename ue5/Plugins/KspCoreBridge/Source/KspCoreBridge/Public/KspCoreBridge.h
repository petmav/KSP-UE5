#pragma once

#include "CoreMinimal.h"

/**
 * Floating-point environment guard around every call into kspcore (ue5/DESIGN.md 1.5 point 4).
 *
 * kspcore's numerics are gated bit for bit in the MXCSR state the core's own test binaries run in: round to nearest, all
 * exceptions masked, no flush-to-zero / denormals-are-zero (0x1F80). Unreal's game thread, a plugin or a driver may run with a
 * different state. The constructor saves MXCSR and sets 0x1F80, the destructor restores the saved value. Nest freely.
 * Jolt's own scoped denormal flushing inside its update is not touched. No-op on CPUs without MXCSR.
 */
class KSPCOREBRIDGE_API FKspFpGuard
{
public:
	/** The state kspcore requires: round to nearest, exceptions masked, FTZ and DAZ off. */
	static constexpr uint32 CoreMxcsr = 0x1F80u;

	FKspFpGuard();
	~FKspFpGuard();
	FKspFpGuard(const FKspFpGuard&) = delete;
	FKspFpGuard& operator=(const FKspFpGuard&) = delete;

	/** The current thread's MXCSR (0 where there is none). */
	static uint32 ReadMxcsr();
	/** Sets the current thread's MXCSR. Test and guard use only. */
	static void WriteMxcsr(uint32 Value);

private:
	uint32 Saved = 0;
};

/** Thin, engine-side facade over the kspcore C ABI. Everything else in the game talks to this, never to kspcore directly. */
class KSPCOREBRIDGE_API FKspCoreBridge
{
public:
	/** Build-info string reported by the linked kspcore library (stub or real). */
	static FString GetCoreBuildInfo();

	/** ABI version the bridge was compiled against (KSPCORE_ABI_VERSION from kspcore_c.h). */
	static int32 GetCompiledAbiVersion();

	/** ABI version the linked library reports at run time (kspcore_abi_version). 0 for the stub. */
	static int32 GetCoreAbiVersion();

	/** True when linked against ue5/Stub instead of the real core. */
	static bool IsStub();

	/** Number of libraries the link manifest listed (kspcore.lib, Jolt.lib ...). 0 for the stub. */
	static int32 GetManifestLibraryCount();

	/** Jolt configuration the bridge's definitions (from the link manifest) describe, "key=value\n" lines as GetCoreJoltConfig. */
	static FString GetManifestJoltConfig();

	// ---- test ABI (kspcore_test.h): only for automation tests; not available on the stub (the calls return false / empty) ----

	/** Jolt configuration of the linked library as observed by behaviour, e.g. "double_precision=1\n". */
	static FString GetCoreJoltConfig();

	/** MXCSR as seen inside kspcore.lib, called through the guard. */
	static uint32 ReadCoreMxcsr();

	/** Steps the core's pile scenario (Jolt, 0.02 s step) through the test ABI under the FP guard; false on failure. */
	static bool RunJoltPileScenario(int32 Threads, int32 Steps, uint64& OutHash);
};
