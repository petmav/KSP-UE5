using KSP.Localization;
using TMPro;
using UnityEngine;
using UnityEngine.UI;

namespace KSP.UI
{
	public class SliderValueToText : MonoBehaviour
	{
		public Slider slider;

		public TextMeshProUGUI text;

		public string prefix;

		public string suffix;

		public float sliderValueMultiplier;

		private void Awake()
		{
			if (slider == null)
			{
				slider = GetComponent<Slider>();
			}
			if (slider == null)
			{
				Debug.LogError("SliderValueToText: Terminating -> Slider not found.");
				return;
			}
			slider.onValueChanged.AddListener(SliderValueChangeListener);
			SliderValueChangeListener(slider.value);
		}

		private void SliderValueChangeListener(float value)
		{
			text.text = Localizer.Format(prefix) + value * sliderValueMultiplier + suffix;
		}
	}
}
