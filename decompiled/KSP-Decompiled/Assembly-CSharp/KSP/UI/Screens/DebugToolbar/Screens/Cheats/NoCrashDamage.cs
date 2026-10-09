namespace KSP.UI.Screens.DebugToolbar.Screens.Cheats
{
	public class NoCrashDamage : DebugScreenToggle
	{
		protected override void SetupValues()
		{
			SetToggle(CheatOptions.NoCrashDamage);
		}

		protected override void OnToggleChanged(bool state)
		{
			CheatOptions.NoCrashDamage = state;
		}
	}
}
