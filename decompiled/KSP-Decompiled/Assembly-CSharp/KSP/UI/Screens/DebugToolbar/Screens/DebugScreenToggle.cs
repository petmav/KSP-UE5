using TMPro;
using UnityEngine;
using UnityEngine.UI;

namespace KSP.UI.Screens.DebugToolbar.Screens
{
	public class DebugScreenToggle : MonoBehaviour
	{
		public Toggle toggle;

		public TextMeshProUGUI toggleText;

		public bool initiallyOn;

		public string text = "";

		private void Awake()
		{
			SetToggle(initiallyOn);
			SetToggleText(text);
			SetupValues();
			toggle.onValueChanged.AddListener(OnToggleChanged);
		}

		protected void SetToggleText(string text)
		{
			if (!string.IsNullOrEmpty(text))
			{
				if (toggleText.text != text)
				{
					toggleText.text = text;
				}
				this.text = text;
			}
		}

		protected void SetToggle(bool state)
		{
			toggle.isOn = state;
		}

		protected virtual void SetupValues()
		{
		}

		protected virtual void OnToggleChanged(bool state)
		{
		}

		public void Set(bool b)
		{
			if (toggle.isOn != b)
			{
				toggle.isOn = b;
			}
		}
	}
}
