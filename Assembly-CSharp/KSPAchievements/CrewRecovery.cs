using FinePrint.Utilities;
using KSP.Localization;

namespace KSPAchievements
{
	public class CrewRecovery : ProgressNode
	{
		private CrewRef firstCrew;

		public CrewRecovery()
			: base("FirstCrewToSurvive", startReached: false)
		{
			firstCrew = new CrewRef();
			OnDeploy = delegate
			{
				GameEvents.onVesselRecovered.Add(onVesselRecovered);
			};
			OnStow = delegate
			{
				GameEvents.onVesselRecovered.Remove(onVesselRecovered);
			};
		}

		private void onVesselRecovered(ProtoVessel pv, bool quick)
		{
			if (pv.situation != Vessel.Situations.PRELAUNCH && pv.GetVesselCrew().Count > 0)
			{
				if (!base.IsComplete)
				{
					firstCrew = CrewRef.FromProtoVessel(pv);
					Complete();
					AwardProgressStandard(Localizer.Format("#autoLOC_296238"), ProgressType.CREWRECOVERY);
				}
				Achieve();
			}
		}

		protected override void OnLoad(ConfigNode node)
		{
			if (node.HasNode("crew"))
			{
				firstCrew.Load(node.GetNode("crew"));
			}
		}

		protected override void OnSave(ConfigNode node)
		{
			firstCrew.Save(node.AddNode("crew"));
		}
	}
}
