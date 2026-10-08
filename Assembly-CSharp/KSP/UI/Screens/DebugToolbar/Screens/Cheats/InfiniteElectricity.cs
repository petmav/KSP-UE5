namespace KSP.UI.Screens.DebugToolbar.Screens.Cheats
{
	public class InfiniteElectricity : DebugScreenToggle
	{
		protected override void SetupValues()
		{
			SetToggle(CheatOptions.InfiniteElectricity);
		}

		protected override void OnToggleChanged(bool state)
		{
			CheatOptions.InfiniteElectricity = state;
		}
	}
}
