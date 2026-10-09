using FinePrint.Utilities;
using KSP.Localization;

namespace KSPAchievements
{
	public class Spacewalk : ProgressNode
	{
		private CelestialBody body;

		private CrewRef firstKerbal;

		public Spacewalk(CelestialBody cb)
			: base("Spacewalk", startReached: false)
		{
			body = cb;
			firstKerbal = new CrewRef();
			OnDeploy = delegate
			{
				GameEvents.onCrewOnEva.Add(OnGoEVA);
			};
			OnStow = delegate
			{
				GameEvents.onCrewOnEva.Remove(OnGoEVA);
			};
		}

		private void OnGoEVA(GameEvents.FromToAction<Part, Part> fv)
		{
			if ((fv.from.vessel.situation == Vessel.Situations.ORBITING || fv.from.vessel.situation == Vessel.Situations.SUB_ORBITAL) && fv.from.vessel.mainBody == body)
			{
				if (!base.IsComplete)
				{
					firstKerbal = CrewRef.FromVessel(fv.to.vessel);
					Complete();
					AwardProgressStandard(Localizer.Format("#autoLOC_298311", body.displayName), ProgressType.SPACEWALK, body);
				}
				Achieve();
			}
		}

		protected override void OnLoad(ConfigNode node)
		{
			if (node.HasNode("crew"))
			{
				firstKerbal.Load(node.GetNode("crew"));
			}
		}

		protected override void OnSave(ConfigNode node)
		{
			if (firstKerbal.HasAny)
			{
				firstKerbal.Save(node.AddNode("crew"));
			}
		}
	}
}
