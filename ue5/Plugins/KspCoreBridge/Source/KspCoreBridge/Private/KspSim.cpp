#include "KspSim.h"

#include "KspCoreBridge.h"

#include "kspcore/kspcore_c.h"

DEFINE_LOG_CATEGORY_STATIC(LogKspCore, Log, All);

static_assert(KspScene::MainMenu == KSPCORE_SCENE_MAINMENU && KspScene::SpaceCenter == KSPCORE_SCENE_SPACECENTER &&
	KspScene::Editor == KSPCORE_SCENE_EDITOR && KspScene::Flight == KSPCORE_SCENE_FLIGHT && KspScene::TrackStation == KSPCORE_SCENE_TRACKSTATION,
	"KspScene mirrors KSPCORE_SCENE_*");
static_assert(KspEvent::SceneLoadRequested == KSPCORE_EVENT_SCENE_LOAD_REQUESTED && KspEvent::VesselCreated == KSPCORE_EVENT_VESSEL_CREATED &&
	KspEvent::VesselDestroyed == KSPCORE_EVENT_VESSEL_DESTROYED && KspEvent::VesselSwitched == KSPCORE_EVENT_VESSEL_SWITCHED &&
	KspEvent::PartDied == KSPCORE_EVENT_PART_DIED && KspEvent::PartExploded == KSPCORE_EVENT_PART_EXPLODED &&
	KspEvent::StructureChanged == KSPCORE_EVENT_STRUCTURE_CHANGED && KspEvent::FrameChange == KSPCORE_EVENT_FRAME_CHANGE &&
	KspEvent::FloatingOriginRebase == KSPCORE_EVENT_FLOATING_ORIGIN_REBASE && KspEvent::ScreenMessage == KSPCORE_EVENT_SCREEN_MESSAGE &&
	KspEvent::SoundCue == KSPCORE_EVENT_SOUND_CUE && KspEvent::SceneLoaded == KSPCORE_EVENT_SCENE_LOADED,
	"KspEvent mirrors KSPCORE_EVENT_*");
static_assert(KspVesselFlag::Loaded == KSPCORE_VESSEL_LOADED && KspVesselFlag::Packed == KSPCORE_VESSEL_PACKED &&
	KspVesselFlag::Active == KSPCORE_VESSEL_ACTIVE && KspVesselFlag::Landed == KSPCORE_VESSEL_LANDED &&
	KspVesselFlag::Splashed == KSPCORE_VESSEL_SPLASHED, "KspVesselFlag mirrors KSPCORE_VESSEL_*");
static_assert(int32(FKspColliderProxy::EKind::Box) == KSPCORE_COLLIDER_BOX && int32(FKspColliderProxy::EKind::Sphere) == KSPCORE_COLLIDER_SPHERE &&
	int32(FKspColliderProxy::EKind::Capsule) == KSPCORE_COLLIDER_CAPSULE && int32(FKspColliderProxy::EKind::ConvexHull) == KSPCORE_COLLIDER_CONVEX_HULL,
	"FKspColliderProxy::EKind mirrors KSPCORE_COLLIDER_*");

// ---- axis conversion ------------------------------------------------------------------------------------------------------

FVector FKspAxis::DirectionToUe(const FVector& Unity)
{
	return FVector(Unity.Z, Unity.X, Unity.Y);
}

FVector FKspAxis::DirectionToUnity(const FVector& Ue)
{
	return FVector(Ue.Y, Ue.Z, Ue.X);
}

FVector FKspAxis::PositionToUe(const FVector& Unity)
{
	return FVector(Unity.Z * CmPerMetre, Unity.X * CmPerMetre, Unity.Y * CmPerMetre);
}

FVector FKspAxis::PositionToUnity(const FVector& Ue)
{
	return FVector(Ue.Y / CmPerMetre, Ue.Z / CmPerMetre, Ue.X / CmPerMetre);
}

FQuat FKspAxis::RotationToUe(const FQuat& Unity)
{
	return FQuat(Unity.Z, Unity.X, Unity.Y, Unity.W);
}

FQuat FKspAxis::RotationToUnity(const FQuat& Ue)
{
	return FQuat(Ue.Y, Ue.Z, Ue.X, Ue.W);
}

