#pragma once

#include "CoreMinimal.h"

struct kspcore_sim;

/**
 * Unit and axis conversion between core and Unreal (ue5/DESIGN.md 1.3, the one place it lives).
 *
 * Core uses Unity's axes: left-handed, Y up, metres. Unreal is left-handed, Z up, centimetres.
 *   UE (X, Y, Z) = Unity (Z, X, Y) x 100
 * The axis map is a cyclic permutation (determinant +1), so a rotation keeps its sense and a quaternion converts by permuting its
 * vector part. "Unity" values travel in FVector / FQuat as plain (x, y, z[, w]) containers.
 */
struct KSPCOREBRIDGE_API FKspAxis
{
	static constexpr double CmPerMetre = 100.0;

	/** Unity metres -> UE centimetres. */
	static FVector PositionToUe(const FVector& Unity);
	/** UE centimetres -> Unity metres. */
	static FVector PositionToUnity(const FVector& Ue);
	/** Axis map without the unit scale (directions, half extents, angular quantities). */
	static FVector DirectionToUe(const FVector& Unity);
	static FVector DirectionToUnity(const FVector& Ue);
	static FQuat RotationToUe(const FQuat& Unity);
	static FQuat RotationToUnity(const FQuat& Ue);
};

/** GameScenes numbers of core's HighLogic.LoadedScene. */
namespace KspScene
{
	inline constexpr int32 MainMenu = 2;
	inline constexpr int32 SpaceCenter = 5;
	inline constexpr int32 Editor = 6;
	inline constexpr int32 Flight = 7;
	inline constexpr int32 TrackStation = 8;
}

/** Snapshot event kinds (kspcore_frame.h KSPCORE_EVENT_*). */
namespace KspEvent
{
	inline constexpr int32 SceneLoadRequested = 1;
	inline constexpr int32 VesselCreated = 2;
	inline constexpr int32 VesselDestroyed = 3;
	inline constexpr int32 VesselSwitched = 4;
	inline constexpr int32 PartDied = 5;
	inline constexpr int32 PartExploded = 6;
	inline constexpr int32 StructureChanged = 7;
	inline constexpr int32 FrameChange = 8;
	inline constexpr int32 FloatingOriginRebase = 9;
	inline constexpr int32 ScreenMessage = 10;
	inline constexpr int32 SoundCue = 11;
	inline constexpr int32 SceneLoaded = 12;
}

/** Vessel flag bits (KSPCORE_VESSEL_*). */
namespace KspVesselFlag
{
	inline constexpr uint32 Loaded = 1;
	inline constexpr uint32 Packed = 2;
	inline constexpr uint32 Active = 4;
	inline constexpr uint32 Landed = 8;
	inline constexpr uint32 Splashed = 16;
}

/** Process initialisation of core (kspcore_game_config). Paths are absolute. */
struct FKspGameConfig
{
	/** The folder holding GameData/ (decompiled/KSP-dev at run time, the repository's mirror in gates). */
	FString GameRoot;
	/** core/data of the repository (terrain maps, scenery colliders, engine thrust transforms). */
	FString DataDir;
	/** ue5/GameAssets; empty when there is none. */
	FString ExtractionRoot;
	/** A save is <SavesDir>/<save name>/<file>.sfs. */
	FString SavesDir;
	/** Reduced model data for a game root whose .mu files are stubs (the mirror); empty for a real game root. */
	FString ReducedModelDir;
	int32 PqsHeightThreads = 0;
	int32 PhysicsThreads = 0;

	bool operator==(const FKspGameConfig& Other) const = default;
};

/** The process-wide part of core: one initialisation and one game database per process. */
class KSPCOREBRIDGE_API FKspGame
{
public:
	/** kspcore_game_init + kspcore_game_load_database. True at once when core is already initialised with an equal configuration. */
	static bool Init(const FKspGameConfig& Config, FString& OutError);
	/** kspcore_game_shutdown. False (nothing done) while a FKspSim exists. */
	static bool Shutdown();
	static bool IsInitialised();
	static const FKspGameConfig& GetConfig();
	/** Parts of the game database; their model ids are 1 .. PartCount. */
	static int32 PartCount();
	/** The AvailablePart name of a snapshot part's model id, empty when unknown. */
	static FString ModelName(uint32 ModelId);
};

/** Raw device state of one frame (kspcore_input_frame). Key numbers are UnityEngine.KeyCode values. */
struct FKspInputFrame
{
	TArray<int32> KeysDown;
	TArray<int32> KeysUp;
	TArray<int32> KeysHeld;
	/** Pixels, origin bottom-left as Unity. */
	FVector2D MousePosition = FVector2D::ZeroVector;
	FVector2f MouseDelta = FVector2f::ZeroVector;
	uint32 MouseButtonsHeld = 0;
	uint32 MouseButtonsDown = 0;
	uint32 MouseButtonsUp = 0;
	float MouseWheel = 0.0f;
	int32 ScreenWidth = 0;
	int32 ScreenHeight = 0;
	bool bHasFocus = true;
	bool bUiHoverLock = false;
	/** Time.realtimeSinceStartup. */
	float RealTimeSinceStartup = 0.0f;
};

