using KSP.Localization;
using UnityEngine;

namespace KSP.UI.TooltipTypes
{
	public class TooltipController_Text : TooltipController
	{
		public Tooltip_Text prefab;

		[TextArea(1, 2)]
		public string textString = "No text";

		public bool continuousUpdate;

		protected Tooltip_Text tooltipInstance;

		protected override void Awake()
		{
			base.Awake();
		}

		public override bool OnTooltipAboutToSpawn()
		{
			if (!string.IsNullOrEmpty(textString))
			{
				textString = Localizer.Format(textString);
			}
			return !string.IsNullOrEmpty(textString);
		}

		public override void OnTooltipSpawned(Tooltip instance)
		{
			tooltipInstance = instance as Tooltip_Text;
			tooltipInstance.label.text = textString;
		}

		public override bool OnTooltipUpdate(Tooltip instance)
		{
			if (continuousUpdate && tooltipInstance != null && tooltipInstance.label.text != textString)
			{
				tooltipInstance.label.text = textString;
			}
			return OnTooltipAboutToSpawn();
		}

		public override void OnTooltipDespawned(Tooltip instance)
		{
			tooltipInstance = null;
		}

		public virtual void SetText(string textString)
		{
			this.textString = textString;
			if (!continuousUpdate && tooltipInstance != null && tooltipInstance.label.text != textString)
			{
				tooltipInstance.label.text = textString;
			}
		}
	}
}
