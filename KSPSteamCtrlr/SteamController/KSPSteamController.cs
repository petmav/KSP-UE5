using System;
using System.Collections;
using KSP.UI.Screens.Flight;
using Steamworks;
using UnityEngine;

namespace SteamController
{
	[KSPAddon(KSPAddon.Startup.Instantly, true)]
	public class KSPSteamController : MonoBehaviour
	{
		public enum KSPControllerModes
		{
			Menu,
			Flight,
			Docking,
			Editor,
			EVA,
			Map
		}

		private ControllerHandle_t[] ctrlHandles = new ControllerHandle_t[16];

		private KSPControllerModes modePrev;

		private ControllerActionSetHandle_t menuCtrls;

		private ControllerActionSetHandle_t flightCtrls;

		private ControllerActionSetHandle_t dockingCtrls;

		private ControllerActionSetHandle_t editorCtrls;

		private ControllerActionSetHandle_t mapCtrls;

		private ControllerActionSetHandle_t EVACtrls;

		private bool steamInitialized;

		private bool steamServerInitialized;

		private float updateInterval = 1f;

		private WaitForSeconds updateYield;

		private bool haveController;

		private bool ctrlFound;

		private ScreenMessage statusMessage;

		private Coroutine sch_setModeCoroutine;

		private int fCount;

		private ControllerHandle_t ctrl => ctrlHandles[0];

		protected void Awake()
		{
			UnityEngine.Object.DontDestroyOnLoad(this);
		}

		protected void Start()
		{
			steamInitialized = SteamManager.Initialized;
			if (steamInitialized)
			{
				Debug.Log("[KSPSteamController]: Loaded. Initializing hooks...");
				SteamAPI.RunCallbacks();
				statusMessage = new ScreenMessage(string.Empty, 5f, ScreenMessageStyle.UPPER_RIGHT);
				GameEvents.onLevelWasLoaded.Add(onLevelWasLoaded);
				GameEvents.onGamePause.Add(onGamePause);
				GameEvents.onGameUnpause.Add(onGameUnpause);
				GameEvents.OnFlightUIModeChanged.Add(OnFlightUIModeChanged);
				GameEvents.OnMapEntered.Add(OnEnterMapView);
				GameEvents.onVesselChange.Add(OnVesselChange);
				Steamworks.SteamController.Init();
				modePrev = KSPControllerModes.Menu;
				updateYield = new WaitForSeconds(updateInterval);
				StartCoroutine(ControllerWatchDaemon());
			}
			else if (Versioning.IsSteam)
			{
				if (!Packsize.Test())
				{
					Debug.LogError("[KSPSteamController] Packsize Test returned false, the wrong version of Steamworks.NET is being run in this platform.", this);
				}
				if (!DllCheck.Test())
				{
					Debug.LogError("[KSPSteamController] DllCheck Test returned false, One or more of the Steamworks binaries seems to be the wrong version.", this);
				}
				Debug.LogError("<color=orange><b>[KSPSteamController]:</b> Failed to initialize Steam API!</color>");
			}
		}

		protected void OnDestroy()
		{
			if (steamInitialized)
			{
				GameEvents.onLevelWasLoaded.Remove(onLevelWasLoaded);
				GameEvents.onGamePause.Remove(onGamePause);
				GameEvents.onGameUnpause.Remove(onGameUnpause);
				GameEvents.OnFlightUIModeChanged.Remove(OnFlightUIModeChanged);
				GameEvents.OnMapEntered.Remove(OnEnterMapView);
				GameEvents.onVesselChange.Remove(OnVesselChange);
			}
		}

