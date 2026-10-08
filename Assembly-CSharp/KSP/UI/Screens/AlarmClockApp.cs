using KSP.Localization;
using UnityEngine;

namespace KSP.UI.Screens
{
	public class AlarmClockApp : UIApp
	{
		internal class UsageStats
		{
			internal bool UsageFound => false;

			internal void Reset()
			{
			}
		}

		public static AlarmClockApp Instance;

		public static bool Ready;

		[SerializeField]
		private AlarmClockUIFrame appFramePrefab;

		private AlarmClockUIFrame appFrame;

		private RectTransform appRectTransform;

		[SerializeField]
		private int appWidth = 230;

		private int appHeight = 300;

		internal UsageStats usage;

		protected override void OnAppInitialized()
		{
			if (Instance != null)
			{
				Object.Destroy(this);
				return;
			}
			Instance = this;
			AlarmClockScenario.appInstance = this;
			appFrame = Object.Instantiate(appFramePrefab);
			appFrame.transform.SetParent(base.transform, worldPositionStays: false);
			appFrame.transform.localPosition = Vector3.zero;
			appRectTransform = appFrame.transform as RectTransform;
			appFrame.Setup(base.appLauncherButton, base.name, Localizer.Format("#autoLOC_8003500"), appWidth, appHeight);
			appFrame.AddGlobalInputDelegate(base.MouseInput_PointerEnter, base.MouseInput_PointerExit);
			HideApp();
			if (usage == null)
			{
				usage = new UsageStats();
			}
			Ready = true;
		}

		protected override void OnAppDestroy()
		{
			if (appFrame != null)
			{
				if ((bool)ApplicationLauncher.Instance)
				{
					ApplicationLauncher.Instance.RemoveOnRepositionCallback(appFrame.Reposition);
				}
				appFrame.gameObject.DestroyGameObject();
				if (InputLockManager.GetControlLock("UIAlarmClockApp") != ControlTypes.None)
				{
					RemoveAppFrameInputLock();
				}
			}
			Ready = false;
			if (Instance != null && Instance == this)
			{
				Instance = null;
			}
		}

		protected override bool OnAppAboutToStart()
		{
			if (HighLogic.CurrentGame != null && (HighLogic.CurrentGame.Mode == Game.Modes.MISSION_BUILDER || HighLogic.CurrentGame.Mode == Game.Modes.SCENARIO || HighLogic.CurrentGame.Mode == Game.Modes.SCENARIO_NON_RESUMABLE))
			{
				return false;
			}
			if (HighLogic.LoadedScene != GameScenes.SPACECENTER && HighLogic.LoadedScene != GameScenes.EDITOR && HighLogic.LoadedScene != GameScenes.FLIGHT)
			{
				return HighLogic.LoadedScene == GameScenes.TRACKSTATION;
			}
			return true;
		}

		protected override void DisplayApp()
		{
			if (appFrame != null)
			{
				appFrame.gameObject.SetActive(value: true);
			}
		}

		protected override void HideApp()
		{
			if (appFrame != null && !appFrame.MouseOver)
			{
				appFrame.AddAlarmPaneCloseImmediate();
				appFrame.gameObject.SetActive(value: false);
			}
		}

		protected override ApplicationLauncher.AppScenes GetAppScenes()
		{
			return ApplicationLauncher.AppScenes.ALWAYS;
		}

		protected override Vector3 GetAppScreenPos(Vector3 defaultAnchorPos)
		{
			return defaultAnchorPos;
		}

		private void Start()
		{
		}

		private void Update()
		{
		}

		public static bool AnyTextFieldHasFocus()
		{
			if (Instance == null)
			{
				return false;
			}
			return Instance.TextFieldHasFocus();
		}

		internal static bool AppFrameHasLock()
		{
			return InputLockManager.GetControlLock("UIAlarmClockApp") > ControlTypes.None;
		}

		private void RemoveAppFrameInputLock()
		{
			InputLockManager.RemoveControlLock("UIAlarmClockApp");
		}

		public bool TextFieldHasFocus()
		{
			if (appFrame != null)
			{
				return false;
			}
			return appFrame.AnyTextFieldHasFocus();
		}

		public static bool EditAlarm(AlarmTypeBase alarm)
		{
			if (Instance == null)
			{
				Debug.LogWarning($"[AlarmClockApp]: App is not running at this time - unable to edit alarm id={alarm.Id}");
				return false;
			}
			Instance.DisplayApp();
			if (Instance.appFrame != null)
			{
				Instance.appLauncherButton.SetTrue(makeCall: false);
				return Instance.appFrame.EditAlarm(alarm);
			}
			return false;
		}

		private void OnSceneSwitch(GameEvents.FromToAction<GameScenes, GameScenes> scn)
		{
			if (usage.UsageFound)
			{
				usage.Reset();
			}
		}
	}
}
