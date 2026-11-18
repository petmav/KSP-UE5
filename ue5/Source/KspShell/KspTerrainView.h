#pragma once

#include "CoreMinimal.h"
#include "KspRenderAnchor.h"
#include "KspSim.h"
#include "UObject/Object.h"

#include "KspTerrainView.generated.h"

class UMaterialInterface;
class UProceduralMeshComponent;

/**
 * The terrain of the dominant body as core's PQS builds it (WP-UE-11, ue5/DESIGN.md 3.2): one mesh component per quad of the
 * snapshot's terrain stream, pooled. A quad's vertices are floats relative to the quad's own origin (the jitter-free rule of the scale
 * spike); the origin is a double in the body's fixed frame. All quads hang below one actor that stands for that frame, so a frame costs
 * one transform when the body or the render anchor moves and nothing otherwise.
 *
 * Drawn with the PQS vertex colours only (the colour maps of the stock mod stack); the surface textures of the game's terrain
 * material are not applied yet.
 */
UCLASS()
class KSPSHELL_API UKspTerrainView : public UObject
{
	GENERATED_BODY()

public:
	/** Applies one frame of the terrain stream: removed quads go back to the pool, added or re-stitched ones are (re)built. */
	void Apply(UWorld* World, TConstArrayView<FKspTerrainQuad> Added, TConstArrayView<FKspTerrainRemoved> Removed);
	/** Puts the body frame where the snapshot has it, relative to the render anchor. */
	void Place(const FKspRenderAnchor& Anchor, TConstArrayView<FKspBodyState> Bodies);
	/** Destroys everything (scene change). */
	void Clear();

	int32 NumQuads() const { return Quads.Num(); }
	/** The component of a quad that is drawn, null otherwise. */
	const UProceduralMeshComponent* FindQuad(int32 QuadId) const;
	const TMap<int32, TObjectPtr<UProceduralMeshComponent>>& GetQuads() const { return Quads; }

	struct FStats
	{
		int32 Added = 0;
		int32 Restitched = 0;
		int32 Removed = 0;
		int32 MaxAtOnce = 0;
		int32 ComponentsMade = 0;
		/** Seconds spent building mesh sections, and the longest single frame of it. */
		double BuildSeconds = 0.0;
		double MaxFrameSeconds = 0.0;
	};
	const FStats& GetStats() const { return Stats; }
	FString DescribeStats() const;

private:
	UPROPERTY(Transient)
	TObjectPtr<AActor> BodyFrame;

	/** Quad id -> its component. */
	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<UProceduralMeshComponent>> Quads;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UProceduralMeshComponent>> Pool;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> Material;

	int32 BodyId = INDEX_NONE;
	FStats Stats;
};
