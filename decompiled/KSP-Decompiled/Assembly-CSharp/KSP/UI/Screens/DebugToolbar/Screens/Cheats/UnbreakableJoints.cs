namespace KSP.UI.Screens.DebugToolbar.Screens.Cheats
{
	public class UnbreakableJoints : DebugScreenToggle
	{
		protected override void SetupValues()
		{
			SetToggle(CheatOptions.UnbreakableJoints);
		}

		protected override void OnToggleChanged(bool state)
		{
			CheatOptions.UnbreakableJoints = state;
		}
	}
}
