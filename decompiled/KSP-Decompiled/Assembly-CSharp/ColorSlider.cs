using UnityEngine;
using UnityEngine.UI;

[RequireComponent(typeof(Slider))]
public class ColorSlider : MonoBehaviour
{
	public ColorPicker hsvpicker;

	public ColorValues type;

	private Slider slider;

	private bool listen = true;

	private void Start()
	{
		slider = GetComponent<Slider>();
		ColorChanged(hsvpicker.CurrentColor);
		HsvColor hsvColor = HSVUtil.ConvertRgbToHsv(hsvpicker.CurrentColor);
		HSVChanged(hsvColor.normalizedH, hsvColor.normalizedS, hsvColor.normalizedV);
		hsvpicker.onValueChanged.AddListener(ColorChanged);
		hsvpicker.onInternalValueChanged.AddListener(ColorChanged);
		hsvpicker.onHSVChanged.AddListener(HSVChanged);
		slider.onValueChanged.AddListener(SliderChanged);
	}

	private void OnDestroy()
	{
		hsvpicker.onValueChanged.RemoveListener(ColorChanged);
		hsvpicker.onInternalValueChanged.RemoveListener(ColorChanged);
		hsvpicker.onHSVChanged.RemoveListener(HSVChanged);
		if (slider != null)
		{
			slider.onValueChanged.RemoveListener(SliderChanged);
		}
	}

	private void ColorChanged(Color newColor)
	{
		listen = false;
		switch (type)
		{
		case ColorValues.R:
			slider.normalizedValue = newColor.r;
			break;
		case ColorValues.G:
			slider.normalizedValue = newColor.g;
			break;
		case ColorValues.B:
			slider.normalizedValue = newColor.b;
			break;
		case ColorValues.A:
			slider.normalizedValue = newColor.a;
			break;
		}
		listen = true;
	}

	private void HSVChanged(float hue, float saturation, float value)
	{
		listen = false;
		switch (type)
		{
		case ColorValues.Hue:
			slider.normalizedValue = hue;
			break;
		case ColorValues.Saturation:
			slider.normalizedValue = saturation;
			break;
		case ColorValues.Value:
			slider.normalizedValue = value;
			break;
		}
		listen = true;
	}

	private void SliderChanged(float newValue)
	{
		if (listen)
		{
			newValue = slider.normalizedValue;
			hsvpicker.AssignColor(type, newValue);
		}
		listen = true;
	}
}
