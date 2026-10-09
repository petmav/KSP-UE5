using UnityEngine;
using UnityEngine.UI;

namespace KSP.UI.Screens.Settings.Controls
{
	public class SettingsIntRange : SettingsControlReflection
	{
		[SettingsValue(0)]
		public int minValue;

		[SettingsValue(1)]
		public int maxValue;

		public Slider slider;

		protected override void OnStart()
		{
			slider.onValueChanged.AddListener(OnSliderValueChange);
			slider.minValue = minValue;
			slider.maxValue = maxValue;
			slider.wholeNumbers = true;
		}

		private void OnSliderValueChange(float newValue)
		{
			base.Value = Mathf.FloorToInt(newValue);
		}

		protected override void ValueInitialized()
		{
			slider.value = (int)base.Value;
		}

		protected override void ValueUpdated()
		{
			valueText.text = ((int)base.Value).ToString();
		}
	}
}
