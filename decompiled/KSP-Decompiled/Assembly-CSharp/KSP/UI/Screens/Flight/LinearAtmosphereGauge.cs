using UnityEngine;

namespace KSP.UI.Screens.Flight
{
	public class LinearAtmosphereGauge : MonoBehaviour
	{
		public LinearGauge gauge;

		private CelestialBody body;

		private double densityRecip = 1.0;

		private void Reset()
		{
			gauge = GetComponent<LinearGauge>();
		}

		private void LateUpdate()
		{
			if (gauge == null || !FlightGlobals.ready)
			{
				return;
			}
			if (body != FlightGlobals.currentMainBody)
			{
				body = FlightGlobals.currentMainBody;
				if (body.atmosphere)
				{
					densityRecip = 1.0 / body.GetDensity(body.GetPressure(0.0), body.GetTemperature(0.0));
				}
				else
				{
					densityRecip = 1.0;
				}
			}
			if (body.atmosphere)
			{
				double num = FlightGlobals.ActiveVessel.atmDensity * densityRecip;
				if (num > 1.0)
				{
					num = 1.0;
				}
				gauge.SetValue(num);
			}
			else
			{
				gauge.SetValue(0f);
			}
		}
	}
}
