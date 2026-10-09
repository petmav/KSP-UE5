#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Spike/SpikeMath.h"

#include "SpikeDirector.generated.h"

class ASpikePlanet;
class ASpikeMarkerCube;
class ASpikeCameraPawn;
class ASpikeBoxActor;
class UMaterialInterface;

UENUM()
enum class ESpikeRebase : uint8
{
	None,      // LWC only, world origin fixed at Kerbin's centre
	UEOrigin,  // UWorld::SetNewWorldOrigin when the camera is ThresholdCm from the origin
	MoveWorld  // keep the camera near the origin by SetActorLocation on every movable actor
};

/**
 * Phase-0 scale spike runner. Spawned by ASpikeGameMode. Runs a scripted programme of camera paths for each
 * rebasing variant, measures frame times, origin-shift cost, a numeric jitter proxy (float32 / LWC emulation)
 * and an image jitter proxy (per-frame centroid of marker cubes read back from the viewport), writes CSVs to
 * <Project>/spike/data and PNGs to <Project>/spike/shots, then quits. Options (command line):
 *   -SpikeVariants=None,UE100k,UE10k,MW100k,MW10k   -SpikeStress=0|1   -SpikeQuick   -SpikeTag=<name>
 *   -SpikeFlipWinding   -SpikePlanetCentred   -SpikeNoSkyLight   -SpikeSunLux=<lux>   -SpikeParts=457 -SpikeColliders=1325
 */
UCLASS()
class KSPUE5_API ASpikeDirector : public AActor
{
	GENERATED_BODY()

public:
	ASpikeDirector();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	enum class EPhase : uint8
	{
		Warmup, OrbitImg, OrbitTiming, OrbitWarp, Descent, Horizon, GroundStatic, GroundStrafe, MunFromOrbit, MunNear, Stress
	};
	static constexpr int32 NumRefs = 6;
	static constexpr int32 NumTargets = 3; // red cube A, yellow cube B, magenta Mun

private:
	struct FStep
	{
		FString Label;
		FString Variant;
		EPhase Phase = EPhase::Warmup;
		ESpikeRebase Mode = ESpikeRebase::None;
		double ThresholdCm = 0.0;
		int32 Frames = 0;
		double Seconds = 0.0;
		bool bKspScene = false;
		bool bKspNoPhysics = false;
		bool bFullShots = false;
	};

	struct FRow
	{
		int32 Step = 0;
		int32 Frame = 0;
		double SimT = 0.0;
		double RealDtMs = 0.0, GameMs = 0.0, RenderMs = 0.0, RhiMs = 0.0, GpuMs = 0.0;
		bool bShift = false;
		double ShiftMs = 0.0;
		double LodMs = 0.0;
		int32 Patches = 0;
		double AltKm = 0.0;
		double CamFromOriginKm = 0.0;
		FVector3d OriginKm = FVector3d::ZeroVector;
		double Jit[NumRefs] = {};
		double JitPx[NumRefs] = {};
		bool bImg = false;
		double Cx[NumTargets] = {}, Cy[NumTargets] = {};
		int32 Cn[NumTargets] = {};
	};

	struct FPendingCapture
	{
		int32 Row = INDEX_NONE;
		FIntRect Roi[NumTargets];
		bool bRoi[NumTargets] = {};
		FString SaveTo;
	};

	void ParseOptions();
	void ApplyRenderSettings();
	void SpawnScene();
	void SpawnKspScene();
	void BuildProgram();
	void BeginStep();
	void TickStep();
	void EndStep();
	void Finish();

	void ComputePose(const FStep& S, int32 Frame, FVector3d& OutCam, FQuat4d& OutRot, double& OutSimDt);
	FQuat4d OrbitAttitude(double Theta) const;
	FVector3d OrbitPos(double Theta) const;

	double ShiftOriginTo(const FVector3d& NewOriginU, ESpikeRebase Mode);
	void MoveWorldBy(const FVector& Offset);
	void ResetOrigin();
	void ValidateOrigin();
	FVector ToWorld(const FVector3d& U) const { return FVector(U - Origin); }

	void SetupCubesForStep(const FStep& S, const FVector3d& Cam, const FQuat4d& Rot);
	void UpdateCoMovingCubes(const FVector3d& Cam);
	bool ProjectPoint(const FVector3d& PU, const FVector3d& Cam, const FQuat4d& Rot, FVector2D& OutPx) const;
	bool ProjectCubeRoi(const FVector3d& CentreU, const FVector3d Axes[3], double H, const FVector3d& Cam, const FQuat4d& Rot, FIntRect& Out) const;
	void RequestCapture(int32 RowIndex, const FVector3d& Cam, const FQuat4d& Rot, bool bMeasure, const FString& SaveTo);
	void OnScreenshot(int32 Width, int32 Height, const TArray<FColor>& Colors);

