namespace KSP.UI.Screens.Settings.Controls
{
	public class SettingsIntToggle : SettingsControlReflection
	{
		public UIButtonToggle toggle;

		[SettingsValue(1)]
		public int intEnabled = 1;

		[SettingsValue(0)]
		public int intDisabled;

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
			base.Value = (toggle.state ? intEnabled : intDisabled);
		}

		protected override void ValueInitialized()
		{
			toggle.SetState((int)base.Value == intEnabled);
		}

		protected override void ValueUpdated()
		{
			if (valueText == null)
			{
				return;
			}
			if ((int)base.Value == intEnabled)
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
