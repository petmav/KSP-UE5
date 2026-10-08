namespace KSP.UI.Screens.DebugToolbar.Screens.Cheats
{
	public class PauseOnVesselUnpack : DebugScreenToggle
	{
		protected override void SetupValues()
		{
			SetToggle(CheatOptions.PauseOnVesselUnpack);
		}

		protected override void OnToggleChanged(bool state)
		{
			CheatOptions.PauseOnVesselUnpack = state;
		}
	}
}
