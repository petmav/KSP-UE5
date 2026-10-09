using UnityEngine;
using UnityEngine.UI;

[UI_FieldFloatRange]
public class UIPartActionFieldFloatRange : UIPartActionFieldItem
{
	public Text fieldName;

	public Text fieldAmount;

	public Slider slider;

	private BaseField minValueField;

	private float minValue;

	private BaseField maxValueField;

	private float maxValue;

	private float fieldValue;

	private bool handlingChange;

	protected UI_FieldFloatRange progBarControl => (UI_FieldFloatRange)control;

	public override void Setup(UIPartActionWindow window, Part part, PartModule partModule, UI_Scene scene, UI_Control control, BaseField field)
	{
		base.Setup(window, part, partModule, scene, control, field);
		minValueField = GetField(progBarControl.minValue);
		maxValueField = GetField(progBarControl.maxValue);
		UpdateFieldValues();
		SetSliderValue(fieldValue);
		slider.onValueChanged.AddListener(OnValueChanged);
		window.usingNumericValue = true;
	}

	public override void UpdateItem()
	{
		UpdateFieldValues();
		fieldName.text = field.guiName;
		fieldAmount.text = KSPUtil.LocalizeNumber(fieldValue, field.guiFormat);
		SetSliderValue(fieldValue);
	}

	private void UpdateFieldValues()
	{
		fieldValue = field.GetValue<float>(base.Host);
		minValue = minValueField.GetValue<float>(base.Host);
		maxValue = maxValueField.GetValue<float>(base.Host);
	}

	private void SetSliderValue(float rawValue)
	{
		slider.value = Mathf.Clamp01((rawValue - minValue) / (maxValue - minValue));
	}

	private void OnValueChanged(float obj)
	{
		if (!handlingChange)
		{
			handlingChange = true;
			if (InputLockManager.IsUnlocked((control == null || !control.requireFullControl) ? ControlTypes.TWEAKABLES_ANYCONTROL : ControlTypes.TWEAKABLES_FULLONLY))
			{
				fieldValue = slider.value * (maxValue - minValue) + minValue;
				SetFieldValue(fieldValue);
			}
			handlingChange = false;
		}
	}
}
