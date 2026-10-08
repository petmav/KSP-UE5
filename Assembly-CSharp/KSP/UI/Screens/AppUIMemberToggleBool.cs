using KSP.Localization;
using TMPro;
using UnityEngine;
using UnityEngine.UI;

namespace KSP.UI.Screens
{
	[AppUI_ToggleBool]
	public class AppUIMemberToggleBool : AppUIMember
	{
		public Toggle trueToggle;

		public Toggle falseToggle;

		public TextMeshProUGUI trueLabel;

		public TextMeshProUGUI falseLabel;

		[SerializeField]
		private string _trueText = "True";

		[SerializeField]
		private string _falseText = "False";

		private bool valueState;

		public string trueText
		{
			get
			{
				return _trueText;
			}
			set
			{
				_trueText = Localizer.Format(value);
			}
		}

		public string falseText
		{
			get
			{
				return _falseText;
			}
			set
			{
				_falseText = Localizer.Format(value);
			}
		}

		private void OnDestroy()
		{
			trueToggle.onValueChanged.RemoveListener(OnTrueToggleChanged);
		}

		protected override void OnInitialized()
		{
			if (_attribs is AppUI_ToggleBool appUI_ToggleBool)
			{
				trueText = (string.IsNullOrEmpty(appUI_ToggleBool.trueText) ? "" : appUI_ToggleBool.trueText);
				falseText = (string.IsNullOrEmpty(appUI_ToggleBool.falseText) ? "" : appUI_ToggleBool.falseText);
			}
			trueToggle.onValueChanged.AddListener(OnTrueToggleChanged);
		}

		protected override void OnRefreshUI()
		{
			trueLabel.text = trueText;
			falseLabel.text = falseText;
			valueState = GetValue<bool>();
			if (valueState)
			{
				trueToggle.isOn = true;
			}
			else
			{
				falseToggle.isOn = true;
			}
		}

		private void OnTrueToggleChanged(bool value)
		{
			SetValue(value);
		}
	}
}
