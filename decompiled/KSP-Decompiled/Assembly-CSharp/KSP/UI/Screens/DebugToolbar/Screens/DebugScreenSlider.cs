using TMPro;
using UnityEngine;
using UnityEngine.UI;

namespace KSP.UI.Screens.DebugToolbar.Screens
{
	public class DebugScreenSlider : MonoBehaviour
	{
		public Slider slider;

		public float sliderMin;

		public float sliderMax = 100f;

		public float sliderDefault = 50f;

		public TextMeshProUGUI sliderText;

		private void Awake()
		{
			slider.minValue = sliderMin;
			slider.maxValue = sliderMax;
			slider.value = sliderDefault;
			SetupValues();
			slider.onValueChanged.AddListener(OnSliderChanged);
		}

		protected void SetSlider(float value)
		{
			if (slider.value != value)
			{
				slider.value = value;
			}
		}

		protected void SetSliderText(string text)
		{
			if (!string.IsNullOrEmpty(text) && sliderText.text != text)
			{
				sliderText.text = text;
			}
		}

		protected virtual void SetupValues()
		{
		}

		protected virtual void OnSliderChanged(float value)
		{
		}
	}
}
