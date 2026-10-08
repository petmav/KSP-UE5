#include "Spike/SpikeDirector.h"

#include "KspUe5.h"
#include "Spike/SpikeActors.h"
#include "Spike/SpikePlanet.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "DynamicRHI.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/SkyLight.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Info.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMemory.h"
#include "ImageUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RHIStats.h"
#include "RenderTimer.h"
#include "UnrealClient.h"
#if WITH_EDITOR
#include "ShaderCompiler.h"
#endif

namespace
{
constexpr double FovDeg = 90.0;
constexpr double SimDt60 = 1.0 / 60.0;
constexpr int32 MinPixels = 20;

const TCHAR* PhaseName(ASpikeDirector::EPhase P)
{
	switch (P)
	{
	case ASpikeDirector::EPhase::Warmup: return TEXT("Warmup");
	case ASpikeDirector::EPhase::OrbitImg: return TEXT("OrbitImg");
	case ASpikeDirector::EPhase::OrbitTiming: return TEXT("OrbitTiming");
	case ASpikeDirector::EPhase::OrbitWarp: return TEXT("OrbitWarp");
	case ASpikeDirector::EPhase::Descent: return TEXT("Descent");
	case ASpikeDirector::EPhase::Horizon: return TEXT("Horizon");
	case ASpikeDirector::EPhase::GroundStatic: return TEXT("GroundStatic");
	case ASpikeDirector::EPhase::GroundStrafe: return TEXT("GroundStrafe");
	case ASpikeDirector::EPhase::MunFromOrbit: return TEXT("MunFromOrbit");
	case ASpikeDirector::EPhase::MunNear: return TEXT("MunNear");
	case ASpikeDirector::EPhase::Stress: return TEXT("Stress");
	}
	return TEXT("?");
}

const TCHAR* ModeName(ESpikeRebase M)
{
	switch (M)
	{
	case ESpikeRebase::None: return TEXT("None");
	case ESpikeRebase::UEOrigin: return TEXT("UEOrigin");
	case ESpikeRebase::MoveWorld: return TEXT("MoveWorld");
	}
	return TEXT("?");
}

bool IsRed(const FColor& C) { return C.R > 110 && C.G < 80 && C.B < 80; }
bool IsYellow(const FColor& C) { return C.R > 110 && C.G > 110 && C.B < 80; }
bool IsMagenta(const FColor& C) { return C.R > 110 && C.B > 110 && C.G < 80; }

FQuat4d MakeRot(const FVector3d& Fwd, const FVector3d& Up)
{
	return FRotationMatrix::MakeFromXZ(FVector(Fwd), FVector(Up)).ToQuat();
}

double Percentile(TArray<double> V, double P)
{
	if (V.Num() == 0)
	{
		return 0.0;
	}
	V.Sort();
	const int32 I = FMath::Clamp(FMath::FloorToInt32(P * (V.Num() - 1) + 0.5), 0, V.Num() - 1);
	return V[I];
}

double Mean(const TArray<double>& V)
{
	double S = 0.0;
	for (double X : V)
	{
		S += X;
	}
	return V.Num() ? S / V.Num() : 0.0;
}

/** Least-squares quadratic fit y = a + b n + c n^2; returns residuals. */
TArray<double> QuadResiduals(const TArray<double>& N, const TArray<double>& Y)
{
	TArray<double> R;
	const int32 K = N.Num();
	if (K < 4)
	{
		return R;
	}
	// Centre n for conditioning.
	double NMean = 0.0;
	for (double X : N) { NMean += X; }
	NMean /= K;
	double S[5] = {}, T[3] = {};
	for (int32 I = 0; I < K; ++I)
	{
		const double Nc = N[I] - NMean;
		double P = 1.0;
		for (int32 E = 0; E < 5; ++E)
		{
			S[E] += P;
			if (E < 3)
			{
				T[E] += P * Y[I];
			}
			P *= Nc;
		}
	}
	auto Det3 = [](double A11, double A12, double A13, double A21, double A22, double A23, double A31, double A32, double A33)
	{ return A11 * (A22 * A33 - A23 * A32) - A12 * (A21 * A33 - A23 * A31) + A13 * (A21 * A32 - A22 * A31); };
	const double D = Det3(S[0], S[1], S[2], S[1], S[2], S[3], S[2], S[3], S[4]);
	if (FMath::Abs(D) < 1e-30)
	{
		return R;
	}
	const double A = Det3(T[0], S[1], S[2], T[1], S[2], S[3], T[2], S[3], S[4]) / D;
	const double B = Det3(S[0], T[0], S[2], S[1], T[1], S[3], S[2], T[2], S[4]) / D;
	const double C = Det3(S[0], S[1], T[0], S[1], S[2], T[1], S[2], S[3], T[2]) / D;
	for (int32 I = 0; I < K; ++I)
	{
		const double Nc = N[I] - NMean;
		R.Add(Y[I] - (A + B * Nc + C * Nc * Nc));
	}
	return R;
}
} // namespace

ASpikeDirector::ASpikeDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Movable);
}

void ASpikeDirector::ParseOptions()
{
	const TCHAR* Cmd = FCommandLine::Get();
	FString Str;
	if (FParse::Value(Cmd, TEXT("SpikeVariants="), Str))
	{
		Str.ParseIntoArray(VariantNames, TEXT(","), true);
	}
	else
	{
		VariantNames = { TEXT("None"), TEXT("UE100k"), TEXT("UE10k"), TEXT("MW100k"), TEXT("MW10k") };
	}
	int32 Int = 1;
	if (FParse::Value(Cmd, TEXT("SpikeStress="), Int))
	{
		bStress = Int != 0;
	}
	FParse::Value(Cmd, TEXT("SpikeTag="), Tag);
	FParse::Value(Cmd, TEXT("SpikeSunLux="), SunLux);
	FParse::Value(Cmd, TEXT("SpikeParts="), NumParts);
	FParse::Value(Cmd, TEXT("SpikeColliders="), NumColliders);
	bQuick = FParse::Param(Cmd, TEXT("SpikeQuick"));
	bFlipWinding = FParse::Param(Cmd, TEXT("SpikeFlipWinding"));
	bPlanetCentred = FParse::Param(Cmd, TEXT("SpikePlanetCentred"));
	bSkyLight = !FParse::Param(Cmd, TEXT("SpikeNoSkyLight"));
	OutDir = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), TEXT("spike")));
	IFileManager::Get().MakeDirectory(*FPaths::Combine(OutDir, TEXT("data")), true);
	IFileManager::Get().MakeDirectory(*FPaths::Combine(OutDir, TEXT("shots")), true);
}

void ASpikeDirector::ApplyRenderSettings()
{
	// Low scalability everywhere, but full resolution (no upscale blur in the pixel measurements).
	static const TCHAR* Cmds[] = {
		TEXT("sg.ViewDistanceQuality 0"), TEXT("sg.AntiAliasingQuality 0"), TEXT("sg.ShadowQuality 0"),
		TEXT("sg.GlobalIlluminationQuality 0"), TEXT("sg.ReflectionQuality 0"), TEXT("sg.PostProcessQuality 0"),
		TEXT("sg.TextureQuality 0"), TEXT("sg.EffectsQuality 0"), TEXT("sg.FoliageQuality 0"), TEXT("sg.ShadingQuality 0"),
		TEXT("sg.ResolutionQuality 100"), TEXT("r.ScreenPercentage 100"), TEXT("r.AntiAliasingMethod 0"),
		TEXT("r.MotionBlurQuality 0"), TEXT("r.VSync 0"), TEXT("t.MaxFPS 0"), TEXT("r.DynamicGlobalIlluminationMethod 0"),
		TEXT("r.ReflectionMethod 0"), TEXT("r.Shadow.Virtual.Enable 0") };
	for (const TCHAR* C : Cmds)
	{
		GEngine->Exec(GetWorld(), C);
	}
}

