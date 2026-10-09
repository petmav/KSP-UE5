using KSP.Localization;
using KSP.UI.TooltipTypes;
using TMPro;

namespace KSP.UI.Screens
{
	public class EditorActionOverrideToggle : UISelectableGridLayoutGroupItem
	{
		public delegate void SetOverrideStateDelegate(int groupOverride, bool state);

		public UIButtonToggle toggle;

		public TextMeshProUGUI text;

		public SetOverrideStateDelegate SetOverrideState;

		public int groupOverride { get; private set; }

		public void Setup(int groupOverride, bool on, bool isControl, string tooltipText, bool isGroupAction, int setIndex)
		{
			if (isControl)
			{
				text.text = Localizer.Format("#autoLOC_6013017");
			}
			else
			{
				text.text = Localizer.Format("#autoLOC_6013002");
			}
			this.groupOverride = groupOverride;
			toggle.onToggle.AddListener(OnToggle);
			toggle.state = on;
			GetComponent<TooltipController_Text>().textString = Localizer.Format(tooltipText);
			groupAction = isGroupAction;
			mySetIndex = setIndex;
		}

		private void OnToggle()
		{
			if (SetOverrideState != null)
			{
				SetOverrideState(groupOverride, toggle.state);
			}
		}

		private void OnValueChanged(bool on)
		{
			if (SetOverrideState != null)
			{
				SetOverrideState(groupOverride, on);
			}
		}
	}
}
