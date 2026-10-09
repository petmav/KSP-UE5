using KSP.Localization;
using KSP.UI;
using TMPro;
using UnityEngine;

[UI_Label]
public class UIPartActionFuelFlowOverlay : UIPartActionItem
{
	[SerializeField]
	private UIButtonToggle flowToggle;

	[SerializeField]
	private TextMeshProUGUI flowText;

	private bool isConsumer;

	private bool isProvider;

	public virtual void Setup(UIPartActionWindow window, Part part, UI_Scene scene)
	{
		SetupItem(window, part, null, scene, null);
		if (FuelFlowOverlay.instance != null)
		{
			isConsumer = FuelFlowOverlay.instance.isConsumer(part);
			isProvider = FuelFlowOverlay.instance.isProvider(part);
			if (isConsumer || isProvider)
			{
				flowToggle.onToggle.AddListener(FlowToggle);
				flowToggle.SetState(part.fuelFlowOverlayEnabled);
				flowText.text = Localizer.Format("#autoLOC_5700004", isConsumer);
			}
		}
	}

	public void FlowToggle()
	{
		part.fuelFlowOverlayEnabled = !part.fuelFlowOverlayEnabled;
		if (part.fuelFlowOverlayEnabled)
		{
			FuelFlowOverlay.instance.SpawnOverlay(part);
		}
		else
		{
			FuelFlowOverlay.instance.ClearOverlay();
		}
	}
}
