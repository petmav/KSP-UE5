using System.Collections;
using FinePrint.Utilities;
using KSP.Localization;

namespace KSPAchievements
{
	public class FirstLaunch : ProgressNode
	{
		public FirstLaunch()
			: base("FirstLaunch", startReached: false)
		{
			OnDeploy = delegate
			{
				GameEvents.onVesselSituationChange.Add(OnVesselSituationChange);
			};
			OnStow = delegate
			{
				GameEvents.onVesselSituationChange.Remove(OnVesselSituationChange);
			};
		}

		private void OnVesselSituationChange(GameEvents.HostedFromToAction<Vessel, Vessel.Situations> vs)
		{
			if (!vs.host.isEVA && vs.host.IsControllable && vs.to == Vessel.Situations.FLYING && (vs.from == Vessel.Situations.PRELAUNCH || (vs.from == Vessel.Situations.LANDED && vs.host.landedAtLast != null && (vs.host.landedAtLast.Contains("Runway") || vs.host.landedAtLast.Contains("LaunchPad")))))
			{
				vs.host.StartCoroutine(TestFlight(vs.host));
			}
		}

		private IEnumerator TestFlight(Vessel v)
		{
			double endTime = Planetarium.fetch.time + 1.0;
			while (true)
			{
				if (!(Planetarium.fetch.time < endTime))
				{
					CrewSensitiveComplete(v);
					if (!base.IsComplete)
					{
						Complete();
						AwardProgressStandard(Localizer.Format("#autoLOC_296348"), ProgressType.FIRSTLAUNCH);
					}
					Achieve();
					GameEvents.VesselSituation.onLaunch.Fire(v);
					break;
				}
				if (v.situation == Vessel.Situations.FLYING && v.verticalSpeed >= 0.0)
				{
					yield return null;
					continue;
				}
				break;
			}
		}
	}
}
