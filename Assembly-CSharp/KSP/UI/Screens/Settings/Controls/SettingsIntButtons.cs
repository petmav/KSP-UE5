using UnityEngine.UI;

namespace KSP.UI.Screens.Settings.Controls
{
	public class SettingsIntButtons : SettingsControlReflection
	{
		public Button buttonUp;

		public Button buttonDown;

		[SettingsValue(0)]
		public int minValue;

		[SettingsValue(1)]
		public int maxValue;

		protected override void OnStart()
		{
			buttonUp.onClick.AddListener(OnButtonUp);
			buttonDown.onClick.AddListener(OnButtonDown);
		}

		private void OnButtonUp()
		{
			int num = (int)base.Value;
			if (num < maxValue)
			{
				base.Value = num + 1;
			}
		}

		private void OnButtonDown()
		{
			int num = (int)base.Value;
			if (num > minValue)
			{
				base.Value = num - 1;
			}
		}
	}
}