void ASpikeDirector::BeginPlay()
{
	Super::BeginPlay();
	ParseOptions();
	ApplyRenderSettings();
	OrbitSpeedCmS = FMath::Sqrt(KspSpike::KerbinMuM3S2 / (OrbitRadiusCm / 100.0)) * 100.0;
	SiteDir = FVector3d(1, 1, 1).GetSafeNormal();                 // latitude 35.3 deg
	East = FVector3d(-SiteDir.Y, SiteDir.X, 0.0).GetSafeNormal();
	North = (FVector3d(0, 0, 1) - SiteDir * SiteDir.Z).GetSafeNormal();
	MunDir = FVector3d(0.35, 0.85, 0.40).GetSafeNormal();
	MunCentreU = MunDir * KspSpike::MunDistanceCm;
	MunMotionDir = FVector3d(MunDir.Y, -MunDir.X, 0.0).GetSafeNormal();
	OrbitTheta = OrbitTheta0;

	LitVertexColorMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineDebugMaterials/VertexColorMaterial.VertexColorMaterial"));
	UnlitVertexColorMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineDebugMaterials/VertexColorViewMode_ColorOnly.VertexColorViewMode_ColorOnly"));
	UE_LOG(LogKspUe5, Log, TEXT("SPIKE_INIT tag=%s variants=%s stress=%d quick=%d lit=%s unlit=%s orbit_v=%.1f m/s"), *Tag,
		*FString::Join(VariantNames, TEXT(",")), bStress ? 1 : 0, bQuick ? 1 : 0, LitVertexColorMat ? TEXT("ok") : TEXT("MISSING"),
		UnlitVertexColorMat ? TEXT("ok") : TEXT("MISSING"), OrbitSpeedCmS / 100.0);

	SpawnScene();
	BuildProgram();
	ScreenshotHandle = UGameViewportClient::OnScreenshotCaptured().AddUObject(this, &ASpikeDirector::OnScreenshot);
	ProgramStartTime = FPlatformTime::Seconds();
}

void ASpikeDirector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UGameViewportClient::OnScreenshotCaptured().Remove(ScreenshotHandle);
	Super::EndPlay(EndPlayReason);
}

void ASpikeDirector::SpawnScene()
{
	UWorld* World = GetWorld();
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	Kerbin = World->SpawnActor<ASpikePlanet>(FVector::ZeroVector, FRotator::ZeroRotator, P);
	ASpikePlanet::FConfig KC;
	KC.RadiusCm = KspSpike::KerbinRadiusCm;
	KC.MaxLevel = 14;
	KC.bAtmosphere = true;
	KC.AtmosphereHeightKm = KspSpike::KerbinAtmosphereHeightKm;
	KC.bPlanetCentredVerts = bPlanetCentred;
	KC.bFlipWinding = bFlipWinding;
	KC.Material = LitVertexColorMat;
	Kerbin->Configure(KC);

	Mun = World->SpawnActor<ASpikePlanet>(FVector(MunCentreU), FRotator::ZeroRotator, P);
	ASpikePlanet::FConfig MC;
	MC.RadiusCm = KspSpike::MunRadiusCm;
	MC.MinLevel = 2;
	MC.MaxLevel = 5;
	MC.bAtmosphere = false;
	MC.bFlipWinding = bFlipWinding;
	MC.ColorMode = KspSpike::EPatchColor::Flat;
	MC.FlatColor = FColor(255, 0, 255);
	MC.Material = UnlitVertexColorMat;
	Mun->Configure(MC);

	// Sun: 50 deg elevation over the test site, 30 deg north of east.
	const double El = FMath::DegreesToRadians(50.0);
	const double Az = FMath::DegreesToRadians(30.0);
	const FVector3d ToSun = (SiteDir * FMath::Sin(El) + (East * FMath::Cos(Az) + North * FMath::Sin(Az)) * FMath::Cos(El)).GetSafeNormal();
	ADirectionalLight* Sun = World->SpawnActor<ADirectionalLight>(FVector::ZeroVector, FVector(-ToSun).Rotation(), P);
	if (Sun)
	{
		Sun->GetRootComponent()->SetMobility(EComponentMobility::Movable);
		if (UDirectionalLightComponent* SC = Cast<UDirectionalLightComponent>(Sun->GetLightComponent()))
		{
			SC->SetIntensity(SunLux);
			SC->SetAtmosphereSunLight(true);
			SC->SetCastShadows(false);
		}
	}
	if (bSkyLight)
	{
		ASkyLight* Sky = World->SpawnActor<ASkyLight>(FVector::ZeroVector, FRotator::ZeroRotator, P);
		if (Sky && Sky->GetLightComponent())
		{
			Sky->GetRootComponent()->SetMobility(EComponentMobility::Movable);
			Sky->GetLightComponent()->SetRealTimeCapture(true);
			Sky->GetLightComponent()->SetCastShadows(false);
			Sky->GetLightComponent()->RecaptureSky();
		}
	}

	CubeA = World->SpawnActor<ASpikeMarkerCube>(FVector::ZeroVector, FRotator::ZeroRotator, P);
	CubeB = World->SpawnActor<ASpikeMarkerCube>(FVector::ZeroVector, FRotator::ZeroRotator, P);
	CubeA->SetActorHiddenInGame(true);
	CubeB->SetActorHiddenInGame(true);

	WarmupBox = World->SpawnActor<ASpikeBoxActor>(FVector::ZeroVector, FRotator::ZeroRotator, P);
	if (WarmupBox)
	{
		WarmupBox->InitBox(false, true, FVector::ZeroVector);
	}
}

void ASpikeDirector::SpawnKspScene()
{
	const double T0 = FPlatformTime::Seconds();
	UWorld* World = GetWorld();
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	FRandomStream Rng(1234);
	// Vessel stand-in: NumParts free rigid bodies, kept awake (spinning, no damping), on a 2.5 m grid ahead of the camera.
	const FVector3d Base = StepCam + StepRot.GetForwardVector() * 3000.0;
	for (int32 I = 0; I < NumParts; ++I)
	{
		const FVector3d U = Base + FVector3d((I % 10) * 250.0, ((I / 10) % 10) * 250.0, (I / 100) * 250.0);
		ASpikeBoxActor* B = World->SpawnActor<ASpikeBoxActor>(ToWorld(U), FRotator::ZeroRotator, P);
		B->InitBox(true, true, FVector(Rng.FRandRange(10, 30), Rng.FRandRange(10, 30), Rng.FRandRange(10, 30)));
		KspBoxes.Add(B);
		KspBoxesU.Add(U);
	}
	// PQS collider stand-in: NumColliders static (non-simulating, movable, hidden) box colliders on the ground at the site.
	const FVector3d GroundBase = SiteDir * (KspSpike::KerbinRadiusCm - 200.0);
	const int32 Side = FMath::CeilToInt32(FMath::Sqrt(static_cast<double>(NumColliders)));
	for (int32 I = 0; I < NumColliders; ++I)
	{
		const FVector3d U = GroundBase + East * ((I % Side) * 300.0) + North * ((I / Side) * 300.0);
		ASpikeBoxActor* B = World->SpawnActor<ASpikeBoxActor>(ToWorld(U), FRotator::ZeroRotator, P);
		B->InitBox(false, false, FVector::ZeroVector);
		KspBoxes.Add(B);
		KspBoxesU.Add(U);
	}
	UE_LOG(LogKspUe5, Log, TEXT("SPIKE_KSPSCENE parts=%d colliders=%d spawn_ms=%.1f"), NumParts, NumColliders, (FPlatformTime::Seconds() - T0) * 1000.0);
}

