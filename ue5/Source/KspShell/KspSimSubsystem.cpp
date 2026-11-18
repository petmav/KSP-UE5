#include "KspSimSubsystem.h"

#include "Camera/CameraActor.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/IConsoleManager.h"
#include "KspInputDevice.h"
#include "KspPartAssets.h"
#include "KspShell.h"
#include "KspTerrainView.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "ShaderCompiler.h"
#include "UnrealClient.h"
#include "Widgets/SViewport.h"

static TAutoConsoleVariable<int32> CVarKspDrawColliderProxies(TEXT("ksp.DrawColliderProxies"), 0,
	TEXT("Draw the collider proxies of every loaded part as a debug overlay (a part without a readable model always shows them)."));

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
	FString Keys;
	if (FParse::Value(FCommandLine::Get(), TEXT("KspKeys="), Keys, false))
	{
		TArray<FString> Entries;
		Keys.ParseIntoArray(Entries, TEXT(","));
		for (const FString& Entry : Entries)
		{
			FString Frame, Name;
			const FKey Key = Entry.Split(TEXT(":"), &Frame, &Name) ? FKey(*Name) : FKey();
			if (Key.IsValid())
			{
				ScriptedKeys.Emplace(FCString::Atoi64(*Frame), Key);
			}
			else
			{
				UE_LOG(LogKspShell, Error, TEXT("-KspKeys: '%s' is not <frame>:<Unreal key name>"), *Entry);
			}
		}
	}
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

	// the input device sees keys and mouse before any widget, while this process has a window
	InputDevice = MakeShared<FKspInputDevice>();
	bHadFocus = true;
	if (FApp::CanEverRender() && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().RegisterInputPreProcessor(InputDevice);
	}

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
	if (const FKspVesselState* Active = Sim->ActiveVessel())
	{
		UE_LOG(LogKspShell, Log, TEXT("active vessel at the end: altitude %.1f m, surface speed %.1f m/s, vertical speed %.1f m/s, %u parts, situation %d, UT %.2f"),
			Active->Altitude, Active->SurfaceSpeed, Active->VerticalSpeed, Active->PartCount, Active->Situation, Sim->FrameState().Ut);
	}
	if (InputDevice && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(InputDevice);
	}
	InputDevice.Reset();
	DestroyPartActors();
	if (PartAssets)
	{
		UE_LOG(LogKspShell, Log, TEXT("part assets: %s"), *PartAssets->DescribeStats());
	}
	if (Terrain)
	{
		UE_LOG(LogKspShell, Log, TEXT("terrain: %s"), *Terrain->DescribeStats());
		Terrain->Clear();
	}
	Sim.Reset();
	UE_LOG(LogKspShell, Log, TEXT("sim stopped: %lld frames, core %.3f ms per frame, render anchor moved %d time(s), %.3f ms in the frames it moved"), FrameCount,
		FrameCount > 0 ? 1000.0 * CoreSeconds / double(FrameCount) : 0.0, Anchor.MoveCount, 1000.0 * ReanchorSeconds);
}

bool UKspSimSubsystem::DefaultInput(FKspInputFrame& Input)
{
	FIntPoint Size(1280, 720);
	FVector2D Mouse(0.0, 720.0);
	bool bFocus = true, bUiHover = false;
	if (GEngine && GEngine->GameViewport && GEngine->GameViewport->Viewport)
	{
		Size = GEngine->GameViewport->Viewport->GetSizeXY();
		GEngine->GameViewport->GetMousePosition(Mouse);
		if (FSlateApplication::IsInitialized())
		{
			bFocus = FSlateApplication::Get().IsActive();
			// the pointer is over shell UI when it is over something that is not the game viewport itself
			const TSharedPtr<SViewport> ViewportWidget = FSlateApplication::Get().GetGameViewport();
			bUiHover = ViewportWidget.IsValid() && !ViewportWidget->IsDirectlyHovered();
		}
	}
	if (bHadFocus && !bFocus)
	{
		InputDevice->ReleaseAll();   // no key-up reaches a window without focus
	}
	bHadFocus = bFocus;
	InputDevice->TakeFrame(Input, Size, Mouse, bFocus, bUiHover, float(FPlatformTime::Seconds() - StartSeconds));
	return true;
}

