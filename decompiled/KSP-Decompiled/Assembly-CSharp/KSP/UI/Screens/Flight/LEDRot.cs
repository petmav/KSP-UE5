using UnityEngine;

namespace KSP.UI.Screens.Flight
{
	public class LEDRot : MonoBehaviour
	{
		public LED led;

		private void Reset()
		{
			led = GetComponent<LED>();
		}

		private void LateUpdate()
		{
			if (FlightGlobals.ready && led != null)
			{
				led.SetOn(InputBinding.linRotState);
			}
		}
	}
}