void ASpikeDirector::BuildProgram()
{
	auto F = [this](int32 Frames) { return bQuick ? FMath::Max(30, Frames / 6) : Frames; };
	FStep W;
	W.Label = TEXT("warmup");
	W.Variant = TEXT("-");
	W.Phase = EPhase::Warmup;
	W.Frames = 360;
	Steps.Add(W);

	struct FPlanEntry
	{
		EPhase P;
		int32 N;
	};
	const FPlanEntry Plan[] = {
		{ EPhase::OrbitImg, 300 }, { EPhase::OrbitTiming, 900 }, { EPhase::OrbitWarp, 300 }, { EPhase::Descent, 600 },
		{ EPhase::Horizon, 240 }, { EPhase::GroundStatic, 120 }, { EPhase::GroundStrafe, 300 }, { EPhase::MunFromOrbit, 120 },
		{ EPhase::MunNear, 240 } };

	bool bFirst = true;
	for (const FString& Name : VariantNames)
	{
		ESpikeRebase Mode = ESpikeRebase::None;
		double Thr = 0.0;
		if (Name.StartsWith(TEXT("UE")))
		{
			Mode = ESpikeRebase::UEOrigin;
		}
		else if (Name.StartsWith(TEXT("MW")))
		{
			Mode = ESpikeRebase::MoveWorld;
		}
		if (Mode != ESpikeRebase::None)
		{
			Thr = FCString::Atod(*Name.Mid(2).Replace(TEXT("k"), TEXT(""))) * 1.0e5; // km -> cm
		}
		for (const FPlanEntry& It : Plan)
		{
			FStep S;
			S.Variant = Name;
			S.Phase = It.P;
			S.Mode = Mode;
			S.ThresholdCm = Thr;
			S.Frames = F(It.N);
			S.bFullShots = bFirst;
			S.Label = FString::Printf(TEXT("%s/%s"), *Name, PhaseName(It.P));
			Steps.Add(S);
		}
		bFirst = false;
	}
	if (bStress)
	{
		const ESpikeRebase Modes[] = { ESpikeRebase::None, ESpikeRebase::UEOrigin, ESpikeRebase::MoveWorld };
		for (int32 Ksp = 0; Ksp < 2; ++Ksp)
		{
			for (ESpikeRebase Mode : Modes)
			{
				FStep S;
				S.Variant = FString::Printf(TEXT("Stress%s"), Ksp ? TEXT("Ksp") : TEXT("Bare"));
				S.Phase = EPhase::Stress;
				S.Mode = Mode;
				S.ThresholdCm = 0.0; // shift every tick
				S.Seconds = bQuick ? 3.0 : 10.0;
				S.bKspScene = Ksp != 0;
				S.Label = FString::Printf(TEXT("%s/%s-every-tick"), *S.Variant, ModeName(Mode));
				Steps.Add(S);
			}
		}
	}
	UE_LOG(LogKspUe5, Log, TEXT("SPIKE_PROGRAM steps=%d"), Steps.Num());
}

FVector3d ASpikeDirector::OrbitPos(double Theta) const
{
	return FVector3d(FMath::Cos(Theta), FMath::Sin(Theta), 0.0) * OrbitRadiusCm;
}

FQuat4d ASpikeDirector::OrbitAttitude(double Theta) const
{
	const FVector3d Up(FMath::Cos(Theta), FMath::Sin(Theta), 0.0);
	const FVector3d Pro(-FMath::Sin(Theta), FMath::Cos(Theta), 0.0);
	const double P = FMath::DegreesToRadians(25.0);
	return MakeRot(Pro * FMath::Cos(P) - Up * FMath::Sin(P), Pro * FMath::Sin(P) + Up * FMath::Cos(P));
}

void ASpikeDirector::ComputePose(const FStep& S, int32 Frame, FVector3d& OutCam, FQuat4d& OutRot, double& OutSimDt)
{
	const double R = KspSpike::KerbinRadiusCm;
	OutSimDt = SimDt60;
	const FVector3d Ground1m = SiteDir * (R + 100.0);
	const double P15 = FMath::DegreesToRadians(15.0);
	const FQuat4d GroundRot = MakeRot(East * FMath::Cos(P15) - SiteDir * FMath::Sin(P15), East * FMath::Sin(P15) + SiteDir * FMath::Cos(P15));
	const FVector3d MunNearStart = MunCentreU - MunDir * (KspSpike::MunRadiusCm + 2.0e6); // 20 km above the near side
	const FQuat4d MunNearRot = MakeRot(MunDir, FVector3d(0, 0, 1));

	switch (S.Phase)
	{
	case EPhase::Warmup:
	{
		const int32 View = (Frame / 60) % 5;
		if (View == 0)
		{
			OutCam = OrbitPos(OrbitTheta0);
			OutRot = OrbitAttitude(OrbitTheta0);
		}
		else if (View == 1)
		{
			OutCam = Ground1m;
			OutRot = GroundRot;
		}
		else if (View == 2)
		{
			OutCam = OrbitPos(OrbitTheta0);
			OutRot = MakeRot((MunCentreU - OutCam).GetSafeNormal(), FVector3d(0, 0, 1));
		}
		else if (View == 3)
		{
			OutCam = MunNearStart;
			OutRot = MunNearRot;
		}
		else
		{
			OutCam = SiteDir * (R + 1.0e6);
			OutRot = MakeRot(East * 0.5 - SiteDir * 0.866, East * 0.866 + SiteDir * 0.5);
		}
		break;
	}
	case EPhase::OrbitImg:
	case EPhase::OrbitTiming:
	case EPhase::OrbitWarp:
	case EPhase::Stress:
	{
		const double Warp = S.Phase == EPhase::OrbitWarp ? 50.0 : 1.0;
		OutSimDt = SimDt60 * Warp;
		if (Frame > 0)
		{
			OrbitTheta += OrbitSpeedCmS * OutSimDt / OrbitRadiusCm;
		}
		OutCam = OrbitPos(OrbitTheta);
		OutRot = S.Phase == EPhase::OrbitImg ? StepRot : OrbitAttitude(OrbitTheta);
		break;
	}
	case EPhase::Descent:
	{
		const double U = S.Frames > 1 ? double(Frame) / (S.Frames - 1) : 1.0;
		const double H = 1.0e7 * FMath::Pow(1.0e3 / 1.0e7, U); // 100 km -> 10 m (cm)
		const double Lg = FMath::Clamp((FMath::LogX(10.0, H / 100.0) - 1.0) / 4.0, 0.0, 1.0);
		const double Pitch = FMath::DegreesToRadians(2.0 + 58.0 * Lg);
		OutCam = SiteDir * (R + H);
		OutRot = MakeRot(East * FMath::Cos(Pitch) - SiteDir * FMath::Sin(Pitch), East * FMath::Sin(Pitch) + SiteDir * FMath::Cos(Pitch));
		break;
	}
	case EPhase::Horizon:
	{
		const double Yaw = FMath::DegreesToRadians(0.1 * Frame);
		const double Pitch = FMath::DegreesToRadians(2.0);
		const FVector3d Dir = East * FMath::Cos(Yaw) + North * FMath::Sin(Yaw);
		OutCam = SiteDir * (R + 1000.0);
		OutRot = MakeRot(Dir * FMath::Cos(Pitch) - SiteDir * FMath::Sin(Pitch), Dir * FMath::Sin(Pitch) + SiteDir * FMath::Cos(Pitch));
		break;
	}
	case EPhase::GroundStatic:
		OutCam = Ground1m;
		OutRot = GroundRot;
		break;
	case EPhase::GroundStrafe:
		OutCam = Ground1m + North * (0.1 * Frame); // 1 mm per frame
		OutRot = GroundRot;
		break;
	case EPhase::MunFromOrbit:
		OutCam = OrbitPos(OrbitTheta0);
		OutRot = MakeRot((MunCentreU - OutCam).GetSafeNormal(), FVector3d(0, 0, 1));
		break;
	case EPhase::MunNear:
		OutCam = MunNearStart + MunMotionDir * (5.0e4 * SimDt60 * Frame); // 500 m/s tangential
		OutRot = MunNearRot;
		break;
	}
}

