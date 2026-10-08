using FinePrint.Utilities;
using KSP.Localization;

namespace KSPAchievements
{
	public class CelestialBodyEscape : ProgressNode
	{
		private CelestialBody body;

		public CelestialBodyEscape(CelestialBody cb)
			: base("Escape", startReached: false)
		{
			body = cb;
			OnDeploy = delegate
			{
				GameEvents.onVesselSOIChanged.Add(OnSOIEscaped);
			};
			OnStow = delegate
			{
				GameEvents.onVesselSOIChanged.Remove(OnSOIEscaped);
			};
		}

		private void OnSOIEscaped(GameEvents.HostedFromToAction<Vessel, CelestialBody> vcs)
		{
			if (vcs.host.DiscoveryInfo.Level == DiscoveryLevels.Owned && vcs.from == body)
			{
				CrewSensitiveComplete(vcs.host);
				if (!base.IsComplete)
				{
					Complete();
					AwardProgressStandard(Localizer.Format("#autoLOC_295242", body.displayName), ProgressType.ESCAPE, body);
					AnalyticsUtil.LogCelestialBodyProgress(body, AnalyticsUtil.cbProgressTypes.escaped);
				}
				Achieve();
				GameEvents.VesselSituation.onEscape.Fire(vcs.host, body);
			}
		}
	}
}
