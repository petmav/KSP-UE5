namespace KSP.UI.Screens.DebugToolbar.Screens.Cheats
{
	public class AllowPartClipping : DebugScreenToggle
	{
		protected override void SetupValues()
		{
			SetToggle(CheatOptions.AllowPartClipping);
		}

		protected override void OnToggleChanged(bool state)
		{
			CheatOptions.AllowPartClipping = state;
		}
	}
}
