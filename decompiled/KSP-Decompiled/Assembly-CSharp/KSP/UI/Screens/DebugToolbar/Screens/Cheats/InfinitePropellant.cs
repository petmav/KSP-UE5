namespace KSP.UI.Screens.DebugToolbar.Screens.Cheats
{
	public class InfinitePropellant : DebugScreenToggle
	{
		protected override void SetupValues()
		{
			SetToggle(CheatOptions.InfinitePropellant);
		}

		protected override void OnToggleChanged(bool state)
		{
			CheatOptions.InfinitePropellant = state;
		}
	}
}