namespace
{
	FVector PositionToUe(const double* P) { return FKspAxis::PositionToUe(FVector(P[0], P[1], P[2])); }
	FVector PositionToUe(const float* P) { return FKspAxis::PositionToUe(FVector(P[0], P[1], P[2])); }
	FQuat RotationToUe(const float* Q) { return FKspAxis::RotationToUe(FQuat(Q[0], Q[1], Q[2], Q[3])); }

	// ---- process state ----
	bool GKspInitialised = false;
	FKspGameConfig GKspConfig;
	int32 GKspLiveSims = 0;

	void OnCoreLog(int Level, const char* Message, void*)
	{
		// Core's Debug.LogWarning / LogError equivalents are KSP's own diagnostics (cfg warnings and the like), not shell failures:
		// they are logged at Log verbosity so they do not fail automation tests.
		if (Level >= KSPCORE_LOG_WARNING)
		{
			UE_LOG(LogKspCore, Log, TEXT("[core %s] %s"), Level >= KSPCORE_LOG_ERROR ? TEXT("error") : TEXT("warning"), UTF8_TO_TCHAR(Message));
		}
	}
}

// ---- process --------------------------------------------------------------------------------------------------------------

bool FKspGame::Init(const FKspGameConfig& Config, FString& OutError)
{
	if (GKspInitialised)
	{
		if (Config == GKspConfig)
		{
			return true;
		}
		OutError = TEXT("kspcore is already initialised with another configuration (FKspGame::Shutdown first)");
		return false;
	}
	const FTCHARToUTF8 GameRoot(*Config.GameRoot), DataDir(*Config.DataDir), ExtractionRoot(*Config.ExtractionRoot), SavesDir(*Config.SavesDir),
		ReducedModelDir(*Config.ReducedModelDir);
	kspcore_game_config Cfg;
	FMemory::Memzero(Cfg);
	Cfg.struct_size = sizeof(Cfg);
	Cfg.game_root = GameRoot.Get();
	Cfg.data_dir = DataDir.Get();
	Cfg.extraction_root = Config.ExtractionRoot.IsEmpty() ? nullptr : ExtractionRoot.Get();
	Cfg.saves_dir = SavesDir.Get();
	Cfg.reduced_model_dir = Config.ReducedModelDir.IsEmpty() ? nullptr : ReducedModelDir.Get();
	Cfg.pqs_height_threads = Config.PqsHeightThreads;
	Cfg.physics_threads = Config.PhysicsThreads;

	FKspFpGuard Guard;
	const double T0 = FPlatformTime::Seconds();
	if (kspcore_game_init(&Cfg) != 1)
	{
		OutError = FKspSim::LastError();
		return false;
	}
	if (kspcore_game_load_database(nullptr, nullptr) != 1)
	{
		OutError = FKspSim::LastError();
		kspcore_game_shutdown();
		return false;
	}
	// after the database load: PartLoader logs hundreds of cfg warnings
	kspcore_set_log_callback(&OnCoreLog, nullptr);
	GKspInitialised = true;
	GKspConfig = Config;
	UE_LOG(LogKspCore, Log, TEXT("kspcore game initialised in %.2f s: %d parts, %d resources, game root %s"), FPlatformTime::Seconds() - T0,
		kspcore_game_part_count(), kspcore_game_resource_count(), *Config.GameRoot);
	return true;
}

bool FKspGame::Shutdown()
{
	if (!GKspInitialised)
	{
		return true;
	}
	if (GKspLiveSims != 0)
	{
		return false;
	}
	FKspFpGuard Guard;
	kspcore_set_log_callback(nullptr, nullptr);
	kspcore_game_shutdown();
	GKspInitialised = false;
	return true;
}

bool FKspGame::IsInitialised()
{
	return GKspInitialised;
}

const FKspGameConfig& FKspGame::GetConfig()
{
	return GKspConfig;
}

FString FKspGame::ModelName(uint32 ModelId)
{
	FKspFpGuard Guard;
	return FString(UTF8_TO_TCHAR(kspcore_game_model_name(ModelId)));
}

