using KSP.Localization;

namespace KSP.UI.Screens.DebugToolbar.Screens.Cheats
{
	public class StringsShowKeysToggle : DebugScreenToggle
	{
		protected override void SetupValues()
		{
			SetToggle(Localizer.ShowKeysOnScreen);
		}

		protected override void OnToggleChanged(bool state)
		{
			Localizer.ShowKeysOnScreen = state;
		}
	}
}
