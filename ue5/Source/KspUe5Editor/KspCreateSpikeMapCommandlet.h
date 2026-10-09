#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"

#include "KspCreateSpikeMapCommandlet.generated.h"

/**
 * Generates /Game/Maps/ScaleSpike.umap from code so the map stays reproducible and agent-legible:
 * an empty world whose WorldSettings point at ASpikeGameMode (which spawns everything at runtime).
 * Run: UnrealEditor-Cmd.exe <uproject> -run=KspCreateSpikeMap -unattended -nopause
 */
UCLASS()
class UKspCreateSpikeMapCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UKspCreateSpikeMapCommandlet();
	virtual int32 Main(const FString& Params) override;
};
