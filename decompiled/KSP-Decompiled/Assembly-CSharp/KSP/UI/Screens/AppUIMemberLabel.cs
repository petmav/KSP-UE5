using TMPro;

namespace KSP.UI.Screens
{
	[AppUI_Label]
	public class AppUIMemberLabel : AppUIMember
	{
		public TextMeshProUGUI value;

		public AppUI_Control.HorizontalAlignment horizontalAlignment = AppUI_Control.HorizontalAlignment.Right;

		protected override void OnInitialized()
		{
			if (_attribs is AppUI_Label appUI_Label)
			{
				horizontalAlignment = appUI_Label.guiNameHorizAlignment;
			}
		}

		protected override void OnRefreshUI()
		{
			value.text = GetValue().ToString();
			switch (horizontalAlignment)
			{
			case AppUI_Control.HorizontalAlignment.Left:
				value.alignment = TextAlignmentOptions.CaplineLeft;
				break;
			case AppUI_Control.HorizontalAlignment.Middle:
				value.alignment = TextAlignmentOptions.Capline;
				break;
			case AppUI_Control.HorizontalAlignment.Right:
				value.alignment = TextAlignmentOptions.CaplineRight;
				break;
			case AppUI_Control.HorizontalAlignment.None:
				break;
			}
		}
	}
}
