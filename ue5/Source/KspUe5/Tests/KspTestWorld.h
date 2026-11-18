#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

/**
 * A game world ticked by a test, without game instance or game mode (the project's default game mode is the scale spike's).
 * Each Tick advances GFrameCounter as the engine loop would: a tick function runs once per engine frame.
 */
struct FKspTestWorld
{
	UWorld* World = nullptr;
	const uint64 SavedFrameCounter = GFrameCounter;

	FKspTestWorld()
	{
		World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("KspTestWorld"));
		World->AddToRoot();
		World->SetShouldTick(false);
		GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		World->BeginPlay();
	}

	~FKspTestWorld()
	{
		World->BeginTearingDown();
		World->RemoveFromRoot();
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		GFrameCounter = SavedFrameCounter;
	}

	void Tick(float DeltaSeconds)
	{
		World->Tick(LEVELTICK_All, DeltaSeconds);
		++GFrameCounter;
	}
};
