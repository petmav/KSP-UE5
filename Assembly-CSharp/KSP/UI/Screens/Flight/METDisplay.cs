using KSP.Localization;
using TMPro;
using UnityEngine;

namespace KSP.UI.Screens.Flight
{
	public class METDisplay : MonoBehaviour
	{
		public TextMeshProUGUI text;

		public TextMeshProUGUI buttonText;

		public bool displayUT;

		private bool gamePaused;

		private static string cacheMETDatePortion = "";

		private string currentMETDatePortion = "";

		private static string cacheAutoLOC_7003242;

		private void Reset()
		{
			text = GetComponent<TextMeshProUGUI>();
		}

		private void Start()
		{
			cacheMETDatePortion = null;
			GameEvents.onGamePause.Add(onGamePause);
			GameEvents.onGameUnpause.Add(onGameUnPause);
		}

		private void OnDestroy()
		{
			GameEvents.onGamePause.Remove(onGamePause);
			GameEvents.onGameUnpause.Remove(onGameUnPause);
		}

		private void LateUpdate()
		{
			if (!FlightGlobals.ready || (object)text == null)
			{
				return;
			}
			if (displayUT)
			{
				text.text = KSPUtil.PrintDateCompact(Planetarium.GetUniversalTime(), includeTime: true, includeSeconds: true);
			}
			else
			{
				currentMETDatePortion = KSPUtil.PrintTimeStampCompact(FlightLogger.met, days: true, years: true);
				if (currentMETDatePortion != cacheMETDatePortion)
				{
					text.text = cacheAutoLOC_7003242 + " " + currentMETDatePortion;
					cacheMETDatePortion = currentMETDatePortion;
				}
			}
			if (!gamePaused)
			{
				text.color = ((Time.deltaTime < Time.maximumDeltaTime * 1f) ? Color.green : ((Time.deltaTime < Time.maximumDeltaTime * 1.3f) ? Color.yellow : Color.red));
			}
		}

		public void ToggleTimeMode()
		{
			displayUT = !displayUT;
			cacheMETDatePortion = null;
			if ((object)buttonText != null)
			{
				buttonText.text = (displayUT ? "#autoLOC_6001498" : "#autoLOC_900615");
			}
		}

		public void SetTimeMode(bool displayUT)
		{
			this.displayUT = displayUT;
			cacheMETDatePortion = null;
			if ((object)buttonText != null)
			{
				buttonText.text = (displayUT ? "#autoLOC_6001498" : "#autoLOC_900615");
			}
		}

		private void onGamePause()
		{
			text.color = new Color(1f, 1f, 1f, 0.4375f);
			gamePaused = true;
		}

		private void onGameUnPause()
		{
			text.color = Color.green;
			gamePaused = false;
		}

		internal static void CacheLocalStrings()
		{
			cacheAutoLOC_7003242 = Localizer.Format("#autoLOC_7003242");
		}
	}
}
