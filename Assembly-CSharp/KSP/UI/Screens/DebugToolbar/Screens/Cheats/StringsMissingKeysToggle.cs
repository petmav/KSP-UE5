using KSP.Localization;

namespace KSP.UI.Screens.DebugToolbar.Screens.Cheats
{
	public class StringsMissingKeysToggle : DebugScreenToggle
	{
		protected override void SetupValues()
		{
			SetToggle(Localizer.debugWriteMissingKeysToLog);
		}

		protected override void OnToggleChanged(bool state)
		{
			Localizer.debugWriteMissingKeysToLog = state;
		}
	}
}
