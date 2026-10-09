using TMPro;
using UnityEngine;

namespace KSP.UI
{
	public class UIPlanetariumDateTime : MonoBehaviour
	{
		public TextMeshProUGUI textDate;

		public bool showTime = true;

		public bool showSeconds;

		private void Start()
		{
			GameEvents.onGamePause.Add(onGamePause);
			GameEvents.onGameUnpause.Add(onGameUnPause);
			if (FlightDriver.Pause)
			{
				onGamePause();
			}
			SetTime();
		}

		private void OnDestroy()
		{
			GameEvents.onGamePause.Remove(onGamePause);
			GameEvents.onGameUnpause.Remove(onGameUnPause);
		}

		private void Update()
		{
			SetTime();
		}

		public void SetTime()
		{
			if (!(Planetarium.fetch == null))
			{
				textDate.text = KSPUtil.PrintDate(Planetarium.GetUniversalTime(), showTime, showSeconds);
			}
		}

		private void onGamePause()
		{
			textDate.color = new Color(1f, 1f, 1f, 0.4375f);
		}

		private void onGameUnPause()
		{
			textDate.color = Color.green;
		}
	}
}
