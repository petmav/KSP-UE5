namespace KSP.UI.Screens.DebugToolbar.Screens.Cheats
{
	public class IgnoreEVAConstructionMassLimit : DebugScreenToggle
	{
		protected override void SetupValues()
		{
			SetToggle(CheatOptions.IgnoreEVAConstructionMassLimit);
		}

		protected override void OnToggleChanged(bool state)
		{
			CheatOptions.IgnoreEVAConstructionMassLimit = state;
		}
	}
}
