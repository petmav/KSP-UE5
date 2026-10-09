using TMPro;
using UnityEngine;

public class ColorLabel : MonoBehaviour
{
	public ColorPicker picker;

	public ColorValues type;

	public string prefix = "R: ";

	public float minValue;

	public float maxValue = 255f;

	public int precision;

	private TextMeshProUGUI label;

	private void Awake()
	{
		label = GetComponent<TextMeshProUGUI>();
	}

	private void OnEnable()
	{
		if (Application.isPlaying && picker != null)
		{
			picker.onValueChanged.AddListener(ColorChanged);
			picker.onInternalValueChanged.AddListener(ColorChanged);
			picker.onHSVChanged.AddListener(HSVChanged);
			ColorChanged(picker.CurrentColor);
			HSVChanged(picker.H, picker.S, picker.V);
		}
	}

	private void OnDestroy()
	{
		if (picker != null)
		{
			picker.onValueChanged.RemoveListener(ColorChanged);
			picker.onInternalValueChanged.RemoveListener(ColorChanged);
			picker.onHSVChanged.RemoveListener(HSVChanged);
		}
	}

	private void ColorChanged(Color color)
	{
		UpdateValue();
	}

	private void HSVChanged(float hue, float sateration, float value)
	{
		UpdateValue();
	}

	private void UpdateValue()
	{
		if (!(label == null))
		{
			if (picker == null)
			{
				label.text = prefix + "-";
				return;
			}
			float value = minValue + picker.GetValue(type) * (maxValue - minValue);
			label.text = prefix + ConvertToDisplayString(value);
		}
	}

	private string ConvertToDisplayString(float value)
	{
		if (precision > 0)
		{
			return value.ToString("f " + precision);
		}
		return Mathf.FloorToInt(value).ToString();
	}
}
