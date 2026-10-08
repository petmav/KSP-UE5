#pragma once

#include "CoreMinimal.h"

/**
 * Pure maths for the phase-0 scale spike: precision emulation of the float32 and UE5 LWC vertex pipelines,
 * and cubesphere patch generation. No UObjects, so the automation tests can exercise it headless.
 * Units are Unreal units (cm) unless noted.
 */
namespace KspSpike
{
	inline constexpr double KerbinRadiusCm = 6.0e7;              // 600 km
	inline constexpr double KerbinMuM3S2 = 3531599999999.9995;   // float-derived double, as in KSP
	inline constexpr double KerbinAtmosphereHeightKm = 70.0;
	inline constexpr double MunDistanceCm = 1.2e9;               // 12,000 km
	inline constexpr double MunRadiusCm = 2.0e7;                 // 200 km

	/** Exact camera-space position (camera axes X fwd, Y right, Z up) of absolute point V seen from camera C. */
	KSPUE5_API FVector3d CamSpaceExact(const FVector3d& V, const FVector3d& C, const FQuat4d& CamRot);

	/** Float32 world-space pipeline (UE4 / Unity style, no LWC): positions stored as float relative to world origin O. */
	KSPUE5_API FVector3d CamSpaceF32World(const FVector3d& V, const FVector3d& C, const FVector3d& O, const FQuat4d& CamRot);

	/**
	 * UE5 LWC pipeline, emulated op for op: vertex stored as float relative to its primitive origin P; primitive
	 * origin and camera position reach the GPU as double-float (high, low) pairs and are subtracted with
	 * DFFastLocalSubtractDemote; then float add of the local position and float view rotation.
	 * V, P, C must be UE *world* coordinates (after any origin rebasing): the rounding depends on their magnitude.
	 */
	KSPUE5_API FVector3d CamSpaceLwc(const FVector3d& V, const FVector3d& P, const FVector3d& C, const FQuat4d& CamRot);

	/** Per-frame jitter of an emulated pipeline: e_n = emulated - exact, jitter_n = |e_n - e_(n-1)|. */
	struct KSPUE5_API FJitterTracker
	{
		FVector3d PrevErr = FVector3d::ZeroVector;
		bool bHasPrev = false;
		double MaxJitter = 0.0;
		double SumSqJitter = 0.0;
		int32 Count = 0;
		double MaxAbsErr = 0.0;

		void Reset() { *this = FJitterTracker(); }
		/** Breaks the frame-to-frame chain (e.g. after a camera teleport); keeps the accumulated stats. */
		void Break() { bHasPrev = false; }
		double Add(const FVector3d& Emulated, const FVector3d& Exact);
		double Rms() const { return Count > 0 ? FMath::Sqrt(SumSqJitter / Count) : 0.0; }
	};

	// ---- cubesphere ------------------------------------------------------------------------------
	KSPUE5_API void FaceBasis(int32 Face, FVector3d& OutN, FVector3d& OutT1, FVector3d& OutT2);
	/** Spherified-cube mapping of a point on the unit cube surface to the unit sphere. */
	KSPUE5_API FVector3d CubeToSphere(const FVector3d& C);
	/** Unit direction of face point (U,V) in [-1,1]^2. */
	KSPUE5_API FVector3d FaceUVToDir(int32 Face, double U, double V);
	/** Approximate edge length (cm) of a quadtree patch at Level on a sphere of Radius. */
	KSPUE5_API double PatchEdgeLength(int32 Level, double Radius);
	/** Planet-local centre point of a patch. */
	KSPUE5_API FVector3d PatchCentre(int32 Face, int32 Level, int32 IX, int32 IY, double Radius);

	struct FPatchMesh
	{
		FVector3d Origin = FVector3d::ZeroVector; // primitive origin, planet-local (cm)
		TArray<FVector> Verts;                    // relative to Origin
		TArray<int32> Tris;
		TArray<FVector> Normals;
		TArray<FColor> Colors;
	};

	enum class EPatchColor : uint8 { Terrain, Flat };

	/**
	 * Builds an (N x N quads + skirts) patch. bPlanetCentred puts the primitive origin at the planet centre
	 * (the naive single-mesh layout; jitters under LWC), otherwise at the patch centre.
	 * Winding follows UE's convention (front face when cross(B-A, C-A) points away from the viewer, see
	 * KismetProceduralMeshLibrary.cpp GenerateBoxMesh). bFlipWinding inverts it.
	 */
	KSPUE5_API void BuildPatch(int32 Face, int32 Level, int32 IX, int32 IY, int32 N, double Radius, bool bPlanetCentred,
		bool bFlipWinding, EPatchColor ColorMode, const FColor& FlatColor, FPatchMesh& Out);

	/** Planet surface colour for a unit direction: ocean / land / ice. Never red or yellow (those mark the jitter cubes). */
	KSPUE5_API FColor SurfaceColor(const FVector3d& Dir);

	/** Cube mesh with centre Centre (relative to the primitive origin), orthonormal axes Ax/Ay/Az, half size H. */
	KSPUE5_API void BuildCube(const FVector3d& Centre, const FVector3d& Ax, const FVector3d& Ay, const FVector3d& Az, double H,
		const FColor& Color, TArray<FVector>& V, TArray<int32>& T, TArray<FVector>& N, TArray<FColor>& C);

	/** Appends a triangle so that it is front-facing when seen from the side Outward points to. */
	KSPUE5_API void AddTriFacing(TArray<int32>& T, const TArray<FVector>& V, int32 A, int32 B, int32 C, const FVector3d& Outward, bool bFlip);
}
