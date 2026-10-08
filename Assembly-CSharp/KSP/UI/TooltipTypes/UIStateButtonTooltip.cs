using UnityEngine;

namespace KSP.UI.TooltipTypes
{
	[RequireComponent(typeof(UIStateButton))]
	public class UIStateButtonTooltip : TooltipController
	{
		public Tooltip_Text tooltipPrefab;

		public ButtonStateTooltip[] tooltipStates = new ButtonStateTooltip[0];

		private Tooltip_Text spawnedTooltip;

		private UIStateButton stateButton;

		private ButtonStateTooltip currentTooltipState;

		protected override void Awake()
		{
			stateButton = GetComponent<UIStateButton>();
			stateButton.onValueChanged.AddListener(OnValueChanged);
			currentTooltipState = FindButtonStateTooltip(stateButton.currentState);
			base.Awake();
		}

		private ButtonStateTooltip FindButtonStateTooltip(string state)
		{
			int num = tooltipStates.Length;
			do
			{
				if (num-- <= 0)
				{
					return null;
				}
			}
			while (!(tooltipStates[num].name == state));
			return tooltipStates[num];
		}

		private void OnValueChanged(UIStateButton button)
		{
			currentTooltipState = FindButtonStateTooltip(button.currentState);
			if (spawnedTooltip != null && !string.IsNullOrEmpty(currentTooltipState.tooltipText))
			{
				spawnedTooltip.label.text = currentTooltipState.tooltipText;
			}
		}

		public override bool OnTooltipAboutToSpawn()
		{
			if (currentTooltipState == null)
			{
				return false;
			}
			if (!string.IsNullOrEmpty(currentTooltipState.tooltipText))
			{
				return true;
			}
			return false;
		}

		public override void OnTooltipSpawned(Tooltip instance)
		{
			spawnedTooltip = (Tooltip_Text)instance;
			spawnedTooltip.label.text = currentTooltipState.tooltipText;
		}

		public override void OnTooltipDespawned(Tooltip instance)
		{
			spawnedTooltip = null;
		}
	}
}
