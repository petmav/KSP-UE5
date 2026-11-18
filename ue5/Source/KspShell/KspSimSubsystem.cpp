#include "KspSimSubsystem.h"

#include "Camera/CameraActor.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformFileManager.h"
#include "KspShell.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

static TAutoConsoleVariable<int32> CVarKspDrawColliderProxies(TEXT("ksp.DrawColliderProxies"), 1,
	TEXT("Draw the collider proxies of every loaded part (the stand-in for part rendering until WP-UE-10, a debug overlay afterwards)."));

void FKspSimTickFunction::ExecuteTick(float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
{
	if (Target)
	{
		Target->RunFrame(Target->FixedFrameDt.Get(double(DeltaTime)));
	}
}

bool UKspSimSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// one sim per process, in the game world; play-in-editor with two worlds is not supported (DESIGN 1.3 "Lifetime")
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

FKspGameConfig UKspSimSubsystem::DefaultGameConfig()
{
	const FString Repo = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), TEXT("..")));
	FKspGameConfig Config;
	Config.GameRoot = FPaths::Combine(Repo, TEXT("decompiled"), TEXT("KSP-dev"));
	Config.DataDir = FPaths::Combine(Repo, TEXT("core"), TEXT("data"));
	const FString Extraction = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), TEXT("GameAssets")));
	if (FPaths::DirectoryExists(Extraction))
	{
		Config.ExtractionRoot = Extraction;
	}
	Config.SavesDir = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("ksp"), TEXT("saves")));
	Config.PqsHeightThreads = 2;   // DESIGN 1.4: bit-identical results with any worker count
	return Config;
}

void UKspSimSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	FString Craft;
	if (!FParse::Value(FCommandLine::Get(), TEXT("KspLaunch="), Craft))
	{
		return;
	}
	FKspGameConfig Config = DefaultGameConfig();
	FParse::Value(FCommandLine::Get(), TEXT("KspGameRoot="), Config.GameRoot);
	if (FPaths::IsRelative(Craft))
	{
		Craft = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), TEXT(".."), Craft));
	}
	FString Site = TEXT("LaunchPad");
	FParse::Value(FCommandLine::Get(), TEXT("KspSite="), Site);
	FString Error;
	if (!StartSandboxFlight(Config, TEXT("slice"), Craft, Site, Error))
	{
		UE_LOG(LogKspShell, Error, TEXT("-KspLaunch failed: %s"), *Error);
		return;
	}
	bDebugView = true;
	FParse::Value(FCommandLine::Get(), TEXT("KspShot="), ShotFile);
	FParse::Value(FCommandLine::Get(), TEXT("KspShotFrame="), ShotFrame);
}

void UKspSimSubsystem::Deinitialize()
{
	StopSim();
	Super::Deinitialize();
}

bool UKspSimSubsystem::StartSandboxFlight(const FKspGameConfig& Config, const FString& SaveName, const FString& CraftPath, const FString& LaunchSite,
	FString& OutError)
{
	StopSim();
	if (!FKspGame::Init(Config, OutError))
	{
		return false;
	}
	Sim = MakeUnique<FKspSim>();
	if (!Sim->IsValid() || !Sim->NewSandboxGame(SaveName) || !Sim->LaunchCraft(CraftPath, LaunchSite))
	{
		OutError = FKspSim::LastError();
		Sim.Reset();
		return false;
	}
	FrameCount = 0;
	CoreSeconds = 0.0;
	ReanchorSeconds = 0.0;
	StartSeconds = FPlatformTime::Seconds();
	Anchor.Reset();
	ColliderCache.Reset();

	TickFunction.Target = this;
	TickFunction.TickGroup = TG_PrePhysics;
	TickFunction.bCanEverTick = true;
	TickFunction.bStartWithTickEnabled = true;
	TickFunction.RegisterTickFunction(GetWorld()->PersistentLevel);
	UE_LOG(LogKspShell, Log, TEXT("sim started: %s from %s"), *CraftPath, *LaunchSite);
	return true;
}

void UKspSimSubsystem::StopSim()
{
	if (!Sim)
	{
		return;
	}
	TickFunction.UnRegisterTickFunction();
	Sim.Reset();
	UE_LOG(LogKspShell, Log, TEXT("sim stopped: %lld frames, core %.3f ms per frame, render anchor moved %d time(s), %.3f ms in the frames it moved"), FrameCount,
		FrameCount > 0 ? 1000.0 * CoreSeconds / double(FrameCount) : 0.0, Anchor.MoveCount, 1000.0 * ReanchorSeconds);
}

bool UKspSimSubsystem::DefaultInput(FKspInputFrame& Input) const
{
	FIntPoint Size(1280, 720);
	if (GEngine && GEngine->GameViewport && GEngine->GameViewport->Viewport)
	{
		Size = GEngine->GameViewport->Viewport->GetSizeXY();
	}
	Input.ScreenWidth = Size.X;
	Input.ScreenHeight = Size.Y;
	Input.RealTimeSinceStartup = float(FPlatformTime::Seconds() - StartSeconds);
	return true;
}

