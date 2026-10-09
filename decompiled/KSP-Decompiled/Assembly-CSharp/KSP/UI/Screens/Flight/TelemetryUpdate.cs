using System;
using CommNet;
using KSP.Localization;
using KSP.UI.TooltipTypes;
using UnityEngine;
using UnityEngine.UI;

namespace KSP.UI.Screens.Flight
{
	public class TelemetryUpdate : MonoBehaviour
	{
		public CommNetUIModeButton modeButton;

		public Sprite NOSIG;

		public Sprite NOEP;

		public Sprite BLK;

		public Sprite AUP;

		public Sprite ADN;

		public Sprite EP0;

		public Sprite EP1;

		public Sprite EP2;

		public Sprite CK1;

		public Sprite CK2;

		public Sprite CK3;

		public Sprite CP1;

		public Sprite CP2;

		public Sprite CP3;

		public Sprite SS0;

		public Sprite SS1;

		public Sprite SS2;

		public Sprite SS3;

		public Sprite SS4;

		public Image arrow_icon;

		public Image firstHop_icon;

		public Image lastHop_icon;

		public Image control_icon;

		public Image signal_icon;

		public TooltipController_Text firstHop_tooltip;

		public TooltipController_Text arrow_tooltip;

		public TooltipController_Text lastHop_tooltip;

		public TooltipController_Text control_tooltip;

		public TooltipController_SignalStrength signal_tooltip;

		private static string cacheAutoLOC_461342;

		private static string cacheAutoLOC_461346;

		private static string cacheAutoLOC_461350;

		private static string cacheAutoLOC_461354;

		private static string cacheAutoLOC_461358;

		private static string cacheAutoLOC_461362;

		private static string cacheAutoLOC_461366;

		private static string cacheAutoLOC_461381;

		private static string cacheAutoLOC_461382;

		private static string cacheAutoLOC_461402;

		private static string cacheAutoLOC_461407;

		private static string cacheAutoLOC_461418;

		private static string cacheAutoLOC_461422;

		private static string cacheAutoLOC_461426;

		private static string cacheAutoLOC_461436;

		private static string cacheAutoLOC_461442;

		private static string cacheAutoLOC_461446;

		public static TelemetryUpdate Instance { get; protected set; }

		protected virtual void Start()
		{
			if (!CommNetScenario.CommNetEnabled)
			{
				base.gameObject.SetActive(value: false);
			}
		}

		protected virtual void Awake()
		{
			if (Instance == null)
			{
				Instance = this;
				HideModeButton();
			}
			else
			{
				UnityEngine.Object.DestroyImmediate(this);
			}
		}

		protected virtual void OnDestroy()
		{
			if (Instance != null && Instance == this)
			{
				Instance = null;
			}
		}

		protected virtual void Update()
		{
			if (MapView.MapIsEnabled)
			{
				ShowModeButton();
			}
			else
			{
				HideModeButton();
			}
			Vessel activeVessel = FlightGlobals.ActiveVessel;
			if (activeVessel == null)
			{
				return;
			}
			if (activeVessel.isEVA)
			{
				SetEVAUI(activeVessel);
				return;
			}
			if (activeVessel.connection == null)
			{
				ClearGui();
				return;
			}
			Sprite sprite = BLK;
			Sprite sprite2 = BLK;
			Sprite sprite3 = BLK;
			Sprite sprite4 = BLK;
			Sprite sprite5 = BLK;
			if (Time.timeSinceLevelLoad > 1f)
			{
				switch (activeVessel.connection.ControlState)
				{
				case VesselControlState.ProbePartial:
					sprite = CP2;
					control_tooltip.textString = cacheAutoLOC_461362;
					break;
				case VesselControlState.None:
					sprite = BLK;
					control_tooltip.textString = cacheAutoLOC_461342;
					break;
				case VesselControlState.Probe:
					sprite = CP1;
					control_tooltip.textString = cacheAutoLOC_461358;
					break;
				case VesselControlState.Kerbal:
					sprite = CK1;
					control_tooltip.textString = cacheAutoLOC_461346;
					break;
				case VesselControlState.KerbalFull:
					sprite = CK3;
					control_tooltip.textString = cacheAutoLOC_461354;
					break;
				case VesselControlState.ProbeFull:
					sprite = CP3;
					control_tooltip.textString = cacheAutoLOC_461366;
					break;
				case VesselControlState.KerbalPartial:
					sprite = CK2;
					control_tooltip.textString = cacheAutoLOC_461350;
					break;
				}
				if (activeVessel.connection.ControlState != VesselControlState.Kerbal && activeVessel.connection.ControlState != VesselControlState.Probe && activeVessel.connection.ControlState != 0)
				{
					switch (activeVessel.connection.Signal)
					{
					case SignalStrength.None:
						sprite4 = SS0;
						sprite5 = NOSIG;
						sprite3 = NOEP;
						arrow_tooltip.textString = cacheAutoLOC_461381;
						lastHop_tooltip.textString = cacheAutoLOC_461382;
						break;
					case SignalStrength.Red:
						sprite4 = SS1;
						break;
					case SignalStrength.Orange:
						sprite4 = SS2;
						break;
					case SignalStrength.Yellow:
						sprite4 = SS3;
						break;
					case SignalStrength.Green:
						sprite4 = SS4;
						break;
					}
					if (activeVessel.connection.Signal != 0)
					{
						if (activeVessel.Connection.CanScience)
						{
							arrow_tooltip.textString = cacheAutoLOC_461402;
							sprite5 = AUP;
						}
						else
						{
							arrow_tooltip.textString = cacheAutoLOC_461407;
							sprite5 = ADN;
						}
					}
					if (activeVessel.connection.IsConnected)
					{
						switch (activeVessel.connection.ControlPath.Last.hopType)
						{
						case HopType.Relay:
							sprite3 = EP0;
							lastHop_tooltip.textString = cacheAutoLOC_461418;
							break;
						case HopType.ControlPoint:
							sprite3 = EP1;
							lastHop_tooltip.textString = cacheAutoLOC_461422;
							break;
						case HopType.Home:
							sprite3 = EP2;
							lastHop_tooltip.textString = cacheAutoLOC_461426;
							break;
						}
						if (activeVessel.connection.ControlPath.Last.hopType != activeVessel.connection.ControlPath.First.hopType)
						{
							switch (activeVessel.connection.ControlPath.First.hopType)
							{
							case HopType.Relay:
								sprite2 = EP0;
								firstHop_tooltip.textString = cacheAutoLOC_461436;
								break;
							case HopType.ControlPoint:
								sprite2 = EP1;
								firstHop_tooltip.textString = cacheAutoLOC_461442;
								break;
							case HopType.Home:
								sprite2 = EP2;
								firstHop_tooltip.textString = cacheAutoLOC_461446;
								break;
							}
							firstHop_tooltip.textString += $" ({Math.Ceiling(activeVessel.connection.ControlPath.First.signalStrength * 100.0) * 0.01:0%})";
						}
					}
				}
			}
			SetIcon(arrow_icon, sprite5);
			SetIcon(firstHop_icon, sprite2);
			SetIcon(lastHop_icon, sprite3);
			SetIcon(control_icon, sprite);
			SetIcon(signal_icon, sprite4);
		}

