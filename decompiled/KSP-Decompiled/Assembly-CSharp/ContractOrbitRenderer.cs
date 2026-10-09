using Contracts;
using KSP.Localization;
using KSP.UI.Screens.Mapview;

public class ContractOrbitRenderer : OrbitTargetRenderer
{
	public Contract contract;

	public static ContractOrbitRenderer Setup(Contract contract, Orbit orbit, bool activedraw = true)
	{
		ContractOrbitRenderer contractOrbitRenderer = OrbitTargetRenderer.Setup<ContractOrbitRenderer>("ContractOrbitRenderer " + contract.MissionSeed, contract.MissionSeed, orbit);
		contractOrbitRenderer.contract = contract;
		return contractOrbitRenderer;
	}

	protected override void UpdateLocals()
	{
		targetVessel = FlightGlobals.ActiveVessel;
		base.UpdateLocals();
		string text = ((contract == null || contract.ContractState != Contract.State.Active) ? XKCDColors.HexFormat.Amber : XKCDColors.HexFormat.Chartreuse);
		startColor = "<color=" + text + ">";
	}

	protected override void apNode_OnUpdateCaption(MapNode n, MapNode.CaptionData data)
	{
		data.Header = Localizer.Format("#autoLOC_277907", startColor, base.orbit.ApA.ToString("N0"), "</color>");
		if (contract != null)
		{
			data.captionLine1 = Localizer.Format("#autoLOC_277910", startColor, contract.Agent.Title, "</color>");
		}
	}

	protected override void peNode_OnUpdateCaption(MapNode n, MapNode.CaptionData data)
	{
		data.Header = Localizer.Format("#autoLOC_277915", startColor, base.orbit.PeA.ToString("N0"), "</color>");
		if (contract != null)
		{
			data.captionLine1 = Localizer.Format("#autoLOC_277918", startColor, contract.Agent.Title, "</color>");
		}
	}

	protected override void ascNode_OnUpdateCaption(MapNode n, MapNode.CaptionData data)
	{
		if (ANDNVisible && activeDraw)
		{
			data.Header = Localizer.Format("#autoLOC_277932", startColor, relativeInclination.ToString("0"), "</color>");
			if (contract != null)
			{
				data.captionLine1 = Localizer.Format("#autoLOC_277935", startColor, contract.Agent.Title, "</color>");
			}
		}
	}

	protected override void descNode_OnUpdateCaption(MapNode n, MapNode.CaptionData data)
	{
		if (ANDNVisible && activeDraw)
		{
			data.Header = Localizer.Format("#autoLOC_277943", startColor, (0.0 - relativeInclination).ToString("0"), "</color>");
			if (contract != null)
			{
				data.captionLine1 = Localizer.Format("#autoLOC_277946", startColor, contract.Agent.Title, "</color>");
			}
		}
	}
}
