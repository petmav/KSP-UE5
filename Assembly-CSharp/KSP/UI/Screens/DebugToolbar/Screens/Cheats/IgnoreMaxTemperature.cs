namespace KSP.UI.Screens.DebugToolbar.Screens.Cheats
{
	public class IgnoreMaxTemperature : DebugScreenToggle
	{
		protected override void SetupValues()
		{
			SetToggle(CheatOptions.IgnoreMaxTemperature);
		}

		protected override void OnToggleChanged(bool state)
		{
			CheatOptions.IgnoreMaxTemperature = state;
		}
	}
}
