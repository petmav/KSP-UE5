#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/Pawn.h"

#include "SpikeActors.generated.h"

class UCameraComponent;
class UProceduralMeshComponent;
class UStaticMeshComponent;
class UMaterialInterface;

/** Camera pawn driven entirely by ASpikeDirector. 90 deg horizontal FOV, fixed exposure, no bloom/blur/vignette. */
UCLASS()
class KSPUE5_API ASpikeCameraPawn : public APawn
{
	GENERATED_BODY()

public:
	ASpikeCameraPawn();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> Camera;
};

/** 1 m marker cube (unlit vertex colour). Geometry is baked relative to the actor origin at LocalCentre. */
UCLASS()
class KSPUE5_API ASpikeMarkerCube : public AActor
{
	GENERATED_BODY()

public:
	ASpikeMarkerCube();

	/** Rebuilds the mesh: cube centre at LocalCentre (relative to actor origin), axes in world orientation. */
	void Build(const FVector3d& LocalCentre, const FVector3d& Ax, const FVector3d& Ay, const FVector3d& Az, double HalfSize,
		const FColor& Color, UMaterialInterface* Material);

	FVector3d GetLocalCentre() const { return LocalCentre; }
	double GetHalfSize() const { return HalfSize; }
	FVector3d Axes[3] = { FVector3d::XAxisVector, FVector3d::YAxisVector, FVector3d::ZAxisVector };

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UProceduralMeshComponent> Mesh;

private:
	FVector3d LocalCentre = FVector3d::ZeroVector;
	double HalfSize = 50.0;
};

/** Engine cube used as a stand-in vessel part (simulating rigid body) or a static collider (PQS quad stand-in). */
UCLASS()
class KSPUE5_API ASpikeBoxActor : public AActor
{
	GENERATED_BODY()

public:
	ASpikeBoxActor();

	void InitBox(bool bSimulate, bool bVisible, const FVector& AngularVelocityDeg);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Box;
};

UCLASS()
class KSPUE5_API ASpikeGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASpikeGameMode();
	virtual void StartPlay() override;
};