		protected IEnumerator ControllerWatchDaemon()
		{
			yield return new WaitForSeconds(15f);
			while (true)
			{
				yield return updateYield;
				Steamworks.SteamController.RunFrame();
				haveController = Steamworks.SteamController.GetConnectedControllers(ctrlHandles) > 0;
				if (haveController && !ctrlFound)
				{
					Debug.Log("[KSPSteamController]: Controller connected");
					SetupCtrlSetHandles();
					StartCoroutine(WelcomePulses());
					ctrlFound = true;
				}
				if (!haveController && ctrlFound)
				{
					Debug.Log("[KSPSteamController]: Controller disconnected");
					ctrlFound = false;
				}
			}
		}

		protected void SetupCtrlSetHandles()
		{
			menuCtrls = SetupCtrlSetHandle("MenuControls");
			flightCtrls = SetupCtrlSetHandle("FlightControls");
			dockingCtrls = SetupCtrlSetHandle("DockingControls");
			editorCtrls = SetupCtrlSetHandle("EditorControls");
			mapCtrls = SetupCtrlSetHandle("MapControls");
			EVACtrls = SetupCtrlSetHandle("EVAControls");
		}

		protected ControllerActionSetHandle_t SetupCtrlSetHandle(string ctrlName)
		{
			ControllerActionSetHandle_t actionSetHandle = Steamworks.SteamController.GetActionSetHandle(ctrlName);
			if (actionSetHandle.m_ControllerActionSetHandle == 0L)
			{
				Debug.LogError("[KSPSteamController]: Action set handle for " + ctrlName + " has &0. That's probably not right.");
			}
			return actionSetHandle;
		}

		protected IEnumerator WelcomePulses()
		{
			for (int i = 0; i < 2; i++)
			{
				Steamworks.SteamController.TriggerHapticPulse(ctrl, Steamworks.ESteamControllerPad.k_ESteamControllerPad_Left, 32768);
				Steamworks.SteamController.TriggerHapticPulse(ctrl, Steamworks.ESteamControllerPad.k_ESteamControllerPad_Right, ushort.MaxValue);
				yield return new WaitForSeconds(0.1f);
				Steamworks.SteamController.TriggerHapticPulse(ctrl, Steamworks.ESteamControllerPad.k_ESteamControllerPad_Left, ushort.MaxValue);
				Steamworks.SteamController.TriggerHapticPulse(ctrl, Steamworks.ESteamControllerPad.k_ESteamControllerPad_Right, 32768);
			}
		}

		protected void onLevelWasLoaded(GameScenes scn)
		{
			if (!haveController)
			{
				return;
			}
			if (scn == GameScenes.FLIGHT)
			{
				StartCoroutine(CallbackUtil.WaitUntil(() => FlightGlobals.ready, delegate
				{
					NavBallToggle.Instance.onManeuverModeToggle += OnManeuverModeToggled;
				}));
			}
			ScheduleSetModeForCurrentContext();
		}

		protected void OnManeuverModeToggled(bool toggleState)
		{
			if (haveController)
			{
				ScheduleSetModeForCurrentContext();
			}
		}

		protected void OnFlightUIModeChanged(FlightUIMode mode)
		{
			if (haveController)
			{
				ScheduleSetModeForCurrentContext();
			}
		}

		protected void OnVesselChange(Vessel ves)
		{
			if (haveController)
			{
				ScheduleSetModeForCurrentContext();
			}
		}

		private void OnTextFieldFocused()
		{
			if (!haveController)
			{
				return;
			}
			Debug.Log("[Steam Controller]: On-screen keyboard requested");
			try
			{
				SteamGameServerUtils.ShowGamepadTextInput(EGamepadTextInputMode.k_EGamepadTextInputModeNormal, EGamepadTextInputLineMode.k_EGamepadTextInputLineModeSingleLine, "Enter text", 64u, "");
			}
			catch (InvalidOperationException ex)
			{
				statusMessage.message = "<b>[Steam Controller]:</b> Click the left analog stick to bring up the on-screen keyboard.";
				Debug.Log("[Steam Controller]: Couldn't automatically summon the keyboard: " + ex.Message + ". SteamServer initialized is " + steamServerInitialized);
				ScreenMessages.PostScreenMessage(statusMessage);
			}
		}

