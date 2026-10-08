using FinePrint.Utilities;
using KSP.Localization;

namespace KSPAchievements
{
	public class CelestialBodyTransfer : ProgressNode
	{
		private CelestialBody body;

		public CelestialBodyTransfer(CelestialBody cb)
			: base("CelestialBodyTransfer", startReached: false)
		{
			body = cb;
			OnDeploy = delegate
			{
				GameEvents.onCrewTransferred.Add(OnCrewTransfer);
			};
			OnStow = delegate
			{
				GameEvents.onCrewTransferred.Remove(OnCrewTransfer);
			};
		}

		private void OnCrewTransfer(GameEvents.HostedFromToAction<ProtoCrewMember, Part> fp)
		{
			if (!(fp.from == null) && !(fp.to == null) && fp.from.missionID != fp.to.missionID && fp.to.vessel.mainBody == body)
			{
				if (!base.IsComplete)
				{
					Complete();
					AwardProgressStandard(Localizer.Format("#autoLOC_296191", body.displayName), ProgressType.CREWTRANSFER, body);
				}
				Achieve();
			}
		}

		protected override void OnLoad(ConfigNode node)
		{
		}

		protected override void OnSave(ConfigNode node)
		{
		}
	}
}