double ASpikeDirector::ShiftOriginTo(const FVector3d& NewOriginU, ESpikeRebase Mode)
{
	const double T0 = FPlatformTime::Seconds();
	if (Mode == ESpikeRebase::UEOrigin)
	{
		const FIntVector NewO(FMath::RoundToInt32(NewOriginU.X), FMath::RoundToInt32(NewOriginU.Y), FMath::RoundToInt32(NewOriginU.Z));
		if (GetWorld()->SetNewWorldOrigin(NewO))
		{
			Origin = FVector3d(NewO.X, NewO.Y, NewO.Z);
		}
		else
		{
			UE_LOG(LogKspUe5, Warning, TEXT("SPIKE_WARN SetNewWorldOrigin refused"));
		}
	}
	else if (Mode == ESpikeRebase::MoveWorld)
	{
		const FVector3d Delta = NewOriginU - Origin;
		MoveWorldBy(FVector(-Delta));
		Origin = NewOriginU;
	}
	const double Ms = (FPlatformTime::Seconds() - T0) * 1000.0;
	// Validate: Kerbin's centre is U=0, so it must sit at W = -Origin.
	if (Kerbin)
	{
		ShiftValidErrMax = FMath::Max(ShiftValidErrMax, (FVector3d(Kerbin->GetActorLocation()) + Origin).Length());
	}
	return Ms;
}

void ASpikeDirector::MoveWorldBy(const FVector& Offset)
{
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* A = *It;
		if (A == this || A->IsA<AInfo>() || A->IsA<AController>() || A->IsA<APlayerCameraManager>())
		{
			continue;
		}
		USceneComponent* Root = A->GetRootComponent();
		if (!Root || Root->Mobility != EComponentMobility::Movable)
		{
			continue;
		}
		A->SetActorLocation(A->GetActorLocation() + Offset, false, nullptr, ETeleportType::TeleportPhysics);
	}
}

void ASpikeDirector::ResetOrigin(ESpikeRebase /*Mode*/)
{
	if (Origin.IsNearlyZero())
	{
		return;
	}
	// Undo with the same mechanism that created the offset.
	const bool bUE = GetWorld()->OriginLocation != FIntVector::ZeroValue;
	ShiftOriginTo(FVector3d::ZeroVector, bUE ? ESpikeRebase::UEOrigin : ESpikeRebase::MoveWorld);
}

void ASpikeDirector::SetupCubesForStep(const FStep& S, const FVector3d& Cam, const FQuat4d& Rot)
{
	bCubesVisible = false;
	bCubesCoMoving = false;
	const double R = KspSpike::KerbinRadiusCm;
	switch (S.Phase)
	{
	case EPhase::OrbitImg:
	case EPhase::MunNear:
		bCubesVisible = true;
		bCubesCoMoving = true;
		BodyCentreForB = S.Phase == EPhase::MunNear ? MunCentreU : FVector3d::ZeroVector;
		CubeAxes[0] = Rot.GetAxisX();
		CubeAxes[1] = Rot.GetAxisY();
		CubeAxes[2] = Rot.GetAxisZ();
		CubeCentreA = Cam + Rot.RotateVector(CubeOffsetA);
		CubeCentreB = Cam + Rot.RotateVector(CubeOffsetB);
		break;
	case EPhase::GroundStatic:
	case EPhase::GroundStrafe:
	case EPhase::Warmup:
		bCubesVisible = true;
		BodyCentreForB = FVector3d::ZeroVector;
		CubeAxes[0] = East;
		CubeAxes[1] = North;
		CubeAxes[2] = SiteDir;
		CubeCentreA = SiteDir * (R + 50.0) + East * 300.0 - North * 70.0;
		CubeCentreB = SiteDir * (R + 50.0) + East * 300.0 + North * 70.0;
		break;
	default:
		// Non-image phases: virtual co-moving "vessel" points feed the numeric proxy only.
		BodyCentreForB = FVector3d::ZeroVector;
		CubeAxes[0] = Rot.GetAxisX();
		CubeAxes[1] = Rot.GetAxisY();
		CubeAxes[2] = Rot.GetAxisZ();
		CubeCentreA = Cam + Rot.RotateVector(CubeOffsetA);
		CubeCentreB = Cam + Rot.RotateVector(CubeOffsetB);
		bCubesCoMoving = true;
		break;
	}
	CubeStartB = CubeCentreB;
	CubeA->SetActorHiddenInGame(!bCubesVisible);
	CubeB->SetActorHiddenInGame(!bCubesVisible);
	if (bCubesVisible)
	{
		// A: geometry around its own origin (small local coordinates). B: the same geometry stored relative to the
		// body centre (large local coordinates: the naive "one mesh in planet space" layout).
		CubeA->Build(FVector3d::ZeroVector, CubeAxes[0], CubeAxes[1], CubeAxes[2], 50.0, FColor(255, 0, 0), UnlitVertexColorMat);
		CubeB->Build(CubeCentreB - BodyCentreForB, CubeAxes[0], CubeAxes[1], CubeAxes[2], 50.0, FColor(255, 255, 0), UnlitVertexColorMat);
		CubeA->SetActorLocation(ToWorld(CubeCentreA));
		CubeB->SetActorLocation(ToWorld(BodyCentreForB));
	}
}

void ASpikeDirector::UpdateCoMovingCubes(const FVector3d& Cam)
{
	if (!bCubesCoMoving)
	{
		return;
	}
	CubeCentreA = Cam + StepRot.RotateVector(CubeOffsetA);
	CubeCentreB = Cam + StepRot.RotateVector(CubeOffsetB);
	if (bCubesVisible)
	{
		CubeA->SetActorLocation(ToWorld(CubeCentreA));
		CubeB->SetActorLocation(ToWorld(BodyCentreForB + (CubeCentreB - CubeStartB)));
	}
}

bool ASpikeDirector::ProjectPoint(const FVector3d& PU, const FVector3d& Cam, const FQuat4d& Rot, FVector2D& OutPx) const
{
	if (!GEngine || !GEngine->GameViewport || !GEngine->GameViewport->Viewport)
	{
		return false;
	}
	const FIntPoint Size = GEngine->GameViewport->Viewport->GetSizeXY();
	const FVector3d C = Rot.UnrotateVector(PU - Cam);
	if (C.X < 1.0)
	{
		return false;
	}
	const double F = (Size.X * 0.5) / FMath::Tan(FMath::DegreesToRadians(FovDeg * 0.5));
	OutPx = FVector2D(Size.X * 0.5 + F * C.Y / C.X, Size.Y * 0.5 - F * C.Z / C.X);
	return true;
}

