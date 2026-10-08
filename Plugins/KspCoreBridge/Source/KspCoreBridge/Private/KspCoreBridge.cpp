#include "KspCoreBridge.h"

#include "Modules/ModuleManager.h"

#include "kspcore/kspcore_c.h"

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

FString FKspCoreBridge::GetCoreBuildInfo()
{
	const char* Info = kspcore_build_info();
	return Info ? FString(UTF8_TO_TCHAR(Info)) : FString();
}

int32 FKspCoreBridge::GetCompiledAbiVersion()
{
	return KSPCORE_ABI_VERSION;
}

bool FKspCoreBridge::IsStub()
{
	return KSPCORE_STUB != 0;
}
