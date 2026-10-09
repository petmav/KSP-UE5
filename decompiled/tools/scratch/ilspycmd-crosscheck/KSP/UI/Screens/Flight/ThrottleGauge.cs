using UnityEngine;

namespace KSP.UI.Screens.Flight
{
	public class ThrottleGauge : MonoBehaviour
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
				KerbalEVA kerbalEVA = (FlightGlobals.ActiveVessel.isEVA ? FlightGlobals.ActiveVessel.evaController : null);
				if (kerbalEVA == null)
				{
					gauge.SetValue(FlightGlobals.ActiveVessel.ctrlState.mainThrottle);
				}
				else
				{
					gauge.SetValue(Mathf.Lerp(gauge.Value, kerbalEVA.JetpackIsThrusting ? 1 : 0, Time.deltaTime * 0.5f));
				}
			}
		}
	}
}
