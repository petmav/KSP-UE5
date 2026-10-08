namespace KSP.UI.Screens.Settings.Controls
{
	public class SettingsBoolToggleBitMask : SettingsControlReflection
	{
		public UIButtonToggle toggle;

		[SettingsValue(0)]
		public int bitMask;

		[SettingsValue("Enabled")]
		public string valueOn;

		[SettingsValue("Disabled")]
		public string valueOff;

		private bool isOn;

		protected override void OnStart()
		{
			toggle.onToggle.AddListener(OnToggled);
		}

		private void OnToggled()
		{
			isOn = toggle.state;
		}

		protected override void ValueInitialized()
		{
			int num = (int)base.Value;
			isOn = (num & (1 << bitMask)) != 0;
			toggle.SetState(isOn);
		}

		public override void OnApply()
		{
			if (accessor != null)
			{
				int num = (int)accessor.Value;
				num = ((!isOn) ? (num & ~(1 << bitMask)) : (num | (1 << bitMask)));
				accessor.Value = num;
			}
		}
	}
}
