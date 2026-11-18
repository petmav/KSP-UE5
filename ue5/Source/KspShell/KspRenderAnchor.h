#pragma once

#include "CoreMinimal.h"

/**
 * The shell's render anchor (ue5/DESIGN.md 1.3, decision D6).
 *
 * Core exports every position as a double relative to the centre of the dominant body. The shell places every actor at
 * (position - anchor), so what Unreal renders stays near its world origin whatever the distance from the body's centre. The anchor
 * moves to the focus (the camera; the active vessel until the camera package lands) when the focus is more than 100 km from it, and at
 * every frame change of core (rotating <-> inertial axes, SOI change). There is no UWorld origin shift: all transforms are recomputed
 * from the snapshot every frame, so moving the anchor only costs the re-placement of static primitives.
 *
 * Positions are in UE axes and centimetres (FKspAxis).
 */
struct KSPSHELL_API FKspRenderAnchor
{
	static constexpr double ThresholdCm = 100.0 * 1000.0 * 100.0;

	/** Moves the anchor to Focus when there is none yet, when core's frame-change serial moved, or when Focus is beyond the threshold. True when it moved. */
	bool Update(const FVector& Focus, uint32 FrameChangeSerial);
	/** A dominant-body-relative position as an Unreal world position. */
	FVector ToWorld(const FVector& Position) const { return Position - Origin; }
	void Reset() { *this = FKspRenderAnchor(); }

	FVector Origin = FVector::ZeroVector;
	bool bValid = false;
	uint32 Serial = 0;
	/** Moves so far, the first placement included. */
	int32 MoveCount = 0;
};