	FString ShotPath(const FString& Name) const;
	void AppendText(const FString& File, const FString& Text) const;

	// ---- options ----
	FString Tag = TEXT("run");
	TArray<FString> VariantNames;
	bool bStress = true;
	bool bQuick = false;
	bool bFlipWinding = false;
	bool bPlanetCentred = false;
	bool bSkyLight = true;
	float SunLux = 10.0f;
	int32 NumParts = 457;
	int32 NumColliders = 1325;
	FString OutDir;

	// ---- scene ----
	UPROPERTY(Transient) TObjectPtr<ASpikePlanet> Kerbin;
	UPROPERTY(Transient) TObjectPtr<ASpikePlanet> Mun;
	UPROPERTY(Transient) TObjectPtr<ASpikeMarkerCube> CubeA;
	UPROPERTY(Transient) TObjectPtr<ASpikeMarkerCube> CubeB;
	UPROPERTY(Transient) TObjectPtr<ASpikeCameraPawn> CamPawn;
	UPROPERTY(Transient) TObjectPtr<ASpikeBoxActor> WarmupBox;
	UPROPERTY(Transient) TArray<TObjectPtr<ASpikeBoxActor>> KspBoxes;
	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> LitVertexColorMat;
	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> UnlitVertexColorMat;
	TArray<FVector3d> KspBoxesU;

	// ---- universe frame: Kerbin centre at U = 0; UE world W = U - Origin ----
	FVector3d Origin = FVector3d::ZeroVector;   // = UWorld::OriginLocation + MwOffset
	FVector3d MwOffset = FVector3d::ZeroVector; // part of Origin created by MoveWorld shifts
	// Generic directions on purpose: every axis carries a large coordinate, so float32 precision loss shows up
	// for motion in any direction (an axis-aligned site would hide it). Set in BeginPlay.
	FVector3d SiteDir = FVector3d::XAxisVector, East = FVector3d::YAxisVector, North = FVector3d::ZAxisVector;
	FVector3d MunDir = FVector3d::YAxisVector;
	FVector3d MunCentreU = FVector3d::ZeroVector;
	FVector3d MunMotionDir = FVector3d::ZAxisVector;
	double OrbitTheta0 = UE_DOUBLE_PI * 0.25;
	double OrbitRadiusCm = KspSpike::KerbinRadiusCm + 1.0e7;
	double OrbitSpeedCmS = 0.0;
	double OrbitTheta = 0.0;

	// ---- programme ----
	TArray<FStep> Steps;
	int32 StepIndex = 0;
	int32 StepFrame = -1;
	double StepStartTime = 0.0;
	double ProgramStartTime = 0.0;
	int32 StepFirstRow = 0;
	int32 WarmupQuietFrames = 0;
	bool bFinished = false;
	bool bHeaderWritten = false;

	// per-step state
	FQuat4d StepRot = FQuat4d::Identity;
	FVector3d StepCam = FVector3d::ZeroVector;
	bool bCubesVisible = false;
	bool bCubesCoMoving = false;
	FVector3d CubeOffsetA = FVector3d(300, -70, -60), CubeOffsetB = FVector3d(300, 70, -60);
	FVector3d CubeCentreA = FVector3d::ZeroVector, CubeCentreB = FVector3d::ZeroVector;
	FVector3d CubeStartB = FVector3d::ZeroVector;
	FVector3d BodyCentreForB = FVector3d::ZeroVector;
	FVector3d CubeAxes[3] = { FVector3d::XAxisVector, FVector3d::YAxisVector, FVector3d::ZAxisVector };
	FVector3d GroundRef = FVector3d::ZeroVector, GroundPatchOrigin = FVector3d::ZeroVector, GroundBody = FVector3d::ZeroVector;
	KspSpike::FJitterTracker Trackers[NumRefs];
	KspSpike::FJitterTracker PxTrackers[NumRefs];
	double StepFpx = 640.0;
	double StepHalfW = 640.0, StepHalfH = 360.0;
	bool bKspPhysicsDisabled = false;
	bool bCsvProfiling = false;
	double StepDist[NumTargets] = { 314.0, 314.0, 1.0 };
	double ShiftValidErrMax = 0.0;
	TArray<double> DescentShotAltsKm;
	double PrevAltKm = 0.0;

	// rows + captures
	TArray<FRow> Rows;
	TArray<FPendingCapture> Pending;
	FDelegateHandle ScreenshotHandle;
	int32 CapturesReceived = 0;
};