// ---- sim ------------------------------------------------------------------------------------------------------------------

FKspSim::FKspSim()
{
	if (GKspInitialised)
	{
		FKspFpGuard Guard;
		Handle = kspcore_sim_create();
		if (Handle)
		{
			++GKspLiveSims;
		}
	}
}

FKspSim::~FKspSim()
{
	if (Handle)
	{
		FKspFpGuard Guard;
		kspcore_sim_destroy(Handle);
		--GKspLiveSims;
	}
}

FString FKspSim::LastError()
{
	return FString(UTF8_TO_TCHAR(kspcore_last_error()));
}

bool FKspSim::NewSandboxGame(const FString& SaveName)
{
	FKspFpGuard Guard;
	return Handle && kspcore_game_new(Handle, KSPCORE_MODE_SANDBOX, TCHAR_TO_UTF8(*SaveName)) == 1;
}

bool FKspSim::LaunchCraft(const FString& CraftPath, const FString& LaunchSite)
{
	FKspFpGuard Guard;
	return Handle && kspcore_game_launch_craft(Handle, TCHAR_TO_UTF8(*CraftPath), TCHAR_TO_UTF8(*LaunchSite)) == 1;
}

bool FKspSim::SceneLoaded(int32 Scene)
{
	if (!Handle)
	{
		return false;
	}
	FKspFpGuard Guard;
	const bool bOk = kspcore_scene_loaded(Handle, Scene) == 1;
	RefreshViews();
	return bOk;
}

int32 FKspSim::CurrentScene() const
{
	FKspFpGuard Guard;
	return Handle ? kspcore_game_current_scene(Handle) : -1;
}

void FKspSim::SetInput(const FKspInputFrame& Input)
{
	if (!Handle)
	{
		return;
	}
	kspcore_input_frame In;
	FMemory::Memzero(In);
	In.struct_size = sizeof(In);
	In.keys_down = Input.KeysDown.GetData();
	In.keys_down_count = Input.KeysDown.Num();
	In.keys_up = Input.KeysUp.GetData();
	In.keys_up_count = Input.KeysUp.Num();
	In.keys_held = Input.KeysHeld.GetData();
	In.keys_held_count = Input.KeysHeld.Num();
	In.mouse_position[0] = Input.MousePosition.X;
	In.mouse_position[1] = Input.MousePosition.Y;
	In.mouse_delta[0] = Input.MouseDelta.X;
	In.mouse_delta[1] = Input.MouseDelta.Y;
	In.mouse_buttons_held = Input.MouseButtonsHeld;
	In.mouse_buttons_down = Input.MouseButtonsDown;
	In.mouse_buttons_up = Input.MouseButtonsUp;
	In.mouse_wheel = Input.MouseWheel;
	In.screen_width = Input.ScreenWidth;
	In.screen_height = Input.ScreenHeight;
	In.has_focus = Input.bHasFocus ? 1 : 0;
	In.ui_hover_lock = Input.bUiHoverLock ? 1 : 0;
	In.real_time_since_startup = Input.RealTimeSinceStartup;
	FKspFpGuard Guard;
	kspcore_input_set_frame(Handle, &In);
}

bool FKspSim::Frame(double DtReal)
{
	if (!Handle)
	{
		return false;
	}
	FKspFpGuard Guard;
	const bool bOk = kspcore_frame(Handle, DtReal) == 1;
	RefreshViews();
	return bOk;
}