bool ASpikeDirector::ProjectCubeRoi(const FVector3d& CentreU, const FVector3d Axes[3], double H, const FVector3d& Cam, const FQuat4d& Rot, FIntRect& Out) const
{
	FBox2D Box(ForceInit);
	for (int32 I = 0; I < 8; ++I)
	{
		const FVector3d Corner = CentreU + Axes[0] * ((I & 1) ? H : -H) + Axes[1] * ((I & 2) ? H : -H) + Axes[2] * ((I & 4) ? H : -H);
		FVector2D Px;
		if (!ProjectPoint(Corner, Cam, Rot, Px))
		{
			return false;
		}
		Box += Px;
	}
	const FIntPoint Size = GEngine->GameViewport->Viewport->GetSizeXY();
	Out = FIntRect(FMath::Clamp(FMath::FloorToInt32(Box.Min.X) - 12, 0, Size.X), FMath::Clamp(FMath::FloorToInt32(Box.Min.Y) - 12, 0, Size.Y),
		FMath::Clamp(FMath::CeilToInt32(Box.Max.X) + 12, 0, Size.X), FMath::Clamp(FMath::CeilToInt32(Box.Max.Y) + 12, 0, Size.Y));
	return Out.Area() > 0;
}

void ASpikeDirector::RequestCapture(int32 RowIndex, const FVector3d& Cam, const FQuat4d& Rot, bool bMeasure, const FString& SaveTo)
{
	FPendingCapture P;
	P.Row = RowIndex;
	P.SaveTo = SaveTo;
	if (bMeasure)
	{
		if (bCubesVisible)
		{
			P.bRoi[0] = ProjectCubeRoi(CubeCentreA, CubeAxes, 50.0, Cam, Rot, P.Roi[0]);
			P.bRoi[1] = ProjectCubeRoi(CubeCentreB, CubeAxes, 50.0, Cam, Rot, P.Roi[1]);
		}
		FVector2D MunPx;
		const double Dist = (MunCentreU - Cam).Length();
		if (Dist > KspSpike::MunRadiusCm * 2.0 && ProjectPoint(MunCentreU, Cam, Rot, MunPx))
		{
			const FIntPoint Size = GEngine->GameViewport->Viewport->GetSizeXY();
			const double F = (Size.X * 0.5) / FMath::Tan(FMath::DegreesToRadians(FovDeg * 0.5));
			const double Rpx = F * KspSpike::MunRadiusCm / FMath::Sqrt(FMath::Max(1.0, Dist * Dist - KspSpike::MunRadiusCm * KspSpike::MunRadiusCm)) * 1.3 + 4.0;
			P.Roi[2] = FIntRect(FMath::Clamp(FMath::FloorToInt32(MunPx.X - Rpx), 0, Size.X), FMath::Clamp(FMath::FloorToInt32(MunPx.Y - Rpx), 0, Size.Y),
				FMath::Clamp(FMath::CeilToInt32(MunPx.X + Rpx), 0, Size.X), FMath::Clamp(FMath::CeilToInt32(MunPx.Y + Rpx), 0, Size.Y));
			P.bRoi[2] = P.Roi[2].Area() > 0;
		}
	}
	Pending.Add(P);
	if (Pending.Num() > 4)
	{
		Pending.RemoveAt(0); // never let a missed capture desync rows
	}
	FScreenshotRequest::RequestScreenshot(false);
}

void ASpikeDirector::OnScreenshot(int32 Width, int32 Height, const TArray<FColor>& Colors)
{
	if (Pending.Num() == 0)
	{
		return;
	}
	const FPendingCapture P = Pending[0];
	Pending.RemoveAt(0);
	++CapturesReceived;
	if (Rows.IsValidIndex(P.Row))
	{
		FRow& Row = Rows[P.Row];
		for (int32 T = 0; T < NumTargets; ++T)
		{
			if (!P.bRoi[T])
			{
				continue;
			}
			Row.bImg = true;
			double Sx = 0.0, Sy = 0.0;
			int32 N = 0;
			const FIntRect& Roi = P.Roi[T];
			for (int32 Y = Roi.Min.Y; Y < FMath::Min(Roi.Max.Y, Height); ++Y)
			{
				for (int32 X = Roi.Min.X; X < FMath::Min(Roi.Max.X, Width); ++X)
				{
					const FColor& C = Colors[Y * Width + X];
					const bool bHit = T == 0 ? IsRed(C) : (T == 1 ? IsYellow(C) : IsMagenta(C));
					if (bHit)
					{
						Sx += X + 0.5;
						Sy += Y + 0.5;
						++N;
					}
				}
			}
			Row.Cn[T] = N;
			Row.Cx[T] = N ? Sx / N : 0.0;
			Row.Cy[T] = N ? Sy / N : 0.0;
		}
	}
	if (!P.SaveTo.IsEmpty())
	{
		TArray<FColor> Copy = Colors;
		for (FColor& C : Copy)
		{
			C.A = 255;
		}
		TArray64<uint8> Png;
		FImageUtils::PNGCompressImageArray(Width, Height, TArrayView64<const FColor>(Copy.GetData(), Copy.Num()), Png);
		FFileHelper::SaveArrayToFile(Png, *P.SaveTo);
		UE_LOG(LogKspUe5, Log, TEXT("SPIKE_SHOT %s"), *P.SaveTo);
	}
}

FString ASpikeDirector::ShotPath(const FString& Name) const
{
	return FPaths::Combine(OutDir, TEXT("shots"), FString::Printf(TEXT("%s_%s.png"), *Tag, *Name));
}

void ASpikeDirector::AppendText(const FString& File, const FString& Text) const
{
	FFileHelper::SaveStringToFile(Text, *FPaths::Combine(OutDir, TEXT("data"), File), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
		&IFileManager::Get(), FILEWRITE_Append);
}

void ASpikeDirector::BeginStep()
{
	FStep& S = Steps[StepIndex];
	StepFrame = 0;
	StepStartTime = FPlatformTime::Seconds();
	StepFirstRow = Rows.Num();
	ShiftValidErrMax = 0.0;
	WarmupQuietFrames = 0;
	for (KspSpike::FJitterTracker& T : Trackers)
	{
		T.Reset();
	}

	const bool bVariantStart = StepIndex == 0 || Steps[StepIndex - 1].Variant != S.Variant;
	if (bVariantStart)
	{
		ResetOrigin(S.Mode);
		OrbitTheta = OrbitTheta0;
	}
	// UE logs two lines per SetNewWorldOrigin; silence them for the every-tick stress so I/O is not measured.
	GEngine->Exec(GetWorld(), S.Phase == EPhase::Stress ? TEXT("log LogLevel Warning") : TEXT("log LogLevel Log"));

	double Dt = 0.0;
	const double SavedTheta = OrbitTheta;
	ComputePose(S, 0, StepCam, StepRot, Dt); // frame 0 does not advance the orbit
	OrbitTheta = SavedTheta;
	if (S.Phase == EPhase::OrbitImg)
	{
		StepRot = OrbitAttitude(OrbitTheta); // inertial attitude for the co-moving cube test
	}
	if (S.bKspScene && KspBoxes.Num() == 0)
	{
		SpawnKspScene();
	}
	if (WarmupBox)
	{
		WarmupBox->SetActorHiddenInGame(S.Phase != EPhase::Warmup);
		WarmupBox->SetActorLocation(ToWorld(StepCam + StepRot.GetForwardVector() * 600.0));
	}
	SetupCubesForStep(S, StepCam, StepRot);

	// Static ground reference vertex and a nearby "patch origin" (leaf patches are ~57 m across).
	const bool bMun = S.Phase == EPhase::MunFromOrbit || S.Phase == EPhase::MunNear;
	GroundBody = bMun ? MunCentreU : FVector3d::ZeroVector;
	const double BodyR = bMun ? KspSpike::MunRadiusCm : KspSpike::KerbinRadiusCm;
	if (S.Phase == EPhase::Descent || S.Phase == EPhase::Horizon || S.Phase == EPhase::GroundStatic || S.Phase == EPhase::GroundStrafe)
	{
		GroundRef = (SiteDir * KspSpike::KerbinRadiusCm + East * 2000.0).GetSafeNormal() * KspSpike::KerbinRadiusCm;
	}
	else
	{
		GroundRef = GroundBody + (StepCam - GroundBody).GetSafeNormal() * BodyR;
	}
	const FVector3d Nrm = (GroundRef - GroundBody).GetSafeNormal();
	FVector3d T1 = FVector3d::CrossProduct(Nrm, FVector3d(0, 0, 1));
	if (T1.SizeSquared() < 1e-6)
	{
		T1 = FVector3d(0, 1, 0);
	}
	T1.Normalize();
	const FVector3d T2 = FVector3d::CrossProduct(Nrm, T1);
	GroundPatchOrigin = GroundRef + T1 * 1500.0 + T2 * 900.0;

	DescentShotAltsKm = { 100.0, 30.0, 10.0, 1.0, 0.1, 0.0101 };
	PrevAltKm = 1.0e9;
	UE_LOG(LogKspUe5, Log, TEXT("SPIKE_STEP_BEGIN %d %s mode=%s thr_km=%.1f frames=%d secs=%.1f"), StepIndex, *S.Label, ModeName(S.Mode),
		S.ThresholdCm / 1.0e5, S.Frames, S.Seconds);
}

