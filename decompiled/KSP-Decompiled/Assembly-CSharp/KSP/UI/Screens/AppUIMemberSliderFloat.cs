using UnityEngine.UI;

namespace KSP.UI.Screens
{
	[AppUI_SliderFloat]
	public class AppUIMemberSliderFloat : AppUIMember
	{
		public Slider valueSlider;

		public float minValue;

		public float maxValue = 1f;

		public bool wholeNumbers;

		private void OnDestroy()
		{
			valueSlider.onValueChanged.RemoveListener(OnSliderChanged);
		}

		protected override void OnInitialized()
		{
			if (_attribs is AppUI_SliderFloat appUI_SliderFloat)
			{
				minValue = appUI_SliderFloat.minValue;
				maxValue = appUI_SliderFloat.maxValue;
				wholeNumbers = appUI_SliderFloat.wholeNumbers;
			}
			valueSlider.onValueChanged.AddListener(OnSliderChanged);
		}

		protected override void OnRefreshUI()
		{
			valueSlider.minValue = minValue;
			valueSlider.maxValue = maxValue;
			valueSlider.wholeNumbers = wholeNumbers;
			valueSlider.value = GetValue<float>();
		}

		private void OnSliderChanged(float value)
		{
			SetValue(value);
		}
	}
}
