namespace KSP.UI.Screens.DebugToolbar.Screens.Cheats
{
	public class MiddleMouseClickSetPosition : DebugScreenToggle
	{
		protected override void SetupValues()
		{
			SetToggle(CheatOptions.MiddleMouseClickSetPosition);
		}

		protected override void OnToggleChanged(bool state)
		{
			CheatOptions.MiddleMouseClickSetPosition = state;
		}
	}
}