		protected virtual void ClearGui()
		{
			SetIcon(arrow_icon, BLK);
			SetIcon(firstHop_icon, BLK);
			SetIcon(lastHop_icon, BLK);
			SetIcon(control_icon, BLK);
			SetIcon(signal_icon, BLK);
		}

		protected virtual void SetIcon(Image image, Sprite sprite, bool disableOnBlank = true)
		{
			if (sprite == BLK)
			{
				if (disableOnBlank && image.gameObject.activeSelf)
				{
					image.gameObject.SetActive(value: false);
				}
				return;
			}
			if (disableOnBlank && !image.gameObject.activeSelf)
			{
				image.gameObject.SetActive(value: true);
			}
			image.sprite = sprite;
		}

		protected virtual void SetEVAUI(Vessel v)
		{
			SetIcon(arrow_icon, BLK);
			SetIcon(firstHop_icon, BLK);
			SetIcon(lastHop_icon, BLK);
			SetIcon(control_icon, CK3);
			SetIcon(signal_icon, BLK);
			control_tooltip.textString = Localizer.Format("#autoLOC_445419");
		}

		protected virtual void ShowModeButton()
		{
			if (!modeButton.gameObject.activeSelf)
			{
				modeButton.gameObject.SetActive(value: true);
			}
		}

		protected virtual void HideModeButton()
		{
			if (modeButton.gameObject.activeSelf)
			{
				modeButton.gameObject.SetActive(value: false);
			}
		}

		internal static void CacheLocalStrings()
		{
			cacheAutoLOC_461342 = Localizer.Format("#autoLOC_461342");
			cacheAutoLOC_461346 = Localizer.Format("#autoLOC_461346");
			cacheAutoLOC_461350 = Localizer.Format("#autoLOC_461350");
			cacheAutoLOC_461354 = Localizer.Format("#autoLOC_461354");
			cacheAutoLOC_461358 = Localizer.Format("#autoLOC_461358");
			cacheAutoLOC_461362 = Localizer.Format("#autoLOC_461362");
			cacheAutoLOC_461366 = Localizer.Format("#autoLOC_461366");
			cacheAutoLOC_461381 = Localizer.Format("#autoLOC_461381");
			cacheAutoLOC_461382 = Localizer.Format("#autoLOC_461382");
			cacheAutoLOC_461402 = Localizer.Format("#autoLOC_461402");
			cacheAutoLOC_461407 = Localizer.Format("#autoLOC_461407");
			cacheAutoLOC_461418 = Localizer.Format("#autoLOC_461418");
			cacheAutoLOC_461422 = Localizer.Format("#autoLOC_461422");
			cacheAutoLOC_461426 = Localizer.Format("#autoLOC_461426");
			cacheAutoLOC_461436 = Localizer.Format("#autoLOC_461436");
			cacheAutoLOC_461442 = Localizer.Format("#autoLOC_461442");
			cacheAutoLOC_461446 = Localizer.Format("#autoLOC_461446");
		}
	}
}