void UKspSimSubsystem::RunFrame(double DtReal)
{
	if (!Sim)
	{
		return;
	}
	// scripted keys of the command-line session go the way real ones do: through Slate to the input device
	if (!ScriptedKeys.IsEmpty() && FSlateApplication::IsInitialized())
	{
		for (const TPair<int64, FKey>& Scripted : ScriptedKeys)
		{
			const FKeyEvent Event(Scripted.Value, FModifierKeysState(), 0, false, 0, 0);
			if (Scripted.Key == FrameCount)
			{
				FSlateApplication::Get().ProcessKeyDownEvent(Event);
			}
			else if (Scripted.Key + 5 == FrameCount)
			{
				FSlateApplication::Get().ProcessKeyUpEvent(Event);
			}
		}
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
		DestroyPartActors();
		if (Terrain)
		{
			Terrain->Clear();
		}
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
		// the shot waits for the shader compiler: a material draws as the engine's grey default until its shaders exist
		if (ShotTakenFrame < 0 && FrameCount >= ShotFrame && !(GShaderCompilingManager && GShaderCompilingManager->IsCompiling()))
		{
			FScreenshotRequest::RequestScreenshot(ShotFile, false, false);
			ShotTakenFrame = FrameCount;
		}
		else if (ShotTakenFrame >= 0 && FrameCount == ShotTakenFrame + 30)
		{
			FPlatformMisc::RequestExit(false, TEXT("KspShot"));
		}
	}
}