void ASpikeDirector::TickStep()
{
	FStep& S = Steps[StepIndex];
	FRow Row;
	Row.Step = StepIndex;
	Row.Frame = StepFrame;

	FVector3d Cam;
	FQuat4d Rot;
	double SimDt = 0.0;
	ComputePose(S, StepFrame, Cam, Rot, SimDt);
	if (S.Phase == EPhase::OrbitImg)
	{
		Rot = StepRot;
	}
	Row.SimT = StepFrame * SimDt;

	// ---- origin rebasing ----
	if (S.Mode != ESpikeRebase::None && S.Phase != EPhase::Warmup)
	{
		const bool bForced = S.Phase == EPhase::GroundStatic && StepFrame == FMath::Min(60, S.Frames / 2);
		if (bForced)
		{
			Row.ShiftMs = ShiftOriginTo(Cam + East * 500.0, S.Mode); // a real shift while everything is static
			Row.bShift = true;
		}
		else if ((Cam - Origin).Length() > S.ThresholdCm)
		{
			Row.ShiftMs = ShiftOriginTo(Cam, S.Mode);
			Row.bShift = true;
		}
	}

	CamPawn->SetActorLocationAndRotation(ToWorld(Cam), FQuat(Rot), false, nullptr, ETeleportType::TeleportPhysics);
	UpdateCoMovingCubes(Cam);

	// ---- planet LOD ----
	int32 Created = 0, Destroyed = 0, C2 = 0, D2 = 0;
	Row.LodMs = Kerbin->UpdateLod(ToWorld(Cam), false, Created, Destroyed) + Mun->UpdateLod(ToWorld(Cam), false, C2, D2);
	Row.Patches = Kerbin->NumPatches() + Mun->NumPatches();
	const double BodyR = GroundBody.IsZero() ? KspSpike::KerbinRadiusCm : KspSpike::MunRadiusCm;
	Row.AltKm = ((Cam - GroundBody).Length() - BodyR) / 1.0e5;
	Row.CamFromOriginKm = (Cam - Origin).Length() / 1.0e5;
	Row.OriginKm = Origin / 1.0e5;

	// ---- numeric jitter proxy ----
	{
		const FVector3d Corner = (-CubeAxes[0] - CubeAxes[1] + CubeAxes[2]) * 50.0;
		const FVector3d VA = CubeCentreA + Corner;
		const FVector3d VB = CubeCentreB + Corner;
		const FVector3d PA = CubeCentreA;
		const FVector3d PB = BodyCentreForB + (CubeCentreB - CubeStartB);
		const FVector3d EA = KspSpike::CamSpaceExact(VA, Cam, Rot);
		const FVector3d EB = KspSpike::CamSpaceExact(VB, Cam, Rot);
		const FVector3d EG = KspSpike::CamSpaceExact(GroundRef, Cam, Rot);
		Row.Jit[0] = Trackers[0].Add(KspSpike::CamSpaceLwc(VA, PA, Cam, Rot), EA);
		Row.Jit[1] = Trackers[1].Add(KspSpike::CamSpaceLwc(VB, PB, Cam, Rot), EB);
		Row.Jit[2] = Trackers[2].Add(KspSpike::CamSpaceLwc(GroundRef, GroundPatchOrigin, Cam, Rot), EG);
		Row.Jit[3] = Trackers[3].Add(KspSpike::CamSpaceLwc(GroundRef, GroundBody, Cam, Rot), EG);
		Row.Jit[4] = Trackers[4].Add(KspSpike::CamSpaceF32World(VA, Cam, Origin, Rot), EA);
		Row.Jit[5] = Trackers[5].Add(KspSpike::CamSpaceF32World(VA, Cam, FVector3d::ZeroVector, Rot), EA);
	}

	// ---- captures ----
	const int32 RowIndex = Rows.Num();
	Rows.Add(Row);
	const bool bMeasure = S.Phase == EPhase::OrbitImg || S.Phase == EPhase::GroundStatic || S.Phase == EPhase::GroundStrafe ||
		S.Phase == EPhase::MunFromOrbit || S.Phase == EPhase::MunNear;
	FString Save;
	const FString Base = FString::Printf(TEXT("%s_%s"), *S.Variant, PhaseName(S.Phase));
	const int32 ShotFrame = FMath::Min(10, S.Frames - 1);
	if (S.Phase == EPhase::Descent)
	{
		if (S.bFullShots)
		{
			for (int32 I = 0; I < DescentShotAltsKm.Num(); ++I)
			{
				if (PrevAltKm > DescentShotAltsKm[I] && Row.AltKm <= DescentShotAltsKm[I] * 1.0001)
				{
					Save = ShotPath(FString::Printf(TEXT("%s_alt%skm"), *Base, *FString::SanitizeFloat(DescentShotAltsKm[I] < 0.02 ? 0.01 : DescentShotAltsKm[I])));
				}
			}
		}
		PrevAltKm = Row.AltKm;
	}
	else if (S.Phase == EPhase::Warmup)
	{
		if (StepFrame % 60 == 30 && StepFrame < 300)
		{
			Save = ShotPath(FString::Printf(TEXT("warmup_view%d"), StepFrame / 60));
		}
	}
	else if (S.Phase != EPhase::Stress && StepFrame == ShotFrame && (S.bFullShots || S.Phase == EPhase::GroundStatic || S.Phase == EPhase::MunNear))
	{
		Save = ShotPath(Base);
	}
	else if (S.Phase == EPhase::OrbitWarp && S.bFullShots && StepFrame == S.Frames / 2)
	{
		Save = ShotPath(Base + TEXT("_mid"));
	}
	if (bMeasure || !Save.IsEmpty())
	{
		RequestCapture(RowIndex, Cam, Rot, bMeasure, Save);
	}
}

