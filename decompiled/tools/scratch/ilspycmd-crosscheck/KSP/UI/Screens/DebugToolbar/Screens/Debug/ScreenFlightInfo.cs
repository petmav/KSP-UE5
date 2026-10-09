using KSP.Localization;
using TMPro;
using UnityEngine;

namespace KSP.UI.Screens.DebugToolbar.Screens.Debug
{
	public class ScreenFlightInfo : MonoBehaviour
	{
		[SerializeField]
		private TextMeshProUGUI uT_text;

		[SerializeField]
		private TextMeshProUGUI physicsTimeRatio_text;

		[SerializeField]
		private TextMeshProUGUI latitude_txt;

		[SerializeField]
		private TextMeshProUGUI longitude_text;

		[SerializeField]
		private TextMeshProUGUI altitude_text;

		[SerializeField]
		private TextMeshProUGUI radar_altitude_text;

		[SerializeField]
		private TextMeshProUGUI biomeName_text;

		private Vessel currentActiveVessel;

		[SerializeField]
		private TextMeshProUGUI activeVesselName_text;

		[SerializeField]
		private TextMeshProUGUI refBody_text;

		[SerializeField]
		private TextMeshProUGUI frameOfReference_text;

		[SerializeField]
		private GameObject[] EVA_parameters;

		[SerializeField]
		private TextMeshProUGUI slopeAngle_text;

		[SerializeField]
		private float UpdateUISeconds = 0.1f;

		private float lastRealUITime;

		private bool gamePaused;

		private void Awake()
		{
			GameEvents.onVesselChange.Add(GetActiveVesselName);
			GameEvents.onGamePause.Add(onGamePause);
			GameEvents.onGameUnpause.Add(onGameUnPause);
		}

		private void Start()
		{
			if (!currentActiveVessel && (bool)FlightGlobals.ActiveVessel)
			{
				GetActiveVesselName(FlightGlobals.ActiveVessel);
			}
			else
			{
				activeVesselName_text.text = Localizer.Format("#autoLOC_901099");
			}
		}

		private void LateUpdate()
		{
			if (Time.realtimeSinceStartup - lastRealUITime > UpdateUISeconds)
			{
				SetPhysics();
				SetLocation(currentActiveVessel != null);
				SetVessel(currentActiveVessel != null);
				SetVesselSlopeAngle(currentActiveVessel != null);
				lastRealUITime = Time.realtimeSinceStartup;
			}
		}

		private void FixedUpdate()
		{
		}

		private void OnDestroy()
		{
			GameEvents.onVesselChange.Remove(GetActiveVesselName);
			GameEvents.onGamePause.Remove(onGamePause);
			GameEvents.onGameUnpause.Remove(onGameUnPause);
		}

		private void GetActiveVesselName(Vessel data)
		{
			currentActiveVessel = data.GetVessel();
		}

		private void onGamePause()
		{
			gamePaused = true;
		}

		private void onGameUnPause()
		{
			gamePaused = false;
		}

		private void SetUT(bool active)
		{
		}

		private void SetLocation(bool active)
		{
			if (active)
			{
				latitude_txt.text = currentActiveVessel.latitude.ToString("0.000000") ?? "";
				longitude_text.text = currentActiveVessel.longitude.ToString("0.000000") ?? "";
				altitude_text.text = currentActiveVessel.altitude.ToString("0.00") ?? "";
				radar_altitude_text.text = currentActiveVessel.radarAltitude.ToString("0.00") ?? "";
				string text = "";
				if (currentActiveVessel.displaylandedAt != string.Empty)
				{
					biomeName_text.text = Localizer.Format(currentActiveVessel.displaylandedAt);
					return;
				}
				text = ((!(currentActiveVessel.landedAt != string.Empty)) ? ScienceUtil.GetExperimentBiome(currentActiveVessel.mainBody, currentActiveVessel.latitude, currentActiveVessel.longitude) : Vessel.GetLandedAtString(currentActiveVessel.landedAt));
				biomeName_text.text = ScienceUtil.GetBiomedisplayName(currentActiveVessel.mainBody, text);
			}
			else
			{
				TextMeshProUGUI textMeshProUGUI = latitude_txt;
				TextMeshProUGUI textMeshProUGUI2 = longitude_text;
				TextMeshProUGUI textMeshProUGUI3 = altitude_text;
				TextMeshProUGUI textMeshProUGUI4 = radar_altitude_text;
				string text3 = (biomeName_text.text = Localizer.Format("#autoLOC_901099"));
				string text5 = (textMeshProUGUI4.text = text3);
				string text7 = (textMeshProUGUI3.text = text5);
				string text9 = (textMeshProUGUI2.text = text7);
				textMeshProUGUI.text = text9;
			}
		}

		private void SetPhysics()
		{
			if (Planetarium.fetch != null)
			{
				uT_text.text = Planetarium.GetUniversalTime().ToString("0.000");
			}
			else
			{
				uT_text.text = "";
			}
			if (!gamePaused)
			{
				physicsTimeRatio_text.text = (Time.deltaTime / Time.maximumDeltaTime).ToString("0.000");
			}
		}

		private void SetVessel(bool active)
		{
			if (FlightGlobals.ready && active)
			{
				string text = "";
				text = ((!FlightGlobals.currentMainBody.rotates || !FlightGlobals.currentMainBody.inverseRotation) ? Localizer.Format("#autoLOC_901101") : Localizer.Format("#autoLOC_901100"));
				activeVesselName_text.text = currentActiveVessel.GetDisplayName();
				refBody_text.text = Localizer.Format("#autoLOC_8002171", FlightGlobals.currentMainBody.bodyDisplayName.LocalizeRemoveGender(), (!FlightGlobals.currentMainBody.orbitDriver) ? Localizer.Format("#autoLOC_901101") : (FlightGlobals.currentMainBody.orbitDriver.reverse ? Localizer.Format("#autoLOC_8002169") : Localizer.Format("#autoLOC_8002170")));
				frameOfReference_text.text = text;
			}
			else
			{
				activeVesselName_text.text = Localizer.Format("#autoLOC_901099");
				TextMeshProUGUI textMeshProUGUI = refBody_text;
				string text3 = (frameOfReference_text.text = "");
				textMeshProUGUI.text = text3;
			}
		}

		private void SetVesselSlopeAngle(bool active)
		{
			if (active)
			{
				for (int i = 0; i < EVA_parameters.Length; i++)
				{
					EVA_parameters[i].SetActive(currentActiveVessel.isEVA);
				}
				currentActiveVessel.GetGroundLevelAngle();
				slopeAngle_text.text = currentActiveVessel.GroundLevelAngle.ToString();
			}
		}
	}
}
