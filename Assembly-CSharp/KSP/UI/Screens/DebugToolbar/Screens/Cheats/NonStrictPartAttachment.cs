namespace KSP.UI.Screens.DebugToolbar.Screens.Cheats
{
	public class NonStrictPartAttachment : DebugScreenToggle
	{
		protected override void SetupValues()
		{
			SetToggle(CheatOptions.NonStrictAttachmentOrientation);
		}

		protected override void OnToggleChanged(bool state)
		{
			CheatOptions.NonStrictAttachmentOrientation = state;
		}
	}
}
