using System;
using FinePrint.Utilities;
using KSP.Localization;

namespace Contracts.Parameters
{
	[Serializable]
	public class ReachSituation : ContractParameter
	{
		public Vessel.Situations Situation;

		protected string title = "";

		public ReachSituation()
		{
		}

		public ReachSituation(Vessel.Situations sit, string title)
		{
			Situation = sit;
			this.title = title;
		}

		protected override string GetTitle()
		{
			return title + GetTitleStringShort(Situation);
		}

		public static string GetTitleStringShort(Vessel.Situations sit)
		{
			switch (sit)
			{
			case Vessel.Situations.FLYING:
				return Localizer.Format("#autoLOC_268856");
			case Vessel.Situations.LANDED:
				return Localizer.Format("#autoLOC_268855");
			case Vessel.Situations.SPLASHED:
				return Localizer.Format("#autoLOC_268858");
			case Vessel.Situations.PRELAUNCH:
				return Localizer.Format("#autoLOC_268854");
			default:
				return Localizer.Format("#autoLOC_268861");
			case Vessel.Situations.ESCAPING:
				return Localizer.Format("#autoLOC_268860");
			case Vessel.Situations.ORBITING:
				return Localizer.Format("#autoLOC_268857");
			case Vessel.Situations.SUB_ORBITAL:
				return Localizer.Format("#autoLOC_268859");
			}
		}

		protected override void OnLoad(ConfigNode node)
		{
			if (node.HasValue("sit"))
			{
				Situation = (Vessel.Situations)Enum.Parse(typeof(Vessel.Situations), node.GetValue("sit"));
			}
			if (node.HasValue("title"))
			{
				title = node.GetValue("title");
			}
		}

		protected override void OnSave(ConfigNode node)
		{
			node.AddValue("sit", Situation.ToString());
			node.AddValue("title", title);
		}

		protected override void OnRegister()
		{
			GameEvents.onVesselSituationChange.Add(OnVesselSituationChange);
		}

		protected override void OnUnregister()
		{
			GameEvents.onVesselSituationChange.Remove(OnVesselSituationChange);
		}

		protected override void OnReset()
		{
			if (SystemUtilities.FlightIsReady(checkVessel: true))
			{
				TrackVessel(FlightGlobals.ActiveVessel);
			}
			else
			{
				SetIncomplete();
			}
		}

		private void OnVesselSituationChange(GameEvents.HostedFromToAction<Vessel, Vessel.Situations> v)
		{
			if (v.host == FlightGlobals.ActiveVessel)
			{
				TrackVessel(v.host);
			}
		}

		private void TrackVessel(Vessel v)
		{
			if (base.State == ParameterState.Incomplete && checkVesselSituation(v))
			{
				SetComplete();
			}
			else if (base.State == ParameterState.Complete && !checkVesselSituation(v))
			{
				SetIncomplete();
			}
		}

		public bool checkVesselSituation(Vessel v)
		{
			if (Situation == Vessel.Situations.LANDED)
			{
				if (v.situation != Vessel.Situations.LANDED)
				{
					return v.situation == Vessel.Situations.PRELAUNCH;
				}
				return true;
			}
			if (Situation == Vessel.Situations.PRELAUNCH)
			{
				if (v.situation != Vessel.Situations.PRELAUNCH)
				{
					if (v.situation == Vessel.Situations.LANDED)
					{
						if (!v.landedAt.Contains("Runway"))
						{
							return v.landedAt.Contains("LaunchPad");
						}
						return true;
					}
					return false;
				}
				return true;
			}
			return v.situation == Situation;
		}
	}
}
