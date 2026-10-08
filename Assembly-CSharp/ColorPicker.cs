using System;
using UnityEngine;
using UnityEngine.UI;

public class ColorPicker : MonoBehaviour
{
	[SerializeField]
	private float _hue;

	[SerializeField]
	private float _saturation;

	[SerializeField]
	private float _brightness;

	[SerializeField]
	private float _red;

	[SerializeField]
	private float _green;

	[SerializeField]
	private float _blue;

	[SerializeField]
	private float _alpha = 1f;

	public Button modeButton;

	public ColorChangedEvent onValueChanged = new ColorChangedEvent();

	public HSVChangedEvent onHSVChanged = new HSVChangedEvent();

	public ColorChangedEvent onInternalValueChanged = new ColorChangedEvent();

	public GameObject[] rgbModeSliders;

	public GameObject[] hueModeSliders;

	public Color CurrentColor
	{
		get
		{
			return new Color(_red, _green, _blue, _alpha);
		}
		set
		{
			if (!(CurrentColor == value))
			{
				_red = value.r;
				_green = value.g;
				_blue = value.b;
				_alpha = value.a;
				RGBChanged();
				SendChangedEvent();
			}
		}
	}

	public float H
	{
		get
		{
			return _hue;
		}
		set
		{
			if (_hue != value)
			{
				_hue = value;
				HSVChanged();
				SendChangedEvent();
			}
		}
	}

	public float S
	{
		get
		{
			return _saturation;
		}
		set
		{
			if (_saturation != value)
			{
				_saturation = value;
				HSVChanged();
				SendChangedEvent();
			}
		}
	}

	public float V
	{
		get
		{
			return _brightness;
		}
		set
		{
			if (_brightness != value)
			{
				_brightness = value;
				HSVChanged();
				SendChangedEvent();
			}
		}
	}

	public float R
	{
		get
		{
			return _red;
		}
		set
		{
			if (_red != value)
			{
				_red = value;
				RGBChanged();
				SendChangedEvent();
			}
		}
	}

	public float G
	{
		get
		{
			return _green;
		}
		set
		{
			if (_green != value)
			{
				_green = value;
				RGBChanged();
				SendChangedEvent();
			}
		}
	}

	public float B
	{
		get
		{
			return _blue;
		}
		set
		{
			if (_blue != value)
			{
				_blue = value;
				RGBChanged();
				SendChangedEvent();
			}
		}
	}

	private float A
	{
		get
		{
			return _alpha;
		}
		set
		{
			if (_alpha != value)
			{
				_alpha = value;
				SendChangedEvent();
			}
		}
	}

	public void SetColor(Color color)
	{
		if (!(CurrentColor == color))
		{
			_red = color.r;
			_green = color.g;
			_blue = color.b;
			_alpha = color.a;
			RGBChanged();
			SendInternalEvent();
		}
	}

	private void Start()
	{
		SendInternalEvent();
		modeButton.onClick.AddListener(ToggleModeSliders);
	}

	private void OnDestroy()
	{
		modeButton.onClick.RemoveListener(ToggleModeSliders);
	}

	private void OnEnable()
	{
		SendInternalEvent();
	}

	private void RGBChanged()
	{
		HsvColor hsvColor = HSVUtil.ConvertRgbToHsv(CurrentColor);
		_hue = hsvColor.normalizedH;
		_saturation = hsvColor.normalizedS;
		_brightness = hsvColor.normalizedV;
	}

	private void HSVChanged()
	{
		Color color = HSVUtil.ConvertHsvToRgb(_hue * 360f, _saturation, _brightness, _alpha);
		_red = color.r;
		_green = color.g;
		_blue = color.b;
	}

	private void SendChangedEvent()
	{
		if (base.gameObject.activeInHierarchy)
		{
			onValueChanged.Invoke(CurrentColor);
			onHSVChanged.Invoke(_hue, _saturation, _brightness);
		}
	}

	private void SendInternalEvent()
	{
		onInternalValueChanged.Invoke(CurrentColor);
		onHSVChanged.Invoke(_hue, _saturation, _brightness);
	}

	public void AssignColor(ColorValues type, float value)
	{
		switch (type)
		{
		case ColorValues.R:
			R = value;
			break;
		case ColorValues.G:
			G = value;
			break;
		case ColorValues.B:
			B = value;
			break;
		case ColorValues.A:
			A = value;
			break;
		case ColorValues.Hue:
			H = value;
			break;
		case ColorValues.Saturation:
			S = value;
			break;
		case ColorValues.Value:
			V = value;
			break;
		}
	}

	public float GetValue(ColorValues type)
	{
		switch (type)
		{
		default:
			throw new NotImplementedException("");
		case ColorValues.R:
			return R;
		case ColorValues.G:
			return G;
		case ColorValues.B:
			return B;
		case ColorValues.A:
			return A;
		case ColorValues.Hue:
			return H;
		case ColorValues.Saturation:
			return S;
		case ColorValues.Value:
			return V;
		}
	}

	public void ToggleModeSliders()
	{
		int i = 0;
		for (int num = rgbModeSliders.Length; i < num; i++)
		{
			rgbModeSliders[i].SetActive(!rgbModeSliders[i].activeSelf);
		}
		int j = 0;
		for (int num2 = hueModeSliders.Length; j < num2; j++)
		{
			hueModeSliders[j].SetActive(!hueModeSliders[j].activeSelf);
		}
	}
}
