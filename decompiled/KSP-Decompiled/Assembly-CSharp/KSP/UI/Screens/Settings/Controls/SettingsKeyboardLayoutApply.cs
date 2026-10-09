using UnityEngine.UI;

namespace KSP.UI.Screens.Settings.Controls
{
	public class SettingsKeyboardLayoutApply : SettingsControlBase
	{
		public Button buttonApply;

		private void Start()
		{
			if (GameSettings.KeyboardLayouts.Count == 0)
			{
				buttonApply.interactable = false;
			}
		}
	}
}
