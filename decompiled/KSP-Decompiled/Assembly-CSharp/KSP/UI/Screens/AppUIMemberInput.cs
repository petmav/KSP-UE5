using KSP.Localization;
using TMPro;
using UnityEngine;

namespace KSP.UI.Screens
{
	[AppUI_Input]
	public class AppUIMemberInput : AppUIMember
	{
		public TMP_InputField input;

		internal ITypeParser parser;

		public TextMeshProUGUI suffixLabel;

		private RectTransform inputRect;

		private RectTransform suffixRect;

		private Vector2 inputOffsetMax;

		public bool showSuffix;

		[SerializeField]
		private string _suffixText;

		public float suffixWidth = 20f;

		public float suffixPadding = 4f;

		public string suffixText
		{
			get
			{
				return _suffixText;
			}
			set
			{
				_suffixText = Localizer.Format(value);
			}
		}

		protected override void OnStart()
		{
			input.onEndEdit.AddListener(InputEdited);
		}

		protected override void OnInitialized()
		{
			if (_attribs is AppUI_Input appUI_Input)
			{
				suffixText = (string.IsNullOrEmpty(appUI_Input.suffixText) ? base.Name : appUI_Input.suffixText);
				showSuffix = appUI_Input.showSuffix;
				suffixWidth = appUI_Input.suffixWidth;
				suffixPadding = appUI_Input.suffixPadding;
			}
			inputRect = input.transform as RectTransform;
			suffixRect = suffixLabel.transform as RectTransform;
			inputOffsetMax = inputRect.offsetMax;
			if (valueType == typeof(int))
			{
				parser = new IntTypeParser();
			}
			else if (valueType == typeof(float))
			{
				parser = new FloatTypeParser();
			}
			else if (valueType == typeof(double))
			{
				parser = new DoubleTypeParser();
			}
			else
			{
				parser = new StringTypeParser();
			}
		}

		protected virtual void RefreshUIInput()
		{
			input.text = GetValue().ToString();
		}

		protected override void OnRefreshUI()
		{
			suffixLabel.text = suffixText;
			suffixLabel.gameObject.SetActive(showSuffix);
			inputOffsetMax = inputRect.offsetMax;
			inputOffsetMax.x = (showSuffix ? (0f - suffixWidth - suffixPadding) : 0f);
			inputRect.offsetMax = inputOffsetMax;
			RefreshUIInput();
		}

		private void InputEdited(string value)
		{
			SetValue(parser.Parse(value));
		}

		internal override bool AnyTextFieldHasFocus()
		{
			return input.isFocused;
		}
	}
}