		protected void onGamePause()
		{
			if (haveController)
			{
				StopScheduledSetMode();
				SetControllerMode(KSPControllerModes.Menu);
			}
		}

		protected void OnEnterMapView()
		{
			if (haveController)
			{
				SetControllerMode(KSPControllerModes.Map);
			}
		}

		protected void onGameUnpause()
		{
			if (haveController)
			{
				SetControllerMode(GetModeForCurrentContext());
			}
		}

		protected void ScheduleSetModeForCurrentContext()
		{
			fCount = Time.frameCount;
			if (sch_setModeCoroutine == null)
			{
				sch_setModeCoroutine = StartCoroutine(schedule_SetModeForCurrentContext());
			}
		}

		private IEnumerator schedule_SetModeForCurrentContext()
		{
			while (Time.frameCount - fCount < 10)
			{
				yield return null;
			}
			SetControllerMode(GetModeForCurrentContext());
			sch_setModeCoroutine = null;
		}

		protected void StopScheduledSetMode()
		{
			if (sch_setModeCoroutine != null)
			{
				StopCoroutine(sch_setModeCoroutine);
				sch_setModeCoroutine = null;
			}
		}

		protected KSPControllerModes GetModeForCurrentContext()
		{
			if (HighLogic.LoadedSceneIsFlight)
			{
				if (FlightDriver.Pause)
				{
					return KSPControllerModes.Menu;
				}
				switch (FlightUIModeController.Instance.Mode)
				{
				case FlightUIMode.STAGING:
					if (FlightGlobals.ActiveVessel.isEVA)
					{
						return KSPControllerModes.EVA;
					}
					return KSPControllerModes.Flight;
				case FlightUIMode.DOCKING:
					if (FlightGlobals.ActiveVessel.isEVA)
					{
						return KSPControllerModes.EVA;
					}
					return KSPControllerModes.Docking;
				case FlightUIMode.MAPMODE:
					if (!MapView.MapIsEnabled && NavBallToggle.Instance.ManeuverModeActive)
					{
						return KSPControllerModes.Flight;
					}
					return KSPControllerModes.Map;
				}
			}
			if (HighLogic.LoadedScene == GameScenes.TRACKSTATION)
			{
				return KSPControllerModes.Map;
			}
			if (HighLogic.LoadedSceneIsEditor)
			{
				return KSPControllerModes.Editor;
			}
			if (HighLogic.LoadedScene == GameScenes.MISSIONBUILDER)
			{
				return KSPControllerModes.Editor;
			}
			return KSPControllerModes.Menu;
		}

		protected void SetControllerMode(KSPControllerModes m)
		{
			if (haveController)
			{
				switch (m)
				{
				case KSPControllerModes.Menu:
					Steamworks.SteamController.ActivateActionSet(ctrl, menuCtrls);
					break;
				case KSPControllerModes.Flight:
					Steamworks.SteamController.ActivateActionSet(ctrl, flightCtrls);
					break;
				case KSPControllerModes.Docking:
					Steamworks.SteamController.ActivateActionSet(ctrl, dockingCtrls);
					break;
				case KSPControllerModes.Editor:
					Steamworks.SteamController.ActivateActionSet(ctrl, editorCtrls);
					break;
				case KSPControllerModes.Map:
					Steamworks.SteamController.ActivateActionSet(ctrl, mapCtrls);
					break;
				case KSPControllerModes.EVA:
					Steamworks.SteamController.ActivateActionSet(ctrl, EVACtrls);
					break;
				}
				if (m != modePrev)
				{
					statusMessage.message = "<b>[Steam Controller]:</b> " + m.GetModeName() + ".";
					ScreenMessages.PostScreenMessage(statusMessage);
				}
				modePrev = m;
			}
		}
	}
}
