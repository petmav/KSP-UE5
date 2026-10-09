using UnityEngine;
using UnityEngine.UI;

namespace KSP.UI.Screens.Flight
{
	public class AltitudeTumbler : MonoBehaviour
	{
		internal class UsageStats
		{
			public int aglSelected;

			public int aslSelected;

			internal bool UsageFound
			{
				get
				{
					if (aglSelected <= 0)
					{
						return aslSelected > 0;
					}
					return true;
				}
			}

			internal void Reset()
			{
				aglSelected = 0;
				aslSelected = 0;
			}
		}

		public static AltitudeTumbler Instance;

		public Tumbler tumbler;

		public static double altitudeMultiplier = 1.0;

		public Button altitudeModeBtn;

		private AltimeterDisplayState aglMode;

		public SpriteTumblerObject modeTumbler;

		public float modeTumblerSharpness = 6f;

		private float lastUpdateTime;

		internal UsageStats usage;

		public AltimeterDisplayState CurrentMode => aglMode;

		private void Awake()
		{
			if (Instance != null && Instance != this)
			{
				Object.Destroy(this);
			}
			Instance = this;
			lastUpdateTime = Time.realtimeSinceStartup;
			usage = new UsageStats();
			SetTumblerPosition();
		}

		private void SetTumblerPosition()
		{
			base.transform.localPosition = new Vector3(base.transform.localPosition.x, base.transform.localPosition.y, -650f * Mathf.Pow(GameSettings.UI_SCALE, -1.077322f));
		}

		private void Start()
		{
			altitudeModeBtn.onClick.AddListener(ChangeAltimeterMode);
			GameEvents.onVesselChange.Add(OnVesselChange);
			if (FlightGlobals.ActiveVessel != null)
			{
				aglMode = FlightGlobals.ActiveVessel.altimeterDisplayState;
			}
			else
			{
				aglMode = AltimeterDisplayState.ASL;
			}
			SetModeTumbler(aglMode);
			GameEvents.onGameSceneSwitchRequested.Add(OnSceneSwitch);
			GameEvents.OnMapEntered.Add(OnMapEntered);
			GameEvents.onUIScaleChange.Add(SetTumblerPosition);
		}

		private void OnDestroy()
		{
			GameEvents.onVesselChange.Remove(OnVesselChange);
			GameEvents.onGameSceneSwitchRequested.Remove(OnSceneSwitch);
			GameEvents.OnMapEntered.Remove(OnMapEntered);
			GameEvents.onUIScaleChange.Remove(SetTumblerPosition);
		}

		private void OnVesselChange(Vessel vsl)
		{
			aglMode = vsl.altimeterDisplayState;
			SetModeTumbler(aglMode);
		}

		private void ChangeAltimeterMode()
		{
			switch (aglMode)
			{
			case AltimeterDisplayState.AGL:
				aglMode = AltimeterDisplayState.ASL;
				usage.aslSelected++;
				break;
			case AltimeterDisplayState.DEFAULT:
			case AltimeterDisplayState.ASL:
				aglMode = AltimeterDisplayState.AGL;
				usage.aglSelected++;
				break;
			}
			SetModeTumbler(aglMode);
		}

		public void SetModeTumbler(AltimeterDisplayState mode)
		{
			aglMode = mode;
			if (aglMode == AltimeterDisplayState.DEFAULT)
			{
				aglMode = AltimeterDisplayState.ASL;
			}
			if (FlightGlobals.ActiveVessel != null)
			{
				FlightGlobals.ActiveVessel.altimeterDisplayState = aglMode;
			}
			if (modeTumbler != null)
			{
				if (aglMode == AltimeterDisplayState.AGL)
				{
					modeTumbler.TumbleTo(0.0, 1);
				}
				else
				{
					modeTumbler.TumbleTo(1.0, 0);
				}
			}
			GameEvents.OnAltimeterDisplayModeToggle.Fire(aglMode);
		}

		private void Reset()
		{
			tumbler = GetComponent<Tumbler>();
		}

		private void LateUpdate()
		{
			if (modeTumbler != null)
			{
				float deltaTime = Time.realtimeSinceStartup - lastUpdateTime;
				lastUpdateTime = Time.realtimeSinceStartup;
				modeTumbler.UpdateDelta(deltaTime, modeTumblerSharpness);
			}
			if (FlightGlobals.ready && (object)tumbler != null)
			{
				if (aglMode == AltimeterDisplayState.AGL)
				{
					tumbler.SetValue(FlightGlobals.ActiveVessel.radarAltitude);
				}
				else
				{
					tumbler.SetValue(FlightGlobals.ActiveVessel.altitude * altitudeMultiplier);
				}
			}
		}

		private void OnSceneSwitch(GameEvents.FromToAction<GameScenes, GameScenes> scn)
		{
			if (!MapView.MapIsEnabled && usage.UsageFound)
			{
				AnalyticsUtil.AltimeterModeUsage(HighLogic.CurrentGame, usage);
				usage.Reset();
			}
		}

		private void OnMapEntered()
		{
			if (usage.UsageFound)
			{
				AnalyticsUtil.AltimeterModeUsage(HighLogic.CurrentGame, usage);
				usage.Reset();
			}
		}
	}
}
