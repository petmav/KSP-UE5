using UnityEngine;
using UnityEngine.EventSystems;

namespace KSP.UI.TooltipTypes
{
	[RequireComponent(typeof(UIRadioButton))]
	public class UIRadioButtonTooltip : TooltipController
	{
		public Tooltip_Text tooltipPrefab;

		public string textTrue = "";

		public string textFalse = "";

		private Tooltip_Text spawnedTooltip;

		private UIRadioButton radioButton;

		protected override void Awake()
		{
			radioButton = GetComponent<UIRadioButton>();
			radioButton.onTrue.AddListener(OnTrue);
			radioButton.onFalse.AddListener(OnFalse);
			base.Awake();
		}

		private void OnTrue(PointerEventData data, UIRadioButton.CallType callType)
		{
			if ((bool)spawnedTooltip && !string.IsNullOrEmpty(textTrue))
			{
				spawnedTooltip.label.text = textTrue;
			}
		}

		private void OnFalse(PointerEventData data, UIRadioButton.CallType callType)
		{
			if ((bool)spawnedTooltip && !string.IsNullOrEmpty(textFalse))
			{
				spawnedTooltip.label.text = textFalse;
			}
		}

		public override bool OnTooltipAboutToSpawn()
		{
			if (radioButton.Value && !string.IsNullOrEmpty(textTrue))
			{
				return true;
			}
			if (!radioButton.Value && !string.IsNullOrEmpty(textFalse))
			{
				return true;
			}
			return false;
		}

		public override void OnTooltipSpawned(Tooltip instance)
		{
			spawnedTooltip = (Tooltip_Text)instance;
			if (radioButton.Value && !string.IsNullOrEmpty(textTrue))
			{
				spawnedTooltip.label.text = textTrue;
			}
			else if (!radioButton.Value && !string.IsNullOrEmpty(textFalse))
			{
				spawnedTooltip.label.text = textFalse;
			}
		}

		public override void OnTooltipDespawned(Tooltip instance)
		{
			spawnedTooltip = null;
		}
	}
}