void ASpikeDirector::EndStep()
{
	const FStep& S = Steps[StepIndex];
	const double RealS = FPlatformTime::Seconds() - StepStartTime;
	TArray<double> Frame, Game, Render, Rhi, Gpu, ShiftMs;
	double LodTotal = 0.0, LodMax = 0.0, CamOriginMax = 0.0;
	int32 PatchesMax = 0, Shifts = 0;
	for (int32 I = StepFirstRow; I < Rows.Num(); ++I)
	{
		const FRow& R = Rows[I];
		if (R.RealDtMs > 0.0)
		{
			Frame.Add(R.RealDtMs);
			Game.Add(R.GameMs);
			Render.Add(R.RenderMs);
			Rhi.Add(R.RhiMs);
			Gpu.Add(R.GpuMs);
		}
		if (R.bShift)
		{
			++Shifts;
			ShiftMs.Add(R.ShiftMs);
		}
		LodTotal += R.LodMs;
		LodMax = FMath::Max(LodMax, R.LodMs);
		PatchesMax = FMath::Max(PatchesMax, R.Patches);
		CamOriginMax = FMath::Max(CamOriginMax, R.CamFromOriginKm);
	}
	double ShiftMax = 0.0;
	for (double X : ShiftMs)
	{
		ShiftMax = FMath::Max(ShiftMax, X);
	}

	// ---- image metrics ----
	const bool bStrafe = S.Phase == EPhase::GroundStrafe;
	double Fpx = 640.0;
	if (GEngine && GEngine->GameViewport && GEngine->GameViewport->Viewport)
	{
		Fpx = GEngine->GameViewport->Viewport->GetSizeXY().X * 0.5 / FMath::Tan(FMath::DegreesToRadians(FovDeg * 0.5));
	}
	const int32 PopFrame = FMath::Min(60, S.Frames / 2);
	FString ImgCsv, ImgLog;
	static const TCHAR* TargetNames[NumTargets] = { TEXT("A"), TEXT("B"), TEXT("Mun") };
	for (int32 T = 0; T < NumTargets; ++T)
	{
		TArray<double> N, X, Y;
		double CountSum = 0.0;
		int32 CountMin = MAX_int32, CountMax = 0;
		for (int32 I = StepFirstRow; I < Rows.Num(); ++I)
		{
			const FRow& R = Rows[I];
			if (R.bImg && R.Cn[T] >= MinPixels)
			{
				N.Add(R.Frame);
				X.Add(R.Cx[T]);
				Y.Add(R.Cy[T]);
				CountSum += R.Cn[T];
				CountMin = FMath::Min(CountMin, R.Cn[T]);
				CountMax = FMath::Max(CountMax, R.Cn[T]);
			}
		}
		double MaxDev = 0.0, Rms = 0.0, F2F = 0.0, Pop = 0.0;
		if (N.Num() >= 4)
		{
			TArray<double> Rx, Ry;
			if (bStrafe)
			{
				Rx = QuadResiduals(N, X);
				Ry = QuadResiduals(N, Y);
			}
			else
			{
				const double Mx = Mean(X), My = Mean(Y);
				for (int32 I = 0; I < N.Num(); ++I)
				{
					Rx.Add(X[I] - Mx);
					Ry.Add(Y[I] - My);
				}
			}
			for (int32 I = 0; I < Rx.Num(); ++I)
			{
				const double D = FMath::Sqrt(Rx[I] * Rx[I] + Ry[I] * Ry[I]);
				MaxDev = FMath::Max(MaxDev, D);
				Rms += D * D;
				if (I > 0 && N[I] == N[I - 1] + 1)
				{
					const double StepPx = FMath::Sqrt(FMath::Square(Rx[I] - Rx[I - 1]) + FMath::Square(Ry[I] - Ry[I - 1]));
					F2F = FMath::Max(F2F, StepPx);
					if (S.Phase == EPhase::GroundStatic && FMath::RoundToInt32(N[I]) == PopFrame)
					{
						Pop = StepPx;
					}
				}
			}
			Rms = FMath::Sqrt(Rms / FMath::Max(1, Rx.Num()));
		}
		const double Dist = T == 0 ? (CubeCentreA - StepCam).Length() : (T == 1 ? (CubeCentreB - StepCam).Length() : (MunCentreU - StepCam).Length());
		const double CmPerPx = Dist / Fpx;
		const double CntAvg = N.Num() ? CountSum / N.Num() : 0.0;
		ImgCsv += FString::Printf(TEXT(",%d,%.0f,%d,%d,%.4f,%.4f,%.4f,%.4f,%.5f"), N.Num(), CntAvg, N.Num() ? CountMin : 0, CountMax, MaxDev, Rms, F2F, Pop, CmPerPx);
		if (N.Num() > 0)
		{
			ImgLog += FString::Printf(TEXT(" img%s[n=%d px=%.0f maxdev=%.3fpx(%.3fcm) f2f=%.3fpx pop=%.3fpx]"), TargetNames[T], N.Num(), CntAvg, MaxDev,
				MaxDev * CmPerPx, F2F, Pop);
		}
	}

	const FPlatformMemoryStats Mem = FPlatformMemory::GetStats();
	FTextureMemoryStats Tex;
	RHIGetTextureMemoryStats(Tex);
	const double MemMb = Mem.UsedPhysical / (1024.0 * 1024.0);
	const double PeakMb = Mem.PeakUsedPhysical / (1024.0 * 1024.0);
	const double TexMb = double(Tex.StreamingMemorySize + Tex.NonStreamingMemorySize) / (1024.0 * 1024.0);

	double PhysDrift = 0.0;
	if (S.bKspScene)
	{
		for (int32 I = 0; I < KspBoxes.Num(); ++I)
		{
			if (KspBoxes[I])
			{
				PhysDrift = FMath::Max(PhysDrift, (FVector3d(KspBoxes[I]->GetActorLocation()) + Origin - KspBoxesU[I]).Length());
			}
		}
	}

	FString JitCsv, JitLog;
	static const TCHAR* RefNames[NumRefs] = { TEXT("A_lwc"), TEXT("B_lwc"), TEXT("G_patch"), TEXT("G_planet"), TEXT("A_f32_rebased"), TEXT("A_f32_norebase") };
	for (int32 R = 0; R < NumRefs; ++R)
	{
		JitCsv += FString::Printf(TEXT(",%.5f,%.5f,%.5f"), Trackers[R].MaxJitter, Trackers[R].Rms(), Trackers[R].MaxAbsErr);
		JitLog += FString::Printf(TEXT(" %s=%.4f"), RefNames[R], Trackers[R].MaxJitter);
	}

	if (!bHeaderWritten)
	{
		FString H = TEXT("tag,step,label,variant,mode,threshold_km,phase,frames,real_s,frame_ms_avg,frame_ms_p50,frame_ms_p95,frame_ms_max,game_ms_avg,game_ms_p95,render_ms_avg,render_ms_p95,rhi_ms_avg,gpu_ms_avg,gpu_ms_p95,shifts,shift_ms_avg,shift_ms_p50,shift_ms_max,shift_valid_err_cm,lod_ms_total,lod_ms_max,patches_max,cam_from_origin_km_max,mem_used_mb,mem_peak_mb,tex_mb,phys_drift_cm");
		for (const TCHAR* RN : RefNames)
		{
			H += FString::Printf(TEXT(",%s_jit_max_cm,%s_jit_rms_cm,%s_err_max_cm"), RN, RN, RN);
		}
		static const TCHAR* ImgNames[NumTargets] = { TEXT("imgA"), TEXT("imgB"), TEXT("imgMun") };
		for (const TCHAR* TN : ImgNames)
		{
			H += FString::Printf(TEXT(",%s_n,%s_px_avg,%s_px_min,%s_px_max,%s_maxdev_px,%s_rms_px,%s_f2f_px,%s_pop_px,%s_cm_per_px"), TN, TN, TN, TN, TN, TN, TN, TN, TN);
		}
		AppendText(Tag + TEXT("_summary.csv"), H + TEXT("\n"));
		AppendText(Tag + TEXT("_frames.csv"), TEXT("step,label,frame,sim_t,real_dt_ms,game_ms,render_ms,rhi_ms,gpu_ms,shift,shift_ms,lod_ms,patches,alt_km,cam_from_origin_km,origin_x_km,origin_y_km,origin_z_km,j_A_lwc,j_B_lwc,j_G_patch,j_G_planet,j_A_f32_rebased,j_A_f32_norebase,img,ax,ay,an,bx,by,bn,mx,my,mn\n"));
		bHeaderWritten = true;
	}
	FString Line = FString::Printf(TEXT("%s,%d,%s,%s,%s,%.1f,%s,%d,%.2f,"), *Tag, StepIndex, *S.Label, *S.Variant, ModeName(S.Mode), S.ThresholdCm / 1.0e5,
		PhaseName(S.Phase), Rows.Num() - StepFirstRow, RealS);
	Line += FString::Printf(TEXT("%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,"), Mean(Frame), Percentile(Frame, 0.5), Percentile(Frame, 0.95),
		Percentile(Frame, 1.0), Mean(Game), Percentile(Game, 0.95), Mean(Render), Percentile(Render, 0.95), Mean(Rhi), Mean(Gpu), Percentile(Gpu, 0.95));
	Line += FString::Printf(TEXT("%d,%.4f,%.4f,%.4f,%.4f,%.1f,%.2f,%d,%.1f,%.0f,%.0f,%.0f,%.4f"), Shifts, Mean(ShiftMs), Percentile(ShiftMs, 0.5), ShiftMax,
		ShiftValidErrMax, LodTotal, LodMax, PatchesMax, CamOriginMax, MemMb, PeakMb, TexMb, PhysDrift);
	Line += JitCsv + ImgCsv + TEXT("\n");
	AppendText(Tag + TEXT("_summary.csv"), Line);

	FString FramesCsv;
	for (int32 I = StepFirstRow; I < Rows.Num(); ++I)
	{
		const FRow& R = Rows[I];
		FramesCsv += FString::Printf(TEXT("%d,%s,%d,%.4f,%.3f,%.3f,%.3f,%.3f,%.3f,%d,%.4f,%.3f,%d,%.5f,%.3f,%.3f,%.3f,%.3f,"), R.Step, *S.Label, R.Frame, R.SimT,
			R.RealDtMs, R.GameMs, R.RenderMs, R.RhiMs, R.GpuMs, R.bShift ? 1 : 0, R.ShiftMs, R.LodMs, R.Patches, R.AltKm, R.CamFromOriginKm, R.OriginKm.X,
			R.OriginKm.Y, R.OriginKm.Z);
		FramesCsv += FString::Printf(TEXT("%.5f,%.5f,%.5f,%.5f,%.5f,%.5f,%d,%.3f,%.3f,%d,%.3f,%.3f,%d,%.3f,%.3f,%d\n"), R.Jit[0], R.Jit[1], R.Jit[2], R.Jit[3],
			R.Jit[4], R.Jit[5], R.bImg ? 1 : 0, R.Cx[0], R.Cy[0], R.Cn[0], R.Cx[1], R.Cy[1], R.Cn[1], R.Cx[2], R.Cy[2], R.Cn[2]);
	}
	AppendText(Tag + TEXT("_frames.csv"), FramesCsv);

	UE_LOG(LogKspUe5, Log, TEXT("SPIKE_STEP_DONE %d %s frames=%d real_s=%.1f frame_ms avg=%.2f p95=%.2f max=%.2f game=%.2f render=%.2f rhi=%.2f gpu=%.2f shifts=%d shift_ms avg=%.3f max=%.3f valid_err_cm=%.4f lod_ms=%.1f patches=%d mem_mb=%.0f phys_drift_cm=%.3f jit_cm[%s ]%s"),
		StepIndex, *S.Label, Rows.Num() - StepFirstRow, RealS, Mean(Frame), Percentile(Frame, 0.95), Percentile(Frame, 1.0), Mean(Game), Mean(Render),
		Mean(Rhi), Mean(Gpu), Shifts, Mean(ShiftMs), ShiftMax, ShiftValidErrMax, LodTotal, PatchesMax, MemMb, PhysDrift, *JitLog, *ImgLog);
}

