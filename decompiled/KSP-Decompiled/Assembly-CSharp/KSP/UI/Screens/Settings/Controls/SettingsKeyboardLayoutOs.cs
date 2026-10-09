using KSP.Localization;
using TMPro;
using UnityEngine;

namespace KSP.UI.Screens.Settings.Controls
{
	public class SettingsKeyboardLayoutOs : SettingsControlBase
	{
		public TextMeshProUGUI keyboardLabel;

		private void Start()
		{
			KeyboardLayout keyboardLayout = KeyboardLayout.GetKeyboardLayout();
			Debug.Log("Locale is: " + keyboardLayout.Locale.DisplayName);
			keyboardLabel.text = Localizer.Format("#autoLOC_6001209", keyboardLayout.Type, keyboardLayout.Locale.NativeName);
		}
	}
}
