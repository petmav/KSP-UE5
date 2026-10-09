namespace KSP.UI.Screens
{
	[AppUI_InputFloat]
	public class AppUIMemberInputFloat : AppUIMemberInput
	{
		public string guiFormat;

		protected override void OnInitialized()
		{
			base.OnInitialized();
			if (_attribs is AppUI_InputFloat appUI_InputFloat)
			{
				guiFormat = (string.IsNullOrEmpty(appUI_InputFloat.guiFormat) ? "" : appUI_InputFloat.guiFormat);
			}
		}

		protected override void RefreshUIInput()
		{
			input.text = KSPUtil.LocalizeNumber(GetValue<float>(), ((AppUI_InputFloat)_attribs).guiFormat);
		}
	}
}
