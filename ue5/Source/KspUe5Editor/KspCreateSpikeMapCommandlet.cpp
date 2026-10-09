#include "KspCreateSpikeMapCommandlet.h"

#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/PackageName.h"
#include "Spike/SpikeActors.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

DEFINE_LOG_CATEGORY_STATIC(LogKspCreateSpikeMap, Log, All);

UKspCreateSpikeMapCommandlet::UKspCreateSpikeMapCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UKspCreateSpikeMapCommandlet::Main(const FString& Params)
{
	const FString PackageName = TEXT("/Game/Maps/ScaleSpike");
	UPackage* Package = CreatePackage(*PackageName);
	if (!Package)
	{
		UE_LOG(LogKspCreateSpikeMap, Error, TEXT("CreatePackage failed"));
		return 1;
	}
	UWorld* World = UWorld::CreateWorld(EWorldType::Inactive, false, FName(TEXT("ScaleSpike")), Package, true);
	if (!World)
	{
		UE_LOG(LogKspCreateSpikeMap, Error, TEXT("CreateWorld failed"));
		return 1;
	}
	World->SetFlags(RF_Public | RF_Standalone);
	if (AWorldSettings* Settings = World->GetWorldSettings())
	{
		Settings->DefaultGameMode = ASpikeGameMode::StaticClass();
	}

	const FString Filename = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetMapPackageExtension());
	FSavePackageArgs Args;
	Args.TopLevelFlags = RF_Public | RF_Standalone;
	Args.SaveFlags = SAVE_NoError;
	const bool bOk = UPackage::SavePackage(Package, World, *Filename, Args);
	UE_LOG(LogKspCreateSpikeMap, Display, TEXT("Saved %s -> %s : %s"), *PackageName, *Filename, bOk ? TEXT("OK") : TEXT("FAILED"));

	World->DestroyWorld(false);
	World->RemoveFromRoot();
	return bOk ? 0 : 1;
}
