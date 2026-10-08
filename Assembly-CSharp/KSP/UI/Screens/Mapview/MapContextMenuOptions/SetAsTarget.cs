using System;
using KSP.Localization;

namespace KSP.UI.Screens.Mapview.MapContextMenuOptions
{
	public class SetAsTarget : MapContextMenuOption
	{
		private readonly Func<ITargetable> getOTTarget;

		private ITargetable currTgt;

		private ITargetable tgt;

		public SetAsTarget(ITargetable tgt, Func<ITargetable> getOTTarget)
			: base("Set Target")
		{
			this.getOTTarget = getOTTarget;
			this.tgt = tgt;
		}

		protected override bool OnCheckEnabled(out string fbText)
		{
			currTgt = getOTTarget();
			if (currTgt != tgt)
			{
				fbText = Localizer.Format("#autoLOC_465801");
			}
			else
			{
				fbText = Localizer.Format("#autoLOC_465805");
			}
			return true;
		}

		protected override void OnSelect()
		{
			if (currTgt != tgt)
			{
				FlightGlobals.fetch.SetVesselTarget(tgt);
			}
			else
			{
				FlightGlobals.fetch.SetVesselTarget(null);
			}
		}

		public override bool CheckAvailable()
		{
			if (currTgt == tgt)
			{
				return currTgt != null;
			}
			if (!(tgt.GetOrbitDriver().celestialBody != null))
			{
				return tgt.GetOrbitDriver().vessel.DiscoveryInfo.HaveKnowledgeAbout(DiscoveryLevels.StateVectors);
			}
			return true;
		}
	}
}
