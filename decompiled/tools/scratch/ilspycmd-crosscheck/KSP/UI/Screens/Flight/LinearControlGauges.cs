using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

namespace KSP.UI.Screens.Flight
{
	public class LinearControlGauges : MonoBehaviour
	{
		public List<Image> inputGaugeImages;

		public LinearGauge pitch;

		public LinearGauge yaw;

		public LinearGauge roll;

		public LinearGauge linX;

		public LinearGauge linY;

		public LinearGauge linZ;

		public bool linXInverted;

		public bool linYInverted;

		public bool linZInverted;

		public void Start()
		{
			GameEvents.Input.OnPrecisionModeToggle.Add(onPrecisionModeToggle);
		}

		public void OnDestroy()
		{
			GameEvents.Input.OnPrecisionModeToggle.Remove(onPrecisionModeToggle);
		}

		protected void onPrecisionModeToggle(bool precisionMode)
		{
			int count = inputGaugeImages.Count;
			while (count-- > 0)
			{
				inputGaugeImages[count].color = (precisionMode ? XKCDColors.BrightCyan : XKCDColors.Orange);
			}
		}

		private void LateUpdate()
		{
			if (!FlightGlobals.ready)
			{
				return;
			}
			if (pitch != null)
			{
				pitch.SetValue(0f - FlightGlobals.ActiveVessel.ctrlState.pitch);
			}
			if (yaw != null)
			{
				yaw.SetValue(FlightGlobals.ActiveVessel.ctrlState.yaw);
			}
			if (roll != null)
			{
				roll.SetValue(FlightGlobals.ActiveVessel.ctrlState.roll);
			}
			if (linX != null)
			{
				if (!linXInverted)
				{
					linX.SetValue(FlightGlobals.ActiveVessel.ctrlState.X);
				}
				else
				{
					linX.SetValue(0f - FlightGlobals.ActiveVessel.ctrlState.X);
				}
			}
			if (linY != null)
			{
				if (!linYInverted)
				{
					linY.SetValue(FlightGlobals.ActiveVessel.ctrlState.Y);
				}
				else
				{
					linY.SetValue(0f - FlightGlobals.ActiveVessel.ctrlState.Y);
				}
			}
			if (linZ != null)
			{
				if (!linZInverted)
				{
					linZ.SetValue(FlightGlobals.ActiveVessel.ctrlState.Z);
				}
				else
				{
					linZ.SetValue(0f - FlightGlobals.ActiveVessel.ctrlState.Z);
				}
			}
		}
	}
}
