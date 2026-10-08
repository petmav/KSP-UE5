#include "Spike/SpikePlanet.h"

#include "Components/SkyAtmosphereComponent.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"

ASpikePlanet::ASpikePlanet()
{
	PrimaryActorTick.bCanEverTick = false;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	Atmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("Atmosphere"));
	Atmosphere->SetupAttachment(Root);
	Atmosphere->TransformMode = ESkyAtmosphereTransformMode::PlanetCenterAtComponentTransform;
	Atmosphere->SetAutoActivate(false);
	Atmosphere->SetVisibility(false);
}

void ASpikePlanet::Configure(const FConfig& InConfig)
{
	Config = InConfig;
	if (Config.bAtmosphere && Atmosphere)
	{
		Atmosphere->TransformMode = ESkyAtmosphereTransformMode::PlanetCenterAtComponentTransform;
		Atmosphere->SetBottomRadius(static_cast<float>(Config.RadiusCm / 1.0e5)); // km
		Atmosphere->SetAtmosphereHeight(static_cast<float>(Config.AtmosphereHeightKm));
		// Kerbin-like: thinner, shorter scale-height atmosphere than Earth's 8 km / 1.2 km defaults.
		Atmosphere->SetRayleighExponentialDistribution(5.6f);
		Atmosphere->SetMieExponentialDistribution(0.84f);
		Atmosphere->SetVisibility(true);
		Atmosphere->Activate();
	}
	else if (Atmosphere)
	{
		// Only one SkyAtmosphere per scene is supported; bodies without an atmosphere drop the component.
		Atmosphere->DestroyComponent();
		Atmosphere = nullptr;
	}
	for (auto& Pair : Patches)
	{
		if (Pair.Value)
		{
			Pair.Value->DestroyComponent();
		}
	}
	Patches.Reset();
	bHasEval = false;
}

uint64 ASpikePlanet::MakeKey(int32 Face, int32 Level, int32 IX, int32 IY)
{
	return (uint64(Face) << 60) | (uint64(Level) << 54) | (uint64(IX) << 27) | uint64(IY);
}

void ASpikePlanet::Collect(int32 Face, int32 Level, int32 IX, int32 IY, const FVector3d& CamLocal, TSet<uint64>& Out) const
{
	const double Edge = KspSpike::PatchEdgeLength(Level, Config.RadiusCm);
	const FVector3d Centre = KspSpike::PatchCentre(Face, Level, IX, IY, Config.RadiusCm);
	const double Dist = FMath::Max(0.0, (CamLocal - Centre).Length() - Edge * 0.75);
	const bool bSplit = Level < Config.MinLevel || (Level < Config.MaxLevel && Dist < Config.SplitFactor * Edge);
	if (bSplit)
	{
		for (int32 C = 0; C < 4; ++C)
		{
			Collect(Face, Level + 1, IX * 2 + (C & 1), IY * 2 + (C >> 1), CamLocal, Out);
		}
	}
	else
	{
		Out.Add(MakeKey(Face, Level, IX, IY));
	}
}

UProceduralMeshComponent* ASpikePlanet::CreatePatch(uint64 Key)
{
	const int32 Face = int32((Key >> 60) & 0xF);
	const int32 Level = int32((Key >> 54) & 0x3F);
	const int32 IX = int32((Key >> 27) & 0x7FFFFFF);
	const int32 IY = int32(Key & 0x7FFFFFF);

	KspSpike::FPatchMesh Mesh;
	KspSpike::BuildPatch(Face, Level, IX, IY, Config.GridN, Config.RadiusCm, Config.bPlanetCentredVerts, Config.bFlipWinding,
		Config.ColorMode, Config.FlatColor, Mesh);

	UProceduralMeshComponent* Comp = NewObject<UProceduralMeshComponent>(this, NAME_None, RF_Transient);
	Comp->SetMobility(EComponentMobility::Movable);
	Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Comp->SetCanEverAffectNavigation(false);
	Comp->SetCastShadow(false);
	Comp->SetupAttachment(Root);
	Comp->SetRelativeLocation(FVector(Mesh.Origin));
	Comp->CreateMeshSection(0, Mesh.Verts, Mesh.Tris, Mesh.Normals, TArray<FVector2D>(), Mesh.Colors, TArray<FProcMeshTangent>(), false);
	if (Config.Material)
	{
		Comp->SetMaterial(0, Config.Material);
	}
	Comp->RegisterComponent();
	return Comp;
}

double ASpikePlanet::UpdateLod(const FVector& CameraWorld, bool bForce, int32& OutCreated, int32& OutDestroyed)
{
	OutCreated = 0;
	OutDestroyed = 0;
	const FVector3d CamLocal = FVector3d(CameraWorld) - FVector3d(GetActorLocation());
	const double Altitude = FMath::Max(0.0, CamLocal.Length() - Config.RadiusCm);
	const double MoveThreshold = FMath::Max(100.0, Altitude * 0.02);
	if (!bForce && bHasEval && (CamLocal - LastEvalLocal).Length() < MoveThreshold)
	{
		return 0.0;
	}
	const double T0 = FPlatformTime::Seconds();
	LastEvalLocal = CamLocal;
	bHasEval = true;

	TSet<uint64> Wanted;
	for (int32 Face = 0; Face < 6; ++Face)
	{
		Collect(Face, 0, 0, 0, CamLocal, Wanted);
	}
	for (auto It = Patches.CreateIterator(); It; ++It)
	{
		if (!Wanted.Contains(It.Key()))
		{
			if (It.Value())
			{
				It.Value()->DestroyComponent();
			}
			It.RemoveCurrent();
			++OutDestroyed;
		}
	}
	for (const uint64 Key : Wanted)
	{
		if (!Patches.Contains(Key))
		{
			Patches.Add(Key, CreatePatch(Key));
			++OutCreated;
		}
	}
	return (FPlatformTime::Seconds() - T0) * 1000.0;
}
