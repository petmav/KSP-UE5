using FinePrint.Utilities;
using KSP.Localization;

namespace KSPAchievements
{
	public class Docking : ProgressNode
	{
		private CelestialBody body;

		public Docking(CelestialBody cb)
			: base("Docking", startReached: false)
		{
			body = cb;
			OnDeploy = delegate
			{
				GameEvents.onPartCouple.Add(OnPartsCoupled);
			};
			OnStow = delegate
			{
				GameEvents.onPartCouple.Remove(OnPartsCoupled);
			};
		}

		private void OnPartsCoupled(GameEvents.FromToAction<Part, Part> pa)
		{
			if (!pa.from.vessel.isEVA && !pa.to.vessel.isEVA && pa.from.missionID != pa.to.missionID && pa.from.vessel.mainBody == body)
			{
				CrewSensitiveComplete(pa.from.vessel);
				if (!base.IsComplete)
				{
					Complete();
					AwardProgressStandard(Localizer.Format("#autoLOC_296294", body.displayName), ProgressType.DOCKING, body);
					AnalyticsUtil.LogVesselDocked(pa.from.vessel);
				}
				Achieve();
			}
		}
	}
}
