using FinePrint.Utilities;
using KSP.Localization;

namespace KSPAchievements
{
	public class BaseConstruction : ProgressNode
	{
		private CelestialBody body;

		private VesselRef firstBase;

		public BaseConstruction(CelestialBody cb)
			: base("BaseConstruction", startReached: false)
		{
			body = cb;
			firstBase = new VesselRef();
			OnDeploy = delegate
			{
				GameEvents.onPartCouple.Add(onPartsCoupled);
			};
			OnStow = delegate
			{
				GameEvents.onPartCouple.Remove(onPartsCoupled);
			};
		}

		private void onPartsCoupled(GameEvents.FromToAction<Part, Part> pa)
		{
			if (!pa.from.vessel.isEVA && !pa.to.vessel.isEVA && pa.from.missionID != pa.to.missionID && pa.to.vessel.LandedOrSplashed && pa.to.vessel.mainBody == body)
			{
				CrewSensitiveComplete(pa.from.vessel);
				if (!base.IsComplete)
				{
					firstBase = VesselRef.FromVessel(pa.to.vessel);
					Complete();
					AwardProgressStandard(Localizer.Format("#autoLOC_295188", body.displayName), ProgressType.BASECONSTRUCTION, body);
				}
				Achieve();
			}
		}

		protected override void OnLoad(ConfigNode node)
		{
			if (node.HasNode("base"))
			{
				firstBase.Load(node.GetNode("base"));
			}
		}

		protected override void OnSave(ConfigNode node)
		{
			firstBase.Save(node.AddNode("base"));
		}
	}
}
