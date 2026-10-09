#include "Spike/SpikeActors.h"

#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "ProceduralMeshComponent.h"
#include "Spike/SpikeDirector.h"
#include "Spike/SpikeMath.h"
#include "UObject/ConstructorHelpers.h"

ASpikeCameraPawn::ASpikeCameraPawn()
{
	PrimaryActorTick.bCanEverTick = false;
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetMobility(EComponentMobility::Movable);
	RootComponent = Camera;
	Camera->FieldOfView = 90.0f;
	Camera->bConstrainAspectRatio = false;
	Camera->PostProcessBlendWeight = 1.0f;

	FPostProcessSettings& PP = Camera->PostProcessSettings;
	PP.bOverride_AutoExposureMethod = true;
	PP.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
	PP.bOverride_AutoExposureBias = true;
	PP.AutoExposureBias = -1.0f; // fixed exposure; -1 EV keeps the lit ground from washing out
	PP.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	PP.AutoExposureApplyPhysicalCameraExposure = 0;
	PP.bOverride_BloomIntensity = true;
	PP.BloomIntensity = 0.0f;
	PP.bOverride_MotionBlurAmount = true;
	PP.MotionBlurAmount = 0.0f;
	PP.bOverride_VignetteIntensity = true;
	PP.VignetteIntensity = 0.0f;
	PP.bOverride_LensFlareIntensity = true;
	PP.LensFlareIntensity = 0.0f;
}

ASpikeMarkerCube::ASpikeMarkerCube()
{
	PrimaryActorTick.bCanEverTick = false;
	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Mesh"));
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCanEverAffectNavigation(false);
	Mesh->SetCastShadow(false);
	RootComponent = Mesh;
}

void ASpikeMarkerCube::Build(const FVector3d& InLocalCentre, const FVector3d& Ax, const FVector3d& Ay, const FVector3d& Az, double InHalfSize,
	const FColor& Color, UMaterialInterface* Material)
{
	LocalCentre = InLocalCentre;
	HalfSize = InHalfSize;
	Axes[0] = Ax;
	Axes[1] = Ay;
	Axes[2] = Az;
	TArray<FVector> V;
	TArray<int32> T;
	TArray<FVector> N;
	TArray<FColor> C;
	KspSpike::BuildCube(LocalCentre, Ax, Ay, Az, HalfSize, Color, V, T, N, C);
	Mesh->ClearAllMeshSections();
	Mesh->CreateMeshSection(0, V, T, N, TArray<FVector2D>(), C, TArray<FProcMeshTangent>(), false);
	if (Material)
	{
		Mesh->SetMaterial(0, Material);
	}
}

ASpikeBoxActor::ASpikeBoxActor()
{
	PrimaryActorTick.bCanEverTick = false;
	Box = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Box"));
	Box->SetMobility(EComponentMobility::Movable);
	RootComponent = Box;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Box->SetStaticMesh(CubeMesh.Object);
	}
	Box->SetCastShadow(false);
	Box->SetCanEverAffectNavigation(false);
}

void ASpikeBoxActor::InitBox(bool bSimulate, bool bVisible, const FVector& AngularVelocityDeg)
{
	Box->SetCollisionProfileName(bSimulate ? TEXT("PhysicsActor") : TEXT("BlockAll"));
	Box->SetHiddenInGame(!bVisible);
	if (bSimulate)
	{
		Box->SetEnableGravity(false);
		Box->SetLinearDamping(0.0f);
		Box->SetAngularDamping(0.0f);
		Box->SetSimulatePhysics(true);
		Box->SetPhysicsAngularVelocityInDegrees(AngularVelocityDeg);
	}
}

ASpikeGameMode::ASpikeGameMode()
{
	DefaultPawnClass = ASpikeCameraPawn::StaticClass();
}

void ASpikeGameMode::StartPlay()
{
	Super::StartPlay();
	UWorld* World = GetWorld();
	if (World && !TActorIterator<ASpikeDirector>(World))
	{
		World->SpawnActor<ASpikeDirector>();
	}
}
