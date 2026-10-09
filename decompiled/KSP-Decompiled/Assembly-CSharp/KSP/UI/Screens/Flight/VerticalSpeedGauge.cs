using UnityEngine;

namespace KSP.UI.Screens.Flight
{
	public class VerticalSpeedGauge : MonoBehaviour
	{
		public RotationalGauge gauge;

		public LED sinkRateLED;

		public static double SinkRateOn = -15.0;

		public static double SinkRateOff = -14.0;

		private void Reset()
		{
			gauge = GetComponent<RotationalGauge>();
		}

		public void Start()
		{
			sinkRateLED.SetColor(LED.colorIndices.yellow);
			sinkRateLED.setOff();
		}

		private void LateUpdate()
		{
			if (!FlightGlobals.ready)
			{
				return;
			}
			if (sinkRateLED.IsOn)
			{
				if (FlightGlobals.ActiveVessel.verticalSpeed > SinkRateOff)
				{
					sinkRateLED.setOff();
				}
			}
			else if (FlightGlobals.ActiveVessel.verticalSpeed < SinkRateOn)
			{
				sinkRateLED.SetOn();
			}
			gauge.SetValue(FlightGlobals.ActiveVessel.verticalSpeed);
		}
	}
}
