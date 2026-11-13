#include "KspCoreBridge.h"

#include "Modules/ModuleManager.h"

#include "kspcore/kspcore_c.h"
#if !KSPCORE_STUB
#include "kspcore/kspcore_test.h"
#endif

#if PLATFORM_CPU_X86_FAMILY
#include <xmmintrin.h>
#endif

static_assert(KSPCORE_ABI_VERSION == 1, "KspCoreBridge was written against kspcore C ABI v1");

DEFINE_LOG_CATEGORY_STATIC(LogKspCoreBridge, Log, All);

class FKspCoreBridgeModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		UE_LOG(LogKspCoreBridge, Log, TEXT("KspCoreBridge loaded: %s (stub=%d)"), *FKspCoreBridge::GetCoreBuildInfo(), KSPCORE_STUB);
	}
};

IMPLEMENT_MODULE(FKspCoreBridgeModule, KspCoreBridge)

// ---- FP environment guard -------------------------------------------------------------------------------------------------

uint32 FKspFpGuard::ReadMxcsr()
{
#if PLATFORM_CPU_X86_FAMILY
	return _mm_getcsr();
#else
	return 0;
#endif
}

void FKspFpGuard::WriteMxcsr(uint32 Value)
{
#if PLATFORM_CPU_X86_FAMILY
	_mm_setcsr(Value);
#else
	(void)Value;
#endif
}

FKspFpGuard::FKspFpGuard()
	: Saved(ReadMxcsr())
{
	WriteMxcsr(CoreMxcsr);
}

FKspFpGuard::~FKspFpGuard()
{
	WriteMxcsr(Saved);
}

// ---- facade ---------------------------------------------------------------------------------------------------------------

FString FKspCoreBridge::GetCoreBuildInfo()
{
	FKspFpGuard Guard;
	const char* Info = kspcore_build_info();
	return Info ? FString(UTF8_TO_TCHAR(Info)) : FString();
}

int32 FKspCoreBridge::GetCompiledAbiVersion()
{
	return KSPCORE_ABI_VERSION;
}

int32 FKspCoreBridge::GetCoreAbiVersion()
{
#if !KSPCORE_STUB
	FKspFpGuard Guard;
	return kspcore_abi_version();
#else
	return 0;
#endif
}

bool FKspCoreBridge::IsStub()
{
	return KSPCORE_STUB != 0;
}

int32 FKspCoreBridge::GetManifestLibraryCount()
{
#ifdef KSPCORE_MANIFEST_LIB_COUNT
	return KSPCORE_MANIFEST_LIB_COUNT;
#else
	return 0;
#endif
}

FString FKspCoreBridge::GetManifestJoltConfig()
{
	// JPH_* come from the link manifest (KspCoreBridge.Build.cs); only the ones observable through the test ABI are listed.
#ifdef JPH_DOUBLE_PRECISION
	return TEXT("double_precision=1\n");
#else
	return TEXT("double_precision=0\n");
#endif
}

FString FKspCoreBridge::GetCoreJoltConfig()
{
#if !KSPCORE_STUB
	FKspFpGuard Guard;
	return FString(UTF8_TO_TCHAR(kspcore_test_jolt_config()));
#else
	return FString();
#endif
}

uint32 FKspCoreBridge::ReadCoreMxcsr()
{
#if !KSPCORE_STUB
	FKspFpGuard Guard;
	return kspcore_test_mxcsr();
#else
	return 0;
#endif
}

bool FKspCoreBridge::RunJoltPileScenario(int32 Threads, int32 Steps, uint64& OutHash)
{
#if !KSPCORE_STUB
	FKspFpGuard Guard;
	uint64_t Hash = 0;
	if (kspcore_test_physics_scenario(KSPCORE_TEST_SCENARIO_PILE, Threads, Steps, &Hash) != 1)
	{
		return false;
	}
	OutHash = Hash;
	return true;
#else
	return false;
#endif
}
