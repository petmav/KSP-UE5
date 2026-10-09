using UnityEngine;

namespace KSP.UI.Screens.Flight
{
	public class StagingLED : MonoBehaviour
	{
		public LED stagingLed;

		private void LateUpdate()
		{
			if (!FlightGlobals.ready)
			{
				return;
			}
			if (InputLockManager.IsLocked(ControlTypes.STAGING))
			{
				if (stagingLed.currentColor != LED.colorIndices.purple)
				{
					stagingLed.SetColor(LED.colorIndices.purple);
					stagingLed.SetOn();
				}
				return;
			}
			stagingLed.SetColor(StageManager.CanSeparate ? LED.colorIndices.green : LED.colorIndices.yellow);
			if ((stagingLed.currentColor != LED.colorIndices.green || stagingLed.currentColor != LED.colorIndices.yellow) && !stagingLed.IsBlinking)
			{
				stagingLed.SetOn();
			}
			if (StageManager.CurrentStage == StageManager.StageCount && !stagingLed.IsBlinking)
			{
				stagingLed.Blink(0.3f);
			}
			if (StageManager.CurrentStage == 0)
			{
				stagingLed.setOff();
			}
			else if (StageManager.CanSeparate && StageManager.CurrentStage != StageManager.StageCount)
			{
				if (!stagingLed.IsOn)
				{
					stagingLed.SetOn();
				}
			}
			else if (!stagingLed.IsBlinking)
			{
				stagingLed.Blink(0.2f);
			}
		}
	}
}
