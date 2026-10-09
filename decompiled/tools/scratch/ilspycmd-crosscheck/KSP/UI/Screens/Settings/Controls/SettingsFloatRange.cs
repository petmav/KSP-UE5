using UnityEngine;
using UnityEngine.UI;

namespace KSP.UI.Screens.Settings.Controls
{
	public class SettingsFloatRange : SettingsControlReflection
	{
		[SettingsValue(0f)]
		public float minValue;

		[SettingsValue(1f)]
		public float maxValue = 1f;

		[SettingsValue(1f)]
		public float displayMultiply = 1f;

		[SettingsValue("F0")]
		public string displayFormat = "F0";

		[SettingsValue("")]
		public string displayUnits = "";

		[SettingsValue(-1)]
		public int roundToPlaces = -1;

		public Slider slider;

		protected override void OnStart()
		{
			slider.onValueChanged.AddListener(OnSliderValueChange);
			slider.minValue = minValue;
			slider.maxValue = maxValue;
		}

		private void OnSliderValueChange(float newValue)
		{
			if (roundToPlaces > -1)
			{
				newValue = ((roundToPlaces != 0) ? (Mathf.Round(newValue * Mathf.Pow(10f, roundToPlaces)) / Mathf.Pow(10f, roundToPlaces)) : Mathf.Round(newValue));
			}
			base.Value = newValue;
		}

		protected override void ValueInitialized()
		{
			slider.value = (float)base.Value;
			if (settingName == "UI_SCALE")
			{
				SetColorForRecommendedUIScale(slider.value);
				slider.onValueChanged.AddListener(SetColorForRecommendedUIScale);
			}
		}

		protected override void ValueUpdated()
		{
			if (valueText != null)
			{
				valueText.text = ((float)base.Value * displayMultiply).ToString(displayFormat) + displayUnits;
			}
		}

		private void SetColorForRecommendedUIScale(float value)
		{
			if ((float)base.Value <= UIMasterController.Instance.GetMaxSuggestedUIScale())
			{
				titleText.color = Color.white;
				valueText.color = Color.white;
			}
			else
			{
				titleText.color = Color.red;
				valueText.color = Color.red;
			}
		}
	}
}
