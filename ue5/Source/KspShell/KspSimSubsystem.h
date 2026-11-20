#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineBaseTypes.h"
#include "InputCoreTypes.h"
#include "KspRenderAnchor.h"
#include "KspSim.h"
#include "Subsystems/WorldSubsystem.h"

#include "KspSimSubsystem.generated.h"

class ACameraActor;
class ADirectionalLight;
class FKspInputDevice;
class UKspPartAssets;
class UKspTerrainView;
class UKspSimSubsystem;

/** The sim's tick: TG_PrePhysics, before cameras and rendering (ue5/DESIGN.md 1.3 "Per-frame sequence"). */
struct FKspSimTickFunction : public FTickFunction
{
	UKspSimSubsystem* Target = nullptr;

	virtual void ExecuteTick(float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent) override;
	virtual FString DiagnosticMessage() override { return TEXT("FKspSimTickFunction"); }
	virtual FName DiagnosticContext(bool bDetailed) override { return FName(TEXT("KspSim")); }
};

/**
 * Owns the simulation (one per process) and runs core's frame once per rendered frame (WP-UE-07):
 *
 *   input -> kspcore_frame -> snapshot -> events (scene loads) -> render anchor -> placement
 *
 * Placement today: one actor per loaded part with the part's own meshes and textures (UKspPartAssets, WP-UE-10), a directional light
 * from core's sun direction, the dominant body's terrain quads (UKspTerrainView, WP-UE-11) and, for a session started from the command line, a fixed chase camera (the stand-in until the camera
 * packages, WP-UE-03 / 09). Collider proxies are a debug overlay (cvar ksp.DrawColliderProxies) and what a part without a readable
 * model is drawn with.
 *
 * Command line of a game session (UnrealEditor.exe <uproject> <map> -game):
 *   -KspLaunch=<craft file>     start a sandbox game and launch the craft (absolute, or relative to the repository root)
 *   -KspSite=<LaunchPad|Runway> launch site, default LaunchPad
 *   -KspGameRoot=<dir>          game root, default <repository>/decompiled/KSP-dev
 *   -KspKeys=<frame>:<key>,...   press a key at a sim frame and release it five frames later, as Slate key events (Unreal key names:
 *                               "-KspKeys=100:Z,100:T,110:SpaceBar" is full throttle, SAS, stage)
 *   -KspShot=<png file>         take a screenshot at sim frame -KspShotFrame=<n> (default 300; later while shaders compile), then quit
 */
UCLASS()
class KSPSHELL_API UKspSimSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	/** The run-time configuration: the game copy decompiled/KSP-dev, core/data, ue5/GameAssets when extracted, saves under Saved/ksp/saves. */
	static FKspGameConfig DefaultGameConfig();

	/**
	 * Initialises core (once per process), creates the sim with a new sandbox game and launches the craft. The flight scene starts within
	 * the next frames: core requests the scene load, the shell answers.
	 */
	bool StartSandboxFlight(const FKspGameConfig& Config, const FString& SaveName, const FString& CraftPath, const FString& LaunchSite, FString& OutError);
	/** Destroys the sim and logs the session's frame statistics. */
	void StopSim();

	/** The sim while one runs, else null. */
	const FKspSim* GetSim() const { return Sim.Get(); }
	const FKspRenderAnchor& GetAnchor() const { return Anchor; }
	/** Frames run by the current sim. */
	int64 GetFrameCount() const { return FrameCount; }
	/** Seconds spent in input + kspcore_frame + snapshot copy over those frames. */
	double GetCoreSeconds() const { return CoreSeconds; }
	/** Seconds spent placing on the frames in which the anchor moved. */
	double GetReanchorSeconds() const { return ReanchorSeconds; }
	/** The part models built so far; null before the first part is drawn. */
	const UKspPartAssets* GetPartAssets() const { return PartAssets; }
	/** The terrain of the dominant body; null before the first quad arrives. */
	const UKspTerrainView* GetTerrain() const { return Terrain; }

	/**
	 * Whether the scene is presented (part actors, terrain, light). Unset: when the process can render. Tests of the presentation set
	 * it to true to build the scene in a headless run.
	 */
	TOptional<bool> PresentOverride;

	/** Parity gates and benchmarks: every frame passes exactly this delta to core instead of the world's delta time. */
	TOptional<double> FixedFrameDt;
	/**
	 * Fills the input of the next frame; false = no input is set for that frame. Unbound (the normal case): the input device's frame
	 * with the viewport's size, the cursor and the window's focus. Tests bind it to script the input.
	 */
	TFunction<bool(FKspInputFrame&)> InputSource;
	/** The input device of the running sim (keys and mouse as Slate delivers them); null without a sim. */
	FKspInputDevice* GetInputDevice() const { return InputDevice.Get(); }

	/** One frame of the sequence above. The tick function calls it; public for callers that own the frame loop. */
	void RunFrame(double DtReal);

private:
	bool DefaultInput(FKspInputFrame& Input);
	void Place();
	void DrawColliderProxies();
	void UpdatePartActors();
	AActor* SpawnPartActor(const FKspPartState& Part);
	void DestroyPartActors();
	void UpdateSun();
	void UpdateDebugCamera(const FKspVesselState& Active);

	FKspSimTickFunction TickFunction;
	TUniquePtr<FKspSim> Sim;
	TSharedPtr<FKspInputDevice> InputDevice;
	bool bHadFocus = true;
	FKspRenderAnchor Anchor;
	/** Collider proxies per part id (they do not change while a part lives). */
	TMap<uint32, TArray<FKspColliderProxy>> ColliderCache;

	int64 FrameCount = 0;
	double CoreSeconds = 0.0;
	double ReanchorSeconds = 0.0;
	double StartSeconds = 0.0;

	// the command-line session
	bool bDebugView = false;
	FString ShotFile;
	/** Scripted key presses of the command-line session: sim frame and key. */
	TArray<TPair<int64, FKey>> ScriptedKeys;
	int64 ShotFrame = 300;
	int64 ShotTakenFrame = -1;

	UPROPERTY(Transient)
	TObjectPtr<ACameraActor> DebugCamera;

	UPROPERTY(Transient)
	TObjectPtr<UKspPartAssets> PartAssets;

	/** Part id -> the actor that draws it. */
	UPROPERTY(Transient)
	TMap<uint32, TObjectPtr<AActor>> PartActors;

	/** Parts whose model draws nothing: shown as their collider proxies. */
	TSet<uint32> PartsWithoutModel;

	UPROPERTY(Transient)
	TObjectPtr<ADirectionalLight> SunLight;

	UPROPERTY(Transient)
	TObjectPtr<UKspTerrainView> Terrain;
};