void ASpikeDirector::Finish()
{
	bFinished = true;
	const double Total = FPlatformTime::Seconds() - ProgramStartTime;
	UE_LOG(LogKspUe5, Log, TEXT("SPIKE_DONE tag=%s steps=%d rows=%d captures=%d program_s=%.1f since_process_start_s=%.1f"), *Tag, Steps.Num(), Rows.Num(),
		CapturesReceived, Total, FPlatformTime::Seconds() - GStartTime);
	GLog->Flush();
	FPlatformMisc::RequestExit(false, TEXT("SpikeDirector"));
}

void ASpikeDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bFinished)
	{
		return;
	}
	if (!CamPawn)
	{
		CamPawn = Cast<ASpikeCameraPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
		if (!CamPawn)
		{
			return;
		}
	}

	// Stats published during this tick describe the previous frame: attribute them to the previous row.
	if (Rows.Num() > 0)
	{
		FRow& Prev = Rows.Last();
		Prev.RealDtMs = FApp::GetDeltaTime() * 1000.0;
		Prev.GameMs = FPlatformTime::ToMilliseconds(GGameThreadTime);
		Prev.RenderMs = FPlatformTime::ToMilliseconds(GRenderThreadTime);
		Prev.RhiMs = FPlatformTime::ToMilliseconds(GRHIThreadTime);
		Prev.GpuMs = FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles(0));
	}

	if (StepIndex >= Steps.Num())
	{
		Finish();
		return;
	}
	if (StepFrame < 0)
	{
		BeginStep();
	}
	TickStep();
	++StepFrame;

	const FStep& S = Steps[StepIndex];
	bool bDone = false;
	if (S.Phase == EPhase::Warmup)
	{
		int32 Remaining = 0;
#if WITH_EDITOR
		Remaining = GShaderCompilingManager ? GShaderCompilingManager->GetNumRemainingJobs() : 0;
#endif
		WarmupQuietFrames = Remaining == 0 ? WarmupQuietFrames + 1 : 0;
		if (StepFrame % 120 == 0)
		{
			UE_LOG(LogKspUe5, Log, TEXT("SPIKE_WARMUP frame=%d shader_jobs_remaining=%d t=%.1fs since_process_start=%.1fs"), StepFrame, Remaining,
				FPlatformTime::Seconds() - StepStartTime, FPlatformTime::Seconds() - GStartTime);
		}
		const bool bTimeout = FPlatformTime::Seconds() - StepStartTime > 45.0 * 60.0;
		bDone = (StepFrame >= S.Frames && WarmupQuietFrames >= 120) || bTimeout;
		if (bDone)
		{
			UE_LOG(LogKspUe5, Log, TEXT("SPIKE_WARMUP_DONE frames=%d warmup_s=%.1f since_process_start_s=%.1f timeout=%d"), StepFrame,
				FPlatformTime::Seconds() - StepStartTime, FPlatformTime::Seconds() - GStartTime, bTimeout ? 1 : 0);
		}
	}
	else if (S.Phase == EPhase::Stress)
	{
		bDone = FPlatformTime::Seconds() - StepStartTime >= S.Seconds;
	}
	else
	{
		bDone = StepFrame >= S.Frames;
	}
	if (bDone)
	{
		EndStep();
		++StepIndex;
		StepFrame = -1;
	}
}
