using KSP.Localization;

namespace KSP.UI.Screens.DebugToolbar.Screens.Cheats
{
	public class StringsOverrideMELock : DebugScreenToggle
	{
		protected override void SetupValues()
		{
			SetToggle(Localizer.OverrideMELock);
		}

		protected override void OnToggleChanged(bool state)
		{
			Localizer.OverrideMELock = state;
			GameEvents.Mission.onLocalizationLockOverriden.Fire();
		}
	}
}