void UKspSimSubsystem::Place()
{
	const bool bPresent = PresentOverride.Get(FApp::CanEverRender());
	if (bPresent)
	{
		// the terrain stream is delivered once: taken every frame, whether or not there is a vessel to look at
		if (!Terrain)
		{
			Terrain = NewObject<UKspTerrainView>(this);
		}
		Terrain->Apply(GetWorld(), Sim->QuadsAdded(), Sim->QuadsRemoved());
	}
	const FKspVesselState* Active = Sim->ActiveVessel();
	if (!Active)
	{
		return;
	}
	const double T0 = FPlatformTime::Seconds();
	// the focus is the camera once core has one (WP-UE-03); until then the active vessel
	const bool bMoved = Anchor.Update(Active->Com, Sim->FrameState().FrameChangeSerial);
	if (bPresent)
	{
		Terrain->Place(Anchor, Sim->Bodies());
		UpdatePartActors();
		UpdateSun();
		DrawColliderProxies();
	}
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
	const bool bAll = CVarKspDrawColliderProxies.GetValueOnGameThread() != 0;
	if (!bAll && PartsWithoutModel.IsEmpty())
	{
		return;
	}
	const UWorld* World = GetWorld();
	const FColor Color(90, 220, 255);
	for (const FKspPartState& Part : Sim->Parts())
	{
		if (!bAll && !PartsWithoutModel.Contains(Part.Id))
		{
			continue;
		}
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

// ---- parts ----------------------------------------------------------------------------------------------------------------

void UKspSimSubsystem::UpdatePartActors()
{
	if (!PartAssets)
	{
		PartAssets = NewObject<UKspPartAssets>(this);
	}
	TSet<uint32> Seen;
	Seen.Reserve(Sim->Parts().Num());
	for (const FKspPartState& Part : Sim->Parts())
	{
		Seen.Add(Part.Id);
		TObjectPtr<AActor>& Actor = PartActors.FindOrAdd(Part.Id);
		if (!Actor)
		{
			Actor = SpawnPartActor(Part);
		}
		// every transform is recomputed from the snapshot each frame (DESIGN D6): no sweep, no physics
		Actor->SetActorLocationAndRotation(Anchor.ToWorld(Part.Position), Part.Rotation, false, nullptr, ETeleportType::TeleportPhysics);
		Actor->SetActorHiddenInGame((Part.Flags & 1u) != 0);   // bit 0: the part is dead
	}
	for (auto It = PartActors.CreateIterator(); It; ++It)
	{
		if (!Seen.Contains(It.Key()))
		{
			if (It.Value())
			{
				It.Value()->Destroy();
			}
			PartsWithoutModel.Remove(It.Key());
			It.RemoveCurrent();
		}
	}
}

AActor* UKspSimSubsystem::SpawnPartActor(const FKspPartState& Part)
{
	const FKspPartTemplate& Template = PartAssets->GetTemplate(Part.ModelId);
	FActorSpawnParameters Params;
	Params.ObjectFlags = RF_Transient;
	AActor* Actor = GetWorld()->SpawnActor<AActor>(Params);
	USceneComponent* Root = NewObject<USceneComponent>(Actor, TEXT("Part"));
	Root->SetMobility(EComponentMobility::Movable);
	Actor->SetRootComponent(Root);
	Root->RegisterComponent();
	Actor->SetActorEnableCollision(false);

	// the model's nodes that draw something, as a component tree with the nodes' local transforms (parents come first)
	bool bDraws = false;
	TArray<USceneComponent*> Components;
	Components.SetNumZeroed(Template.Nodes.Num());
	for (int32 N = 0; N < Template.Nodes.Num(); ++N)
	{
		const FKspPartNodeTemplate& Node = Template.Nodes[N];
		if (!Node.bKeep)
		{
			continue;
		}
		USceneComponent* Component = nullptr;
		if (Node.Mesh)
		{
			UStaticMeshComponent* MeshComponent = NewObject<UStaticMeshComponent>(Actor);
			MeshComponent->SetStaticMesh(Node.Mesh);
			for (int32 M = 0; M < Node.Materials.Num(); ++M)
			{
				MeshComponent->SetMaterial(M, Node.Materials[M]);
			}
			MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);   // DESIGN D13: nothing in the shell creates a physics body
			MeshComponent->SetGenerateOverlapEvents(false);
			MeshComponent->SetCastShadow(Node.bCastShadows);
			Component = MeshComponent;
			bDraws = true;
		}
		else
		{
			Component = NewObject<USceneComponent>(Actor);
		}
		Component->SetMobility(EComponentMobility::Movable);
		Component->SetupAttachment(Components.IsValidIndex(Node.Parent) && Components[Node.Parent] ? Components[Node.Parent] : Root);
		Component->SetRelativeTransform(Node.Local);
		Component->RegisterComponent();
		Components[N] = Component;
	}
	if (!bDraws)
	{
		PartsWithoutModel.Add(Part.Id);
	}
	return Actor;
}

void UKspSimSubsystem::DestroyPartActors()
{
	for (const TPair<uint32, TObjectPtr<AActor>>& Pair : PartActors)
	{
		if (Pair.Value)
		{
			Pair.Value->Destroy();
		}
	}
	PartActors.Reset();
	PartsWithoutModel.Reset();
}

void UKspSimSubsystem::UpdateSun()
{
	if (!SunLight)
	{
		// the scene's sun until WP-UE-12 (sky, atmosphere, light colour): white, 10 lux as in the scale spike
		FActorSpawnParameters Params;
		Params.ObjectFlags = RF_Transient;
		SunLight = GetWorld()->SpawnActor<ADirectionalLight>(Params);
		SunLight->GetRootComponent()->SetMobility(EComponentMobility::Movable);
		SunLight->GetLightComponent()->SetIntensity(10.0f);
	}
	// light travels from the sun towards the dominant body
	SunLight->SetActorRotation((-Sim->SunDirection()).Rotation());
}
