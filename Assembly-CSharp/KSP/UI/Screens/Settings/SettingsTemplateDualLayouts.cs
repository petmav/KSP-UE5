using UnityEngine;

namespace KSP.UI.Screens.Settings
{
	public class SettingsTemplateDualLayouts : SettingsTemplate
	{
		public RectTransform layout2;

		private bool flop;

		public RectTransform GetLayoutFlipFlop()
		{
			if (flop)
			{
				flop = !flop;
				return layout2;
			}
			flop = !flop;
			return layout;
		}
	}
}