void UKspSimSubsystem::RunFrame(double DtReal)
{
	if (!Sim)
	{
		return;
	}
	const double T0 = FPlatformTime::Seconds();
	FKspInputFrame Input;
	if (InputSource ? InputSource(Input) : DefaultInput(Input))
	{
		Sim->SetInput(Input);
	}
	if (!Sim->Frame(DtReal))
	{
		UE_LOG(LogKspShell, Error, TEXT("kspcore_frame failed at frame %lld: %s"), FrameCount, *FKspSim::LastError());
		StopSim();
		return;
	}
	CoreSeconds += FPlatformTime::Seconds() - T0;
	++FrameCount;

	// Scene loads. The scene director of WP-UE-17 will build the scene's actor sets over several frames here; until then there is
	// nothing to build and the scene is ready at once.
	int32 RequestedScene = -1;
	for (const FKspEvent& Event : Sim->Events())
	{
		if (Event.Kind == KspEvent::SceneLoadRequested)
		{
			RequestedScene = Event.A;
		}
	}
	if (RequestedScene >= 0)
	{
		Anchor.Reset();
		ColliderCache.Reset();
		if (!Sim->SceneLoaded(RequestedScene))
		{
			UE_LOG(LogKspShell, Error, TEXT("kspcore_scene_loaded(%d) failed: %s"), RequestedScene, *FKspSim::LastError());
			StopSim();
			return;
		}
	}
	Place();

	if (!ShotFile.IsEmpty())
	{
		if (FrameCount == ShotFrame)
		{
			FScreenshotRequest::RequestScreenshot(ShotFile, false, false);
		}
		else if (FrameCount == ShotFrame + 30)
		{
			FPlatformMisc::RequestExit(false, TEXT("KspShot"));
		}
	}
}

void UKspSimSubsystem::Place()
{
	const FKspVesselState* Active = Sim->ActiveVessel();
	if (!Active)
	{
		return;
	}
	const double T0 = FPlatformTime::Seconds();
	// the focus is the camera once core has one (WP-UE-03); until then the active vessel
	const bool bMoved = Anchor.Update(Active->Com, Sim->FrameState().FrameChangeSerial);
	DrawColliderProxies();
	UpdateDebugCamera(*Active);
	if (bMoved)
	{
		const double Seconds = FPlatformTime::Seconds() - T0;
		ReanchorSeconds += Seconds;
		UE_LOG(LogKspShell, Log, TEXT("render anchor moved (%d): frame %lld, altitude %.0f m, placement of that frame %.3f ms"), Anchor.MoveCount, FrameCount,
			Active->Altitude, 1000.0 * Seconds);
	}
}

void UKspSimSubsystem::DrawColliderProxies()
{
#if ENABLE_DRAW_DEBUG
	if (!FApp::CanEverRender() || CVarKspDrawColliderProxies.GetValueOnGameThread() == 0)
	{
		return;
	}
	const UWorld* World = GetWorld();
	const FColor Color(90, 220, 255);
	for (const FKspPartState& Part : Sim->Parts())
	{
		TArray<FKspColliderProxy>* Proxies = ColliderCache.Find(Part.Id);
		if (!Proxies)
		{
			Proxies = &ColliderCache.Add(Part.Id);
			Sim->GetPartColliders(Part.Id, *Proxies);
		}
		const FTransform PartToWorld(Part.Rotation, Anchor.ToWorld(Part.Position));
		for (const FKspColliderProxy& Proxy : *Proxies)
		{
			const FTransform ToWorld = Proxy.Frame * PartToWorld;
			switch (Proxy.Kind)
			{
			case FKspColliderProxy::EKind::Box:
				DrawDebugBox(World, ToWorld.GetLocation(), Proxy.HalfExtents, ToWorld.GetRotation(), Color);
				break;
			case FKspColliderProxy::EKind::Sphere:
				DrawDebugSphere(World, ToWorld.GetLocation(), float(Proxy.Radius), 12, Color);
				break;
			case FKspColliderProxy::EKind::Capsule:
				DrawDebugCapsule(World, ToWorld.GetLocation(), float(Proxy.HalfHeight + Proxy.Radius), float(Proxy.Radius), ToWorld.GetRotation(), Color);
				break;
			case FKspColliderProxy::EKind::ConvexHull:
				for (const FVector& Point : Proxy.HullPoints)
				{
					DrawDebugPoint(World, ToWorld.TransformPosition(Point), 4.0f, Color);
				}
				break;
			}
		}
	}
#endif
}

void UKspSimSubsystem::UpdateDebugCamera(const FKspVesselState& Active)
{
	if (!bDebugView)
	{
		return;
	}
	UWorld* World = GetWorld();
	APlayerController* Controller = World->GetFirstPlayerController();
	if (!Controller)
	{
		return;
	}
	if (!DebugCamera)
	{
		DebugCamera = World->SpawnActor<ACameraActor>();
		Controller->SetViewTarget(DebugCamera);
	}
	// 25 m to the side of the vessel and 4 m above its centre of mass, "up" = away from the body's centre
	const FVector Up = Active.Com.GetSafeNormal();
	FVector Side = FVector::CrossProduct(Up, FVector::UpVector);
	if (!Side.Normalize())
	{
		Side = FVector::ForwardVector;
	}
	const FVector Target = Anchor.ToWorld(Active.Com);
	const FVector Eye = Target + Side * 2500.0 + Up * 400.0;
	DebugCamera->SetActorLocationAndRotation(Eye, FRotationMatrix::MakeFromXZ(Target - Eye, Up).ToQuat());
}
