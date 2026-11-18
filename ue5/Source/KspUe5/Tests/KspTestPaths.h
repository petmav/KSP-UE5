#pragma once

#include "CoreMinimal.h"
#include "HAL/PlatformMisc.h"
#include "Misc/Paths.h"

/** Where the automation tests find the repository and the core build their ctest twins ran in. */
namespace KspTestPaths
{
	/** The repository root (the folder holding core/, ue5/, harness/ ...). */
	inline FString RepoRoot()
	{
		return FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), TEXT("..")));
	}

	/**
	 * <KSPCORE_ROOT or <repository>/core>/build/RelWithDebInfo: where the core build puts kspcore_link.txt and where the ctest twins
	 * write their hashes.
	 */
	inline FString CoreBuildDir()
	{
		FString Root = FPlatformMisc::GetEnvironmentVariable(TEXT("KSPCORE_ROOT"));
		if (Root.IsEmpty())
		{
			Root = FPaths::Combine(RepoRoot(), TEXT("core"));
		}
		return FPaths::Combine(Root, TEXT("build"), TEXT("RelWithDebInfo"));
	}
}
