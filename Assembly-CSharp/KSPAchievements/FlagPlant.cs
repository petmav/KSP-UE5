using FinePrint.Utilities;
using KSP.Localization;

namespace KSPAchievements
{
	public class FlagPlant : ProgressNode
	{
		private CelestialBody body;

		private CrewRef firstKerbal;

		public FlagPlant(CelestialBody cb)
			: base("FlagPlant", startReached: false)
		{
			body = cb;
			firstKerbal = new CrewRef();
			OnDeploy = delegate
			{
				GameEvents.onFlagPlant.Add(OnFlagPlant);
			};
			OnStow = delegate
			{
				GameEvents.onFlagPlant.Remove(OnFlagPlant);
			};
		}

		private void OnFlagPlant(Vessel v)
		{
			if (v.orbit.referenceBody == body)
			{
				if (!base.IsComplete)
				{
					firstKerbal = CrewRef.FromVessel(v);
					Complete();
					AwardProgressStandard((body == FlightGlobals.GetHomeBody()) ? Localizer.Format("#autoLOC_6001953") : Localizer.Format("#autoLOC_6001954", body.displayName), ProgressType.FLAGPLANT, body);
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
