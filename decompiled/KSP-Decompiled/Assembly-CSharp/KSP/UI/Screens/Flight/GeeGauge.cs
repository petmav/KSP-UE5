using UnityEngine;

namespace KSP.UI.Screens.Flight
{
	public class GeeGauge : MonoBehaviour
	{
		public RotationalGauge gauge;

		private void Reset()
		{
			gauge = GetComponent<RotationalGauge>();
		}

		private void LateUpdate()
		{
			if (FlightGlobals.ready && (object)gauge != null)
			{
				gauge.SetValue(FlightGlobals.ActiveVessel.geeForce);
			}
		}
	}
}
