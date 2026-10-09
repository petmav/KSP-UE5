using UnityEngine;

namespace KSP.UI.Screens.Settings.Controls.Values
{
	public class SettingsValueIntToString : MonoBehaviour
	{
		[SettingsValue(new string[] { "A", "B" })]
		public string[] values = new string[2] { "A", "B" };

		private SettingsControlReflection control;

		private void Awake()
		{
			control = GetComponent<SettingsControlReflection>();
		}

		public void UpdateValue(object value)
		{
			if (control.valueText != null && value is int num && num >= 0 && num < values.Length)
			{
				control.valueText.text = values[num];
			}
		}
	}
}
