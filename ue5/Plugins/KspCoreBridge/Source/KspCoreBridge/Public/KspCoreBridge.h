#pragma once

#include "CoreMinimal.h"

/** Thin, engine-side facade over the kspcore C ABI. Everything else in the game talks to this, never to kspcore directly. */
class KSPCOREBRIDGE_API FKspCoreBridge
{
public:
	/** Build-info string reported by the linked kspcore library (stub or real). */
	static FString GetCoreBuildInfo();

	/** ABI version the bridge was compiled against (KSPCORE_ABI_VERSION from kspcore_c.h). */
	static int32 GetCompiledAbiVersion();

	/** True when linked against ue5/Stub instead of the real core. */
	static bool IsStub();
};
