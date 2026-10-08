using KSP.Localization;
using KSP.UI.Screens.Flight;
using UnityEngine;

public class FlightOverlays : MonoBehaviour
{
	private ScreenMessage statusMessage;

	private static string cacheAutoLOC_135685;

	private static string cacheAutoLOC_135690;

	private static string cacheAutoLOC_135693;

	private static string cacheAutoLOC_135696;

	private static string cacheAutoLOC_135699;

	private static string cacheAutoLOC_6003108;

	private static string cacheAutoLOC_6003106;

	private static string cacheAutoLOC_6001071;

	private static string cacheAutoLOC_6001072;

	private static string cacheAutoLOC_6003107;

	private void Start()
	{
		statusMessage = new ScreenMessage("", 4f, ScreenMessageStyle.LOWER_CENTER);
		PhysicsGlobals.AeroForceDisplay = false;
		PhysicsGlobals.ThermalColorsDebug = false;
	}

	private void Update()
	{
		if (!FlightGlobals.ready || !HighLogic.LoadedSceneIsFlight || CameraManager.Instance.currentCameraMode != 0)
		{
			return;
		}
		if (GameSettings.TOGGLE_TEMP_GAUGES.GetKeyDown() && !GameSettings.MODIFIER_KEY.GetKey())
		{
			if (TemperatureGaugeSystem.Instance != null)
			{
				GameSettings.TEMPERATURE_GAUGES_MODE++;
				if (GameSettings.TEMPERATURE_GAUGES_MODE > 3)
				{
					GameSettings.TEMPERATURE_GAUGES_MODE = 0;
				}
				GameSettings.SaveSettings();
				string text = cacheAutoLOC_135685;
				switch (GameSettings.TEMPERATURE_GAUGES_MODE)
				{
				default:
					text += cacheAutoLOC_135690;
					break;
				case 1:
					text += cacheAutoLOC_135693;
					break;
				case 2:
					text += cacheAutoLOC_135696;
					break;
				case 3:
					text += cacheAutoLOC_135699;
					break;
				}
				ScreenMessages.PostScreenMessage(text, statusMessage);
			}
			else
			{
				ScreenMessages.PostScreenMessage(cacheAutoLOC_6003108, statusMessage);
			}
		}
		if (GameSettings.TOGGLE_TEMP_OVERLAY.GetKeyDown() && !GameSettings.MODIFIER_KEY.GetKey())
		{
			for (int i = 0; i < FlightGlobals.ActiveVessel.parts.Count; i++)
			{
				FlightGlobals.ActiveVessel.parts[i].ResetMPB();
			}
			PhysicsGlobals.ThermalColorsDebug = !PhysicsGlobals.ThermalColorsDebug;
			ScreenMessages.PostScreenMessage(cacheAutoLOC_6003106 + (PhysicsGlobals.ThermalColorsDebug ? cacheAutoLOC_6001072 : cacheAutoLOC_6001071), statusMessage);
		}
		if (GameSettings.TOGGLE_FLIGHT_FORCES.GetKeyDown() && !GameSettings.MODIFIER_KEY.GetKey())
		{
			PhysicsGlobals.AeroForceDisplay = !PhysicsGlobals.AeroForceDisplay;
			ScreenMessages.PostScreenMessage(cacheAutoLOC_6003107 + (PhysicsGlobals.AeroForceDisplay ? cacheAutoLOC_6001072 : cacheAutoLOC_6001071), statusMessage);
		}
	}

	internal static void CacheLocalStrings()
	{
		cacheAutoLOC_135685 = Localizer.Format("#autoLOC_135685");
		cacheAutoLOC_135690 = Localizer.Format("#autoLOC_135690");
		cacheAutoLOC_135693 = Localizer.Format("#autoLOC_135693");
		cacheAutoLOC_135696 = Localizer.Format("#autoLOC_135696");
		cacheAutoLOC_135699 = Localizer.Format("#autoLOC_135699");
		cacheAutoLOC_6003108 = "<color=" + XKCDColors.HexFormat.KSPNotSoGoodOrange + ">" + Localizer.Format("#autoLOC_6003108") + "</color>";
		cacheAutoLOC_6003106 = Localizer.Format("#autoLOC_6003106") + ": ";
		cacheAutoLOC_6003107 = Localizer.Format("#autoLOC_6003107") + ": ";
		cacheAutoLOC_6001071 = Localizer.Format("#autoLOC_6001071");
		cacheAutoLOC_6001072 = Localizer.Format("#autoLOC_6001072");
	}
}
