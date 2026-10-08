namespace KSP.UI.Screens.DebugToolbar.Screens.Cheats
{
	public class IgnoreKerbalInventoryLimits : DebugScreenToggle
	{
		protected override void SetupValues()
		{
			SetToggle(CheatOptions.IgnoreKerbalInventoryLimits);
		}

		protected override void OnToggleChanged(bool state)
		{
			CheatOptions.IgnoreKerbalInventoryLimits = state;
		}
	}
}
