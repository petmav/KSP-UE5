namespace KSP.UI.Screens.Settings.Controls
{
	public class SettingsBoolToggle : SettingsControlReflection
	{
		public UIButtonToggle toggle;

		[SettingsValue("True")]
		public string valueEnabled = "";

		[SettingsValue("False")]
		public string valueDisabled = "";

		protected override void OnStart()
		{
			toggle.onToggle.AddListener(OnToggled);
		}

		private void OnToggled()
		{
			base.Value = toggle.state;
		}

		protected override void ValueInitialized()
		{
			toggle.SetState((bool)base.Value);
		}

		protected override void ValueUpdated()
		{
			if (valueText == null)
			{
				return;
			}
			if ((bool)base.Value)
			{
				if (!string.IsNullOrEmpty(valueEnabled))
				{
					valueText.text = valueEnabled;
				}
			}
			else if (!string.IsNullOrEmpty(valueEnabled))
			{
				valueText.text = valueDisabled;
			}
		}
	}
}
