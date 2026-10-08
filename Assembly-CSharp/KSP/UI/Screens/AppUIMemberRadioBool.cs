using KSP.Localization;
using TMPro;
using UnityEngine;
using UnityEngine.UI;

namespace KSP.UI.Screens
{
	[AppUI_RadioBool]
	public class AppUIMemberRadioBool : AppUIMember
	{
		public Toggle valueToggle;

		public TextMeshProUGUI valueLabel;

		[SerializeField]
		private string _valueText = "Value";

		private bool valueState;

		public string valueText
		{
			get
			{
				return _valueText;
			}
			set
			{
				_valueText = Localizer.Format(value);
			}
		}

		private void OnDestroy()
		{
			valueToggle.onValueChanged.RemoveListener(OnTrueToggleChanged);
		}

		protected override void OnInitialized()
		{
			if (_attribs is AppUI_RadioBool appUI_RadioBool)
			{
				valueText = (string.IsNullOrEmpty(appUI_RadioBool.valueText) ? "" : appUI_RadioBool.valueText);
			}
			valueToggle.onValueChanged.AddListener(OnTrueToggleChanged);
		}

		protected override void OnRefreshUI()
		{
			valueLabel.text = valueText;
			valueState = GetValue<bool>();
			valueToggle.isOn = valueState;
		}

		private void OnTrueToggleChanged(bool value)
		{
			SetValue(value);
		}
	}
}