void FKspSim::RefreshViews()
{
	VesselView.Reset();
	PartView.Reset();
	EventView.Reset();
	const kspcore_snapshot_view* Snap = kspcore_snapshot(Handle);
	if (!Snap)
	{
		FrameView = FKspFrameState();
		return;
	}
	const kspcore_frame_state& F = Snap->frame;
	FrameView.Ut = F.ut;
	FrameView.FrameCount = F.frame_count;
	FrameView.FixedSteps = F.fixed_steps;
	FrameView.FixedStepsLast = F.fixed_steps_last;
	FrameView.WarpRate = F.warp_rate;
	FrameView.WarpIndex = F.warp_index;
	FrameView.WarpMode = F.warp_mode;
	FrameView.bPaused = F.paused != 0;
	FrameView.Scene = F.scene;
	FrameView.FrameKind = F.frame_kind;
	FrameView.DominantBodyId = F.dominant_body_id;
	FrameView.FrameChangeSerial = F.frame_change_serial;
	FrameView.ActiveVesselId = F.active_vessel_id;

	VesselView.Reserve(Snap->vessel_count);
	for (uint32 I = 0; I < Snap->vessel_count; ++I)
	{
		const kspcore_vessel_state& V = Snap->vessels[I];
		FKspVesselState& Out = VesselView.AddDefaulted_GetRef();
		Out.Id = V.id;
		Out.Flags = V.flags;
		Out.Situation = V.situation;
		Out.Com = PositionToUe(V.com);
		Out.RootPosition = PositionToUe(V.root_position);
		Out.RootRotation = RotationToUe(V.root_rotation);
		Out.FirstPart = V.first_part;
		Out.PartCount = V.part_count;
		Out.Altitude = V.altitude;
		Out.SurfaceSpeed = V.surface_speed;
		Out.Mach = V.mach;
		Out.VerticalSpeed = V.vertical_speed;
	}
	PartView.Reserve(Snap->part_count);
	for (uint32 I = 0; I < Snap->part_count; ++I)
	{
		const kspcore_part_state& P = Snap->parts[I];
		FKspPartState& Out = PartView.AddDefaulted_GetRef();
		Out.Id = P.id;
		Out.VesselId = P.vessel_id;
		Out.ModelId = P.model_id;
		Out.Flags = P.flags;
		Out.Position = PositionToUe(P.position);
		Out.Rotation = RotationToUe(P.rotation);
	}
	EventView.Reserve(Snap->event_count);
	for (uint32 I = 0; I < Snap->event_count; ++I)
	{
		const kspcore_event& E = Snap->events[I];
		FKspEvent& Out = EventView.AddDefaulted_GetRef();
		Out.Kind = E.kind;
		Out.A = E.a;
		Out.B = E.b;
		Out.Id = E.id;
		FMemory::Memcpy(Out.D, E.d, sizeof(Out.D));
	}
}

const FKspVesselState* FKspSim::ActiveVessel() const
{
	return VesselView.FindByPredicate([](const FKspVesselState& V) { return (V.Flags & KspVesselFlag::Active) != 0; });
}

uint64 FKspSim::SnapshotHash() const
{
	if (!Handle)
	{
		return 0;
	}
	FKspFpGuard Guard;
	const kspcore_snapshot_view* Snap = kspcore_snapshot(Handle);
	return Snap ? kspcore_snapshot_hash(Snap) : 0;
}

void FKspSim::GetPartColliders(uint32 PartId, TArray<FKspColliderProxy>& Out) const
{
	Out.Reset();
	if (!Handle)
	{
		return;
	}
	FKspFpGuard Guard;
	TArray<kspcore_collider_proxy> Raw;
	Raw.SetNumUninitialized(int32(kspcore_part_collider_proxies(Handle, PartId, nullptr, 0)));
	Raw.SetNum(FMath::Min(Raw.Num(), int32(kspcore_part_collider_proxies(Handle, PartId, Raw.GetData(), Raw.Num()))));
	Out.Reserve(Raw.Num());
	for (const kspcore_collider_proxy& C : Raw)
	{
		FKspColliderProxy& P = Out.AddDefaulted_GetRef();
		P.Kind = FKspColliderProxy::EKind(C.kind);
		P.Frame = FTransform(RotationToUe(C.rotation), PositionToUe(C.center));
		// half extents are sizes along the collider frame's axes: axis map and unit scale, no sign
		P.HalfExtents = PositionToUe(C.half_extents);
		P.Radius = C.radius * FKspAxis::CmPerMetre;
		P.HalfHeight = C.half_height * FKspAxis::CmPerMetre;
		P.HullPoints.Reserve(C.hull_point_count);
		for (uint32 I = 0; I < C.hull_point_count; ++I)
		{
			P.HullPoints.Add(PositionToUe(C.hull_points + 3 * I));
		}
	}
}
