using KSP.Localization;
using KSP.UI;
using KSP.UI.Screens;
using UnityEngine;

public class DeltaVApp : UIApp
{
	internal class UsageStats
	{
		public int body;

		public int situation;

		public int atmosphereSlider;

		public int atmosphereText;

		public int displayFields;

		internal bool UsageFound
		{
			get
			{
				if (body <= 0 && situation <= 0 && atmosphereSlider <= 0 && atmosphereText <= 0)
				{
					return displayFields > 0;
				}
				return true;
			}
		}

		internal void Reset()
		{
			body = 0;
			situation = 0;
			atmosphereSlider = 0;
			atmosphereText = 0;
			displayFields = 0;
		}
	}

	public static DeltaVApp Instance;

	public static bool Ready;

	[SerializeField]
	private GenericAppFrame appFramePrefab;

	private GenericAppFrame appFrame;

	[SerializeField]
	private DeltaVAppSituation situationPrefab;

	private DeltaVAppSituation situation;

	[SerializeField]
	private DeltaVAppStageInfo stageInfoPrefab;

	private DeltaVAppStageInfo stageInfo;

	private RectTransform appRectTransform;

	[SerializeField]
	private int appWidth = 230;

	private int appHeight;

	[SerializeField]
	private int appHeightSituation = 161;

	[SerializeField]
	private int appHeightStageInfo = 173;

	[SerializeField]
	private int appStageInfoLineHeight = 20;

	private bool initialHeightSet;

	internal UsageStats usage;

	protected override void OnAppInitialized()
	{
		if (!GameSettings.DELTAV_APP_ENABLED)
		{
			Object.DestroyImmediate(base.gameObject);
			return;
		}
		Instance = this;
		DeltaVAppValues deltaVAppValues = DeltaVGlobals.DeltaVAppValues;
		bool num = situationPrefab != null && HighLogic.LoadedSceneIsEditor;
		bool flag = (stageInfoPrefab != null && HighLogic.LoadedSceneIsEditor) || HighLogic.LoadedSceneIsFlight;
		appHeight = 27;
		if (num)
		{
			appHeight += appHeightSituation;
		}
		if (flag)
		{
			appHeight += appHeightStageInfo;
			int num2 = 7;
			if (deltaVAppValues != null)
			{
				num2 = deltaVAppValues.infoLines.Count;
			}
			if (GameSettings.DELTAV_APP_TWOCOLUMN_MODE)
			{
				appHeight += appStageInfoLineHeight * ((num2 + 1) / 2);
			}
			else
			{
				appHeight += appStageInfoLineHeight * num2;
			}
		}
		appFrame = Object.Instantiate(appFramePrefab);
		appFrame.transform.SetParent(base.transform, worldPositionStays: false);
		appFrame.transform.localPosition = Vector3.zero;
		appRectTransform = appFrame.transform as RectTransform;
		appFrame.Setup(base.appLauncherButton, base.name, Localizer.Format("#autoLOC_8003216"), appWidth, appHeight);
		appFrame.AddGlobalInputDelegate(base.MouseInput_PointerEnter, base.MouseInput_PointerExit);
		if (num)
		{
			situation = Object.Instantiate(situationPrefab);
			situation.transform.SetParent(appFrame.scrollList.transform, worldPositionStays: false);
		}
		if (flag)
		{
			stageInfo = Object.Instantiate(stageInfoPrefab);
			stageInfo.transform.SetParent(appFrame.scrollList.transform, worldPositionStays: false);
			stageInfo.infoLineHeight = appStageInfoLineHeight;
		}
		HideApp();
		if (usage == null)
		{
			usage = new UsageStats();
		}
		Ready = true;
		GameEvents.onGameSceneSwitchRequested.Add(OnSceneSwitch);
		GameEvents.onGUIDeltaVAppReady.Fire();
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
		}
		Ready = false;
		GameEvents.onGameSceneSwitchRequested.Remove(OnSceneSwitch);
		GameEvents.onGUIDeltaVAppDestroy.Fire();
		if (Instance != null && Instance == this)
		{
			Instance = null;
		}
	}

	protected override bool OnAppAboutToStart()
	{
		if (!HighLogic.LoadedSceneIsEditor)
		{
			return HighLogic.LoadedSceneIsFlight;
		}
		return true;
	}

	protected override void DisplayApp()
	{
		if (appFrame != null)
		{
			if (!initialHeightSet)
			{
				appRectTransform.sizeDelta = new Vector2(appWidth, appHeight);
				initialHeightSet = true;
			}
			if (stageInfo != null)
			{
				stageInfo.SetColumnLayout(GameSettings.DELTAV_APP_TWOCOLUMN_MODE);
			}
			appFrame.gameObject.SetActive(value: true);
		}
	}

	protected override void HideApp()
	{
		if (appFrame != null && (situation == null || !situation.IsBodyDropDownExpanded))
		{
			appFrame.gameObject.SetActive(value: false);
		}
	}

	protected override ApplicationLauncher.AppScenes GetAppScenes()
	{
		return ApplicationLauncher.AppScenes.FLIGHT | ApplicationLauncher.AppScenes.VAB | ApplicationLauncher.AppScenes.SPH;
	}

	protected override Vector3 GetAppScreenPos(Vector3 defaultAnchorPos)
	{
		return defaultAnchorPos;
	}

	public static bool AnyTextFieldHasFocus()
	{
		if (Instance == null)
		{
			return false;
		}
		return Instance.TextFieldHasFocus();
	}

	public bool TextFieldHasFocus()
	{
		if (situation == null)
		{
			return false;
		}
		return situation.AltitudeHasFocus();
	}

	private void OnSceneSwitch(GameEvents.FromToAction<GameScenes, GameScenes> scn)
	{
		if ((scn.from == GameScenes.EDITOR || scn.from == GameScenes.FLIGHT) && usage.UsageFound)
		{
			AnalyticsUtil.DeltaVAppUsage(HighLogic.CurrentGame, scn.from, usage);
			usage.Reset();
		}
	}
}
