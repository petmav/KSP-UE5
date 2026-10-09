using TMPro;
using UnityEngine;

namespace KSP.UI.Screens
{
	[AppUI_Heading]
	public class AppUIMemberHeading : AppUIMember
	{
		public AppUI_Control.HorizontalAlignment horizontalAlignment = AppUI_Control.HorizontalAlignment.Left;

		public bool reverseLayoutGroupPadding;

		[SerializeField]
		private RectTransform headingBackground;

		private Vector2 backgroundOffsetMin;

		private Vector2 backgroundOffsetMax;

		protected override void OnInitialized()
		{
			if (_attribs is AppUI_Heading appUI_Heading)
			{
				horizontalAlignment = appUI_Heading.textAlignment;
				reverseLayoutGroupPadding = appUI_Heading.reverseLayoutGroupPadding;
			}
			AdjustHeadingBackground();
		}

		protected override void OnRefreshUI()
		{
			object value = GetValue();
			if (value != null && !string.IsNullOrWhiteSpace(value.ToString()))
			{
				guiNameLabel.text = value.ToString();
			}
			else
			{
				guiNameLabel.text = base.guiName;
			}
			switch (horizontalAlignment)
			{
			case AppUI_Control.HorizontalAlignment.Left:
				guiNameLabel.alignment = TextAlignmentOptions.CaplineLeft;
				break;
			case AppUI_Control.HorizontalAlignment.Middle:
				guiNameLabel.alignment = TextAlignmentOptions.Capline;
				break;
			case AppUI_Control.HorizontalAlignment.Right:
				guiNameLabel.alignment = TextAlignmentOptions.CaplineRight;
				break;
			}
			AdjustHeadingBackground();
		}

		private void AdjustHeadingBackground()
		{
			if (parentLayoutGroup != null)
			{
				backgroundOffsetMin = new Vector2(-parentLayoutGroup.padding.left, 0f);
				backgroundOffsetMax = new Vector2(-parentLayoutGroup.padding.right, 0f);
			}
			if (headingBackground != null && reverseLayoutGroupPadding)
			{
				headingBackground.offsetMin = backgroundOffsetMin;
				headingBackground.offsetMax = backgroundOffsetMax;
			}
		}
	}
}
