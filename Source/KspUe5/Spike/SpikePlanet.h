#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Spike/SpikeMath.h"

#include "SpikePlanet.generated.h"

class UProceduralMeshComponent;
class USkyAtmosphereComponent;
class UMaterialInterface;

/**
 * Cubesphere planet: 6 faces, quadtree of ProceduralMeshComponent patches selected by camera distance.
 * Each patch is its own primitive whose origin sits at the patch centre, so vertex coordinates stay small
 * (the property that keeps LWC rendering jitter-free; KSP's PQS does the same with one GameObject per quad).
 */
UCLASS()
class KSPUE5_API ASpikePlanet : public AActor
{
	GENERATED_BODY()

public:
	ASpikePlanet();

	struct FConfig
	{
		double RadiusCm = KspSpike::KerbinRadiusCm;
		int32 MinLevel = 0;
		int32 MaxLevel = 14;
		int32 GridN = 16;
		double SplitFactor = 2.5;
		bool bAtmosphere = false;
		double AtmosphereHeightKm = 70.0;
		bool bPlanetCentredVerts = false;
		bool bFlipWinding = false;
		KspSpike::EPatchColor ColorMode = KspSpike::EPatchColor::Terrain;
		FColor FlatColor = FColor::White;
		UMaterialInterface* Material = nullptr;
	};

	void Configure(const FConfig& InConfig);

	/**
	 * Re-evaluates the quadtree for a camera at CameraWorld (UE world space). Skips the work unless the camera
	 * moved more than max(1 m, 2% of altitude) since the last evaluation, or bForce. Returns milliseconds spent.
	 */
	double UpdateLod(const FVector& CameraWorld, bool bForce, int32& OutCreated, int32& OutDestroyed);

	int32 NumPatches() const { return Patches.Num(); }
	double GetRadius() const { return Config.RadiusCm; }
	const FConfig& GetConfig() const { return Config; }

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USkyAtmosphereComponent> Atmosphere;

private:
	static uint64 MakeKey(int32 Face, int32 Level, int32 IX, int32 IY);
	void Collect(int32 Face, int32 Level, int32 IX, int32 IY, const FVector3d& CamLocal, TSet<uint64>& Out) const;
	UProceduralMeshComponent* CreatePatch(uint64 Key);

	FConfig Config;

	UPROPERTY(Transient)
	TMap<uint64, TObjectPtr<UProceduralMeshComponent>> Patches;

	FVector3d LastEvalLocal = FVector3d::ZeroVector;
	bool bHasEval = false;
};