struct FKspFrameState
{
	double Ut = 0.0;
	uint64 FrameCount = 0;
	uint64 FixedSteps = 0;
	uint32 FixedStepsLast = 0;
	float WarpRate = 1.0f;
	int32 WarpIndex = 0;
	int32 WarpMode = 0;
	bool bPaused = false;
	int32 Scene = -1;
	/** 0 rotating (body-fixed axes), 1 inertial. */
	int32 FrameKind = 0;
	int32 DominantBodyId = -1;
	uint32 FrameChangeSerial = 0;
	int32 ActiveVesselId = 0;
};

/** Positions are in UE axes and centimetres, relative to the centre of the dominant body (invariant under core's own rebases). */
struct FKspVesselState
{
	int32 Id = 0;
	uint32 Flags = 0;
	int32 Situation = 0;
	FVector Com = FVector::ZeroVector;
	FVector RootPosition = FVector::ZeroVector;
	FQuat RootRotation = FQuat::Identity;
	uint32 FirstPart = 0;
	uint32 PartCount = 0;
	/** Metres above sea level (the root part). */
	double Altitude = 0.0;
	/** m/s. */
	float SurfaceSpeed = 0.0f;
	float Mach = 0.0f;
	float VerticalSpeed = 0.0f;
};

struct FKspPartState
{
	uint32 Id = 0;
	int32 VesselId = 0;
	uint32 ModelId = 0;
	uint32 Flags = 0;
	FVector Position = FVector::ZeroVector;
	FQuat Rotation = FQuat::Identity;
};

/** Payload as core sent it (kspcore_event): D is in core's axes and metres. */
struct FKspEvent
{
	int32 Kind = 0;
	int32 A = 0;
	int32 B = 0;
	int32 Id = 0;
	double D[7] = {};
};

/** One collider of a part in PART space, converted to UE axes and centimetres (kspcore_collider_proxy). */
struct FKspColliderProxy
{
	enum class EKind : uint8 { Box, Sphere, Capsule, ConvexHull };
	EKind Kind = EKind::Box;
	/** The collider frame in part space. */
	FTransform Frame = FTransform::Identity;
	FVector HalfExtents = FVector::ZeroVector;
	double Radius = 0.0;
	/** Capsule: half length of the cylinder part, along the collider frame's Z (Unity's Y). */
	double HalfHeight = 0.0;
	/** Convex hull points in the collider frame. */
	TArray<FVector> HullPoints;
};

/**
 * One simulation (kspcore_sim): the game, its scene and the per-frame bridge of ue5/DESIGN.md 1.3. One per process.
 * Every call into core runs under FKspFpGuard. The views are copies made by Frame() / SceneLoaded(), valid until the next of those.
 */
class KSPCOREBRIDGE_API FKspSim
{
public:
	/** kspcore_sim_create; FKspGame::Init must have succeeded. */
	FKspSim();
	~FKspSim();
	FKspSim(const FKspSim&) = delete;
	FKspSim& operator=(const FKspSim&) = delete;

	bool IsValid() const { return Handle != nullptr; }

	/** A new sandbox game saved as <save>/persistent.sfs. */
	bool NewSandboxGame(const FString& SaveName);
	/** FlightDriver.StartWithNewLaunch: queues the FLIGHT scene load (event SceneLoadRequested in the next frame). */
	bool LaunchCraft(const FString& CraftPath, const FString& LaunchSite);
	/** The shell has built the scene's actors: core starts the scene. Refreshes the views. */
	bool SceneLoaded(int32 Scene);
	int32 CurrentScene() const;

	/** Input of the next Frame(). */
	void SetInput(const FKspInputFrame& Input);
	/** core's PlayerLoop.Frame with the frame's real delta in seconds, then the views are refreshed. False on a core failure. */
	bool Frame(double DtReal);

	const FKspFrameState& FrameState() const { return FrameView; }
	TConstArrayView<FKspVesselState> Vessels() const { return VesselView; }
	TConstArrayView<FKspPartState> Parts() const { return PartView; }
	TConstArrayView<FKspEvent> Events() const { return EventView; }
	/** Unit vector from the centre of the dominant body to the sun, UE axes. */
	const FVector& SunDirection() const { return SunView; }
	/** The active vessel of the snapshot, null when there is none. */
	const FKspVesselState* ActiveVessel() const;

	/** kspcore_snapshot_hash of the current snapshot (the parity gates of DESIGN 1.5). */
	uint64 SnapshotHash() const;
	/** The bubble's collider proxies of a loaded part; empty for an unknown part. Does not change the simulation state. */
	void GetPartColliders(uint32 PartId, TArray<FKspColliderProxy>& Out) const;

	/** kspcore_last_error of the calling thread. */
	static FString LastError();

private:
	void RefreshViews();

	kspcore_sim* Handle = nullptr;
	FKspFrameState FrameView;
	FVector SunView = FVector::ForwardVector;
	TArray<FKspVesselState> VesselView;
	TArray<FKspPartState> PartView;
	TArray<FKspEvent> EventView;
};
